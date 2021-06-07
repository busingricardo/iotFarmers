#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <Arduino.h>
#include <ArduinoJson.h>  // https://arduinojson.org/
#include <WiFi.h>
#include <Wire.h>
#include "DHT.h" // Include DHT library
#include "ccs811.h"                     // include library for CCS811 - Sensor from martin-pennings https://github.com/maarten-pennings/CCS811
#include "Adafruit_Si7021.h"            // include main library for SI7021 - Sensor
#include "Adafruit_BMP280.h"            // include main library for BMP280 - Sensor

CCS811 ccs811;
Adafruit_BMP280 bmp280;                // I2C
Adafruit_Si7021 SI702x = Adafruit_Si7021();


// Define new subscription topics here
#define TEMP_AMB11 "Amb1/1/Temp"
#define TEMP_AMB12 "Amb1/2/Temp"
#define HUM_AMB11 "Amb1/1/Hum"
#define PRES_AMB11 "Amb1/1/Pres"
#define CO2_AMB11 "Amb1/1/CO2"
#define TVOC_AMB11 "Amb1/1/TVOC"
#define LUZ_AMB11 "Amb1/1/Lux"

#define TEMP_AMB2 "Amb2/1/Temp"
#define HUM_AMB2 "Amb2/1/Hum"
#define LUZ_AMB2 "Amb2/1/Lux"

#define COMMAND_TOPIC "cmd"
#define TEST_TOPIC "test"

//DEFINE PINS
#define DHT_PIN 23     // Defines pin number to which the sensor is connected
#define DHT_TYPE DHT11 // Defines the sensor type. It can be DHT11 or DHT22

#define DHT2_PIN 17     // Defines pin number to which the sensor is connected
#define DHT2_TYPE DHT11 // Defines the sensor type. It can be DHT11 or DHT22

#define ECHO_PIN 36 // Analog input that receives the echo signal
#define TRIG_PIN 12 // Digital output that sends the trigger signal

#define REFRESH_RATE 10000 // Defined in miliseconds
#define REFRESH_RATE_READ 7000 // Defined in miliseconds

#define DARKNESS_RES    1000  // Resistance in darkness in KΩ
#define BRIGHTNESS_RES  15    // Resistance in brightness (10 Lux) in KΩ
#define CALIBRARION_RES 10    // Calibration resistance in KΩ
#define LDR_PIN         33    // LDR Pin


#define ANALOG_BIT_RESOLUTION 12.0

int voltage_measure;
int lux;

DHT dhtSensor(DHT_PIN, DHT_TYPE); // Defines the sensor dht
DHT dhtSensor2(DHT2_PIN, DHT2_TYPE); // Defines the sensor dht 2

// Replace the next variables with your Wi-Fi SSID/Password
const char *WIFI_SSID = "Cal_Bonastre";
const char *WIFI_PASSWORD = "KLTSPCWMCG8M52";
char macAddress[18];

// Add MQTT Broker settings
const char *MQTT_BROKER_IP = "vmiotbonastre.northeurope.cloudapp.azure.com";
const int MQTT_PORT = 1883;
const char *MQTT_USER = "marc";
const char *MQTT_PASSWORD = "marc199435";
const bool RETAINED = true;
const int QoS = 0; // Quality of Service for the subscriptions

float distance=0;
float temperature=0.0;
float humidity=0.0;
float light=0.0;
float temperature_tk1=0.0;
float humidity_tk1=0.0;

//CJMCU Sensors
float bmp280Temp = 0.0;
float bmp280Pres = 0.0;
float SI702xTemp = 0.0;
float SI702xHum = 0.0;
float ccs811eCO2 = 0.0;
float ccs811eTVOC = 0.0;


WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  Serial.begin(9600); // Starts the serial communication
  Serial.println("\nBooting device...");
  dhtSensor.begin(); // Starts sensor communication
  dhtSensor2.begin(); // Starts sensor communication

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

//  TimerHandle_t xTimer2 = xTimerCreate("readSensor", REFRESH_RATE_READ, pdTRUE, (void *) 0, sendData);
//  xTimerStart(xTimer2, 0);


  //CJMCU Sensor
  Wire.begin(21,22);                  // put here the Pins of I2C
  Serial.println("CCS811 test");      /* --- SETUP CCS811 on 0x5A ------ */
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  if (!ccs811.begin()) {
    Serial.println("Failed to start sensor! Please check your wiring.");
    while (true);
  }
  // Print CCS811 versions
  Serial.print("setup: hardware    version: "); Serial.println(ccs811.hardware_version(), HEX);
  Serial.print("setup: bootloader  version: "); Serial.println(ccs811.bootloader_version(), HEX);
  Serial.print("setup: application version: "); Serial.println(ccs811.application_version(), HEX);

  // Start measuring
  bool ok = ccs811.start(CCS811_MODE_1SEC);
  if ( !ok ) Serial.println("setup: CCS811 start FAILED");

  Serial.println("BMP280 test");     /* --- SETUP BMP on 0x76 ------ */
  if (!bmp280.begin(0x76)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (true);
  }

  Serial.println("Si7021 test!");     /* ---- SETUP SI702x ----- */
  if (!SI702x.begin()) {
    Serial.println("Did not find Si702x sensor!");
    while (true);
  }
  Serial.print("Found model ");
  switch (SI702x.getModel()) {
    case SI_Engineering_Samples:
      Serial.print("SI engineering samples"); break;
    case SI_7013:
      Serial.print("Si7013"); break;
    case SI_7020:
      Serial.print("Si7020"); break;
    case SI_7021:
      Serial.print("Si7021"); break;
    case SI_UNKNOWN:
    default:
      Serial.print("Unknown");
  }
  Serial.print(" Revision(");
  Serial.print(SI702x.getRevision());
  Serial.print(")");
  Serial.print(" Serial #"); Serial.print(SI702x.sernum_a, HEX); Serial.println(SI702x.sernum_b, HEX);
}

void loop() {

  checkConnections(); // We check the connection every time
  getCJMCU_data();
  temperature = SI702x.readTemperature(); // Reads the temperature, it takes
  humidity = SI702x.readHumidity(); //dhtSensor.readHumidity();
  temperature_tk1 = dhtSensor2.readTemperature(); // Reads the temperature, it takes
  humidity_tk1 = dhtSensor2.readHumidity();
  distance = getDistance();
  light = luz();

}

void getCJMCU_data() {
  
  bmp280Temp = bmp280.readTemperature();
  bmp280Pres = (bmp280.readPressure() / 100);
  
  SI702xTemp = SI702x.readTemperature();
  SI702xHum = SI702x.readHumidity();

  uint16_t eco2, etvoc, errstat, raw;     // Read CCS811

  ccs811.set_envdata(bmp280.readTemperature(), SI702x.readHumidity());
  ccs811.read(&eco2, &etvoc, &errstat, &raw);
  if ( errstat == CCS811_ERRSTAT_OK ) {
    ccs811eCO2 = eco2;
    ccs811eTVOC = etvoc;
  }
  delay(5000);
}

void sendData(TimerHandle_t xTimer){
  publishFactoryAmbient("Ambient");
  publishData(TEMP_AMB11, bmp280Temp);
  publishData(TEMP_AMB12, SI702xTemp);
  publishData(HUM_AMB11, SI702xHum);
  publishData(PRES_AMB11, bmp280Pres);
  publishData(CO2_AMB11, ccs811eCO2);
  publishData(TVOC_AMB11, ccs811eTVOC);
  publishData(LUZ_AMB11, light);

  publishFactoryReception("Reception");
  publishData("Recep1_Tnk1_T", temperature_tk1);
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
  String topicStr = String(macAddress) + "Factory" + "/" + topic;
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

void publishFactoryAmbient(char* Topic) {
  static const String topicStr = createTopic(Topic);
  static const char *topic = topicStr.c_str();

  StaticJsonDocument<128> doc; // Create JSON document of 128 bytes
  char buffer[128]; // Create the buffer where we will print the JSON document
                    // to publish through MQTT

  doc["t"] = temperature;
  doc["h"] = humidity;
  doc["l"] = light;
  
  // Serialize the JSON document to a buffer in order to publish it
  serializeJson(doc, buffer);
  mqttClient.publish(topic, buffer, RETAINED);
  Serial.println(" <= " + String(topic) + ": " + String(buffer));
}

void publishData(char* Topic, float value) {
  //static const String topicStr = createTopic(Topic);
  //static const char *topic = topicStr.c_str();
  String topicStr = createTopic(Topic);
  const char *topic = topicStr.c_str();
  
  mqttClient.publish(topic, String(value).c_str(), RETAINED);
  Serial.println(" <= " + String(topic) + ": " + String(value));
}

void publishFactoryReception(char* Topic) {
  static const String topicStr = createTopic(Topic);
  static const char *topic = topicStr.c_str();

  DynamicJsonDocument doc(512); // Create JSON document
  char buffer[512]; // Create the buffer where we will print the JSON document
                     // to publish through MQTT

  JsonObject R1 = doc.createNestedObject("R1"); // Add another Object
  R1["t_tk1"] = temperature_tk1;
  R1["l_tk1"] = humidity_tk1;
  R1["t_tk2"] = temperature_tk1 + 0.34;
  R1["l_tk2"] = humidity_tk1 + 5;
  R1["d_cmn"] = distance;

  JsonObject R2 = doc.createNestedObject("R2"); // Add another Object
  R2["t_tk1"] = temperature_tk1;
  R2["l_tk1"] = humidity_tk1;
  R2["t_tk2"] = temperature_tk1 + 0.34;
  R2["l_tk2"] = humidity_tk1 + 5;
  R2["d_cmn"] = distance;
  
  // Serialize the JSON document to a buffer in order to publish it
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish_P(topic, buffer, n); // No RETAINED option
  Serial.println(" <= " + String(topic) + ": " + String(buffer));
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW); // Clear the TRIG_PIN by setting it LOW
  delayMicroseconds(5);

  // Trigger the sensor by setting the TRIG_PIN to HIGH for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH); // pulseIn() returns the duration (length of the pulse) in microseconds

  //Serial.println("Distancia" + String(duration * 0.034 / 2));

  return duration * 0.034 / 2; // Returns the distance in cm
}

float luz(){
 voltage_measure = analogRead(LDR_PIN);  // Reads the value from the pin in a 0-4095 resolution corresponding to a linear 0-3.3V

 lux = voltage_measure * DARKNESS_RES * 10 / (BRIGHTNESS_RES * CALIBRARION_RES * (pow(2.0, ANALOG_BIT_RESOLUTION) - voltage_measure)); // Use with LDR & Vcc

 //Serial.println("luz" + String(lux));

 return lux;
 }
