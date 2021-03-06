
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266RestClient.h> //https://github.com/tigerwill90/esp8266-restclient
#include <AES.h> //https://github.com/spaniakos/AES
#include <Base64.h> //https://github.com/adamvr/arduino-base64
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <ESP8266TrueRandom.h> //https://github.com/marvinroger/ESP8266TrueRandom

#define DEBUG true

const char *ssid = "labocisco";
const char *password = "Class123";

ESP8266WebServer server(80);
AES aes;

void getWind() {
  digitalWrite(2, LOW);
  // Rest Client
  RestClient client = RestClient("10.136.1.112", 443, 1);
  String response = "";
  client.setHeader("authorization: Basic NTQzMjEwOjU0MzIxMDk4NzY1NDMyMTA=");
  String bearer = server.header("Authorization");
  String accessToken = "token: " + bearer.substring(7, bearer.length());
  char token[accessToken.length() + 1];
  accessToken.toCharArray(token, accessToken.length() + 1);
  client.setHeader(token);
  int statusCode = client.get("/introspect", &response);
  Serial.println(response);

  // Parsing json response
  char json[response.length() + 1];
  response.toCharArray(json,response.length() + 1);
  StaticJsonBuffer<100> jsonBuffer; // 50 for single mode, and 60 for cbc with IV
  JsonObject& input = jsonBuffer.parseObject(json);
  bool active = input["active"];
  switch(statusCode) {
    case 200:
      if (active) {
        String key = input["key"];
        char aesKey[key.length() + 1];
        key.toCharArray(aesKey, key.length() + 1);
        //process message here

        char randomNum[16];
        ESP8266TrueRandom.memfill(randomNum, 16);
        String randomString = randomNum;
        int sensor = analogRead(0);
        Serial.println(sensor);
        //char buf[3];
        //sprintf(buf,"%i", sensor);
        String response;
        if (sensor >= 0 && sensor < 100) {
          response = "origine = vent d'est";
        } else if (sensor >= 100 && sensor < 200) {
          response = "origine = vend de sud est";
        } else if (sensor >= 200 && sensor < 350) {
          response = "origine = vend de sud";
        } else if (sensor >= 350 && sensor < 500) {
          response = "origine = vend de nord est"; 
        } else if (sensor >= 500 && sensor < 750) {
          response = "origine = vend de sud ouest";
        } else if (sensor >= 750 && sensor < 850) {
          response = "origine = vend de nord";
        } else if (sensor >=850 && sensor < 920) {
          response = "origine = vend de nord ouest";
        } else if (sensor >= 920) {
          response = "origine = vend d'ouest";
        } else {
          response = "origine = inconnue";
        }
        String plaintext = randomString + response;
        byte plain[plaintext.length() + 1];
        plaintext.getBytes(plain, plaintext.length() + 1);
        int plainLength = sizeof(plain)-1;  // don't count the trailing /0 of the string !
        int padedLength = plainLength + N_BLOCK - plainLength % N_BLOCK;;
        byte cipher[padedLength] ;
        aes.do_aes_encrypt(plain,plainLength,cipher,(byte *)aesKey,256);

        // base 64 encoding
        int inputLen = sizeof(cipher);
        int encodedLength = base64_enc_len(inputLen);
        char encoded[encodedLength]; //base64 len should be 24
        base64_encode(encoded, (char *)cipher, encodedLength);
        Serial.println(encoded);

        StaticJsonBuffer<500> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["encoded"] = encoded;
        root["length"] = plainLength;
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
  digitalWrite(2, HIGH);
}

void handleNotFound() {
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

void setup() {
  // put your setup code here, to run once:
  #if DEBUG
    Serial.begin(115200);
  #endif

  pinMode(0, INPUT);
  pinMode(2, OUTPUT);

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

  server.on("/wind/direction", HTTP_GET, getWind);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started");
  digitalWrite(2, HIGH);
}

void loop() {
  server.handleClient();
}
