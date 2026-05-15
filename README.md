# DIY Photobooth Controller

ESP32 firmware for a DIY photobooth camera controller. The current code controls a camera shutter with a servo and advances paper with a stepper motor, all from a small web interface hosted directly on the ESP32.

## Features

- Servo-driven camera shutter with configurable open and closed angles.
- Timed snapshot action that opens the shutter and automatically closes it after the selected duration.
- Stepper-driven paper feeder with feed, jog forward, jog reverse, and stop controls.
- Optional auto-feed after a snapshot.
- Persistent settings stored in ESP32 NVS flash.
- WiFi station mode with a browser-based control panel.

## Hardware

Default pin assignments are defined in `src/config.h`.

| Function | ESP32 GPIO |
| --- | --- |
| Shutter servo signal | GPIO 27 |
| Stepper IN1 | GPIO 26 |
| Stepper IN2 | GPIO 25 |
| Stepper IN3 | GPIO 33 |
| Stepper IN4 | GPIO 32 |

The feeder is configured for a 4-wire stepper driver using a two-coil full-step sequence. The default stepper configuration assumes `2048` steps per revolution.

## Project Layout

```text
src/
  main.cpp            Firmware startup and main loop
  config.h            Pin assignments and default limits
  shutter.*           Servo shutter control
  paper_feeder.*      Stepper feed control
  web.*               HTTP routes and web interface wiring
  html.h              Embedded browser UI
  settings.*          NVS-backed persistent integer settings
  wifi_manager.*      WiFi connect and reconnect logic
include/
  wifi_secrets.example.h
```

## Setup

1. Install PlatformIO.
2. Copy the WiFi example header:

   ```powershell
   Copy-Item include\wifi_secrets.example.h include\wifi_secrets.h
   ```

3. Edit `include/wifi_secrets.h` with your WiFi network name and password:

   ```cpp
   static const char* WIFI_SSID = "YOUR_SSID";
   static const char* WIFI_PASS = "YOUR_PASSWORD";
   ```

`include/wifi_secrets.h` is ignored by git so real credentials are not committed.

## Build and Upload

Build the firmware:

```powershell
C:\Users\halcyon\.platformio\penv\Scripts\pio.exe run
```

Upload to an ESP32 dev board:

```powershell
C:\Users\halcyon\.platformio\penv\Scripts\pio.exe run --target upload --environment esp32dev
```

Open the serial monitor:

```powershell
C:\Users\halcyon\.platformio\penv\Scripts\pio.exe device monitor --baud 115200
```

After boot, the ESP32 prints its IP address to the serial monitor. Open that IP address in a browser on the same network to use the control panel.

## Web Controls

The root page provides:

- Open and close shutter buttons.
- Editable shutter open and close angles.
- Snapshot duration in milliseconds.
- Paper feed rotations and feed speed.
- Jog controls for feeder calibration.
- Auto-feed toggle for feeding paper after a snapshot.
- Stop motor button.

## HTTP API

The web UI uses these endpoints internally:

| Route | Method | Purpose |
| --- | --- | --- |
| `/` | GET | Serve the control page |
| `/open` | POST | Move shutter to the configured open angle |
| `/close` | POST | Move shutter to the configured closed angle |
| `/save?open=<0-180>&close=<0-180>` | POST | Save shutter angles |
| `/snapshot?duration=<1-60000>` | POST | Open shutter temporarily, then auto-close |
| `/feed?rotations=<1-100>&speed=<100-1000>` | POST | Feed paper |
| `/feed/jog?steps=<-4096..4096>&speed=<100-1000>` | POST | Jog feeder for calibration |
| `/feed/stop` | POST | Stop and release the feeder motor |
| `/feed/settings` | GET | Read saved feeder settings |
| `/feed/settings?rotations=<1-100>&speed=<100-1000>` | POST | Save feeder settings |
| `/feed/auto?enabled=<0-or-1>` | POST | Enable or disable auto-feed after snapshot |

## Configuration Notes

- Shutter angle defaults are `106` degrees open and `74` degrees closed.
- Snapshot duration is limited to `1` through `60000` ms.
- Feed speed is limited to `100` through `1000` steps per second.
- Feed rotation count is limited to `1` through `100`.
- Saved shutter and feeder settings persist across ESP32 reboots.
