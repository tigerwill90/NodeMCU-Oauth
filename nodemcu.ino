
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266RestClient.h>
#include <AES.h> // https://github.com/spaniakos/AES
#include <Base64.h> //https://github.com/adamvr/arduino-base64
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

#define DEBUG true

const char *ssid = "......";
const char *password = ".....";

int sensor = -1;

ESP8266WebServer server(80);
AES aes;

void getWind() {
  // Rest Client
  RestClient client = RestClient("192.168.192.29", 443, 1);
  String response = "";
  client.setHeader("authorization: Basic NTQzMjEw");
  String bearer = server.header("Authorization");
  String accessToken = "token: " + bearer.substring(7, bearer.length());
  char token[accessToken.length() + 1];
  accessToken.toCharArray(token, accessToken.length() + 1);
  client.setHeader(token);
  int statusCode = client.get("/introspect", &response);

  // Parsing json response
  char json[response.length() + 1];
  response.toCharArray(json,response.length() + 1);
  StaticJsonBuffer<100> jsonBuffer; // 50 for single mode, and 60 for cbc with IV
  JsonObject& input = jsonBuffer.parseObject(json);
  bool active = input["active"];
  Serial.println(response);
  switch(statusCode) {
    case 200:
      if (active) {
        String key = input["key"];
        char test[key.length() + 1];
        key.toCharArray(test, key.length() + 1);
        unsigned long long int my_iv = 36753562;
        //process message here
        sensor = digitalRead(4);
        byte plain[] = "La force du vent est de 117 sur l'échelle de Beaufort";
        int plainLength = sizeof(plain)-1;  // don't count the trailing /0 of the string !
        int padedLength = plainLength + N_BLOCK - plainLength % N_BLOCK;
        aes.iv_inc();
        byte iv [N_BLOCK] ;
        byte plain_p[padedLength];
        byte cipher [padedLength] ;
        byte check [padedLength] ;
        aes.set_IV(my_iv);
        aes.get_IV(iv);
        aes.do_aes_encrypt(plain,plainLength,cipher,(byte *)test,256,iv);

        // base 64 encoding
        int inputLen = sizeof(cipher);
        int encodedLength = base64_enc_len(inputLen);
        char encoded[encodedLength]; //base64 len should be 24
        base64_encode(encoded, (char *)cipher, encodedLength);
        Serial.println(encoded);

        StaticJsonBuffer<500> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["message"] = encoded;
        root["iv"] = 36753562;
        String output;
        root.printTo(output);
        
        server.send(statusCode, "application/json", output);
      } 
      
    default:
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["error"] = "access denied";
        String output;
        root.printTo(output);
        server.send(401, "application/json", output);

  }
}

void handleNotFound() {
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

void setup() {
  // put your setup code here, to run once:
  #if DEBUG
    Serial.begin(115200);
  #endif

  pinMode(4, INPUT);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/weather", HTTP_GET, getWind);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
