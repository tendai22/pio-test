#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"

#include "blink.pio.h"

//
// Pin Definitions
// This section should be located
// before #include "blink.pio.h"
//
#define DEBUG_Pin 31
#define WAIT_Pin 30
#define RFSH_Pin 28
#define M1_Pin   27
#define RD_Pin   26
#define MREQ_Pin 25
#define IORQ_Pin 24
#define RESET_Pin 42
#define BUSAK_Pin 29
#define BUSRQ_Pin 43
#define INT_Pin  41
#define CLK_Pin  40
#define TEST_Pin 45

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d kHz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000 / (2 * freq)) - 3;
}

void out8bit_forever(PIO pio, uint sm, uint offset) {
    out8bit_program_init(pio, sm, offset);
    pio_sm_set_enabled(pio, sm, true);

    printf("out8bit pin start\n");
}

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart0
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 46
#define UART_RX_PIN 47

static int toggle_value = 1;
#define TOGGLE() do {    gpio_xor_mask64(((uint64_t)1)<<TEST_Pin); } while(0)


void gpio_out_init(uint gpio, bool value) {
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, value);
    gpio_set_function(gpio, GPIO_FUNC_SIO);
}

int main()
{
    stdio_init_all();

    gpio_out_init(TEST_Pin, false);
    gpio_out_init(RESET_Pin, false);

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // PIO Blinking example
    PIO pio = pio0;
    pio_set_gpio_base(pio, 16);
    uint offset = pio_add_program(pio, &blink_program);
    printf("Loaded program at %d\n", offset);
    
    blink_pin_forever(pio, 0, offset, CLK_Pin, 3);

    offset = pio_add_program(pio, &out8bit_program);
    printf("Loaded program at %d\n", offset);
    out8bit_forever(pio, 2, offset);

    // For more pio examples see https://github.com/raspberrypi/pico-examples/tree/master/pio

    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    gpio_put(RESET_Pin, true);

    uint32_t count = 0;
    while (true) {
        TOGGLE();
        pio_sm_put(pio, 2, count);
        count += 1;
        sleep_us(10);
        TOGGLE();
        sleep_ms(2);
    }
}
