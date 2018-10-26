#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <windows.h>
#include <malloc.h>

#include "serial_comm.h"
#include "bootloader.h"
#include "hexfile.h"

#include "E_W_ROUTINEs_32K_ver_1.3.h"

// buffer sizes
#define  STRLEN   1000
#define  BUFSIZE  10000000


// configuration
#define COM_PORT 	"COM6"
#define HEX_FILE 	"test_hex/main.ihx"
#define ERASE_ALL	1
#define VERIFY 		0


int main(int argc, char ** argv) {
  char      portname[STRLEN];     // name of communication port
  HANDLE    ptrPort;              // handle to communication port
  int       baudrate;             // communication baudrate [Baud]
  uint8_t   flashErase;           // erase P-flash and D-flash prior to upload
  uint8_t   verifyUpload;         // verify memory after upload
  char      *ptr=NULL;            // pointer to memory
  int       i, j;                 // generic variables
  char      buf[1000];            // misc buffer

  // for upload to flash
  char      fileIn[STRLEN];       // name of file to upload to STM8
  char      *fileBufIn;           // buffer for hexfiles
  char      *imageIn;             // memory buffer for upload hexfile
  uint32_t  imageInStart;         // starting address of imageIn
  uint32_t  imageInBytes;         // number of bytes in imageIn

  // for download from flash
  char      fileOut[STRLEN];      // name of file to download from STM8
  char      *imageOut;            // memory buffer for download hexfile
  uint32_t  imageOutStart;        // starting address of imageOut
  uint32_t  imageOutBytes;        // number of bytes in imageOut

  // allocate buffers (can't be static for large buffers)
  imageIn   = (char*) malloc(BUFSIZE);
  imageOut  = (char*) malloc(BUFSIZE);
  fileBufIn = (char*) malloc(BUFSIZE);

  // initialize default arguments
  baudrate   = 230400;            // default baudrate
  flashErase = ERASE_ALL;                 // erase P-flash and D-flash prior to upload
  verifyUpload = VERIFY;               // verify memory content after upload
  memmove(portname, COM_PORT, sizeof(portname));
  memmove(fileIn, HEX_FILE, sizeof(fileIn));

  if (strlen(fileIn) > 0) {
    // convert to memory image, support .hex and .ihx
    fflush(stdout);
    load_hexfile(fileIn, fileBufIn, BUFSIZE);
    convert_hex(fileBufIn, &imageInStart, &imageInBytes, imageIn);
  }

  //reset STM8
  ptrPort = init_port(portname, 9600, 1000, 8, 0, 1, 0, 0);   // use no parity
  printf("  reset via UART command ... ");
  fflush(stdout);
  sprintf(buf, "##reset##");           // reset command (same as in STM8 SW!)
  for (i=0; i<9; i++) {
    send_port(ptrPort, 1, buf+i);   // send reset command bytewise to account for possible slow handling on STM8 side
    Sleep(10);
  }
  fflush(stdout);
  set_baudrate(ptrPort, baudrate);  // restore specified baudrate
  Sleep(20);                        // allow BSL to initialize

  flush_port(ptrPort);

  // communicate with STM8 bootloader

  usleep(200000); // required to make flush work, for some reason
  flush_port(ptrPort);

  // synchronize baudrate
  bsl_sync(ptrPort);

  // upload RAM routines
  ptr = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19;
  ptr[STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19_len]=0;

  char      ramImage[8192];
  uint32_t  ramImageStart;
  uint32_t  numRamBytes;

  convert_s19(ptr, &ramImageStart, &numRamBytes, ramImage);
  fflush(stdout);
  
  bsl_memWrite(ptrPort, ramImageStart, numRamBytes, ramImage, -1);
  fflush(stdout);

  // if flash mass erase
  if (flashErase)
    bsl_flashMassErase(ptrPort);

  // upload file to flash
  if (strlen(fileIn) > 0) {
    // upload memory image to STM8
    bsl_memWrite(ptrPort, imageInStart, imageInBytes, imageIn, 0);

    // verify upload
    if (verifyUpload) {
      bsl_memRead(ptrPort, imageInStart, imageInBytes, imageOut);
      printf("  verify memory ... ");
      for (i=0; i<imageInBytes; i++) {
        if (imageIn[i] != imageOut[i]) {
          printf("\nfailed at address 0x%04x (0x%02x vs 0x%02x), exit!\n", (uint32_t) (imageInStart+i), (uint8_t) (imageIn[i]), (uint8_t) (imageOut[i]));
          exit(1);
        }
      }
      printf("ok\n");
    }

    // enable ROM bootloader after upload (option bytes always on same address)
    fflush(stdout);
    bsl_memWrite(ptrPort, 0x487E, 2, (char*)"\x55\xAA", -1);
    fflush(stdout);
  }

  // jump to application
  fflush(stdout);
  bsl_jumpTo(ptrPort, PFLASH_START);
  fflush(stdout);

  close_port(&ptrPort);
  exit(0);

  return(0);
}

