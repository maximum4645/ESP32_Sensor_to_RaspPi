# ESP32 Onboard Sensor Data to Raspberry Pi with Encryption
## Overview
This project implements secure communication between an ESP32-S2 microcontroller and a Flask-based server. The ESP32 collects data from various sensors, encrypts it using AES (with a key derived from Diffie-Hellman key exchange), and transmits the encrypted data to the Flask server for decryption and processing.

**youtube**: https://youtu.be/n-s7BczhbVI?si=uzhSN1f2a_-Ab_6s<br/>

![p4](https://github.com/user-attachments/assets/f486236f-72eb-4410-bc54-c6d9317f0a9f)

## ESP32 side
- Thread 1: update_sensor_data
  - Collects data from sensors every 10 seconds:
    - BMP280: Temperature, Pressure
    - MPU6050: Acceleration, Gyroscope, Temperature
    - SHT4x: Temperature, Humidity
  - Encrypts the collected data using AES in CBC mode with a 16-byte key derived from the Diffie-Hellman key exchange.
  - Transmits the encrypted data to the Flask server over HTTP.
- Thread 2: print_local_time
  - Updates and prints the current local time every second.
- Thread 3: update_with_NTP
  - Synchronizes the ESP32's time with an NTP server every minute.
  - Ensures that timestamps in the collected data are accurate and consistent.

## Raspberry Pi side
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
