import tkinter as tk
import json
import threading
import paho.mqtt.client as mqtt
import time

# MQTT Broker settings
broker_address = "mqtt.ics.ele.tue.nl"
topic_subscribe_robot_1 = "/pynqbridge/31/send"
topic_publish_robot_1 = "/pynqbridge/31/recv"
topic_subscribe_robot_2 = "/pynqbridge/69/send"
topic_publish_robot_2 = "/pynqbridge/69/recv"

username_robot = "Student61"
password_robot = "EeXahT5V"
username_2 = "Student133"
password_2 = "zaiy7ieX"

# Setup the application window
app = tk.Tk()
app.title("Grid Visualizer")

# Constants for grid size, cell size, and panel width
GRID_WIDTH = 98
GRID_HEIGHT = 98
CELL_SIZE = 8
PANEL_WIDTH = 200

# Create a canvas to draw the grid
canvas = tk.Canvas(app, width=GRID_WIDTH * CELL_SIZE, height=GRID_HEIGHT * CELL_SIZE)
canvas.pack(side="left")

# Determine the center of the grid
CENTER_X = GRID_WIDTH // 2
CENTER_Y = GRID_HEIGHT // 2

# Function to draw the grid and the center square
def draw_grid():
    for i in range(GRID_WIDTH):
        for j in range(GRID_HEIGHT):
            color = "white"
            if i == CENTER_X and j == CENTER_Y:
                color = "grey"
            canvas.create_rectangle(i * CELL_SIZE, j * CELL_SIZE, (i + 1) * CELL_SIZE, (j + 1) * CELL_SIZE, fill=color, outline="")

# Function to place colored squares according to JSON data
def place_colors(item):
    x = item['x']
    y = item['y']
    color = item['color']
    
    # Default size for different colors
    if color == "#5F574F":
        size = 10
    else:
        size = item.get('size', 0)  # Get size from item, default to 0 if not provided

    # Translate coordinates based on the center point
    translated_x = CENTER_X + x
    translated_y = CENTER_Y - y  # Invert y to go up

    # Adjust size for size = 1 to 2x2, size = 0 to 1x1
    if size == 1:
        size = 2
    elif size == 0:
        size = 1

    for i in range(size):
        for j in range(size):
            canvas.create_rectangle(
                (translated_x + i) * CELL_SIZE, 
                (translated_y + j) * CELL_SIZE, 
                (translated_x + i + 1) * CELL_SIZE, 
                (translated_y + j + 1) * CELL_SIZE, 
                fill=color, 
                outline=""
            )

# Function to add messages to the text area
def add_message(message):
    text_area.config(state="normal")
    text_area.insert("end", message + "\n")
    text_area.config(state="disabled")
    text_area.see("end")

# Timer related variables and functions
start_time = None
elapsed_time = 0
timer_running = False

def update_timer():
    if timer_running:
        global elapsed_time
        elapsed_time = time.time() - start_time
        minutes = int(elapsed_time // 60)
        seconds = int(elapsed_time % 60)
        timer_label.config(text=f"{minutes:02}:{seconds:02}")
        app.after(1000, update_timer)

def start_timer():
    global start_time, timer_running
    if not timer_running:
        start_time = time.time() - elapsed_time  # Resume from where it was paused
        timer_running = True
        update_timer()

def pause_timer():
    global timer_running
    if timer_running:
        timer_running = False

# Add side panel for messages and a button
def add_side_panel():
    # Frame for side panel
    frame = tk.Frame(app, width=PANEL_WIDTH, height=GRID_HEIGHT * CELL_SIZE, bg="#f0f0f0")
    frame.pack(side="right", fill="both", expand="yes")

    # Timer label
    global timer_label  # Make timer_label global to access it outside this function
    timer_label = tk.Label(frame, text="00:00", bg="#f0f0f0", font=("Helvetica", 16))
    timer_label.pack(pady=10)

    # Text widget with scrollbar for messages
    global text_area  # Make text_area global to access it outside this function
    text_area = tk.Text(frame, wrap="word", state="disabled", bg="#f0f0f0")
    scroll_bar = tk.Scrollbar(frame, command=text_area.yview)
    text_area.configure(yscrollcommand=scroll_bar.set)
    scroll_bar.pack(side="right", fill="y")
    text_area.pack(side="left", fill="both", expand=True)

    # Button to trigger adding messages
    global action_button  # Make action_button global to access it outside this function
    action_button = tk.Button(frame, text="Start Robot", command=toggle_robot)
    action_button.pack(pady=10)

# MQTT callbacks
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"{client._client_id} Connected to MQTT Broker!")
        if client._client_id == b"home_base_1":
            client.subscribe(topic_subscribe_robot_1, qos=1)
        elif client._client_id == b"home_base_2":
            client.subscribe(topic_subscribe_robot_2, qos=1)
    else:
        print(f"Failed to connect, return code {rc}\n")

def on_message(client, userdata, message):
    raw_data = str(message.payload.decode("utf-8"))
    try:
        data = json.loads(raw_data)
        app.after(0, place_colors, data)  # Safe call to update GUI
        app.after(0, add_message, f"Received: x={data['x']}, y={data['y']}, color={data['color']}, size={data['size']}")  # Log the message
    except Exception as e:
        print(e)
    print(raw_data)

def send_message(client, topic, message):
    client.publish(topic, message, qos=1)

# Function to toggle the robot state
robot_started = False

def toggle_robot():
    global robot_started
    if robot_started:
        action_button.config(text="Start Robot")
        send_message(client_1, topic_publish_robot_1, "Stop")
        add_message("stop1")
        send_message(client_2, topic_publish_robot_2, "Stop")
        add_message("stop2")
        pause_timer()
    else:
        action_button.config(text="Stop Robot")
        send_message(client_1, topic_publish_robot_1, "Start")
        add_message("start1")
        send_message(client_2, topic_publish_robot_2, "Start")
        add_message("start2")
        start_timer()
    robot_started = not robot_started

# Setup MQTT clients and callbacks
client_1 = mqtt.Client("home_base_1", clean_session=True)
client_1.on_connect = on_connect
client_1.on_message = on_message

client_2 = mqtt.Client("home_base_2", clean_session=True)
client_2.on_connect = on_connect
client_2.on_message = on_message

# Set the username and password for each client
client_1.username_pw_set(username_robot, password_robot)
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

# Start the loops for both clients
client_1.loop_start()
client_2.loop_start()

draw_grid()
add_side_panel()

app.mainloop()  # Start the GUI event loop

# Stop MQTT loop
client_1.loop_stop()
client_2.loop_stop()
client_1.disconnect()
client_2.disconnect()