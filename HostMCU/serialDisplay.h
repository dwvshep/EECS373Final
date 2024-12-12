/*
 * serialDisplay.h
 *
 *  Created on: Nov 15, 2024
 *      Author: enriqueorozc
 */

#ifndef INC_SERIALDISPLAY_H_
#define INC_SERIALDISPLAY_H_

// Includes:
#include "stm32l4xx_hal.h"

// Alias for UART:
typedef UART_HandleTypeDef UART;

// SongPacket Structure:
typedef struct {

	// Name Variables:
	uint8_t *songName;
	uint8_t *artistName;
	uint8_t *genre;
	int artistLength;
	int nameLength;

	// Display Variables:
	int inactivePrint;
	int activePrint;

	// Time Variables (ms):
	int duration;
	int progress;
	int paused;

	// Packet Validity:
	int activeSong;

} SongPacket;

// Power Functions:
void turnDisplayOff(UART *huart);
void turnDisplayOn(UART *huart);

// Display Hardware Information:
void displayBaud(UART *huart);
void displayFirmware(UART *huart);

// Clear Display:
void clearDisplay(UART *huart);

// Change Baud Rate:
void changeDisplayBaud(UART *huart, uint8_t baudType);
void changeUARTBaud(UART *huart, uint8_t baudType);

// Cursor Position:
void setCursorPos(UART *huart, uint8_t pos);

// Song Time Functions:
void displayTime(UART *huart, SongPacket *song);
void updateTime(SongPacket *song);

// Song Bar Progress:
void updateProgress(UART *huart, SongPacket *song);

// Initial Song Display:
void displaySong(UART *huart, SongPacket *song);

// Display Song Artist:
void displayArtist(UART *huart, SongPacket *song);

// Display Song Title:
void displayTitle(UART *huart, SongPacket *song);


#endif /* INC_SERIALDISPLAY_H_ */
