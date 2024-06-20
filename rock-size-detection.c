#include <libpynq.h>
#include <iic.h>
#include <vl53l0x.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>


int vl53l0x_init (void) {
    /*
    Initialises the vl53l0x distance sensor
    Connect the sensor to 3.3v and GND before running code
    */
    int i = tofInit(IIC0, 0x29, 0); // set long range mode (up to 2m)
    if (i != 1) {
        printf("FAILED TO CONNECT");
        return 0; // problem - quit
    }

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

        printf("tot:%dmm, diff:%dmm\n", distance, diff);
        
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
        return 1;
    } else if (count_small > count_big) {
        return 0;
    } else {
        return -1;
    }
}

int main (void) {
    pynq_init();

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

    int rock;

    while (1) {
        rock = measure_rock();
        if (rock == 1) {
            printf("LARGE ROCK\n");
        } else if (rock == 0) {
            printf("SMALL ROCK\n");
        } else if (rock == -1) {
            printf("NO ROCK\n");
        }
        sleep_msec(1000);
    }

    iic_destroy(IIC0);
    pynq_destroy();
    return 1;
}







