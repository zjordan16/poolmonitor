#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>

#define ssid "Zach iPhone"
#define password "utdbaseball16"
#define ph_address 99
#define orp_address 98

byte code = 0;
char ph_data[20];
char ORP_data[20];
byte in_char = 0;
byte i = 0;
int time_delay = 900;
float ph_float, ORP_float;

// Temperature stuff
OneWire oneWire(12);
DallasTemperature sensors(&oneWire);

// Timestamp stuff
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

// Run upload test in setup() so that it only runs once.
void setup() {                                                                          
  Serial.begin(9600);
  delay(500);
  Wire.begin();
  
  // Get temp reading
  Serial.println("Getting temperature reading...");
  sensors.begin();
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  Serial.print("Temp reading = ");
  Serial.println(temp);

  // Get pH reading
  Serial.println("Getting pH reading...");
  phReading();
  float ph = atof(ph_data);
  
  // Get ORP reading
  Serial.println("Getting ORP reading...");
  orpReading();
  float orp = atof(ORP_data);

  // Connect to wifi and post.
  Serial.println("Connecting to wifi...");
  wifi_connect();
  
  // Get timestamp for database upload
  Serial.println("Getting timestamp...");
  timeClient.begin();
  timeClient.update();
  String timestamp = timeClient.getFormattedDate();
  Serial.print("Timestamp: ");
  Serial.println(timestamp);

  // Upload to mongo
  Serial.println("Posting to mongo...");
  mongo_post(temp, orp, ph, timestamp);

  ESP.deepSleep(20e6);
}

void loop() {
}

void wifi_connect() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Wifi connected..");
}

void mongo_post(float temp, float orp, float ph, String timestamp) {
  String jsonData = String("{\"dataSource\":\"Cluster1\",\"database\":\"SeniorDesign\",\"collection\":\"PrimaryCollection\",\"document\":{\"temp\":") + String(temp) + ",\"orp\":" + String(orp) + ",\"ph\":" + String(ph) + ",\"time\":\"" + timestamp + "\"}}";

  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("data.mongodb-api.com", 443)) {
    Serial.println("HTTPS connection failed.");
    return;
  }

  Serial.println("Sending HTTP Request...");
  HTTPClient http;
  http.begin(client, "https://data.mongodb-api.com/app/data-uakhq/endpoint/data/beta/action/insertOne");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(jsonData.length()));
  http.addHeader("api-key", "J901fqVTO4msIiVpMxcFpWbOwtoqV1IJVeg4yvKH0layFixMX8N4TVzMbEDkWEpm");
  http.addHeader("Access-Control-Request-Headers", "*");
  int httpResponseCode = http.POST(jsonData); 
  http.end();
  
  Serial.println(String("HTTP Response Code: ") + httpResponseCode);
}

void phReading() {
  Wire.beginTransmission(ph_address);
  Wire.write('r');
  Wire.endTransmission();
  delay(time_delay);

  Wire.requestFrom(ph_address, 20, 1);
  code = Wire.read();

  switch (code) {
    case 1:
      Serial.println("pH Reading - Success");
      break;
    case 2:
      Serial.println("pH Reading - Failed");
      break;
    case 254:
      Serial.println("pH Reading - Pending");
      break;
    case 255:
      Serial.println("pH Reading - No Data");
      break;
  }
  while (Wire.available()) {
    in_char = Wire.read();
    ph_data[i] = in_char;
    i += 1;
    if (in_char == 0) {
      i = 0;
      break;
    }
  }

  Wire.beginTransmission(ph_address);
  Wire.write("sleep");
  Wire.endTransmission();
  
  Serial.print("pH = ");
  Serial.println(ph_data);
}
void orpReading(){
  Wire.beginTransmission(orp_address);
  Wire.write('r');
  Wire.endTransmission();
  delay(time_delay);
  
  Wire.requestFrom(orp_address, 20, 1);
  
  code = Wire.read();
  switch (code) {
    case 1:
      Serial.println("ORP Reading - Success");
      break;
    case 2:
      Serial.println("ORP Reading - Failed");
      break;
    case 254:
      Serial.println("ORP Reading - Pending");
      break;
    case 255:
      Serial.println("ORP Reading - No Data");
      break;
  }
  
  while (Wire.available()) {
    in_char = Wire.read();
    ORP_data[i] = in_char;
    i += 1;
    if (in_char == 0) {
      i = 0;
      break;
    }
  }

  Wire.beginTransmission(orp_address);
  Wire.write("sleep");
  Wire.endTransmission();
  
  Serial.print("ORP = ");
  Serial.println(ORP_data);
}
