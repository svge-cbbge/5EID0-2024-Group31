#include <libpynq.h>
#include <iic.h>
#include <vl53l0x.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stepper.h>
#include <math.h>
#include "simple_uart.h"

#define DEVICE "/dev/ttyUSB0"
#define BAUDRATE 115200
#define SETTINGS "8N1"
#define BUFFER_SIZE 32

#define SPEED_OF_SOUND_CM_PER_SEC 34300
#define CONVERSION_FACTOR 2
#define TIME_CONVERSION_TO_MICROSECONDS 1000000
#define TIMEOUT 10000

struct simple_uart *uart;
const float ERROR_MARGIN = 1.0;  // Adjust as needed

#define MOVE_CONST    63.26656122907641
#define ROTATE_CONST  6.901311249137336
const float FULL_ROTATION = 360.0;
float angle;


float get_angle (void) {
    /*
    gets the anngle using the gyroscopes
    */
    printf("starting get_angle()\n");
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


void stepper_wait() {
    /*
    waits fir robot to finish moving
    */
    printf("starting stepper_wait()\n");
    while(!stepper_steps_done()) {
    sleep_msec(10);
    }
}


void move(int d) {
    /*
    moves approximately d centimeters
    */
    printf("starting move()\n");
	int steps = (int)(d*MOVE_CONST);
	stepper_steps(steps, steps);
}


void readjust_angle() {
    /*
    adjusts the robot to be within the error margins of the angle
    */
    printf("starting readjust_angle()\n");
    // minor corrections if the angle is not perfect
    float current = get_angle();
    while (!(angle - ERROR_MARGIN < current || current < angle + ERROR_MARGIN)) {
        if (angle - ERROR_MARGIN < current) {
            stepper_steps(1, -1);
        } else {
            stepper_steps(-1, 1);
        }
        stepper_wait();
        current = get_angle();
    }
}


#define ROTATE_CONST  6.901311249137336

void rotate(int theta) {
  float angle = get_angle() + theta;
  // less leniency when rotating first. Try to get it as close as possible
  if (theta < 0) {
    // rotate to the correct angle
    while (angle < get_angle()) {
      stepper_steps(3, -3);
      stepper_wait();
      //printf("angle: %f, theta: %d, curr: %f\n", angle, theta, get_angle());
    } 
  } else {
    // rotate to the correct angle
    while (angle > get_angle()) {
      stepper_steps(-3, 3);
      stepper_wait();
      //printf("angle: %f, theta: %d, curr: %f\n", angle, theta, get_angle());
    }
  }
  // recalibrate to make sure that the robot angle is within the error margin
  readjust_angle();
}


int vl53l0x_init (void) {
    /*
    Initialises the vl53l0x distance sensor
    Connect the sensor to 3.3v and GND before running code
    */
    printf("starting vl53l0x_init()\n");
    int i = tofInit(IIC0, 0x29, 0); // set long range mode (up to 2m)
    if (i != 1) {
        printf("FAILED TO CONNECT");
        return 0; // problem - quit
    }

    uint8_t model, revision;

    printf("VL53L0X device successfully opened.\n");
    tofGetModel(&model, &revision);
    printf("Model ID - %d\n", model);
    printf("Revision ID - %d\n", revision);
    fflush(NULL); //Get some output even is distance readings hang

    return 1;
}


int count_occurrences(int arr[], int size, int element) {
    /*
    Counts the number of times a number occurs in array
    */
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (arr[i] == element) {
            count++;
        }
    }
    return count;
}


const uint32_t SENSOR_HEIGHT = 77; // height of sensor off the ground
const uint32_t BIG = 50; // size of big rock
const uint32_t SMALL = 30; // size of small rock
int measure_rock (void) {
    /*
    Measures the size of the rock with distance sensor

    OUTPUT:
    1 -> 5x5x5cm rock
    0 -> 3x3x3cm rock
    -1 -> no rock detected
    */
    printf("starting measure_rock()\n");
    uint32_t distance;

    uint32_t diff; // height difference

    int readings = 10;
    int result[readings];

    for (int i = 0; i < readings; i++) {
        result[i] = -1;
    }

    for (int i = 0; i < readings; i++) {
        distance = (tofReadDistance());
        diff = SENSOR_HEIGHT - distance;

        //printf("tot:%dmm, diff:%dmm\n", distance, diff);
        
        // 5x5x5
        if (diff <= (BIG + 15) && diff >= (BIG - 15)) {
            // printf("Detected cube 5x5x5 [cm]\n");
            result[i] = 1;
        // 3x3x3
        } else if (diff <= (SMALL + 10) && diff >= (SMALL - 10)) {
            // printf("Detected cube 3x3x3 [cm]\n");
            result[i] = 0;
        }
        sleep_msec(10);
    }

    int count_big = count_occurrences(result, readings, 1);
    int count_small = count_occurrences(result, readings, 0);

    if (count_big > count_small) {
        printf("Big\n");
        return 1;
    } else if (count_small > count_big) {
        printf("small\n");
        return 0;
    } else {
        return -1;
    }
}


double measure_distance (int trigger_pin, int echo_pin) {
    gpio_set_level(trigger_pin, GPIO_LEVEL_HIGH);
    sleep_msec(1);
    gpio_set_level(trigger_pin, GPIO_LEVEL_LOW);

    int timeout = TIMEOUT;
    while(gpio_get_level(echo_pin)) {
        if (--timeout == 0) {
            return -1;
        }
    }

    while(!gpio_get_level(echo_pin)) {
        if (--timeout == 0) {
            return -1;
        }
    }
    long int start_clock = clock();
    while(gpio_get_level(echo_pin)) {
        if (--timeout == 0) {
            return -1;
        }
    }
    long int end_clock = clock();

    long int clock_diff = end_clock - start_clock;
    double distance = ((double)clock_diff / CLOCKS_PER_SEC) * SPEED_OF_SOUND_CM_PER_SEC / CONVERSION_FACTOR;

    return distance;
}


void ultrasound_dist (double *result) {
    double distance1 = measure_distance(IO_AR5, IO_AR6);
    printf("First sensor: %f cm\n", distance1);

    double distance2 = measure_distance(IO_AR7, IO_AR8);
    printf("Second sensor: %f cm\n", distance2);

    sleep_msec(100);

    result[0] = distance1;
    result[1] = distance2;
}


double top_ultrasound (void) {
    /*
    Returns double of distance measured from top ultrasound sensor
    */
    double distance = measure_distance(IO_AR9, IO_AR10);
    printf("Top sensor: %f cm\n", distance);

    return distance;
}


void obj_in_range (double distances[], int in_range[]) {
    /*
    Checks if distance measured by front two US sensors is in a useful range
    Returns in_range[] where 
    in_range[0] = 1 or 0 for true/false
    in_range[1] = 1 or 0 for true/false
    */
    printf("starting obj_in_range()\n");
    int max_range = 20;
    int min_range = 0;

    // checks left sensor
    if (distances[0] >= min_range && distances[0] <= max_range) {
        in_range[0] = 1;
    } else {
        in_range[0] = 0;
    }

    // checks right sensor
    if (distances[1] >= min_range && distances[1] <= max_range) {
        in_range[1] = 1;
    } else {
        in_range[1] = 0;
    }
}



const int ROT_ANGLE = 20; // degrees
const int MV_DIST = 3; // cm
int obj_left (void) {
    /*
    Steering to go towards object that is in the left
    returs either size of rock (1, 0, -1)
    or retursn mountain (2)
    */
    printf("starting obj_left() \n");
    int rock_size = measure_rock();
    printf("L rock_size: %d\n", rock_size);
    int mountain_dist = (int) top_ultrasound();
    int UStoDS = 10; // cm
    if (rock_size == -1 && (mountain_dist > UStoDS || mountain_dist == -1)) {
        rotate(-ROT_ANGLE);
        stepper_wait();
        rock_size = measure_rock();
        move(MV_DIST);
        stepper_wait();
        rock_size = measure_rock();
        mountain_dist = top_ultrasound();
    }

    // mountain detected
    if (mountain_dist >= 0 && mountain_dist <= UStoDS) {
        return 2;
    } else {
        return measure_rock();
    }
}


int obj_right (void) {
    /*
    Steering to go towards object that is in the right
    returs either size of rock (1, 0, -1)
    or retursn mountain (2)
    */
    printf("starting obj_right()\n");
    int rock_size = measure_rock();
    printf("R rock_size: %d\n", rock_size);
    int mountain_dist = top_ultrasound();
    int UStoDS = 10; // cm
    if (rock_size == -1 && (mountain_dist > UStoDS || mountain_dist == -1)) {
        rotate(ROT_ANGLE);
        stepper_wait();
        rock_size = measure_rock();
        move(MV_DIST);
        stepper_wait();
        rock_size = measure_rock();
        mountain_dist = top_ultrasound();
    }

    // mountain detected
    if (mountain_dist >= 0 && mountain_dist <= UStoDS) {
        return 2;
    } else {
        return measure_rock();
    }
}


void backout (void) {
    move(-10);
    stepper_wait();
    rotate(90);
    stepper_wait();
    move(20);
    stepper_wait();
    rotate(-90);
    stepper_wait();
}


int main (void) {
    pynq_init();
    
    // uart init for nano
    uart = simple_uart_open(DEVICE, BAUDRATE, SETTINGS);

    // stepper init
    stepper_init();
    stepper_enable();
    stepper_set_speed(32000, 32000);

    // ultrasounds pin init
    gpio_init();
    // left sensor
    gpio_set_direction(IO_AR5, GPIO_DIR_OUTPUT); // Trig_RX_SCI_I/0 (AR5)
    gpio_set_direction(IO_AR6, GPIO_DIR_INPUT);  // Echo_TX_SDA (AR6)
    // right sensor
    gpio_set_direction(IO_AR7, GPIO_DIR_OUTPUT); // Trig_RX_SCI_I/1 (AR7)
    gpio_set_direction(IO_AR8, GPIO_DIR_INPUT);  // Echo_TX_SDA (AR8)
    // top sensor
    gpio_set_direction(IO_AR9, GPIO_DIR_OUTPUT); // Trig_RX_SCI_I/1 (AR9)
    gpio_set_direction(IO_AR10, GPIO_DIR_INPUT);  // Echo_TX_SDA (AR10)

    // vl53l0x pin init
    switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
    switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
    iic_init(IIC0);

    // initialize the distance sensor
    int vl53l0x = vl53l0x_init();
    if (vl53l0x == 0) {
        iic_destroy(IIC0);
        pynq_destroy();
        return 1;
    }

    // MAIN COLLISION DETECTION //
    double distances[2]; // [left, right]
    int in_range[2]; // [left, right] 1/0 (1 = in range 0 = isnt in range)
    int object = -1; // value for type of object that it sees (1 = big rock, 0 = small rock, 2 = mountain, -1 = no rock)
    int top_us;

    int loop = 1;
    while (loop == 1) {
        // measure ultrasound distances
        ultrasound_dist(distances);
        // check if a sensor detects an object in the specified range
        obj_in_range(distances, in_range);
        if (in_range[0] == 1) {
            object = obj_left();
        }
        if (in_range[1] == 1) {
            object = obj_right();
        }

        top_us = measure_rock();
        if (top_us == 1 || top_us == 0) {
            move(6);
            stepper_wait();
            loop = 0;
        } else if (top_us == 2) {
            loop = 0;
        }
    }
    printf("%d, %d\n", top_us, object);
    sleep_msec(5000);
    backout();
    iic_destroy(IIC0);
    stepper_destroy();
    pynq_destroy();
    return EXIT_SUCCESS;
}
