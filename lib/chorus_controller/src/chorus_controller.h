#ifndef __chorus_controller_h_INCLUDED__
#define __chorus_controller_h_INCLUDED__

#include "stdint.h"

#define CHROUS_BAUD 115200
#define CHORUS_MARKER '\n'

typedef struct
{
  uint8_t uart;
  uint8_t txPin;
  uint8_t rxPin;
  uint32_t baud;
} ChorusControllerConfig_t;

void chorusControllerInit(ChorusControllerConfig_t *_config);

#endif