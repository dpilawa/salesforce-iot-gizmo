#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266OTA.h>
#include <DNSServer.h>
#include <FS.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"

static const char     fw_build[]  = "0.1.0";
 
#define BUTTON_PIN    0
#define LED_PIN       10
#define DEBOUNCE_MS   50
#define LONG_PRESS_MS 5000
#define DNS_PORT      53
#define WIFI_ATTEMPTS 25
#define REBOOT_S      10

DFRobot_SHT20     sht20;
ESP8266WebServer  httpServer(80);
ESP8266OTA        otaUpdater;
DNSServer         dnsServer;
HTTPClient        httpClient;

static unsigned long previousMillis = 0;
static unsigned long previousButtonMillis = 0;
static boolean       buttonActive = false;
static boolean       buttonPress = false;
static boolean       configMode = false;
static char          ssid[32] = "";
static char          password[128] = "";
static char          sensorid[16] = "";
static char          rest_url[512] = "https://";
static char          sha1_fingerprint[512] = "";
static unsigned int  interval = 10;

static const char    config_host[]  = "iotgizmo";
static const char    config_fname[] = "/iotgizmo.cfg";

IPAddress ap_ip(192, 168, 5, 1);

void setup(void) {

  // set up serial console
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");
  Serial.printf("Build: %s\n", fw_build);

  // set up pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // set up SHT20
  sht20.initSHT20();

  // try to read configuration and connect to wifi
  // if config cannot be read, set config mode
  if (read_config()) { 
    
    Serial.println("Successfully read config...");
    
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.print("Sensor Id: ");
    Serial.println(sensorid);
    Serial.print("Interval: ");
    Serial.println(interval);
    Serial.print("REST url: ");
    Serial.println(rest_url);
    Serial.print("SHA1: ");
    Serial.println(sha1_fingerprint);


    WiFi.softAPdisconnect(true);
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");
    for (int i=0; i<=WIFI_ATTEMPTS; i++) {
      if (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        Serial.print(".");
      }
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("error");
      configMode = true;
      prepare_config_mode();
    }
    else {
      Serial.println("OK");
    }
  }
  else {
    configMode = true;
    prepare_config_mode();
  }
}

void prepare_config_mode(void) {

  // light a LED
  digitalWrite(LED_PIN, HIGH);
  
  // set up OTAUpdater
  otaUpdater.setTitle("Firmware Update");
  otaUpdater.setBanner("Salesforce IoT Gizmo");
  otaUpdater.setBuild(fw_build);
  otaUpdater.setup(&httpServer);

  // set up access point
  WiFi.persistent(false);
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.persistent(true);

  Serial.print("Disconnecting STA...");
  while (WiFi.status() == WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK");
    
  String ap_ssid = "iotgizmo_" + String(ESP.getChipId());
  WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255, 255, 255, 0));
  Serial.print("Setting AP...");
  Serial.println(WiFi.softAP(ap_ssid.c_str()) ? "Ready" : "Failed!");
  
  Serial.print("Config mode AP: ");
  Serial.println(ap_ssid);
  Serial.print("Config mode IP: ");
  Serial.println(WiFi.softAPIP());

  // set up DNS for captive portal
  // dnsServer.start(DNS_PORT, config_host, ap_ip);  
  dnsServer.start(DNS_PORT, "*", ap_ip); 
  
  // start http server
  httpServer.on("/", config_form);
  httpServer.on("/reboot", handle_reboot);
  httpServer.onNotFound(redirect);
  httpServer.begin();
}

int read_config() {

  String s;
  
  // read SPFISS
  if (SPIFFS.begin()) {
    Serial.println("Mounted SPIFFS filesystem...");
    File f = SPIFFS.open(config_fname, "r");
    if (!f) {
      Serial.println("File open failed");
      return 0;
    } else {
      s = f.readStringUntil('\n');
      strcpy(ssid, s.c_str());

      s = f.readStringUntil('\n');
      strcpy(password, s.c_str());

      s = f.readStringUntil('\n');
      strcpy(sensorid, s.c_str());

      s = f.readStringUntil('\n');
      interval = s.toInt();

      s = f.readStringUntil('\n');
      strcpy(rest_url, s.c_str());

      s = f.readStringUntil('\n');
      strcpy(sha1_fingerprint, s.c_str());

      f.close();
    }
  }
  else {
    return 0;
  }
  return 1;
}

void redirect() {

  char html[256];

  snprintf(html, 256, "<!DOCTYPE HTML><html><head><meta http-equiv='refresh' content='2; url=http://%s/'></head></html>", config_host);
 
  httpServer.send(200, "text/html", html);
}

void config_form() {

  char html[1024];

  snprintf(html, 1024,
  "<!DOCTYPE HTML><html><body>"
  "<p>Salesforce IoT Gizmo - Configuration</p>"
  "<p>Build: %s</p>"
  "<form action='/' method='get'>"
  "SSID: <input type='text' name='ssid' value='%s'><br>"
  "Password: <input type='text' name='password' value='%s'><br>"
  "Sensor ID: <input type='text' name='sensorid' value='%s'><br>"
  "Interval (s): <input type='number' name='interval' min='1' max='3600' value='%d'><br>"
  "REST url: <input type='text' name='rest_url' value='%s'><br>"
  "SHA1 fingerprint: <input type='text' name='sha1_fingerprint' value='%s'><br>"  
  "<input type='hidden' name='s' value='1'><br>"
  "<input type='submit' value='Submit'>"
  "</form></body></html>",
  fw_build,
  ssid, password, sensorid, interval, rest_url, sha1_fingerprint);
  
  if (httpServer.hasArg("s")) {
    handle_form();
  }
  else {
    httpServer.send(200, "text/html", html);
  }
  
}

void handle_form() {

  char html[1024];
  
  strcpy(ssid, httpServer.arg("ssid").c_str());
  strcpy(password, httpServer.arg("password").c_str());
  strcpy(sensorid, httpServer.arg("sensorid").c_str());
  interval = httpServer.arg("interval").toInt();
  strcpy(rest_url, httpServer.arg("rest_url").c_str());
  strcpy(sha1_fingerprint, httpServer.arg("sha1_fingerprint").c_str());  

  // save SPIFFS
  SPIFFS.begin();
  File f = SPIFFS.open(config_fname, "w");
  if (!f) {
    Serial.println("File open failed");
  } else {
    f.printf("%s\n%s\n%s\n%d\n%s\n%s\n", ssid, password, sensorid, interval, rest_url, sha1_fingerprint);
    f.close();
    snprintf(html, 1024, "<!DOCTYPE HTML><html><head><meta http-equiv='refresh' content='%d; url=http://%s/reboot'></head><body><p>Configuration saved. Device will reboot in %d seconds.</p><a href='http://%s/'>Return</a></body></html>", REBOOT_S, config_host, REBOOT_S, config_host);
  }
 
  httpServer.send(200, "text/html", html);
  
}

void handle_reboot() {

  ESP.restart();
  
}


void loop(void){
  
  unsigned long currentMillis = millis();

  if (configMode == false) {
  
    // check if mode of operation should be changed
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (buttonActive == false) {
        buttonActive = true;
        previousButtonMillis = currentMillis;
      }
      if (currentMillis - previousButtonMillis > LONG_PRESS_MS) {
        Serial.println("Entering config mode...");
        // prepare config mode
        configMode = true;
        prepare_config_mode();
      }
    } else {
      if ((buttonActive == true) && (currentMillis - previousButtonMillis > DEBOUNCE_MS)) {
        buttonPress = true;
      }
      buttonActive = false;
    }
    
    if ((currentMillis - previousMillis >= interval * 1000) && configMode == false) {  
      previousMillis = currentMillis;
  
      // blink a LED to indicate we are alive
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
  
      // read data from SHT20 sensor
      float humd = sht20.readHumidity();                  
      float temp = sht20.readTemperature();              
  
      Serial.print("Temperature:");
      Serial.print(temp, 1);
      Serial.print("C");
      Serial.print(" Humidity:");
      Serial.print(humd, 1);
      Serial.print("%");
      Serial.print(" Light:");
      Serial.print(analogRead(A0), 1);
      Serial.print("V");
      Serial.println();
  
      // call salesforce webservice
      httpClient.begin(String(rest_url) + "?temperature=" + String(temp) + "&humidity=" + String(humd) + "&button=" + String(buttonPress) + "&sensorid=" + String(sensorid), sha1_fingerprint);
      int httpCode = httpClient.GET();
      if(httpCode > 0) {
        Serial.printf("[HTTP] %d: ", httpCode);
        if(httpCode == HTTP_CODE_OK) {
          String payload = httpClient.getString();
          Serial.println(payload);
        } 
      } else {
        Serial.printf("[HTTP] error: %s\n", httpClient.errorToString(httpCode).c_str());
      }
      httpClient.end();
      
    }

  } else {

    // config mode
    
    dnsServer.processNextRequest();
    httpServer.handleClient();
  }
}
