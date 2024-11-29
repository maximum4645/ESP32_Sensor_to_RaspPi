/*
Thread 1: update_sensor_data
- Collects data from sensors every 10 seconds:
  - BMP280: Temperature, Pressure
  - MPU6050: Acceleration, Gyroscope, Temperature
  - SHT4x: Temperature, Humidity
- Encrypts the collected data using AES in CBC mode with a 16-byte key derived from the Diffie-Hellman key exchange.
- Transmits the encrypted data to the Flask server over HTTP.
Thread 2: print_local_time
- Updates and prints the current local time every second.
- Sends the local time to the server to keep track of when data is collected.
Thread 3: update_with_NTP
- Synchronizes the ESP32's time with an NTP server every minute.
- Ensures that timestamps in the collected data are accurate and consistent.
- Sends the updated time to the Flask server for additional processing.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
// #include <SPIFFS.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_HTS221.h>
#include <Adafruit_MPU6050.h>
#include "Adafruit_SHT4x.h"
#include <Base64.h>
#include "mbedtls/aes.h"

// Global Variable Initialization

const char* ssid = "WIFI_SSID";
const char* password = "PASSWORD";

WebServer server(80);

const long gmtOffset_sec = 7 * 3600;    // GMT+7 for Thailand (7 hours * 3600 seconds)
const int daylightOffset_sec = 0;       // No daylight saving time in Thailand
const char* ntpServer = "pool.ntp.org"; // NTP server

Adafruit_BMP280 bmp;
Adafruit_HTS221 hts;
Adafruit_MPU6050 mpu;
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

float pressure_bmp, temp_bmp, ax, ay, az, gx, gy, gz, temp_mpu, temp_sht4, humid_sht4;

String current_time = "";

// Diffie-Hellman Parameters
const uint32_t P = 23;  // Small prime number
const uint32_t G = 5;   // Generator
uint32_t private_key;   // Randomly generated private key
uint32_t public_key;    // ESP32's public key
uint32_t shared_secret; // Computed shared secret

// Function Declaration

// void handleRoot() {
//   File file = SPIFFS.open("/index.html", "r");
//   if (!file) {
//     server.send(404, "text/plain", "File not found");
//     return;
//   }
//   server.streamFile(file, "text/html");
//   file.close();
// }

void print_local_time(void * parameter) {
  struct tm timeinfo;
  char timeStringBuff[64];  // Buffer to hold the formatted time string
  
  while (1) {  // Infinite loop to continuously print the time
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
    } else {
      // Format the time and store it in the buffer
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      
      // Assign the formatted time to the global variable
      current_time = String(timeStringBuff);

      // Print the current time to the serial monitor
      Serial.println(current_time);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay for 1 second
  }
}

void update_with_NTP(void * parameter) {
  while (1) {
    Serial.println("Updating time from NTP server...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  // Sync with NTP
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Give time for the NTP sync to complete

    // Print the updated time after syncing
    struct tm timeinfo;
    char timeStringBuff[64];  // Buffer for formatted time string

    if (getLocalTime(&timeinfo)) {
      // Format the time and store it in the buffer
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      
      // Assign the formatted time to the global variable
      current_time = String(timeStringBuff);

      Serial.println("Time successfully updated:");
      Serial.println(current_time);
    } else {
      Serial.println("Failed to obtain updated time after NTP sync");
    }

    vTaskDelay(60000 / portTICK_PERIOD_MS);  // Delay for 1 minute before next update
  }
}

// Function to calculate modular exponentiation
uint32_t mod_exp(uint32_t base, uint32_t exp, uint32_t mod) {
    uint32_t result = 1;
    for (uint32_t i = 0; i < exp; i++) {
        result = (result * base) % mod;
    }
    return result;
}

// Generate DH keys
void generate_dh_keys() {
    private_key = random(1, P - 1); // Random private key
    public_key = mod_exp(G, private_key, P); // Calculate public key
    Serial.printf("Generated Public Key: %d\n", public_key);
}

// Handle DH key exchange (server endpoint)
void handle_dh_key_exchange() {
    if (!server.hasArg("public_key")) {
        server.send(400, "text/plain", "Public key missing");
        return;
    }

    // Parse received public key
    uint32_t received_public_key = server.arg("public_key").toInt();
    Serial.printf("Received Public Key: %d\n", received_public_key);

    // Calculate shared secret
    shared_secret = mod_exp(received_public_key, private_key, P);
    Serial.printf("Calculated Shared Secret: %d\n", shared_secret);

    // Respond with ESP32's public key
    server.send(200, "text/plain", String(public_key));
}

void format_data() {

    String jsonResponse = "{\"pressure_bmp\": " + String(pressure_bmp / 100) +
                          ", \"temp_bmp\": " + String(temp_bmp) +
                          ", \"ax\": " + String(ax) +
                          ", \"ay\": " + String(ay) +
                          ", \"az\": " + String(az) +
                          ", \"gx\": " + String(gx) +
                          ", \"gy\": " + String(gy) +
                          ", \"gz\": " + String(gz) +
                          ", \"temp_mpu\": " + String(temp_mpu) +
                          ", \"temp_sht4\": " + String(temp_sht4) +
                          ", \"humid_sht4\": " + String(humid_sht4) +
                          ", \"current_time\": \"" + current_time + "\"" + "}";

    // AES key and IV (leave unchanged)
    // uint8_t key[16] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    //                    0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66}; // "1234567890abcdef"

    uint8_t key[16];
    for (int i = 0; i < 16; i++) {
        key[i] = (shared_secret >> (8 * (i % 4))) & 0xFF;  // Map shared_secret into 16 bytes
    }

    uint8_t iv[16] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                      0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66};  // "1234567890abcdef"

    // Add PKCS#7 padding
    size_t jsonLen = jsonResponse.length();
    size_t paddedLen = ((jsonLen + 15) / 16) * 16;  // Round up to a multiple of 16
    uint8_t plainText[paddedLen];
    memset(plainText, 0, paddedLen);  // Fill with zeros
    memcpy(plainText, jsonResponse.c_str(), jsonLen);  // Copy JSON into buffer
    uint8_t paddingValue = paddedLen - jsonLen;
    for (size_t i = jsonLen; i < paddedLen; i++) {
        plainText[i] = paddingValue;  // Apply PKCS#7 padding
    }

    // Encrypt the full padded plaintext
    uint8_t encryptedData[paddedLen];  // Buffer for encrypted data
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);  // Set encryption key
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLen, iv, plainText, encryptedData);
    mbedtls_aes_free(&aes);

    // Convert encrypted data to Base64
    int base64Len = Base64.encodedLength(paddedLen);
    char base64Output[base64Len + 1];
    Base64.encode(base64Output, (char*)encryptedData, paddedLen);
    base64Output[base64Len] = '\0';

    // Send Base64 data via HTTP
    server.send(200, "text/plain", base64Output);
}


void update_sensor_data(void *parameter) {
  while (1) {
    // Read BMP280 sensor
    pressure_bmp = bmp.readPressure();
    temp_bmp = bmp.readTemperature();

    // Read MPU6050 sensor
    sensors_event_t a, g, temp_mpu_event;
    mpu.getEvent(&a, &g, &temp_mpu_event);
    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
    gx = g.gyro.x;
    gy = g.gyro.y;
    gz = g.gyro.z;
    temp_mpu = temp_mpu_event.temperature;

    // Read SHT4x sensor
    sensors_event_t humidity, temp_sht4_event;
    sht4.getEvent(&humidity, &temp_sht4_event);
    temp_sht4 = temp_sht4_event.temperature;
    humid_sht4 = humidity.relative_humidity;

    // Delay for 10 seconds
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

// Setup Function

void setup() {
  Serial.begin(115200);
  Wire.begin(41, 40);
  
  // connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // initialize SPIFFS
  // if (!SPIFFS.begin(true)) {
  //   Serial.println("An error occurred while mounting SPIFFS");
  //   return;
  // }

  // define routes
  // server.on("/", handleRoot);
  server.on("/sensor", []() {
    format_data();
  });
  
  // Diffie-Hellman
  generate_dh_keys();
  server.on("/dh-key-exchange", handle_dh_key_exchange);

  // start the server
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());

  // initialize bmp280
  Serial.println("initializing BMP280");
  unsigned status;
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) {
      // Serial.println("waiting for BMP280");
      delay(10);
    }
  }
  Serial.println("BMP280 Found!");

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  /*
  // initialize hts221
  if (!hts.begin_I2C(0x5f)) {
    Serial.println("Failed to find HTS221 chip");
    while (1) {
      // Serial.println("waiting for HTS221");
      delay(10); 
    }
  }
  Serial.println("HTS221 Found!");
  */

  // initializa mpu6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      // Serial.println("waiting for MPU6050");
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  // initialize sht4x
  if (! sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) {
      // Serial.println("waiting for SHT4x");
      delay(10);
    }
  }
  Serial.println("SHT4x Found!");

  xTaskCreate(update_sensor_data, "Update Sensor Data", 10000, NULL, 2, NULL);
  xTaskCreate(print_local_time, "Display Time", 15000, NULL, 1, NULL);
  xTaskCreate(update_with_NTP, "Update Time", 15000, NULL, 0, NULL);


}

// Loop function

void loop() {
  Serial.println("Handling client...");
  server.handleClient();
  Serial.printf("Public Key: %d\n", public_key);
  Serial.printf("Shared Secret: %d\n", shared_secret);
  delay(1000); // Add a delay to avoid flooding the Serial Monitor
}

