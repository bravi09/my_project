// -----------------------------
// Pin Configuration
// -----------------------------
#define LCD_DATA P2
sbit rs = P2^0;
sbit en = P2^1;
sbit motor = P3^3;
sbit red_led = P3^4;
sbit SDA = P0^1;
sbit SCL = P0^0;

// -----------------------------
// RTC I2C DS1307 Address Macros
// -----------------------------
#define RTC_ADDRESS 0x68
#define RTC_WRITE (RTC_ADDRESS << 1)   //D0
#define RTC_READ  ((RTC_ADDRESS << 1) | 1)  //D1

// -----------------------------
// Global Variables
// -----------------------------
char valid_codes[4][11] = {
    "23P35A0404",
    "23P35A0405",
    "23P35A0406",
    "23P35A04"
};

char input[20];
unsigned int idx = 0;

// -----------------------------
// Delay Function
// -----------------------------
void delay_ms(unsigned int t) {
    unsigned int i, j;
    for (i = 0; i < t; i++)
        for (j = 0; j < 1275; j++);
}

// -----------------------------
// LCD Functions
// -----------------------------
void lcd_cmd(unsigned char cmd) {
    LCD_DATA = cmd & 0xF0;
    rs = 0; en = 1; delay_ms(1); en = 0;
    LCD_DATA = (cmd << 4) & 0xF0;
    rs = 0; en = 1; delay_ms(1); en = 0;
}

void lcd_data(unsigned char dat) {
    LCD_DATA = dat & 0xF0;
    rs = 1; en = 1; delay_ms(1); en = 0;
    LCD_DATA = (dat << 4) & 0xF0;
    rs = 1; en = 1; delay_ms(1); en = 0;
}

void lcd_init() {
    lcd_cmd(0x02); lcd_cmd(0x28); lcd_cmd(0x0C);
    lcd_cmd(0x06); lcd_cmd(0x01); lcd_cmd(0x80);
}

void lcd_print(char *str) {
    while (*str) lcd_data(*str++);
}

// -----------------------------
// UART Functions
// -----------------------------
void uart_init() {
    TMOD = 0x20;
    TH1 = 0xFD;
    SCON = 0x50;
    TR1 = 1;
}

char uart_rx() {
    while (!RI);
    RI = 0;
    return SBUF;
}

void uart_tx(char ch) {
    SBUF = ch;
    while (!TI);
    TI = 0;
}

// -----------------------------
// I2C Functions
// -----------------------------
void I2C_Start() {
    SDA = 1; SCL = 1; delay_ms(1);
    SDA = 0; delay_ms(1);
    SCL = 0;
}

void I2C_Stop() {
    SDA = 0; SCL = 1; delay_ms(1);
    SDA = 1; delay_ms(1);
}

void I2C_Write(unsigned char dat) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        SDA = (dat & 0x80) ? 1 : 0;
        SCL = 1; delay_ms(1);
        SCL = 0; delay_ms(1);
        dat <<= 1;
    }
    SDA = 1;
    SCL = 1; delay_ms(1);
    SCL = 0;
}

unsigned char I2C_Read(bit ack) {
    unsigned char i, dat = 0;
    SDA = 1;
    for (i = 0; i < 8; i++) {
        SCL = 1; delay_ms(1);
        dat <<= 1;
        if (SDA) dat |= 1;
        SCL = 0; delay_ms(1);
    }
    SDA = (ack == 1) ? 0 : 1;
    SCL = 1; delay_ms(1);
    SCL = 0;
    SDA = 1;
    return dat;
}

// -----------------------------
// RTC Functions
// -----------------------------
unsigned char BCD_to_dec(unsigned char val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

unsigned char dec_to_BCD(unsigned char val) {
    return ((val / 10) << 4) | (val % 10);
}

void rtc_read(unsigned char *hr, unsigned char *min, unsigned char *sec, unsigned char *date, unsigned char *month, unsigned char *year) {
    I2C_Start();
    I2C_Write(RTC_WRITE);
    I2C_Write(0x00);
    I2C_Start();
    I2C_Write(RTC_READ);
    *sec = BCD_to_dec(I2C_Read(1));
    *min = BCD_to_dec(I2C_Read(1));
    *hr  = BCD_to_dec(I2C_Read(1));
    I2C_Read(1); // Skip day
    *date = BCD_to_dec(I2C_Read(1));
    *month = BCD_to_dec(I2C_Read(1));
    *year = BCD_to_dec(I2C_Read(0));
    I2C_Stop();
}

void rtc_set(unsigned char hr, unsigned char min, unsigned char sec) {
    I2C_Start();
    I2C_Write(RTC_WRITE);
    I2C_Write(0x00);
    I2C_Write(dec_to_BCD(sec));
    I2C_Write(dec_to_BCD(min));
    I2C_Write(dec_to_BCD(hr));
    I2C_Stop();
}

// -----------------------------
// Helper Functions
// -----------------------------
bit is_valid(char *ch) {
    unsigned char k;
    for (k = 0; k < 4; k++) {
        if (strcmp(ch, valid_codes[k]) == 0)
            return 1;
    }
    return 0;
}

// Function: display_datetime()
// Purpose : To display Date and Time on LCD in the format:
//           Date: DD/MM/YY  (on 1st line)
//           Time: HH:MM:SS  (on 2nd line)

void display_datetime(unsigned char hr, unsigned char min, unsigned char sec,
                      unsigned char date, unsigned char month, unsigned char year) {

    // --- Display Date on First Line ---
    lcd_cmd(0x80);   // Move LCD cursor to first line, first position

    // Display Date (DD)
    lcd_data((date / 10) + '0');    // Display tens digit of date
    lcd_data((date % 10) + '0');    // Display ones digit of date
    lcd_data('/');                  // Display separator '/'

    // Display Month (MM)
    lcd_data((month / 10) + '0');   // Tens digit of month
    lcd_data((month % 10) + '0');   // Ones digit of month
    lcd_data('/');                  // Display separator '/'

    // Display Year (YY)
    lcd_data((year / 10) + '0');    // Tens digit of year
    lcd_data((year % 10) + '0');    // Ones digit of year

    // --- Display Time on Second Line ---
    lcd_cmd(0xC0);   // Move LCD cursor to second line, first position

    // Display Hours (HH)
    lcd_data((hr / 10) + '0');      // Tens digit of hour
    lcd_data((hr % 10) + '0');      // Ones digit of hour
    lcd_data(':');                  // Display separator ':'

    // Display Minutes (MM)
    lcd_data((min / 10) + '0');     // Tens digit of minute
    lcd_data((min % 10) + '0');     // Ones digit of minute
    lcd_data(':');                  // Display separator ':'

    // Display Seconds (SS)
    lcd_data((sec / 10) + '0');     // Tens digit of second
    lcd_data((sec % 10) + '0');     // Ones digit of second
}
