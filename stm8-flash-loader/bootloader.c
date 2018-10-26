#include "bootloader.h"
#include "serial_comm.h"

/**
  synchronize to microcontroller BSL, e.g. baudrate. If already synchronized
  checks for NACK
*/
uint8_t bsl_sync(HANDLE ptrPort) {

  int   i, count;
  int   lenTx, lenRx, len;
  char  Tx[1000], Rx[1000];

  // init receive buffer
  memset(Rx, 0, 1000);

  if (!ptrPort) {
    //Port not open
    exit(1);
  }

  // purge UART input buffer
  flush_port(ptrPort);

  // construct SYNC command
  lenTx = 1;
  Tx[0] = SYNCH;
  lenRx = 1;

  count = 0;
  do {
    send_port(ptrPort, lenTx, Tx);
    len = receive_port(ptrPort, lenRx, Rx);

    // increase retry counter
    count++;

    // avoid flooding the STM8
    Sleep(10);

  } while ((count < 15) && ((len != lenRx) || ((Rx[0] != ACK) && (Rx[0] != NACK))));

  // check if ok
  if ((len == lenRx) && (Rx[0] == ACK)) {
    printf("ok (ACK)\n");
    fflush(stdout);
  } else if ((len == lenRx) && (Rx[0] == NACK)) {
    printf("ok (NACK)\n");
    fflush(stdout);
  } else if (len == lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_sync()': wrong response 0x%02x from BSL, exit!\n\n", (uint8_t) (Rx[0]));
    exit(1);
  } else {
    fprintf(stderr, "\n\nerror in 'bsl_sync()': no response from BSL, exit!\n\n");
    exit(1);
  }

  // return success
  return(0);
}

/**
  read from microcontroller memory via READ command
*/
uint8_t bsl_memRead(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, idx=0;

  // print message
  printf("  read ");
  fflush(stdout);

  // init receive buffer
  memset(Rx, 0, 1000);

  if (!ptrPort) {
    //port not open
    exit(1);
  }

  // init data buffer
  memset(buf, 0, numBytes);

  // loop over addresses in <=256B steps
  idx = 0;
  addrStep = 256;
  for (addrTmp = addrStart; addrTmp < addrStart + numBytes; addrTmp += addrStep) {
    // if addr too close to end of range reduce stepsize
    if (addrTmp+256 > addrStart+numBytes)
      addrStep = addrStart+numBytes-addrTmp;

    // READ command

    lenTx = 2;
    Tx[0] = READ;
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = 1;

    // send command
    len = send_port(ptrPort, lenTx, Tx);

    if (len != lenTx) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      exit(1);
    }

    // receive response
    len = receive_port(ptrPort, lenRx, Rx);

    if (len != lenRx) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK1 timeout, exit!\n\n");
      exit(1);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK1 failure 0x%2x, exit!\n\n", Rx[0]);
      exit(1);
    }

    // Send address

    // construct address + checksum (XOR over address)
    lenTx = 5;
    Tx[0] = (char) (addrTmp >> 24);
    Tx[1] = (char) (addrTmp >> 16);
    Tx[2] = (char) (addrTmp >> 8);
    Tx[3] = (char) (addrTmp);
    Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
    lenRx = 1;

    // send command
    len = send_port(ptrPort, lenTx, Tx);

    if (len != lenTx) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      exit(1);
    }

    // receive response
    len = receive_port(ptrPort, lenRx, Rx);

    if (len != lenRx) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      exit(1);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK2 failure, exit!\n\n");
      exit(1);
    }

    // Send number of bytes

    // construct number of bytes + checksum
    lenTx = 2;
    Tx[0] = addrStep-1;     // -1 from BSL
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = addrStep + 1;

    // send command
    len = send_port(ptrPort, lenTx, Tx);

    if (len != lenTx) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending range failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      exit(1);
    }

    // receive response
    len = receive_port(ptrPort, lenRx, Rx);

    if (len != lenRx) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': data timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      exit(1);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK3 failure, exit!\n\n");
      exit(1);
    }

    // copy data to buffer
    for (i=1; i<lenRx; i++) {
      buf[idx++] = Rx[i];
    }

    // print progress
    if ((idx % 1024) == 0) {
      printf(".");
      fflush(stdout);
    }
  }

  printf(" ok\n");
  fflush(stdout);

  return(0);
}

/**
  check if microcontroller address exists. Specifically read 1B from microcontroller
  memory via READ command. If it fails, memory doesn't exist. Used to get STM8 type
*/
uint8_t bsl_memCheck(HANDLE ptrPort, uint32_t addr) {
  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];

  // init receive buffer
  memset(Rx, 0, 1000);

  if (!ptrPort) {
    // port not open
    exit(1);
  }

  // READ command

  // construct command
  lenTx = 2;
  Tx[0] = READ;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  len = send_port(ptrPort, lenTx, Tx);

  if (len != lenTx) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    exit(1);
  }

  // receive response
  len = receive_port(ptrPort, lenRx, Rx);

  if (len != lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    exit(1);
  }

  // check acknowledge
  if (Rx[0] != ACK) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK1 failure 0x%2x, exit!\n\n", Rx[0]);
    exit(1);
  }

  // Send address

  // construct address + checksum (XOR over address)
  lenTx = 5;
  Tx[0] = (char) (addr >> 24);
  Tx[1] = (char) (addr >> 16);
  Tx[2] = (char) (addr >> 8);
  Tx[3] = (char) (addr);
  Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
  lenRx = 1;

  // send command
  len = send_port(ptrPort, lenTx, Tx);

  if (len != lenTx) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    exit(1);
  }

  // receive response
  len = receive_port(ptrPort, lenRx, Rx);

  if (len != lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    exit(1);
  }

  // check acknowledge -> on NACK memory cannot be read -> return 0
  if (Rx[0]!=ACK) {
    return(0);
  }

  // send number of bytes to read

  // construct number of bytes + checksum
  lenTx = 2;
  Tx[0] = 1-1;            // -1 from BSL
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 2;

  // send command
  len = send_port(ptrPort, lenTx, Tx);

  if (len != lenTx) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending range failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    exit(1);
  }

  // receive response
  len = receive_port(ptrPort, lenRx, Rx);

  if (len != lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': data timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    exit(1);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK3 failure, exit!\n\n");
    exit(1);
  }

  // memory read succeeded -> memory exists
  return(1);
}

/**
  mass erase microcontroller P-flash and D-flash/EEPROM
*/
uint8_t bsl_flashMassErase(HANDLE ptrPort) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];

  // print message
  printf("  mass erase flash ... ");
  fflush(stdout);

  // init receive buffer
  memset(Rx, 0, 1000);

  if (!ptrPort) {
    // port not open
    exit(1);
  }

  // send erase command

  // construct command
  lenTx = 2;
  Tx[0] = ERASE;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  len = send_port(ptrPort, lenTx, Tx);

  if (len != lenTx) {
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    exit(1);
  }

  // receive response
  len = receive_port(ptrPort, lenRx, Rx);

  if (len != lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    exit(1);
  }

  // check acknowledge
  if (Rx[0] != ACK) {
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK1 failure, exit!\n\n");
    exit(1);
  }

  // send 0xFF+0x00 to trigger mass erase

  // construct pattern
  lenTx = 2;
  Tx[0] = 0xFF;
  Tx[1] = 0x00;
  lenRx = 1;

  // send command
  len = send_port(ptrPort, lenTx, Tx);

  if (len != lenTx) {
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': sending trigger failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    exit(1);
  }

  // receive response
  len = receive_port(ptrPort, lenRx, Rx);

  if (len != lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    exit(1);
  }

  // check acknowledge
  if (Rx[0] != ACK) {
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK2 failure, exit!\n\n");
    exit(1);
  }

  printf("ok\n");
  fflush(stdout);

  return(0);
}

/**
  upload data to microcontroller memory via WRITE command
*/
uint8_t bsl_memWrite(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf, int verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, idx=0, idx2=0;
  uint8_t   chk, flagEmpty;

  printf("  write ");
  fflush(stdout);

  // init receive buffer
  memset(Rx, 0, 1000);

  // check if port is open
  if (!ptrPort) {
    // port not open
    exit(1);
  }

  // loop over addresses in <=128B steps
  idx = 0;
  idx2 = 0;
  addrStep = 128;
  for (addrTmp=addrStart; addrTmp<addrStart+numBytes; addrTmp+=addrStep) {

    // if addr too close to end of range reduce stepsize
    if (addrTmp+128 > addrStart+numBytes)
      addrStep = addrStart+numBytes-addrTmp;

    // check if next block contains data. If not, skip complete block
    flagEmpty = 1;
    for (i=0; i<addrStep; i++) {
      if (buf[idx+i]) {
        flagEmpty = 0;
        break;
      }
    }
    if (flagEmpty) {
      idx += addrStep;
      continue;
    }

    // send write command

    // construct command
    lenTx = 2;
    Tx[0] = WRITE;
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = 1;

    // send command
    len = send_port(ptrPort, lenTx, Tx);

    if (len != lenTx) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      exit(1);
    }

    // receive response
    len = receive_port(ptrPort, lenRx, Rx);

    if (len != lenRx) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      exit(1);
    }

    // check acknowledge
    if (Rx[0] != ACK) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK1 failure, exit!\n\n");
      exit(1);
    }

    // send address

    // construct address + checksum (XOR over address)
    lenTx = 5;
    Tx[0] = (char) (addrTmp >> 24);
    Tx[1] = (char) (addrTmp >> 16);
    Tx[2] = (char) (addrTmp >> 8);
    Tx[3] = (char) (addrTmp);
    Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
    lenRx = 1;

    // send command
    len = send_port(ptrPort,  lenTx, Tx);

    if (len != lenTx) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      exit(1);
    }

    // receive response
    len = receive_port(ptrPort, lenRx, Rx);

    if (len != lenRx) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      exit(1);
    }

    // check acknowledge
    if (Rx[0] != ACK) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK2 failure, exit!\n\n");
      exit(1);
    }

    // send number of bytes and data

    // construct number of bytes + data + checksum
    lenTx = 0;
    Tx[lenTx++] = addrStep-1;     // -1 from BSL
    chk         = addrStep-1;
    for (i=0; i<addrStep; i++) {
      Tx[lenTx] = buf[idx++];
      idx2++;                     // only used for printing
      chk ^= Tx[lenTx];
      lenTx++;
    }
    Tx[lenTx++] = chk;
    lenRx = 1;

    // send command
    len = send_port(ptrPort, lenTx, Tx);

    if (len != lenTx) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending data failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      exit(1);
    }

    // receive response
    len = receive_port(ptrPort, lenRx, Rx);

    if (len != lenRx) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK3 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      exit(1);
    }

    // check acknowledge
    if (Rx[0] != ACK) {
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK3 failure, exit!\n\n");
      exit(1);
    }

    // print progress
    if ((idx2 % 1024) == 0) {
      printf(".");
      fflush(stdout);
    }
  }

  printf(" ok\n");
  fflush(stdout);

  return(0);
}

/**
  jump to address and continue code execution. Generally RAM or flash
  starting address
*/
uint8_t bsl_jumpTo(HANDLE ptrPort, uint32_t addr) {
  int       i;
  int       lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];

  // init receive buffer
  memset(Rx, 0, 1000);

  // check if port is open
  if (!ptrPort) {
    // port not open
    exit(1);
  }

  // send go command

  // construct command
  lenTx = 2;
  Tx[0] = GO;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  len = send_port(ptrPort, lenTx, Tx);

  if (len != lenTx) {
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    exit(1);
  }

  // receive response
  len = receive_port(ptrPort, lenRx, Rx);

  if (len != lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    exit(1);
  }

  // check acknowledge
  if (Rx[0] != ACK) {
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK1 failure, exit!\n\n");
    exit(1);
  }

  // send address

  // construct address + checksum (XOR over address)
  lenTx = 5;
  Tx[0] = (char) (addr >> 24);
  Tx[1] = (char) (addr >> 16);
  Tx[2] = (char) (addr >> 8);
  Tx[3] = (char) (addr);
  Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
  lenRx = 1;

  // send command
  len = send_port(ptrPort, lenTx, Tx);

  if (len != lenTx) {
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    exit(1);
  }

  // receive response
  len = receive_port(ptrPort, lenRx, Rx);

  if (len != lenRx) {
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    exit(1);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK2 failure, exit!\n\n");
    exit(1);
  }

  return(0);
}
