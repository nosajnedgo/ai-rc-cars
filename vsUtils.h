/*
 * vsUtils.h
 *
 *  Created on: Feb 11, 2013
 *      Author: ecestudent
 */

#ifndef VSUTILS_H_
#define VSUTILS_H_

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "control.h"

// GENERAL DEBUG: TURN ON (1) and OFF (0) DEBUG MODE
#define VS_DEBUG 0
#define DEBUG(...) do { if (VS_DEBUG) xil_printf( __VA_ARGS__); } while (0)

// TASK DEBUG: WHETHER TO SHOW A SINGLE LETTER FOR EACH TASK RUN
#define VS_DEBUGT 0
#define DEBUGT(...) do { if (VS_DEBUGT) xil_printf( __VA_ARGS__); } while (0)


#define VS_OKAY ( 1 )
#define VS_FAIL ( 0 )
#define VS_TRUE ( 1 )
#define VS_FALSE ( 0 )

#define TARGET_NONE ( 0 )

#define MS_TO_TICK(x) ( x * configTICK_RATE_HZ / 1000 )
#define TICKS_TO_MS(ticks) ( ((ticks)*2) )

//task handles
extern xTaskHandle hbHandle;
extern xTaskHandle commHandle;
extern xTaskHandle visionHandle;
extern xTaskHandle controlHandle;
extern xTaskHandle navHandle;
extern xTaskHandle aiHandle;

extern xSemaphoreHandle vsTxSem;
extern xSemaphoreHandle vsNavStateTransitionSem;



void vsInit();
void vsDelay(int ms);
int vsDebug(const char * message);
int vsTx(const char * message);
void vsHeartbeatReceived();
int vsHeartbeatAck();
void vsControlEStop();
void vsControlAbort();
void vsControlAbortAll();
unsigned vsControlIsAborted();
void vsControlSendCommand(ControlStruct command);
void vsControlSendCommandParam(int type, int a, int b);
ControlStruct vsControlGetCommand(void);
unsigned vsControlGetQSize();
void vsSignalAI();
void vsSignalNav();
int vsPendAI();
int vsPendNav();



#endif /* VSUTILS_H_ */
