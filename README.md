# Embedded Smart Lock

A smart lock system built with ESP32, MQTT, Firebase, and Flutter. The project
includes ESP32 firmware for the physical lock and a mobile app for authentication,
remote control, lock management, and activity history.

## Project Structure

```text
.
|-- firmware-esp32/   # ESP32 firmware using PlatformIO + Arduino
|-- my_app/           # Flutter mobile app
`-- README.md
```

## Features

- Unlock by password, RFID card, exit button, or mobile app command.
- Publish lock status, battery level, and activity events through MQTT.
- Manage RFID cards from the app.
- Store users and lock history with Firebase Authentication and Firestore.
- Auto-lock and temporary lockout after repeated failed attempts.

## Tech Stack

- ESP32, Arduino framework, PlatformIO
- Flutter, Riverpod
- Firebase Authentication, Cloud Firestore
- MQTT over TLS
- RC522 RFID, keypad, LCD I2C, buzzer, relay/solenoid lock

## Firmware

Requirements:

- PlatformIO CLI or VS Code with PlatformIO
- ESP32 development board
- Hardware wiring from `firmware-esp32/docs/HARDWARE.md`

Build:

```bash
cd firmware-esp32
make build
```

Upload:

```bash
make upload
```

Serial monitor:

```bash
make monitor
```

## Flutter App

Requirements:

- Flutter SDK
- Firebase project with Authentication and Firestore enabled
- Firebase config files generated for the target platforms

Install dependencies:

```bash
cd my_app
flutter pub get
```

Run:

```bash
flutter run
```

## MQTT

Base topic:

```text
smartlock/<lock_id>
```

Common topics:

```text
smartlock/<lock_id>/status
smartlock/<lock_id>/cmd
```

Example command:

```json
{
  "action": "unlock",
  "by": "Mobile App"
}
```

More details are available in `firmware-esp32/docs/MQTT_API.md`.

## Security Notes

Do not commit real credentials to a public repository, including:

- Wi-Fi SSID/password
- MQTT username/password
- Keystore files and `key.properties`
- Private keys or service account files

Firebase client config files may be required to build the app, but Firebase
security must still be enforced through proper Authentication, Firestore rules,
and Google Cloud API restrictions.
