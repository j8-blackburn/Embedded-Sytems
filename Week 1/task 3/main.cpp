/*
This Program flashes the LEDs on in a ping-pong
Sequence
*/
#include "mbed.h"

// Define outputs for functions
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);


// main() runs in its own thread in the OS
int main()
{
    while (true) {
        //Sequnce for the Ping-Pong Sequence
        led1 = !led1;
        ThisThread::sleep_for(200ms);
        led1 = !led1;
        led2 = !led2;
        ThisThread::sleep_for(200ms);
        led2 = !led2;
        led3 = !led3;
        ThisThread::sleep_for(200ms);
        led3 = !led3;
        led2 = !led2;
        ThisThread::sleep_for(200ms);
        led2 = !led2;
        led1 = !led1;
        ThisThread::sleep_for(200ms);
        led1 = !led1;
    }
}
