#include <M5EPD.h>
#include "humi.jpg.c"
#include "lamp.jpg.c"
#include "projector.jpg.c"
#include "temperature.jpg.c"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "pass.h"

WiFiMulti wifiMulti;
WiFiClient espClient;
PubSubClient client(espClient);
HTTPClient http;

const char* ssid = W_SSID;
const char* password = W_PASS;
const char* mqtt_server = M_ADDR;
gpio_num_t down_buttom_gpio_num = GPIO_NUM_39;

// flags
int flg_mqtt = 0;
int flg_his_mqtt = 0;
int flg_print = 0;
int flg_wifi = 0;
char flg_server_config = '1';

RTC_DATA_ATTR uint bootCount = 0;

// display constant
#define W_LETTER (20)
#define H_LETTER (28)
#define W_SCREEN (960)
#define H_SCREEN (540)
#define FSTROW (40)
#define SNDROW (FSTROW + 260)
#define TRDROW (H_SCREEN - 12 - 150)
#define Y_TSTRING (SNDROW)
#define Y_HSTRING (FSTROW)

#define FSTCOL (12)
#define SNDCOL (FSTCOL + 260)
#define TRDCOL (SNDCOL + 260)
#define FRTCOL (W_SCREEN - 12 - 260)
#define X_TSTRING (SNDCOL)
#define X_HSTRING (TRDCOL)

#define Y_BTYSTRING (H_SCREEN - 12)
#define X_BTYSTRING (FRTCOL)
#define Y_BOTSTRING (Y_BTYSTRING - 32)
#define X_SLPSTRING (X_BTYSTRING)

#define Y_TOUCHINFO (Y_BOTSTRING - H_LETTER)
#define Y_HTICON (Y_TSTRING - 136)
#define Y_BTR1ICON (60)
#define Y_BTR2ICON (Y_BTR1ICON + 128 + 90)

// define timers
#define TIME_LOOP_DELAY_MS (50)
#define TIME_1S_LOOP (1000 / TIME_LOOP_DELAY_MS)
#define TIMER_MAX_HT (6 * TIME_1S_LOOP) // * 60 // MINS
#define TIMER_MAX_BATTERY (6 * TIME_1S_LOOP) // * 60 // MINS
#define TIMER_MAX_WIFI (6 * TIME_1S_LOOP)
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_WAKE  (10 * TIME_1S_LOOP)        /* Time ESP32 will wake up (in seconds) */
#define TIME_TO_SLEEP (200 * TIME_1S_LOOP) //
#define TIME_MQTT_HEAT_DELAY_MS (20 * TIME_1S_LOOP)

#define WAKE_TIMER (2)
#define WAKE_BOOT (0)
#define WAKE_MANUAL (1)

// define buttons
#define NUM_BUTTONS 6

// timers with 50 ms unit, 1200 = 1 min, 20 = 1 sec
int mcount = 0;
int decount = 700; // mqtt online count
int bacount = TIMER_MAX_BATTERY;
int htcount = TIMER_MAX_HT;
int wake_count = 0; // wake up counter, awake for TIME_TO_SLEEP then sleep
int wificount = TIMER_MAX_WIFI +1;
int int_rwake = WAKE_BOOT; //reason of wake up, timer or IO

// measures
char temStr[5];
char humStr[5];
float tem;
float hum;

uint32_t battery_level = 0;

// display var
M5EPD_Canvas canvas(&M5.EPD);

// MQTT var
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(128)
char msg[MSG_BUFFER_SIZE];
String str_msg;

// botton config
String bt_conf_pic[NUM_BUTTONS] = {"0",};

String bt_conf_func[NUM_BUTTONS] = {"0"};

String bt_conf_topic[NUM_BUTTONS] = {"0"};

String bt_conf_cmd[NUM_BUTTONS] = {"0"};

// touch screen points
int t_point[2][2];

//
struct t_area {
    int x1, x2, y1, y2;
};

const struct t_area Butt1 = {X_HTTSTRING - 8, X_HTTSTRING + 124 + 8, Y_BTR1ICON - 8, Y_BTR1ICON + 124 + 8};
const struct t_area Butt2 = {X_HTHSTRING - 8, X_HTHSTRING + 124 + 8, Y_BTR1ICON - 8, Y_BTR1ICON + 124 + 8};
const struct t_area Butt3 = {X_HTTSTRING - 8, X_HTTSTRING + 124 + 8, Y_BTR2ICON - 8, Y_BTR2ICON + 124 + 8};
const struct t_area Butt4 = {X_HTHSTRING - 8, X_HTHSTRING + 124 + 8, Y_BTR2ICON - 8, Y_BTR2ICON + 124 + 8};
const struct t_area ButtTH = {X_HTTSTRING - 8, X_HTHSTRING + 124 + 8, Y_HTICON - 8, Y_HTSTRING + H_LETTER + 8};

// functions
void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reConnectMQTT();
void drawBackground();
bool check_click(const struct t_area* ar_touched);
void checkMQTT();
void wakeup_setup();
void start_sleep();
int get_server_config();
void load_sd_config();

int print_wakeup_reason();
void print_info(String msg, unsigned int x, unsigned int y);