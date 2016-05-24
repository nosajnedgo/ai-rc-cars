/*
 * vsUtils.c
 *
 *  Created on: Feb 11, 2013
 *      Author: ecestudent
 */

/******************************************************************
 * INCLUDES
 *-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-*/

//Xilinx includes
//#include "xutil.h"
#include "stdio.h"
#include "ServoControl.h"

#include "vsUtils.h"
#include "control.h"
#include "vision.h"
#include <stdarg.h>
/*-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-*
 * INCLUDES
 ******************************************************************/

/******************************************************************
 * DEFINES
 *-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-*/
#ifndef debug
//comment this line to suppress debug output
#define debug
#endif
/*-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-*
 * DEFINES
 ******************************************************************/



/******************************************************************
 * GLOBALS
 *-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-\/-*/
xSemaphoreHandle vsTxSem = NULL;
xSemaphoreHandle vsHBSem = NULL;
xSemaphoreHandle vsAISem = NULL;
xSemaphoreHandle vsNavSem = NULL;
xSemaphoreHandle vsNavStateTransitionSem = NULL;
xSemaphoreHandle vsControlAbortSem = NULL;
xSemaphoreHandle vsControlSem = NULL;
//xQueueHandle vsControlQ = NULL;
VisionStruct vsVision;
ControlStruct controlCommand;
//this flag allows the control task to run set it to 0 to E-Stop
int vsRunControl = 1;
/*-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-/\-*
 * GLOBALS
 ******************************************************************/

/****************************************************************
 * void vsInit()
 * Blocking: NO
 * Parameters: none
 * Returns: none
 *
 * This function must be called ONCE before any other function in
 * this file is called. It sets up any semaphores and queues
 * that are used. It is recommended that this function be called
 * in main() before the kernel is started.
 *
 ****************************************************************/
void vsInit() {
	//serial tx semaphore
	//this sem ensures that all packets sent over the serial
	//are contiguous
	vsTxSem = xSemaphoreCreateMutex();
	vsHBSem = xSemaphoreCreateMutex();
	vsAISem = xSemaphoreCreateMutex();
	vsNavSem = xSemaphoreCreateMutex();
	vsNavStateTransitionSem = xSemaphoreCreateMutex();
	vsControlAbortSem = xSemaphoreCreateMutex();
	vsControlSem = xSemaphoreCreateMutex();
//	vsControlQ = xQueueCreate((unsigned portBASE_TYPE) 10,
//							  (unsigned portBASE_TYPE) sizeof(ControlStruct));

	if(vsTxSem == NULL ||
	   vsHBSem == NULL ||
	   vsNavSem == NULL ||
	   vsAISem == NULL) {
		xil_printf("####BAAAAAH!!!!!! SOMETHING WENT WRONG WHILE INITIALIZING OUR RTOS!!!!####\r\n");
		while(1);
	}
}

/****************************************************************
 * void vsDelay()
 * Blocking: YES
 * Parameters: time in ms to delay
 * Returns: none
 *
 * This function delays a task for the specified number of ms.
 * It can only delay for integer values of the RTOS tick
 * period.
 *
 ****************************************************************/
void vsDelay(int ms) {
	vTaskDelay(MS_TO_TICK(ms));

}

/****************************************************************
 * int vsDebug(char * message)
 * Blocking: NO
 * Parameters: Pointer to the message "string"
 * Returns: VS_OKAY if message was sent
 * 			VS_FAIL if resource not available
 *
 * This function should be used for sending non-vital debug info
 * over the wireless uart. It checks to see if the uart is being
 * used for transmission. If it is available, it sends the
 * message. Otherwise it fails. This ensures that this function
 * DOES NOT BLOCK. Use vsTx() if you want a blocking function
 * to transmit a message that is more likely to succeed.
 *
 ****************************************************************/
int vsDebug(const char * message) {
#ifdef debug
	// See if we can obtain the semaphore.  If the semaphore is not available
		// wait 0 ticks to simply poll the semaphore.
		if( vsTxSem != NULL &&
			xSemaphoreTake( vsTxSem, ( portTickType ) 0 ) == pdTRUE ) {
			// We were able to obtain the semaphore and can now access the
			// shared resource.

	//		xil_printf(message);
			xil_printf(message);
			xSemaphoreGive( vsTxSem );
		}
		else {
			return VS_FAIL;
		}
#endif
	return VS_OKAY;
}

/****************************************************************
 * int vsTx(char * message)
 * Blocking: YES
 * Parameters: Pointer to the message "string"
 * Returns: VS_OKAY if message was sent
 * 			VS_FAIL if resource not available
 *
 * This function should be used for sending vital info over
 * the wireless uart. It checks to see if the uart is being
 * used for transmission. If it is available, it sends the
 * message. Otherwise it BLOCKS. This ensures that this function
 * DOES NOT BLOCK. Use vsTX() if you want a blocking function
 * to transmit a message that is more likely to succeed.
 *
 ****************************************************************/
int vsTx(const char * message) {
	// See if we can obtain the semaphore.  If the semaphore is not available
	// wait indefinitely.
	if( vsTxSem != NULL &&
		xSemaphoreTake( vsTxSem, ( portTickType ) portMAX_DELAY ) == pdTRUE ) {
		// We were able to obtain the semaphore and can now access the
		// shared resource.
		//xil_printf(message);
		xil_printf(message);
		xSemaphoreGive( vsTxSem );
	}
	else {
		return VS_FAIL;
	}
	return VS_OKAY;
}


/****************************************************************
 * void vsHeartbeatReceived()
 * Blocking: NO
 * Parameters: None
 * Returns: None
 *
 * This function posts to a semaphore signaling that a heartbeat
 * package was received from the gui.
 * It is called from the comm task.
 *
 ****************************************************************/
void vsSignalAI() {
	xSemaphoreGive( vsAISem );
}

/****************************************************************
 * void vsHeartbeatAck()
 * Blocking: NO
 * Parameters: None
 * Returns: None
 *
 * This function polls a semaphore, receiving the signal that a
 * heartbeat has been received.
 * It is called from the heartbeat task.
 *
 ****************************************************************/
int vsPendAI() {
	if( vsAISem != NULL &&
		xSemaphoreTake( vsAISem, ( portTickType ) portMAX_DELAY ) == pdTRUE ) {
		// We were able to obtain the semaphore and can now access the
		// shared resource.
	}
	else {
		return VS_FAIL;
	}

	return VS_OKAY;
}

/****************************************************************
 * void vsHeartbeatReceived()
 * Blocking: NO
 * Parameters: None
 * Returns: None
 *
 * This function posts to a semaphore signaling that a heartbeat
 * package was received from the gui.
 * It is called from the comm task.
 *
 ****************************************************************/
void vsSignalNav() {
	xSemaphoreGive( vsNavSem );
}

/****************************************************************
 * void vsHeartbeatAck()
 * Blocking: NO
 * Parameters: None
 * Returns: None
 *
 * This function polls a semaphore, receiving the signal that a
 * heartbeat has been received.
 * It is called from the heartbeat task.
 *
 ****************************************************************/
int vsPendNav() {
	if( vsNavSem != NULL &&
		xSemaphoreTake( vsNavSem, ( portTickType ) portMAX_DELAY ) == pdTRUE ) {
		// We were able to obtain the semaphore and can now access the
		// shared resource.
	}
	else {
		return VS_FAIL;
	}

	return VS_OKAY;
}

/****************************************************************
 * void vsHeartbeatReceived()
 * Blocking: NO
 * Parameters: None
 * Returns: None
 *
 * This function posts to a semaphore signaling that a heartbeat
 * package was received from the gui.
 * It is called from the comm task.
 *
 ****************************************************************/
void vsHeartbeatReceived() {
	signed portBASE_TYPE temp;
//	xSemaphoreGive( vsHBSem );
	xSemaphoreGiveFromISR( vsHBSem , &temp);
}

/****************************************************************
 * void vsHeartbeatAck()
 * Blocking: NO
 * Parameters: None
 * Returns: None
 *
 * This function polls a semaphore, receiving the signal that a
 * heartbeat has been received.
 * It is called from the heartbeat task.
 *
 ****************************************************************/
int vsHeartbeatAck() {
	if( vsHBSem != NULL &&
		xSemaphoreTake( vsHBSem, ( portTickType ) MS_TO_TICK(500) ) == pdTRUE ) {
		// We were able to obtain the semaphore and can now access the
		// shared resource.
	}
	else {
		return VS_FAIL;
	}

	return VS_OKAY;
}

/****************************************************************
 * void vsControlEStop()
 * Blocking: NO
 * Parameters: None
 * Returns: None
 *
 * This function sends the E-Stop command to the controller and
 * turns off the servos. However, don't trust this function
 * to ensure servos stay off. The control task needs to obey.
 *
 ****************************************************************/
void vsControlEStop() {
	vsRunControl = 0;
	SetServo(RC_STR_SERVO, 0);
	SetServo(RC_VEL_SERVO, 0);
}

/****************************************************************
 * void vsControlAbort()
 * Blocking: NO
 * Parameters: None
 * Returns: none
 *
 * This function signals the controller to abort.
 *
 ****************************************************************/
void vsControlAbort() {
	xSemaphoreGive(vsControlAbortSem);
}

/****************************************************************
 * void vsControlAbortAll()
 * Blocking: NO
 * Parameters: None
 * Returns: none
 *
 * This function empties the control queue and signals the
 *  controller to abort.
 *
 ****************************************************************/
void vsControlAbortAll() {
//	xQueueReset(vsControlQ);
	xSemaphoreGive(vsControlAbortSem);
}

/****************************************************************
 * void vsControlAbort()
 * Blocking: NO
 * Parameters: None
 * Returns: none
 *
 * This function signals the controller to abort.
 *
 ****************************************************************/
unsigned vsControlIsAborted() {
	//just poll the semaphore
	if( vsControlAbortSem != NULL &&
			xSemaphoreTake( vsControlAbortSem, ( portTickType ) 0 ) == pdTRUE ) {
			// We were able to obtain the semaphore and can now access the
			// shared resource.

		return VS_TRUE;
		}
		else {
			return VS_FALSE;// needs to be VS_TRUE
		}

}

/****************************************************************
 * void vsControlSendCommand()
 * Blocking: NO
 * Parameters: command - pointer to ControlStruct
 * Returns: None
 *
 * This function adds a command to the controller's queue
 *
 ****************************************************************/
void vsControlSendCommand(ControlStruct command) {
	vsControlAbortAll();
	controlCommand.type = command.type;
	controlCommand.a = command.a;
	controlCommand.b = command.b;
	//signal that a command is available
	xSemaphoreGive( vsControlSem );

}

/****************************************************************
 * void vsControlSendCommand()
 * Blocking: NO
 * Parameters: command - pointer to ControlStruct
 * Returns: None
 *
 * This function adds a command to the controller's queue
 *
 ****************************************************************/
void vsControlSendCommandParam(int type, int a, int b) {
	vsControlAbortAll();
	controlCommand.type = type;
	controlCommand.a = a;
	controlCommand.b = b;
	//signal that a command is available
	xSemaphoreGive( vsControlSem );

}

/****************************************************************
 * void vsControlGetCommand()
 * Blocking: YES
 * Parameters: command - pointer to ControlStruct that will be filled
 * Returns: None
 *
 * This function gets a command from the controller's queue
 *
 ****************************************************************/
ControlStruct vsControlGetCommand(void) {
//	xQueueReceive(vsControlQ, command, ( portTickType ) portMAX_DELAY);
	// See if we can obtain the semaphore.  If the semaphore is not available
	// wait indefinitely.
//	if( vsControlSem != NULL &&
		xSemaphoreTake( vsControlSem, ( portTickType ) portMAX_DELAY );// == pdTRUE ) {
		// We were able to obtain the semaphore and can now access the
		// shared resource.
		return controlCommand;
//		xSemaphoreGive( vsControlSem );
//	}
//	else {
//		return NULL;
//	}
//	return NULL;
}

/****************************************************************
 * unsigned vsControlGetQSize()
 * Blocking: NO
 * Parameters: None
 * Returns: Number of messages waiting in the queue
 *
 * This function gets the queue size.
 *
 ****************************************************************/
//unsigned vsControlGetQSize() {
//	return uxQueueMessagesWaiting(vsControlQ);
//}




