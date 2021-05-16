#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h>  // https://arduinojson.org/
#include <WiFi.h>
#include <Wire.h>
#include "DHT.h" // Include DHT library

// Define new subscription topics here
#define COMMAND_TOPIC "cmd"
#define TEST_TOPIC "test"

#define DHT_PIN 23     // Defines pin number to which the sensor is connected
#define DHT_TYPE DHT11 // Defines the sensor type. It can be DHT11 or DHT22

#define ECHO_PIN 36 // Analog input that receives the echo signal
#define TRIG_PIN 25 // Digital output that sends the trigger signal

#define REFRESH_RATE 2000 // Defined in miliseconds

#define DARKNESS_RES    1000  // Resistance in darkness in KΩ
#define BRIGHTNESS_RES  15    // Resistance in brightness (10 Lux) in KΩ
#define CALIBRARION_RES 10    // Calibration resistance in KΩ
#define LDR_PIN         33    // LDR Pin


#define ANALOG_BIT_RESOLUTION 12.0

int voltage_measure;
int lux;

DHT dhtSensor(DHT_PIN, DHT_TYPE); // Defines the sensor dht

// Replace the next variables with your Wi-Fi SSID/Password
const char *WIFI_SSID = "AulaAutomatica";
const char *WIFI_PASSWORD = "ticsFcim";
char macAddress[18];

// Add MQTT Broker settings
const char *MQTT_BROKER_IP = "iiot-upc.gleeze.com";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "iiot-upc";
const char *MQTT_PASSWORD = "cim2020";
const bool RETAINED = true;
const int QoS = 0; // Quality of Service for the subscriptions

float distance=0;
float temperature=0.0;
float humidity=0.0;
float light=0.0;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  Serial.begin(9600); // Starts the serial communication
  Serial.println("\nBooting device...");
  dhtSensor.begin(); // Starts sensor communication

  pinMode(ECHO_PIN, INPUT);  // Sets the ECHO_PIN as an Input
  pinMode(TRIG_PIN, OUTPUT); // Sets the TRIG_PIN as an Output
  
  mqttClient.setServer(MQTT_BROKER_IP,
                       MQTT_PORT); // Connect the configured mqtt broker
  mqttClient.setCallback(
      callback); // Prepare what to do when a message is recieved

  connectToWiFiNetwork(); // Connects to the configured network
  connectToMqttBroker();  // Connects to the configured mqtt broker
  setSubscriptions();     // Subscribe defined topics

  TimerHandle_t xTimer = xTimerCreate("sendData", REFRESH_RATE, pdTRUE, (void *) 0, sendData);
  xTimerStart(xTimer, 0);
}

void loop() {
  
  checkConnections(); // We check the connection every time
  temperature = dhtSensor.readTemperature(); // Reads the temperature, it takes
  humidity = dhtSensor.readHumidity();
  distance = getDistance();
  light = luz(); 
  
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

  JsonObject values =
      doc.createNestedObject("values"); // We can add another Object
  values["t"] = temperature;
  values["h"] = humidity;
  values["d"] = distance;
  values["l"] = light;
  
  // Serialize the JSON document to a buffer in order to publish it
  serializeJson(doc, buffer);
  mqttClient.publish(topic, buffer, RETAINED);
  Serial.println(" <= " + String(topic) + ": " + String(buffer));
}



void sendData(TimerHandle_t xTimer){
  publishJSONData("Factory");
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW); // Clear the TRIG_PIN by setting it LOW
  delayMicroseconds(5);

  // Trigger the sensor by setting the TRIG_PIN to HIGH for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH); // pulseIn() returns the duration (length of the pulse) in microseconds

  return duration * 0.034 / 2; // Returns the distance in cm
}

float luz(){
 voltage_measure = analogRead(LDR_PIN);  // Reads the value from the pin in a 0-4095 resolution corresponding to a linear 0-3.3V

 lux = voltage_measure * DARKNESS_RES * 10 / (BRIGHTNESS_RES * CALIBRARION_RES * (pow(2.0, ANALOG_BIT_RESOLUTION) - voltage_measure)); // Use with LDR & Vcc
 
 return lux;
 }
