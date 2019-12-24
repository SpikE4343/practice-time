/* SPI Slave example, sender (uses SPI master driver)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "rssi_reader.h"
#include "filters.h"
#include "timers.h"

static RssiReaderConfig_t *config;

RssiReading_t readings[MAX_RSSI_CHANNEL_COUNT];

float lpf_alpha = 0.025f;
float lpf2_alpha = 0.025f;

static QueueHandle_t rssiReadLock = NULL;

void rssiReadTask(void *args);

void rssiConfigPrint(RssiReaderConfig_t *config)
{
  printf("rssi-reader-config:\n");
  printf(" channelCount=%u\n", config->channelCount);
  printf(" updateHz=%u\n", config->updateHz);
  printf(" lpfCutoffHz=%u\n", config->lpfCutoffHz);
}

RssiReading_t *rssiReadings()
{
  return readings;
}

void rssiInit(RssiReaderConfig_t *info)
{
  config = info;
  rssiConfigPrint(config);

  memset(readings, 0, sizeof(float) * MAX_RSSI_CHANNEL_COUNT);

  lpf_alpha = lpfAlpha(config->lpfCutoffHz, config->updateHz);
  lpf2_alpha = lpfAlpha(config->lpf2CutoffHz, config->updateHz);

  adc1_config_width(config->bitWidth);

  for (int c = 0; c < config->channelCount; ++c)
  {
    adc1_config_channel_atten(config->channels[c], config->attenuation);
  }

  rssiReadLock = xSemaphoreCreateBinary();

  timerInit(TIMER_1, TIMER_GROUP_1, rssiReadLock, true, 1.0f / config->updateHz);
  xTaskCreate(rssiReadTask, "rssiReadTask", 1024 * 3, NULL, 10, NULL);
}

void rssiReadTask(void *arg)
{
  while (1)
  {
    if (xSemaphoreTake(rssiReadLock, portMAX_DELAY) == pdFALSE)
    {
      continue;
    }

    uint32_t timestamp = millis();

    for (int c = config->channelCount - 1; c >= 0; --c)
    {
      readings[c].timestamp = timestamp;
      readings[c].raw = adc1_get_raw(config->channels[c]);
      readings[c].normalized = (readings[c].raw / 4095.0f - 0.5f) / 0.5f;
      readings[c].filtered = lowPassFilter(readings[c].filtered, readings[c].normalized, lpf_alpha);
      readings[c].filtered = lowPassFilter(readings[c].filtered, readings[c].filtered, lpf2_alpha);
      ++readings[c].sampleCount;
    }
  }
}
