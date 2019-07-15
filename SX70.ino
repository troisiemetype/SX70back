/*
 * This program handles the ejection system of a modified polaroid SX70 instant camera
 * Copyright 2019 - Pierre-Loup Martin / le labo du troisième
 *
 * SX70 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SX70 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This program handles the ejection system of a modified polaroid SX70 instant camera.
 * It is built around a state machine, and is using the original sensors of the original camera.
 *
 * Here is a list of the sensors :
 * S1	Shutter. Original sensor is crimped in the shutter unit, but a new one is used so the user can trigger the ejection process.
 * S2	Flash mode. In shutter unit, not used.
 * S3	Detects when the mirror is raised. useless as the mirror is removed, but used as a confirmation ejection has started.
 * S4	Solenoid sensor. Used in the camera to reduce power on the solenoid once the shutter is closed. Not used.
 * S5	Detects when the mirror is rearmed. used to stop the ejection process with ynamic braking of the motor.
 * S6	Main circuit power. Close when the camera is opened. Present, but not used.
 * S7	Main circuit power. Open when the front door is opened. Used as a general power-off switch.
 * S8	Closed when a new pack is inserted, then opened when the dark slide is ejected. Not used.
 * S9	Closed when counter reaches 0 (and the pack is empty). Not used.
 *
 * Here are a list of the IO of the system (all switches are internally pulled-up and active low):
 * S1	User-button.		PB0 / D8		Wake up the system out of sleep / triggers the ejection process.
 * S3	Mirror raised		PB1 / D9		Detects the ejection has started.
 * S5	Ejection ended		PB2 / D10		Stop the ejection process.
 * S7	Front door			main switch		Main switch.
 * 
 * motor					PB3 / D11		Run the motor through a mosfet transistor.
 *
 * bargraph					port D 			8 leds placed in a row, to display remaining pictures.
 *
 * status led 				PB5 / D13		Status led lit when awake. 
 *
 * Here is how the state machine works :
 * - When a new pack is inserted into the camera, and the front door is closed, current can flow into the circuit.
 * - The Arduino resets and initialize, sets the remaining pictures to 9 (8 pictures plus the dark slide).
 * 		Maybe the darkslide can be ejected by itself too.
 * - Then after 20 seconds, it goes to sleep, which is its "normal" mode.
 * - When the user needs to eject a film after having took a picture, the shutter (S1) is pressed.
 * - The system there goes out of sleep, displays the remaining views of the pack, and the sleep timer is launched.
 * - When the shutter (S1) is pressed again, the motor runs, the ejection process starts, and the sleep timer is stopped.
 * - When S3 is trigerred, the system starts to monitor S5.
 * - When S5 is trigerred, the motors stops, the image counter is decremented, and the timer for sleep is re-launched.
 * - If the shutter is not pressed before the sleep timer is over, then the system goes to sleep again.
 */

// includes
#include <avr/sleep.h>

#include "PushButton.h"
#include "Timer.h"

const uint16_t SW1_LONG_DELAY = 500;			// Delay for a long press of the user button, us
const uint8_t TIMER_SLEEP_DELAY = 20;			// timer delay before sleep, seconds.

// Picture counter init.
uint8_t pictureCounter = 9;
uint8_t pictureByte = 60;
uint8_t pictureLoop = 0;

bool statusLed = 0;

// pin definition
const uint8_t PIN_SW1 = 9;
const uint8_t PIN_SW3 = 10;
const uint8_t PIN_SW5 = 11;
const uint8_t PIN_MOTOR = 12;
const uint8_t PIN_STATUS_LED = 13;

// No pin definition for bargraph, as it is initialised in a loop, and set with direct port D manipulation.

// switches definition
PushButton sw1 = PushButton();
PushButton sw3 = PushButton();
PushButton sw5 = PushButton();

// Timer
Timer timerSleep = Timer();
Timer timerProcess = Timer();
const uint8_t TIMER_PROCESS_DELAY = 50;		// timer delay for led blking when processing, milliseconds.

// Definition of the states used for different phases of work.
enum state_t{
	WAKE_UP = 0,
	WAIT_SW1,
	IDLE,
	EJECT_START,
	EJECT_WAIT_SW3,
	EJECT_WAIT_SW5,
	EJECT_STOP,
} state;

// initialisation at power up, i.e. when a new pack is inserted.
void setup(){
	// Init buttons
	sw1.begin(PIN_SW1, INPUT_PULLUP);
	sw1.setLongDelay(SW1_LONG_DELAY);
	sw3.begin(PIN_SW3, INPUT_PULLUP);
	sw5.begin(PIN_SW5, INPUT_PULLUP);

	timerSleep.init();
	timerProcess.init();

	// Outputs
	// led bargraph
	for(uint8_t i = 0; i < 8 ; ++i){
		pinMode(i, OUTPUT);
	}
	// Status led
	pinMode(PIN_STATUS_LED, OUTPUT);
	// Motor
	pinMode(PIN_MOTOR, OUTPUT);

	// disable ADC
	ADCSRA &= ~(1 << ADEN);

	// disable all the functions not used by the program.
	// TWI
	// Timer 2
	// Timer 1
	// SPI
	// USART
	// ADC
	PRR |= (1 << PRTWI) | (1 << PRTIM2) | (1 << PRTIM1) | (1 << PRSPI) | (1 << PRUSART0) | (1 << PRADC);

	state = WAKE_UP;

//	wakeUp();

	// For debuging : display the value of Power Reduction Register
//	displayPort(PRR);
}

void loop(){
	if (timerSleep.update()) goToSleep();

	if (pictureLoop == 0){
		PORTD = pictureByte;
		digitalWrite(PIN_STATUS_LED, statusLed);

	} else {
		PORTD = 0;
		digitalWrite(PIN_STATUS_LED, 0);
	}
	++pictureLoop;
	pictureLoop &= 7;

	sw1.update();
	sw3.update();
	sw5.update();

	switch(state){
		case WAKE_UP:
			wakeUp();
			state = WAIT_SW1;
			break;

		case WAIT_SW1 :
			if (sw1.isReleased()){
				state = IDLE;
				timerSleep.setSeconds(TIMER_SLEEP_DELAY);
				timerSleep.start();
			}
			break;

		case IDLE :
			if (sw1.isLongPressed()){
				timerSleep.stop();
				timerProcess.setDelay(TIMER_PROCESS_DELAY);
				timerProcess.start(-1);
				
				digitalWrite(PIN_MOTOR, 1);
				state = EJECT_WAIT_SW3;
			} else if (sw1.justReleased()){
				timerSleep.start();
			}
			break;

		case EJECT_START :
			break;

		case EJECT_WAIT_SW3 :
			if(timerProcess.update()) statusLed ^= 1;
			if (sw3.isReleased()) state = EJECT_WAIT_SW5;
			break;

		case EJECT_WAIT_SW5 :
			if(timerProcess.update()) statusLed ^= 1;
			if (sw5.isPressed()){
				digitalWrite(PIN_MOTOR, 0);
				timerProcess.stop();

				if(pictureCounter !=0) pictureCounter--;
				pictureByte = 0;
				for (uint8_t i = 0; i < pictureCounter; ++i){
					pictureByte |= (1 << i);
				}

				state = WAKE_UP;
			}
			break;

		case EJECT_STOP :
			break;

		default:
			break;
	}

//	if (sw1.isLongPressed()) eject();
	
}

// Initialisation when waking up, i.e. when the user pushes the button.
void wakeUp(){

	statusLed = 1;
	digitalWrite(PIN_MOTOR, 0);

	// We turn pullup on on wake up, as it is disabled when asleep.
	sw3.begin(PIN_SW3, INPUT_PULLUP);
	sw5.begin(PIN_SW5, INPUT_PULLUP);


//	displayPictureCount();
/*
	// check for button being depressed to avoid accidental picture ejection.
	for(;;){
		sw1.update();
		if (sw1.isReleased()) break;
	}

	timerSleep.setSeconds(TIMER_SLEEP_DELAY);
	timerSleep.start();
*/
}

// Start ejection sequence
// Motor started.
// S3 triggered confirms ejection is running.
// S5 trigerred confirms ejection is finished.
// Motor stopped.
// Picture counter decremented.
// Sleep timer relaunched.
/*
void eject(){
	timerSleep.stop();
	
	timerProcess.setDelay(TIMER_PROCESS_DELAY);
	timerProcess.start(-1);
	
	digitalWrite(PIN_MOTOR, 1);

	for(;;){
		if(timerProcess.update()){
			statusLed ^= 1;
			digitalWrite(PIN_STATUS_LED, statusLed);
		}

		sw3.update();
		sw5.update();
		if (sw3.isReleased()) break;
	}

	for(;;){
		if(timerProcess.update()){
			statusLed ^= 1;
			digitalWrite(PIN_STATUS_LED, statusLed);
		}
		
		sw3.update();
		sw5.update();
		if (sw5.isPressed()) break;
	}

	digitalWrite(PIN_MOTOR, 0);

	timerProcess.stop();

	if(pictureCounter !=0) pictureCounter--;
	for (uint8_t i = 0; i < pictureCounter; ++i){
		pictureByte |= (1 << i);
	}

	wakeUp();

}
*/

void displayPictureCount(){
	uint8_t output = 0;
	if (pictureCounter == 9){
		output = 60;
//	} else if (pictureCounter == 0){
//		output = 36;
	} else {
		for (uint8_t i = 0; i < pictureCounter; ++i){
			output |= (1 << i);
		}
	}

	PORTD = output;

}

// For debugging purpose : use the eight leds to display the state of a register
void displayPort(uint8_t data){
	PORTD = data;
	delay(3000);
	displayPictureCount();
}

void goToSleep(){
	// Set an interrupt ont PCINT1 (pin 9 / port B1 ) to wakeup.
	PCICR = (1 << PCIE0);
	PCMSK0 = (1 << PCINT1);

	// disable picture count
	PORTD = 0;
	// disable status led
	digitalWrite(PIN_STATUS_LED, 0);

	// disable pullup for sleep on the ejection switches.
	sw3.begin(PIN_SW3, INPUT);
	sw5.begin(PIN_SW5, INPUT);

	// Store the register values.
	char sregTemp = SREG;

	//enable sleep, set power down mode
	SMCR |= (1 << SE);
	SMCR |= (1 << SM1);

	// Disable watchdog timer
	WDTCSR = 0;
	
	// Disable Brown Out detector
	// (the datasheet gives this way of proceeding)
	MCUCR |= (1 << BODSE) | (1 << BODS);
	MCUCR |= (1 << BODS);
	MCUCR &= ~(1 << BODSE);
	
	//	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();

	//disable sleep
	SMCR &= ~(1 << SE);

	//Restore status register
	SREG = sregTemp;

	//reinit devices
	state = WAKE_UP;
//	wakeUp();
}

ISR(PCINT0_vect){
	//clear the interrupts, are they are only for sleep wakeup.
	PCICR = 0;
	PCMSK0 = 0;

}