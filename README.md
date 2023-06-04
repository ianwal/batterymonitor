<div align="center">
  
  <img src="https://github.com/ianwal/batterymonitor/assets/52143079/0b73d81e-2a91-4f7b-a16e-5897b8bf1a87" width="200" height="300">
  
  <img src="https://github.com/ianwal/batterymonitor/assets/52143079/21f15508-2c54-4fc7-be62-a272eb7c4d09" width="500" height="300">
  
  Wi-Fi battery monitor using a custom [ESP32-C3](https://www.espressif.com/sites/default/files/documentation/esp32-c3-wroom-02_datasheet_en.pdf) board
  
</div>

---
## About:

I drive a Crown Victoria Police Interceptor. It is *old*. The only way I can tell if it has issues is through the instrument cluster.

When I don't drive for extended periods, the battery can drain to a harmful level. I needed an alert system for when the battery needs charging to avoid killing it (again, oops).

But, third-party battery monitor systems require a nearby Phone and Bluetooth. This is a problem.

#### Solution: Build my own!

## Hardware:
#### Features:
- GPIO screw terminal block with adjustable voltage divider
- [Low Iq LDO regulator](https://www.diodes.com/assets/Datasheets/AP7361C.pdf)
- [High accuracy temperature sensor](https://www.ti.com/lit/ds/symlink/tmp235-q1.pdf)
- USB-C
- Direct USB Serial/JTAG control without a USB to UART IC

The PCB was fabricated through [JLCPCB](https://jlcpcb.com/)
- JLCPCB's assembly service does not have the required components in their inventory. I assembled mine myself.

Components were ordered from [Mouser Electronics](https://www.mouser.com). You can find the complete bill of materials in the KiCad project.

## Software:
#### Features:
- Deep sleep to reduce power consumption

#### Setup:

Clone the repository and flash the microcontroller using the PlatformIO IDE Visual Studio Code extension.

Fill in your Home Assistant URL, Long Lived Access Token, Wi-Fi SSID, and Wi-Fi Password in `include/secrets.h`

#### Dependencies:
- [esp-ha-lib](https://github.com/ianwal/esp-ha-lib)
- FreeRTOS

You can modify the program to use a web server other than [Home Assistant](https://www.home-assistant.io/) easily. I use it for other IOT devices, so it was useful to incorporate into this project.

## Performance:

Voltage accuracy is around ± 0.1 V

Power consumption is still being evaluated
