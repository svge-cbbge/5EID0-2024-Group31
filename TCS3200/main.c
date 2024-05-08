// returns arbitrary values proportional to the colour read
// very slow
// if values are not similar (~15) when pointed at white calibration falied - try again

#include <libpynq.h>
#include <time.h>

#define MEASUREMENT_TIME 0.1 // Measurement time in seconds

void read_tcs3200(float *red, float *green, float *blue) {
    // Set S0 - S3 pins to output
    gpio_set_direction(IO_AR0, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_AR1, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_AR2, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_AR3, GPIO_DIR_OUTPUT);

    // Set AR4 pin to input
    gpio_set_direction(IO_AR4, GPIO_DIR_INPUT);

    // Set S0 and S1 to 2% frequency scaling
    gpio_set_level(IO_AR0, GPIO_LEVEL_LOW);
    gpio_set_level(IO_AR1, GPIO_LEVEL_HIGH);

    // Initialize the interrupt
    gpio_interrupt_init();
    gpio_enable_interrupt(IO_AR4);

    struct timespec start, end;
    int last_state, current_state, interrupt_count;

    // Read red filtered photodiodes
    gpio_set_level(IO_AR2, GPIO_LEVEL_LOW);
    gpio_set_level(IO_AR3, GPIO_LEVEL_LOW);
    interrupt_count = 0;
    last_state = gpio_get_level(IO_AR4);
    clock_gettime(CLOCK_REALTIME, &start);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &end);
        if (end.tv_sec - start.tv_sec >= MEASUREMENT_TIME) {
            break;
        }
        current_state = gpio_get_level(IO_AR4);
        if (current_state != last_state) {
            interrupt_count++;
            last_state = current_state;
        }
    }
    *red = (float)interrupt_count / MEASUREMENT_TIME;

    // Read green filtered photodiodes
    gpio_set_level(IO_AR2, GPIO_LEVEL_HIGH);
    gpio_set_level(IO_AR3, GPIO_LEVEL_HIGH);
    interrupt_count = 0;
    last_state = gpio_get_level(IO_AR4);
    clock_gettime(CLOCK_REALTIME, &start);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &end);
        if (end.tv_sec - start.tv_sec >= MEASUREMENT_TIME) {
            break;
        }
        current_state = gpio_get_level(IO_AR4);
        if (current_state != last_state) {
            interrupt_count++;
            last_state = current_state;
        }
    }
    *green = (float)interrupt_count / MEASUREMENT_TIME;

    // Read blue filtered photodiodes
    gpio_set_level(IO_AR2, GPIO_LEVEL_LOW);
    gpio_set_level(IO_AR3, GPIO_LEVEL_HIGH);
    interrupt_count = 0;
    last_state = gpio_get_level(IO_AR4);
    clock_gettime(CLOCK_REALTIME, &start);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &end);
        if (end.tv_sec - start.tv_sec >= MEASUREMENT_TIME) {
            break;
        }
        current_state = gpio_get_level(IO_AR4);
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
    printf("Begin calibration. Point sensor at 100%% white.\n");
    sleep_msec(2500);
    read_tcs3200(&red_cal, &green_cal, &blue_cal);
    read_tcs3200(&red_cal, &green_cal, &blue_cal);
    printf("Calibration complete.\n");
    while (true) {
        read_tcs3200(&red, &green, &blue);
        printf("Red: %f, Green: %f, Blue: %f\n", (red/red_cal)*100, ((green+(red_cal-green_cal))/green_cal)*100, ((blue+(red_cal-blue_cal))/blue_cal)*100);
    }
    pynq_destroy();
    return EXIT_SUCCESS;
}
