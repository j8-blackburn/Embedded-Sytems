/*
This program have functions to flash all LEDs 5 times. After the fifth flash
LED1 will stay on and LED2 and LED3 will stay off.
*/

#include "mbed.h"

//Define outputs LEDs
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

//Function that turns LEDs on and off then only LED1 on
void flash(){
    for (int i = 0; i <= 4; i++){
        led1 = 1;
        led2 = 1;
        led3 = 1;
        ThisThread::sleep_for(200ms); 
        led1 = 0;
        led2 = 0;
        led3 = 0;
        ThisThread::sleep_for(200ms); 
        }
    led1 = 1;
    led2 = 0;
    led3 = 0;
}

int main()
{
    flash();
}
