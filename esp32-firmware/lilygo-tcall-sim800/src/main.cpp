#include <Arduino.h>

#include "config.h"
#include "utilities.h"
#include <TinyGsmClient.h>
#include <WiFi.h>

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);

bool isFirstRun=true;

InfluxDBClient influx(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("signal");

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    SerialMon.println("Connected to WIFI AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    SerialMon.println("WiFi connected");
    SerialMon.println("IP address: ");
    SerialMon.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    SerialMon.println("Disconnected from WiFi access point");
    SerialMon.print("WiFi lost connection. Reason: ");
    SerialMon.println(info.wifi_sta_disconnected.reason);
    SerialMon.println("Trying to Reconnect");
    WiFi.begin(ssid, password);
}

void setupWifi()
{
    // delete old config
    WiFi.disconnect(true);

    delay(1000);

    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    /* Remove WiFi event
    SerialMon.print("WiFi Event ID: ");
    SerialMon.println(eventID);
    WiFi.removeEvent(eventID);*/

    WiFi.begin(ssid, password);
    SerialMon.println();
    SerialMon.println();
    SerialMon.println("Wait for WiFi... ");
    while (!WiFi.isConnected())
    {
        for (int i = 0; i < 3; i++)
        {
            digitalWrite(LED_GPIO, LED_ON);
            delay(80);
            digitalWrite(LED_GPIO, LED_OFF);
            delay(80);
        }
        SerialMon.print(".");
    }

    // Accurate time is necessary for certificate validation and writing in batches
    // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
    // Syncing progress and the time will be printed to Serial.
    // TODO: make sure that the right serial port is used in any case
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    // Check server connection
    if (influx.validateConnection())
    {
        SerialMon.print("Connected to InfluxDB: ");
        SerialMon.println(influx.getServerUrl());
    }
    else
    {
        SerialMon.print("InfluxDB connection failed: ");
        SerialMon.println(influx.getLastErrorMessage());
    }
}

void turnOffNetlight()
{
    SerialMon.println("Turning off SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=0");
    modem.waitResponse();
}

void turnOnNetlight()
{
    SerialMon.println("Turning on SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=1");
    modem.waitResponse();
}

void runDebugTerminal()
{
    while (SerialAT.available())
    {
        char c = SerialAT.read();
        SerialMon.write(c);
    }
    while (SerialMon.available())
    {
        char c = SerialMon.read();
        SerialAT.write(c);
    }
}

void setupModem()
{
#ifdef MODEM_RST
    // Keep reset high
    pinMode(MODEM_RST, OUTPUT);
    digitalWrite(MODEM_RST, HIGH);
#endif

    pinMode(MODEM_PWRKEY, OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);

    // Initialize the indicator as an output
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LED_OFF);

    // Turn on the Modem power first
    digitalWrite(MODEM_POWER_ON, HIGH);

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);

    delay(5000);

    // Restart takes quite some time
    SerialMon.println("Initializing modem...");
    while (!modem.restart())
    {
        SerialMon.print(".");
        delay(5000);
    }
    SerialMon.println();

    String modemName = modem.getModemName();
    SerialMon.print("Modem: ");
    SerialMon.println(modemName);
    sensor.addTag("modem", modemName.c_str());

    String imei = modem.getIMEI();
    SerialMon.print("imei: ");
    SerialMon.println(imei);
    sensor.addTag("imei", imei.c_str());

    SimStatus simstatus = modem.getSimStatus();
    if (simstatus == 0)
    {
        SerialMon.println("SIM ERROR!");
        // modem.poweroff();
        while (true)
        {
            for (int i = 0; i < 4; i++)
            {
                digitalWrite(LED_GPIO, LED_ON);
                delay(50);
                digitalWrite(LED_GPIO, LED_OFF);
                delay(50);
            }
            runDebugTerminal();
        }
    }

    // Unlock your SIM card with a PIN if needed
    if (strlen(simPIN) && simstatus != 3)
    {
        modem.simUnlock(simPIN);
    }

    String imsi = modem.getIMSI();
    SerialMon.print("IMSI: ");
    SerialMon.println(imsi);
    sensor.addTag("imsi", imsi.c_str());
}

void measureNetwork()
{
    // turnOnNetlight();

    sensor.clearFields();

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork(240000L))
    {
        SerialMon.println(" fail");

        // SEND ERROR INDICATION TO INFLUXDB
        sensor.addField("csq", 99);
        SerialMon.print("Writing: ");
        SerialMon.println(influx.pointToLineProtocol(sensor));
        if (!influx.writePoint(sensor))
        {
            SerialMon.print("InfluxDB write failed: ");
            SerialMon.println(influx.getLastErrorMessage());
        }
        return;
    }
    SerialMon.println(" OK");

    // When the network connection is successful, turn on the indicator
    digitalWrite(LED_GPIO, LED_ON);

    int16_t csq = modem.getSignalQuality();
    SerialMon.print("CSQ: ");
    SerialMon.println(csq);
    // SEND CSQ TO INFLUXDB
    sensor.addField("csq", csq);

    if(isFirstRun)
    {
        String networkoperator = modem.getOperator();
        sensor.addTag("operator", networkoperator.c_str());
        isFirstRun=false;
    }
    
    SerialMon.print("Writing: ");
    SerialMon.println(influx.pointToLineProtocol(sensor));
    if (!influx.writePoint(sensor))
    {
        SerialMon.print("InfluxDB write failed: ");
        SerialMon.println(influx.getLastErrorMessage());
    }

    /*
        //CONNECT TO APN AND CREATE GPRS PACKET DATA CONTEXT
        SerialMon.print(F("Connecting to APN: "));
        SerialMon.print(apn);
        if (!modem.gprsConnect(apn, gprsUser, gprsPass))
        {
            SerialMon.println(" fail");
            delay(10000);
            return;
        }
        SerialMon.println(" OK");


        SerialMon.println();
        modem.gprsDisconnect();
        SerialMon.println(F("GPRS disconnected"));
    */

    // turnOffNetlight();
}

void setup()
{
    SerialMon.begin(115200);
    delay(10);
    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

    // Start power management
    if (setupPMU() == false)
    {
        SerialMon.println("Setting power error");
    }

    setupWifi();
    setupModem();
}

void loop()
{
    measureNetwork();
    // modem.poweroff();
    delay(30000);
}