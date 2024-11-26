#include <Arduino.h>

// Example small prime modulus P and generator G
const uint32_t P = 23;  // Small prime number
const uint32_t G = 5;   // Generator

// Example private key (random value between 1 and P-1)
const uint32_t private_key = 6;

// Simulate public key received from another device
const uint32_t received_public_key = 8;  // Example public key from the other party

void test_dh_key_exchange() {
    // Calculate public key: (G^private_key) % P
    uint32_t public_key = 1;
    for (uint32_t i = 0; i < private_key; i++) {
        public_key = (public_key * G) % P;
    }
    Serial.printf("Public Key: %d\n", public_key);

    // Calculate shared secret: (received_public_key^private_key) % P
    uint32_t shared_secret = 1;
    for (uint32_t i = 0; i < private_key; i++) {
        shared_secret = (shared_secret * received_public_key) % P;
    }
    Serial.printf("Shared Secret: %d\n", shared_secret);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Starting Simplified DH Test...");
    test_dh_key_exchange();
}

void loop() {}
