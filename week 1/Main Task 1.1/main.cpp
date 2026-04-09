/* Code for maintask 1.1
This code is edited to allow the LED's on the board to light up
 */

#include "mbed.h"


// Blinking rate in milliseconds
#define BLINKING_RATE     2000ms


int main()
{
    // Initialise the digital pin LED1 as an output
    DigitalOut led1(LED1);
    // Added digital pins to both LED2 and LED3 as an output
    DigitalOut led2(LED2);
    DigitalOut led3(LED3);

    while (true) {
        led1 = !led1;
        ThisThread::sleep_for(BLINKING_RATE);
        //add both threads for the new LED outputs
        led2 = !led2;
        ThisThread::sleep_for(BLINKING_RATE);
        led3 = !led3;
        ThisThread::sleep_for(BLINKING_RATE);
    }
}
