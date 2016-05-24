/*
 * vision.h
 *
 *  Created on: Feb 9, 2013
 *      Author: Jason Ogden
 */

#include <stdio.h>
#include <xbasic_types.h>
#include <xintc_l.h>
#include <xexception_l.h>
#include <xutil.h>
#include <xio.h>
#include "plb_vision.h"
#include <xparameters.h>
#include <init.h>
#include <FrameTable.h>
#include <xuartlite_l.h>
#include "ServoControl.h"
#include "HeliosIO.h"
#define PLB_VISION_SELFTEST_BUFSIZE  512 /* Size of buffer (for transfer test) in bytes */

#define TRUE 1
#define FALSE 0

// VISION DEBUG MODE
#define VS_DEBUGV 0
#define DEBUGV(...) do { if (VS_DEBUGV) xil_printf( __VA_ARGS__); } while (0)

#define CALIBRATE_VISION 0
#define SHOW_FPS 0
#define FRAMES_BEFORE_SHOW_FPS 100

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define LOW_RES_STRIDE_X 12
#define LOW_RES_STRIDE_Y 3
#define LOW_RES_START_Y 0
#define LOW_RES_STOP_Y 240
#define HI_RES_STRIDE_X 6
#define HI_RES_STRIDE_Y 2
#define HI_RES_START_Y 20
#define HI_RES_STOP_Y 260
#define MAX_RES_STRIDE_X 1
#define MAX_RES_STRIDE_Y 1
#define MAX_RES_START_Y 100
#define MAX_RES_STOP_Y 340

#define CENTER_OFFSET 390 // Correct the cameras shifted frame issue
#define MIN_PIXEL_COUNT 5 // How many pixels seen constitutes a sighting

typedef struct VisionParametersStruct {
	int sendDebug;
} VisionParamStruct;

extern VisionParamStruct visionParams;

typedef struct VisionDataStruct {
	int target;
	int seesTarget;
	int seesSecondary;
	int pixels; // total number of pixels counted
	int theta; // number of pixels from the frame center to the target center
} VisionStruct;

typedef enum {
	red,
	blue,
	yellow,
	green,
	nullColor
} Color;

typedef struct {
	uint16 high;
	uint16 low;
	uint16 sat;
} Threshold;

// Keep track of the stats for the target and secondary colors
typedef struct TargetDataStruct {
	int pixelCount;
	long int sumX;
	long int sumY;
	int minX;
	int maxX;
	int minY;
	int maxY;
} TargetData;

extern VisionStruct vsVision;
extern Threshold targetThresh;
extern Threshold secondThresh;
extern int visionMode;

// Function Prototypes
void vision(void *pvParameters);
void processFrame(FrameTableEntry* frame);
void setHiResVision(int mode);
void setTargetColors(Color targetColorParam, Color secondaryColorParam, int requireBoth);
void setNewThresholds(Threshold * thisThreshold, Color newColor);
void doDebugCalculations(TargetData * td, int * x, int * y);
void sendTargetDebug(char type, TargetData td);
void showFPS();
