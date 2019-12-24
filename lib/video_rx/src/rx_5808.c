#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rx_5808.h"

#define SHIFT_BITS (32 - 20)
#define RX5808_REGISTER_PACKET_SIZE 25

void print_address(uint8_t *data, uint16_t num)
{
  for (int i = 0; i < num; ++i)
  {
    printf("%02x", data[i]);
  }

  printf("\n");
}

uint32_t rx5808_spi_read(spi_device_handle_t dev, uint8_t address)
{
  esp_err_t ret = ESP_OK;

  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  //t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  t.user = (void *)1; //D/C needs to be set to 1

  //rx5808_request_t* r = (rx5808_request_t*)&t.tx_data[0];
  rx5808_request_t r; // = (rx5808_request_t*)&t.tx_data[0];
  memset(&r, 0, sizeof(rx5808_request_t));
  //r->address = address;
  //r->readWrite = RX5808_READ;
  t.tx_buffer = &r;
  //printf("tx: %u, %x\n", address, t.tx_data[0]);
  //print_address(r, sizeof(rx5808_request_t));

  ret = spi_device_transmit(dev, &t); //Transmit!

  printf("rx:[ num: %u, data: %x]\n", t.rxlength, t.rx_data[0]);
  assert(ret == ESP_OK); //Should have had no issues.
  return 0;
}

#define READ_SPI_REGISTER(rn)                         \
  value = rx5808_spi_read(dev, 0x##rn) << SHIFT_BITS; \
  state->reg##rn = *((rx5808_register_##rn##_data_t *)&value);

int rx5808_spi_read_state(spi_device_handle_t dev, rx5808_state_t *state)
{
  uint32_t value = 0;

  //READ_SPI_REGISTER(00);
  READ_SPI_REGISTER(01);
  // READ_SPI_REGISTER(02);
  // READ_SPI_REGISTER(03);
  // READ_SPI_REGISTER(04);
  // READ_SPI_REGISTER(05);
  // READ_SPI_REGISTER(06);
  // READ_SPI_REGISTER(07);
  // READ_SPI_REGISTER(08);
  // READ_SPI_REGISTER(09);
  // READ_SPI_REGISTER(0A);
  // READ_SPI_REGISTER(0F);

  return 0;
}