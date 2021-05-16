#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h>  // https://arduinojson.org/
#include <WiFi.h>
#include <Wire.h>
#include "DHT.h" // Include DHT library
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Define new subscription topics here
#define COMMAND_TOPIC "cmd"
#define TEST_TOPIC "test"

#define DHT_PIN 23     // Defines pin number to which the sensor is connected
#define DHT_TYPE DHT11 // Defines the sensor type. It can be DHT11 or DHT22

#define RX_PIN 16 // Pinout RX of ESP32
#define TX_PIN 17 // Pinout TX of ESP32

#define REFRESH_SEND 3000 // Defined in miliseconds
#define REFRESH_GETDATA 2000 // Defined in miliseconds

DHT dhtSensor(DHT_PIN, DHT_TYPE); // Defines the sensor dht

// Replace the next variables with your Wi-Fi SSID/Password
const char *WIFI_SSID = "******";
const char *WIFI_PASSWORD = "******";
char macAddress[18];

// Add MQTT Broker settings
const char *MQTT_BROKER_IP = "iiot-upc.gleeze.com";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "******";
const char *MQTT_PASSWORD = "******";
const bool RETAINED = true;
const int QoS = 0; // Quality of Service for the subscriptions

float temperature=0.0;
float humidity=0.0;
float latitud;
float longitud;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

HardwareSerial SerialGPS(1);
TinyGPSPlus gps;

void setup() {
  Serial.begin(9600); // Starts the serial communication
  Serial.println("\nBooting device...");
  dhtSensor.begin(); // Starts sensor communication

  SerialGPS.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Starts gps communication with UART

  mqttClient.setServer(MQTT_BROKER_IP,
                       MQTT_PORT); // Connect the configured mqtt broker
  mqttClient.setCallback(callback); // Prepare what to do when a message is recieved

  connectToWiFiNetwork(); // Connects to the configured network
  connectToMqttBroker();  // Connects to the configured mqtt broker
  setSubscriptions();     // Subscribe defined topics

  TimerHandle_t xTimer1 = xTimerCreate("GetData", REFRESH_GETDATA, pdTRUE, (void *) 0, GetData);
  TimerHandle_t xTimer2 = xTimerCreate("sendData", REFRESH_SEND, pdTRUE, (void *) 0, sendData);
  
  xTimerStart(xTimer1, 0);
  xTimerStart(xTimer2, 0);
}

void loop() {
  
  checkConnections(); // We check the connection every time

}

/* Additional functions */
void setSubscriptions() {
  subscribe(COMMAND_TOPIC);
  subscribe(TEST_TOPIC);
}

void subscribe(char *newTopic) {
  const String topicStr = createTopic(newTopic);
  const char *topic = topicStr.c_str();
  mqttClient.subscribe(topic, QoS);
  Serial.println("Client MQTT subscribed to topic: " + topicStr +
                 " (QoS:" + String(QoS) + ")");
}

void callback(char *topic, byte *payload, unsigned int length) {
  // Register all subscription topics
  static const String cmdTopicStr = createTopic(COMMAND_TOPIC);
  static const String testTopicStr = createTopic(TEST_TOPIC);

  String msg = unwrapMessage(payload, length);
  Serial.println(" => " + String(topic) + ": " + msg);

  // What to do in each topic case?
  if (String(topic) == cmdTopicStr) {
    //TurnOn();    
  } else if (String(topic) == testTopicStr) {
   // TurnOn2();// Do some other stuff
  } else {
    Serial.println("[WARN] - '" + String(topic) +
                   "' topic was correctly subscribed but not defined in the "
                   "callback function");
  }
}

String unwrapMessage(byte *message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) { // Unwraps the string message
    msg += (char)message[i];
  }
  return msg;
}

String createTopic(char *topic) {
  String topicStr = String(macAddress) + "/" + topic;
  return topicStr;
}

void connectToWiFiNetwork() {
  Serial.print(
      "Connecting with Wi-Fi: " +
      String(WIFI_SSID)); // Print the network which you want to connect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".."); // Connecting effect
  }
  Serial.print("..connected!  (ip: "); // After being connected to a network,
                                       // our ESP32 should have a IP
  Serial.print(WiFi.localIP());
  Serial.println(")");
  String macAddressStr = WiFi.macAddress().c_str();
  strcpy(macAddress, macAddressStr.c_str());
}

void connectToMqttBroker() {
  Serial.print(
      "Connecting with MQTT Broker: " +
      String(MQTT_BROKER_IP));    // Print the broker which you want to connect
  mqttClient.connect(macAddress, MQTT_USER, MQTT_PASSWORD); // Using unique mac address from ESP32
  while (!mqttClient.connected()) {
    delay(500);
    Serial.print("..");             // Connecting effect
    mqttClient.connect(macAddress); // Using unique mac address from ESP32
  }
  Serial.println("..connected! (ClientID: " + String(macAddress) + ")");
}

void checkConnections() {
  if (mqttClient.connected()) {
    mqttClient.loop();
  } else { // Try to reconnect
    Serial.println("Connection has been lost with MQTT Broker");
    if (WiFi.status() != WL_CONNECTED) { // Check wifi connection
      Serial.println("Connection has been lost with Wi-Fi");
      connectToWiFiNetwork(); // Reconnect Wifi
    }
    connectToMqttBroker(); // Reconnect Server MQTT Broker
    setSubscriptions();    // Subscribes to configured topics
  }
}

void publishJSONData(char* Topic) {
  static const String topicStr = createTopic(Topic);
  static const char *topic = topicStr.c_str();

  StaticJsonDocument<128> doc; // Create JSON document of 128 bytes
  char buffer[128]; // Create the buffer where we will print the JSON document
                    // to publish through MQTT
  JsonObject values = doc.createNestedObject("values"); // We can add another Object     
  values["t"] = temperature;
  values["h"] = humidity;
  values["lt"] = latitud;
  values["lg"] = longitud;
  
  // Serialize the JSON document to a buffer in order to publish it
  serializeJson(doc, buffer);
  mqttClient.publish(topic, buffer, RETAINED);
  Serial.println(" <= " + String(topic) + ": " + String(buffer));
}

void sendData(TimerHandle_t xTimer1){
  publishJSONData("Vaca1");
  publishJSONData("Vaca2");
  publishJSONData("Vaca3");
}

void GetData(TimerHandle_t xTimer2){
  temperature = dhtSensor.readTemperature(); 
  humidity = dhtSensor.readHumidity();
  
  if (SerialGPS.available()) {
    gps.encode(SerialGPS.read()); // Encodes all messages from GPS
    latitud=gps.location.lat();
    longitud=gps.location.lng();
  }
}
