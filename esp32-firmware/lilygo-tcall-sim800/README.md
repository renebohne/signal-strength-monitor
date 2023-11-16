# TTGO T-CALL SIM800 Firmware 

This folder contains the firmware for the Lilygo T-Call-SIM800 board.

More information about the board and example code can be found here: <https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800>

## Installation

This project uses PlatformIO. You can use PlatformIO core in a terminal, but we recommend to use Visual Studio Code and the PlatformIO extension.

## External Libraries

The following libraries will be installed automatically by platformio:

- [TinyGSM](https://github.com/vshymanskyy/TinyGSM)
- [StreamDebugger](https://github.com/vshymanskyy/StreamDebugger)
- [AXP202X_Library](https://github.com/lewisxhe/AXP202X_Library)
- [InfluxDB-Client-for-Arduino](https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino)

A good tutorial about Influxdb and Arduino: https://randomnerdtutorials.com/esp32-influxdb/

## Troubleshooting

If your modem does not work as expected, try some of these commands:
```c
    /*
        modem.sendAT(GF("+CMEE=0"));  // turn off error codes
        modem.waitResponse();
        modem.sendAT(GF("+CIURC=0"));  // turn off URC presentation
        modem.waitResponse();
        modem.sendAT(GF("&w"));  // save
        modem.waitResponse();
        modem.streamClear();
        delay(5000);
    */

    // modem.factoryDefault();
```