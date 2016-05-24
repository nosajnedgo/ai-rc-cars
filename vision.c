/*
 * vision.h
 *
 *  Created on: Feb 9, 2013
 *      Author: Jason Ogden
 */

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

// VS includes
#include "control.h"
#include "comm.h"
#include "vision.h"
#include "vsUtils.h"
#include "FreeRTOS.h"
#include "task.h"

// Global Variables
VisionParamStruct visionParams;
int strideX = HI_RES_STRIDE_X; // Start in HiRes
int strideY = HI_RES_STRIDE_Y;
int startY = HI_RES_START_Y;
int stopY = HI_RES_STOP_Y;
int requireBothTargets = 0;
int visionMode = 0;
Color targetColor;
Color secondaryColor;
Threshold targetThresh;
Threshold secondThresh;

// Calibrated color thresholds { hueMax, hueMin, satMin }
Threshold redThreshold = { 12, 0, 200 };
Threshold blueThreshold = { 240, 200, 385 };
Threshold yellowThreshold = { 85, 25, 410 };
Threshold greenThreshold = { 150, 110, 400 };
Threshold nullThreshold = { 513, 513, 513 };
/**
 * Vision Task
 */
void vision(void *pvParameters) {
	FrameTableEntry* frame;
	while (1) {
		frame = FT_CheckOutFrame();
		if (frame != NULL) {
			DEBUGT("V>");
			if (SHOW_FPS)
				showFPS();
			processFrame(frame);
			FT_CheckInFrame(frame);
			vsSignalAI();
			vsSignalNav();
			if (CALIBRATE_VISION)
				vTaskDelay(250); // 500 ms
			else
				vTaskDelay(2); // 4 ms
		}
	}
}

/**
 * For each frame, this function iterates though the pixels of the frame.
 */
void processFrame(FrameTableEntry* frame) {
	
	// Initialize the pixel pointers
	u8 * myFrame = frame->frame_address[VISION_FRAME_RGB565]->data.data8;
	uint8* bPixel = myFrame + 0;
	uint8* gPixel = myFrame + 1;
	uint8* rPixel = myFrame + 2;
	
	// Create a struct to hold the current color values (value never seemed to work right)
	struct HSV_STRUCT {
		uint16 hue;
		uint16 sat;
	} c;

	// Counting Variables
	int x, y, position;
	int currentRow = 0;
	TargetData target = {0, 0, 0, FRAME_WIDTH, 0, FRAME_HEIGHT, 0};
	TargetData second = {0, 0, 0, FRAME_WIDTH, 0, FRAME_HEIGHT, 0};

	// Parse each rows
	for (y = startY; y < stopY; y += strideY) {
		currentRow = y * FRAME_WIDTH;

		// Parse each column
		for (x = 0; x < FRAME_WIDTH; x += strideX) {
			position = currentRow + x;

			// Extract the color of the current pixel
			c.hue = (*(rPixel+3*position) << 1) | (*(gPixel+3*position) >> 7);
			c.sat = (*(gPixel+3*position) << 1) | (*(bPixel+3*position) >> 7) & 0x00FF;

			// Check for Target Color
			if ( c.hue < targetThresh.high && c.hue > targetThresh.low  && c.sat > targetThresh.sat) {
				target.pixelCount++;
				target.sumX += x;
				if (visionParams.sendDebug)
					doDebugCalculations(&target, &x, &y);
			}
			// Check for Secondary Color (if one is set)
			else if (secondaryColor != nullColor) {
				if (c.hue < secondThresh.high && c.hue > secondThresh.low && c.sat > secondThresh.sat) {
					second.pixelCount++;
					second.sumX += x;
					if (visionParams.sendDebug)
						doDebugCalculations(&second, &x, &y);
				}
			}
			//Move pointers to next pixel
			bPixel = myFrame + 0;
			gPixel = myFrame + 1;
			rPixel = myFrame + 2;
		}
	}


	// Did we see enough pixels to consider it an official sighting?
	vsVision.seesTarget = (target.pixelCount > MIN_PIXEL_COUNT) ? TRUE : FALSE;
	vsVision.seesSecondary = (second.pixelCount > MIN_PIXEL_COUNT) ? TRUE : FALSE;
	
	// If we need to see both targets to report a sighting
	if (requireBothTargets)
		vsVision.seesTarget = (vsVision.seesTarget && vsVision.seesSecondary) ? TRUE : FALSE;

	// Set the pixel count and angle based on the target color only
	vsVision.pixels = target.pixelCount;
	vsVision.theta = (vsVision.seesTarget) ? -(target.sumX/target.pixelCount - CENTER_OFFSET) : 0;
	
	// If enabled, send the debug summary to the GUI
	if (visionParams.sendDebug) {
		DEBUGV("Should be sending debug.");
		if (vsVision.seesTarget)
			sendTargetDebug('p', target);
		if (vsVision.seesSecondary)
			sendTargetDebug('s', second);
	}

	DEBUGV("targetThresh.low: %d; targetThresh.high: %d\r\n", targetThresh.low, targetThresh.high);
	DEBUGV("Vision saw %d target and %d secondary pixels.\r\n", target.pixelCount, second.pixelCount);

}

/**
 * Sets HI RES mode on or OFF
 */
void setHiResVision(int mode) {
	visionMode = mode;
	switch (visionMode) {
		case 0:
			strideX = LOW_RES_STRIDE_X;
			strideY = LOW_RES_STRIDE_Y;
			startY = LOW_RES_START_Y;
			stopY = LOW_RES_STOP_Y;
			break;
		case 1:
			strideX = HI_RES_STRIDE_X;
			strideY = HI_RES_STRIDE_Y;
			startY = HI_RES_START_Y;
			stopY = HI_RES_STOP_Y;
			break;
		case 2:
			strideX = MAX_RES_STRIDE_X;
			strideY = MAX_RES_STRIDE_Y;
			startY = MAX_RES_START_Y;
			stopY = MAX_RES_STOP_Y;
			break;
		default:
			strideX = LOW_RES_STRIDE_X;
			strideY = LOW_RES_STRIDE_Y;
			startY = LOW_RES_START_Y;
			stopY = LOW_RES_STOP_Y;
	}
}

/**
 * Sets changes to the target colors (set by AI)
 */
void setTargetColors(Color targetColorParam, Color secondaryColorParam, int requireBoth) {
	DEBUGV("Set target colors: %d, %d\r\n", targetColorParam, secondaryColorParam);
	targetColor = targetColorParam;
	secondaryColor = secondaryColorParam;
	requireBothTargets = requireBoth;
	setNewThresholds(&targetThresh, targetColor);
	setNewThresholds(&secondThresh, secondaryColor);
}

/**
 * Change the passed threshold to the passed color. This function makes
 * the setTargetColors function more compact.
 */
void setNewThresholds(Threshold * thisThreshold, Color newColor) {
	switch(newColor) {
		case red: *thisThreshold = redThreshold; break;
		case blue: *thisThreshold = blueThreshold; break;
		case yellow: *thisThreshold = yellowThreshold; break;
		case green: *thisThreshold = greenThreshold; break;
		default: *thisThreshold = nullThreshold; break;
	}
}

/**
 * Save more detailed info for debugging for each pixel found
 */
void doDebugCalculations(TargetData * td, int * x, int * y) {
	// Calculate the sum of Y coordinates
	td->sumY += *y;
	// Determine the min and max pixels
	if (*x < td->minX)
		td->minX = *x;
	if (*x > td->maxX)
		td->maxX = *x;
	if (*y < td->minY)
		td->minY = *y;
	if (*y > td->maxY)
		td->maxY = *y;
}

/**
 * Calculate and send a debug summary of the target for the GUI 
 */
void sendTargetDebug(char type, TargetData td) {
	DEBUGV("Should be sending '%c'.", type);
	int aveX = td.sumX / td.pixelCount;
	int aveY = td.sumY / td.pixelCount;
	xil_printf("<di=%c=%d=%d=%d=%d=%d=%d=%d>",
	  type, td.pixelCount, aveX, aveY, td.minX, td.minY, td.maxX, td.maxY);

}

/**
 * Calculates the frames per second processed by vision
 */
void showFPS() {
	static int frameCount = 0;
	static unsigned startTicks = 0;
	unsigned finishTicks = 0;
	int fps = 0;
	if (frameCount == 0) {
		startTicks = xTaskGetTickCount();
		frameCount++;
	}
	else if (frameCount == FRAMES_BEFORE_SHOW_FPS) {
		finishTicks = xTaskGetTickCount();
		fps = frameCount / (TICKS_TO_MS(finishTicks - startTicks) / 1000);
		xil_printf("FPS: %d\r\n", fps);
		frameCount = 0;
	}
	else
		frameCount++;


}

/***********************************************************************************/
/************************** NON-TASK RELATED CODE BELOW ****************************/
/***********************************************************************************/


/************************** Function Definitions ***************************/
int frameCount = 0;
/* Camera ISR */
typedef Xuint32 CPU_MSR;
#define DISABLE_INTERRUPTS() ({ Xuint32 msr; \
											asm volatile ("mfmsr %0" : "=r" (msr)); \
											asm volatile ("wrteei 0"); \
											msr; });
#define RESTORE_INTERRUPTS(MSR) asm volatile ("wrtee %0" :: "r" (MSR));

#define MEMORY_ADDRESS 0x81d00000//0x007A2558 //XPAR_MPMC_0_MPMC_BASEADDR
#define MEMORY_ADDRESS2 0x00838558
typedef struct {
	Xuint8 magic; //0x00BE
	Xuint8 type;
	Xuint16 checksum_header;
	Xuint16 checksum_data;
	Xuint32 bufferSize;
	Xuint16 packed;
} __attribute__((__packed__)) HeliosCommHeader;

char buffer[10];
unsigned x = 0;
unsigned y = 0;
Xuint8 isPrinted = 0;
Xuint8 ccEn = 0;
Xuint8 isDone;
Xuint8 isNeg;
Xuint32 temp;
Xuint32 fixed;
Xuint32 digit;
Xuint8 ccmCoe;


void CameraISR() {
	CPU_MSR msr = DISABLE_INTERRUPTS();
	frameCount++;
	FT_InterruptHandlerFrameTable();
	XIntc_AckIntr(XPAR_INTC_SINGLE_BASEADDR, "XPAR_PLB_VISION_0_INTERRUPT_MASK");
	RESTORE_INTERRUPTS(msr);
}

void initVision() {
	Init();
//	SetRGB();
	SetHSV();
	XExc_Init();
	XExc_RegisterHandler( XEXC_ID_NON_CRITICAL_INT, (XExceptionHandler)XIntc_DeviceInterruptHandler, (void*)XPAR_XPS_INTC_0_DEVICE_ID);
	XExc_mEnableExceptions(XEXC_NON_CRITICAL);
	XIntc_RegisterHandler( XPAR_XPS_INTC_0_BASEADDR, XPAR_XPS_INTC_0_PLB_VISION_0_INTERRUPT_INTR, (XInterruptHandler)CameraISR, (void*)NULL);
	XIntc_EnableIntr( XPAR_XPS_INTC_0_BASEADDR, XPAR_PLB_VISION_0_INTERRUPT_MASK);
	XIntc_MasterEnable( XPAR_XPS_INTC_0_BASEADDR );
	FT_StartCapture(g_frametable[0]);
	DisableTestPattern(); //Not verified
}
