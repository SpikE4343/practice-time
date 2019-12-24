//
#include "wifi_controller.h"
#include "webserver.h"
#include "lap_timer.h"
#include "udp_send.h"
#include "display_controller.h"

static LapTimerConfig_t config;
static WifiConfig_t wifiConfig;
static ChorusControllerConfig_t chorusConfig;
static WebServerConfig_t webConfig;
static RxControllerConfig_t rxConfig;
static UdpSendConfig_t udpSendConfig;
static DisplayControllerConfig_t display;

#define COUNT 1

void app_main()
{
  wifiConfig.ssid = "lap-timer";
  wifiConfig.password = "txmeansvagina";
  wifiConfig.maxConnections = 5;

  sprintf(&webConfig.port[0], "80");
  config.pilotCount = COUNT;
  config.minLapTime = 5000;
  config.pilots[0].id = 0;
  config.pilots[0].band = 0;
  config.pilots[0].channel = 5;
  config.pilots[0].threshold = 3900;
  config.updateHz = 1000;

  config.rssiReader.lpfCutoffHz = 50;
  config.rssiReader.lpf2CutoffHz = 150;
  config.rssiReader.updateHz = 10000;
  config.rssiReader.channelCount = COUNT;
  config.rssiReader.bitWidth = ADC_WIDTH_12Bit;
  config.rssiReader.attenuation = ADC_ATTEN_DB_2_5;
  config.rssiReader.channels[0] = ADC1_CHANNEL_4;

  config.rxController.rxCount = 1;
  config.rxController.spiClockSpeed = 8000000;
  config.rxController.spiClockPin = GPIO_NUM_25;
  config.rxController.spiOutputPin = GPIO_NUM_27;
  config.rxController.spiSelectPin[0] = GPIO_NUM_26;

  udpSendConfig.host = "192.168.4.2";
  udpSendConfig.port = 1234;

  display.updateDelay = 100;

  wifiInit(&wifiConfig);
  webserverInit(&webConfig);
  udpSendInit(&udpSendConfig);

  displayInit(&display);
  //wsClientInit(&wsConfig);
  lapTimerInit(&config);
}