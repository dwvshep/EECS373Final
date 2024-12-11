/*
 * dance_library.h
 *
 *  Created on: Nov 18, 2024
 *      Author: zender
 */

#include <stdio.h>
#include <string.h>

#ifndef INC_DANCE_LIBRARY_H_
#define INC_DANCE_LIBRARY_H_

#define TIM3_ADDR 0x40000400 //timer 3 base register
#define TIM4_ADDR 0x40000800 //timer 4 base register
#define TIM_CCR1_OFFSET 0x34 //capture/compare register 1
#define TIM_CCR2_OFFSET 0x38 //capture/compare register 2
#define CCR_MASK 0xFFFF

uint32_t * tim3_ccr2 = (uint32_t *)(TIM3_ADDR + TIM_CCR2_OFFSET); // pointer to right motor CCR
uint32_t * tim4_ccr1 = (uint32_t *)(TIM4_ADDR + TIM_CCR1_OFFSET); // pointer to left motor CCR

int bool_stop = 0;

struct table_entry {
	char genre[50];
	void (*fun_ptr)(int);
};

/*
 * Stops Zumo by setting motor speeds to zero
 */
void stop(){
	  *tim3_ccr2 &= ~CCR_MASK;
	  *tim3_ccr2 |= 0; // sets right motor speed
	  *tim4_ccr1 &= ~CCR_MASK;
	  *tim4_ccr1 |= 0; // sets left motor speed
}

/*
 * Moves Zumo forward in a straight line
 * Arguments: speed - Determines the linear velocity of the Zumo
 */
void straight_forward(int speed) {
  *tim3_ccr2 &= ~CCR_MASK;
  *tim3_ccr2 |= speed; // sets right motor speed
  *tim4_ccr1 &= ~CCR_MASK;
  *tim4_ccr1 |= speed; // sets left motor speed

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0); // sets right motor direction
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0); // sets left motor direction

  HAL_Delay(1000);
  stop();
}

/*
 * Moves Zumo backward in a straight line
 * Arguments: speed - Determines the linear velocity of the Zumo
 */
void straight_backward(int speed) {
  *tim3_ccr2 &= ~CCR_MASK;
  *tim3_ccr2 |= speed; // sets right motor speed
  *tim4_ccr1 &= ~CCR_MASK;
  *tim4_ccr1 |= speed; // sets left motor speed

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1); // sets right motor direction
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1); // sets left motor direction

  HAL_Delay(1000);
  stop();
}

/*
 * Spins Zumo in place clockwise
 * Arguments: speed - Determines the angular velocity of the Zumo
 */
void spin_cw(int speed) {
  *tim3_ccr2 &= ~CCR_MASK;
  *tim3_ccr2 |= speed; // sets right motor speed
  *tim4_ccr1 &= ~CCR_MASK;
  *tim4_ccr1 |= speed; // sets left motor speed

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1); // sets right motor direction
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0); // sets left motor direction

  HAL_Delay(2000);
  stop();
}

void spin_cw_angle(int w, float angle) {
  int speed = (int)(23 + (w * 15));
  *tim3_ccr2 &= ~CCR_MASK;
  *tim3_ccr2 |= speed; // sets right motor speed
  *tim4_ccr1 &= ~CCR_MASK;
  *tim4_ccr1 |= speed; // sets left motor speed

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1); // sets right motor direction
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0); // sets left motor direction

  float time = (angle * 3.1415 / 180) / w;
  HAL_Delay(time * 1000);
  stop();
}


/*
 * Spins Zumo in place counter-clockwise
 * Arguments: speed - Determines the angular velocity of the Zumo
 */
void spin_ccw(int speed) {
  *tim3_ccr2 &= ~CCR_MASK;
  *tim3_ccr2 |= speed; // sets right motor speed
  *tim4_ccr1 &= ~CCR_MASK;
  *tim4_ccr1 |= speed; // sets left motor speed

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0); // sets right motor direction
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1); // sets left motor direction

  HAL_Delay(2000);
  stop();
}

/*
 * Turns Zumo left while also moving forward
 */
void turn_left() {
  *tim3_ccr2 &= ~CCR_MASK;
  *tim3_ccr2 |= 199; // sets right motor speed
  *tim4_ccr1 &= ~CCR_MASK;
  *tim4_ccr1 |= 49; // sets left motor speed

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0); // sets right motor direction
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0); // sets left motor direction

  HAL_Delay(2000);
  stop();
}

/*
 * Turns Zumo right while also moving forward
 */
void turn_right() {
  *tim3_ccr2 &= ~CCR_MASK;
  *tim3_ccr2 |= 49; // sets right motor speed
  *tim4_ccr1 &= ~CCR_MASK;
  *tim4_ccr1 |= 199; // sets left motor speed

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0); // sets right motor direction
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0); // sets left motor direction

  HAL_Delay(2000);
  stop();
}

void turn(float rad, float w, float angle, int rl) { // Angle in degrees, rad & vel in cm
	if (rl) { // right turn for rl == 1
		*tim3_ccr2 &= ~CCR_MASK;
		*tim3_ccr2 |= (int)(25 + w * rad - (w * 19)); // sets right motor speed
		*tim4_ccr1 &= ~CCR_MASK;
		*tim4_ccr1 |= (int)(25 + w * rad + (w * 19)); // sets left motor speed
	}
	else {
		*tim3_ccr2 &= ~CCR_MASK;
		*tim3_ccr2 |= (int)(25 + w * rad + (w * 19)); // sets right motor speed
		*tim4_ccr1 &= ~CCR_MASK;
		*tim4_ccr1 |= (int)(25 + w * rad - (w * 19)); // sets left motor speed
	}

	float time = (angle * 3.1415 / 180) / w;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0); // sets right motor direction
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0); // sets left motor direction

	HAL_Delay(time * 1000);
	stop();
}

/*
 * Discretely spins Zumo in place clockwise for a given number of seconds
 * Arguments:
 * 		speed - Determines the angular velocity of the Zumo while spinning
 * 		seconds - How long to spin for
 */
void discrete_spin_cw(int speed, int seconds) {
	for (int i = 0; i < 2 * seconds; i++) {
		spin_cw(speed);
		HAL_Delay(400); // delay for 0.4 seconds
		stop();
		HAL_Delay(100); // delay for 0.1 seconds
	}
}

/*
 * Discretely spins Zumo in place counter clockwise for a given number of seconds
 * Arguments:
 * 		speed - Determines the angular velocity of the Zumo while spinning
 * 		seconds - How long to spin for
 */
void discrete_spin_ccw(int speed, int seconds) {
	for (int i = 0; i < 2 * seconds; i++) {
		spin_ccw(speed);
		HAL_Delay(400); // delay for 0.4 seconds
		stop();
		HAL_Delay(100); // delay for 0.1 seconds
	}
}

void snake(float rad, float w, int rl) {
	turn(rad, w, 180, rl);
	turn(rad, w, 180, 1 - rl);
//	turn(rad, w, 180, rl);
//	turn(rad, w, 180, 1 - rl);
}

void star() {
	straight_forward(100);
    straight_backward(100);
	spin_cw_angle(3, 72);
//	for (int i = 0; i < 5; i++) {
//		straight_forward(100);
//		straight_backward(100);
//		spin_cw_angle(3, 72);
//	}
}

void square() {
	straight_forward(100);
	spin_cw_angle(3, 90);
//	for (int i = 0; i < 4; i++) {
//		straight_forward(100);
//		spin_cw_angle(3, 90);
//	}
}

void christmas_dance() {
	star();
}

void groovy_dance() {
	square();
}

void jazz_dance() {
//	turn(20, 3, 180, 1);
//	turn(20, 3, 180, 0);
	snake(20, 3, 1);
	snake(20, 3, 0);
}

void rock_dance() {
	spin_cw_angle(3, 360); //maybe w=4
}

void start_dance(char genre) {
	switch (genre) {
	    case 'C':
	        christmas_dance();
	        break;
	    case 'G':
	        groovy_dance();
	        break;
	    case 'J':
	    	jazz_dance();
	        break;
	    case 'R':
	    	rock_dance();
	    	break;
	    case 'S':
	    	stop();
	    default:
	        stop();
	}
}

#endif /* INC_DANCE_LIBRARY_H_ */
