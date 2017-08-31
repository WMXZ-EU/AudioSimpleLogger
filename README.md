
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

# Last Version 31-08-2017 16:00 UTC
