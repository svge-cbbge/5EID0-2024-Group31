import paho.mqtt.client as mqtt
import threading
import time
import sys
import json

broker_address = "mqtt.ics.ele.tue.nl"
topic_publish_robot_1 = "/pynqbridge/31/recv"
topic_subscribe_robot_1 = "/pynqbridge/31/send"

topic_publish_robot_2 = "/pynqbridge/69/recv"
topic_subscribe_robot_2 = "/pynqbridge/69/send"

# ___Robot 1___
username = "Student61" 
password = "EeXahT5V" 
print(username)

# ___Robot 2___
username_2 = "Student133" 
password_2 = "zaiy7ieX" 
print(username_2)

# Callback when connecting to the MQTT broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"{client._client_id.decode()} Connected to MQTT Broker!")
        if client._client_id.decode() == "home_base_1":
            client.subscribe(topic_subscribe_robot_1, qos=1)
            print(f"{client._client_id.decode()} subscribed to {topic_subscribe_robot_1}")
        elif client._client_id.decode() == "home_base_2":
            client.subscribe(topic_subscribe_robot_2, qos=1)
            print(f"{client._client_id.decode()} subscribed to {topic_subscribe_robot_2}")
    else:
        print(f"Failed to connect, return code {rc}\n")

# Callback when receiving a message from the MQTT broker
def on_message(client, userdata, message):
    print(f"Received message: {str(message.payload.decode('utf-8'))} on topic {message.topic}")

# Setup MQTT clients and callbacks
client_1 = mqtt.Client("home_base_1", clean_session=True)
client_1.on_connect = on_connect
client_1.on_message = on_message

client_2 = mqtt.Client("home_base_2", clean_session=True)
client_2.on_connect = on_connect
client_2.on_message = on_message

# Set the username and password for each client
client_1.username_pw_set(username, password)
client_2.username_pw_set(username_2, password_2)

# Connect to MQTT broker
try:
    client_1.connect(broker_address, port=1883, keepalive=60)
except Exception as e:
    print(f"Failed to connect to MQTT broker at {broker_address} for client 1: {e}")
    exit(1)

try:
    client_2.connect(broker_address, port=1883, keepalive=60)
except Exception as e:
    print(f"Failed to connect to MQTT broker at {broker_address} for client 2: {e}")
    exit(1)

# Function to start the MQTT loop for a client
def start_mqtt_loop(client):
    client.loop_start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print(f"Interrupted: Stopping client {client._client_id.decode()}")
        client.loop_stop()
        client.disconnect()

# Start the loops for both clients in separate threads
thread_1 = threading.Thread(target=start_mqtt_loop, args=(client_1,))
thread_2 = threading.Thread(target=start_mqtt_loop, args=(client_2,))
thread_1.start()
thread_2.start()

def send_message(client, topic, message):
    result = client.publish(topic, message, qos=1)
    status = result.rc
    if status == 0:
        print(f"Message `{message}` sent to topic `{topic}`")
    else:
        print(f"Failed to send message to topic `{topic}`")

try:
    while True:
        message = input("Enter message: ")
        send_message(client_1, topic_publish_robot_1, message)
        send_message(client_2, topic_publish_robot_2, message)
except KeyboardInterrupt:
    print("Interrupted by user")

# Stop the loops and disconnect (graceful shutdown)
client_1.loop_stop()
client_1.disconnect()
client_2.loop_stop()
client_2.disconnect()
thread_1.join()
thread_2.join()
