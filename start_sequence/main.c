#include "main.h"

const float ERROR_MARGIN = 1.0;  // Adjust as needed
const float FULL_ROTATION = 360.0;
struct simple_uart *uart;

#define DEVICE "/dev/ttyUSB0"
#define BAUDRATE 115200
#define SETTINGS "8N1"
#define BUFFER_SIZE 32

json_object *generate_json_object(int x, int y, char *timestamp, int size){
   // {}
   json_object *child = json_object_new_object();
   //  {"robot": ... }
   json_object_object_add(child, "x", json_object_new_int(x));
   // {"loglevel":"...", "msg":"..."}
   json_object_object_add(child, "y", json_object_new_int(y));
   // {"loglevel":"...", "msg":"...", "timestamp":"..."}
   json_object_object_add(child, "color", json_object_new_string(timestamp));
   json_object_object_add(child, "Size", json_object_new_int(size));
   return child;
}

enum direction{
  north,
  east,
  west,
  south
};

int x_coor = 0;
int y_coor = 0;
float angle;
enum direction dir = north;

float get_angle(struct simple_uart *uart) {
    const char *command = "a\n";
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    ssize_t bytes_written;
    ssize_t bytes_read;
    int buffer_index = 0;

    bytes_written = simple_uart_write(uart, command, strlen(command));
    if (bytes_written < 0) {
        fprintf(stderr, "Failed to send start command\n");
        simple_uart_close(uart);
        exit(EXIT_FAILURE);
    }

    while (1) {
        bytes_read = simple_uart_read(uart, temp_buffer, 1); // Read one byte at a time
        if (bytes_read > 0) {
            if (temp_buffer[0] == '\n') { // Check for new line
                buffer[buffer_index] = '\0'; // Null-terminate the string
                fflush(stdout); // Ensure the output is printed immediately
                float theta;
                sscanf(buffer, "%f", &theta);
                return theta;
            } else {
                if (buffer_index < BUFFER_SIZE - 1) { // Ensure we do not overflow the buffer
                    buffer[buffer_index++] = temp_buffer[0];
                } else {
                    fprintf(stderr, "Buffer overflow detected!\n");
                    buffer_index = 0; // Reset the buffer index to prevent overflow
                }
            }
        }
    }
}

char* get_color(struct simple_uart *uart) {
    const char *command = "c\n";
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    ssize_t bytes_written;
    ssize_t bytes_read;
    int buffer_index = 0;

    bytes_written = simple_uart_write(uart, command, strlen(command));
    if (bytes_written < 0) {
        fprintf(stderr, "Failed to send start command\n");
        simple_uart_close(uart);
        exit(EXIT_FAILURE);
    }

    while (1) {
        bytes_read = simple_uart_read(uart, temp_buffer, 1); // Read one byte at a time
        if (bytes_read > 0) {
            if (temp_buffer[0] == '\n') { // Check for new line
                buffer[buffer_index] = '\0'; // Null-terminate the string
                fflush(stdout); // Ensure the output is printed immediately

                // Allocate memory for the color code
                char *color = malloc(8 * sizeof(char)); // 7 characters + null terminator
                if (color == NULL) {
                    fprintf(stderr, "Memory allocation failed\n");
                    exit(EXIT_FAILURE);
                }

                sscanf(buffer, "%7s", color);
                color[7] = '\0'; // Ensure the string is null-terminated
                return color;
            } else {
                if (buffer_index < BUFFER_SIZE - 1) { // Ensure we do not overflow the buffer
                    buffer[buffer_index++] = temp_buffer[0];
                } else {
                    fprintf(stderr, "Buffer overflow detected!\n");
                    buffer_index = 0; // Reset the buffer index to prevent overflow
                }
            }
        }
    }
}


bool stepper_irq(int irq) {
  while(!stepper_steps_done()) {
    if (gpio_get_level(irq)) {
      stepper_reset();
      sleep_msec(100);
      stepper_enable();
      return false;
    }
    sleep_msec(10);
  }
  return true;
}

void stepper_wait() {
  while(!stepper_steps_done()) {
    sleep_msec(10);
  }
}

void send_data(int x, int y, char* color, int size) {
  char timestamp[10];

  sprintf(timestamp, "%d", x);  // Create a simple timestamp as a string
  json_object *packet = generate_json_object(x, y, color, size);

  const char *json_str = json_object_to_json_string(packet);
  printf("JSON String: %s\n", json_str);

  int n = strlen(json_str);
  unsigned char size_of_byte[4];

  size_of_byte[3] = (n >> 24) & 0xFF;
  size_of_byte[2] = (n >> 16) & 0xFF;
  size_of_byte[1] = (n >> 8) & 0xFF;
  size_of_byte[0] = n & 0xFF;
  for (int b = 0; b < 4; b++) {
    int send = 1;
    while (send) {
      uart_send (UART0, size_of_byte[b]);
      send = gpio_get_level(IO_AR2);
    }
  }

  for (int c = 0; c < n; c++) {
    int send = 1;
    while (send) {
      uart_send (UART0, json_str[c]);
      send = gpio_get_level(IO_AR2);
    }
  }
  sleep_msec(5000);
}

// moves approximately d centimeters
void move(int d) {
  int steps = (int)(d*MOVE_CONST);
  stepper_steps(steps, steps);
}

// adjusts the robot to be within the error margins of the angle
void readjust_angle() {
  // stepper_set_speed(5000,5000);
  // minor corrections if the angle is not perfect
  float current = get_angle(uart);
  while (!(angle - ERROR_MARGIN < current || current < angle + ERROR_MARGIN)) {
    if (angle - ERROR_MARGIN < current) {
      stepper_steps(1, -1);
    } else {
      stepper_steps(-1, 1);
    }
    stepper_wait();
    current = get_angle(uart);
  }
  // stepper_set_speed(16000,16000);

}

void rotate(int theta) {
  // // estimate first
  // float left = -ROTATE_CONST * theta * 7.5;
  // float right = ROTATE_CONST * theta * 7.5;
  // stepper_steps((int) left, (int) right);
  // stepper_wait();

  // stepper_set_speed(5000,5000);
  float angle = get_angle(uart) + theta;
  // less leniency when rotating first. Try to get it as close as possible
  if (theta < 0) {
    // rotate to the correct angle
    while (angle < get_angle(uart)) {
      stepper_steps(3, -3);
      stepper_wait();
    } 
  } else {
    // rotate to the correct angle
    while (angle > get_angle(uart)) {
      stepper_steps(-3, 3);
      stepper_wait();
    }
  }
  // recalibrate to make sure that the robot angle is within the error margin
  readjust_angle();
}


void move_forward(void) {
  float current_theta = get_angle(uart);
  while(!gpio_get_level(FRONT_IR)) {
    move(10);
    current_theta = get_angle(uart);
    stepper_irq(FRONT_IR);
    if (current_theta < (angle - ERROR_MARGIN) || current_theta > (angle - ERROR_MARGIN)) {
      readjust_angle();
    }
  }
}

void startup() {
  // go straight
  move_forward();
  sleep_msec(500);

  // adjust to be perpendicular
  rotate(12);
  stepper_wait();
  move(3);
  stepper_irq(FRONT_IR);
  int corner = 0;

  while (1) {
    // left IR is aligned with the black tape
    while(!gpio_get_level(FRONT_IR) && gpio_get_level(LEFT_IR)) {
      move(1);
      stepper_irq(FRONT_IR);
      readjust_angle();
    }

    if (!gpio_get_level(LEFT_IR)) {
      move(5);
      stepper_irq(FRONT_IR);
      rotate(-12);
      stepper_wait();
      while(!gpio_get_level(FRONT_IR)) {
          move(1);
          stepper_irq(FRONT_IR);
      }
      rotate(12);
      stepper_wait();
    }
    if (gpio_get_level(FRONT_IR)) {
      rotate(12);
      stepper_wait();
      move(3);
      stepper_irq(FRONT_IR);
    }

    while(!gpio_get_level(FRONT_IR)) {
      move(1);
      stepper_irq(FRONT_IR);
      readjust_angle();
    }
    //check if it is a corner
    if (gpio_get_level(FRONT_IR) && gpio_get_level(LEFT_IR)) {
      corner++;
    }
    if (corner == 4) {
      return;
    }
  }
}

void snake_algo(void) {
  while(1) {
    move_forward();
    rotate(24);
    stepper_wait();
    move_forward();
    rotate(12);
    stepper_wait();
    move(5);
    stepper_irq(FRONT_IR);
    rotate(12);
    stepper_wait();
  }
}


int main(void) {
  // Open the UART device
  uart = simple_uart_open(DEVICE, BAUDRATE, SETTINGS);
  if (uart == NULL) {
      fprintf(stderr, "Failed to open UART device %s\n", DEVICE);
      return EXIT_FAILURE;
  }
  pynq_init();
  switchbox_set_pin(IO_AR0, SWB_UART0_RX);
  switchbox_set_pin(IO_AR1, SWB_UART0_TX);
  adc_init();
  color_leds_init_pwm();

  // Initialize the stepper driver.
  stepper_init();
  // Apply power to the stepper motors.
  stepper_enable();
  // Move one full rotation.
  stepper_set_speed(24000,24000);

  uart_init(UART0);

  uart_reset_fifos(UART0);
  printf("\n");
  
  // int left, right;


  while (1) {
    if (uart_has_data(UART0)) {
      printf("\n");
      uint8_t length[4];
      for (int x = 0; x < 4; x++) {
        length[x] = uart_recv(UART0);
      }
      uint8_t s = length[0] + (length[1] << 2) + (length[2] << 4) + (length[3] << 6);
      printf("length: %d\n", s);
      char buffer[s];
      buffer[s] = '\0';

      for (int i = 0; i < s; i++) {
        buffer[i] = (char) uart_recv(UART0);
        //printf("%02X ", buffer[i]);
      }
      printf("%s\n", buffer);
      if (strcmp(buffer, "straight") == 0) {
          move_forward();
      } else if (strcmp(buffer, "spin") == 0) {
          stepper_steps(2439, -2439);
      } else if (strcmp(buffer, "left") == 0) {
          stepper_steps(663, -663);
      } else if (strcmp(buffer, "right") == 0) {
          stepper_steps(-663, 663);
      } else if (strcmp(buffer, "back") == 0) {
          stepper_steps(-1325,1325);
      } else if (strcmp(buffer, "start") == 0) {
          snake_algo();
      } else if (strcmp(buffer, "rotate") == 0) {
        rotate(12);
      } else if(strcmp(buffer, "color") == 0) {
        printf("%s \n", get_color(uart)); 
      }
      // sscanf(buffer, "%d %d", &left, &right);

      // stepper_steps(left, right);
      // sleep_msec(100);

    }
  }
}