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
#include "driver/spi_master.h"

#include "rx_channels.h"
#include "rx_controller.h"

#include "rx_5808.h"

static RxControllerConfig_t *config;

static spi_device_handle_t devHandle[MAX_RX_COUNT];

void rxInit(RxControllerConfig_t *info)
{
  config = info;

  esp_err_t ret;

  //Configuration for the SPI bus
  spi_bus_config_t buscfg = {
      .mosi_io_num = config->spiOutputPin,
      .miso_io_num = -1, //config->spiOutputPin,
      .sclk_io_num = config->spiClockPin,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1};

  ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
  assert(ret == ESP_OK);

  spi_device_interface_config_t devcfg = {
      .command_bits = 0,
      .address_bits = 0,
      .dummy_bits = 0,
      .clock_speed_hz = config->spiClockSpeed,
      .duty_cycle_pos = 128, //50% duty cycle
      .mode = 0,
      .flags = SPI_DEVICE_TXBIT_LSBFIRST | SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX,
      .spics_io_num = 0,
      .cs_ena_posttrans = 1, //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
      .queue_size = 1,
      //.post_cb=
  };

  for (int i = 0; i < config->rxCount; ++i)
  {
    devcfg.spics_io_num = config->devices[i].spiSelectPin;

    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &devHandle[i]);
    assert(ret == ESP_OK);
  }
}

void rxProcessTransaction(spi_transaction_t *t)
{
  // esp_err_t ret;
  // ret=spi_device_transmit(spi, &t);  //Transmit!
  // assert(ret==ESP_OK);            //Should have had no issues.
}

void rxTransmit(spi_device_handle_t spi, const uint8_t *data, int len)
{
  // simple sync send
  // TODO: implement async sending
  esp_err_t ret;
  spi_transaction_t t;

  memset(&t, 0, sizeof(t)); //Zero out the transaction
  //t.flags = SPI_TRANS_USE_RXDATA;
  t.length = len * 8;                 //Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;                 //Data
  ret = spi_device_transmit(spi, &t); //Transmit!
  //printf("rx:%X%X\n", t.rx_data[0], t.rx_data[1]);
  assert(ret == ESP_OK); //Should have had no issues.
}

uint16_t getChannelData(uint16_t frequency)
{
  uint8_t i;
  uint16_t channelData;

  channelData = frequency - 479;
  channelData /= 2;
  i = channelData % 32;
  channelData /= 32;
  channelData = (channelData << 7) + i;
  return channelData;
}

void rxSetState(uint8_t deviceId, uint8_t band, uint8_t channel)
{
  assert(deviceId >= (uint8_t)0);
  assert(deviceId < MAX_RX_COUNT);
  assert(deviceId < config->rxCount);


  int index = (channel - 1) + (8 * band);
  uint16_t freq = channelFreqTable[index];
  uint16_t data = channelTable[index];

  rx5808_request_t ra = {
      .address = 1,
      .readWrite = 0,
      .data = 0};

  rx5808_request_t rd = {
      .address = 1,
      .readWrite = 1,
      .data = data};

  rxTransmit(devHandle[deviceId], (uint8_t *)&ra, sizeof(rx5808_request_t));
  rxTransmit(devHandle[deviceId], (uint8_t *)&rd, sizeof(rx5808_request_t));

  printf("rx[%u]->[b:%u, c:%u, f:%u, d:%x]\n", deviceId, band, channel, freq, data);
}

const char rxGetBandShortName(int band)
{
  return bandShortNameTable[band];
}

int rxGetFrequency(int band, int channel)
{
  int index = (channel - 1) + (8 * band);
  return channelFreqTable[index];
}