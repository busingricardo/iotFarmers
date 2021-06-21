# Prototipo para la Fábrica

Este programa [Fabrica.ino](Fabrica.ino) se encarga de simular el estado ambiente de 2 salas de la fabrica. Las senyales que recoge són temperatura, humedad, presión atmosférica, co2, tvoc y midel la luz de la sala.

Además también simula la temperatura y humedad exterior.

Una vez leidos los datos la ESP32 se conecta a la red wifi y publica los datos al broker MQTT de Mosquitto instalado en la máquina virtual de Azure.
