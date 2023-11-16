#include <Arduino.h>

#include "config.h"
#include "utilities.h"
#include <TinyGsmClient.h>
#include <WiFi.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Connected to WIFI AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
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
    Serial.print("WiFi Event ID: ");
    Serial.println(eventID);
    WiFi.removeEvent(eventID);*/

    WiFi.begin(ssid, password);
    SerialMon.println();
    SerialMon.println();
    SerialMon.println("Wait for WiFi... ");
    while(!WiFi.isConnected())
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
    
}

void turnOffNetlight()
{
    SerialMon.println("Turning off SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=0");
}

void turnOnNetlight()
{
    SerialMon.println("Turning on SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=1");
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

    // Turn on the Modem power first
    digitalWrite(MODEM_POWER_ON, HIGH);

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);

    // Initialize the indicator as an output
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LED_OFF);

    // Restart takes quite some time
    SerialMon.println("Initializing modem...");
    modem.restart();

    // Turn off network status lights to reduce current consumption
    turnOffNetlight();

    // The status light cannot be turned off, only physically removed
    // turnOffStatuslight();

    // Or, use modem.init() if you don't need the complete restart
    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem: ");
    SerialMon.println(modemInfo);
    SerialMon.println();

    String imei = modem.getIMEI();
    SerialMon.print("imei: ");
    SerialMon.println(imei);
    SerialMon.println();

    SimStatus simstatus = modem.getSimStatus();
    if (simstatus == 0)
    {
        SerialMon.println("SIM ERROR!");
        modem.poweroff();
        SerialMon.println();
        printVoltages();
        for (int i = 0; i < 10; i++)
        {
            digitalWrite(LED_GPIO, LED_ON);
            delay(100);
            digitalWrite(LED_GPIO, LED_OFF);
            delay(100);
        }
    }

    // Unlock your SIM card with a PIN if needed
    if (strlen(simPIN) && simstatus != 3)
    {
        modem.simUnlock(simPIN);
    }
}



void measureNetwork()
{
    turnOnNetlight();

    String imsi = modem.getIMSI();
    SerialMon.print("IMSI: ");
    SerialMon.println(imsi);
    SerialMon.println();

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork(240000L))
    {
        SerialMon.println(" fail");

        // TODO: SEND ERROR INDICATION TO INFLUXDB

        return;
    }
    SerialMon.println(" OK");

    // When the network connection is successful, turn on the indicator
    digitalWrite(LED_GPIO, LED_ON);

    int16_t csq = modem.getSignalQuality();
    SerialMon.print("CSQ: ");
    SerialMon.println(csq);
    //TODO: SEND CSQ TO INFLUXDB

    if (modem.isNetworkConnected())
    {
        SerialMon.println("network connected");
    }
    else
    {
        SerialMon.println("lost network connection");
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

    turnOffNetlight();
    modem.poweroff();
    delay(1000);
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
    measureNetwork();

    // TODO: switch off modem via PMU
    // TODO: DEEP SLEEP
}

void loop()
{
    // NO LOOP
}