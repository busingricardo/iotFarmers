In this folder you will find 2 files. 

First one is a .ino file. This is the program code of a Heltec Wifi LoRa 32 (V2) board. This program is uset to connect to Wifi and publish data to an MQTT Brocker.

The data we obtain from the board are temperatures, humidity, HeartBeat, and cow location.

Once this data is published to the MQTT Brocker we subscribe to the topics with Node-RED.

In the json file you will find the subscription to the topics and de visualitzation of the data with some dashboards.