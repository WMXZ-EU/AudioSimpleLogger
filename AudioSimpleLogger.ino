/* wmxzAudio Library for Teensy 3.X
 * Copyright (c) 2017, Walter Zimmer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
// the following includes are explicit to avoid inclusion of "SD.h"
// WMXZ: modified 30-09-2019
//
#include <kinetis.h>
#include <core_pins.h>
#include <usb_serial.h>


/*************** change if required *****************************/
// set DO_DEBUG
// to 0 for no printout
// to 1 to print only filename
// to 2 to print also progress and number of audio blocks used
// the last can be useful to select good uSD card and tune queue length (NQ below)
#define DO_DEBUG 2

// some definitions
// for AudioRecordLogger
#define NCH 2   // number of channels can be 1, 2, 4

#if !((NCH==1) || (NCH==2) || (NCH==4))
	#error worong number of channels
#endif
// 
#define NQ  (600/NCH) // number of elements in queue
// NCH*NQ should be <600 (for about 200 kB RAM usage on T3.6) 

// for uSD_Logger
#define FMT "A%04d.bin"
#define MXFN 10     // max number of files
#define MAX_MB 40   // max (expected) file size in MB

// for I2S
#define I2S_RX_ONLY 0 // set to 0 for PJRC Audio board; set to 1 if using pin 11,12 for RX_BCLK, RX_FS (e.g. for MEMS)

// to use USB needs USB_AUDIO selected
#define USE_USB 0

//-------------------- Connections ------------------------
// 01-sep-2017: 
// for ICS4343x microphones
// T3.2a  T3.6a   T3.2  T3.6            Mic1  Mic2  Mic3  Mic4   
// GND    GND     GND   GND             GND   GND   GND   GND    
// 3.3V   3.3V    3.3V  3.3V            VCC   VCC   VCC   VCC    
// Pin11  Pin11   Pin9  Pin9    BCLK    CLK   CLK   CLK   CLK 
// Pin12  Pin12   Pin23 Pin23   FS      WS    WS    WS    WS  
// Pin13  Pin13   Pin13 Pin13   RXD0    SD    SD    --    --  
// Pin30  Pin38   Pin30 Pin38   RXD1    --    --    SD    SD  
// GND    GND     GND   GND             L/R   --    L/R   --
// 3.3    3.3     3.3   3.3             --    L/R   --    L/R
//
// T3.xa are alternative pin settings for pure RX mode (input only)

// PJRC Audio uses for I2S MCLK and therefore uses RX internally sync'ed to TX
// that is, TX_BCLK and TX_FS are sending out data but receive data on RDX0, RDX1
//
// the modified (RX_ONLY) implementation does not use MCLK and therefore can use
// RX_BCLK and RX_FS
//
// PJRC pin selection
// Pin11 MCLK
// Pin9  TX_BCLK
// Pin23 TX_FS
//
// RX_ONLY pin selection
// Pin11  RX_BCLK
// Pin12  RX_FS
 
// the following function patches the I2S driver and may have side-effects
// not needed if only PJRC I2S pin selection is used
//


/*************** end of possible changes ************************/
#include "record_logger.h"

#if I2S_RX_ONLY == 0
	#if NCH<=2
		#include "AudioInputI2S.h"
	  AudioInputI2S                i2s1; 
	#elif NCH==4
		#include "AudioInputI2SQuad.h"
	  AudioInputI2SQuad            i2s1; 
	#endif

#else
	static void is2_switchRxOnly(int on)
	{
	  if(on)
	  { //switch rx async, tx sync'd to rx
		I2S0_TCR2 |= I2S_TCR2_SYNC(1);
		I2S0_RCR2 |= I2S_RCR2_SYNC(0);
		CORE_PIN11_CONFIG = PORT_PCR_MUX(4); // pin 11, PTC6, I2S0_RX_BCLK
		CORE_PIN12_CONFIG = PORT_PCR_MUX(4); // pin 12, PTC7, I2S0_RX_FS
		pinMode(23,INPUT);
		pinMode(9,INPUT);
	  }
	  else
	  { //switch tx async, rx sync'd to tx (PJRC mode)
		I2S0_TCR2 |= I2S_TCR2_SYNC(0);
		I2S0_RCR2 |= I2S_RCR2_SYNC(1);
	  // configure pin mux for 3 clock signals (PJRC_AudioAdapter)
		CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
		CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
		CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
		pinMode(12,INPUT);
	  }
	}

	#if NCH<=2
		#include "input_i2s.h"
		class AudioInputI2Sm: public AudioInputI2S 
		{
		public:
		  AudioInputI2Sm(void) : AudioInputI2S() { begin();}
		  void begin(void)
		  { // switch I2S from PJRC mode to pure RX mode
			is2_switchRxOnly(I2S_RX_ONLY);
		  }
		};
		AudioInputI2Sm          i2s1; 

	#elif NCH==4
		#include "input_i2s_quad.h"
		class AudioInputI2SQuadm: public AudioInputI2SQuad 
		{
		public:
		  AudioInputI2SQuadm(void) : AudioInputI2SQuad() { begin();}
		  
		  void begin(void)
		  { // switch I2S from PJRC mode to pure RX mode
			is2_switchRxOnly(I2S_RX_ONLY);
		  }
		};
		AudioInputI2SQuadm      i2s1; 
	#endif
#endif

#if USE_USB ==1
	#include "usb_audio.h"
	AudioOutputUSB                  usb1;  
#endif
// 
// for uSD_Logger write buffer
// the sequence (64,32,16) is for 16kB write buffer
#define NAUD (64/NCH)
AudioRecordLogger<NCH, NQ, NAUD> 		logger1; 

// connecting all audio objects
#if USE_USB ==1
	AudioConnection          patchCord1(i2s1, 0, usb1, 0);
	AudioConnection          patchCord2(i2s1, 1, usb1, 1);
#endif
AudioConnection          patchCord3(i2s1, 0, logger1, 0);

#if NCH>1
  AudioConnection        patchCord4(i2s1, 1, logger1, 1);
#endif
#if NCH==4
  AudioConnection        patchCord5(i2s1, 2, logger1, 2);
  AudioConnection        patchCord6(i2s1, 3, logger1, 3);
#endif

/************************ Main Sketch *********************************/
void setup() {
  // put your setup code here, to run once:
  AudioMemory(50+NCH*NQ);
  
  //
  while(!Serial);
  Serial.println("Simple Logger");

  logger1.init();
  
  logger1.begin();

}

void loop() {
  // put your main code here, to run repeatedly:

//  if(logger1.save(FMT,MXFN,MAX_MB)==INF)
  if(logger1.save(MAX_MB)==INF) { logger1.end(); }
  return;
}

//Auxillary functions

#include <time.h>

struct tm seconds2tm(uint32_t tt);
uint16_t generateFilename(char *filename)
{
  struct tm tx=seconds2tm(RTC_TSR);
  sprintf(filename,"%s_%04d%02d%02d_%02d%02d%02d.bin",
		"WMXZ",
        tx.tm_year, tx.tm_mon, tx.tm_mday,
        tx.tm_hour, tx.tm_min, tx.tm_sec);
  return 1;
}



