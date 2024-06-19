import tkinter as tk
import json
import threading
import paho.mqtt.client as mqtt

# MQTT Broker settings
broker_address = "mqtt.ics.ele.tue.nl"
topic_subscribe = "/pynqbridge/69/send"
topic_publish = "/pynqbridge/69/recv"
username_robot = "Student133"
password_robot = "zaiy7ieX" 

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
    action_button = tk.Button(frame, text="Send MQTT Message", command=lambda: send_message(client, topic_publish, "Start"))
    action_button.pack()

# MQTT Callback functions
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(topic_subscribe, qos=1)
    else:
        print(f"Failed to connect, return code {rc}\n")

def on_message(client, userdata, message):
    raw_data = str(message.payload.decode("utf-8"))
    try:
        data = json.loads(raw_data)
        app.after(0, place_colors, data)  # Safe call to update GUI
        app.after(0, add_message, f"Received: x={data['x']}, y={data['y']}, color={data['color']}")  # Log the message
    except Exception as e:
        print(e)
    print(raw_data)

def send_message(client, topic, message):

    add_message("Start")
    client.publish(topic, message, qos=1)


# Setup MQTT client
client = mqtt.Client("base_1", clean_session=True)
client.username_pw_set(username_robot, password_robot)
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT broker
try:
    client.connect(broker_address, port=1883, keepalive=60)
except Exception as e:
    print(f"Failed to connect to MQTT broker at {broker_address}: {e}")
    #sys.exit(1)

client.loop_start()  # Start the MQTT background thread

draw_grid()
add_side_panel()

app.mainloop()  # Start the GUI event loop

client.loop_stop()  # Stop MQTT loop
client.disconnect()  # Disconnect from the broker
