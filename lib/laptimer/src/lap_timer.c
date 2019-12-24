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

#include "display_controller.h"
#include "lap_timer.h"
#include "timers.h"
#include "signal_detect.h"
#include "filters.h"
#include "udp_send.h"
#include "webserver.h"
#include "rx_controller.h"
#include "mongoose.h"
#include "cJSON.h"

typedef struct
{
  QueueHandle_t readTimerLock;
} TimerState_t;

static LapTimerConfig_t *config;
static TimerState_t state;
static PilotLapData_t allPilotLapData[MAX_RX_COUNT];
static signal_data_t signal;
static NetMessage_t *udpMsg = NULL;

static WebRequestHandler_t statusHandler;
static WebRequestHandler_t commandHandler;
static WebSocketDataHandler_t pilotsCommandHandler;

static void rx_task();
static char web_buffer[8192];

void lapTimerTask(void *arg);
void lapTimerDisplayTask(void *arg);

void statusCallback(struct mg_connection *nc, struct http_message *hm)
{
  char *start = &web_buffer[0];
  start += sprintf(start, "<html><body><h1>Devices</h1><p>%d</p>", config->pilotCount);
  start += sprintf(start, "<table>");
  start += sprintf(start, "<th>Id</th>");
  start += sprintf(start, "<th>Band</th>");
  start += sprintf(start, "<th>Channel</th>");
  start += sprintf(start, "<th>Threshold</th>");
  start += sprintf(start, "<th>RSSI</th>");

  RssiReading_t *rssi_readings = rssiReadings();
  for (int c = 0; c < config->pilotCount; ++c)
  {
    PilotConfig_t *pilot = &config->pilots[c];
    PilotLapData_t *device = &allPilotLapData[c];
    start += sprintf(start, "<tr>");
    start += sprintf(start, "<td>%d</td>", c);
    start += sprintf(start, "<td>%d</td>", pilot->band);
    start += sprintf(start, "<td>%d</td>", pilot->channel);
    start += sprintf(start, "<td>%d</td>", pilot->threshold);
    start += sprintf(start, "<td>%f</td>", rssi_readings[c].filtered);

    start += sprintf(start, "</tr>");
  }

  start += sprintf(start, "</table>");
  sprintf(start, "</body></html>");
  int len = strlen(&web_buffer[0]);
  mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/html\r\nContent-Length: %d\r\n\r\n%.*s", len, len, &web_buffer[0]);
}

bool lapTimerPilotCommand(cJSON *command_json, cJSON *resp)
{
  cJSON *value = cJSON_GetObjectItemCaseSensitive(command_json, "id");
  if (value == NULL)
  {
    cJSON_AddNumberToObject(resp, "result", -1);
    return true;
  }

  int id = value->valueint;

  if (id < 0 || id >= config->pilotCount)
  {
    cJSON_AddNumberToObject(resp, "result", -1);
    return true;
  }

  printf("pilot.id: %d\n", id);
  PilotConfig_t *pilot = &config->pilots[id];
  if ((value = cJSON_GetObjectItemCaseSensitive(command_json, "band")) != NULL)
    pilot->band = value->valueint;

  printf("pilot.band: %d\n", pilot->band);
  if ((value = cJSON_GetObjectItemCaseSensitive(command_json, "channel")) != NULL)
    pilot->channel = value->valueint;

  printf("pilot.threshold: %d\n", pilot->threshold);
  if ((value = cJSON_GetObjectItemCaseSensitive(command_json, "threshold")) != NULL)
    pilot->threshold = value->valueint;

  printf("rx set state: %d, %d, %d\n", id, pilot->band, pilot->channel);
  rxSetState(id, pilot->band, pilot->channel);
  cJSON_AddNumberToObject(resp, "pilot", pilot->threshold);
  return true;
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
    if (strcmp(command->valuestring, "pilot") == 0)
    {
      printf("command: pilot\n");

      lapTimerPilotCommand(command_json, resp);
    }
  }

  cJSON_PrintPreallocated(resp, web_buffer, sizeof(web_buffer), true);
  int len = strlen(&web_buffer[0]);

  printf("write response\n");
  mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%.*s", len, len, &web_buffer[0]);

  cJSON_Delete(command);
  cJSON_Delete(resp);
}

void lapTimerCommandHandler(struct mg_connection *nc, cJSON *data)
{
  cJSON *type = cJSON_GetObjectItem(data, "type");
  if (type == NULL)
  {
    printf("Unknown command\n");
    return;
  }

  cJSON *resp = cJSON_CreateObject();

  if (strcmp(type->valuestring, "pilot") == 0)
  {
    lapTimerPilotCommand(data, resp);
  }

  cJSON_PrintPreallocated(resp, web_buffer, sizeof(web_buffer), true);
  int len = strlen(&web_buffer[0]);

  mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, &web_buffer[0], len);

  cJSON_Delete(resp);
}

void lapTimerInit(LapTimerConfig_t *info)
{
  config = info;

  // init sub modules
  rssiInit(&config->rssiReader);
  rxInit(&config->rxController);

  statusHandler.callback = &statusCallback;
  statusHandler.path = "/status";
  statusHandler.request = HTTP_GET;

  commandHandler.callback = &commandCallback;
  commandHandler.path = "/command";
  commandHandler.request = HTTP_GET;

  webserverRegister(&commandHandler);
  webserverRegister(&statusHandler);

  pilotsCommandHandler.callback = &lapTimerCommandHandler;
  pilotsCommandHandler.command = "pilot";
  webserverWSRegister(&pilotsCommandHandler);

  state.readTimerLock = xSemaphoreCreateBinary();

  memset(&signal, 0, sizeof(signal));
  signal.alpha = lpfAlpha(50, 1.0f / config->updateHz);
  signal.threshold = 5;
  signal.influence = 0;

  timerInit(TIMER_1, TIMER_GROUP_0, state.readTimerLock, true, 1.0f / config->updateHz);

  xTaskCreate(lapTimerTask, "lapTimerTask", 1024 * 3, NULL, 10, NULL);
  xTaskCreate(lapTimerDisplayTask, "lapTimerDisplayTask", 1024 * 3, NULL, 10, NULL);
}

void lapTimerSetupPilotRx()
{
  for (int p = 0; p < config->pilotCount; ++p)
  {
    PilotConfig_t *pilot = &config->pilots[p];
    rxSetState(
        pilot->id,
        pilot->band,
        pilot->channel);
  }
}

void lapTimerSetup()
{
  //assert(config->pilotCount == config->rssiReader.channelCount);
  memset(&allPilotLapData, 0, sizeof(allPilotLapData));

  lapTimerSetupPilotRx();
}

bool lapTimerUpdatePilot(PilotConfig_t *pilot, PilotLapData_t *lapData, float rssi, uint32_t now)
{
  // potential passing
  uint32_t last = lapData->timesCount ? lapData->timestamps[lapData->timesCount - 1] : 0;
  uint32_t lapTime = now - last;

  bool lap = false;
  float threshold = pilot->threshold / 4095.0f;
  //float s = signal_detect(&signal, rssi);

  switch (pilot->state)
  {
  case LAP_STATE_LOW:
    if (rssi >= threshold)
    {
      lapData->timestamps[lapData->timesCount++] = now;
      pilot->state = LAP_STATE_HIGH;

      if (lapData->timesCount > 1)
      {
        // A lap occurred
        float seconds = (float)lapTime / 1000;
        lap = true;
        //printf("Pilot: %u, lap: %f, signal: %f\n", pilot->id, seconds, s);

        lapData->times[lapData->timesCount - 2] = lapTime;
      }
    }
    break;

  case LAP_STATE_DROP_WAIT:
  case LAP_STATE_HIGH:
    if (rssi < threshold * 0.75f)
      pilot->state = LAP_STATE_LOW;
    break;
  }

  return lap;
}

void lapTimerTask(void *arg)
{
  lapTimerSetup();

  uint32_t updateCount = 0;
  while (1)
  {
    if (xSemaphoreTake(state.readTimerLock, portMAX_DELAY) != pdTRUE)
      continue;
    //assert(config->pilotCount == config->rssiReader.channelCount);

    RssiReading_t *rssi_readings = rssiReadings();
    uint32_t now = millis();

    bool update = false;

    for (int i = 0; i < config->pilotCount; ++i)
    {
      PilotConfig_t *pilot = &config->pilots[i];
      float rssi = rssi_readings[i].filtered;

      PilotLapData_t *lapData = &allPilotLapData[i];
      update |= lapTimerUpdatePilot(pilot, lapData, rssi, now);
    }

    if (!update)
      continue;

    update = false;
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "type", "lap");
    cJSON *pilots = cJSON_CreateArray();
    cJSON_AddItemToObject(msg, "pilots", pilots);

    for (int i = 0; i < config->pilotCount; ++i)
    {
      PilotConfig_t *pilot = &config->pilots[i];
      PilotLapData_t *lapData = &allPilotLapData[i];

      if (pilot->state != LAP_STATE_HIGH || lapData->timestamps[lapData->timesCount - 1] != now || lapData->timesCount < 2)
        continue;

      update = true;
      pilot->state = LAP_STATE_DROP_WAIT;
      uint32_t lapTime = lapData->times[lapData->timesCount - 2];
      printf("LapTime: %d:%u: %u, %f\n", i, lapData->timesCount - 1, lapTime, (float)lapTime / 1000.0f);

      cJSON *data = cJSON_CreateObject();
      cJSON_AddItemToArray(pilots, data);

      cJSON_AddNumberToObject(data, "pilot", i);
      cJSON_AddNumberToObject(data, "count", lapData->timesCount - 1);
      cJSON_AddNumberToObject(data, "time", lapTime);
    }

    if (update)
    {
      webServerBroadcastJson(msg);
    }

    cJSON_Delete(msg);
    // TODO: queue readings
  }
}

void lapTimerDisplayTask(void *arg)
{
  int tri[3][2] = {
      {16, 16},
      {16, 0},
      {0, 0},

  };
  //memset(tri, 0, sizeof(tri));

  //    render_initialize(DISPLAY_WIDTH, DISPLAY_HEIGHT);

  int y = 5;
  char buf[128 / 8];

  while (1)
  {
    //render_begin_frame();

    //render_draw();

    // displayDrawLine(
    //     DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1,
    //     0, 0,
    //     1
    // );

    uint32_t now = millis();

    RssiReading_t *rssi_readings = rssiReadings();

    int percent = (int)((rssi_readings->filtered) * DISPLAY_WIDTH);

    for (int x = 0; x < percent; ++x)
    {
      displayDraw(x, y, 1);
      displayDraw(x, y + 1, 1);
    }

    
    sprintf(buf, "%d", (int)(rssi_readings->filtered * 10));
    displayDrawString(1, 17, buf); 

    PilotConfig_t *pilot = &config->pilots[0];
    PilotLapData_t *lapData = &allPilotLapData[0];

    sprintf(
        buf, "%c%u:%d:%.2f",
        rxGetBandShortName(pilot->band),
        pilot->channel,
        rxGetFrequency(pilot->band, pilot->channel),
        (now - lapData->timestamps[lapData->timesCount - 1]) / 1000.0f);
    displayDrawString(8, 17, buf); 

    if (lapData->timesCount > 1)
    {
      int lapCount = lapData->timesCount - 1;
      int lapTime = lapData->times[lapData->timesCount - 2];
      sprintf(buf, "%d:%0.2f", lapCount, lapTime / 1000.0f);
      displayDrawString(1, 27, buf); 
    }


    sprintf(buf, "%u", (int)rssi_readings->raw);
    displayDrawString(1, 37, buf); 

    displayUpdate();

    vTaskDelay(0);
    displayClear();
  }
}
