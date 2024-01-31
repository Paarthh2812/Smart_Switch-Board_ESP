#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define PAIR_PIN 4
#define IN_LED 2

const char *self_ssid = "Smart Switch/Board";
const char *self_password = "123456789";
String identification = "input1";
String wifi_ssid = "input2";
String wifi_password = "input3";

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const int no_pins = 4;
int pin_button[no_pins] = {15, 13, 14, 5};
int button[no_pins] = {1, 1, 1, 1};
int start_mode = 0;
int in_led_status = 0;
unsigned long int change_millis = millis();

AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);
  for (int i = 0; i < no_pins; i++)
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
    server.on("/Apprequests", HTTP_POST, [](AsyncWebServerRequest *request){
      Serial.println("Got Request");
      String req_type = request->header("req_type");
      Serial.println(req_type);
      if(req_type == "Pairing_Data"){
        String response = "Data Saved Successfully";
        request->send(200, "text/plain", response);
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
        String response = "Connection Successful";
        request->send(200, "text/plain", response);
      } });
    server.begin();
  }
  else
  {
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
    server.on("/Apprequests", HTTP_POST, [](AsyncWebServerRequest *request){
      String req_type = request->header("req_type");
      Serial.println(req_type);
      if(req_type == "sync"){
        String response = "";
        for (int i = 0 ; i<no_pins;i++){
          response += button[i];
        }
        request->send(200, "text/plain", response);
      }
      if(req_type == "refresh"){
        String response = WiFi.localIP().toString();
        response += ":";
        response += identification;
        response += ":";
        response += no_pins;
        request->send(200, "text/plain", response);
      } 
      if(req_type == "Change_State"){
        String nos = request->header("Switch_Number");
        int number = nos.toInt()-1;
        if (button[number] == 1)
        {
            button[number] = 0;
            digitalWrite(pin_button[number], LOW);
        }
        else
        {
            button[number] = 1;
            digitalWrite(pin_button[number], HIGH);
        }
        String response = "";
          for (int i = 0 ; i<no_pins;i++){
            response += button[i];
          }
          request->send(200, "text/plain", response);
      } });
    server.begin();
  }
}

void loop()
{
  // Nothing to do here
}
