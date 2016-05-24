/*
 * nav.c
 *
 *  Created on: Feb 9, 2013
 *      Author: Jason Ogden
 */

#include "nav.h"
#include "vision.h"
#include "control.h"
#include "ai.h"
#include "HeliosIO.h"
#include "vsUtils.h"
#include "FreeRTOS.h"


NavState navState, lastNavState;
unsigned tooCloseStateCounter = 0;
unsigned tooCloseFrameCount = 0;
int zigToggle = 1;

/**
 * This is the navigation task function.
 */
void nav( void *pvParameters ) {
	ControlStruct command;
    unsigned searchingFrameCount = 0; // How many frames we've process and not seen a target
    unsigned missedFrameCount = 0;

	if (CALIBRATE_VISION) {
		setHiResVision(1);
		while(1)
			vsPendNav();
	}

	setNavState(uTurn);

	while(1) {
		// Wait for a frame to be processed (AI runs first)
		vsPendNav();

		//Below is new state machine for nav
		switch (navState) {
			case straight:
				if (vsVision.seesTarget) {
					setNavState(highFollowLight);
					break;
				}
				if ( isTruckNotMoving() ) {
					setNavState(stuck);
					break;
				}
				if (deadReckoningComplete) {
					deadReckoningComplete = FALSE;
					setNavState(zig);
				}
				break;

			case zig:
				if (vsVision.seesTarget) {
					setNavState(highFollowLight);
					break;
				}
				if ( isTruckNotMoving() ) {
					setNavState(stuck);
					break;
				}
				if (deadReckoningComplete) {
					deadReckoningComplete = FALSE;
					setNavState(slowCircle);
				}
				break;

			case zag:
				if (vsVision.seesTarget) {
					setNavState(highFollowLight);
					break;
				}
				if ( isTruckNotMoving() ) {
					setNavState(stuck);
					break;
				}
				if (deadReckoningComplete) {
					deadReckoningComplete = FALSE;
					setNavState(slowCircle);
				}
				break;

			case maxFollowLight:
				if ( isTruckNotMoving() ) {
					setNavState(stuck);
					break;
				}
				// If we are close enough, switch to low res
				if (vsVision.pixels > MAX_TO_HI_PIXEL_THRESH) {
					setNavState(highFollowLight);
					break;
				}

				// Check for contiguous missed frames
				if (!vsVision.seesTarget)
					missedFrameCount++;
				else
					missedFrameCount = 0;

				if (missedFrameCount > MAX_LOST_FRAME_THRESHOLD) {
					setNavState(maxCircle);
					break;
				}
				vsControlSendCommandParam(FOLLOW_LIGHT, 0, 800);
				break;

			case highFollowLight:
				if ( isTruckNotMoving() ) {
					setNavState(stuck);
					break;
				}
				// If we are close enough, switch to low res
				if (vsVision.pixels > HI_TO_LOW_PIXEL_THRESH) {
					setNavState(lowFollowLight);
					break;
				}

				// Check for contiguous missed frames
				if (!vsVision.seesTarget)
					missedFrameCount++;
				else
					missedFrameCount = 0;

				if (missedFrameCount > LOST_FRAME_THRESHOLD) {
					setNavState(slowCircle);
					break;
				}
				vsControlSendCommandParam(FOLLOW_LIGHT, 0, 1500);
				break;

			case lowFollowLight:
				if ( isTruckNotMoving() ) {
					setNavState(stuck);
					break;
				}
				// Check for contiguous missed frames
				if (!vsVision.seesTarget)
					missedFrameCount++;
				else
					missedFrameCount = 0;

				if (missedFrameCount > LOST_FRAME_THRESHOLD) {
					setNavState(slowCircle);
					break;
				}

				// Check for contiguous missed frames
				if (vsVision.pixels > TOO_CLOSE_THRESH)
					tooCloseFrameCount++;

				if (tooCloseFrameCount > TOO_CLOSE_FRAME_LIMIT) {
					setNavState(tooClose);
					break;
				}

				vsControlSendCommandParam(FOLLOW_LIGHT, 0, 1500);
				break;

			case tooClose:
				//DEBUGNAV("- Nav State: Too Close: %d\r\n",GetServo(RC_VEL_SERVO));
				if (deadReckoningComplete) {
					deadReckoningComplete = FALSE;
					setNavState(lowFollowLight);
				}
				break;

			case stuck:
				if ( !isTruckNotMoving() ) {
					if (lastNavState == maxCircle)
						setNavState(maxCircle);
					else
						setNavState(slowCircle);
				}
				else
					vsControlSendCommandParam(MANUAL, 0, -4000);
				break;

			case maxCircle:
				if (vsVision.seesTarget) {
					searchingFrameCount = 0;
					setNavState(maxFollowLight);
					break;
				}
				if ( isTruckNotMoving() ) {
					searchingFrameCount = 0;
					setNavState(stuck);
					break;
				}
				vsControlSendCommandParam(MANUAL, zigToggle*BIG_CIRCLE_ANGLE, 800);
				break;

			case slowCircle:
				if (vsVision.seesTarget) {
					searchingFrameCount = 0;
					setNavState(highFollowLight);
					break;
				}
				if ( isTruckNotMoving() ) {
					searchingFrameCount = 0;
					setNavState(stuck);
					break;
				}
				if (searchingFrameCount++ > NO_TARGET_FRAMES_LIMIT) {
					searchingFrameCount = 0;
					setNavState(maxCircle);
					break;
				}
				vsControlSendCommandParam(MANUAL, zigToggle*SLOW_CIRCLE_ANGLE, 950);
				break;

			case uTurn:
				if ( isTruckNotMoving() ) {
					setNavState(stuck);
					break;
				}
				if (deadReckoningComplete) {
					deadReckoningComplete = FALSE;
					setNavState(straight);
				}
				break;

		}


	}

}


/**
 * Wrapper function to set the Game State. Set anything that only
 * needs to run on the state transition in here.
 */
void setNavState(NavState newState) {
	xSemaphoreTake( vsNavStateTransitionSem, ( portTickType ) portMAX_DELAY );
	lastNavState = navState;
	navState = newState;
	deadReckoningComplete = FALSE;
	switch(navState) {
		case straight:
			setHiResVision(1);
			vsControlSendCommandParam(DEAD_RECKONING, 0, 2000);
			DEBUGNAV("- Nav: straight\r\n");
			break;
		case zig:
			vsControlSendCommandParam(DEAD_RECKONING, -10, 3000); // right
			DEBUGNAV("- Nav: zig\r\n");
			break;
		case zag:
			vsControlSendCommandParam(DEAD_RECKONING, zigToggle*15, 3000); // left
			DEBUGNAV("- Nav: zag\r\n");
			break;
		case maxFollowLight:
			setHiResVision(2);
			DEBUGNAV("- Nav: maxFollowLight\r\n");
			break;
		case highFollowLight:
			setHiResVision(1);
			DEBUGNAV("- Nav: highFollowLight\r\n");
			break;
		case lowFollowLight:
			tooCloseFrameCount = 0;
			setHiResVision(0);
			DEBUGNAV("- Nav: lowFollowLight\r\n");
			break;
		case uTurn:
			vsControlSendCommandParam(DEAD_RECKONING, -50, 3700); //was 3700
			zigToggle *= -1;
			DEBUGNAV("- Nav: uTurn\r\n");
			break;
		case tooClose:
			vsControlSendCommandParam(DEAD_RECKONING, 0, -1000);
			DEBUGNAV("- Nav: tooClose\r\n");
			break;
		case stuck:
			DEBUGNAV("- Nav: stuck\r\n");
			vsControlAbortAll();
			break;
		case slowCircle:
			setHiResVision(1);
			DEBUGNAV("- Nav: slowCircle\r\n");
			break;
		case maxCircle:
			setHiResVision(2);
			DEBUGNAV("- Nav: maxCircle\r\n");
			break;
	}

	xSemaphoreGive( vsNavStateTransitionSem );
}



/**
 * Performs a check to see if we have stopped moving forward (hit an obstacle)
 * for long enough that we need to reverse.
 */
unsigned isTruckNotMoving() {

	static unsigned previousTickReading;
	static unsigned numberOfTicksToBackup; // this determines how many ticks to back up
	static unsigned framesUnchanged;

	unsigned currentTickReading;
	unsigned isTruckStuck = FALSE;

	currentTickReading = getTicks();

	// If we are currently backing up
	if (numberOfTicksToBackup > 0) {
		// If we have still have more backing to do
		if (currentTickReading > numberOfTicksToBackup) // ticks decrement on reverse
			isTruckStuck = TRUE;
		// Otherwise, we are done backing up
		else
			numberOfTicksToBackup = 0;
	}
	// Otherwise, check to see if we are stuck
	else {
		// If we are stuck at only 20 ticks apart
		if ((previousTickReading + 20) > currentTickReading)
			framesUnchanged++;
		else
			framesUnchanged = 0;
			
		//Reverses if we are 20 ticks apart for BACKUP_AFTER_TICKS calls.
		if (framesUnchanged > (BACKUP_AFTER_STUCK_FRAMES / (visionMode*2 + 1))) {
			numberOfTicksToBackup = currentTickReading - 1000; // Back up for 1000 ticks. ( 0.5 meter)
			framesUnchanged = 0;
		}
	}
	
	previousTickReading = currentTickReading;
	return isTruckStuck;
}
