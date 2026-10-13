/*
 * Lab06.c
 *
 * Created: 10/4/2022 9:16:57 AM
 * Initial Coder: jfhutton
 * Current Coder: <Jack Robinson>
 * Modified:      <10/6/2026>
 *
 * This lab uses hardware LEDs wired to PORTA, an interrupt from the joystick center
 * button, and a timer interrupt to build a game loop program that can display
 * different patterns on the LEDs.
 *
 * While there are many "down and dirty" ways to get this coding done, try to
 * remember your coding and data structures classes.  Things like ENUM, Arrays, Functions
 * could help make for more elegant coding.
 *
 */

#include <avr/io.h>              // Needed for AVR IO defines
#include <avr/interrupt.h>       // Needed for AVR interupt devines

#define LEDS            PORTA    // alias PORTA


// global variables for communication between ISRs and main
const unsigned char TCNT0_COUNT_SET = 0x8E;// Count for 1ms loop (Provided by Prof Hutton)

enum patterns
{
	BottomToTop, // A0 to A7
	TopToBottom, // A7 to A0
	BackAndForth, // A0 to A7 to A0
	MysteryPattern, // low half / high half alternate
	BinaryCount, // Count up in binary 0 to 255
	BinaryCountdown // Counts down from 255 to 0
};


volatile enum patterns Mode = BottomToTop;   // which pattern we are on, set by INT0 ISR
volatile unsigned char Tick = 0;             // incremented by TIMER0 ISR every 1ms
volatile unsigned char step = 0;             // position in the current pattern

int main(void){

	// Initialization for LEDs
	// Set direction for A ports.
	// (Prof Note:  This is similar to our two line assembly commands.)

	DDRA = 0xFF;  // Set the Direction for all PORTA pints to be outputs
	LEDS = 0xFF;  // Set the PORTA for all pins to be high (i.e. OFF)
    //LEDS = 0x00; TEST - Set all PORTA pins to be low (i.e. ON)

	// Initialization for Timer Interrupt
	// With a 7.3MHz crystal we have a 0.137us period
	// To build a 1ms timer "tick" we
	// - set the pre-scaler to 1/64 (8.77us)
	// - set the TCNT1 to count 114 counts (0-114=0x8E)
	// - enable the interrupt on overflow
	// (Prof Note: these are the commands you need, BUT they are commented
	//  out to start.  Be sure you fully understand them (book, datasheet, etc)
	//  before you enable them...  You also have to have the proper ISR routine
	//  ready for this to work.)
	// (Prof Note Two:  You will need to show your own calculations in the
	//  Lab Report to verify the TCNT0_COUNT_SET value!)

	TCCR0 = (1<<CS02);
	TCNT0 = TCNT0_COUNT_SET;
	TIMSK = (1<<TOIE0);

	// Port Initialization
	// Using the LED PORTA initilization above and your Lab05 code,
	// Configure the joystick button.
	// (Prof Note: you will need to add another jumper wire, or
	// change the ones you have, to get the center button working!

	DDRB  &= ~(1 << PB0);   // PB0 = joystick center button (input)
	PORTB |=  (1 << PB0);   // pull-up on
	DDRD  &= ~(1 << PD0);   // PD0 = INT0 (input), jumpered to PB0

	// Interrupt Enable Block
	// Using your Lab05 code, you will need to update these
	// lines from 0x00 to have the appropriate mask.
	EICRA = 0x03;  // INT0 on rising edge (button released)
	EIMSK = 0x01;  // enable INT0
	// Enable Global Interrupts
	sei();

	// Main Loop
	while (1) {
		while(Tick < 50);   // wait here until 50ms passed
		Tick = 0;           // start counting again
		switch(Mode)
		{
			case BottomToTop:
				LEDS = ~(1 << step);   // turn on one LED (~ because active low)
				step++;
				if (step > 7){
					step = 0;   // back to the start
				}
				break;

			case TopToBottom:
				LEDS = ~(0x80 >> step);   // start at A7 and shift down
				step++;
				if (step > 7){
					step = 0;
				}
				break;

			case BackAndForth:
				// first 8 steps go up, the next 6 come back down
				if (step < 8){
					LEDS = ~(1 << step);          // A0 to A7
				}
				else{
					LEDS = ~(1 << (14 - step));   // A6 to A1
				}
				step++;
				if (step > 13){
					step = 0;
				}
				break;

			case MysteryPattern:
				// swaps between the bottom half and top half
				if (step == 0){
					LEDS = 0xF0;   // A0-A3 on (active low)
				}
				else{
					LEDS = 0x0F;   // A4-A7 on (active low)
				}
				step++;
				if (step > 1){
					step = 0;
				}
				break;
			case BinaryCount:
				LEDS = ~(0x00 + step);   // step is the number being shown
				step++;
				break;

			case BinaryCountdown:
				LEDS = ~(0xFF - step);   // 255 down to 0 on A7-A0 (active low)
				step++;                  // 8-bit step wraps 255 -> 0, so the count repeats
				break;
		}
	}
}

// This is the required format of an ISR routine for a
// Timer.  Your ISR code should go inside
// (Prof Note:  When you define this correctly, the JUMP TABLE
//  will be properly updated by the compiler.  (Thank you
ISR(TIMER0_OVF_vect){

	TCNT0 = TCNT0_COUNT_SET;   // reload for next 1ms
	Tick++;

}

// This is the required format of an ISR routine for a
// Hardware Pin Interupt.  Format is "PIN#_vect" where
// '#' is the number of the interupt...  PD0->0, PD1->1, etc
ISR(INT0_vect){

	// button was released so go to the next pattern
	if (Mode == BinaryCountdown){
		Mode = BottomToTop;   // last pattern, so go back to the first
	}
	else{
		Mode++;
	}
	step = 0;   // restart the new pattern

}