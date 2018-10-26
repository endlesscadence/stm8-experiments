#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "serial_comm.h"

// BSL command codes
#define GET     0x00      // gets version and commands supported by the BSL
#define READ    0x11      // read up to 256 bytes of memory 
#define ERASE   0x43      // erase flash program memory/data EEPROM sectors
#define WRITE   0x31      // write up to 128 bytes to RAM or flash
#define GO      0x21      // jump to a specified address e.g. flash

// BSL return codes
#define SYNCH   0x7F      // Synchronization byte
#define ACK     0x79      // Acknowledge
#define NACK    0x1F      // No acknowledge
#define BUSY    0xAA      // Busy flag status

#define PFLASH_START      0x8000    // starting address of flash (same for all STM8 devices)
#define PFLASH_BLOCKSIZE  1024      // size of flash block for erase or block write (same for all STM8 devices)

/// synchronize to microcontroller BSL
uint8_t bsl_sync(HANDLE ptrPort);

/// read from microcontroller memory
uint8_t bsl_memRead(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf);

/// check if address exists
uint8_t bsl_memCheck(HANDLE ptrPort, uint32_t addr);

/// mass erase microcontroller P- and D-flash
uint8_t bsl_flashMassErase(HANDLE ptrPort);

/// upload to microcontroller flash or RAM
uint8_t bsl_memWrite(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf, int verbose);

/// jump to flash or RAM
uint8_t bsl_jumpTo(HANDLE ptrPort, uint32_t addr);

#endif
