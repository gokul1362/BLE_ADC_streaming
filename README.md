# BLE_ADC_Stream

This is an ESP-IDF project that uses an ESP32 to sample an analog input (ADC) every 100ms, bundles 10 samples every 1 second, and sends them over BLE using a custom GATT characteristic.

## 📡 Overview

- The ESP32 acts as a **BLE peripheral (server)**.
- It advertises a custom **GATT service** named `sampler`.
- Inside the service is a **characteristic** named `stream` that supports **Notify**.
- Every 1 second, the device samples the analog pin 10 times (100ms interval) and sends a 20-byte notification (10 `uint16_t` values).

## 🛠 Hardware Requirements

- ESP32 board (e.g., ESP32-WROOM-32)
- Floating or analog voltage source on **GPIO36** (ADC1 Channel 0)

## 🔧 Software Requirements

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- USB cable to flash the board

## 📁 Project Structure

├── main/
│ ├── main.c # Core logic: BLE + ADC sampling
│ └── CMakeLists.txt
├── CMakeLists.txt
└── README.md


## ⚙️ BLE Service Details

| Element        | UUID                                              | Notes                        |
|----------------|---------------------------------------------------|------------------------------|
| Service        | `0x00FF` → `0000FF00-0000-1000-8000-00805F9B34FB` | Named `sampler` |
| Characteristic | `0xFF01` → `0000FF01-0000-1000-8000-00805F9B34FB` | Named `stream`, Notify-only  |

- Notify payload: 20 bytes = 10 ADC samples (`uint16_t[]`, little-endian)

## 📦 How to Build & Flash

```bash
# Select your IDF terminal via ctr+shit+p and then do the following commands

# Set up your ESP32 target (once)
idf.py set-target esp32c6

# Configure project if needed
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p PORT flash # to Flash

idf.py -p PORT flash # to see the output
