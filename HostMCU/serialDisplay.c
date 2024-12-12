/*
 * serialDisplay.c
 *
 *  Created on: Nov 15, 2024
 *      Author: enriqueorozc
 */

// Includes:
#include "serialDisplay.h"
#include <string.h>
#include <stdio.h>

// Power Functions:
// -----------------------------------------------------------------------
void turnDisplayOff(UART *huart) {

	// Create Array:
	uint8_t offCommands[] = {0xFE, 0x42};

	// Send UART:
	HAL_UART_Transmit(huart, offCommands, 2, 10);

	// Delay:
	HAL_Delay(5);

}

void turnDisplayOn(UART *huart) {

	// Create Array:
	uint8_t onCommands[] = {0xFE, 0x41};

	// Send UART:
	HAL_UART_Transmit(huart, onCommands, 2, 10);

	// Delay:
	HAL_Delay(5);

}

// Hardware Information Functions:
// -----------------------------------------------------------------------
void displayBaud(UART *huart) {

	// Create Array:
	uint8_t displayBaud[] = {0xFE, 0x71};

	// Send UART:
	HAL_UART_Transmit(huart, displayBaud, 2, 15);

	// Delay:
	HAL_Delay(15);

}

void displayFirmware(UART *huart) {

	// Create Array:
	uint8_t displayFirmware[] = {0xFE, 0x70};

	// Send UART:
  HAL_UART_Transmit(huart, displayFirmware, 2, 10);

  // Delay:
  HAL_Delay(5);

}

// Clear Display:
// -----------------------------------------------------------------------
void clearDisplay(UART *huart) {

	// Create Array:
	uint8_t clearCommands[] = {0xFE, 0x51};

	// Send UART:
	HAL_UART_Transmit(huart, clearCommands, 2, 15);

}

// Change Baud Rate:
// 1 = 300 | 2 = 1200 | 3 = 2400 | 4 = 9600
// 5 = 14400 | 6 = 19.2K | 7 = 57.6K | 8 = 115.2K
// -----------------------------------------------------------------------
void changeUARTBaud(UART *huart, uint8_t baudType) {

	// Stop UART:
	HAL_UART_DeInit(huart);

	// Update Baud:
	switch(baudType) {

		case 0x01:
			huart->Init.BaudRate = 300;
			break;

		case 0x02:
			huart->Init.BaudRate = 1200;
			break;

		case 0x03:
			huart->Init.BaudRate = 2400;
			break;

		case 0x04:
			huart->Init.BaudRate = 9600;
			break;

		case 0x05:
			huart->Init.BaudRate = 14400;
			break;

		case 0x06:
			huart->Init.BaudRate = 19200;
			break;

		case 0x07:
			huart->Init.BaudRate = 57600;
			break;

		default:
			huart->Init.BaudRate = 115200;
			break;

	}

	// Start UART:
	HAL_UART_Init(huart);

}

void changeDisplayBaud(UART *huart, uint8_t baudType) {

	// Error Check:
	if (baudType < 1 || baudType > 8) {
		return;
	}

	// Create Array:
	uint8_t changeBaud[] = {0xFE, 0x61, baudType};

	// Send UART:
	HAL_UART_Transmit(huart, changeBaud, 3, 15);

	// Delay:
	HAL_Delay(5);

	// Change UART Setting:
	changeUARTBaud(huart, baudType);

}

// Cursor Position:
// pos = 0x00 <-> 0x67
// Line 1: 0x00 <-> 0x13
// Line 2: 0x40 <-> 0x53
// Line 3: 0x14 <-> 0x27
// Line 4: 0x54 <-> 0x67
// -----------------------------------------------------------------------
void setCursorPos(UART *huart, uint8_t pos) {

	// Error Check:
	if (pos > 0x67) {
		return;
	}

	// Create Array:
	uint8_t changeCursor[] = {0xFE, 0x45, pos};

	// Send UART:
	HAL_UART_Transmit(huart, changeCursor, 3, 10);

}

// Display Song Time Progress:
void displayTime(UART *huart, SongPacket *song) {

	char currentTime[10];
	char totalTime[10];

	// Convert Time Variables:
	int totalMinutes = song->duration / 60000;
	int totalSeconds = (song->duration % 60000) / 1000;

	int currentMinutes = song->progress / 60000;
	int currentSeconds = (song->progress % 60000) / 1000;

	// Split into Longer than 10 minutes, vs. shorter than
	if (totalMinutes >= 10) {
		sprintf(currentTime, "%2d:%02d", currentMinutes, currentSeconds);
		sprintf(totalTime, "%2d:%02d", totalMinutes, totalSeconds);
	} else {
		sprintf(currentTime, "%d:%02d", currentMinutes, currentSeconds);
		sprintf(totalTime, "%d:%02d", totalMinutes, totalSeconds);
	}

	char output[20];
	snprintf(output, sizeof(output), "%s / %s", currentTime, totalTime);

	// Calculate Starting Position:
	size_t len = strlen(output);
	int startCol = (20 - len) / 2;

	// Change Cursor:
	uint8_t cursorPos = 0x14 + startCol;
	setCursorPos(huart, cursorPos);

	// Transmit:
	HAL_UART_Transmit(huart, (uint8_t*)output, len, 100);

}

// Update Song Time:
void updateTime(SongPacket *song) {

	// Update Song Time by One Second:
	song->progress += 1000;

	// Ensure Progress Doesn't Exceed Duration:
	if (song->progress >= song->duration) {
		song->progress = song->duration;
	}

}

// Song Bar Progress:
void updateProgress(UART *huart, SongPacket *song) {

	// Determine Amount of Columns:
	int columnDuration = song->duration / 18;
	int filledColumns = song->progress / columnDuration;

	// Construct the Bar:
	char progressBar[18 + 1];

	for (int i = 0; i < 18; i++) {
		if (i < filledColumns) {
			progressBar[i] = 0xFF;
		} else {
			progressBar[i] = 0x20;
		}
	}

	progressBar[18] = '\0';

	// Cursor Position:
	uint8_t position = 0x54 + 1;
	setCursorPos(huart, position);

	// Transmit:
	HAL_UART_Transmit(huart, (uint8_t*)progressBar, 18, 100);

	// Check if Song is Done:
	if (song->progress >= song->duration) {
		song->activeSong = 0;
	}

}

void displaySong(UART *huart, SongPacket *song) {

	// No Active Song:
	if (!song->activeSong && !song->inactivePrint) {

		// No Song String:
		char noSong_1[] = "No Active Song";
		char noSong_2[] = "Playing";

		// Center Strings:
		int startCol_1 = (20 - strlen(noSong_1)) / 2;
		int startCol_2 = (20 - strlen(noSong_2)) / 2;

		uint8_t cursorPos_1 = 0x40 + startCol_1;
		uint8_t cursorPos_2 = 0x14 + startCol_2;

		// Print Strings:
		setCursorPos(huart, cursorPos_1);
		HAL_UART_Transmit(huart, (uint8_t*)noSong_1, strlen(noSong_1), 100);

		setCursorPos(huart, cursorPos_2);
		HAL_UART_Transmit(huart, (uint8_t*)noSong_2, strlen(noSong_2), 100);

		// Update Print Status:
		song->inactivePrint = 1;

	}

	// Initial Print of Song:
	else if (song->activeSong && !song->activePrint) {

		// Clear Screen:
		clearDisplay(huart);

		// Print Name:
		displayArtist(huart, song);
		song->activePrint = 1;

	}


}

// Displays the Artist Name (Truncates if > 20 chars):
void displayArtist(UART *huart, SongPacket *song) {

	// Decide Whether to Truncate:
	if (song->artistLength > 20) {
		song->artistName[18] = '.';
		song->artistName[19] = '.';
		song->artistLength = 20;
	}

	// Calculate Starting Position:
	int startCol = (20 - song->artistLength) / 2;
	uint8_t cursorPos = 0x40 + startCol;

	// Move Cursor Position:
	setCursorPos(huart, cursorPos);

	// Transmit:
	HAL_UART_Transmit(huart, song->artistName, song->artistLength, 100);

}



//// Power Functions:
//// -----------------------------------------------------------------------
//void turnDisplayOff(UART *huart) {
//
//	// Create Array:
//	uint8_t offCommands[] = {0xFE, 0x42};
//
//	// Send UART:
//	HAL_UART_Transmit(huart, offCommands, 2, 10);
//
//	// Delay:
//	HAL_Delay(5);
//
//}
//
//void turnDisplayOn(UART *huart) {
//
//	// Create Array:
//	uint8_t onCommands[] = {0xFE, 0x41};
//
//	// Send UART:
//	HAL_UART_Transmit(huart, onCommands, 2, 10);
//
//	// Delay:
//	HAL_Delay(5);
//
//}
//
//// Hardware Information Functions:
//// -----------------------------------------------------------------------
//void displayBaud(UART *huart) {
//
//	// Create Array:
//	uint8_t displayBaud[] = {0xFE, 0x71};
//
//	// Send UART:
//	HAL_UART_Transmit(huart, displayBaud, 2, 15);
//
//	// Delay:
//	HAL_Delay(15);
//
//}
//
//void displayFirmware(UART *huart) {
//
//	// Create Array:
//	uint8_t displayFirmware[] = {0xFE, 0x70};
//
//	// Send UART:
//  HAL_UART_Transmit(huart, displayFirmware, 2, 10);
//
//  // Delay:
//  HAL_Delay(5);
//
//}
//
//// Clear Display:
//// -----------------------------------------------------------------------
//void clearDisplay(UART *huart) {
//
//	// Create Array:
//	uint8_t clearCommands[] = {0xFE, 0x51};
//
//	// Send UART:
//	HAL_UART_Transmit(huart, clearCommands, 2, 15);
//
//	// Delay:
//	HAL_Delay(5);
//
//}
//
//// Change Baud Rate:
//// 1 = 300 | 2 = 1200 | 3 = 2400 | 4 = 9600
//// 5 = 14400 | 6 = 19.2K | 7 = 57.6K | 8 = 115.2K
//// -----------------------------------------------------------------------
//void changeUARTBaud(UART *huart, uint8_t baudType) {
//
//	// Stop UART:
//	HAL_UART_DeInit(&huart);
//
//	// Update Baud:
//	switch(baudType) {
//
//		case 0x01:
//			huart->Init.BaudRate = 300;
//			break;
//
//		case 0x02:
//			huart->Init.BaudRate = 1200;
//			break;
//
//		case 0x03:
//			huart->Init.BaudRate = 2400;
//			break;
//
//		case 0x04:
//			huart->Init.BaudRate = 9600;
//			break;
//
//		case 0x05:
//			huart->Init.BaudRate = 14400;
//			break;
//
//		case 0x06:
//			huart->Init.BaudRate = 19200;
//			break;
//
//		case 0x07:
//			huart->Init.BaudRate = 57600;
//			break;
//
//		default:
//			huart->Init.BaudRate = 115200;
//			break;
//
//	}
//
//	// Start UART:
//	HAL_UART_Init(huart);
//
//}
//
//void changeDisplayBaud(UART *huart, uint8_t baudType) {
//
//	// Error Check:
//	if (baudType < 1 || baudType > 8) {
//		return;
//	}
//
//	// Create Array:
//	uint8_t changeBaud[] = {0xFE, 0x61, baudType};
//
//	// Send UART:
//	HAL_UART_Transmit(huart, changeBaud, 3, 15);
//
//	// Delay:
//	HAL_Delay(5);
//
//	// Change UART Setting:
//	changeUARTBaud(huart, baudType);
//
//}
//
//// Cursor Position:
//// pos = 0x00 <-> 0x67
//// Line 1: 0x00 <-> 0x13
//// Line 2: 0x40 <-> 0x53
//// Line 3: 0x14 <-> 0x27
//// Line 4: 0x54 <-> 0x67
//// -----------------------------------------------------------------------
//void setCursorPos(UART *huart, uint8_t pos) {
//
//	// Error Check:
//	if (pos > 0x67) {
//		return;
//	}
//
//	// Create Array:
//	uint8_t changeCursor[] = {0xFE, 0x45, pos};
//
//	// Send UART:
//	HAL_UART_Transmit(huart, changeCursor, 3, 10);
//
//	// Delay:
//	HAL_Delay(5);
//
//}
