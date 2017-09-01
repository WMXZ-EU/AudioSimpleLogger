
# AudioSimpleLogger
Simple Audio Logger for I2S devices

To Run, edit the following lines

- // some definitions
- // for AudioRecordLogger
- #define NCH 4   // number of channels can be 1, 2, 4
- #define NQ  (600/NCH) // number of elements in queue
- // NCH*NQ should be <600 (for about 200 kB RAM usage) 
- 
- // for uSD_Logger
- #define FMT "A%04d.bin"
- #define MXFN 10     // max number fo files
- #define MAX_MB 40   // max (expected) file size in MB

- // for I2S
- #define I2S_RX_ONLY 1 // set to 1 if using pin 11,12 for RX_BCLK, RX_FS; set to 0 for PJRC Audio board

# Files used
- AudioSimpleLogger.ino
- record_logger.h
- mfs.h

# Last Version 01-09-2017 08:00 Local
