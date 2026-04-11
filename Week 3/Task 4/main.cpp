#include "mbed.h"


UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//Initialising board components
DigitalIn   B1(BUTTON1);
DigitalIn   gasSensor(D2);
DigitalIn   tempSensor(D3);
DigitalIn   buttonA(D4);
DigitalIn   buttonB(D5);
DigitalIn   buttonC(D6);
DigitalIn   buttonD(D7);
DigitalOut  gasAlarm(LED1);
DigitalOut  tempAlarm(LED2);

bool toggleGas = false;
bool toggleTemp = false;
bool monitoring = false;
bool lastGas = false;
bool lastTemp = false;

//Prototype functions
void outputs();
void inputs();
void menu();
void menuMessage();
void tempAlarmState();
void gasAlarmState();
void monitoringMode();
void warning();

// main() runs in its own thread in the OS
int main(){
    //Calls input functions to reset alarms
    inputs();
    outputs();
    menuMessage();
    while (true) {
        menu();
        warning();
        //If statement that loops monitoring and breaks when toggled off in the menu
        if(monitoring){
            monitoringMode();
            //2 second timer that repeats the monitoring function
            ThisThread::sleep_for(2000ms);
        }
        
    }
}

void inputs(){
    //Set the mode that the button will be used
    gasSensor.mode(PullDown);
    tempSensor.mode(PullDown);
    buttonA.mode(PullDown);
    buttonB.mode(PullDown);
    buttonC.mode(PullDown);
    buttonD.mode(PullDown);
}

void outputs(){
    gasAlarm = 0;
    tempAlarm = 0;
}

//Menu function that carries out tasks based on user input on serial monitor
void menu(){
    //Reset the input
    char receivedchar = '\0';
    //Scans for users input via serial monitor
    if(uartUsb.readable()){
        uartUsb.read(&receivedchar, 1);
        //User inputs number character to select menu option
        switch (receivedchar){
            case '1':
                //Toggles on and off the boolean variable
                toggleGas = !toggleGas;
                if (toggleGas){
                    uartUsb.write("Gas alarm simulation is active!\r\n", 33);
                    gasAlarm = 1;
                }else {
                    uartUsb.write("Gas alarm simulation is off!\r\n", 30);
                    gasAlarm = 0;
                }
                menuMessage();
                break;
            case '2':
            //Calls Function to check the gas alarm state
                gasAlarmState();
                menuMessage();
                break;
            //Calls Function to check the oven alarm state    
            case '3':
                tempAlarmState();
                menuMessage();
                break; 
            case '4':
                //Toggles on and off the boolean variable
                toggleTemp = !toggleTemp;
                if (toggleTemp){
                    uartUsb.write("Temp alarm simulation is active!\r\n", 34);
                    tempAlarm = 1;
                }else {
                    uartUsb.write("Temp alarm simulation is off!\r\n", 31);
                    tempAlarm = 0;
                }
                menuMessage();
                break; 
                case '6':
                    monitoring = !monitoring;
                    if(monitoring){
                        uartUsb.write("Entering Monitoring Mode is on.\r\n", 33);                       
                    }else{
                        uartUsb.write("Exiting Monitoring Mode.\r\n", 26);
                    }
                break;             
            //If an input is not a menu input, asks the user for another input    
            default:
                uartUsb.write("Invalid Input, Try again.\r\n", 27);
        }

    }
    
}
//Function to loop and check the state of the temperature
void warning(){

    bool gasState = (toggleGas || gasSensor);
    bool tempState = (toggleTemp || tempSensor);

    if (gasState && !lastGas){
        uartUsb.write("WARNING: GAS DETECTED\r\n", 23);
    }
    if (tempState && !lastTemp){
        uartUsb.write("WARNING: TEMPERATURE TOO HIGH\r\n", 31);
    }

    //Saves the last value to for when the loop resets
    lastGas = gasState;
    lastTemp = tempState;
}

void monitoringMode(){
    uartUsb.write("Entering Monitoring Mode!\r\n", 27);
    if(tempAlarm || tempSensor){
        uartUsb.write("Temperature sensor is on!\r\n", 27);
    }else{
        uartUsb.write("Temperature sensor is off.\r\n.", 28);
    }
    if(gasAlarm || gasSensor){
        uartUsb.write("Gas sensor is on!\r\n", 19);
    }else{
        uartUsb.write("Gas sensor is off.\r\n", 120);
    }
}

//Function that detects if oven sensor is active and prints to serial monitor
void tempAlarmState(){
    if(tempAlarm || tempSensor){
        uartUsb.write("\nTEMPERATURE ALARM ACTIVE\r\n", 27);
    }else{
        uartUsb.write("\nTEMPERATURE ALARM CLEAR\r\n", 26);
    }
}

//Function that detects if gas sensor is active and prints to serial monitor
void gasAlarmState(){
    if(gasAlarm || gasSensor){
        uartUsb.write("\nGAS ALARM ACTIVE\r\n", 19);
    }else{
        uartUsb.write("\nGAS ALARM CLEAR\r\n", 18);
    }
}


//Function to print menu options
void menuMessage(){
    //Prints all menu options to serial monitor
    uartUsb.write("\r\n", 2);
    uartUsb.write("Press 1 to toggle gas alarm simulation ON/OFF.\r\n", 48);
    uartUsb.write("Press 2 to request gas alarm state.\r\n", 37);
    uartUsb.write("Press 3 to request oven temp alarm state.\r\n", 43);
    //Extra byte added to string to patch Menu UI issue
    uartUsb.write("Press 4 to toggle oven temp alarm simulation ON/OFF.\r\n", 54);
    uartUsb.write("Press 5 to show and reset both alarms.\r\n", 40);
    uartUsb.write("Press 6 to Open/Exit monitoring mode.\r\n", 39);
}
