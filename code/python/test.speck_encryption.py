from speck import SpeckCipher
import secrets
import yaml
import paho.mqtt.client as mqtt


# YAML-Datei einlesen
with open('config.yaml', 'r') as f:
    config = yaml.safe_load(f)


# This is the key to be stored on both sender and receiver
print(f"Importing key from configuration")
key = config['cipher']['key']
key_bytes = bytes.fromhex(key)
key_int = int.from_bytes(key_bytes, 'big')

print(f"The installed key is [hex]: {key_bytes.hex()}")

cipher = SpeckCipher(key_int, key_size=128, block_size=128)


rand_int = secrets.randbits(128)
speck_ciphertext = cipher.encrypt(rand_int)
speck_plaintext = cipher.decrypt(speck_ciphertext)

print(f"  Raw Plaintext as int: {rand_int}")
print(f"Speck Plaintext as int: {speck_plaintext}")





try:
  while True:
      
      # Generate new random integer to be sent to the world
      rand_int = secrets.randbits(128)
      print(f"New random int: {rand_int}")

      # Encrypt the newly generated integer
      speck_ciphertext = cipher.encrypt(rand_int)
      print(f"Speck Ciphertext: {speck_ciphertext}")

except Exception as e:
   print(f"An error occured: {e}")

except KeyboardInterrupt:
   print("KeyboardInterrupt")







