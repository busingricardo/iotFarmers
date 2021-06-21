# iotFarmers App

La aplicación esta instalada en una máquina virtual de Azure y trabaja con dockers. Esta incluye:

* Broker MQTT: mosquitto
* Timeseries SQL database: TimescaleDB (PostgreSQL)
* Dashboard: Grafana
* Application: Node-RED

## Requerimientos

* Docker & Docker-Compose

### Instalar Docker

Para instalarlos seguiremos los siguientes pasos:

1. Update and Upgrade
```bash
sudo apt-get update && sudo apt-get upgrade
```
2. Download the Convenience Script and Install Docker
```
curl -fsSL https://get.docker.com -o get-docker.sh
```
Execute the script using the command:
```
sudo sh get-docker.sh
```
3. Add a Non-Root User to the Docker Group
By default, only users who have administrative privileges (root users) can run containers. If you are not logged in as the root, one option is to use the sudo prefix.
However, you could also add your non-root user to the Docker group which will allow it to execute docker commands.
The syntax for adding users to the Docker group is:
```
sudo usermod -aG docker [user_name]
```
4. Check Docker Version and Info
Check the version of Docker on your Raspberry Pi by typing:
```
docker version
```
The output will display the Docker version along with some additional information.

5. Reboot
```
sudo reboot –h now
```
### Instalar Docker-Compose
1. Instalar Python3
```
sudo apt-get install libffi-dev libssl-dev
sudo apt install python3-dev
sudo apt-get install -y python3 python3-pip
```
2. Instalar docker-compose
Once you have python and pip installed just run the following command:
```
sudo pip3 install docker-compose
```

## Instalar la Aplicación

1. Clonar el repositorio

    ```bash
    git clone https://github.com/bonastreyair/basic_iot_app.git
    ```

2. Configurar:

    - TimescaleDB Database: **.env** `POSTGRES_USER` and `POSTGRES_PASSWORD`
    - Mosquitto MQTT Broker: **mosquitto/config/mosquitto.conf**
    - Grafana Dashboard: **grafana/grafana.ini**
    - Node-RED Application: **node-red/settings.json**

3. Iniciar la aplicación con docker-compose

    ```bash
    docker-compose up -d
    ```

4. Create _Database_ and _Table_ using Node-RED

5. Connect Grafana to TimescaleDB _Database_
