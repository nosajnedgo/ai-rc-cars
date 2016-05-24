/*
 * ai.c
 *
 *  Created on: Feb 9, 2013
 *      Author: Jason Ogden
 */

// RTOS includes
#include "FreeRTOS.h"
#include "Task.h"
#include "semphr.h"

// Other includes
#include "vsUtils.h"
#include "ai.h"
#include "nav.h"
#include "vision.h"
#include "HeliosIO.h"

// Globals
int weHitThem = 0; // Set by HeliosIO.c (use wrapper function shotHit() to test)
GameState gameState;
int seeGreenCounter;


/**
 *  The AI Task
 */
void ai(void *pvParameters) {

	// Set initial game state
	if (CALIBRATE_VISION) {
		setGameState(returnFlag); // Set both colors to avoid nullColor preventing calibration
		while(1)
			vsPendAI();
	}
	else
		setGameState(huntEnemy); // Default state

	while(1) {
		DEBUGT("A>");

		switch (gameState) {

			case huntEnemy:
				if ( vsVision.seesTarget ) {
					if ( Game_Enabled() )
						Game_Shoot(GAME_KILL_SHOT);
				}
				if ( Game_HaveFlag() ){
					setGameState(returnFlag);
					setNavState(uTurn);
				}
				// Have we been shot?
				if ( !Game_Truck_Alive() ){
					setNavState(slowCircle);
					setGameState(reviveTruck);
				}
			break;

			case returnFlag:
				// If we see our base
				if ( vsVision.seesTarget ) {
					// If our base does not have a flag, go hunting
					if ( vsVision.seesSecondary == TRUE ){
						seeGreenCounter++;
						if (seeGreenCounter > 25) {
							setGameState(huntTruck);
							setNavState(uTurn);
						}
					}
					else
						Game_Shoot(GAME_PASS_SHOT);
				}
				// Did we return the flag?
				if ( !Game_HaveFlag() ) {
					setGameState(huntEnemy);
					setNavState(uTurn);
				}
				// Have we been shot?
				if ( !Game_Truck_Alive() ){
					setGameState(reviveTruck);
					setNavState(slowCircle);
				}
			break;

			case huntTruck:
				// If we see the enemy truck, shoot to kill
				if ( vsVision.seesTarget ) {
					if ( Game_Enabled() )
						Game_Shoot(GAME_KILL_SHOT);
				}
				if ( Game_Hit() ) {
					DEBUGAI("HIT!\r\n");
					setGameState(returnFlag);
					setNavState(slowCircle);
				}
				// Have we been shot?
				if ( !Game_Truck_Alive() )
					setGameState(reviveTruck);
			break;

			case reviveTruck:
				// If we see our base, shoot a revive shot
				if ( vsVision.seesTarget ) {
					Game_Shoot(GAME_REVIVE_SHOT);
				}
				if ( Game_Truck_Alive() ){
					setGameState(huntEnemy);
					setNavState(uTurn);
				}
			break;

		}
		vsPendAI();
	}
}


/**
 * Wrapper function to test weHitThem 
 */
int shotHit() {
	if (weHitThem == 1) {
		weHitThem = 0;
		return TRUE;
	}
	else
		return FALSE;
}


/**
 * Wrapper function to set the Game State 
 */
void setGameState(GameState newState) {
	gameState = newState;
	switch(gameState) {
		case huntEnemy:
			DEBUGAI("*AI State: huntEnemy\r\n");
			setTargetColors(OPPON_COLOR, nullColor, FALSE);
			break;
		case returnFlag:
			seeGreenCounter=0;
			DEBUGAI("*AI State: returnFlag\r\n");
			setTargetColors(BASE_COLOR, GREEN, FALSE);
			break;
		case huntTruck:
			DEBUGAI("*AI State: huntTruck\r\n");
			setTargetColors(OPPON_COLOR, FLAG_COLOR, TRUE);
			break;
		case reviveTruck:
			DEBUGAI("*AI State: reviveTruck\r\n");
			setTargetColors(BASE_COLOR, nullColor, FALSE);
			break;
	}
}

