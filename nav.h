/*
 * nav.h
 *
 *  Created on: Feb 9, 2013
 *      Author: Jason Ogden
 */

#ifndef NAV_H_
#define NAV_H_

#define SLOW_CIRCLE_ANGLE 40 // Turning angle of the slow circle (toggles)
#define BIG_CIRCLE_ANGLE 25 // Turning angle of the maxCircle circle (toggles)
#define FRAMES_TO_REVERSE (225) //this is about 1 meter in reverse (or more)
#define NO_TARGET_FRAMES_LIMIT (200) // how many frames to see no target before a change
#define BACKUP_AFTER_STUCK_FRAMES 100 // how many frames we are stuck before backing up
#define LOST_FRAME_THRESHOLD 30 // how many frames we must miss in a row to lose target
#define MAX_LOST_FRAME_THRESHOLD 3 // for maxRes
#define MAX_TO_HI_PIXEL_THRESH 55 // how many pixels do we need to see to go HighRes //60
#define HI_TO_LOW_PIXEL_THRESH 45 // how many pixels do we need to see to go LowRes
#define TOO_CLOSE_THRESH 14 // how man pixels we deem as too close (LowRes)
#define TOO_CLOSE_FRAME_LIMIT 8 // How many frames of too close we need before backup
#define TOO_CLOSE_BACKUP_FRAMES 100 // How many frames we should backup

#define VS_DEBUGNAV 0
#define DEBUGNAV(...) do { if (VS_DEBUGNAV) xil_printf( __VA_ARGS__); } while (0)

typedef enum {
	straight,
	zig,
	zag,
	maxFollowLight,
	highFollowLight,
	lowFollowLight,
	uTurn,
	tooClose,
	stuck,
	slowCircle,
	maxCircle,
} NavState;

void setNavState(NavState newState);
void nav( void *pvParameters );
unsigned isTruckNotMoving();
void tellNavAISawTarget();

#endif /* NAV_H_ */
