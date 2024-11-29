import requests
import json
import random
from flask import Flask, jsonify, render_template
from Crypto.Cipher import AES
from base64 import b64decode

app = Flask(__name__)

ESP32_IP = "ESP32_IP_ADDRESS"

# DH Parameters (same as ESP32)
P = 23  # Prime modulus
G = 5   # Generator
private_key = random.randint(1, P - 1)  # Generate a random private key
public_key = pow(G, private_key, P)     # Calculate the Raspberry Pi's public key
shared_secret = None                    # Placeholder for the shared secret

# Serve the webpage
@app.route("/")
def serve_page():
    return render_template("index.html")

# DH Key Exchange Route
@app.route("/dh-key-exchange")
def dh_key_exchange():
    global shared_secret

    try:
        # Send Raspberry Pi's public key to ESP32
        response = requests.get(f"http://{ESP32_IP}/dh-key-exchange", params={"public_key": public_key})
        response.raise_for_status()
        esp32_public_key = int(response.text.strip())

        # Compute the shared secret
        shared_secret = pow(esp32_public_key, private_key, P)
        print(f"Shared Secret: {shared_secret}")

        return jsonify({"message": "DH Key Exchange Successful", "shared_secret": shared_secret})

    except requests.exceptions.RequestException as req_err:
        print(f"HTTP request error: {req_err}")
        return jsonify({"error": "Failed to perform DH key exchange"})

    except Exception as e:
        print(f"Unexpected error: {e}")
        return jsonify({"error": "An unexpected error occurred"})

# Proxy sensor data from ESP32
@app.route("/sensor")
def get_sensor_data():
    try:
        # Fetch encrypted data from ESP32
        response = requests.get(f"http://{ESP32_IP}/sensor")
        response.raise_for_status()
        encrypted_base64 = response.text.strip()  # Remove extra spaces or newline
        print("Encrypted Base64:", encrypted_base64)

        # Decode Base64 and decrypt using the derived shared secret
        if shared_secret is None:
            raise ValueError("Shared secret not established")

        # Derive AES key from shared secret
        key = bytearray(16)
        for i in range(16):
            key[i] = (shared_secret >> (8 * (i % 4))) & 0xFF
        print("Derived AES Key:", key)

        # Static IV (must match ESP32)
        iv = b'1234567890abcdef'

        # Decrypt the data
        encrypted_data = b64decode(encrypted_base64)
        cipher = AES.new(bytes(key), AES.MODE_CBC, iv)
        decrypted_data = cipher.decrypt(encrypted_data)

        # Remove PKCS#7 padding
        padding_value = decrypted_data[-1]  # Get the last byte as padding value
        if padding_value < 1 or padding_value > 16:  # Validate padding range
            raise ValueError("Invalid padding detected")
        decrypted_data = decrypted_data[:-padding_value]  # Remove padding

        # Decode the cleaned data to text
        decrypted_text = decrypted_data.decode('utf-8')
        print("Decrypted Text:", decrypted_text)

        # Parse JSON
        json_data = json.loads(decrypted_text)
        print("JSON Data:", json_data)
        return jsonify(json_data)

    except requests.exceptions.RequestException as req_err:
        print(f"HTTP request error: {req_err}")
        return jsonify({"error": "Failed to fetch data from ESP32"})

    except ValueError as val_err:
        print(f"Decryption or JSON parsing error: {val_err}")
        return jsonify({"error": "Failed to decrypt or parse JSON data"})

    except Exception as e:
        print(f"Unexpected error: {e}")
        return jsonify({"error": "An unexpected error occurred"})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
