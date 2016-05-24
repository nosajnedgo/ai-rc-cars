/***************************** Include Files *******************************/
#include <stdio.h>
#include <xbasic_types.h>
#include <xintc_l.h>
#include <xexception_l.h>
#include <xutil.h>
#include <xio.h>
#include "plb_vision.h"
#include <xparameters.h>
#include <mpmc_calibration.h>
#include <init.h>
#include <FrameTable.h>
#include <xuartlite_l.h>
#include "ServoControl.h"
#include "HeliosIO.h"

#include "control.h"
#include "comm.h"
#include "vision.h"
#include "nav.h"
#include "vsUtils.h"
#include "ai.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

extern int vsRunControl;
extern int useHeartbeat;

//task handles
xTaskHandle hbHandle;
xTaskHandle commHandle;
xTaskHandle visionHandle;
xTaskHandle controlHandle;
xTaskHandle navHandle;
xTaskHandle aiHandle;

int main()
{
	initVision();

	InitGameSystem();
	ServoInit(RC_STR_SERVO);
	ServoInit(RC_VEL_SERVO);
	vsRunControl = 1;
	xil_printf("press any key to continue...\r\n");
	char c;
	read(0,&c,1);
	if(c == '~') {
		useHeartbeat = TRUE;
	} else {
		useHeartbeat = FALSE;
	}

	vsInit();
//	SetServo(RC_STR_SERVO, 10);
	xTaskCreate( heartbeat,
			    ( signed portCHAR * ) "Heartbeat",
			    configMINIMAL_STACK_SIZE,
			    NULL,
			    ( tskIDLE_PRIORITY + 10 ),
			    hbHandle );
	xTaskCreate( comm,
			    ( signed portCHAR * ) "Communications",
			    configMINIMAL_STACK_SIZE,
			    NULL,
			    ( tskIDLE_PRIORITY + 5 ),
			    commHandle );

	xTaskCreate( vision,
			    ( signed portCHAR * ) "Vision",
			    configMINIMAL_STACK_SIZE,
			    NULL,
			    ( tskIDLE_PRIORITY + 6 ),
			    visionHandle );
	xTaskCreate( control,
			    ( signed portCHAR * ) "Control",
			    configMINIMAL_STACK_SIZE,
			    NULL,
			    ( tskIDLE_PRIORITY + 9 ),
			    controlHandle );
	xTaskCreate( nav,
			    ( signed portCHAR * ) "Navigation",
			    configMINIMAL_STACK_SIZE,
			    NULL,
			    ( tskIDLE_PRIORITY + 7 ),
			    navHandle );
	xTaskCreate( ai,
				( signed portCHAR * ) "AI",
				configMINIMAL_STACK_SIZE,
				NULL,
				( tskIDLE_PRIORITY + 8 ),
				aiHandle );


	//This is the start of image processing


//	SetServo(RC_STR_SERVO, 40);
//	SetServo(RC_VEL_SERVO, 8);

	vTaskStartScheduler();
//	loopFrame();

	return 0;
}


