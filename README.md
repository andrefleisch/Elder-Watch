# Elder Watch

ESP32 prototype for fall detection, emergency alerts, and medication reminders.

## Overview

Elder Watch explores a simple problem: when an older adult is alone, a fall or other emergency may go unnoticed. The prototype combines motion and sound sensing with a manual emergency button, then sends an alert to a configured Telegram chat. It also hosts a local dashboard for motion monitoring and medication-alarm management.

> **Academic context:** Elder Watch was developed collaboratively as a Software Engineering university project at the Pontifical Catholic University of Paraná (PUCPR).

## Features

| Feature | Implementation |
| --- | --- |
| Automatic fall alerts | Detects a low-acceleration event followed by impact, or a sustained tilt |
| Manual emergency alerts | Sends an alert when the physical button is pressed |
| Sound context | Samples the sound sensor when a fall is detected and reports whether the fixed threshold was reached |
| Telegram notifications | Sends the event type and timestamp to a configured chat |
| Medication reminders | Stores up to five daily alarms and activates an LED and buzzer for five seconds |
| Local web dashboard | Shows recent acceleration data, the last recorded event time, and controls for adding or deleting alarms |

## How fall detection works

The MPU6050 readings are passed through a five-sample moving-average filter. The sketch then evaluates two detection paths:

1. **Free fall and impact:** acceleration falls below `1.2 g`, then rises above `2.0 g` within `1.5 seconds`.
2. **Sustained tilt:** the calculated tilt remains above `45°` for more than `3 seconds`.

Either path triggers the same fall-alert flow. These thresholds are constants near the top of [`main.ino`](main.ino) and require testing for the intended mounting position and hardware.

## Hardware

- ESP32 development board
- MPU6050 accelerometer/gyroscope module
- Analog sound sensor
- Push button
- Active buzzer
- Two LEDs with appropriate current-limiting resistors
- Breadboard and jumper wires, or an equivalent circuit

### Pin mapping

| Component | ESP32 connection | Notes |
| --- | ---: | --- |
| Emergency button | GPIO 18 | Configured with the ESP32's internal pull-up resistor |
| Buzzer | GPIO 19 | Digital output |
| Alarm LED | GPIO 25 | Medication-reminder indicator |
| Alert LED | GPIO 5 | Fall and button-alert indicator |
| Sound sensor | GPIO 33 | Read through `analogRead()` |
| MPU6050 | Board-default I²C pins | The sketch initializes I²C with `Wire.begin()` |

## Technologies

- Arduino framework for ESP32 (C++)
- MPU6050 sensor library
- UniversalTelegramBot
- ArduinoJson
- ESP32 Wi-Fi, HTTP client, secure client, NTP time, and web-server libraries
- Chart.js, loaded from a CDN by the device-hosted dashboard

## Setup

### 1. Prepare the Arduino environment

Install ESP32 board support in the Arduino IDE, select the correct ESP32 board and port, and install libraries that provide these headers:

- `MPU6050.h`
- `UniversalTelegramBot.h`
- `ArduinoJson.h`

The remaining headers used by the sketch are supplied by the Arduino core, the ESP32 core, or the standard C++ library.

### 2. Assemble and configure

1. Wire the components according to the pin table above and the I²C pins for your ESP32 board.
2. In [`main.ino`](main.ino), replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD`.
3. Create a Telegram bot through [BotFather](https://t.me/BotFather), then replace `YOUR_BOT_TOKEN` and `YOUR_CHAT_ID`.
4. Upload the sketch and open the Serial Monitor at `115200` baud.
5. After the ESP32 connects, open the printed IP address from a device on the same local network.

The ESP32 restarts and retries if it cannot connect to Wi-Fi within 15 seconds. Telegram alerts, NTP synchronization, IP-based location lookup, and the dashboard's Chart.js asset require internet access.

## Configuration

Detection thresholds, timing windows, pin assignments, and the maximum number of alarms are defined near the top of [`main.ino`](main.ino). Adjusting them changes sensitivity and may increase false positives or missed detections; validate changes with the assembled prototype.

Network and Telegram values are compile-time strings. Keep real credentials out of commits, and rotate a bot token if it has ever been published.

## Known limitations

- Alarms are stored only in memory and are lost when the ESP32 restarts.
- Alert handling blocks the main loop for 10 seconds; the five-second medication alarm is also blocking. Sensor sampling and web requests pause during these delays.
- The dashboard has no authentication. Anyone on the same network who can reach the ESP32 can view it and add or delete alarms.
- The code requests an approximate IP-based location, but the returned link is not included in Telegram messages. IP geolocation is not a precise device location.
- TLS certificate verification for Telegram is disabled with `client.setInsecure()`, and the location lookup uses plain HTTP.
- Time is configured with a fixed UTC−3 offset rather than a selectable time zone.
- Detection and sound thresholds are fixed in code and are not calibrated automatically.

This is an academic prototype, not a certified medical or emergency-response device.

## Repository structure

```text
.
├── main.ino     # ESP32 firmware and embedded web dashboard
├── README.md    # Project documentation
├── .gitignore   # Local and generated-file exclusions
└── LICENSE      # MIT license
```

## License

This repository is available under the [MIT License](LICENSE).
