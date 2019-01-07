#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266OTA.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"
 
#define BUTTON_PIN    0
#define LED_PIN       10
#define DEBOUNCE_MS   50
#define LONG_PRESS_MS 5000

DFRobot_SHT20     sht20;
ESP8266WebServer  httpServer(80);
ESP8266OTA        otaUpdater;
HTTPClient        httpClient;

static unsigned long previousMillis = 0;
static unsigned long previousButtonMillis = 0;
static boolean       buttonActive = false;
static boolean       buttonPress = false;
static boolean       configMode = false;
static char          ssid[32];
static char          passwd[128];
static char          sensorid[16];
static char          rest_url[512];
static char          sha1_fingerprint[512];
static unsigned int  interval_ms = 10000;

void setup(void) {

  // set up serial console
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");

  // set up pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // set up SHT20
  sht20.initSHT20();

  // try to read configuration and connect to wifi
  // if config cannot be read, set config mode
  read_config();  
  
}

void prepare_config_mode(void) {
  
  // set up OTAUpdater
  otaUpdater.setTitle("Firmware Update");
  otaUpdater.setBanner("Salesforce IoT Gizmo");
  otaUpdater.setBranch("");
  otaUpdater.setBuild("Build : 20181216");
  otaUpdater.setup(&httpServer);

  // set up DNS for captive portal
  
  // start http server
  httpServer.begin();
}

void read_config() {
  // read SPFISS
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
    
    if (currentMillis - previousMillis >= interval_ms) {  
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
      httpClient.begin("https://iotdemosa1-developer-edition.eu16.force.com/services/apexrest/iotservice?temperature=" + String(temp) + "&humidity=" + String(humd) + + "&button=" + String(buttonPress) + "&sensorid=0000001", "83 A4 EF 08 7A 7A 1C B8 58 B4 26 E7 B1 AD 45 71 66 FA 0F 7D");
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

    httpServer.handleClient();
  }
}
