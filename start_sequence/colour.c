#include "colour.h"
#include <stdio.h>

char get_colour(double c) {
    c = c/100000;
    if (c <= 0.008) {
        color_led_onoff (20, 20, 20);
        return 'U'; // Unknown
    }
    else if (c < 0.4) {
        color_led_onoff (0, 0, 0);
        return 'B'; // Black
    }
    else if (c < 0.85) {
        color_led_onoff (0, 255, 0);
        return 'G'; // Green
    }
    else if (c < 1.65) {
        color_led_onoff (0, 0, 255);
        return 'L'; // Blue
    }
    else if (c < 2.45) {
        color_led_onoff (255, 0, 0);
        return 'R'; // Red
    }
    else if (c < 3.1) {
        color_led_onoff (255, 255, 255);
        return 'W'; // White
    }
    else {
        return 'U'; // Unknown
    }
}