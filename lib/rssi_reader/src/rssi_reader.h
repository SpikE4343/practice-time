/* SPI Slave example, sender (uses SPI master driver)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef __rssi_reader_INCLUDED__
#define __rssi_reader_INCLUDED__

#include "driver/gpio.h"
#include "driver/adc.h"

#define MAX_RSSI_CHANNEL_COUNT 8

typedef struct
{
  uint32_t updateHz;
  adc_bits_width_t bitWidth;
  adc_atten_t attenuation;
  uint8_t channelCount;
  uint16_t lpfCutoffHz;
  uint16_t lpf2CutoffHz;
  uint16_t calibrationSec;
  uint8_t channels[MAX_RSSI_CHANNEL_COUNT];
} RssiReaderConfig_t;

typedef struct
{
  float raw;
  float normalized;
  float filtered;
  uint32_t sampleCount;
  uint32_t timestamp;
  uint16_t bias;
} RssiReading_t;

void rssiConfigPrint(RssiReaderConfig_t *config);

RssiReading_t *rssiReadings();
void rssiInit(RssiReaderConfig_t *info);

#endif