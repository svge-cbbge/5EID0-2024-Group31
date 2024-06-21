#include <libpynq.h>
#include <time.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <string.h>

json_object *generate_json_object(int x, int y, char *timestamp){
   // {}
   json_object *child = json_object_new_object();
   //  {"robot": ... }
   json_object_object_add(child, "x", json_object_new_int(x));
   // {"loglevel":"...", "msg":"..."}
   json_object_object_add(child, "y", json_object_new_int(y));
   // {"loglevel":"...", "msg":"...", "timestamp":"..."}
   json_object_object_add(child, "color", json_object_new_string(timestamp));
   return child;
}

int main (void)
{
 // initialise all I/O
 pynq_init();
 gpio_init();
 switchbox_set_pin(IO_AR0, SWB_UART0_RX);
 switchbox_set_pin(IO_AR1, SWB_UART0_TX);
 
  // initialize UART 0
  uart_init(UART0);
  // flush FIFOs of UART 0
  uart_reset_fifos(UART0);

  char timestamp[10];
 
  printf("has data: %d, has space: %d\n", uart_has_data(UART0), uart_has_space(UART0));
  for (int x = 0; x < 10; x++) {
    sprintf(timestamp, "%d", x);  // Create a simple timestamp as a string
    json_object *packet = generate_json_object(x, x, "#00FFFF");
    printf("message #%d\n", x);

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
  // clean up after use
  uart_destroy(UART0);
  pynq_destroy();
  
  return EXIT_SUCCESS;
}
