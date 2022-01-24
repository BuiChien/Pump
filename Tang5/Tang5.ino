#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define DELAY_TIME  1000
#define AVG_TIME    180000
#define AVG_SAMPLE  AVG_TIME / DELAY_TIME
 
const char* ssid = "MinhThu";
const char* password = "XuJog2230";
int gpio = 4;
int send_val;
StaticJsonBuffer<200> jsonBuffer;
WiFiClient client;
JsonObject& root = jsonBuffer.createObject();
char json_str[100];
int httpCode;
int avgIndex = 0;
int avgSample[AVG_SAMPLE];
int blinkLed = 0;
int cal_avg_sample(int val) {
  int ret = 0;
  avgSample[avgIndex] = val;
  avgIndex++;
  if(avgIndex >= AVG_SAMPLE) {
    avgIndex = 0;
  }
  for(int i = 0; i < AVG_SAMPLE; i++) {
    if(avgSample[i] == 1) {
      ret++;
      break;
    }
  }
  return ret;
}

void setup() 
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pinMode(gpio, INPUT_PULLUP);
  root["id"] = 1;
  root["gpio"] = 5;
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting...");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Connected");
}

void loop() 
{
  int readVal = digitalRead(gpio);
  Serial.println(String("readVal: ") + readVal);
  send_val = cal_avg_sample(readVal == 1 ? 0 : 1);
  if(send_val == 1) {
      digitalWrite(LED_BUILTIN, blinkLed);
      blinkLed++;
      if(blinkLed > 1) {
        blinkLed = 0;
      }
  } else {
      digitalWrite(LED_BUILTIN, HIGH);
  }
  if (WiFi.status() == WL_CONNECTED) 
  {
    root["status"] = send_val;
    Serial.print("Send status: ");
    root.printTo(Serial);
    Serial.println();
    HTTPClient http;    //Declare object of class HTTPClient
    http.setTimeout(2000);
    http.begin(client, "http://esp8266.local/leds");      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    memset(json_str, 0, sizeof(json_str));
    root.prettyPrintTo(json_str, sizeof(json_str)); //Send the request
    httpCode = http.PUT(json_str);
    if(httpCode != 200) {
      Serial.println("Send POST");
      http.POST(json_str);
    } else {
      Serial.println("PUT success");
    }
  } else {
    Serial.print("....");
  }
  delay(DELAY_TIME);
}
