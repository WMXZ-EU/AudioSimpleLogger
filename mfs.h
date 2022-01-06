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
#ifndef MFS_H
#define MFS_H
/************************** File System Interface****************/
#include "SD.h"

// Preallocate 100MB file.
const uint64_t PRE_ALLOCATE_SIZE = 100ULL << 20;

// Use FIFO SDIO or DMA_SDIO
//#define SD_CONFIG SdioConfig(FIFO_SDIO)
#define SD_CONFIG SdioConfig(DMA_SDIO)

//--------------------- For File Time settings ------------------
#include <TimeLib.h>
// Call back for file timestamps.  Only called for file create and sync(). needed by SDFat
void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) 
{       
    tmElements_t tm;
    breakTime(rtc_get(), tm);

    *date = FS_DATE(tmYearToCalendar(tm.Year),tm.Month, tm.Day);
    *time = FS_TIME(tm.Hour, tm.Minute, tm.Second);
    *ms10 = tm.Second & 1 ? 100 : 0;
}

class c_mFS
{
  private:
  SDClass sd;
  FsFile file;
  
  public:
    void init(void)
    {
      if (!sd.sdfs.begin(SD_CONFIG)) halt("begin failed");
      // Set Time callback
      FsDateTime::callback = dateTime;
    }
    
    void open(char * filename)
    {
      if (!file.open(filename, O_CREAT | O_TRUNC |O_RDWR)) { halt("file.open failed"); }
      if (!file.preAllocate(PRE_ALLOCATE_SIZE)) { halt("file.preAllocate failed"); }
    }

    void close(void)
    {
      file.truncate();
      file.close();
    }

    uint32_t write(uint8_t *buffer, uint32_t nbuf)
    { if ((int)nbuf != file.write(buffer, nbuf)) halt("write failed");
      return nbuf;
    }

    uint32_t read(uint8_t *buffer, uint32_t nbuf)
    { if ((int)nbuf != file.read(buffer, nbuf)) halt("read failed");
      return nbuf;
    }
    
  private:
    void halt(char *msg) {sd.sdfs.errorHalt(msg);}
};
#endif
