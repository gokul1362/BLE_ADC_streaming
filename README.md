# BLE_ADC_Stream

This is an ESP-IDF project that uses an ESP32 to sample an analog input (ADC) every 100ms, bundles 10 samples every 1 second, and sends them over BLE using a custom GATT characteristic.

---

## 📡 Overview

- The ESP32 acts as a **BLE peripheral (server)**.
- It advertises a custom **GATT service** named `sampler`.
- Inside the service is a **characteristic** named `stream` that supports **Notify**.
- Every 1 second, the device samples the analog pin 10 times (100ms interval) and sends a 20-byte notification (10 `uint16_t` values, little-endian).

---

## 🛠 Hardware Requirements

- ESP32C6 board 
- Floating or analog voltage source on **GPIO36** (ADC1 Channel 0)

---

## 🔧 Software Requirements

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- USB cable to flash the board

---

## 📁 Project Structure

BLE_ADC_Stream/
├── main/
│ ├── main.c # Core logic: BLE + ADC sampling
│ └── CMakeLists.txt
├── CMakeLists.txt
└── README.md


---

## ⚙️ BLE Service Details

| Element        | UUID                                 | Notes                        |
|----------------|----------------------------------------|------------------------------|
| Service        | `0x00FF` → `0000FF00-0000-1000-8000-00805F9B34FB` | Named `sampler` |
| Characteristic | `0xFF01` → `0000FF01-0000-1000-8000-00805F9B34FB` | Named `stream`, Notify-only |

- Notify payload: **20 bytes = 10 ADC samples** (`uint16_t[]`, little-endian)

---

## 📦 How to Build & Flash

```bash
# Set up your ESP32 target (once)
idf.py set-target esp32c6

# Build
idf.py build

# Flash and monitor (replace PORT)
idf.py -p PORT flash monitor

📲 How to See the Results
Once your ESP32 is flashed and powered, it begins advertising as BLE_Sampler. You can view the streamed ADC values using a BLE-enabled mobile app. Since I could not make an APP via MIT app inventor due to an extension issue, the Firmware can be verified via nRF connect.

✅ Using nRF Connect (Recommended for Quick Testing)
Install the nRF Connect app (Android/iOS).

Scan for BLE devices and connect to BLE_Sampler.

Expand the service 0x00FF, find the characteristic 0xFF01.

Tap the “Enable notifications” button (bell or 3-arrow icon).

You’ll receive notifications every second with 20 bytes (10 ADC samples).