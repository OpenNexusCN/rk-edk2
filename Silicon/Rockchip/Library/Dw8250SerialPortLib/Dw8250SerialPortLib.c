/** @file
  DesignWare APB UART SerialPortLib.

  The UART base and input clock come from PCDs; the board is responsible for
  pin muxing the UART pins before this library is used.

  Copyright (c) 2008-2010, Apple Inc.
  Copyright (c) 2011-2016, ARM Ltd.
  Copyright (c) 2017, Rockchip Inc.
  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include <Protocol/SerialIo.h>

/* DW APB UART registers (reg-shift = 2) */
#define UART_RBR    0x00
#define UART_THR    0x00
#define UART_DLL    0x00
#define UART_DLH    0x04
#define UART_IER    0x04
#define UART_FCR    0x08
#define UART_LCR    0x0C
#define UART_MCR    0x10
#define UART_LSR    0x14
#define UART_USR    0x7C
#define UART_SRR    0x88

#define UART_LSR_TEMT   0x40

#define UART_LCR_DLAB   0x80
#define UART_LCR_8N1    0x03
#define UART_LCR_STOP2  0x04
#define UART_LCR_PARITY 0x08
#define UART_LCR_EPS    0x10

#define UART_FCR_ENABLE  0x01
#define UART_FCR_CLEAR_RX 0x02
#define UART_FCR_CLEAR_TX 0x04
#define UART_FCR_TRIGGER_2 0xC0

#define UART_USR_TX_NOT_FULL   0x02
#define UART_USR_RX_NOT_EMPTY  0x04

#define UART_SRR_RESET_ALL  0x07

STATIC
UINTN
Dw8250Base (
  VOID
  )
{
  return (UINTN)FixedPcdGet64 (PcdSerialRegisterBase);
}

STATIC
UINT32
Dw8250Read (
  IN UINTN  Offset
  )
{
  return MmioRead32 (Dw8250Base () + Offset);
}

STATIC
VOID
Dw8250Write (
  IN UINTN   Offset,
  IN UINT32  Value
  )
{
  MmioWrite32 (Dw8250Base () + Offset, Value);
}

STATIC
RETURN_STATUS
Dw8250SetBaud (
  IN UINT64  BaudRate
  )
{
  UINT32  Clock;
  UINT32  Divisor;
  UINT32  Lcr;

  if (BaudRate == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  Clock   = FixedPcdGet32 (PcdUartClockInHz);
  Divisor = Clock / (16 * (UINT32)BaudRate);
  if (Divisor == 0) {
    return RETURN_UNSUPPORTED;
  }

  Lcr = Dw8250Read (UART_LCR);
  Dw8250Write (UART_LCR, Lcr | UART_LCR_DLAB);
  Dw8250Write (UART_DLL, Divisor & 0xFF);
  Dw8250Write (UART_DLH, (Divisor >> 8) & 0xFF);
  Dw8250Write (UART_LCR, Lcr & ~UART_LCR_DLAB);

  return RETURN_SUCCESS;
}

/**
  Initialize the serial device hardware with default settings.
**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  UINT32              Timeout;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;

  BaudRate         = FixedPcdGet64 (PcdUartDefaultBaudRate);
  ReceiveFifoDepth = 0;
  Timeout          = 0;
  Parity           = (EFI_PARITY_TYPE)FixedPcdGet8 (PcdUartDefaultParity);
  DataBits         = FixedPcdGet8 (PcdUartDefaultDataBits);
  StopBits         = (EFI_STOP_BITS_TYPE)FixedPcdGet8 (PcdUartDefaultStopBits);

  return SerialPortSetAttributes (
           &BaudRate,
           &ReceiveFifoDepth,
           &Timeout,
           &Parity,
           &DataBits,
           &StopBits
           );
}

/**
  Set new attributes on the UART.
**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT UINT32              *Timeout,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  UINT32        Lcr;
  RETURN_STATUS Status;

  if ((BaudRate == NULL) || (Parity == NULL) || (DataBits == NULL) || (StopBits == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  /*
   * FMX1 console is fixed 8N1. The ArmPkg BDS requires non-zero
   * PcdUartDefaultParity/StopBits, so attribute requests are accepted but
   * the line format is intentionally not changed here.
   */
  Dw8250Write (UART_SRR, UART_SRR_RESET_ALL);
  Dw8250Write (UART_IER, 0);
  Dw8250Write (UART_MCR, 0);

  Lcr = UART_LCR_8N1;
  Dw8250Write (UART_LCR, Lcr);

  Status = Dw8250SetBaud (*BaudRate);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  /* Enable and reset FIFOs, trigger RX at 2 characters. */
  Dw8250Write (UART_FCR, UART_FCR_ENABLE | UART_FCR_CLEAR_RX |
                         UART_FCR_CLEAR_TX | UART_FCR_TRIGGER_2);

  if (ReceiveFifoDepth != NULL) {
    *ReceiveFifoDepth = 32;
  }

  return RETURN_SUCCESS;
}

UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  UINTN  Index;

  if ((Buffer == NULL) || (NumberOfBytes == 0)) {
    return 0;
  }

  for (Index = 0; Index < NumberOfBytes; Index++) {
    while ((Dw8250Read (UART_USR) & UART_USR_TX_NOT_FULL) == 0) {
      CpuPause ();
    }
    Dw8250Write (UART_THR, Buffer[Index]);
  }

  return NumberOfBytes;
}

UINTN
EFIAPI
SerialPortRead (
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  UINTN  Index;

  if ((Buffer == NULL) || (NumberOfBytes == 0)) {
    return 0;
  }

  for (Index = 0; Index < NumberOfBytes; Index++) {
    while ((Dw8250Read (UART_USR) & UART_USR_RX_NOT_EMPTY) == 0) {
      CpuPause ();
    }
    Buffer[Index] = (UINT8)Dw8250Read (UART_RBR);
  }

  return NumberOfBytes;
}

BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  return (Dw8250Read (UART_USR) & UART_USR_RX_NOT_EMPTY) != 0;
}

RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  )
{
  UINT32  Mcr;

  Mcr = Dw8250Read (UART_MCR);
  if ((Control & EFI_SERIAL_REQUEST_TO_SEND) != 0) {
    Mcr |= BIT1;
  } else {
    Mcr &= ~BIT1;
  }

  if ((Control & EFI_SERIAL_DATA_TERMINAL_READY) != 0) {
    Mcr |= BIT0;
  } else {
    Mcr &= ~BIT0;
  }

  Dw8250Write (UART_MCR, Mcr);
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  UINT32  Usr;
  UINT32  Lsr;

  if (Control == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Usr = Dw8250Read (UART_USR);
  Lsr = Dw8250Read (UART_LSR);

  *Control = 0;
  if ((Usr & UART_USR_RX_NOT_EMPTY) == 0) {
    *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  if ((Lsr & UART_LSR_TEMT) != 0) {
    *Control |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
  }

  return RETURN_SUCCESS;
}
