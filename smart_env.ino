/*
   Aquatan smart env monitor

   API
   /status
   /config
     room_id = int
   /wifireset
   /reset
   /reboot
*/
#include "smart_env.h" // Configuration parameters

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <OneWire.h>
#include <RTClib.h>
#include <SPI.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <pgmspace.h>

#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>
#include <DallasTemperature.h>

#include <LCD_ST7032.h>
//#include "Ambient.h"
#include "ntp.h"

const String boolstr[2] = {"false", "true"};
const char *apSSID = "WIFI_ENV_TAN";

const char *localserver = "mowatmirror.local";
const int localport = 3000;

unsigned int amb_channelId = 5960;             // AmbientのチャネルID
const char *amb_writeKey = "22147df5c1a201ac"; // ライトキー

String website_name = "mowatenv";
boolean settingMode;

int8_t env_id = 0;
int8_t e_temp = 0;
int8_t e_humi = 0;
int8_t e_pres = 0;
int8_t e_co2 = 0;

uint32_t timer_count = 0;
uint32_t p_millis;

DNSServer dnsServer;
MDNSResponder mdns;
const IPAddress apIP(192, 168, 1, 1);
ESP8266WebServer webServer(80);

OneWire oneWire(PIN_DS);
DallasTemperature ds18b20(&oneWire);
Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

LCD_ST7032 lcd;

RTC_Millis rtc;
WiFiClient client;
// Ambient ambient;
NTP ntp("ntp.nict.jp");

float tempc, humid, pressure, co2;
float tempc_hist[300];
float humid_hist[300];
float pressure_hist[300];
float co2_hist[300];
uint16_t tempc_hist_idx = 0, humid_hist_idx = 0, pressure_hist_idx =0, co2_hist_idx =0;

void setup() {
  ESP.wdtDisable();

  for (int i=0;i<HIST_MAX;i++) {
    tempc_hist[i] = -500;
    humid_hist[i] = -500;
    pressure_hist[i] = -500;
    co2_hist[i] = -500;
  }

  p_millis = millis();
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);

  SPIFFS.begin();
  rtc.begin(DateTime(2017, 1, 1, 0, 0, 0));
#ifdef DEBUG
  Serial.println("RTC began");
#endif

  Wire.begin(PIN_SDA, PIN_SCL);
  lcd.begin();
  // lcd.setcontrast(12);
  lcd.clear();

  uint8_t charmap[8];
  // Pa
  charmap[0] = 0b11100;
  charmap[1] = 0b10010;
  charmap[2] = 0b11100;
  charmap[3] = 0b10011;
  charmap[4] = 0b10101;
  charmap[5] = 0b10101;
  charmap[6] = 0b10111;
  charmap[7] = 0b00000;
  lcd.createchar(0x00, charmap);
  // pp
  charmap[0] = 0b11000;
  charmap[1] = 0b10100;
  charmap[2] = 0b11000;
  charmap[3] = 0b10110;
  charmap[4] = 0b10101;
  charmap[5] = 0b00110;
  charmap[6] = 0b00100;
  charmap[7] = 0b00100;
  lcd.createchar(0x01, charmap);
  // pm
  charmap[0] = 0b11100;
  charmap[1] = 0b10010;
  charmap[2] = 0b11100;
  charmap[3] = 0b10001;
  charmap[4] = 0b11011;
  charmap[5] = 0b10101;
  charmap[6] = 0b10001;
  charmap[7] = 0b00000;
  lcd.createchar(0x02, charmap);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Booting up...");

  ds18b20.begin();
  if (ds18b20.getDS18Count() > 0) {
    e_temp = 1;
#ifdef DEBUG
    Serial.println("DS18B20 enabled.");
#endif
    lcd.setCursor(1, 0);
    lcd.print("D");
  }
  if (bme.begin(I2C_BME280_ADDRESS)) {
    e_temp = 2;
    e_humi = 1;
    e_pres = 1;
#ifdef DEBUG
    Serial.println("BME280 enabled.");
#endif
    lcd.setCursor(1, 2);
    lcd.print("B");
  }
  delay(200);
  if (ccs.begin(I2C_CCS811_ADDRESS)) {
    e_co2 = 1;
#ifdef DEBUG
    Serial.println("CCS811 enabled.");
#endif
    ccs.readData();
    lcd.setCursor(1, 4);
    lcd.print("C");
  }
  delay(200);
  WiFi.persistent(false);
  settingMode = true;
  WiFi.mode(WIFI_STA);
  if (restoreConfig()) {
    if (checkConnection()) {
      setupArduinoOTA();
#ifdef DEBUG
      Serial.println("Arduino OTA began.");
#endif
      settingMode = false;
    }
  }

  if (settingMode == true) {
    lcd.setCursor(0, 0);
    lcd.print("Setting mode");
    delay(200);
#ifdef DEBUG
    Serial.println("Setting mode");
#endif
    //    WiFi.mode(WIFI_STA);
    //    WiFi.disconnect();
#ifdef DEBUG
    Serial.println("Wifi disconnected");
#endif
    delay(100);
    WiFi.mode(WIFI_AP);
#ifdef DEBUG
    Serial.println("Wifi AP mode");
#endif
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID);
    dnsServer.start(53, "*", apIP);

#ifdef DEBUG
    Serial.println("Wifi AP configured");
#endif
    startWebServer_setting();
#ifdef DEBUG
    Serial.print("Starting Access Point at \"");
    Serial.print(apSSID);
    Serial.println("\"");
#endif

    lcd.setCursor(1, 0);
    lcd.print("AP:" + String(apSSID));
  } else {
    ntp.begin();
    //    ambient.begin(amb_channelId, amb_writeKey, &client);
    delay(200);
    startWebServer_normal();
#ifdef DEBUG
    Serial.println("Starting normal operation.");
#endif
  }
  ESP.wdtEnable(WDTO_8S);
}

void loop() {
  int pause = 0;
  int temp;
  char str[10];
  DateTime now;

  ESP.wdtFeed();
  if (settingMode) {
    dnsServer.processNextRequest();
  } else {
    ArduinoOTA.handle();
  }
  webServer.handleClient();

  if (!settingMode) {
    // 100ms秒毎に実行
    if (millis() > p_millis + INTERVAL) {
      p_millis = millis();
      if (timer_count % (3600 * (1000/INTERVAL)) == 0) {
        uint32_t epoch = ntp.getTime();
        if (epoch > 0) {
          rtc.adjust(DateTime(epoch + SECONDS_UTC_TO_JST));
        }
#ifdef DEBUG
        Serial.print("epoch:");
        Serial.println(epoch);
#endif
      }
      now = rtc.now();

      // 2秒おき
      if (timer_count % (2 * (1000/INTERVAL)) == 0) {
        // 温度計測
        if (e_temp == 1) {
          ds18b20.requestTemperatures();
          tempc = ds18b20.getTempCByIndex(0);

#ifdef DEBUG
          Serial.print("temp:");
          Serial.println(tempc);
#endif
          if (tempc < -50.0) {
            e_temp = 0;
          } else {
            e_temp = 1;
            tempc_hist[tempc_hist_idx++] = tempc;
            tempc_hist_idx %= HIST_MAX;
          }

          lcd.setCursor(0, 0);
          lcd.print(tempc, 1);
          lcd.write(0xdf);
          lcd.setCursor(0, 5);
          lcd.print("C ");
        }
        if (e_temp == 2) {
          tempc = bme.readTemperature();          
          tempc_hist[tempc_hist_idx++] = tempc;
          tempc_hist_idx %= HIST_MAX;
#ifdef DEBUG
          Serial.print("temp(bme):");
          Serial.println(tempc);
#endif
          lcd.setCursor(0, 0);
          lcd.print(tempc, 1);
          lcd.write(0xdf);
          lcd.setCursor(0, 5);
          lcd.print("C ");
        }
        if (e_humi) {
          humid = bme.readHumidity();
#ifdef DEBUG
          Serial.print("humid:");
          Serial.println(humid);
#endif
          humid_hist[humid_hist_idx++] = humid;
          humid_hist_idx %= HIST_MAX;
          lcd.setCursor(0, 7);
          lcd.print(humid, 1);
          lcd.print("% ");
        }
        if (e_pres) {
          pressure = bme.readPressure() / 100.0;
#ifdef DEBUG
          Serial.print("press:");
          Serial.println(pressure);
#endif
          pressure_hist[pressure_hist_idx++] = pressure;
          pressure_hist_idx %= HIST_MAX;
          lcd.setCursor(1, 0);
          sprintf(str, "%4d", int(pressure));
          lcd.print(str);
          lcd.setCursor(1, 4);
          lcd.print("h");
          lcd.setCursor(1, 5);
          lcd.write(0x00);
          //          lcd.print("hPa");
        }
        if (e_co2) {
          while (!ccs.available())
            ;
          delay(100);
          if (!ccs.readData()) {
            ccs.setEnvironmentalData(humid, tempc);
            //      Serial.print("eCO2: ");
            co2 = ccs.geteCO2();
#ifdef DEBUG
            Serial.print("co2:");
            Serial.println(co2);
#endif
            co2_hist[co2_hist_idx++] = co2;
            co2_hist_idx %= HIST_MAX;
            lcd.setCursor(1, 7);
            sprintf(str, "%4d", int(co2));
            lcd.print(str);
            lcd.setCursor(1, 11);
            lcd.write(0x01);
            lcd.setCursor(1, 12);
            lcd.print("m");
            //            lcd.write(0x01);
            //            lcd.print("ppm");
          } else {
            delay(100);
#ifdef DEBUG
            Serial.print("co2 error:");
            Serial.println(ccs.readData());
#endif
            ccs.readData();
            ccs.setEnvironmentalData(humid, tempc);
            //      Serial.print("eCO2: ");
            co2 = ccs.geteCO2();

            co2_hist[co2_hist_idx++] = co2;
            co2_hist_idx %= HIST_MAX;

            lcd.setCursor(1, 7);
            sprintf(str, "%4d", int(co2));
            lcd.print(str);
            lcd.setCursor(1, 11);
            lcd.write(0x01);
            lcd.setCursor(1, 12);
            lcd.print("m");
          }
        }
        sprintf(str, "%02d", now.hour());
        lcd.setCursor(0, 14); // LINE 1, ADDRESS 14
        lcd.print(str);
        sprintf(str, "%02d", now.minute());
        lcd.setCursor(1, 14); // LINE 2, ADDRESS 14
        lcd.print(str);

#ifdef DEBUG
        Serial.println(env_id);
#endif

        if (timer_count % (300 * (1000/INTERVAL)) == (150 * (1000/INTERVAL))) {
          if (env_id > 0) {
#ifdef DEBUG
            Serial.println("posted");
#endif
            if (e_temp)
              post_data(env_id, "temperature", mean(tempc_hist,tempc_hist_idx));
            if (e_humi)
              post_data(env_id, "humidity", mean(humid_hist,humid_hist_idx));
            if (e_pres)
              post_data(env_id, "pressure", mean(pressure_hist,pressure_hist_idx));
            if (e_co2)
              post_data(env_id, "co2", mean(co2_hist,co2_hist_idx));
          }
        }
      }

      delay(1);
      timer_count++;
      timer_count %= (86400UL) * (1000/INTERVAL);
    }
  }
  delay(10);
}

/***************************************************************
   EEPROM restoring functions
 ***************************************************************/

boolean restoreConfig() {
#ifdef DEBUG
  Serial.println("Reading EEPROM...");
#endif
  String ssid = "";
  String pass = "";

  // Initialize on first boot
  if (EEPROM.read(0) == 255) {
#ifdef DEBUG
    Serial.println("Initialize EEPROM...");
#endif
    for (int i = 0; i < EEPROM_LAST_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
#ifdef DEBUG
    Serial.println("Erasing EEPROM...");
#endif
    EEPROM.commit();
  }

#ifdef DEBUG
  Serial.println("Reading EEPROM(2)...");
#endif

  env_id = EEPROM.read(EEPROM_ID_ADDR);

  if (EEPROM.read(EEPROM_SSID_ADDR) != 0) {
    for (int i = EEPROM_SSID_ADDR; i < EEPROM_SSID_ADDR + 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    for (int i = EEPROM_PASS_ADDR; i < EEPROM_PASS_ADDR + 64; ++i) {
      pass += char(EEPROM.read(i));
    }
#ifdef DEBUG
    Serial.print("ssid:");
    Serial.println(ssid);
    Serial.print("pass:");
    Serial.println(pass);
#endif
    delay(100);
    WiFi.begin(ssid.c_str(), pass.c_str());
#ifdef DEBUG
    Serial.println("WiFi started");
#endif
    delay(100);
    if (EEPROM.read(EEPROM_MDNS_ADDR) != 0) {
      website_name = "";
      for (int i = 0; i < 32; ++i) {
        byte c = EEPROM.read(EEPROM_MDNS_ADDR + i);
        if (c == 0) {
          break;
        }
        website_name += char(c);
      }
#ifdef DEBUG
      Serial.print("website name:");
      Serial.println(website_name);
#endif
    }
    return true;
  } else {
#ifdef DEBUG
    Serial.println("restore config fails...");
#endif
    return false;
  }
}

/***************************************************************
   Web server functions
 ***************************************************************/

/*
 * Web server for normal operation
 */
void startWebServer_normal() {
#ifdef DEBUG
  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.localIP());
#endif
  webServer.on("/", handleRoot);
  webServer.on("/pure.css", handleCss);
  webServer.on("/reset", handleAllReset);
  webServer.on("/wifireset", handleWiFiReset);
  webServer.on("/reboot", handleReboot);
  webServer.on("/status", handleStatus);
  webServer.on("/config", handleConfig);
  webServer.begin();
}

void handleConfig() {
  String message, argname, argv;
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

  for (int i = 0; i < webServer.args(); i++) {
    argname = webServer.argName(i);
    argv = webServer.arg(i);
#ifdef DEBUG
    Serial.print("argname:");
    Serial.print(argname);
    Serial.print(" = ");
    Serial.println(argv);
#endif
    if (argname == "id") {
      env_id = argv.toInt();
      EEPROM.write(EEPROM_ID_ADDR, char(env_id));
      EEPROM.commit();
    }
  }
  json["id"] = env_id;
  json["enable_temp"] = boolstr[(e_temp > 0)];
  json["enable_humi"] = boolstr[e_humi];
  json["enable_pres"] = boolstr[e_pres];
  json["enable_co2"] = boolstr[e_co2];
  json["timestamp"] = timestamp();
  json.printTo(message);
  webServer.send(200, "application/json", message);
}

void handleStatus() {
  String message;
#ifdef DEBUG
  Serial.println("status command");
#endif
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["id"] = env_id;
  json["temperature"] = tempc;
  json["pressure"] = pressure;
  json["humidity"] = humid;
  json["co2"] = co2;
  json["timestamp"] = timestamp();
  json.printTo(message);
  webServer.send(200, "application/json", message);
}

void handleRoot() { send_fs("/index.html", "text/html"); };

void handleCss() { send_fs("/pure.css", "text/css"); };

void handleWiFiReset() {
  for (int i = 0; i < EEPROM_MDNS_ADDR; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  String s = "<h3 class=\"if-head\">Reset WiFi</h3><p>Cleared WiFi settings. "
             "Please reset device.</p>";
  s += "<p><button class=\"pure-button\" onclick=\"return "
       "quitBox();\">Close</button></p>";
  s += "<script>function quitBox() { open(location, '_self').close();return "
       "false;}</script>";
  webServer.send(200, "text/html", makePage("Reset WiFi Settings", s));
  timer_count = 0;
  ESP.restart();
  while (1) {
    delay(0);
  }
}

void handleAllReset() {
  for (int i = 0; i < EEPROM_LAST_ADDR; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  String s = "<h3 class=\"if-head\">Reset ALL</h3><p>Cleared all settings. "
             "Please reset device.</p>";
  s += "<p><button class=\"pure-button\" onclick=\"return "
       "quitBox();\">Close</button></p>";
  s += "<script>function quitBox() { open(location, '_self').close();return "
       "false;};</script>";
  webServer.send(200, "text/html", makePage("Reset ALL Settings", s));
  timer_count = 0;
  ESP.restart();
  while (1) {
    delay(0);
  }
}

void handleReboot() {
  String message;
  message = "{reboot:\"done\"}";
  webServer.send(200, "application/json", message);
  ESP.restart();
  while (1) {
    delay(0);
  }
}

/*******************************************
 * 
 * HEREAFTER, LESS TO BE CHANGED 
 * 
 * 
 * 
 * 
 * 
 ******************************************** */


/* Web server (setting mode) */

void startWebServer_setting() {
#ifdef DEBUG
  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.softAPIP());
#endif
  webServer.on("/pure.css", handleCss);
  webServer.on("/setap", []() {
#ifdef DEBUG
    Serial.print("Set AP ");
    Serial.println(WiFi.softAPIP());
#endif
    for (int i = 0; i < EEPROM_MDNS_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    String ssid = urlDecode(webServer.arg("ssid"));
    String pass = urlDecode(webServer.arg("pass"));
    String site = urlDecode(webServer.arg("site"));
    for (int i = 0; i < ssid.length(); ++i) {
      EEPROM.write(EEPROM_SSID_ADDR + i, ssid[i]);
    }
    for (int i = 0; i < pass.length(); ++i) {
      EEPROM.write(EEPROM_PASS_ADDR + i, pass[i]);
    }
    if (site != "") {
      for (int i = EEPROM_MDNS_ADDR; i < EEPROM_MDNS_ADDR + 32; ++i) {
        EEPROM.write(i, 0);
      }
      for (int i = 0; i < site.length(); ++i) {
        EEPROM.write(EEPROM_MDNS_ADDR + i, site[i]);
      }
    }
    EEPROM.commit();
    String s = "<h2>Setup complete</h2><p>Device will be connected to \"";
    s += ssid;
    s += "\" after the restart.</p><p>Your computer also need to re-connect to "
         "\"";
    s += ssid;
    s += "\".</p><p><button class=\"pure-button\" onclick=\"return "
         "quitBox();\">Close</button></p>";
    s += "<script>function quitBox() { open(location, '_self').close();return "
         "false;};setTimeout(\"quitBox()\",10000);</script>";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    timer_count = 0;
    ESP.restart();
    while (1) {
      delay(0);
    }
  });
  webServer.onNotFound([]() {
    String ssidList;
#ifdef DEBUG
    Serial.println("captive webpage ");
#endif
    int n = WiFi.scanNetworks();
    delay(100);
    ssidList = "";
    for (int i = 0; i < n; ++i) {
      ssidList += "<option value=\"";
      ssidList += WiFi.SSID(i);
      ssidList += "\">";
      ssidList += WiFi.SSID(i);
      ssidList += "</option>";
    }
    String s = R"=====(
<div class="l-content">
<div class="l-box">
<h3 class="if-head">WiFi Setting</h3>
<p>Please enter your password by selecting the SSID.<br />
You can specify site name for accessing a name like http://aquamonitor.local/</p>
<form class="pure-form pure-form-stacked" method="get" action="setap" name="tm"><label for="ssid">SSID: </label>
<select id="ssid" name="ssid">
)=====";
    s += ssidList;
    s += R"=====(
</select>
<label for="pass">Password: </label><input id="pass" name="pass" length=64 type="password">
<label for="site" >Site name: </label><input id="site" name="site" length=32 type="text" placeholder="Site name">
<button class="pure-button pure-button-primary" type="submit">Submit</button></form>
</div>
</div>
)=====";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  });
  webServer.begin();
}

/***************************************************************
   Network functions
 ***************************************************************/

boolean checkConnection() {
  int count = 0;
  while (count < 20) {
    ESP.wdtFeed();
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
    count++;
  }
#ifdef DEBUG
  Serial.println("Timed out.");
#endif
  return false;
}

/***********************************************************
   WiFi Client functions

 ***********************************************************/
void post_data(int room, String label, float value) {
  if (client.connect(localserver, localport)) {
    // Create HTTP POST Data
    String postData;
    char str[64];

    //    postData = "room=" + String(room) + "&label=" + label + "&value=" +
    //    value;
    sprintf(str, "room=%d&label=%s&value=%4.1f", room, label.c_str(), value);
    postData += str;

    client.print("POST /api/v1/add HTTP/1.1\n");
    client.print("Host: ");
    client.print(localserver);
    client.print("\n");
    client.print("Connection: close\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postData.length());
    client.print("\n\n");

    client.print(postData);
    client.stop();
  }
}

/*
  Utilities
*/

String timestamp() {
  String ts;
  DateTime now = rtc.now();
  char str[20];
  sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(),
          now.day(), now.hour(), now.minute(), now.second());
  ts = str;
  return ts;
}

String timestamp_YYDDMMhhmm() {
  String ts;
  DateTime now = rtc.now();
  char str[20];
  sprintf(str, "%04d-%02d-%02d %02d:%02d", now.year(), now.month(), now.day(),
          now.hour(), now.minute());
  ts = str;
  return ts;
}

void send_fs(String path, String contentType) {
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = webServer.streamFile(file, contentType);
    file.close();
  } else {
    webServer.send(500, "text/plain", "BAD PATH");
  }
}

String makePage(String title, String contents) {
  String s = R"=====(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<link rel="stylesheet" href="/pure.css">
)=====";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += R"=====(
<div class="footer l-box">
<p>Aquatan environment monitor by @omzn 2018</p>
</div>
)=====";
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

void setupArduinoOTA() {
  ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(website_name.c_str());
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
    // using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  delay(100);
}

float mean(float hist[], uint16_t idx) {
  uint16_t histmax = HIST_MAX;
  float result;
  if (hist[HIST_MAX - 1] == -500) {
    histmax = idx;
  }
  for (int i = 0; i < histmax; i++) {
    result += hist[i]; 
  }
  return (result / float(histmax));
}
