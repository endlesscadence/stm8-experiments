#ifndef _SERIAL_COMM_H_
#define _SERIAL_COMM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <windows.h>
#include <conio.h>

/// init comm port
HANDLE      init_port(const char *port, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR);

/// close comm port
void        close_port(HANDLE *fpCom);

/// get comm port settings
void        get_port_attribute(HANDLE fpCom, uint32_t *baudrate, uint32_t *timeout, uint8_t *numBits, uint8_t *parity, uint8_t *numStop, uint8_t *RTS, uint8_t *DTR);

/// modify comm port settings
void        set_port_attribute(HANDLE fpCom, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR);

/// modify comm port baudrate
void        set_baudrate(HANDLE fpCom, uint32_t baudrate);

/// modify comm port timeout
void        set_timeout(HANDLE fpCom, uint32_t timeout);

/// send data
uint32_t    send_port(HANDLE fpCom, uint32_t lenTx, char *Tx);

/// receive data
uint32_t    receive_port(HANDLE fpCom, uint32_t lenRx, char *Rx);

/// flush port buffers
void        flush_port(HANDLE fpCom);

#endif
