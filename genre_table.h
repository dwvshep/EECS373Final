/*
 * genre_table.h
 *
 *  Created on: Nov 26, 2024
 *      Author: zender
 */

#include "dance_library.h"
#include <stdio.h>
#include <string.h>

#ifndef INC_GENRE_TABLE_H_
#define INC_GENRE_TABLE_H_

struct table_entry genre_table[4];

void create_table() {
	// Christmas
	struct table_entry christmas_entry;
	strcpy(christmas_entry.genre, "C");
	christmas_entry.fun_ptr = &straight_forward;
	genre_table[0] = christmas_entry;

	// Groovy
	struct table_entry groovy_entry;
	strcpy(groovy_entry.genre, "G");
	groovy_entry.fun_ptr = &straight_backward;
	genre_table[1] = groovy_entry;

	// Jazz
	struct table_entry jazz_entry;
	strcpy(jazz_entry.genre, "J");
	jazz_entry.fun_ptr = &spin_cw;
	genre_table[2] = jazz_entry;

	// Rock
	struct table_entry rock_entry;
	strcpy(rock_entry.genre, "R");
	rock_entry.fun_ptr = &spin_ccw;
	genre_table[3] = rock_entry;
}

#endif /* INC_GENRE_TABLE_H_ */
