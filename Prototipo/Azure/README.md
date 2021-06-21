# Basic IoT App

Basic deployable IoT application which includes:

- Broker MQTT: mosquitto
- Timeseries SQL database: TimescaleDB (PostgreSQL)
- Dashboard: Grafana
- Application: Node-RED

## Requirements

- Docker

## How to use

1. Clone the repository

    ```bash
    git clone https://github.com/bonastreyair/basic_iot_app.git
    ```

2. Set configurations

    - TimescaleDB Database: **.env** `POSTGRES_USER` and `POSTGRES_PASSWORD`
    - Mosquitto MQTT Broker: **mosquitto/config/mosquitto.conf**
    - Grafana Dashboard: **grafana/grafana.ini**
    - Node-RED Application: **node-red/settings.json**

3. Run the aplication with Docker Compose

    ```bash
    docker compose up -d
    ```

4. Install Node-RED dependencies

    - node-red-contrib-re-postgres

5. Create _Database_ and _Table_ using Node-RED

6. Connect Grafana to TimescaleDB _Database_
