# ESP32 Onboard Sensor Data to Raspberry Pi with Encryption
## Overview
This project implements secure communication between an ESP32-S2 microcontroller and a Flask-based server. The ESP32 collects data from various sensors, encrypts it using AES (with a key derived from Diffie-Hellman key exchange), and transmits the encrypted data to the Flask server for decryption and processing.

## ESP32 side:
- Collects sensor data:
  - BMP280: Temperature, Pressure
  - MPU6050: Acceleration, Gyroscope, Temperature
  - SHT4x: Temperature, Humidity
- Synchronizes time with an NTP server
- Uses Diffie-Hellman key exchange for secure AES key generation
- Encrypts data with AES and transmits it to the server over HTTP

## Raspberry Pi side:
- Handles Diffie-Hellman key exchange
- Decrypts AES-encrypted sensor data
- Exposes JSON-formatted sensor data via a RESTful endpoint

## How It Works
- Key Exchange: Diffie-Hellman key exchange is performed between the ESP32 and Flask server to derive a shared secret for AES encryption.
- Data Encryption: The ESP32 encrypts sensor data using AES with PKCS#7 padding. <br/>
- Data Transmission: The encrypted data is sent to the Flask server over HTTP.
- Data Decryption: The Flask server decrypts the data using the shared AES key and makes it available as JSON.

## Usage
- Deploy the ESP32 firmware using PlatformIO.
- Set up the Flask server in a Python environment and install dependencies using requirements.txt.
- Run the server and interact with the provided endpoints:
  - "/dh-key-exchange": Perform Diffie-Hellman key exchange
  - "/sensor": Fetch and decrypt encrypted sensor data
