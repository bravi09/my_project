#include <regx51.h>
#include <string.h>
#include "rtc.h"



void main() {
    unsigned char hr, min, sec, date, month, year;
    lcd_init();
    uart_init();
    motor = 0;

    // rtc_set(12, 45, 30); // Uncomment once to set time

    lcd_print("Scan RFID Card");

    while (1) {
        char ch = uart_rx();
        uart_tx(ch);

        if (ch == '\r' || ch == '\n') {
            input[idx] = '\0';
            lcd_cmd(0x01);

            if (is_valid(input)) {
                lcd_print("Valid Pass");
                delay_ms(50);
                rtc_read(&hr, &min, &sec, &date, &month, &year);
                display_datetime(hr, min, sec, date, month, year);
                motor = 1;
							red_led=0;
                delay_ms(500);
            } else {
                lcd_print("Invalid Pass");
							   red_led=1;
							delay_ms(100);
							red_led=0;
							
            }

            delay_ms(500);
            motor = 0;
						red_led=1;
            lcd_cmd(0x01);
            lcd_print("Scan RFID Card");
            idx = 0;
        } else {
            input[idx++] = ch;
            if (idx >= sizeof(input) - 1) idx = 0;
        }
    }
}

