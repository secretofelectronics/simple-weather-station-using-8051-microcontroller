#include <reg51.h>
#include <stdio.h>  // Include for sprintf function

// I2C and LCD functions
void i2c_start(void);
void i2c_stop(void);
void i2c_ACK(void);
void i2c_write(unsigned char);
void lcd_send_cmd(unsigned char);
void lcd_send_data(unsigned char);
void lcd_send_str(unsigned char *);
void lcd_slave(unsigned char);
void delay_ms(unsigned int);

// DHT11 sensor functions
void timer_delay20ms(void);
void timer_delay30us(void);
void Request(void);
void Response(void);
int Receive_data(void);

// LCD I2C address
unsigned char slave1 = 0x4E;
unsigned char slave_add;

// DHT11 variables
int I_RH, D_RH, I_Temp, D_Temp, CheckSum;

sbit scl = P2^1;
sbit sda = P2^0;
sbit DHT11 = P2^2;

// I2C Functions
void i2c_start(void) {
    sda = 1;
    scl = 1;
    sda = 0;
    scl = 0;
}

void i2c_stop(void) {
    scl = 0;
    sda = 0;
    scl = 1;
    sda = 1;
}

void i2c_ACK(void) {
    scl = 0;
    sda = 1;
    scl = 1;
    while(sda);
}

void i2c_write(unsigned char dat) {
    unsigned char i;
    for(i = 0; i < 8; i++) {
        scl = 0;
        sda = (dat & (0x80 >> i)) ? 1 : 0;
        scl = 1;
    }
}

void lcd_slave(unsigned char slave) {
    slave_add = slave;
}

// LCD Functions
void lcd_send_cmd(unsigned char cmd) {
    unsigned char cmd_l, cmd_u;
    cmd_l = (cmd << 4) & 0xf0;
    cmd_u = (cmd & 0xf0);
    
    i2c_start();
    i2c_write(slave_add);
    i2c_ACK();
    i2c_write(cmd_u | 0x0C);
    i2c_ACK();
    delay_ms(1);
    i2c_write(cmd_u | 0x08);
    i2c_ACK();
    delay_ms(10);
    i2c_write(cmd_l | 0x0C);
    i2c_ACK();
    delay_ms(1);
    i2c_write(cmd_l | 0x08);
    i2c_ACK();
    delay_ms(10);
    i2c_stop();
}

void lcd_send_data(unsigned char dataw) {
    unsigned char dataw_l, dataw_u;
    dataw_l = (dataw << 4) & 0xf0;
    dataw_u = (dataw & 0xf0);
    
    i2c_start();
    i2c_write(slave_add);
    i2c_ACK();
    i2c_write(dataw_u | 0x0D);
    i2c_ACK();
    delay_ms(1);
    i2c_write(dataw_u | 0x09);
    i2c_ACK();
    delay_ms(10);
    i2c_write(dataw_l | 0x0D);
    i2c_ACK();
    delay_ms(1);
    i2c_write(dataw_l | 0x09);
    i2c_ACK();
    delay_ms(10);
    i2c_stop();
}

void lcd_send_str(unsigned char *p) {
    while(*p != '\0') {
        lcd_send_data(*p++);
    }
}

void delay_ms(unsigned int n) {
    unsigned int m;
    for(; n > 0; n--) {
        for(m = 121; m > 0; m--);
    }
}

void lcd_init() {
    lcd_send_cmd(0x02);  // Return home
    lcd_send_cmd(0x28);  // 4-bit mode
    lcd_send_cmd(0x0C);  // Display On, cursor off
    lcd_send_cmd(0x06);  // Increment Cursor (shift cursor to right)
    lcd_send_cmd(0x01);  // Clear display
}

// Timer delay functions
void timer_delay20ms(void) {
    TMOD = 0x01;
    TH0 = 0xB8;
    TL0 = 0x0C;
    TR0 = 1;
    while(TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}

void timer_delay30us(void) {
    TMOD = 0x01;
    TH0 = 0xFF;
    TL0 = 0xF1;
    TR0 = 1;
    while(TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}

// DHT11 Functions
void Request(void) {
    DHT11 = 0;
    timer_delay20ms();
    DHT11 = 1;
}

void Response(void) {
    while(DHT11 == 1);
    while(DHT11 == 0);
    while(DHT11 == 1);
}

int Receive_data(void) {
    int q, c = 0;
    for (q = 0; q < 8; q++) {
        while(DHT11 == 0);
        timer_delay30us();
        if (DHT11 == 1) {
            c = (c << 1) | (0x01);
        } else {
            c = (c << 1);
        }
        while(DHT11 == 1);
    }
    return c;
}

// Main program
void main() {
    unsigned char dat[20];
    
    lcd_slave(slave1);  // Set the I2C address for the LCD
    lcd_init();         // Initialize the LCD
    
    while (1) {
        Request();      // Send start pulse to DHT11
        Response();     // Wait for DHT11 response
        
        I_RH = Receive_data();  // Get integer humidity
        D_RH = Receive_data();  // Get decimal humidity
        I_Temp = Receive_data(); // Get integer temperature
        D_Temp = Receive_data(); // Get decimal temperature
        CheckSum = Receive_data(); // Get checksum
        
        if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum) {
            lcd_send_cmd(0x80);    // Move cursor to the beginning of the first line
            lcd_send_str("Error");
        } else {
            sprintf(dat, "Hum = %d.%d%%", I_RH, D_RH);  // Add % for humidity
            lcd_send_cmd(0x80);    // Move cursor to the beginning of the first line
            lcd_send_str(dat);
            
            sprintf(dat, "Temp = %d.%d%cC", I_Temp, D_Temp, 0xDF);  // Add ?C using 0xDF
            lcd_send_cmd(0xC0);    // Move cursor to the beginning of the second line
            lcd_send_str(dat);
        }
        
        delay_ms(500);  // Delay before next reading
    }
}
