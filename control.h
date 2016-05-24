/*
 * control.h
 *
 *  Created on: Feb 9, 2013
 *      Author: ecestudent
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#define CONTROL_PERIOD (10) //ms
#define INIT (1)
#define NO_INIT (0)
#define MANUAL (1)
#define DEAD_RECKONING (2)
#define FOLLOW_LIGHT (3)
#define STEADY_STATE_THRESHOLD (5)

#define VS_DEBUGCONTROL 0
#define DEBUGCONTROL(...) do { if (VS_DEBUGCONTROL) xil_printf( __VA_ARGS__); } while (0)


//static void manualPD(int str, int target);
//static void manualDrive(int str, int thr);

typedef struct ControlCommandStruct {
    int type;
    //a is
    //manual: steering angle
    //dead reckoning: steering angle
    //light following: unused
    int a;
    //b is
    //manual: speed
    //dead reckoning: distance
    //light following: unused
    int b;
} ControlStruct;

struct ControlParams {
	//P gain of our velocity controller
	float velP;
	float velD;

};

extern struct ControlParams cParams;
extern unsigned deadReckoningComplete;

void control( void *pvParameters );

#endif /* CONTROL_H_ */
