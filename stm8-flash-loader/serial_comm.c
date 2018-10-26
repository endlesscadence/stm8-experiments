#include "serial_comm.h"

/**
  open comm port for communication, set properties (baudrate, timeout,...).
  Note: baudrate must be supported by COM port driver.
*/
HANDLE init_port(const char *port, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR) {
  char          port_tmp[100];
  HANDLE        fpCom = NULL;
  DCB           fDCB;
  BOOL          fSuccess;
  COMMTIMEOUTS  fTimeout;

  // required to allow COM ports >COM9
  sprintf(port_tmp,"\\\\.\\%s", port);

  // create handle to COM-port
  fpCom = CreateFile(port_tmp,
                     GENERIC_READ | GENERIC_WRITE,  // both read & write
                     0,    // must be opened with exclusive-access
                     NULL, // no security attributes
                     OPEN_EXISTING, // must use OPEN_EXISTING
                     0,    // not overlapped I/O
                     NULL  // hTemplate must be NULL for comm devices
                    );
  if (fpCom == INVALID_HANDLE_VALUE) {
    fpCom = NULL;
    fprintf(stderr, "\n\nerror in 'init_port(%s)': open port failed with code %d, exit!\n\n", port, (int) GetLastError());
    exit(1);
  }

  // reset COM port error buffer
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

  // get the current port configuration
  fSuccess = GetCommState(fpCom, &fDCB);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'init_port(%s)': get port attributes failed with code %d, exit!\n\n", port, (int) GetLastError());
    exit(1);
  }


  // change port settings
  fDCB.BaudRate = baudrate;         // set the baud rate (19200, 57600, 115200)
  fDCB.ByteSize = numBits;          // number of data bits per byte
  if (parity) {
    fDCB.fParity = TRUE;            // Enable parity checking
    fDCB.Parity  = parity;          // 0-4=no,odd,even,mark,space
  } else {
    fDCB.fParity  = FALSE;          // disable parity checking
    fDCB.Parity   = NOPARITY;       // just to make sure
  }
  if (numStop == 1)
    fDCB.StopBits = ONESTOPBIT;     // one stop bit
  else if (numStop == 2)
    fDCB.StopBits = TWOSTOPBITS;    // two stop bit
  else
    fDCB.StopBits = ONE5STOPBITS;   // 1.5 stop bit
  fDCB.fRtsControl = (RTS != 0);    // RTS off(=0=-12V) or on(=1=+12V)
  fDCB.fDtrControl = (DTR != 0);    // DTR off(=0=-12V) or on(=1=+12V)

  // set new COM state
  fSuccess = SetCommState(fpCom, &fDCB);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'init_port(%s)': set port attributes failed with code %d, exit!\n\n", port, (int) GetLastError());
    exit(1);
  }


  // set timeouts for port to avoid hanging of program. For simplicity set all timeouts to same value.
  // For timeout=0 set values to query for buffer content
  if (timeout == 0)
    fTimeout.ReadIntervalTimeout        = MAXDWORD;    // --> no read timeout
  else
    fTimeout.ReadIntervalTimeout        = 0;           // max. ms between following read bytes (0=not used)
  fTimeout.ReadTotalTimeoutMultiplier   = 0;           // time per read byte (use contant timeout instead)
  fTimeout.ReadTotalTimeoutConstant     = timeout;     // total read timeout in ms
  fTimeout.WriteTotalTimeoutMultiplier  = 0;           // time per write byte (use contant timeout instead)
  fTimeout.WriteTotalTimeoutConstant    = timeout;
  fSuccess = SetCommTimeouts(fpCom, &fTimeout);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'init_port(%s)': set port timeout failed with code %d, exit!\n\n", port, (int) GetLastError());
    exit(1);
  }

  // return hande
  return(fpCom);
}

/**
  close & release comm port.
*/
void close_port(HANDLE *fpCom) {
  BOOL      fSuccess;

  if (*fpCom != NULL) {
    fSuccess = CloseHandle(*fpCom);
    if (!fSuccess) {
      *fpCom = NULL;
      fprintf(stderr, "\n\nerror in 'close_port': close port failed with code %d, exit!\n\n", (int) GetLastError());
      exit(1);
    }
  }
  *fpCom = NULL;
}

/**
  get current attributes of an already open comm port.
*/
void get_port_attribute(HANDLE fpCom, uint32_t *baudrate, uint32_t *timeout, uint8_t *numBits, uint8_t *parity, uint8_t *numStop, uint8_t *RTS, uint8_t *DTR) {
  DCB           fDCB;
  COMMTIMEOUTS  fTimeout;
  BOOL          fSuccess;

  // get the current port configuration
  fSuccess = GetCommState(fpCom, &fDCB);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'get_port_attribute': GetCommState() failed with code %d, exit!\n\n", (int) GetLastError());
    exit(1);
  }

  // get port settings
  *baudrate = fDCB.BaudRate;        // baud rate (19200, 57600, 115200)
  *numBits  = fDCB.ByteSize;        // number of data bits per byte
  *parity   = fDCB.Parity;          // parity bit by HW
  *numStop  = fDCB.StopBits;        // number of stop bits
  if (fDCB.StopBits == ONESTOPBIT)
    *numStop = 1;                      // 1 stop bit
  else if (fDCB.StopBits == TWOSTOPBITS)
    *numStop = 2;                      // 2 stop bits
  else
    *numStop = 3;                      // 1.5 stop bits
  *RTS      = fDCB.fRtsControl;     // RTS off(=0=-12V) or on(=1=+12V)
  *DTR      = fDCB.fDtrControl;     // DTR off(=0=-12V) or on(=1=+12V)

  // get port timeout
  fSuccess = GetCommTimeouts(fpCom, &fTimeout);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'get_port_attribute': GetCommTimeouts() failed with code %d, exit!\n\n", (int) GetLastError());
    exit(1);
  }
  *timeout = fTimeout.ReadTotalTimeoutConstant;       // this parameter fits also for timeout=0
}

/**
  change attributes of an already open comm port.
*/
void set_port_attribute(HANDLE fpCom, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR) {
  DCB           fDCB;
  BOOL          fSuccess;
  COMMTIMEOUTS  fTimeout;

  // reset COM port error buffer
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

  // get the current port configuration
  fSuccess = GetCommState(fpCom, &fDCB);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'set_port_attribute()': get port attributes failed with code %d, exit!\n\n", (int) GetLastError());
    exit(1);
  }

  // change port settings
  fDCB.BaudRate = baudrate;         // set the baud rate (19200, 57600, 115200)
  fDCB.ByteSize = numBits;          // number of data bits per byte
  if (parity) {
    fDCB.fParity = TRUE;            // Enable parity checking
    fDCB.Parity  = parity;          // 0-4=no,odd,even,mark,space
  } else {
    fDCB.fParity  = FALSE;          // disable parity checking
    fDCB.Parity   = NOPARITY;       // just to make sure
  }
  if (numStop == 1)
    fDCB.StopBits = ONESTOPBIT;     // one stop bit
  else if (numStop == 2)
    fDCB.StopBits = TWOSTOPBITS;    // two stop bit
  else
    fDCB.StopBits = ONE5STOPBITS;   // 1.5 stop bit
  fDCB.fRtsControl = (RTS != 0);    // RTS off(=0=-12V) or on(=1=+12V)
  fDCB.fDtrControl = (DTR != 0);    // DTR off(=0=-12V) or on(=1=+12V)

  // set new COM state
  fSuccess = SetCommState(fpCom, &fDCB);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'set_port_attribute()': set port attributes failed with code %d, exit!\n\n", (int) GetLastError());
    exit(1);
  }

  // set timeouts for port to avoid hanging of program. For simplicity set all timeouts to same value.
  // For timeout=0 set values to query for buffer content
  if (timeout == 0)
    fTimeout.ReadIntervalTimeout        = MAXDWORD;    // --> no read timeout
  else
    fTimeout.ReadIntervalTimeout        = 0;           // max. ms between following read bytes (0=not used)
  fTimeout.ReadTotalTimeoutMultiplier   = 0;           // time per read byte (use contant timeout instead)
  fTimeout.ReadTotalTimeoutConstant     = timeout;     // total read timeout in ms
  fTimeout.WriteTotalTimeoutMultiplier  = 0;           // time per write byte (use contant timeout instead)
  fTimeout.WriteTotalTimeoutConstant    = timeout;
  fSuccess = SetCommTimeouts(fpCom, &fTimeout);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'set_port_attribute()': set port timeout failed with code %d, exit!\n\n", (int) GetLastError());
    exit(1);
  }
}

/**
  set new baudrate for an already open comm port. Baudrate must be supported by PC driver.
*/
void set_baudrate(HANDLE fpCom, uint32_t baudrate) {
  DCB       fDCB;
  BOOL      fSuccess;

  // get the current port configuration
  fSuccess = GetCommState(fpCom, &fDCB);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'set_baudrate(%d)': get port attributes failed with code %d, exit!\n\n", (int) baudrate, (int) GetLastError());
    exit(1);
  }

  // change port settings
  fDCB.BaudRate = baudrate;     // set the baud rate (19200, 57600, 115200)
  fSuccess = SetCommState(fpCom, &fDCB);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'set_baudrate(%d)': set port attributes failed with code %d, exit!\n\n", (int) baudrate, (int) GetLastError());
    exit(1);
  }
}

/**
  set new timeout for an already open comm port
*/
void set_timeout(HANDLE fpCom, uint32_t timeout) {
  BOOL          fSuccess;
  COMMTIMEOUTS  fTimeout;

  // set timeouts for port to avoid hanging of program. For simplicity set all timeouts to same value.
  // For timeout=0 set values to query for buffer content
  if (timeout == 0)
    fTimeout.ReadIntervalTimeout        = MAXDWORD;    // --> no read timeout
  else
    fTimeout.ReadIntervalTimeout        = 0;           // max. ms between following read bytes (0=not used)
  fTimeout.ReadTotalTimeoutMultiplier   = 0;           // time per read byte (use contant timeout instead)
  fTimeout.ReadTotalTimeoutConstant     = timeout;     // total read timeout in ms
  fTimeout.WriteTotalTimeoutMultiplier  = 0;           // time per write byte (use contant timeout instead)
  fTimeout.WriteTotalTimeoutConstant    = timeout;
  fSuccess = SetCommTimeouts(fpCom, &fTimeout);
  if (!fSuccess) {
    fprintf(stderr, "\n\nerror in 'set_timeout(%d)': set timeout failed with code %d, exit!\n\n", (int) timeout, (int) GetLastError());
    exit(1);
  }
}

/**
  send data via comm port.
*/
uint32_t send_port(HANDLE fpCom, uint32_t lenTx, char *Tx) {

  // for reading back LIN echo
  char      Rx[1000];
  uint32_t  lenRx;

  DWORD   numChars;

  // send data & return number of sent bytes
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
  WriteFile(fpCom, Tx, lenTx, &numChars, NULL);

  // return number of sent bytes
  return((uint32_t) numChars);

}

/**
  receive data via comm port. 
  (UART reply mode with 2-wire interface), reply each byte from STM8 -> SLOW
*/
uint32_t receive_port(HANDLE fpCom, uint32_t lenRx, char *Rx) {
  DWORD     numChars, numTmp;
  uint32_t  i;

  // echo each byte as it is received
  numChars = 0;
  for (i=0; i<lenRx; i++) {
    ReadFile(fpCom, Rx+i, 1, &numTmp, NULL);
    if (numTmp == 1) {
      numChars++;
      send_port(fpCom, 1, Rx+i);
    } else
      break;
  }

  // return number of bytes received
  return((uint32_t) numChars);
}

/**
  flush port input & output buffer.
*/
void flush_port(HANDLE fpCom) {
  // purge all port buffers (see http://msdn.microsoft.com/en-us/library/windows/desktop/aa363428%28v=vs.85%29.aspx)
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
}
