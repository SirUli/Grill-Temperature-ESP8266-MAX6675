// ESP 8266 with the MAX6675 Chip

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <max6675.h>
#include <PubSubClient.h>
#include <SPI.h>

// Reuse the RX-Line for the Connection
int thermoDO = 3;
int thermoCS = 2;
int thermoCLK = 0;

/*************************************************/
/* Settings for WLAN                             */
/*************************************************/

const char* ssid     = "...";
const char* password = "...";

IPAddress ip(192,168,133,7);
IPAddress gateway(192,168,133,1);
IPAddress subnet(255,255,255,0);
IPAddress dns(192,168,133,1);

int wifiConnectCounter = 0;

// Create an WiFiClient object, here called "espClient":
WiFiClient espClient;

/*************************************************/
/* Settings for MQTT                             */
/*************************************************/
const char* mqttBroker = "192.168.133.2";
int mqttPort = 1883;

#define MQTTDeviceID "01"
#define MQTTClientID "ESP8266." MQTTDeviceID
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define TOPIC_ClientID "ESP8266/" MQTTDeviceID
#define TOPIC_LastWill  TOPIC_ClientID "/connected"
#define TOPIC_TemperatureState  TOPIC_ClientID "/temperature/value"
#define TOPIC_VoltageState  TOPIC_ClientID "/voltage/value"

PubSubClient client(espClient);

/*************************************************/
/* Settings for VCC Measurements                 */
/*************************************************/
ADC_MODE(ADC_VCC);
float espVoltage;

/*************************************************/
/* Settings for MAX6675 Measurements             */
/*************************************************/
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
String temperature;  // Values read from sensor

#define REPEAT_INTERVAL 360
char charBuffer[32];

/*************************************************/
/* Declarations of functions                     */
/*************************************************/
boolean myPublish(char* topic, char* payload);
void processMqttItems();


/*************************************************/
/* SETUP                                         */
/*************************************************/
void setup() {
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet, dns);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    wifiConnectCounter++;
    if (wifiConnectCounter > 100) {
      delay(REPEAT_INTERVAL * 1000 * 1000);
      wifiConnectCounter = 0;
    }
  }

  // Set the RX Pin to Input
  pinMode(3, INPUT);

  // Configure MQTT
  client.setServer(mqttBroker, mqttPort);
}

void loop() {
  while (!client.connected()) {
    if (client.connect((char *)TOPIC_ClientID,(char *)TOPIC_LastWill,1,1,(char *)"0")) {
      // Connected, now read Temperature Sensor
      delay(500);
      temperature = String(thermocouple.readCelsius());
      yield();
      // Determine the VCC Voltage
      espVoltage = ESP.getVcc();
      yield();
      processMqttItems();//send temp & espVoltage to MQTT Broker
    } else {
      //MQTT is not connected... retrying
      delay(1000);
    }
  }
  delay(REPEAT_INTERVAL * 1000 * 1000);
}

// publish mit retain
boolean myPublish(char* topic, char* payload)
{
  client.publish(topic,(uint8_t*)payload,strlen(payload),true);
}

// processes the items to be published
void processMqttItems()
{
  String strBuffer;
  strBuffer =  String(espVoltage);
  strBuffer.toCharArray(charBuffer,10);
  myPublish((char *)TOPIC_VoltageState,charBuffer);
  yield();

  strBuffer =  String(temperature);
  strBuffer.toCharArray(charBuffer,10);
  myPublish((char *)TOPIC_TemperatureState,charBuffer);
}
