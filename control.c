/*
 * control.c
 *
 *  Created on: Feb 9, 2013
 *      Author: ecestudent
 */


//RTOS includes
#include "FreeRTOS.h"
#include "semphr.h"

//Xilinx includes
#include "ServoControl.h"

#include "vsUtils.h"
#include "control.h"
#include "vision.h"

#include "plb_quad_encoder.h"

extern int vsRunControl;

struct ControlParams cParams;
//#define TICKS_P_METER = 2050 //this was determined in our first compulsory

int vel = 0;  //mm/s
int accel = 0;  //mm/s/s

unsigned deadReckoningComplete = FALSE;

static void manualDrive(int init, int str, int mmPs);

static void initParams() {

	cParams.velP = 0.006f;
	cParams.velD = 0.1f;
}

static int pwmSaturate(int val) {
	if(val > 30)
		val = 30;
	if(val < -30)
		val = -30;

	return val;
}

//3 Pt Stencil Implementation of Velocity

static void calcVel() {
    //float tickMeter = 0.0f;
	int speed;
    static long tickD1 =0;
    static long tickD2 =0;
    static long tickD3 =0;
    static long tickD4 =0;
    long tick;
    tick = getTicks();

    speed = -tick/12.0 + 2*tickD1/3.0 - 2*tickD3/3.0 + tickD4/12.0;
    vel = speed * 50; // = speed/Ts/ticksPerMillimeter

    accel = -tick/12.0 + 4*tickD1/3.0 - 5*tickD2/2.0 + 4*tickD3/3.0 - tickD4/12.0;

 //   xil_printf("speed: %d\r\n", vel);
 //   xil_printf("accel: %d\r\n", accel);


    tickD4 = tickD3;
    tickD3 = tickD2;
    tickD2 = tickD1;
    tickD1 = tick;
}

static void manualPD(int init, int str, int target) {


	static int ssCount = 0;
	static int ticksBase = 0;
	static int32 thr = 1;
	static int32 ticks = 0;
	static int32 ticksDot = 0;
	static int32 prevTicks = 0;
	static int32 error = 0;
	static uint32 threshold = 150;
	static float kP = 0.03;		//.03
	static float kD = 0.005;	//.013

	if(init) {
		deadReckoningComplete = FALSE;
		thr = 1;
		ticks = 0;
		ticksDot = 0;
		prevTicks = 0;
		error = 0;
		threshold = 150;
		ssCount = 0;

		SetServo(RC_STR_SERVO, str);
		//clearEncoder();
	//	thr = 0;
		SetServo(RC_VEL_SERVO, thr);
		ticksBase = getTicks();
		DEBUGCONTROL("\r\Starting Dead Reckoning to target=%d\r\n",target);
		error = target - ticks;
	}
//	while (abs(error) > threshold){

	prevTicks = ticks;
	ticks = getTicks() - ticksBase;
	error = target - ticks;
	ticksDot = (ticks - prevTicks)*100; //loop frequency is 100Hz

	thr = kP*error - kD*ticksDot;
	DEBUGCONTROL("thr=%d, ticks=%d\r\n",thr, ticks);
	if (thr > 12) {
		thr = 12;
	} else if (thr < -12) {
		thr = -12;
	}

	SetServo(RC_VEL_SERVO, thr);

	if((target > 0 && ticks > target) ||
	   (target < 0 && ticks < target)) {
		target = 0;
		thr = 0;
		SetServo(RC_VEL_SERVO, thr);

		deadReckoningComplete = TRUE;
	}

}

static void manualDist(int init, int str, int dist) {

	static int ticksBase = 0;
	static int ticks = 0;
	static int thr = 0;
	if(init) {
		deadReckoningComplete = FALSE;
		ticksBase = getTicks();
		DEBUGCONTROL("\r\nStarting Dead Reckoning to target=%d\r\n",dist);
	}

	ticks = getTicks() - ticksBase;

	if(dist > 0) {
		manualDrive(0, str, 1500);
	} else {
//		DEBUGCONTROL("\r\nReversing: speed=%d\r\n",-4000);
		manualDrive(0, str, -4000);
	}

	if((dist > 0 && ticks > dist) ||
	   (dist < 0 && ticks < dist)) {
		thr = 0;
		SetServo(RC_VEL_SERVO, thr);

		deadReckoningComplete = TRUE;
	}
}



static void manualDrive(int init, int str, int mmPs) {

	int thr = 0;
	int error = 0;

	int thrFF = 4 + mmPs / 512;	//feed forward term

	SetServo(RC_STR_SERVO, str);
//	SetServo(RC_VEL_SERVO, thr); //thr

	//velocity control in mm per sec
	error = mmPs - vel;

	thr = cParams.velP*error - cParams.velD*accel + thrFF;

//	xil_printf("thr %d\r\n", thr);
	SetServo(RC_VEL_SERVO, pwmSaturate(thr));
}

static void followLight(int init, int mmPs) {

	int str = vsVision.theta / 16;
	//int mmPs = 1500;
//	xil_printf("str=%d\r\n",str);

	manualDrive(0, str, mmPs);
}

// a task to drive the car
void control( void *pvParameters ){
	//do some setup
	ControlStruct command;
	portTickType wakeTime;
	//Our control loops need to be initialized
	//the first time they run a new command
	int init = TRUE;
	int initTick = 0;
	int finalTick = 0;

	initParams();
	clearEncoder();

	wakeTime = xTaskGetTickCount();
	while(vsRunControl) {
		DEBUGT("C>");
		//record start time
		//loop forever unless we get an E-Stop
		//wait for a command
		command = vsControlGetCommand();
		init = TRUE;
		//check to see if we received an abort command
		//if we did, go back and get another command from the Q
		vsControlIsAborted();
		while(!vsControlIsAborted()) {
//			xil_printf("Control command: %d, %d, %d\r\n", command.type, command.a, command.b);
			switch(command.type) {
			case MANUAL:
				manualDrive(init, command.a, command.b);
				break;
			case DEAD_RECKONING:
				//manualPD(init, command.a, command.b);
				manualDist(init, command.a, command.b);
				break;
			case FOLLOW_LIGHT:
				followLight(init, command.b);
				break;
			default:
				break;
			}
			init = FALSE;
			calcVel();
	//		clearEncoder();
			vTaskDelayUntil(&wakeTime, MS_TO_TICK(CONTROL_PERIOD));
			wakeTime = xTaskGetTickCount();
		}
	}


	//if we get here then we received an E-Stop command
	//and vsRunControl was set to false
	while(1) {
		xil_printf("#### Received E-STOP! ####\r\n");
		SetServo(RC_STR_SERVO, 0);
		SetServo(RC_VEL_SERVO, 0);
		vsDelay(2000);
	}
}
