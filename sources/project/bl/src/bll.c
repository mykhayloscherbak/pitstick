/**
 * @file bll.c
 * @date 9-11-2019
 * @brief Contains task switcher implementation and task table.
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bll.h"
#include "heartbeat.h"
#include "clock.h"
#include "watchdog.h"
#include "buttons.h"
#include "led_control.h"

/**
 * @brief Task table element
 */
typedef struct
{
	uint32_t Period; /**< Period of task in tics (10ms per tick) */
	uint32_t Phase;  //!< Task phase (remain of division)
	void (*Task)(void); //!< Task function
} Task_table_t;

/**
 * @brief calls @ref led_control function with ms as a parameter. ms is time from stick on to the current time
 */
static void ledControl_wrapper(void)
{
	static uint8_t firstTime = !0;
	static uint32_t mainTimer;
	if (firstTime != 0)
	{
		ResetTimer(&mainTimer);
		firstTime = 0;
	}
	led_control(ReadTimer(&mainTimer));
}
/**
 * @brief Polls the buttom
 */
static void processButtons(void)
{
	Button_Process(B_CONFIG);
}

/**
 * @brief Contains main while(1) loop.  Tasks are selected from the task table. Last task should be {0,0,NULL}.
 * First parameter is period and the second is "phase" which is a remaining after dividing current time by the first param
 * It's done to make switcher to use different timeslots
 * @return 0 if no tick occured
 */
uint8_t MainLoop_Iteration(void)
{
	static const Task_table_t TaskTable[]={
			{10,2,processButtons},
			{500,3,Toggle_Heartbeat},
			{100,1,ledControl_wrapper},
			{0,0,NULL}
	};
	static uint32_t OldTicksCounter = 0;
	uint32_t TicsCounter = GetTicksCounter();
	uint8_t RetVal = 0;
	Reset_Watchdog();
	if (TicsCounter != OldTicksCounter)
	{
		uint8_t Counter = 0;
		do
		{
			if (((TicsCounter % TaskTable[Counter].Period) == TaskTable[Counter].Phase) &&
					TaskTable[Counter].Task != NULL)
			{
				(*TaskTable[Counter].Task)();
			}
		} while (TaskTable[++Counter].Task != NULL);
		OldTicksCounter = TicsCounter;
		RetVal = 1;
	}
	return RetVal;
}
