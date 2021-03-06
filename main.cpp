#include "mbed.h"

DigitalOut led1(LED1);

PwmOut rightR(PB_6);
//PwmOut rightF(PA_7);
PwmOut rightF(PB_10);
PwmOut leftF(PA_7);
//PwmOut leftF(PB_10);
PwmOut leftR(PC_7);
InterruptIn encoderRightR(PB_3);
InterruptIn encoderRightF(PA_15);
InterruptIn encoderLeftR(PA_1);
InterruptIn encoderLeftF(PC_4);

Timer timer;

volatile unsigned long pulsesRight = 0;
volatile unsigned long pulsesLeft = 0;
int errorPulse = 1;
double Kp = 0.0001;   // arbitrary values
double Kd = 200;
double Ki = 0.5;
double speedChange = 0;     //holds P,I,D fn return values
int integral = 0;
double maxCorrect = 0.1;    //limits correction values
double maxSpeed = 0.15;     //limits motor speed
double minSpeed = 0.05;     //need this?
double speedR = 0.1;        //for setting motor speed; accumulates
double speedL = 0.1;
int cnt=0;                  //for D
int prevError = 0;  //for D

Serial pc(PA_2, PA_3);  //set serial

void incrementRight(){  // increments right pulses
    pulsesRight++;
    return;
}
void incrementLeft(){  //increments left pulses
    pulsesLeft++;
    return;
}

void stop() {    // stops mouse (turns motors to 0)
    rightF = 0;
    rightR = 0;
    leftF = 0;
    leftR = 0;
    wait(0.5);
    return;
}

void turnR90() {   // turns 90 deg right
    int temp = pulsesRight;
    pulsesRight=0;
    stop();
    while(pulsesRight<750) {
        rightF = 0.1;
        leftR = 0.1;
    }
    stop();
    pulsesRight = temp;
    return;
}

void turnL90() {     // turns 90 deg left
    int temp = pulsesRight;
    pulsesRight=0;
    stop();
    while(pulsesRight<750) {
        rightR = 0.1;
        leftF = 0.1;
    }
    stop();
    pulsesRight = temp;
    return;
}

void forward(int cells) {   // moves forward num of cells
    int temp = pulsesRight;
    pulsesRight = 0;
    int n = 2500*cells;
    while(pulsesRight<n){
        rightF = 0.1;
        rightR = 0;
        leftF = 0.1;
        leftR = 0;
    }
    pulsesRight = temp + pulsesRight;
    stop();
    return;
}

int speedLeft(double speed){    // writes the speed of left motor (checks if valid)
    if (speed < maxSpeed){
        leftF.write(speed);
        return 1;
    }
    return 0;
}
int speedRight(double speed){    // writes speed of right motor (checks if valid)
    if (speed < maxSpeed){
        rightF.write(speed);
        return 1;
    }
    return 0;
}

void resetSpeed(double speed){    // resets forward speeds to input
    rightF.write(speed);
    rightR.write(0);
    leftF.write(speed);
    leftR.write(0);
    return;
}

double P_Controller(int error) // multiplies by Kp
{
    double correction = Kp * error;  // check if this is the right constant (1 pulse = 1 rotation of inner shaft, = ____ wheel rotations)
        if (abs(correction) > maxCorrect) // check with max
            correction = maxCorrect;
    return correction;
}

double D_Controller(int error) //calc D correction
{
    if(cnt==0) {    //once per 10 cycles
        int dError = error - prevError; //fairly small #
        double dt = timer.read_us();    //for once/10 cycles, dt = ~20
        //pc.printf("time: %f\n", dt);
        timer.reset();
        prevError = error;
        double correction = Kd*dError/dt;
        pc.printf("derror: %f\n", correction);  //gives ~0.X
        if (abs(correction) > maxCorrect) // check with max
            correction = maxCorrect;     
        return correction;
    }
    cnt++;
    cnt = cnt%10;
    return 0;
}


int main() {
    
    encoderRightF.rise(&incrementRight);
    encoderRightF.fall(&incrementRight);
    encoderLeftF.rise(&incrementLeft);
    encoderLeftF.fall(&incrementLeft);
    pc.baud(115200);
    
    while(1) {
        timer.start();
        led1 = 1;
        errorPulse = pulsesRight - pulsesLeft; //if errorPulse negative: left is faster than right

        //PROPORTIONAL
        speedChange = P_Controller(errorPulse); //can be neg or pos
        
        //DERIVATIVE
        //speedChange += D_Controller(errorPulse); //when P neg, D is pos; this line not tested
      
        speedR -= speedChange; //if speedChange neg (left is faster), speedR increases
        speedL += speedChange;

         if (errorPulse == 0) { //if no error go normal speed
            speedR = 0.1;
            speedL = 0.1;
        }
               
        speedLeft(speedL);
        speedRight(speedR);
        
        //INTEGRAL not done
        integral += errorPulse;
        integral /= 2;
        
        //pc.printf("%d\r", integral);
        //pc.printf(" ");
        //pc.printf("%f\r", speedChange);
        //pc.printf(" ");
        //pc.printf("%d\r\n", errorPulse);
        /*pc.printf(" ");
         pc.printf("%d\r", pulsesRight);
         pc.printf(" ");
         pc.printf("%d\r\n", pulsesLeft);
         pc.printf(" ");
         pc.printf("%f\r", leftF.read());
         pc.printf(" ");
         pc.printf("%f\r\n", rightF.read());
         */
         //pc.printf("error: %d\n", errorPulse);
       
        timer.stop();
    }
}
