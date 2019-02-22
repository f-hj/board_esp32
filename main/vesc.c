#include "vesc.h"

#define VESC_TXD  (GPIO_NUM_4)
#define VESC_RXD  (GPIO_NUM_5)
#define VESC_RTS  (UART_PIN_NO_CHANGE)
#define VESC_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

void init_uart() {
      /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, VESC_TXD, VESC_RXD, VESC_RTS, VESC_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void start (void *pvParameters) {
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART
        printf("uart: %s\n", (const char *) data);
    }
}