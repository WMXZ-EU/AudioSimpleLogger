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

#ifndef RECORD_LOGGER_H
#define RECORD_LOGGER_H

#include "Arduino.h"
#include "AudioStream.h"

#define INF (-1)
#include "mfs.h"

class uSD_Logger
{
  public:
  void init(void);
  uint32_t save(char *fmt, int mxfn, int mxblk);
  uint32_t overrun=0;

  private:
  virtual uint8_t *drain(uint16_t *nbuf) =0;
  uint16_t fileStatus = INF;
  uint32_t ifn = 0;
  uint32_t loggerCount = 0;
  //
  c_mFS mFS;
};

template <int nc, int nq, int na>
class AudioRecordLogger : public AudioStream, public uSD_Logger
{
public:
  AudioRecordLogger (void) : AudioStream(nq, inputQueueArray),
    head(0), tail(0), enabled(0), uSD_Logger(){ }

  void begin(void) { clear(); enabled = 1; }
  void end(void) { enabled = 0; }

  void clear(void)
  {
    uint32_t t = tail;
    while (t != head) {
      if (++t >= nq) t = 0;
      for(int ii=0; ii<nq;ii++) release(queue[ii][t]);
    }
    tail = t;
  }

  void update(void) 
  {
    audio_block_t *block[nq];
    uint32_t h;

    for(int ii=0; ii<nc;ii++) block[ii] = receiveReadOnly(ii);
    for(int ii=0; ii<nc;ii++) if (!block[ii]) return;
    if (!enabled) {
      for(int ii=0; ii<nc;ii++) release(block[ii]);
      return;
    }
    
    h = head + 1;
    if (h >= nq) h = 0;
    if (h == tail) {  // disaster
      overrun++;
      for(int ii=0; ii<nc;ii++) release(block[ii]);
    } else {
      for(int ii=0; ii<nc;ii++) queue[ii][h] = block[ii];
      head = h;
    }
  }

  uint8_t *drain(uint16_t *nbuf)
  {
    uint32_t n;
    if(head>tail) n=head-tail; else n = nq + head -tail;

    int16_t nb=na;
    if(n>nb)
    {
      int16_t *bptr = buffer;
      *nbuf = na*nc*AUDIO_BLOCK_SAMPLES*2;
      //
      uint32_t t = tail;
      while(--nb>=0)
      {
        if (t != head) 
        {
          if (++t >= nq) t = 0;
          
          // copy to buffer     
          for(int ii=0; ii<nc; ii++) 
          { if(queue[ii][t])
            {
              for(int jj=0; jj<AUDIO_BLOCK_SAMPLES; jj++) 
                  bptr[jj*nc+ii]=queue[ii][t]->data[jj];
              release(queue[ii][t]);
            }
          }
          tail = t;
          bptr += nc*AUDIO_BLOCK_SAMPLES;
        }
      }
      return (uint8_t *)buffer;
    }
    return 0;
  }

private:
  audio_block_t *inputQueueArray[nq];
  audio_block_t * volatile queue[nc][nq];
  volatile uint8_t head, tail, enabled;

  int16_t buffer[na*nc*AUDIO_BLOCK_SAMPLES];

};

void uSD_Logger::init(void)
  {
    mFS.init();
    fileStatus=0;
  }

uint32_t uSD_Logger::save(char *fmt, int mxfn, int mxblk )
{ // does also open/close a file when required
  //
  static uint16_t isLogging = 0; // flag to ensure single access to function

  char filename[80];

  if (isLogging) return 0; // we are already busy (should not happen)
  isLogging = 1;

  if(fileStatus==4) { isLogging = 0; return 1; } // don't do anything anymore

  if(fileStatus==0)
  {
    // open new file
    ifn++;
    if (ifn > mxfn) // have end of acquisition reached, so end operation
    { fileStatus = 4;
      isLogging = 0; return INF; // tell calling loop() to stop ACQ
    } // end of all operations

    sprintf(filename, fmt, (unsigned int)ifn);
    mFS.open(filename);
#if DO_DEBUG >0
    Serial.printf(" %s\n\r",filename);
#endif    
    loggerCount=0;  // count successful transfers
    overrun=0;      // count buffer overruns
    //
    fileStatus = 2; // flag as open
    isLogging = 0; return 1;
  }


  if(fileStatus==2)
  { uint16_t nbuf;
    uint8_t *buffer=drain(&nbuf);
    if(buffer)
    {
      if (!mFS.write(buffer, nbuf))
      { fileStatus = 3;} // close file on write failure
      if(fileStatus == 2)
      {
        loggerCount++;
        if(loggerCount == mxblk)
        { fileStatus= 3;}
#if DO_DEBUG == 2
        else
        {
          if (!(loggerCount % 10)) Serial.printf(".");
          if (!(loggerCount % 640)) {Serial.println(); }
          Serial.flush();
        }
#endif
      }
    }
  }
  if(fileStatus==2){ isLogging = 0; return 1; }

  if(fileStatus==3)
  {
    //close file
    mFS.close();
#if DO_DEBUG ==2
    Serial.printf("\n\r(%d,%d)\n\r",overrun,AudioMemoryUsageMax());
    AudioMemoryUsageMaxReset(); 
#endif    
    //
    fileStatus= 0; // flag file as closed   
    isLogging = 0; return 1;
  }
  
  isLogging=0; return 0;
}

#endif
