#include <Arduino.h>
#include <WiFi.h>
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

const int no_pins = 6;
int pin_button[no_pins] = {15,5,18,19,21,22};
int button[no_pins] = {1,1,1,1,1,1};
int start_mode = 0;
int in_led_status = 0;
unsigned long int change_millis = millis();

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);
    for(int i=0;i<6;i++){
    pinMode(pin_button[i], OUTPUT);
    digitalWrite(pin_button[i],HIGH);
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
        server.begin();
    }
    else
    {
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
        // Start the server
        server.begin();
    }
}

void loop()
{
    if (start_mode == 0)
    {
        digitalWrite(IN_LED, in_led_status);
        if (millis() - change_millis > 1000)
        {
            in_led_status = !in_led_status;
            change_millis = millis();
        }

        WiFiClient client = server.available();
        if (client)
        {
            Serial.println("New client connected");
            // Read the request from the client
            String request = client.readStringUntil('$');
            Serial.println(request);
            if (request.equals("Connection Check"))
            {
                client.println("Connected");
            }
            if (request.startsWith("Details:"))
            {
                String substrings[10]; // Adjust the size based on your requirement

                int startIndex = 0;
                int arrayIndex = 0;

                // Tokenize the string using a colon as the delimiter
                while (startIndex < request.length())
                {
                    int colonIndex = request.indexOf(':', startIndex);

                    if (colonIndex == -1)
                    {
                        // If no more colons found, get the rest of the string
                        substrings[arrayIndex] = request.substring(startIndex);
                        break;
                    }
                    else
                    {
                        // Get substring between the current start index and colon index
                        substrings[arrayIndex] = request.substring(startIndex, colonIndex);
                        startIndex = colonIndex + 1;
                        arrayIndex++;
                    }
                }

                identification = substrings[2];
                wifi_ssid = substrings[4];
                wifi_password = substrings[6];
                File filesaveidentity = LittleFS.open("/Identity.txt", "w");
                filesaveidentity.print(identification);
                filesaveidentity.close();
                File filesaveWiFi = LittleFS.open("/SSID.txt", "w");
                filesaveWiFi.print(wifi_ssid);
                filesaveWiFi.close();
                File filesavepassword = LittleFS.open("/Password.txt", "w");
                filesavepassword.print(wifi_password);
                filesavepassword.close();
                Serial.println("New Data Saved");
                Serial.print("Identification : ");
                Serial.println(identification);
                Serial.print("WiFi : ");
                Serial.println(wifi_ssid);
                Serial.print("Password : ");
                Serial.println(wifi_password);
                client.println("Reset Successful !!");
                client.stop();
                ESP.restart();
            }
            client.stop();
            Serial.println("Client disconnected");
        }
    }
    else
    {
        // Check if a client has connected
        WiFiClient client = server.available();
        if (client)
        {
            Serial.println("New client connected");
            // Read the request from the client
            String request = client.readStringUntil('$');
            Serial.println(request);
            if (request.equals("refresh"))
            {
                client.print(WiFi.localIP());
                client.print(":");
                client.print(identification);
                client.print(":");
                client.println(no_pins);
            }
            if (request.startsWith("Change_State-Switch"))
            {
                int number = request.substring(19,21).toInt()-1;
                Serial.println(number);
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
            }
            if (request.equals("Sync"))
            {
                for(int i =0 ;i<6;i++){
                client.print(button[i]);
                }
                client.println();

            }
            client.stop();
            Serial.println("Client disconnected");
        }
    }

    // Other tasks can be performed here
}
