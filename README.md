

<p align="center">
  <img src="https://github.com/ianwal/batterymonitor/assets/52143079/0b73d81e-2a91-4f7b-a16e-5897b8bf1a87" width="200" height="300">
  <img src="https://github.com/ianwal/batterymonitor/assets/52143079/21f15508-2c54-4fc7-be62-a272eb7c4d09" width="500" height="300">
</p>

<p align="center">
  Wi-Fi battery monitor using a custom ESP32-C3 board
</p>

## Hardware:
The PCB was manufactured using JLCPCB. 
- I'm not sure if JLCPCB's fabrication service has all the required components in their inventory, so I assembled it myself.

Components were ordered off of Mouser. You can find the complete bill of materials from the KiCad project.

Connect + and - from the battery to the terminal block to measure the battery voltage.

## Software:
Clone the repository and flash the microcontroller using the PlatformIO IDE Visual Studio Code extension.

Fill in your Home Assistant URL, Long Lived Access Token, Wi-Fi SSID, and Wi-Fi Password in `include/secrets.h`

### Dependencies:
- [esp-ha-lib](https://github.com/ianwal/esp-ha-lib)
- FreeRTOS
