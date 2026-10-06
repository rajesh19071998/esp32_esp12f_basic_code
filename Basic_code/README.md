# ESP32 / ESP8266 OTA Control Project

This project provides a small web dashboard, OTA firmware update support, Wi‑Fi config, LED control, switch monitoring, mDNS hostname access, and Google Sheets reporting.

## Feature macros

Each PlatformIO environment has independent compile-time switches in `platformio.ini`. Set a value to `0` to compile that feature out, or `1` to include it:

```ini
-D ENABLE_WEB_UI=1
-D ENABLE_MQTT=1
-D ENABLE_MDNS=1
-D ENABLE_OTA=1
-D ENABLE_ESP_NOW=1
-D ENABLE_ALEXA=1
```

The web UI and OTA endpoints share the same HTTP server with Alexa but can be disabled independently. Wi-Fi remains active because MQTT, ESP-NOW metadata, Alexa, and cloud reporting may still require it.

## ESP-NOW

ESP-NOW is enabled for both the `esp32dev` and `esp8266` (`nodemcuv2`) environments by this build flag in `platformio.ini`:

```ini
build_flags =
  -D ENABLE_ESP_NOW=1
```

Set the value to `0` to compile the firmware without ESP-NOW support. No ESP-NOW headers or runtime setup are included in the disabled build.

When enabled, each board broadcasts a versioned status packet at startup, whenever an LED or physical switch changes, and every five seconds. The packet contains all LED and switch states, hostname, IPv4 address, Wi-Fi mode, Wi-Fi channel, and SSID/access-point name. Received status packets are printed to Serial.

Peers must use the same Wi-Fi channel. LED control packets can be sent with `esp_now_manager::sendLedControl(ledIndex, on)`; received controls are applied in the main loop through the existing `status_led` API. Broadcast packets are currently unencrypted, so use encrypted peers if controls will be accepted in an untrusted radio environment.

The ESP8266 environment uses GPIO 5 (D1) for the switch and GPIO 4 (D2) for the LED by default. Build a specific target with `pio run -e esp32dev` or `pio run -e esp8266`.

## Alexa

Local Alexa control is enabled for both boards with `-D ENABLE_ALEXA=1` in `platformio.ini`. Set it to `0` to compile out the Alexa manager. Each configured channel label is exposed as an Alexa on/off light, and its reported status follows changes made by Alexa, the web dashboard, MQTT, ESP-NOW, or a physical switch.

Alexa discovery requires the board and Echo device to be on the same 2.4 GHz network. After the board connects, use **Discover Devices** in the Alexa app, then control a channel by its configured label, such as "Alexa, turn on LED 1."

## 1) Open the web UI

Use either the board IP or the local hostname.

### By IP

- Dashboard: http://192.168.31.52/dashboard
- Device info: http://192.168.31.52/device
- Wi‑Fi settings: http://192.168.31.52/wifi-settings
- Firmware update: http://192.168.31.52/update

### By hostname

The device uses mDNS hostname registration and can usually be reached as:

- http://esp32-ota.local/
- http://esp32-ota.lan/

Use the one your network resolves correctly. In some environments, `.lan` works better than `.local`.

> Note: if your browser cannot resolve the hostname, use the IP address instead.

## 2) OTA login

Firmware update page is protected by a login form.

Default credentials:

- Username: `admin`
- Password: `admin`

Login URL:

- http://192.168.31.52/login

After successful login, the device redirects to:

- http://192.168.31.52/update

## 3) Main web pages

- `/dashboard` -> main dashboard page
- `/wifi-settings` -> Wi‑Fi and hotspot settings page
- `/update` -> OTA firmware/LittleFS update page
- `/login` -> OTA login page

## 4) Web API commands

All examples below assume the board IP is `http://192.168.31.52`.

### 4.1 Device info

#### GET /device

Purpose: read board metadata, memory usage, network mode and uptime.

Example:

```bash
curl http://192.168.31.52/device
```

Example response:

```json
{
  "hostname": "esp32-ota",
  "ip": "192.168.31.52",
  "mdns": "esp32-ota.local",
  "swVersion": "1.0.0",
  "buildDate": "Sep 23 2026",
  "buildTime": "12:34:56",
  "ramTotal": 327680,
  "ramUsed": 49988,
  "flashTotal": 1900544,
  "flashUsed": 1096481,
  "wifiMode": "Wi‑Fi",
  "hotspotName": "ESP32-OTA",
  "hotspotPassword": "firmware123",
  "hotspotIp": "192.168.4.1",
  "uptime": "00:12:34"
}
```

Explanation:

- `hostname` = board name
- `ip` = active Wi‑Fi IP address
- `wifiMode` = current mode: `Wi‑Fi`, `Hotspot`, or `Wi‑Fi + Hotspot`
- `uptime` = time since reboot in `HH:MM:SS`

---

### 4.2 Get LED states

#### GET /leds

Purpose: get all LED states.

Example:

```bash
curl http://192.168.31.52/leds
```

Example response:

```json
{"leds":[true,false,true,false]}
```

Explanation:

- `true` = LED ON
- `false` = LED OFF

---

### 4.3 Set one LED

#### POST /led

Purpose: turn one LED ON or OFF.

Use form data or query string:

```bash
curl -X POST "http://192.168.31.52/led?led=0&state=1"
```

Or:

```bash
curl -X POST http://192.168.31.52/led \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "led=0&state=1"
```

Example response:

```json
{"leds":[true,false,true,false]}
```

Explanation:

- `led` = index of LED (0..3)
- `state` = `1` for ON, `0` for OFF

---

### 4.4 Set LED state by JSON payload

#### POST /leds

Purpose: toggle multiple LEDs using JSON.

Example:

```bash
curl -X POST http://192.168.31.52/leds \
  -H "Content-Type: application/json" \
  --data '{"led":0,"state":1}'
```

Example response:

```json
{"leds":[true,false,true,false]}
```

Another example:

```bash
curl -X POST http://192.168.31.52/leds \
  -H "Content-Type: application/json" \
  --data '{"leds":[0,2],"states":[1,0]}'
```

---

### 4.5 Read switch states

#### GET /switches

Purpose: read the physical switch states.

Example:

```bash
curl http://192.168.31.52/switches
```

Example response:

```json
{"switches":[false,true,false,true]}
```

Explanation:

- `true` = pressed / active
- `false` = released / inactive

---

### 4.6 Read persistence setting

#### GET /persist

Purpose: read whether LED state persistence is enabled in EEPROM.

Example:

```bash
curl http://192.168.31.52/persist
```

Example response:

```json
{"persistenceEnabled":true}
```

#### POST /persist

Purpose: enable or disable EEPROM saving of LED states.

Example:

```bash
curl -X POST "http://192.168.31.52/persist?enabled=1"
```

or:

```bash
curl -X POST http://192.168.31.52/persist \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "enabled=1"
```

Example response:

```json
{"persistenceEnabled":true}
```

---

### 4.7 Read Wi‑Fi settings

#### GET /wifi-settings?json=1

Purpose: read current Wi‑Fi and hotspot credentials stored in EEPROM.

Example:

```bash
curl "http://192.168.31.52/wifi-settings?json=1"
```

Example response:

```json
{
  "wifiSsid": "MyWiFi",
  "wifiPassword": "MyPassword",
  "apName": "ESP32-OTA",
  "apPassword": "firmware123"
}
```

---

### 4.8 Save Wi‑Fi settings

#### POST /wifi-settings

Purpose: write new Wi‑Fi station and hotspot settings to EEPROM.

Example:

```bash
curl -X POST http://192.168.31.52/wifi-settings \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "wifiSsid=MyWiFi&wifiPassword=MyPassword&apName=ESP32-OTA&apPassword=firmware123"
```

Example response:

```json
{"status":"saved"}
```

Explanation:

- `wifiSsid` = Wi‑Fi network name
- `wifiPassword` = Wi‑Fi password
- `apName` = hotspot name
- `apPassword` = hotspot password

---

### 4.9 OTA login

#### GET /login

Purpose: open login page.

Example:

```bash
curl http://192.168.31.52/login
```

#### POST /login

Purpose: authenticate OTA access.

Example:

```bash
curl -X POST http://192.168.31.52/login \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "username=admin&password=admin"
```

Explanation:

- successful login sets a browser cookie and redirects to `/update`
- default credentials are `admin / admin`

---

### 4.10 Firmware update page

#### GET /update

Purpose: open OTA update page for firmware or LittleFS upload.

Example:

```bash
curl http://192.168.31.52/update
```

#### POST /upload-mode

Purpose: choose the upload type.

Example:

```bash
curl -X POST http://192.168.31.52/upload-mode \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "type=firmware"
```

or:

```bash
curl -X POST http://192.168.31.52/upload-mode \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "type=littlefs"
```

Example response:

```text
firmware
```

#### POST /update

Purpose: upload the selected firmware or LittleFS image.

Example using curl with file upload:

```bash
curl -X POST http://192.168.31.52/update \
  -F "firmware=@firmware.bin" \
  -F "uploadType=firmware"
```

Example response on success:

```text
Firmware update complete. Rebooting device.
```

Example response for LittleFS:

```text
LittleFS update complete. Rebooting device.
```

Explanation:

- `firmware` upload updates the application firmware
- `littlefs` upload updates the file system image
- after upload, the device reboots automatically

## 5) Quick command summary

| Action | HTTP method | URL |
| --- | --- | --- |
| Open dashboard | GET | `/dashboard` |
| Open info JSON | GET | `/device` |
| Read LED states | GET | `/leds` |
| Turn LED on/off | POST | `/led` |
| Read switch states | GET | `/switches` |
| Read persistence | GET | `/persist` |
| Set persistence | POST | `/persist` |
| Read Wi‑Fi config | GET | `/wifi-settings?json=1` |
| Save Wi‑Fi config | POST | `/wifi-settings` |
| Firmware update page | GET | `/update` |
| Login | POST | `/login` |
| Upload firmware | POST | `/update` |

## 6) Notes

- The dashboard is served by the board itself over Wi‑Fi.
- Hostnames depend on the network and browser support for `.local` or `.lan` name resolution.
- If hostname resolution fails, always use the IP address.
- OTA firmware updates require login before upload.
- Credentials may be changed in the firmware source if needed.

## 7) Typical usage flow

1. Connect the board to Wi‑Fi.
2. Open the browser and visit the IP or hostname.
3. Use the dashboard to control LEDs and view device info.
4. Go to `/update` when updating firmware.
5. Login with `admin / admin`.
6. Upload the generated `.bin` file.

## 8) Example browser URLs

- http://192.168.31.52/dashboard
- http://192.168.31.52/wifi-settings
- http://192.168.31.52/update
- http://esp32-ota.lan/dashboard
- http://esp32-ota.local/dashboard

These are the most common ways to access the device from a browser.
