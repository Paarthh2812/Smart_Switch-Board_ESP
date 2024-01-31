#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#define PAIR_PIN 4
#define IN_LED 2

// const char *ssid = "iPhone";
// const char *password = "amicable";
const char *self_ssid = "Smart Switch/Board";
const char *self_password = "123456789";
String identification = "input1";
String wifi_ssid = "input2";
String wifi_password = "input3";

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const int no_pins = 6;
int pin_button[no_pins] = {15, 5, 18, 19, 21, 22};
int button[no_pins] = {1, 1, 1, 1, 1, 1};
int start_mode = 0;
int in_led_status = 0;
unsigned long int change_millis = millis();

AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);
  for (int i = 0; i < 6; i++)
  {
    pinMode(pin_button[i], OUTPUT);
    digitalWrite(pin_button[i], HIGH);
  }
  pinMode(IN_LED, OUTPUT);
  digitalWrite(IN_LED, LOW);

  pinMode(PAIR_PIN, INPUT_PULLUP);
  start_mode = digitalRead(PAIR_PIN);
  if (!LittleFS.begin())
  {
    Serial.println("error while initializing the littleFS");
    return;
  }

  if (start_mode == 0)
  {
    Serial.println("Pairing Mode");
    Serial.println("Self-Wifi Access Mode");
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(self_ssid, self_password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.on("/Apprequests", HTTP_POST, [](AsyncWebServerRequest *request)
      {
      String req_type = request->header("req_type");
      Serial.println(req_type);
      if(req_type == "Pairing Data"){
          DynamicJsonDocument sending(1024);
          sending["message"] = "Data Saved Successfully";
          String response;
          serializeJson(sending, response);
          request->send(200, "application/json", response);
          identification = request->header("Identification");;
          wifi_ssid = request->header("Wifi_SSID");;
          wifi_password = request->header("Wifi_Password");;
          File filesaveidentity = LittleFS.open("/Identity.txt", "w");
          filesaveidentity.print(identification);
          filesaveidentity.close();
          File filesaveWiFi = LittleFS.open("/SSID.txt", "w");
          filesaveWiFi.print(wifi_ssid);
          filesaveWiFi.close();
          File filesavepassword = LittleFS.open("/Password.txt", "w");
          filesavepassword.print(wifi_password);
          filesavepassword.close();
      }
      if(req_type == "Connection Check"){
        DynamicJsonDocument sending(1024);
        sending["message"] = "Connection Successful";
        String response;
        serializeJson(sending, response);
        request->send(200, "application/json", response);
      } 
    });
    server.begin();
  }
  else{
    // Connect to Wi-Fi
        File fileIdentity = LittleFS.open("/Identity.txt", "r");
        if (!fileIdentity)
        {
            Serial.println("No Saved Data!");
        }
        identification = fileIdentity.readString();
        Serial.print("Identification : ");
        Serial.println(identification);
        fileIdentity.close();
        File fileSSID = LittleFS.open("/SSID.txt", "r");
        if (!fileSSID)
        {
            Serial.println("No Saved Data!");
        }
        wifi_ssid = fileSSID.readString();
        Serial.print("WiFi : {");
        Serial.print(wifi_ssid);
        Serial.println("}");
        fileSSID.close();
        File filePassword = LittleFS.open("/Password.txt", "r");
        if (!filePassword)
        {
            Serial.println("No Saved Data!");
        }
        wifi_password = filePassword.readString();
        Serial.print("Password : {");
        Serial.print(wifi_password);
        Serial.println("}");

        filePassword.close();

        WiFi.begin(wifi_ssid, wifi_password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("Connecting to WiFi...");
        }
        Serial.println("Connected to WiFi");
        Serial.println(WiFi.localIP());
        server.on("/Apprequests", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              String req_type = request->header("req_type");
              Serial.println(req_type);
              if(req_type == "sync"){
                  DynamicJsonDocument sending(1024);
                  sending["Switch_States"] = button;
                  String response;
                  serializeJson(sending, response);
                  request->send(200, "application/json", response);
              }
              if(req_type == "refresh"){
                DynamicJsonDocument sending(1024);
                  sending["IP_Address"] = WiFi.localIP();
                  String response;
                  serializeJson(sending, response);
                  request->send(200, "application/json", response);
              } 
              if(req_type == "Change_State"){
                String number = request->header("Switch Number");
                int no = number.toInt();
                if (button[no] == 1)
                {
                    button[no] = 0;
                    digitalWrite(pin_button[no], LOW);
                }
                else
                {
                    button[no] = 1;
                    digitalWrite(pin_button[no], HIGH);
                }
              }});
          server.begin();
  }
}

void loop()
{
  // Nothing to do here
}
