#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simple_uart.h"

#define UART_DEVICE "/dev/ttyS0"
#define BAUD_RATE 115200
#define UART_SETTINGS "8N1"
#define BUFFER_SIZE 256

int main(void)
{
    struct simple_uart *uart;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    uart = simple_uart_open(UART_DEVICE, BAUD_RATE, UART_SETTINGS);
    if (uart == NULL) {
        fprintf(stderr, "Error: unable to open UART device\n");
        return EXIT_FAILURE;
    }

    printf("Reading from UART device %s at %d baud...\n", UART_DEVICE, BAUD_RATE);

    while (1) {
        bytes_read = simple_uart_read(uart, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }
        usleep(1000);
    }

    simple_uart_close(uart);
    return EXIT_SUCCESS;
}
