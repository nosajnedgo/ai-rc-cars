/*
 * ai.h
 *
 *  Created on: Feb 9, 2013
 *      Author: Jason Ogden
 */

#ifndef AI_H_
#define AI_H_

#define AI_ALIVE (1)

#define OPPON_COLOR blue
#define BASE_COLOR red
#define FLAG_COLOR yellow
#define GREEN	green

typedef enum {
	huntEnemy,
	huntTruck,
	returnFlag,
	reviveTruck,
	mustKillEnemy,
} GameState;


// AI DEBUG: WHETHER TO AI STATE CHANGES, ETC.
#define VS_DEBUGAI 0
#define DEBUGAI(...) do { if (VS_DEBUGAI) xil_printf( __VA_ARGS__); } while (0)


void ai(void *pvParameters);
int shotHit();
void setGameState(GameState newState);

#endif /* AI_H_ */
