import paho.mqtt.client as mqtt
from paho.mqtt.client import CallbackAPIVersion
import struct
import csv
import json
from datetime import datetime
import base64

def save_to_csv(group_number, latitude, longitude):
    with open('gps_data.csv', mode='a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow([datetime.now(), group_number, latitude, longitude])

MQTT_SERVER = "nam1.cloud.thethings.network"
MQTT_USER = "wiot-group4-lab6"
MQTT_PASSWORD = "NNSXS.D57GTGCRVBCMLMQ3WF6FLS52AFEHGMXDTK37RAQ.JNCSIQVSDWYNMEOBEQQOPOSNSHW2GP6IOOD2PNB7VRTM7WTPXGCA" 
DEVICE_ID = "eui-70b3d57ed006642d"

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    client.subscribe(f"v3/{MQTT_USER}@ttn/devices/{DEVICE_ID}/up")

def on_message(client, userdata, msg):
    print("Message received on topic: " + msg.topic)
    try:
        json_payload = json.loads(msg.payload)
        frm_payload_b64 = json_payload["uplink_message"]["frm_payload"]
        frm_payload_bytes = base64.b64decode(frm_payload_b64)
        group_number, latitude, longitude = decode_payload(frm_payload_bytes)
        print(f"Group: {group_number}, Latitude: {latitude}, Longitude: {longitude}")
        save_to_csv(group_number, latitude, longitude)
    except Exception as e:
        print(f"Error processing the message: {e}")

def decode_payload(bytes_payload):
    group_number = bytes_payload[0]
    latitude_bytes = bytes_payload[1:5]
    longitude_bytes = bytes_payload[5:9]
    latitude = struct.unpack('f', latitude_bytes)[0]
    longitude = struct.unpack('f', longitude_bytes)[0]
    return group_number, latitude, longitude

client = mqtt.Client(callback_api_version=CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set(MQTT_USER, password=MQTT_PASSWORD)
client.connect(MQTT_SERVER, 1883, 60)

client.loop_forever()
