# Fishtank

This project simulates the moon moving across the sky over the course of a night using a string of neopixels and a freetronics RTC module.

## UI

The parameters can be set via bluetooth using the corresponding Android app. All times are in `seconds%day` units and vary from `0` to `24*60*60-1`

* Moonrise time
* Moonset time
* Current time
* Moon brighness
* Moon colour
* Moon width
* Number of pixels in the strip

## Pinout

| Data | Arduino Pin | Connect to |
| ------- | ------- | ------- |
| neopixel out | Pin *6* | neopixel in |
| I2C | SDA/SCL (the pins vary depending on the board) | RTC SDA/SCL  |
| Bluetooth TX | Pin ?? | Bluetooth module RX |
| Bluetooth RX | Pin ?? | Bluettoth module TX |

# License

[This project is released into the public domain under the terms of the unlicense](./LICENSE).


