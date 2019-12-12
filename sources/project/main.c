/**
 * @file main.c
 * @version 1.20
 * @brief The file contains upper level init functions calls and main while(1) cycle
 * @date 12-12-2019
 */
/**
 * @mainpage
 * Pitstick
 * ====================================================
 * Pitstick is a device that contains 2 addressable LED strips, STM32F103 CPU and is powered
 * by 2х18650 and DC/DC step down converter. Device has one button for setup and power switch.
 * Button can not be used in normal usecases. It's for config only.\n
 * @author Mykhalo Shcherbak
 * @em mikl74@yahoo.com
 * @version 1.20
 * @date 12-12-2019
 * @startuml
 * header
 * Main flow
 * endheader
 * start
 * if (Button_is_Pressed) then (yes)
 *  :Config mode;
 * else (no)
 *  :Main mode;
 * endif
 * :Lock;
 * end
 * @enduml
 * @startuml
 * header
 *  Configuration flow
 * endheader
 * start
 * :Brighness;
 * :Mode selection\nGreen = PitStick\nRed = Slalom;
 * if (Mode) then (Slalom)
 *  :Min random value (*.1s)\nWhite;
 *  :Max random value (*.1s)\nGreen;
 * else (PitStick)
 *  :Total pit+lap time (s)\nOrange;
 *  :T1(s)\nYellow;
 *  :T2(s)\nBlue;
 * endif
 * :Save;
 * :Lock;
 * end
 * @enduml
 * @startuml
 * header
 *  Workflow for PitStick mode
 * endheader
 * [*] --> Green_Blinking_Phase
 * Green_Blinking_Phase --> Yellow_Traffic_Light
 * Yellow_Traffic_Light : Phase duration is 5s\nDriver must start from pitlane at the end of the phase\nThe end of the phase corresponds T1 time before pitstop+lap end
 * Yellow_Traffic_Light --> 2_Green_Blinks
 * 2_Green_Blinks --> Going_To_Intermediate_Point
 * Going_To_Intermediate_Point --> Intermediate_Point
 * Intermediate_Point : 2 red blinks. Driver must be at intermediate point\nat the start of the first blink.\nThis time is T2 before end of the sequence
 * Intermediate_Point --> Going_To_Start_Finish
 * Going_To_Start_Finish : Knowing if driver was early or late at the\nintermediate point he must speed-up or slow down here
 * Going_To_Start_Finish --> Red_Traffic_Light
 * Red_Traffic_Light : This phase starts at 5s before ideal time(TSeq)\ncrossing start-finish line
 * Red_Traffic_Light --> 2_Green_Blinks_2
 * 2_Green_Blinks_2 : The pilot must cross start-finish line at the first blink.
 * 2_Green_Blinks_2 --> Lock
 * Lock --> [*]
 * @enduml
 * @startuml
 * header
 * Slalom Traffic Light flow
 * endheader
 * [*] --> Yellow_Pre_Phase
 * Yellow_Pre_Phase --> Main_Traffic_Light_Mode
 * Main_Traffic_Light_Mode : Red lights go on one-by one.\nDriver must strat at the moment (random) all of them go off.
 * Main_Traffic_Light_Mode --> Vbat_Show
 * Vbat_Show : At the end of the previuose phase+5s 2 upper leds\nshow the battery state
 * Vbat_Show --> Lock
 * Lock --> [*]
 * @enduml
 * @startuml
 * header
 * Stop-and-go mode flow
 * endheader
 * [*] --> Red_Phase
 * Red_Phase : Duration is taken from configuration phase.
 * Red_Phase --> Green_Phase
 * Green_Phase : Duration is 10 sec
 * Green_Phase --> Vbat_Show
 * Vbat_Show : At the end of the previous phase+5s 2 upper leds\nshow the battery state
 * Vbat_Show --> Lock
 * Lock --> [*]
 * @enduml
 */
#include "bll.h"
#include "clock.h"
#include "gpio.h"
#include "stm32f1xx.h"
#include "timer_dma.h"
#include "buttons.h"
#include "eeemu.h"
#include "watchdog.h"
#include "adc.h"

/**
 * @brief Contains all init functions.
 */
static void Init(void)
{
	Clock_HSE_Init();
	Systick_Init();
	Gpio_Init();
	Buttons_Init();
	tim2_Init();
	eeemu_Init();
	Adc_Init();
    watchdog_Init();
}


void main(void)
{
	Init();
	while(1)
	{
		MainLoop_Iteration();
		Reset_Watchdog();
	}
}
