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
#include <kinetis.h>
#include <core_pins.h>
#include <usb_serial.h>

#define PJRC_AUDIO
#ifdef PJRC_AUDIO
  #include "input_i2s.h"
  #include "input_i2s_quad.h"
#else
  #include "input_i2sm.h"
  #include "input_i2s_quadm.h"
#endif
#include "usb_audio.h"

// set DO_DEBUG
// to 0 for no printout
// to 1 to print only filename
// to 2 to print also progress and number of audio blocks used
// the last can be useful to select good uSD card and tune queue length (NQ below)
#define DO_DEBUG 2
#include "record_logger.h"

// some definitions
// for AudioRecordLogger
#define NCH 2   // number of channels can be 1, 2, 4
#define NQ  (600/NCH) // number of elements in queue
// NCH*NQ should be <600 (for about 200 kB RAM usage) 

// for uSD_Logger
#define FMT "A%04d.bin"
#define MXFN 10     // max number fo files
#define MAX_MB 40   // max (expected) file size in MB

// for uSD_Logger write buffer
// the sequence (64,32,16) is for 16kB write buffer
#if NCH==1
  #define NAUD 64
#elif NCH==2
  #define NAUD 32
#elif NCH==4
  #define NAUD 16
#endif

// 31-aug-2017: 
// for ICS4343x microphones
// T3.2m  T3.6m T3.2  T3.6     Mic1  Mic2  Mic3  Mic4
// GND    GND   GND   GND      GND   GND   GND   GND
// 3.3V   3.3V  3.3V  3.3V     VCC   VCC   VCC   VCC
// Pin12  Pin12 Pin9  Pin9     CLK   CLK   CLK   CLK
// Pin11  Pin11 Pin23 Pin23    WS    WS    WS    WS
// Pin13  Pin13 Pin13 Pin13    SD    SD    --    --
// Pin30  Pin38 Pin30 Pin38    --    --    SD    SD
// GND    GND   GND   GND      L/R   --    L/R   --
// 3.3    3.3   3.3   3.3      --    L/R   --    L/R

// PJRC Audio uses for I2S MCLK and therefore uses RX slaved internally to TX
// that is, TX_BCLK and TX_FS are sending out data but receive data on RDX0, RDX1
// the modified (local) implementation does not use MCLK and therefore can use
// RX_CLK and RX_FS
//
//#define PJRC_AUDIO 
#ifdef PJRC_AUDIO
  #if NCH<=2
    AudioInputI2S                 i2s1; 
  #elif NCH==4
    AudioInputI2SQuad             i2s1; 
  #endif
#else
  #if NCH<=2
    AudioInputI2Sm                 i2s1; 
  #elif NCH==4
    AudioInputI2SQuadm             i2s1; 
  #endif
#endif
AudioOutputUSB                  usb1;  
AudioRecordLogger<NCH,NQ,NAUD>  logger1; 

AudioConnection          patchCord1(i2s1, 0, usb1, 0);
AudioConnection          patchCord2(i2s1, 1, usb1, 1);
AudioConnection          patchCord3(i2s1, 0, logger1, 0);
#if NCH>1
  AudioConnection        patchCord4(i2s1, 1, logger1, 1);
#endif
#if NCH==4
  AudioConnection        patchCord5(i2s1, 2, logger1, 2);
  AudioConnection        patchCord6(i2s1, 3, logger1, 3);
#endif

void setup() {
  // put your setup code here, to run once:
  AudioMemory(50+NCH*NQ);
  while(!Serial);
  Serial.println("Simple Logger");

  logger1.init();
  
  logger1.begin();

}

void loop() {
  // put your main code here, to run repeatedly:

  if(logger1.save(FMT,MXFN,MAX_MB)==INF)
  { logger1.end();
  }
    return;
  
  static uint32_t t0=0;
  if(millis()-t0  > 1000)
  {
    Serial.print("Memory usage : ");
    Serial.println(AudioMemoryUsage());
    t0=millis();
  }
}


