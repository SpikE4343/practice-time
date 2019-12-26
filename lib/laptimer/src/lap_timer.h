
#ifndef __lap_timer_INCLUDED__
#define __lap_timer_INCLUDED__

#include "driver/gpio.h"
#include "driver/adc.h"
#include "rssi_reader.h"
#include "rx_controller.h"

#define MAX_LAPS 32

#define LAP_STATE_LOW 0
#define LAP_STATE_HIGH 1
#define LAP_STATE_UPDATE 2
#define LAP_STATE_DROP_WAIT 3

typedef struct
{
  uint8_t id;
  uint8_t band;
  uint8_t channel;
  uint16_t threshold;
  uint8_t state;
} PilotConfig_t;

typedef struct
{
  uint32_t updateHz;
  uint8_t pilotCount;
  uint16_t minLapTime;

  PilotConfig_t pilots[MAX_RX_COUNT];
  RssiReaderConfig_t rssiReader;
  RxControllerConfig_t rxController;
} LapTimerConfig_t;

typedef struct
{
  uint16_t timesCount;
  uint32_t times[MAX_LAPS];
  uint32_t timestamps[MAX_LAPS];
} PilotLapData_t;

void lapTimerInit(LapTimerConfig_t *info);
void lapTimerUpdatePilotConfig(PilotConfig_t *pilot);

#endif
