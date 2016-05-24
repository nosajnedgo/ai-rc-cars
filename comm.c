/*
 * comm.c
 *
 *  Created on: Feb 9, 2013
 *      Author: ecestudent
 */
//RTOS specific
#include "FreeRTOS.h"

//Xilinx
//#include "xutil.h"
//#include "stdio.h"
//#include <xintc_l.h>
//#include "xparameters.h"
//#include "xcache_l.h"
#include "HeliosIO.h"

#include "comm.h"
#include "vsUtils.h"
#include "vision.h"

int useHeartbeat = 1;

// a task to show the car is alive
void heartbeat( void *pvParameters ){
	//do some setup
	//if watchdog ever reaches 2, we lost connection
	//int watchdog = 0;
	while(1) {
		DEBUGT("H>");
		//loop forever
		//send the heartbeat packet
		vsTx("\r\n<cha>");

		//wait 100ms for the response
		if(vsHeartbeatAck() != VS_OKAY) {
//			xil_printf("Oh No! We lost connection!!!\r\n");
			if(useHeartbeat == TRUE)
				vsControlEStop();
		} else {

//			vsDebug("Received heartbeat response\r\n");
		}

		//delay to send the next heartbeat
		vsDelay(2000);
	}
}

void parseCommand() {
	int code = 0;
	char c = 0;
	read(0,&c,1);
	switch(c) {
	case 'h':	//heartbeat command packet
		read(0,&c,1);
		switch(c) {
		case 'a':	//acknowledge
			read(0,&c,1);
			if(c == '>') {
				vsHeartbeatReceived();
			}
			break;
		default:
			break;
		}
		break;
	case 'p':	//parameter command packet
		//need to read the parameter code
		read(0,&c,1);
		code = 0;
		while(c >= '0' && c <= '9') {
			code = 10*code + (c - '0');
			read(0,&c,1);
		}
		switch(code) {
		case 1:	//debug frame

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					visionParams.sendDebug = code;
					xil_printf("send frames=%d\r\n",visionParams.sendDebug);
				}
				else	//malformed packet
					return;
			}
			break;
		case 2:	//targetColorThresholds.high

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					targetThresh.high = code;
					xil_printf("targetThresh.high=%d\r\n",targetThresh.high);
				}
				else	//malformed packet
					return;
			}
			break;
		case 3:	//targetColorThresholds.low

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					targetThresh.low = code;
					xil_printf("targetThresh.low=%d\r\n",targetThresh.low);
				}
				else	//malformed packet
					return;
			}
			break;
		case 4:	//targetColorThresholds.sat

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					targetThresh.sat = code;
					xil_printf("targetThresh.sat=%d\r\n",targetThresh.sat);
				}
				else	//malformed packet
					return;
			}
			break;
		case 5:	//secondaryTargetColorThresholds.high

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					secondThresh.high = code;
					xil_printf("secondThresh.high=%d\r\n",secondThresh.high);
				}
				else	//malformed packet
					return;
			}
			break;
		case 6:	//secondaryTargetColorThresholds.low

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					secondThresh.low = code;
					xil_printf("secondThresh.low=%d\r\n",secondThresh.low);
				}
				else	//malformed packet
					return;
			}
			break;
		case 7:	//secondaryTargetColorThresholds.sat

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					secondThresh.sat = code;
					xil_printf("secondThresh.sat=%d\r\n",secondThresh.sat);
				}
				else	//malformed packet
					return;
			}
			break;
		case 8:	// Set the HiRes Vision on or off

			if(c == '=') {
				read(0,&c,1);
				code = 0;
				while(c >= '0' && c <= '9') {
					code = 10*code + (c - '0');
					read(0,&c,1);
				}
				if(c == '>') {
					setHiResVision(code);
					xil_printf("setHiResVision: %d\r\n", code);
				}
				else	//malformed packet
					return;
			}
			break;

		default:
			break;
		}
		break;
	case 'c':	//control command packet
		//need to read the command code
		read(0,&c,1);
		int code = 0;
		while(c >= '0' && c <= '9') {
			code = 10*code + (c - '0');
			read(0,&c,1);
		}
//		xil_printf("Code=%d\r\n",code);
		switch(code) {
		case 4:	//E-Stop
			if(c == '>') {
				vsControlEStop();
			}
			else	//malformed packet
				return;
			break;
		case 8:	//Suspend Nav
			if(c == '>') {
				vTaskSuspend(navHandle);
				xil_printf("#####################################################Suspending Nav");
			}
			else	//malformed packet
				return;
			break;
		case 9:	//Resume Nav
			if(c == '>') {
				vTaskResume(navHandle);
			}
			else	//malformed packet
				return;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}


}

void parseQuery() {

}


// a task to handle communication
void comm( void *pvParameters ){
	//do some setup
	char c = 0;
	while(1) {
		DEBUGT("X>");
		//loop forever
//		DEBUG("Debug: Comm is working too.\r\n");

//		DEBUG("\r\nEnter new command:\r\n");
		read(0,&c,1);
//		DEBUG("%c",c);
		if(c != '<') {
			if(c == '#')
				Game_Shoot(GAME_KILL_SHOT);
			continue;
		}
		read(0,&c,1);
		if(c == 'c') {
			parseCommand();
		}
		else if(c == 'q') {
			parseQuery();
		}
		else {	//malformed command
			continue;
		}

	}
}
