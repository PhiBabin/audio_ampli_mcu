#ifndef TINY_IR_RECEIVER_GUARD_H_
#define TINY_IR_RECEIVER_GUARD_H_

#include "sim/arduino.h"

#define NEC_ADDRESS_BITS 16                          // 16 bit address or 8 bit address and 8 bit inverted address
#define NEC_COMMAND_BITS 16                          // Command and inverted command
#define TINY_RECEIVER_ADDRESS_BITS NEC_ADDRESS_BITS  // the address bits + parity
#define TINY_RECEIVER_COMMAND_BITS NEC_COMMAND_BITS  // the command bits + parity
#define TINY_RECEIVER_ADDRESS_HAS_8_BIT_PARITY false

struct TinyIRReceiverCallbackDataStruct
{
#if (TINY_RECEIVER_ADDRESS_BITS > 0)
#if (TINY_RECEIVER_ADDRESS_BITS == 16) && !TINY_RECEIVER_ADDRESS_HAS_8_BIT_PARITY
  uint16_t Address;
#else
  uint8_t Address;
#endif
#endif

#if (TINY_RECEIVER_COMMAND_BITS == 16) && !TINY_RECEIVER_COMMAND_HAS_8_BIT_PARITY
  uint16_t Command;
#else
  uint8_t Command;
#endif
  uint8_t Flags;  // Bit coded flags. Can contain one of the bits: IRDATA_FLAGS_IS_REPEAT and IRDATA_FLAGS_PARITY_FAILED
  bool justWritten;  ///< Is set true if new data is available. Used by the main loop / TinyReceiverDecode(), to avoid
                     ///< multiple evaluations of the same IR frame.
};
volatile TinyIRReceiverCallbackDataStruct TinyIRReceiverData;

bool initPCIInterruptForTinyReceiver();
bool TinyReceiverDecode();
#endif  // TINY_IR_RECEIVER_GUARD_H_