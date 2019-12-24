/* SPI Slave example, sender (uses SPI master driver)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef __rx_controller_INCLUDED__
#define __rx_controller_INCLUDED__

#include "driver/gpio.h"
#include "driver/adc.h"

#define MAX_RX_COUNT 8

typedef struct
{
  uint8_t spiOutputPin;
  uint8_t spiClockPin;
  uint32_t spiClockSpeed;
  uint8_t rxCount;
  uint8_t spiSelectPin[MAX_RX_COUNT];
} RxControllerConfig_t;

void rxInit(RxControllerConfig_t *info);
void rxSetState(uint8_t deviceId, uint8_t band, uint8_t channel);
const char rxGetBandShortName(int band);
int rxGetFrequency(int band, int channel);
#endif