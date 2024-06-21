import tkinter as tk
import json
import threading
import queue
import paho.mqtt.client as mqtt
import time

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

# Setup the application window
app = tk.Tk()
app.title("Grid Visualizer")

# Constants for grid size, cell size, and panel width
GRID_WIDTH = 39
GRID_HEIGHT = 39
CELL_SIZE = 20
PANEL_WIDTH = 200

# Create a canvas to draw the grid
canvas = tk.Canvas(app, width=GRID_WIDTH * CELL_SIZE, height=GRID_HEIGHT * CELL_SIZE)
canvas.pack(side="left")

# Function to draw the grid
def draw_grid():
    for i in range(GRID_WIDTH):
        for j in range(GRID_HEIGHT):
            canvas.create_rectangle(i * CELL_SIZE, j * CELL_SIZE, (i + 1) * CELL_SIZE, (j + 1) * CELL_SIZE, fill="white", outline="")

# Function to place colored squares according to JSON data
def place_colors(item):
    x = item['x']
    y = item['y']
    color = item['color']
    canvas.create_rectangle(x * CELL_SIZE, y * CELL_SIZE, (x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE, fill=color, outline="")

# Function to add messages to the text area
def add_message(message):
    text_area.config(state="normal")
    text_area.insert("end", message + "\n")
    text_area.config(state="disabled")
    text_area.see("end")

# Add side panel for messages and a button
def add_side_panel():
    # Frame for side panel
    frame = tk.Frame(app, width=PANEL_WIDTH, height=GRID_HEIGHT * CELL_SIZE, bg="#f0f0f0")
    frame.pack(side="right", fill="both", expand="yes")

    # Text widget with scrollbar for messages
    global text_area  # Make text_area global to access it outside this function
    text_area = tk.Text(frame, wrap="word", state="disabled", bg="#f0f0f0")
    scroll_bar = tk.Scrollbar(frame, command=text_area.yview)
    text_area.configure(yscrollcommand=scroll_bar.set)
    scroll_bar.pack(side="right", fill="y")
    text_area.pack(side="left", fill="both", expand=True)

    # Button to trigger adding messages
    action_button = tk.Button(frame, text="Send MQTT Message", command=lambda: send_message(client_1, topic_publish_robot_1, "Start"))
    action_button.pack()

# Add side panel
add_side_panel()

# Queue for thread-safe communication between MQTT and Tkinter
message_queue = queue.Queue()

# MQTT Callback functions
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

def on_message(client, userdata, message):
    raw_data = str(message.payload.decode("utf-8"))
    try:
        data = json.loads(raw_data)
        message_queue.put((place_colors, data))
        message_queue.put((add_message, f"Received: x={data['x']}, y={data['y']}, color={data['color']}"))
    except Exception as e:
        print(e)
    print(raw_data)

def send_message(client, topic, message):
    add_message("Start")
    client.publish(topic, message, qos=1)

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

# Function to process messages from the queue
def process_queue():
    while not message_queue.empty():
        func, arg = message_queue.get()
        func(arg)
    app.after(100, process_queue)

# Start processing the queue
app.after(100, process_queue)

# Start the Tkinter main loop
app.mainloop()

# Stop the loops and disconnect (graceful shutdown)
client_1.loop_stop()
client_1.disconnect()
client_2.loop_stop()
client_2.disconnect()
thread_1.join()
thread_2.join()
