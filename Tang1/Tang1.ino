/*
  ESP8266 mDNS responder sample

  This is an example of an HTTP server that is accessible
  via http://esp8266.local URL thanks to mDNS responder.

  Instructions:
  - Update WiFi SSID and password as necessary.
  - Flash the sketch to the ESP8266 board
  - Install host software:
    - For Linux, install Avahi (http://avahi.org/).
    - For Windows, install Bonjour (http://www.apple.com/support/bonjour/).
    - For Mac OSX and iOS support is built in through Bonjour already.
  - Point your browser to http://esp8266.local, you should see a response.

*/


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "PhanLe2"
#define STAPSK  "XuJog2230"
#endif

#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

unsigned long last_running_ = -1;

struct Led {
    byte id;
    byte gpio;
    byte status;
} led_resource;

const char* ssid = STASSID;
const char* password = STAPSK;

// TCP server at port 80 will respond to HTTP requests
ESP8266WebServer server(80);

void init_led_resource(){
    led_resource.id = 0;
    led_resource.gpio = 0;
    led_resource.status = LOW;
}

void get_leds() {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];

    if (led_resource.id == 0)
        server.send(204);
    else {
        jsonObj["id"] = led_resource.id;
        jsonObj["gpio"] = led_resource.gpio;
        jsonObj["status"] = led_resource.status;
        jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        server.send(200, "application/json", JSONmessageBuffer);
    }
}

void json_to_resource(JsonObject& jsonBody) {
    int id, gpio, status;

    id = jsonBody["id"];
    gpio = jsonBody["gpio"];
    status = jsonBody["status"];

    Serial.println(id);
    Serial.println(gpio);
    Serial.println(status);

    led_resource.id = id;
    led_resource.gpio = gpio;
    led_resource.status = status;
}

void post_put_leds() {
    StaticJsonBuffer<500> jsonBuffer;
    String post_body = server.arg("plain");
    Serial.println(post_body);

    JsonObject& jsonBody = jsonBuffer.parseObject(server.arg("plain"));

    Serial.print("HTTP Method: ");
    Serial.println(server.method());
    
    if (!jsonBody.success()) {
        Serial.println("error in parsin json body");
        server.send(400);
    }
    else {   
        if (server.method() == HTTP_POST) {
            if ((jsonBody["id"] != 0) && (jsonBody["id"] != led_resource.id)) {
                json_to_resource(jsonBody);
                server.sendHeader("Location", "/leds/" + String(led_resource.id));
                server.send(201);
                pinMode(led_resource.gpio, OUTPUT);
            }
            else if (jsonBody["id"] == 0)
              server.send(404);
            else if (jsonBody["id"] == led_resource.id)
              server.send(409);
        }
        else if (server.method() == HTTP_PUT) {
            if (jsonBody["id"] == led_resource.id) {
                json_to_resource(jsonBody);
                server.sendHeader("Location", "/leds/" + String(led_resource.id));
                server.send(200);
                if(led_resource.status == 1) {
                  last_running_ = millis();  
                }
                digitalWrite(led_resource.gpio, led_resource.status);
            }
            else
              server.send(404);
        }
    }
}

void config_rest_server_routing() {
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    server.on("/leds", HTTP_GET, get_leds);
    server.on("/leds", HTTP_POST, post_put_leds);
    server.on("/leds", HTTP_PUT, post_put_leds);
}

void setup(void) {
  Serial.begin(115200);

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("esp8266")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    } 
  }
  Serial.println("mDNS responder started");
  config_rest_server_routing();
  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void loop(void) {
  MDNS.update();
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(">");
  }
  if(led_resource.status == 1) {
    if((unsigned long)(millis() - last_running_) > 120000) {
      Serial.println("Turn off due to timeout");
      led_resource.status = 0;
      digitalWrite(led_resource.gpio, led_resource.status);
    }
  }
}
