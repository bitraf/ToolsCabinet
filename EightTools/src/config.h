// https://docs.particle.io/reference/device-os/firmware/photon/#selectantenna-antenna-
STARTUP( WiFi.selectAntenna(ANT_AUTO) );
// https://docs.particle.io/reference/device-os/firmware/photon/#manual-mode
SYSTEM_MODE( MANUAL ); // MANUAL or AUTOMATIC

const int DOORS_IN_CABINET = 8;

#include "secret.h"
#include "TCA6408.h"
#include "MQTT.h"
#include "Adafruit_NeoPixel.h"
#include "DosonLock.h"

enum toolStates {
    TOOL_STATE_UNKNOWN,
    TOOL_STATE_CHECKOUT,
    TOOL_STATE_RETURNED
};
toolStates toolState;

bool isConnected;
void reconnectToWifi();
Timer reconnectTimer(7000, reconnectToWifi);

// Hardware config
const int relayPin[DOORS_IN_CABINET]    = {7   ,6   ,5   ,4   ,3   ,2   ,1   ,0};    // Controlled by a TCA6408      Doornumber: 1, 2, 3, 4, 5, 6, 7, 8
const int ledPin[DOORS_IN_CABINET]      = {P1S5,P1S4,D2  ,D3,  D4,  D5,  D7,  D6};   // Controlled directly by MCU due to timing requirement
//const int ledPin[DOORS_IN_CABINET]      = {D6  ,D7  ,D5  ,D4,  D3,  D2,P1S4,P1S5};   // Controlled directly by MCU due to timing requirement
const int sensorPin[DOORS_IN_CABINET]   = {A0  ,A1  ,A4  ,DAC ,P1S0,P1S1,P1S2,P1S3}; // Read directly by MCU. Must be analog pins
const int doorLockPin[DOORS_IN_CABINET] = {7   ,6   ,5   ,4   ,3   ,2   ,1   ,0};    // Controlled by a TCA6408 (output), 3V3 if closed, 0V if open
char selfSubscribeTopic[50];

TCA6408 ioExpanderOut;
TCA6408 ioExpanderInputs;

// MQTT config
static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
int alphanumLen = strlen( alphanum );
void callback(char* topic, byte* payload, unsigned int length);

MQTT mqttClient(MQTT_SERVER, 1883, callback);
char MQTT_CLIENT_ID[20];
char MQTT_STATUS_TOPIC[200];
char MQTT_OPEN_TOPIC[200];


// Visual feedback in each lock
#define PIXEL_COUNT 1
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip0(PIXEL_COUNT, ledPin[0], PIXEL_TYPE);
Adafruit_NeoPixel strip1(PIXEL_COUNT, ledPin[1], PIXEL_TYPE);
Adafruit_NeoPixel strip2(PIXEL_COUNT, ledPin[2], PIXEL_TYPE);
Adafruit_NeoPixel strip3(PIXEL_COUNT, ledPin[3], PIXEL_TYPE);
Adafruit_NeoPixel strip4(PIXEL_COUNT, ledPin[4], PIXEL_TYPE);
Adafruit_NeoPixel strip5(PIXEL_COUNT, ledPin[5], PIXEL_TYPE);
Adafruit_NeoPixel strip6(PIXEL_COUNT, ledPin[6], PIXEL_TYPE);
Adafruit_NeoPixel strip7(PIXEL_COUNT, ledPin[7], PIXEL_TYPE);

Adafruit_NeoPixel eightleds[8] = {strip0,strip1,strip2,strip3,strip4,strip5,strip6,strip7};

DosonLock eightLocks[8];


/* Debug stuff. remove */
int counter = 0;