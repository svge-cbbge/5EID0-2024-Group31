// returns arbitrary values proportional to the colour read
// very slow
// if values are not similar (~15) when pointed at white calibration falied - try again

#include <libpynq.h>
#include <time.h>

#define MEASUREMENT_TIME 0.1 // Measurement time in seconds

void read_tcs3200(float *red, float *green, float *blue) {
    // Set S0 - S3 pins to output
    gpio_set_direction(IO_AR4, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_AR5, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_AR6, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_AR7, GPIO_DIR_OUTPUT);

    // Set AR8 pin to input
    gpio_set_direction(IO_AR8, GPIO_DIR_INPUT);

    // Set S0 and S1 to 2% frequency scaling
    gpio_set_level(IO_AR4, GPIO_LEVEL_LOW);
    gpio_set_level(IO_AR5, GPIO_LEVEL_HIGH);

    // Initialize the interrupt
    gpio_interrupt_init();
    gpio_enable_interrupt(IO_AR8);

    struct timespec start, end;
    int last_state, current_state, interrupt_count;

    // Read red filtered photodiodes
    gpio_set_level(IO_AR6, GPIO_LEVEL_LOW);
    gpio_set_level(IO_AR7, GPIO_LEVEL_LOW);
    interrupt_count = 0;
    last_state = gpio_get_level(IO_AR8);
    clock_gettime(CLOCK_REALTIME, &start);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &end);
        if (end.tv_sec - start.tv_sec >= MEASUREMENT_TIME) {
            break;
        }
        current_state = gpio_get_level(IO_AR8);
        if (current_state != last_state) {
            interrupt_count++;
            last_state = current_state;
        }
    }
    *red = (float)interrupt_count / MEASUREMENT_TIME;

    // Read green filtered photodiodes
    gpio_set_level(IO_AR6, GPIO_LEVEL_HIGH);
    gpio_set_level(IO_AR7, GPIO_LEVEL_HIGH);
    interrupt_count = 0;
    last_state = gpio_get_level(IO_AR8);
    clock_gettime(CLOCK_REALTIME, &start);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &end);
        if (end.tv_sec - start.tv_sec >= MEASUREMENT_TIME) {
            break;
        }
        current_state = gpio_get_level(IO_AR8);
        if (current_state != last_state) {
            interrupt_count++;
            last_state = current_state;
        }
    }
    *green = (float)interrupt_count / MEASUREMENT_TIME;

    // Read blue filtered photodiodes
    gpio_set_level(IO_AR6, GPIO_LEVEL_LOW);
    gpio_set_level(IO_AR7, GPIO_LEVEL_HIGH);
    interrupt_count = 0;
    last_state = gpio_get_level(IO_AR8);
    clock_gettime(CLOCK_REALTIME, &start);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &end);
        if (end.tv_sec - start.tv_sec >= MEASUREMENT_TIME) {
            break;
        }
        current_state = gpio_get_level(IO_AR8);
        if (current_state != last_state) {
            interrupt_count++;
            last_state = current_state;
        }
    }
    *blue = (float)interrupt_count / MEASUREMENT_TIME;
}

int main(void) {
    pynq_init();
    gpio_init();
    float red, green, blue, red_cal, green_cal, blue_cal;
    red_cal = green_cal = blue_cal = 0;
    printf("Begin calibration. Point sensor at 100%% white.\n");
    sleep_msec(2500);
    read_tcs3200(&red, &green, &blue);
    red_cal += red; green_cal += green; blue_cal += blue;
    read_tcs3200(&red, &green, &blue);
    red_cal += red; green_cal += green; blue_cal += blue;
    read_tcs3200(&red, &green, &blue);
    red_cal += red; green_cal += green; blue_cal += blue;
    red_cal = red_cal / 3; blue_cal = blue_cal / 3; green_cal = green_cal / 3;
    printf("Calibration complete.\n");
    while (true) {
        read_tcs3200(&red, &green, &blue);
        printf("Red: %f, Green: %f, Blue: %f\n", (red/red_cal)*100, (green/green_cal)*100, (blue/blue_cal)*100);
    }
    pynq_destroy();
    return EXIT_SUCCESS;
}
