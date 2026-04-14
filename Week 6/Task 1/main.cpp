#include "mbed.h"
#include <stdint.h>
#include <string.h>
#include "display.h"

//Defining custom varaibles
#define BLINKING_TIME_GAS_ALARM                 1000
#define BLINKING_TIME_OVER_TEMP_ALARM           500
#define BLINKING_TIME_GAS_AND_OVER_TEMP_ALARM   100
#define NUMBER_OF_AVG_SAMPLES                   100
#define TIME_INCREMENT_MS                       10
#define SERIAL_PRINT_MS                         1000

#define NUMBER_OF_KEYS                          4
#define DEBOUNCE_TIME_MS                        40
#define KEYPAD_NUMBER_OF_ROWS                   4
#define KEYPAD_NUMBER_OF_COLS                  4

//Added a Min and Max for the sensors
#define TEMP_MIN    20.0
#define TEMP_MAX    80.0

// custom enumeration data types
typedef enum{
    KEY_SCAN,
    KEY_DEBOUNCE,
    KEY_HOLD,   
    MATRIX_KEYPAD,
}   keypad_state;


DigitalOut alarmLED(LED3);
DigitalOut incorrectCodeLED(LED1);
DigitalOut systemLockedLED(LED2);
DigitalOut buzzer(PE_10);
DigitalIn gasSensor(PE_12);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

AnalogIn lm35(A1);
AnalogIn potentiometer(A0);

//Setting the row and column array and tying them to the nucleo board pins
DigitalOut keyRow[KEYPAD_NUMBER_OF_ROWS] = {PB_3, PB_5, PC_7, PA_15};
DigitalIn keyCol[KEYPAD_NUMBER_OF_COLS] = {PB_12, PB_13, PB_15, PC_6};

//character array to tie the inputs of the kaypad
char matrixKeypadIndexCharArray[]{
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D',
};

int timeBlink = 0;
int timeSerial = 0;
int uptimeSec = 0;

bool alarmSilenced = false;
bool alarmState = false;
bool gasAlarm = false;
bool tempAlarm = false;
bool tempWarning = false;
bool systemHalted = false;

//Float varaibles count for decimals points unlike integers
float lm35ReadingArray[NUMBER_OF_AVG_SAMPLES];
float lm35ReadingSum        = 0.0;
float lm35ReadingAverage    = 0.0;
float lm35TempC             = 0.0;
float potReading            = 0.0;

//Added varaibles that effected the sensitivity
float sensitivity = 0.0;
float tempThreshold = 0.0;

//Sets the number code that fills the array
char sequence[NUMBER_OF_KEYS] = {'1', '9', '5', '8'};
char reactivate[NUMBER_OF_KEYS] = {'1', '1', '1', '1'};
char pressed[NUMBER_OF_KEYS] = {'0', '0', '0', '0'};
int matrixKeyCode            = 0;
int incorrectCodes           = 0;
int HashEvent                = 0;
int debounceTime             = 0;
int lastKeyPressed           = '\0';

char str[200];      
int stringlength;
char alarmLog[5][60];
int alarmLogCount = 0;
 //variable that tracks the state the peypad is currently in
keypad_state keypadState;

void input();
void output();
void tempUpdate();
void gasUpdate();
void potUpdate();
void alarmUpdate();
void buzzerUpdate();
void serialUpdate();
void keypadScanInput();
void keypadCodeUpdate();
void logAlarmEvent();
void displaySensorStatus(char key);

  
char keypadScan();
char keypadUpdate();
bool areEqual();
bool areEqualReactivate();

float celsiusToF(float tempCelsius);
float lm35AnalogReading(float analogReading);

// main() runs in its own thread in the OS
int main(){
    input();
    output();

    while (true) {
        //only activates when the system is not in halted mode
        if(!systemHalted){
            potUpdate();
            tempUpdate();
            gasUpdate();
            alarmUpdate();
            buzzerUpdate();
        }
        keypadCodeUpdate();
        serialUpdate();
        thread_sleep_for(TIME_INCREMENT_MS);

    }
}

void input(){
    gasSensor.mode(PullDown);
    keypadScanInput();
}

void output(){
    alarmLED = 0;
    buzzer = 0;
    incorrectCodeLED = 0;
    systemLockedLED = 0;
    displayInit(DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER);
}

//Function to handle the LCD display
void displaySensorStatus(char key){
    if(key == '4'){
        displayCharPositionWrite(0, 0);
        displayStringWrite("Gas Sensor:     ");
        displayCharPositionWrite(0, 1);
        displayStringWrite(gasAlarm ? "ALARM           " : "OK              ");

    }else if(key == '5'){
        displayCharPositionWrite(0, 0);
        displayStringWrite("Temp Sensor:    ");
        displayCharPositionWrite(0, 1);
        char tempStr[20];
        sprintf(tempStr, "%.2fC %s", lm35TempC, tempAlarm ? "ALARM   " : "OK      ");
        displayStringWrite(tempStr);
    }
}

//Function that constanly reads the pot value
void potUpdate(){
    potReading = potentiometer.read();
     //Converting to allow for a 0-100% scale
    sensitivity = potReading *100.0;
    tempThreshold = TEMP_MAX - (potReading*(TEMP_MAX - TEMP_MIN));
}

//This function measures the temp of the lm35
void tempUpdate(){
    // Static integer will keep the value when function is called
    static int lm35Sample = 0;
    //Circular buffer to account for electrical noise
    // lm35.read takes the lm35 measurement over 1s and stores in the array
    lm35ReadingArray[lm35Sample] = lm35.read();
    //Every read moves on to the next array position
    lm35Sample ++;
    //When 100 readings have been read, reset the variable
    if(lm35Sample >= NUMBER_OF_AVG_SAMPLES){   
        lm35Sample = 0;
    }
    //Reset the count before the loop starts
    lm35ReadingSum = 0.0;
    //For loop that cycles through the array
    for(int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++){
        //This adds all the array reads and saves them in the sum of all readings
        lm35ReadingSum += lm35ReadingArray[i];
    }
    //This line takes the sum of readings and divides by 100 to get an average of all aray readings
    lm35ReadingAverage = lm35ReadingSum / NUMBER_OF_AVG_SAMPLES;
    //This calls the conversion function to convert the C temp to F temp
    lm35TempC = lm35AnalogReading(lm35ReadingAverage);
    // If statement to light up warning LED if over max temp
    if(lm35TempC > tempThreshold){
        tempAlarm = 1;
        tempWarning = true;
    }else{
        tempAlarm = 0;
        tempWarning = false;
    }
}

//This functions measures mq2 gas sensor
void gasUpdate(){
    //mq2 is a DC sensor so it is either on or off and is active low
    if(!gasSensor){
        gasAlarm = true;
    }else{
        gasAlarm = false;
    }
}


//Function to flash LED3 in accoradance to what alarms are active
void alarmUpdate(){
    //If statement to turn on alarm mode 
    static int uptimeMs = 0;
    uptimeMs += TIME_INCREMENT_MS;
    uptimeSec = uptimeMs / 1000;
    if(gasAlarm || tempAlarm){
        //If on then turn alarm mode on    
        alarmState = true;
        //Else turn the alarm mode off
    }else{
        alarmState = false;
        alarmSilenced = false;
    }
    //If the alarm mode is activated
    if(alarmState && !alarmSilenced){
        //This adds the time increment to timeBlink and stores the new value
        //Will loop every 10ms
        timeBlink += TIME_INCREMENT_MS;
        //Variable to set and save the speed of blinking LED
        int blink = 0;
        // If statement to set the blink speed in accoradance to which alarm is active
        if(gasAlarm && tempAlarm){
            blink = BLINKING_TIME_GAS_AND_OVER_TEMP_ALARM;
        }else if(gasAlarm){
            blink = BLINKING_TIME_GAS_ALARM;
        }else if(tempAlarm){
            blink = BLINKING_TIME_OVER_TEMP_ALARM;
        }
        // If the blink repeats for too long and all alarms are off, then reset the LED
        if(timeBlink >= blink){
            timeBlink = 0;
            alarmLED = !alarmLED;
        }
    //Reset if not triggered
    } else{
        alarmLED = 0;
        timeBlink = 0;
    }

    static bool prevAlarmState = false;
    if(alarmState && !prevAlarmState){
        logAlarmEvent();
    }
    prevAlarmState = alarmState;
}

void logAlarmEvent(){
    if(alarmLogCount < 5){
        sprintf(alarmLog[alarmLogCount], "t=%ds Temp=%.2fC Gas=%s Temp=%s", uptimeSec, lm35TempC, gasAlarm ? "ALARM" : "OK", tempAlarm ? "ALARM" : "OK");
        alarmLogCount++;
    }
}

 //Function block that activates the buzzer
void buzzerUpdate(){
    if((gasAlarm || tempAlarm) && !alarmSilenced){
        buzzer = 1;
    }else{
        buzzer = 0;
    }
}
//Function that sets up the keypad ready for use
void keypadScanInput(){
    keypadState = KEY_SCAN;
    for(int i=0; i< KEYPAD_NUMBER_OF_COLS; i++){
        keyCol[i].mode(PullUp);
    }
}
//Continous scans the enitre keypad to detect if a key is being pressed
char keypadScan() {
    for (int row = 0; row < KEYPAD_NUMBER_OF_ROWS; row++) {

        for (int i = 0; i < KEYPAD_NUMBER_OF_ROWS; i++) {
            keyRow[i] = 1;
        }
        keyRow[row] = 0;
        for (int col = 0; col < KEYPAD_NUMBER_OF_COLS; col++) {
            if (keyCol[col] == 0) {
                return matrixKeypadIndexCharArray[row * KEYPAD_NUMBER_OF_COLS + col]; // semicolon added
            }
        }
    }
    return '\0';
}


char keypadUpdate(){
    char keyDetected = '\0';
    char keyReleased = '\0';

    switch(keypadState){

        case KEY_SCAN:
            keyDetected = keypadScan();
            if(keyDetected != '\0'){
                lastKeyPressed = keyDetected;
                debounceTime = 0;
                keypadState = KEY_DEBOUNCE;
            }
        break;

        case KEY_DEBOUNCE:
            if(debounceTime >= DEBOUNCE_TIME_MS){
                keyDetected = keypadScan();
                if(keyDetected == lastKeyPressed){
                    keypadState = KEY_HOLD;
                }else{
                    keypadState = KEY_SCAN;
                }
            }
            debounceTime += TIME_INCREMENT_MS;
            break;
        
        case KEY_HOLD:
            keyDetected = keypadScan();
            if(keyDetected != lastKeyPressed){
                if(keyDetected == '\0'){
                    keyReleased = lastKeyPressed;
                }
                keypadState = KEY_SCAN;
            }
        break;

        default:
            keypadScanInput();
            break;    
    }
    return keyReleased;
}

bool areEqual(){
    for(int i=0; i < NUMBER_OF_KEYS; i++){
        if(sequence[i] != pressed[i]){
            return false;
        }
    }
    return true;
}

bool areEqualReactivate(){
    for(int i = 0; i < NUMBER_OF_KEYS; i++){
        if(reactivate[i] != pressed[i]){
            return false;
        }
    }
    return true;
}

void keypadCodeUpdate(){
    if(incorrectCodes < 5){

        char keyReleased = keypadUpdate();

        if(keyReleased == '4' || keyReleased == '5'){
            displaySensorStatus(keyReleased);
            return;
        }

        if(keyReleased != '\0' && keyReleased != '#'){
            if(matrixKeyCode >= NUMBER_OF_KEYS){
                matrixKeyCode = 0;
            }
            pressed[matrixKeyCode] = keyReleased;
            matrixKeyCode++;
        }

        if(keyReleased == '#'){
            if(incorrectCodeLED){
                HashEvent++;
                if(HashEvent >= 2){
                    incorrectCodeLED = 0;
                    HashEvent        = 0;
                    matrixKeyCode    = 0;
                }
            } else {
                stringlength = sprintf(str, "Entered: %c%c%c%c\r\n", pressed[0], pressed[1], pressed[2], pressed[3]);
                uartUsb.write(str, stringlength);

                stringlength = sprintf(str, "Last %d alarm events\r\n", alarmLogCount);
                uartUsb.write(str, stringlength);
                for(int i = 0; i < alarmLogCount; i++){
                    stringlength = sprintf(str, "[%d] %s\r\n", i+1, alarmLog[i]);
                    uartUsb.write(str, stringlength);
                }
                if(alarmLogCount == 0){
                    stringlength = sprintf(str, "none yet\r\n");
                    uartUsb.write(str, stringlength);
                }

                if(systemHalted){
                    if(areEqualReactivate()){
                        systemHalted     = false;
                        alarmSilenced    = false;
                        alarmState       = false;
                        gasAlarm         = false;
                        tempAlarm        = false;
                        incorrectCodes   = 0;
                        matrixKeyCode    = 0;
                        incorrectCodeLED = 0;
                        alarmLED         = 0;
                        buzzer           = 0;
                        stringlength = sprintf(str, "System reactivated - Normal monitoring resumed\r\n");
                        uartUsb.write(str, stringlength);
                    } else {
                        incorrectCodeLED = 1;
                        incorrectCodes++;
                    }
                } else {
                    if(areEqual()){
                        systemHalted     = true;
                        alarmSilenced    = true;
                        alarmState       = false;
                        gasAlarm         = false;
                        tempAlarm        = false;
                        incorrectCodes   = 0;
                        matrixKeyCode    = 0;
                        incorrectCodeLED = 0;
                        alarmLED         = 0;
                        buzzer           = 0;
                        stringlength = sprintf(str, "Alarms deactivated Enter 1111# to reactivate\r\n");
                        uartUsb.write(str, stringlength);
                    } else {
                        incorrectCodeLED = 1;
                        incorrectCodes++;
                    }
                }
            }  
        }  

    } else {
        systemLockedLED = 1;
    }
}



void serialUpdate(){

    //Same time buffer as used for alarmState LED blinks
    timeSerial += TIME_INCREMENT_MS;
    if(timeSerial >= SERIAL_PRINT_MS){
        timeSerial = 0;
        if(systemHalted){
            stringlength = sprintf(str, "Alarms deactivated\r\n Enter 1111# to reactivate\r\n");
            uartUsb.write(str, stringlength);
            return;
        }


        //Print message print 2 decimeal places frpm float numbers
        stringlength = sprintf(str, "Temp: %.2f C / %.2f F\r\n", lm35TempC, celsiusToF(lm35TempC));
        uartUsb.write(str, stringlength);

        if(tempWarning){
            stringlength = sprintf(str, "TEMPERATURE WARNING: %.2f C EXCEEDS %.1f C \r\n", lm35TempC, tempThreshold);
            uartUsb.write(str, stringlength);
        }

        stringlength = sprintf(str, "Gas: %s\r\n", gasAlarm ? "ALARM" : "OK");
        uartUsb.write(str, stringlength);
        stringlength = sprintf(str, "Potentiometer sensitivity: %.0f\r\n", sensitivity);
        uartUsb.write(str, stringlength);

        stringlength = sprintf(str, "Temp Threshold: %.1f C\r\n", tempThreshold);
        uartUsb.write(str, stringlength);

        if(gasAlarm && tempAlarm){
            stringlength = sprintf(str, "Buzzer on - Cause: Temperature and Gas\r\n");
            uartUsb.write(str, stringlength);
        }else if(gasAlarm){
            stringlength = sprintf(str, "Buzzer on - Cause: Gas\r\n");
            uartUsb.write(str, stringlength);
        }else if(tempAlarm){
            stringlength = sprintf(str, "Buzzer on - Cause: Temperature\r\n");
            uartUsb.write(str, stringlength);
        }else{
            stringlength = sprintf(str, "Buzzer off\r\n");
            uartUsb.write(str, stringlength);
            stringlength = sprintf(str, "System Normal\r\n");
            uartUsb.write(str, stringlength);
        }
        if(alarmState && !alarmSilenced){
        stringlength = sprintf(str, "Enter 4-Digit Code to Deactivate\r\n");
        uartUsb.write(str, stringlength);
        }

        stringlength = sprintf(str, "Alarm: %s\r\n", alarmState ? "Active" : "OFF");
        uartUsb.write(str, stringlength);
    }
}

float lm35AnalogReading(float analogReading){
    return (analogReading * 3.3 / 0.01);
}

float celsiusToF(float tempCelsius){
    return (tempCelsius *9.0/5.0 + 32.0);
}
