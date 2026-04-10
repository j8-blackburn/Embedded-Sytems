#include "mbed.h"

//Initialising Input components
DigitalIn   button0(D2, PullDown);
DigitalIn   button1(D3, PullDown);
DigitalIn   button2(D4, PullDown);
DigitalIn   button3(D5, PullDown);
DigitalIn   button4(D6, PullDown);
DigitalIn   button5(D7, PullDown);

//Initialising Ouput components
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

//Debugging LED to validate code inputs
void correctinput(int presses){
    for(int i = 0; i < presses; i++){
        led2 = 1;
        ThisThread::sleep_for(300ms);
        led2 = 0;
        ThisThread::sleep_for(300ms);
    }

}

//Function the recieves button input and returns the value
int codeinput(){
    int input = -1;

    while (input == -1){
        if (button0) input = 0;
        else if (button1) input = 1;
        else if (button2) input = 2;
        else if (button3) input = 3;
        else if (button4) input = 4;
        else if (button5) input = 5;
        //Debouncing to prevent overlapping code
        ThisThread::sleep_for(50ms);
    }
    //While loop that revieves button press
    while (button0 || button1 || button2 || button3 || button4 || button5) {
        //Debouncing to prevent overlapping code
        ThisThread::sleep_for(50ms);
    }
    //Returns input to main
    return input;
}

// main() runs in its own thread in the OS
int main(){
    led1 = 0;
    while (led1 != 1){
        //Nested if statements that recieves inputs
        int input1 = codeinput();
        if(input1 == 1){
            correctinput(1);
            int input2 = codeinput();
            if (input2 == 2){
                correctinput(2);
                int input3 = codeinput();
                if (input3 == 3){
                    correctinput(3);
                    int input4 = codeinput();
                        if(input4 == 4){
                            led1 = 1;
                            //Leaves correct code LED on for 2 Seconds
                            ThisThread::sleep_for(2000ms);
                            //Resets the loop and starts again
                            led1 = 0;
                        }
                    }
                }
            }

        }  
}
