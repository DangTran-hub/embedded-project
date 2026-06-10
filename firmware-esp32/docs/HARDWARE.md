# Hardware Notes

## ESP32 Pin Map

| Function | GPIO |
| --- | ---: |
| RFID SDA/SS | 5 |
| RFID RST | 32 |
| Buzzer | 4 |
| Exit button | 34 |
| Solenoid relay | 17 |
| Battery ADC | 35 |
| I2C SDA | 21 |
| I2C SCL | 22 |

## Keypad Matrix

Rows:

```text
33, 25, 26, 27
```

Columns:

```text
14, 12, 13
```

Layout:

```text
1 2 3
4 5 6
7 8 9
* 0 #
```

## Battery Monitor

The firmware reads the battery from `GPIO35` and converts the ADC value using a voltage divider ratio of `4.7`.

Current voltage range mapping:

| Voltage | Percent |
| ---: | ---: |
| 9.0V | 0% |
| 12.6V | 100% |

Update `batteryMonitorBegin(BATTERY_PIN, 4.7f)` if the resistor divider changes.
