//
#include "wifi_controller.h"
#include "webserver.h"
#include "lap_timer.h"
#include "udp_send.h"
#include "display_controller.h"
#include "flashFS.h"

static LapTimerConfig_t config;
static WifiConfig_t wifiConfig;
//static ChorusControllerConfig_t chorusConfig;
static WebServerConfig_t webConfig;
static RxControllerConfig_t rxConfig;
static UdpSendConfig_t udpSendConfig;
static DisplayControllerConfig_t display;

#define COUNT 2

void app_main()
{
  FlashFSConfig_t files = {
      .partition = "storage",
      .root = "/spiffs",
      .maxOpenFiles = 5};

  flashFSInit(&files);

  strcpy(wifiConfig.ssid, "practice-timer");
  strcpy(wifiConfig.password, "on-the-tone");

  static 
  LapTimerConfig_t cfg = {
    .pilotCount = COUNT,
    .pilots = {
      {.band = 0, .channel=8, .id=0},
      {.band = 0, .channel=2, .id=1}
    },
    .minLapTime = 5000,
    .updateHz = 1000,
    .rxController = {
      .rxCount = COUNT,
      .spiClockSpeed = 8000000,
      .spiClockPin = GPIO_NUM_25,
      .spiOutputPin = GPIO_NUM_27,
      .devices = {
        {.spiSelectPin=GPIO_NUM_12 },
        {.spiSelectPin=GPIO_NUM_14 }
      }
    },
    .rssiReader = {
      .calibrationSec = 1,
      .lpfCutoffHz = 20,
      .lpf2CutoffHz = 50,
      .updateHz = 10000,
      .channelCount = COUNT,
      .bitWidth = ADC_WIDTH_12Bit,
      .attenuation = ADC_ATTEN_DB_2_5,
      .channels = {
        ADC1_CHANNEL_0,
        ADC1_CHANNEL_1
      }
    }
  };

  wifiConfig.maxConnections = 5;

  sprintf(&webConfig.port[0], "80");


  display.updateDelay = 250;

  //wifiInit(&wifiConfig);
  //webserverInit(&webConfig);
  //udpSendInit(&udpSendConfig);

  displayInit(&display);
  //wsClientInit(&wsConfig);
  lapTimerInit(&cfg);
}