/* SPI Slave example, sender (uses SPI master driver)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "math_util.h"

#include "mongoose.h"
#include "webserver.h"
#include "chorus_controller.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "cJSON.h"

#define MAX_DEVICE_COUNT 8

#define CHORUS_CMD_RSSI_MONITOR "rssiMonitor"

typedef struct
{
  int channel;
  int band;
  int threshold;
  char pilotName[64];
  int isCalibrated;
  int calibrationTime;
  int calibrationValue;
  int currentRSSI;
  int isEnabled;
  uint64_t deviceTime;
  int thresholdSetupState;
  int apiVersion;
  int setupComplete;
} ChorusDevice_t;

typedef struct
{
  ChorusDevice_t devices[8];
  uint8_t deviceCount;
  uint8_t rssiMonitor;
  uint8_t raceStarted;
} ChorusControllerState_t;

static ChorusControllerState_t state;
static ChorusControllerConfig_t *config;

static WebRequestHandler_t statusHandler;
static WebRequestHandler_t commandHandler;

static void rx_task();
static char web_buffer[8192];

bool isSetupComplete()
{
  for (int c = 0; c < state.deviceCount; ++c)
  {
    if (!state.devices[c].setupComplete)
      return false;
  }

  return true;
}

void send_cmd(char *msg)
{
  printf("sending command: %s\n", msg);
  uart_write_bytes(config->uart, msg, strlen(msg));
  printf("sent command: %s\n", msg);
}

void statusCallback(struct mg_connection *nc, struct http_message *hm)
{
  char *start = &web_buffer[0];
  start += sprintf(start, "<html><body><h1>Devices</h1><p>%d</p>", state.deviceCount);
  start += sprintf(start, "<table>");
  start += sprintf(start, "<th>Id</th>");
  start += sprintf(start, "<th>Band</th>");
  start += sprintf(start, "<th>Channel</th>");
  start += sprintf(start, "<th>Threshold</th>");
  start += sprintf(start, "<th>RSSI</th>");
  start += sprintf(start, "<th>API</th>");
  start += sprintf(start, "<th>Setup</th>");
  start += sprintf(start, "<th>Calibrated</th>");
  start += sprintf(start, "<th>Calibration Time</th>");
  start += sprintf(start, "<th>Calibration Value</th>");
  start += sprintf(start, "<th>Enabled</th>");

  for (int c = 0; c < state.deviceCount; ++c)
  {
    ChorusDevice_t *device = &state.devices[c];
    start += sprintf(start, "<tr>");
    start += sprintf(start, "<td>%d</td>", c);
    start += sprintf(start, "<td>%d</td>", device->band);
    start += sprintf(start, "<td>%d</td>", device->channel);
    start += sprintf(start, "<td>%d</td>", device->currentRSSI);
    start += sprintf(start, "<td>%d</td>", device->threshold);
    start += sprintf(start, "<td>%d</td>", device->apiVersion);
    start += sprintf(start, "<td>%d</td>", device->setupComplete);
    start += sprintf(start, "<td>%d</td>", device->isCalibrated);
    start += sprintf(start, "<td>%d</td>", device->calibrationTime);
    start += sprintf(start, "<td>%d</td>", device->calibrationValue);
    start += sprintf(start, "<td>%d</td>", device->isEnabled);

    start += sprintf(start, "</tr>");
  }

  start += sprintf(start, "</table>");
  sprintf(start, "</body></html>");
  int len = strlen(&web_buffer[0]);
  mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%.*s", len, len, &web_buffer[0]);
}

void commandCallback(struct mg_connection *nc, struct http_message *hm)
{
  cJSON *resp = cJSON_CreateObject();

  cJSON *command_json = cJSON_Parse(hm->body.p);
  if (command_json == NULL)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL)
    {
      printf(stderr, "Error before: %s\n", error_ptr);
      mg_http_send_error(nc, 404, error_ptr);
      return;
    }
  }

  cJSON *command = cJSON_GetObjectItemCaseSensitive(command_json, "command");
  if (cJSON_IsString(command) && (command->valuestring != NULL))
  {
    if (strcmp(command->valuestring, CHORUS_CMD_RSSI_MONITOR) == 0)
    {
      cJSON *value = cJSON_GetObjectItemCaseSensitive(command_json, "value");

      state.rssiMonitor = value->valueint;

      if (state.rssiMonitor)
        send_cmd("R*I0064\n");
      else
        send_cmd("R*I0000\n");

      cJSON_AddNumberToObject(resp, CHORUS_CMD_RSSI_MONITOR, state.rssiMonitor);
    }
  }

  cJSON_PrintPreallocated(resp, web_buffer, sizeof(web_buffer), true);
  int len = strlen(&web_buffer[0]);
  mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%.*s", len, len, &web_buffer[0]);
}

void chorusControllerInit(ChorusControllerConfig_t *_config)
{
  memset(&state, 0, sizeof(state));

  printf("chorus init\n");
  config = _config;

  const uart_config_t uart_config = {
      .baud_rate = config->baud,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_param_config(config->uart, &uart_config);
  uart_set_pin(
      config->uart,
      config->txPin,
      config->rxPin,
      UART_PIN_NO_CHANGE,
      UART_PIN_NO_CHANGE);

  //uart_set_line_inverse(config->uart, UART_INVERSE_RXD | UART_INVERSE_TXD);

  printf("uart pin: %u\n", config->uart);

  uart_driver_install(
      config->uart,
      4096, 8192,
      0, NULL, 0);

  statusHandler.callback = &statusCallback;
  statusHandler.path = "/status";
  statusHandler.request = HTTP_GET;

  commandHandler.callback = &commandCallback;
  commandHandler.path = "/command";
  commandHandler.request = HTTP_GET;

  webserverRegister(&commandHandler);
  webserverRegister(&statusHandler);

  xTaskCreate(rx_task, "chorus_rx_task", 1024 * 2, NULL, 100, NULL);

  send_cmd("N0\n");
}

void handle_set_message(int id, char *data, int dataLen)
{
  if (id < 0 || id > state.deviceCount)
    return;

  ChorusDevice_t *device = &state.devices[id];

  char type = data[0];

  switch (type)
  {
  case 'C':
    device->channel = strtol(data + 1, NULL, 16);
    printf("Device: %d, channel: %d\n", id, device->channel);
    break;
  case 'R':
    // int race = Integer.parseInt(chunk.substring(3, 4));
    // result.append("Race: ").append(race != 0 ? "started" : "stopped");
    // AppState.getInstance().changeRaceState(race != 0);
    break;
  case 'M':
    // int minLapTime = Integer.parseInt(chunk.substring(3, 5), 16);
    // result.append("Min Lap: ").append(minLapTime);
    // AppState.getInstance().changeRaceMinLapTime(minLapTime);
    break;
  case 'T':
    // int threshold = Integer.parseInt(chunk.substring(3, 7), 16);
    // result.append("Threshold: ").append(threshold);
    // AppState.getInstance().changeDeviceThreshold(moduleId, threshold);
    break;
  case 'r':
    device->currentRSSI = strtol(data + 1, NULL, 16);
    printf("Device: %d, rssi: %d\n", id, device->currentRSSI);
    break;
  case 'S':
    // int soundState = Integer.parseInt(chunk.substring(3, 4));
    // result.append("Sound: ").append(soundState != 0 ? "enabled" : "disabled");
    // AppState.getInstance().changeDeviceSoundState(soundState != 0);
    break;
  case '1':
    // int shouldWaitFirstLap = Integer.parseInt(chunk.substring(3, 4));
    // result.append("Wait First Min Lap: ").append(shouldWaitFirstLap != 0 ? "enabled" : "disabled");
    // AppState.getInstance().changeSkipFirstLap(shouldWaitFirstLap != 1);
    break;
  case 'L':
    // int lapNo = Integer.parseInt(chunk.substring(3, 5), 16);
    // int lapTime = Integer.parseInt(chunk.substring(5, 13), 16);
    // result.append("Lap #").append(lapNo).append(" : ").append(lapTime);
    // AppState.getInstance().addLapResult(moduleId, lapNo, lapTime);
    break;
  case 'B':
    device->band = strtol(data + 1, NULL, 16);
    printf("Device: %d, band: %d\n", id, device->band);
    break;
  case 'J':
    // int adjustment = (int)Long.parseLong(chunk.substring(3, 11), 16);
    // result.append("TimeAdjustment: ").append(adjustment != 0 ? "done" : "not performed");
    // boolean isCalibrated = adjustment != 0;
    // AppState.getInstance().changeCalibration(moduleId, isCalibrated);
    break;
  case 't':
    // long deviceTime = Long.parseLong(chunk.substring(3, 11), 16);
    // result.append("device time: ").append(deviceTime);
    // AppState.getInstance().changeDeviceCalibrationTime(moduleId, deviceTime);
    break;
  case 'I':
    // int isMonitorOn = Integer.parseInt(chunk.substring(3, 7), 16);
    // result.append("RSSI Monitor: ").append(isMonitorOn != 0 ? "on" : "off");
    // AppState.getInstance().changeRssiMonitorState(isMonitorOn != 0);
    break;
  case 'H':
    // int thresholdSetupState = Integer.parseInt(chunk.substring(3, 4), 16);
    // result.append("Threshold setup state: ").append(thresholdSetupState);
    // AppState.getInstance().changeThresholdSetupState(moduleId, thresholdSetupState);
    break;
  case 'x':
    // result.append("EndOfSequence. Device# ").append(moduleId);
    // AppState.getInstance().receivedEndOfSequence(moduleId);
    //send_cmd("R*I0064\n");
    device->setupComplete = true;
    break;
  case 'y':
    // int isDeviceConfigured = Integer.parseInt(chunk.substring(3, 4), 16);
    // result.append("Device is configured: ").append(isDeviceConfigured != 0 ? "yes" : "no");
    // AppState.getInstance().changeDeviceConfigStatus(moduleId, (isDeviceConfigured != 0));
    break;
  case 'v':
    // int voltageReading = Integer.parseInt(chunk.substring(3, 7), 16);
    // result.append("Voltage(abstract): ").append(voltageReading);
    // AppState.getInstance().changeVoltage(voltageReading);
    break;
  case '#':
    device->apiVersion = strtol(data + 1, NULL, 16);
    printf("Device: %d, api: %d\n", id, device->apiVersion);
    break;
  }
}

void handle_message(char *msg, int len)
{
  char cmd = msg[0];
  char value = msg[1];
  //printf("RX: %c,%c\n", cmd, value);
  int count = 0;

  switch (cmd)
  {
  case 'N':
    count = value - '0';
    printf(" modules: %d\n", count);

    state.deviceCount = count;

    send_cmd("R*#\n");
    send_cmd("R*a\n");

    break;
  case 'S':
    count = value - '0';
    handle_set_message(count, msg + 2, len - 2);
    break;
  }
}

#define READ_STATE_MARKER 0
#define READ_STATE_HEADER 1
#define READ_STATE_PARSE_MSG 2
#define READ_STATE_RESET 3

static char buffer[256];
static char msg[256];

static void rx_task()
{
  int maxSize = 256;

  buffer[255] = 0;

  //int size = 0;
  int read = 0;
  int readSize = 0;
  int readState = READ_STATE_MARKER;

  char *marker = 0;
  char *start = &buffer[0];

  while (1)
  {
    switch (readState)
    {
    case READ_STATE_MARKER:
      read = uart_read_bytes(config->uart, ((uint8_t *)&buffer[0]) + readSize, maxSize - readSize, 100 / portTICK_RATE_MS);
      if (read == 0)
      {
        read = 0;
        break;
      }

      readSize += read;
      printf("read: %d\n", readSize);
      readState = READ_STATE_HEADER;
      break;

    case READ_STATE_HEADER:
      marker = strchr(&buffer[0], '\n');
      readState = marker ? READ_STATE_PARSE_MSG : READ_STATE_MARKER;
      break;

    case READ_STATE_PARSE_MSG:
      while (marker)
      {
        char *pmsg = &msg[0];
        int msgLen = marker - start;
        if (msgLen > 0)
        {
          memcpy(pmsg, start, msgLen);
          pmsg[msgLen] = 0;

          printf("marker: %s, %d\n", pmsg, msgLen);
          handle_message(pmsg, msgLen);
        }
        start = marker + 1;
        marker = strchr(start, '\n');
      }

      readState = READ_STATE_RESET;
      break;

    case READ_STATE_RESET:
      readSize = 0;
      read = 0;
      start = &buffer[0];
      memset(&buffer[0], 0, sizeof(buffer));
      readState = READ_STATE_MARKER;
      break;
    }
  }
}
