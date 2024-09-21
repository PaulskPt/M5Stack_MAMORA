/*
*  Test sketch for M5Stack 1.3 inch OLED SH1107 display with 128 x 64 pixels
*  and RTC unit (8563)
*  by @PaulskPt 2024-09-19
*  OLED unit and RTC unit via a USB hub connected to PortA of a M5Stack AtomPortABC module with on top a M5Stack Atom Matrix
*
* About i2c_scan with following units connected to Atrom PortABC, Port A: RTC unit and OLED unit:
* I2C device found at address 0x3c !  = OLED unit
* I2C device found at address 0x51 !  = RTC unit
* I2C device found at address 0x68 !  = 6-axis IMU sensor (MPU6886) (builtin the ATOM Matrix)
*/
#include <WiFi.h>
#include <TimeLib.h>
#include <time.h>
#include <M5Atom.h>
#include <Unit_RTC.h>
#include <M5UnitOLED.h>
//#include <Wire.h>
#include "secret.h"

#define SCL 21
#define SDA 25
#define I2C_FREQ 400000
#define I2C_PORT 0
#define I2C_ADDR_OLED 0x3c
#define I2C_ADDR_RTC  0x51

#define WIFI_SSID     SECRET_SSID // "YOUR WIFI SSID NAME"
#define WIFI_PASSWORD SECRET_PASS //"YOUR WIFI PASSWORD"
#define NTP_TIMEZONE  SECRET_NTP_TIMEZONE // "JST-9"
#define NTP_SERVER1   SECRET_NTP_SERVER_1 // "0.pool.ntp.org"
#define NTP_SERVER2   "1.pool.ntp.org"
#define NTP_SERVER3   "2.pool.ntp.org"

bool use_local_time = false; // for the external RTC    (was: use_local_time = true // for the ESP32 internal clock )

uint8_t FSM = 0;  // Store the number of key presses
int connect_try = 0;
int max_connect_try = 10;

// Different versions of the framework have different SNTP header file names and availability.
#if __has_include (<esp_sntp.h>)
  #include <esp_sntp.h>
  #define SNTP_ENABLED 1
#elif __has_include (<sntp.h>)
  #include <sntp.h>
  #define SNTP_ENABLED 1
#endif

#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif

Unit_RTC RTC(I2C_ADDR_RTC);

rtc_time_type RTCtime;
rtc_date_type RTCdate;

char str_buffer[64];

// Atom Matrix (ESP32-PICO) with a 5 * 5 RGB LED matrix panel (WS2812C-2020)
#define BUTTON_PIN 39
#define NEOPIXEL_LED_PIN 27
#define IR_PIN 12

M5UnitOLED display(SDA, SCL, I2C_FREQ, I2C_PORT, I2C_ADDR_OLED);

M5Canvas canvas(&display);

static constexpr const char* wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
char text[50];
size_t textlen = 0;
int textpos = 0;
int scrollstep = 2;

uint8_t DisBuff[2 + 5 * 5 * 3];  // Used to store RGB color values.

void setBuff(uint8_t Rdata, uint8_t Gdata,
             uint8_t Bdata) {  // Set the colors of LED, and save the relevant
                               // data to DisBuff[].  设置RGB灯的颜色
    DisBuff[0] = 0x05;
    DisBuff[1] = 0x05;
    for (int i = 0; i < 25; i++) {
        DisBuff[2 + i * 3 + 0] = Rdata;
        DisBuff[2 + i * 3 + 1] = Gdata;
        DisBuff[2 + i * 3 + 2] = Bdata;
    }
}

void upd_dt(void)
{
  RTC.getTime(&RTCtime);
  RTC.getDate(&RTCdate);

  constexpr char sDate[] = "RTC Date Now is: ";
  constexpr char sTime[] = "RTC Time Now is: ";

  sprintf(str_buffer, "%02d-%02d-%02d %3s\n",
    RTCdate.Year, RTCdate.Month, RTCdate.Date, wd[RTCdate.WeekDay]);

  Serial.print(sDate);
  Serial.println(str_buffer);
  canvas.clear();
  canvas.setCursor(0, 15);
  canvas.print(str_buffer);

  sprintf(str_buffer, "%02d:%02d:%02d utc\n", 
    RTCtime.Hours, RTCtime.Minutes, RTCtime.Seconds);
  Serial.print(sTime);
  Serial.println(str_buffer);
  canvas.setCursor(0,45);
  canvas.print(str_buffer);

  display.waitDisplay();
  canvas.pushSprite(&display, 0, (display.height() - canvas.height()) >> 1);
}

void chg_matrix_clr(void)
{
 switch (FSM) 
    {
      case 0:
          setBuff(0x40, 0x00, 0x00);
          break;
      case 1:
          setBuff(0x00, 0x40, 0x00);
          break;
      case 2:
          setBuff(0x00, 0x00, 0x40);
          break;
      case 3:
          setBuff(0x20, 0x20, 0x20);
          break;
      case 4:
          setBuff(0x00, 0x00, 0x00);  // BLACK (OFF)
          break;
      default:
          break;
    }
    //M5.dis.displaybuff(DisBuff);
    M5.dis.displaybuff(DisBuff);

    FSM++;
    if (FSM >= 5)
      FSM = 0;
    // delay(50);
}

bool connect_NTP(void)
{
  bool ret = false;
  Serial.print("NTP:");
#if SNTP_ENABLED
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nSNTP IS ENABLED");
#else
  delay(1600);
  struct tm timeInfo;
  while (!getLocalTime(&timeInfo, 1000))
  {
    Serial.print('.');
  };
  Serial.print(F("\nSNTP IS NOT ENABLED"));
  Serial.print(F("timeInfo received from NTP server: "));
  Serial.println(timeInfo);
#endif
  ret = true;
  Serial.println(F("\r\nNTP Connected."));
  return ret;
}

bool connect_WiFi(void)
{
  bool ret = false;
  Serial.print("\nWiFi:");
  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );

  for (int i = 20; i && WiFi.status() != WL_CONNECTED; --i)
  {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) 
  {
    ret = true;
    Serial.print(F("\r\nWiFi Connected to: "));
    Serial.println(WIFI_SSID);
    IPAddress ip;
    ip = WiFi.localIP();
    Serial.print(F("IP address: "));
    Serial.println(ip);
  }
  else
  {
    Serial.println(F("\r\nWiFi connection failed."));
  }
  return ret;
}

bool set_RTC(void)
{
  bool ret = false;
  constexpr char s[] = "\nset_RTC(): external RTC ";
  if (WiFi.status() == WL_CONNECTED)
  {
    time_t t = time(nullptr)+1; // Advance one second.
    while (t > time(nullptr));  /// Synchronization in seconds

    struct tm *gmt = gmtime(&t);

    // see: https://github.com/m5stack/M5Unit-RTC/blob/master/examples/Unit_RTC_M5Atom/Unit_RTC_M5Atom.ino
    RTCtime.Hours   = gmt->tm_hour;
    RTCtime.Minutes = gmt->tm_min;
    RTCtime.Seconds = gmt->tm_sec;

    RTCdate.Year    = gmt->tm_year + 1900;
    RTCdate.Month   = gmt->tm_mon + 1;
    RTCdate.Date    = gmt->tm_mday;
    RTCdate.WeekDay = gmt->tm_wday;  // 0 = Sunday, 1 = Monday, etc.

    RTC.setDate(&RTCdate);
    RTC.setTime(&RTCtime);
   
    Serial.print(s);
    Serial.print(F("set\n"));
    M5.dis.fillpix(0x00ff00);  // show GREEN matrix
    ret = true;
  }
  else
  {
    Serial.print(s);
    Serial.println(F("not set\n"));
    M5.dis.fillpix(0xff0000);  // show RED matrix
  }
  return ret;
}

void setup(void) 
{
  // bool SerialEnable, bool I2CEnable, bool DisplayEnable
  // Not using I2CEnable because of fixed values for SCL and SDA in M5Atom.begin:
  M5.begin(true, false, true);  // Init Atom-Matrix(Initialize serial port, LED).

  setBuff(0xff, 0x00, 0x00);

  M5.dis.displaybuff(DisBuff);  // Display the DisBuff color on the LED.
  
  // See: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/unit/oled.pdf
  display.init();
  display.setRotation(1);
  canvas.setColorDepth(1); // mono color
  canvas.setFont(&fonts::FreeSans9pt7b); // was: efontCN_14);
  canvas.setTextWrap(false);
  canvas.setTextSize(1);
  canvas.createSprite(display.width() + 64, 72);

  Serial.begin(9600);

  RTC.begin();

  // setup RTC ( NTP auto setting )
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  delay(1000);

  if (connect_WiFi())
  {
    if (connect_NTP())
      set_RTC();
  }
  else
    connect_try++;

  canvas.clear();
}

void loop(void)
{
  if (WiFi.status() != WL_CONNECTED) // Check if we're still connected to WiFi
  {
    if (connect_WiFi())
    {
      connect_try = 0;  // reset count
      if (connect_NTP())
        set_RTC();
    }
    else
    {
      connect_try++;
    }
  }
  if (connect_try >= max_connect_try)
  {
    Serial.print(F("\nWiFi connect try failed "));
    Serial.print(connect_try);
    Serial.println(F("time. Going into infinite loop...\n"));
    for(;;)
    {
      delay(5000);
    }
  }

  //showlog(&RTCtime, &RTCdate);
  upd_dt();

  if (M5.Btn.wasPressed()) // Check if the key is pressed.
  {
    Serial.println(F("\nButton pressed\n"));
    chg_matrix_clr();
  }
  delay(1000);  // Wait 1 seconds
  M5.update();  // Read the press state of the key.
}

