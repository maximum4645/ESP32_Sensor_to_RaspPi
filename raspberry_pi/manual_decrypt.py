from Crypto.Cipher import AES
from base64 import b64decode

# Define AES key and IV
key = b'1234567890abcdef'
iv = b'1234567890abcdef'

print("hard-coded key :", key, '\n')
shared_secret = None
shared_secret = 10
#calculated_key = shared_secret.to_bytes(16, 'big')[:16]
calculated_key = str(shared_secret).zfill(16).encode('utf-8')
print("calculated key :", calculated_key, '\n')

# Replace with Base64-encoded data from ESP32
encrypted_base64 = "sGJR2KqD0AMdMc18VB3V3ZIHogwJTzr0A7d+d90gjk8VWpEtHWmEo5QFC3l+2Bgcxzg5wbF+1imDKujp6joMQslW5yhlAeQyGPHd/EQa027NaIOofUJfqZ8M/HT6HwbmV5+QGQxGXvHXJAeA86Ia7Y4pkD5cv0NaWSu/H9f4lhPy8W9c+lJs9ixUTMRV5T0/rLabV8G5+Hv42KzvjZouKZTzky409YkujuPVPedQqwnTHitdZbx2SRYBcnKKd/e/tiPG0ShZ5OTywf7FFs4LY0OF/Rd9ejdpa5KvfofE1grQtEe19GZKIeCBO/G3gO6z"

# Decode Base64 to get encrypted binary data
encrypted_data = b64decode(encrypted_base64)

# Initialize AES CBC decryption
cipher = AES.new(key, AES.MODE_CBC, iv)

# Decrypt the data
decrypted_data = cipher.decrypt(encrypted_data)
print("decrypted data :", decrypted_data, '\n')

# Print the decrypted text
try:
    decrypted_text = decrypted_data.decode('utf-8')
    print(f"Decrypted text: {decrypted_text}")

except UnicodeDecodeError:
    print(f"Decrypted binary data: {decrypted_data}")
