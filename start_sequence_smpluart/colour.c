#include "colour.h"

static float c;

void colour_init(void) {
    pynq_init();
    adc_init();
    c = 0;
}

const char* get_colour(void) {
    c = adc_read_channel(ADC1);
    for(int i = 0; i < 100000; i++) {
        c += adc_read_channel(ADC1);
    }
    c = c / 100000;

    if (c <= 0.008) {
        color_led_onoff (20, 20, 20);
        return "Unknown";
    } else if (c < 0.4) {
        color_led_onoff (0, 0, 0);
        return "Black";
    } else if (c < 0.85) {
        color_led_onoff (0, 255, 0);
        return "Green";
    } else if (c < 1.65) {
        color_led_onoff (0, 0, 255);
        return "Blue";
    } else if (c < 2.45) {
        color_led_onoff (255, 0, 0);
        return "Red";
    } else if (c < 3.1) {
        color_led_onoff (255, 255, 255);
        return "White";
    } else {
        color_led_onoff (20, 20, 20);
        return "Unknown";
    }
}

void colour_destroy(void) {
    pynq_destroy();
}