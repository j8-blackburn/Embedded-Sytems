#include "mbed.h"
#include <string.h>

//Defining custom varaibles
#define BLINKING_TIME_GAS_ALARM                 1000
#define BLINKING_TIME_OVER_TEMP_ALARM           500
#define BLINKING_TIME_GAS_AND_OVER_TEMP_ALARM   100
#define NUMBER_OF_AVG_SAMPLES                   100
#define OVER_TEMP                               30
#define TIME_INCREMENT_MS                       10
#define SERIAL_PRINT_MS                         1000

DigitalOut alarmLED(LED3);
DigitalIn gasSensor(PE_12);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

AnalogIn lm35(A1);
AnalogIn potentiometer(A0);

int timeBlink = 0;
int timeSerial = 0;

bool alarmState = false;
bool gasAlarm = false;
bool tempAlarm = false;

//Float varaibles count for decimals points unlike integers
float lm35ReadingArray[NUMBER_OF_AVG_SAMPLES];
float lm35ReadingSum        = 0.0;
float lm35ReadingAverage    = 0.0;
float lm35TempC             = 0.0;
float potReading            = 0.0;

void input();
void output();
void tempUpdate();
void gasUpdate();
void potUpdate();
void alarmUpdate();
void serialUpdate();

float celsiusToF(float tempCelsius);
float lm35AnalogReading(float analogReading);

// main() runs in its own thread in the OS
int main(){
    input();
    output();

    while (true) {
        tempUpdate();
        gasUpdate();
        potUpdate();
        alarmUpdate();
        serialUpdate();
        thread_sleep_for(TIME_INCREMENT_MS);

    }
}

void input(){
    gasSensor.mode(PullDown);
}

void output(){
    alarmLED = 0;
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
    if(lm35TempC > OVER_TEMP){
        tempAlarm = 1;
    }else{
        tempAlarm = 0;
    }
}

//This functions measures mq2 gas sensor
void gasUpdate(){
    //mq2 is a DC sensor so it is either on or off and is active low
    if(gasSensor){
        gasAlarm = true;
    }else{
        gasAlarm = false;
    }
}
//Function that constanly reads the pot value
void potUpdate(){
    potReading = potentiometer.read();
}

//Function to flash LED3 in accoradance to what alarms are active
void alarmUpdate(){
    //If statement to turn on alarm mode 
    if(gasAlarm || tempAlarm){
        //If on then turn alarm mode on    
        alarmState = true;
        //Else turn the alarm mode off
    }else{
        alarmState = false;
    }
    //If the alarm mode is activated
    if(alarmState){
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
}

void serialUpdate(){
    //100 character text buffer
    char str[100];
    int stringlength;

    //Same time buffer as used for alarmState LED blinks
    timeSerial += TIME_INCREMENT_MS;
    if(timeSerial >= SERIAL_PRINT_MS){
        timeSerial = 0;
        //Print message print 2 decimeal places frpm float numbers
        stringlength = sprintf(str, "Temp: %.2f C / %.2f F\r\n\r\n", lm35TempC, celsiusToF(lm35TempC));
        uartUsb.write(str, stringlength);

        stringlength = sprintf(str, "Gas: %s\r\n\n\r\n", gasAlarm ? "ALARM" : "OK");
        uartUsb.write(str, stringlength);
        float potOhms = potReading * 10000.0;
        stringlength = sprintf(str, "Potentiometer: %.2f\r\n\r\n", potOhms);
        uartUsb.write(str, stringlength);

        stringlength = sprintf(str, "Alarm: %s\r\n\r\n", alarmState ? "Active" : "OFF");
        uartUsb.write(str, stringlength);
    }
}

float lm35AnalogReading(float analogReading){
    return (analogReading *3.3/0.01);
}

float celsiusToF(float tempCelsius){
    return (tempCelsius *9.0/5.0 + 32.0);
}
