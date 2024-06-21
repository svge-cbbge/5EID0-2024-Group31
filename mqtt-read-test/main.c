#include <json-c/json.h>
#include <json-c/json_object.h>
#include <libpynq.h>
#include <platform.h>
#include <stdint.h>
#include <stepper.h>
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

#define MOVE_CONST 63.26656122907641
#define ROTATE_CONST 6.901311249137336

void stepper_wait() {
  while(!stepper_steps_done()) {
    sleep_msec(100);
  }
  sleep_msec(500);
}

void move(int d) {
  int steps = (int)(d*MOVE_CONST);
  stepper_steps(steps, steps);
  // int16_t left, right;
  // while(!stepper_steps_done()) {
  //   stepper_get_steps(&left, &right);
  //   int speed = 16000 + (int)(50000*(steps - abs(right))/steps);
  //   if (speed > 65000) {
  //     speed = 65000;
  //   }
  //   printf("speed: %d\n", speed);
  //   stepper_set_speed(speed, speed);
  // }
}

void rotate(int theta) {
  float left = -ROTATE_CONST * theta;
  float right = ROTATE_CONST * theta;
  stepper_steps((int) left, (int) right);
}

int main(void) {
  pynq_init();
  switchbox_set_pin(IO_AR0, SWB_UART0_RX);
  switchbox_set_pin(IO_AR1, SWB_UART0_TX);

  // Initialize the stepper driver.
  stepper_init();
  // Apply power to the stepper motors.
  stepper_enable();
  // Move one full rotation.
  stepper_set_speed(16000,16000);

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
          move(10);
      } else if (strcmp(buffer, "spin") == 0) {
          stepper_steps(2439, -2439);
      } else if (strcmp(buffer, "left") == 0) {
          stepper_steps(663, -663);
      } else if (strcmp(buffer, "right") == 0) {
          stepper_steps(-663, 663);
      } else if (strcmp(buffer, "back") == 0) {
          stepper_steps(-1325,1325);
      } else if (strcmp(buffer, "start") == 0) {
        for (int x = 0; x < 4; x++) {
          move(20);
          stepper_wait();
          rotate(90);
          stepper_wait();
        }
      } else if (strcmp(buffer, "rotate") == 0) {
        rotate(90);
      }
      // sscanf(buffer, "%d %d", &left, &right);

      // stepper_steps(left, right);
      // sleep_msec(100);

    }
  }
}