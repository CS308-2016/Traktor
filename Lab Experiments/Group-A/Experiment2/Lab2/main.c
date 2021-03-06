#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))
#define IDLE 0
#define PRESS 1
#define RELEASE 2

/* Some global variables to maintain the state of switches and LEDs. */
int sw2Status = 0;
int state = 0, state2 = 0;
uint8_t ui8LED = 4;

/*
* Function Name: switchPinConfig()
* Input: none
* Output: none
* Description: Set PORTF Pin 0 and Pin 4 as input. Note that Pin 0 is locked.
* Example Call: switchPinConfig();
*/
void switchPinConfig(void)
{
	// Following two line removes the lock from SW2 interfaced on PORTF Pin0
	LOCK_F=0x4C4F434BU;
	CR_F=GPIO_PIN_0;

	// GPIO PORTF Pin 0 and Pin 4
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_0|GPIO_PIN_4,GPIO_DIR_MODE_IN); // Set Pin-0,4 of PORT F as Input.
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_0|GPIO_PIN_4,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}


int main(void)
{
	uint32_t ui32Period;
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	ui32Period = (SysCtlClockGet() / 1) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();
	TimerEnable(TIMER0_BASE, TIMER_A);

	// Set up switches
	switchPinConfig();
	while(1)
	{
	}
}

/*
* Function Name: nextLEDConfig
* Input : None
* Output : void
* Description : Changes LED config from one state to next one.
*/
void nextLEDConfig(){
	if (ui8LED == 2) {
		ui8LED = 8;
	}
	else if (ui8LED == 8) {
		ui8LED = 4;
	}
	else {
		ui8LED = 2;
	}
}

/*
* Function Name: detectKeyPress
* Input : None
* Output : int
* Description : Returns when a key is pressed (after debouncing) using system's state.
*/
int detectKeyPress(){
	if (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00){
		if (state == PRESS) {
			state = RELEASE;
			nextLEDConfig();
			return 1;
		}
		else if (state == IDLE){
			state = PRESS;
		}
		else return 1;
	}
	else {
		state = IDLE;
	}
	return 0;
}

/*
* Function Name: detectSwitch2Press
* Input : None
* Output : int
* Description : Detects if switch-2 is pressed.
*/
int detectSwitch2Press(){
	if (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00){
		if (state2 == PRESS) {
			state2 = RELEASE;
			return 1;
		}
		else if (state2 == IDLE){
			state2 = PRESS;
		}
	}
	else {
		state2 = IDLE;
	}
	return 0;
}


/*
* Function Name: Timer0IntHandler
* Input : None
* Output : None
* Description : Interrupt handling function for Timer-0 timer interrupt.
*/
void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	// Read the current state of the GPIO pin and
	// write back the opposite state
	if(detectKeyPress())
	{
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8LED);

	}
	else
	{
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
	}

	if (detectSwitch2Press()){
		sw2Status++;
	}
}
