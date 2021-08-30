/**
 * @file led_control.c
 * @brief Contains led control logic for all modes:
 *        + Pitstop
 *        + Traffic light for autoslalom
 *        + Rally stop-and-go ("Podnos") mode
 *        + Safety Car ("Zvyagin") mode
 *        + kart 2h mode
 *        + Config mode
 *        .
 *
 *========================================================================
 * @brief In config mode values are shown by displaying bars in different colors. One bar shows tens and other  - ones.
 * @brief For example if one (left) shows 2 leds and other (right) shows 9 leds it's 29. Colors are:
 *
 * + Orange : TSeq
 * + Yellow : T1
 * + Blue   : T2
 * + White  : Tlight min
 * + Green  : Tlight max
 * + Red    : Podnos phase duration
 *
 * @date 30-08-2021
 * @version 1.30
 */
#include <adc.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "buttons.h"
#include "rgbw.h"
#include "led_control.h"
#include "eeemu.h"
#include "clock.h"
#include "project_conf.h"
#include "prng.h"
#include "led_strip.h"

static const uint16_t S = 1000u; /**< millisecons per second */
static const uint32_t MIN = S * 60u; /**< milliseconds per minute */

/**
 * @brief Main state machine states
 */
typedef enum
{
	STATE_IDLE = 0,                 /**< Idle state. It's init state at which system can be for very short time */
	STATE_CONFIG,                   /**< Config state. This state is entered if button is pressed at startup time */
	STATE_PIT,                      /**< Main mode, pit stop */
	STATE_TLIGHT,                   /**< Autoslalom mode. Switching between modes is done by the second config frame */
	STATE_PODNOS,                   /**< Stop-and-go ("Podnos") mode */
	STATE_SC,                       /**< Safety car mode */
	STATE_K2H,					    /**< Karting 2H mode */
	STATE_LOCK,                     /**< Lock mode. It works after ending one if the previous modes and is needed to remind that the stick is not turned off */
	STATE_CONFIG_PARAM_IDLE,        /**< Stick is in the configuration  mode and mode processor is in the idle state */
	STATE_CONFIG_PARAM_PRESSED,     /**< Button is pressed while parameter is displayed */
	STATE_CONFIG_PARAM_RELEASED,    /**< Button is released while the parameter is displayed in configuration mode */
	STATE_CONFIG_PARAM_LONG_PRESSED,/**< Button is long pressed while parameter is displayed */
	STATE_CONFIG_PARAM_SAVING,      /**< Current parameter was changed and will be saved. Short red blink indicates that. */
	STATE_CONFIG_PARAM_NOT_SAVING,  /**< Current parameter was not changed and will not be saved. Short turn the stick off confirms that */
	STATE_CONFIG_PARAM_WAITING,     /**< Waiting after last button press */
	STATE_CONFIG_BEGIN,             /**< Beginning of the configuration process */
	STATE_CONFIG_BRIGHTNESS,        /**< Configuring brightness */
	STATE_CONFIG_MODE,              /**< Configuring mode (tlight for slalom or pitstick mode) */
	STATE_CONFIG_TSEQ,              /**< Configuring total pitstop duration */
	STATE_CONFIG_T1,                /**< Configuring T1. T1 is the time kart starts going from the pitlane */
	STATE_CONFIG_T2,                /**< Configuring T1. T2 is time from intermediate point and crossing start/stop line */
	STATE_CONFIG_TLIGHT_MIN,        /**< Configuring minimal time for the last phase in tlingt mode. 1/10s resolution */
	STATE_CONFIG_TLIGHT_MAX,        /**< Configuring maximal time for the last phase in tlingt mode. 1/10s resolution */
	STATE_CONFIG_PODNOS_MODE,       /**< Set stop-and-go mode duration */
	STATE_CONFIG_SC_END,            /**< End of safety car mode config */
	STATE_CONFIG_K2H_END,           /**< End of Kartig 2h mode config */
	STATE_CONFIG_END,               /**< End of configuration process */
	STATE_LOCK_START,               /**< Start lock mode. Lock mode is turned on after all operations are complete to remimd switchig the stick off */
	STATE_LOCK_ON,                  /**< Lock mode. Red light+battery power is on */
	STATE_LOCK_OFF,                 /**< Lock mode. All lights are off */
	STATE_GB_GREEN,                 /**< Green->black phase. Green leds are on */
	STATE_GB_BLACK,                 /**< Green->black phase. All is off */
	STATE_TLIGHT_SLALOM,            /**< Slalom mode start */
	STATE_TLIGHT_SHOW_RANDOM,       /**< Random pause after the fifth strip is on */
	STATE_SC_SHOW_POWER,            /**< First phase of Safety Car mode, showing battery level */
	STATE_SC_MAIN,                  /**< Main phase of safety car mode */
	STATE_K2H_BLACK,                /**< Black phase of kart 2h mode. Power indicator only is shown */
	STATE_K2H_ALL_GREEN,            /**< First blink at pitsop window open - 4 times all green at 0.5Hz -8s */
	STATE_K2H_ONE_BY_ONE_GREEN,     /**< Next 472s = rows 0-58 4 times at 0.5Hz each */
	STATE_K2H_ONE_BY_ONE_BLUE,      /**< Rows 59-71 5 times 0.5Hz each */
	STATE_K2H_ALL_RED               /**< After 10 minutes 4 times 0.5Hz each, all red */
}States_t;

/**
 * @brief Working modes. Pitstop or slalom tlight
 */
typedef enum
{
	MODE_PIT = 0,/**< Pitstop mode */
	MODE_TLIGHT, /**< Slalom traffic light mode */
	MODE_PODNOS, /**< Mode for rally stop-and-go */
	MODE_SC,     /**< Safety car mode */
	MODE_KART2H, /**< Karting 2h mode */
	MODE_TOTAL   /**< Total number of modes */
} Working_Mode_t;

/**
 * @brief Flash storage config parameters
 */
typedef enum /* Attention! Adding new channels here do not forget to extend MAX_STORAGE in eeemu.h */
{
	CH_BRIGHTNESS = 0,	/**< Brightness */
	CH_MODE,          	/**< Selected work mode */
	CH_TSEQ,          	/**< Total pitstop time */
	CH_T1,            	/**< T1 Time in seconds */
	CH_T2,            	/**< T2 Time in seconds */
	CH_TLIGHT_MIN,    	/**< Tlight random part minimal value in 1/10s */
	CH_TLIGHT_MAX,     	/**< Tlight random part maximal value in 1/10s */
	CH_PODNOS_MODE_TIME /**< Stop-and-go mode time */
}Config_idx_t;

/**
 * @brief End of config param mode
 */
typedef enum
{
  EM_CONTINUE = 0,          /**< Param configuration is not complete but display must be updated */
  EM_CONTINUE_DO_NOT_UPDATE,/**< Param configuration is not complete but display must not be updated */
  EM_END_SAVING,            /**< Param config mode is ended and parameter need to be saved */
  EM_END_NOT_SAVING         /**< Parameter configuration mode is ended but parameter was not changed */
}EndMode_t;

/**
 * @brief the is function type for displaying a value of the parameter that is changed in config mode
 * @param value - the value to display
 */
typedef void (*pDispFunc_t)(const uint8_t value);

/**
 * @brief In/out structure for configuring parameters
 */
typedef struct
{
  uint8_t value; /**< Value itself */
  uint8_t min;   /**< Minimum of the value */
  uint8_t max;   /**< Maximum of the value */
  pDispFunc_t dispFunc; /**< Pointer to the function to display the value */
  EndMode_t endMode; /**< Result of current iteration */
} DispValue_t;

/**
 * @brief Show pattern for configuring brighness: red, green, blue and white strips
 */
static void dispBrighnessPattern(void)
{
	showFull(BLACK);
	for (uint8_t i = 0; i < 5; i++)
	{
		put2pixels(RED,i);
		put2pixels(GREEN,i + 5);
		put2pixels(BLUE,i + 10);
		put2pixels(WHITE,i + 15);
	}
}

/**
 * @brief Displays brightness setup pattern using the current brighness
 * @param value brightness value
 */
static void displayBrightness(const uint8_t __attribute__((unused)) value)
{
  dispBrighnessPattern();
}

/**
 * @brief Displays mode pattern: green points for @ref MODE_PIT and red strips for @ref MODE_TLIGHT
 * @param value selected mode
 */
static void dispModePattern(const uint8_t value)
{
	showFull(BLACK);
	switch(value)
	{
	case MODE_PIT:
		for (uint8_t i = 0; i < 5; i++)
		{
			put2pixels(GREEN,i * 2);
		}

		break;
	case MODE_TLIGHT:
		for (uint8_t i = 0; i < 5; i++)
		{
			put2pixels(RED,i * 3);
			put2pixels(RED,i * 3 + 1);
		}
		break;
	case MODE_PODNOS:
		for (uint8_t i = 0; i < 10; i++)
		{
			put2pixels(GREEN,i + 10);
			put2pixels(RED,i);
		}
		break;
	case MODE_SC:
		put2pixels(YELLOW,0);
		put2pixels(YELLOW,4);
		break;
	case MODE_KART2H:
		for (uint8_t i = 0; i < 10; i++)
		{
			put2pixels(GREEN,i);
		}
		for (uint8_t i = 10; i < 13; i++)
		{
			put2pixels(BLUE,i);
		}
		break;
	default:
		break;
	}
}


static void displayTSEQ(const uint8_t value)
{
	displayNumber(ORANGE,ORANGE,value);
}

static void displayT1(const uint8_t value)
{
  displayNumber(YELLOW,YELLOW,value);
}

static void displayT2(const uint8_t value)
{
  displayNumber(BLUE,BLUE,value);
}

static void displayTlightMin(const uint8_t value)
{
	displayNumber(WHITE,WHITE,value);
}

static void displayTlightMax(const uint8_t value)
{
	displayNumber(GREEN,GREEN,value);
}

static void displayPodnosModeTime(const uint8_t value)
{
	displayNumber(RED,RED,value);
}

/**
 * @brief Incremens the value using min and max limits.
 * @param dV
 */
static void incValue(DispValue_t * const dV)
{
	if (++dV->value > dV->max)
	{
		dV->value = dV->min;
	}
}

/**
 * @brief show the param value and changes it during setup
 * @param dV value and limits
 * @param init This param is zero if it's not first call
 * @return 0 if sending buffer to strip is not needed
 */

static uint8_t changeParam(DispValue_t * const dV,const uint8_t init)
{
  static States_t state = STATE_CONFIG_PARAM_IDLE;
  static uint8_t saved = !0;
  const uint8_t isPressed = IsPressed(B_CONFIG);
  const uint8_t isLongPressed = IsLongPressed(B_CONFIG);
  static uint32_t timer;
  static uint32_t fastIncTimer;
  uint8_t changed = 0;
  if (init != 0)
  {
    state = STATE_CONFIG_PARAM_IDLE;
    saved = !0;
    if (dV->value < dV->min)
    {
      dV->value = dV->min;
      saved = 0;
    }
    if (dV->value > dV->max)
    {
      dV->value = dV->max;
      saved = 0;
    }
    dV->endMode = EM_CONTINUE;
  }

  switch(state)
  {
    case STATE_CONFIG_PARAM_IDLE:
      if (isPressed)
      {
        state = STATE_CONFIG_PARAM_PRESSED;
      }
      else
      {
        state = STATE_CONFIG_PARAM_RELEASED;
        ResetTimer(&timer);
      }
      dV->dispFunc(dV->value);
      changed = !0;
      break;
    case STATE_CONFIG_PARAM_PRESSED:
      if (isPressed == 0)
      {
        state = STATE_CONFIG_PARAM_RELEASED;
        ResetTimer(&timer);
      }
      if (isLongPressed != 0)
      {
    	  ResetTimer(&fastIncTimer);
    	  state = STATE_CONFIG_PARAM_LONG_PRESSED;
      }
      break;
    case STATE_CONFIG_PARAM_RELEASED:
      if (IsExpiredTimer(&timer,10000) != 0)
      {
        if (saved != 0)
        {
          state = STATE_CONFIG_PARAM_NOT_SAVING;
        }
        else
        {
          state = STATE_CONFIG_PARAM_SAVING;
        }
      } else if (isPressed != 0)
      {
    	incValue(dV);
    	saved = 0;
        state = STATE_CONFIG_PARAM_PRESSED;
        dV->dispFunc(dV->value);
        changed = !0;
      }
      break;
    case STATE_CONFIG_PARAM_LONG_PRESSED:
    	if (isPressed == 0)
    	{
    		state = STATE_CONFIG_PARAM_RELEASED;
    		ResetTimer(&timer);
    	}
    	else if (IsExpiredTimer(&fastIncTimer,200) != 0)
    	{
    		ResetTimer(&fastIncTimer);
    		incValue(dV);
    		saved = 0;
    		changed = !0;
    		dV->dispFunc(dV->value);
    	}
    	break;
    case STATE_CONFIG_PARAM_SAVING:
      showFull(BLACK);
      for (uint8_t i = 0; i < 20; i++)
      {
       put2pixels(DARK_RED,i);
      }
      changed = !0;
      ResetTimer(&timer);
      state = STATE_CONFIG_PARAM_WAITING;
      dV->endMode = EM_CONTINUE_DO_NOT_UPDATE;
      break;
    case STATE_CONFIG_PARAM_NOT_SAVING:
      showFull(BLACK);
      changed = !0;
      ResetTimer(&timer);
      state = STATE_CONFIG_PARAM_WAITING;
      dV->endMode = EM_CONTINUE_DO_NOT_UPDATE;
      break;
    case STATE_CONFIG_PARAM_WAITING:
      if (IsExpiredTimer(&timer,1000) != 0)
      {
        dV->endMode = (saved != 0) ? EM_END_NOT_SAVING : EM_END_SAVING;
      }
      else
      {
    	  dV->endMode = EM_CONTINUE_DO_NOT_UPDATE;
      }
      break;
    default:
      break;
  }
  return changed;
}

/**
 * @brief all config mode processing is done here
 * @param nextState pointer to the result. Non zero value shows that configuration is done and you can pass
 * to the next step
 * @return nonzero value shows that data is updated and must be sent to the strip
 */
static uint8_t  config(uint8_t * const nextState)
{
  uint8_t * conf = eeemuGetValue();
  static uint8_t brightness;
  static States_t state = STATE_CONFIG_BEGIN;
  static uint8_t saved = !0;
  static uint32_t timer;
  uint8_t changed = 0;
  static uint8_t savedParams[MAX_STORED];
  static DispValue_t dV;
  if (nextState != NULL)
  {
    *nextState = 0;
  }

  switch(state)
  {
    case STATE_CONFIG_BEGIN:
      memcpy(savedParams,conf,MAX_STORED);
      brightness = savedParams[CH_BRIGHTNESS];
      dV.dispFunc = displayBrightness;
      dV.min = 0;
      dV.max = MAX_BRIGHNESS_LEVELS - 1;
      dV.value = brightness;
      dV.endMode = EM_CONTINUE;
      changed = changeParam(&dV,!0);
      brightness = dV.value;
      state = STATE_CONFIG_BRIGHTNESS;
      break;
    case STATE_CONFIG_BRIGHTNESS:
      changed = changeParam(&dV,0);
      brightness = dV.value;
      setBrightness(brightness);
      savedParams[CH_BRIGHTNESS] = brightness;
      if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
      {
        if (dV.endMode == EM_END_SAVING)
        {
          saved = 0;
        }
        savedParams[CH_BRIGHTNESS] = brightness;
        dV.dispFunc = dispModePattern;
        dV.min = 0;
        dV.max = MODE_TOTAL - 1;
        dV.value = savedParams[CH_MODE];
        dV.endMode = EM_CONTINUE;
        uint8_t changed2 = changeParam(&dV,!0);
        changed = changed || changed2;
        state = STATE_CONFIG_MODE;
      }
      else
      {
    	  if (changed != 0 && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
    	  {
    		  dispBrighnessPattern();
    	  }
      }
      break;
    case STATE_CONFIG_MODE:
    	changed = changeParam(&dV,0);
    	if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
    	{
    		if (dV.endMode == EM_END_SAVING)
    		{
    			saved = 0;
    		}
    		savedParams[CH_MODE] = dV.value;
    		uint8_t noParam = !0;
    		switch (savedParams[CH_MODE])
    		{
    		case MODE_PIT:
    			dV.dispFunc = displayTSEQ;
    			dV.min = TSEQ_MIN;
    			dV.max = TSEQ_MAX;
    			dV.value = savedParams[CH_TSEQ];
    			dV.endMode = EM_CONTINUE;
    			noParam = 0;
    			state = STATE_CONFIG_TSEQ;
    			break;
    		case MODE_TLIGHT:
    			dV.dispFunc = displayTlightMin;
    			dV.min = TLIGHT_MIN;
    			dV.max = TLIGHT_MAX - 2;
    			dV.value = savedParams[CH_TLIGHT_MIN];
    			dV.endMode = EM_CONTINUE;
    			noParam = 0;
    			state = STATE_CONFIG_TLIGHT_MIN;
    			break;
    		case MODE_PODNOS:
    			dV.dispFunc = displayPodnosModeTime,
				dV.min = PODNOS_MODE_MIN;
    			dV.max = PODNOS_MODE_MAX;
    			dV.value = savedParams[CH_PODNOS_MODE_TIME];
    			dV.endMode = EM_CONTINUE;
    			noParam = 0;
    			state = STATE_CONFIG_PODNOS_MODE;
    			break;
    		case MODE_SC:
    	        state = STATE_CONFIG_SC_END;
    	        noParam = !0;
    			break;
    		case MODE_KART2H:
    	        state = STATE_CONFIG_K2H_END;
    	        noParam = !0;
    			break;

    		}
    		if (noParam == 0)
    		{
    			const uint8_t changed2 = changeParam(&dV,!0);
    			changed = changed || changed2;
    		}
    		else
    		{
    			changed = !0;
    		}
    	}
    	break;

    case STATE_CONFIG_TSEQ:
    	changed = changeParam(&dV,0);
    	savedParams[CH_TSEQ] = dV.value;
    	if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
    	{
    		if (dV.endMode == EM_END_SAVING)
    		{
    			saved = 0;
    		}
    		dV.dispFunc = displayT1;
    		dV.min = T1MIN;
    		dV.max = savedParams[CH_TSEQ] - T1MIN2;
    		dV.value = savedParams[CH_T1];
    		dV.endMode = EM_CONTINUE;
    		uint8_t changed2 = changeParam(&dV,!0);
    		changed = changed || changed2;
    		state = STATE_CONFIG_T1;
    	}
    	break;
    case STATE_CONFIG_T1:
      changed = changeParam(&dV,0);
      savedParams[CH_T1] = dV.value;
      if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
      {
        if (dV.endMode == EM_END_SAVING)
        {
          saved = 0;
        }
        dV.dispFunc = displayT2;
        dV.min = T2MIN;
        dV.max = savedParams[CH_T1] - T2MIN2;
        dV.value = savedParams[CH_T2];
        dV.endMode = EM_CONTINUE;
        uint8_t changed2 = changeParam(&dV,!0);
        changed = changed || changed2;
        state = STATE_CONFIG_T2;
      }
      break;
    case STATE_CONFIG_T2:
      changed = changeParam(&dV,0);
      savedParams[CH_T2] = dV.value;
      if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
      {
        if (dV.endMode == EM_END_SAVING)
        {
          saved = 0;
        }
        if (saved == 0)
        {
          eeemu_write(savedParams);
          for (uint8_t i = 0; i < 20; i++)
          {
           put2pixels(RED,i);
          }
          for (uint8_t i = 20; i < NLEDS / 2; i++)
          {
            put2pixels(BLACK,i);
          }
        }
        else
        {
        	showFull(BLACK);
        }
        changed = !0;
        ResetTimer(&timer);
        state = STATE_CONFIG_END;
      }
      break;
    case STATE_CONFIG_TLIGHT_MIN:
    	changed = changeParam(&dV,0);
    	savedParams[CH_TLIGHT_MIN] = dV.value;
    	if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
    	{
    		if (dV.endMode == EM_END_SAVING)
    		{
    			saved = 0;
    		}
    		dV.dispFunc = displayTlightMax;
    		dV.min = savedParams[CH_TLIGHT_MIN] + 1;
    		dV.max = TLIGHT_MAX;
    		dV.value = savedParams[CH_TLIGHT_MAX];
    		dV.endMode = EM_CONTINUE;
    		uint8_t changed2 = changeParam(&dV,!0);
    		changed = changed || changed2;
    		state = STATE_CONFIG_TLIGHT_MAX;
    	}
    	break;

    case STATE_CONFIG_TLIGHT_MAX:
        changed = changeParam(&dV,0);
        savedParams[CH_TLIGHT_MAX] = dV.value;
        if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
        {
          if (dV.endMode == EM_END_SAVING)
          {
            saved = 0;
          }
          if (saved == 0)
          {
            eeemu_write(savedParams);
            showFull(BLACK);
            for (uint8_t i = 0; i < 20; i++)
            {
             put2pixels(RED,i);
            }
          }
          else
          {
        	  showFull(BLACK);
          }
          changed = !0;
          ResetTimer(&timer);
          state = STATE_CONFIG_END;
        }
        break;
    case STATE_CONFIG_PODNOS_MODE:
    	changed = changeParam(&dV,0);
    	savedParams[CH_PODNOS_MODE_TIME] = dV.value;
    	if (dV.endMode != EM_CONTINUE && dV.endMode != EM_CONTINUE_DO_NOT_UPDATE)
    	{
    		if (dV.endMode == EM_END_SAVING)
    		{
    			saved = 0;
    		}
    		if (saved == 0)
    		{
    			eeemu_write(savedParams);
    			showFull(BLACK);
    			for (uint8_t i = 0; i < 20; i++)
    			{
    				put2pixels(RED,i);
    			}
    		}
    		else
    		{
    			showFull(BLACK);
    		}
    		changed = !0;
    		ResetTimer(&timer);
    		state = STATE_CONFIG_END;
    	}
    	break;

    case STATE_CONFIG_K2H_END:
    case STATE_CONFIG_SC_END:
        if (saved == 0)
        {
          eeemu_write(savedParams);
          showFull(BLACK);
          for (uint8_t i = 0; i < 20; i++)
          {
           put2pixels(RED,i);
          }
        }
        else
        {
      	  showFull(BLACK);
        }
        changed = !0;
        ResetTimer(&timer);
        state = STATE_CONFIG_END;
        break;

    case STATE_CONFIG_END:
      if (nextState != NULL)
      {
        *nextState = !0;
      }
      break;
    default:
      break;
  }
  return changed;
}
/**
 * @brief Voltage to color translation table element
 */
typedef struct
{
	uint16_t mv; /**< Milivolts measured from the battery */
	Colors_t color; /**< Corresponding color */
}	V2Color_t;

/**
 * @brief Returns the color corresponding to current battery voltage. Battery needs to be charged immediately if red
 * @return Color index
 */
static Colors_t getPowerColor(void)
{
	static const V2Color_t V2Color[]=
	{
			{6300,REDDER},
			{6500,ORANGE},
			{7000,YELLOW},
			{7600,GREEN}
	};
	const uint16_t mv = GetAdc_Voltage();
	Colors_t retval;
	if (mv <= V2Color[0].mv)
	{
		retval = RED;
	}
	else
	{
		uint8_t i;
		const uint8_t arrsize = sizeof(V2Color)/sizeof(V2Color[0]);
		for (i = 0; i < arrsize; i++)
		{
			if (V2Color[i].mv > mv)
			{
				break;
			}
		}
		retval = V2Color[i - 1].color;
	}
	return retval;
}
/**
 * Structure that describes strips to display
 */
typedef struct
{
	uint8_t from; /**< Number of the first led in the srip */
	uint8_t number; /**< Number of leds in the strip */
}GreenStrip_t;

/**
 * @brief Shows the first "green" phase of the pitstop mode
 * @param init is non zero if this is first call
 * @return nonzero value if data is changed and update needed
 * Green phase shows 5 strips blinking at 1Hz. Each 20secons one strip turns off. So at nineteenth second 5 strips
 * are blinking, at 21 - four strips. At 41 3 strips are blinking.
 */
static uint8_t greenPhase(const uint8_t init)
{
	static const GreenStrip_t strips[5]=
	{
			{.from =  5, 	.number = 2},
			{.from = 20,	.number = 2},
			{.from = 35,	.number = 2},
			{.from = 50,	.number = 2},
			{.from = 65,	.number = 2}
	};
	static uint8_t on = 0;
	static uint32_t timer05;
	static uint32_t timer20;
	static uint8_t count = 5;
	uint8_t changed = 0;
	if (init != 0)
	{
		showFull(BLACK);
		ResetTimer(&timer05);
		ResetTimer(&timer20);
		changed = !0;
	}
	if (IsExpiredTimer(&timer20,20000) != 0)
	{
		if (count != 0)
		{
			count --;
		}
		ResetTimer(&timer20);
	}
	if (IsExpiredTimer(&timer05,500) != 0)
	{
		ResetTimer(&timer05);
		on = !on;
		changed = !0;
		showFull(BLACK);
		if (on != 0)
		{
			for (uint8_t i = 0; i < count; i++)
			{
				for (uint8_t k = 0; k <  strips[i].number; k++)
				{
					put2pixels(GREEN,k + strips[i].from);
				}
			}
			put2pixels(getPowerColor(),0);
		}
	}
	return changed;
}


static void dispYellowStrips(const uint8_t nstrips)
{
	dispStrips(YELLOW,nstrips);
}
/**
 * @brief Yellow phase is 5-4-3-2-1-go phase for going out of pitlane. Starts at -T1-5s
 * @param init is nonzero if it's first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t yellowPhase(const uint8_t init)
{
	static uint8_t nstrips = 0;
	uint8_t changed = 0;
	static uint32_t timer;
	if (init != 0)
	{
		nstrips = 1;
		ResetTimer(&timer);
		dispYellowStrips(nstrips);
		changed = !0;
	}
	if (IsExpiredTimer(&timer,1000) != 0)
	{
		changed = !0;
		nstrips = (nstrips < 5) ? nstrips + 1 : 5;
		dispYellowStrips(nstrips);
		ResetTimer(&timer);
	}
	return changed;
}

/**
 * @brief Green-to-black phase is phase after going out of pitlane. Geen light goes off after 2s and
 * only 2 leds are displaying voltage. Starts at -T1
 * @param init is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t green2Black(const uint8_t init)
{
	uint8_t changed = 0;
	static States_t state = STATE_GB_GREEN;
	static uint32_t timer;
	static uint8_t on = !0;
	if (init != 0)
	{
		showFull(GREEN);
		changed = !0;
		ResetTimer(&timer);
		state = STATE_GB_GREEN;
	}
	switch (state)
	{
	case STATE_GB_GREEN:
		if (IsExpiredTimer(&timer,2000) != 0)
		{
		  showFull(BLACK);
			changed = !0;
			state = STATE_GB_BLACK;
			ResetTimer(&timer);
			on = 0;
		}
		break;
	case STATE_GB_BLACK:
		if (IsExpiredTimer(&timer,500) != 0)
		{
			on = !on;
			changed = !0;
			showFull(BLACK);
			ResetTimer(&timer);
			if (on != 0)
			{
				put2pixels(getPowerColor(),0);
			}
		}
		break;
	default:
		break;
	}
	return changed;
}

/**
 * @brief Red pre-phase is 2 red blinks at -T2. It's time when the driver must be at the intermediate point
 * @param init is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t redPre(const uint8_t init)
{
	return blinkTwice(init,RED,getPowerColor);
}

static void dispRedStrips(const uint8_t nstrips)
{
	dispStrips(RED,nstrips);
}

/**
 * @brief The main phase 5-4-3-2-1-go traffic light before crossing start/finish line. Starts at -5s and ends at 0
 * @param init is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */

static uint8_t tlight(const uint8_t init)
{
	static uint8_t nstrips;
	uint8_t changed = 0;
	static uint32_t timer;
	if (init != 0)
	{
		nstrips = 5;
		ResetTimer(&timer);
		dispRedStrips(nstrips);
		changed = !0;
	}
	if (IsExpiredTimer(&timer,1000) != 0)
	{
		changed = !0;
		nstrips = (nstrips == 0) ? 0 : nstrips - 1;
		dispRedStrips(nstrips);
		ResetTimer(&timer);
	}
	return changed;
}
/**
 * @brief Last phase : two green blinks starting from 0 (TSEQ)
 * @param init is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t lastGreenPhase(const uint8_t init)
{
   return blinkTwice(init,GREEN,getPowerColor);
}

typedef uint8_t (*pPhase_t)(const uint8_t init);
/**
 * @brief Phase description
 */
typedef struct
{
  const uint32_t  start; /**< Phase start time. ms from turning the stick on */
  pPhase_t pPhase; /**< Phase function. Phase function is called every 100ms from @ref start until next phase starts */
}Phase_desc_t;

/**
 * @brief phase function processor. Calls needed phase function every time this function is called starting with
 * @brief @ref Phase_desc_t::start field of it's descriptor.
 * @param desc Descriptor table
 * @param nextState out parameter. Becomes nonzero when all table is processed
 * @param ms millisecons from the start
 * @return  nonzero means the led strip must be updated
 */
static uint8_t processPhaseTable(const Phase_desc_t * const desc,uint8_t * const nextState,const uint32_t ms)
{
	if (nextState != NULL)
	{
		*nextState = 0;
	}
	static uint8_t oldPos = 99;
	uint8_t i = 0;
	for (; desc[i].pPhase != NULL;i++)
	{
		if (ms >= desc[i].start && ms < desc[i + 1].start)
		{
			break;
		}
	}
	uint8_t changed = 0;
	if (desc[i].pPhase == NULL)
	{
		if (nextState != NULL)
		{
			*nextState = !0;
			oldPos = 99;
		}
	}
	else
	{
		changed = desc[i].pPhase(i != oldPos);
		oldPos = i;
	}
	return changed;
}


enum
{
	TLIGHT_WORK_TIME = 5, /**< Tlight working time. is used at yellow and tlight phases */
	LAST_BLINK_TIME = 2   /**< Time of the last green phase */
};
/**
 * @brief Prepares and executes phase table for the pit stop mode
 * @param[in] ms millisecons from the start
 * @param[out] nextState out parameter. Becomes nonzero when all table is processed
 * @return nonzero means the led strip must be updated
 */
static uint8_t pitStopCalc(uint32_t const ms,uint8_t * const nextState)
{

  const uint8_t * const conf = eeemuGetValue();
  const uint8_t tseq = conf[CH_TSEQ];
  const uint8_t t1 = conf[CH_T1];
  const uint8_t t2 = conf[CH_T2];

  const uint16_t yellowPhase_start = tseq - t1 - TLIGHT_WORK_TIME;
  const uint16_t gbPhase_start = tseq - t1;
  const uint16_t preRed_start = tseq - t2;
  const uint16_t tlightPhase_start = tseq - TLIGHT_WORK_TIME;
  const uint16_t green2Phase_start = tseq;
  const uint16_t nullPhaseStart = tseq + LAST_BLINK_TIME;
  const Phase_desc_t desc[]=
  {
      {0 * S,greenPhase},
      {yellowPhase_start * S,yellowPhase},
      {gbPhase_start * S, green2Black},
      {preRed_start * S, redPre},
	    {tlightPhase_start * S, tlight},
      {green2Phase_start * S, lastGreenPhase},
      {nullPhaseStart * S,NULL}
  };
  return processPhaseTable(desc,nextState,ms);
}

/**
 * @brief tlight mode yellow pre-phase, two blinks at the start
 * @param init  is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t yellowTPre(const uint8_t init)
{
	return blinkTwice(init,YELLOW,getPowerColor);
}

/**
 * @brief Slalom traffic light. After the last strip goes on all light goes off at a random time interval
 * @param init  is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t tlightTMain(const uint8_t init)
{
	static uint8_t nstrips;
	uint8_t changed = 0;
	static uint32_t timer;
	static uint32_t lastStripTime;
	static States_t state = STATE_TLIGHT_SLALOM;
	if (init != 0)
	{
		const uint8_t * const conf = eeemuGetValue();
		const uint8_t rMin = conf[CH_TLIGHT_MIN];
		const uint8_t rMax = conf[CH_TLIGHT_MAX];
		lastStripTime = genRandom(rMin,rMax) * 100 - 100;
		nstrips = 1;
		ResetTimer(&timer);
		dispRedStrips(nstrips);
		changed = !0;
		state = STATE_TLIGHT_SLALOM;
	}
	else
	{
		switch(state)
		{
		case STATE_TLIGHT_SLALOM:
			if (IsExpiredTimer(&timer,1000) != 0)
			{
				changed = !0;
				nstrips++;
				dispRedStrips(nstrips);
				ResetTimer(&timer);
				if (nstrips >= 5)
				{
					state = STATE_TLIGHT_SHOW_RANDOM;
				}
			}
			break;
		case STATE_TLIGHT_SHOW_RANDOM:
			if (IsExpiredTimer(&timer,lastStripTime) != 0)
			{
				showFull(BLACK);
				changed = !0;
			}
			break;
		default:
			break;
		}
	}
	return changed;
}

/**
 * @brief shows the current bat voltage
 * @param init is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t showPower(const uint8_t init)
{
	uint8_t changed = 0;
	if (init != 0)
	{
		showFull(BLACK);
		put2pixels(getPowerColor(),0);
		changed = !0;
	}
	return changed;
}

/**
 * @brief fills and executes state table for the slalom traffic light
 * @param ms milliseconds from the start
 * @param nextState out param. Is non-zero if sequence is finished and we need to go to the next (@ref lock) state
 * @return nonzero means the led strip must be updated
 */

static uint8_t tlightMode(uint32_t const ms,uint8_t * const nextState)
{
	const uint8_t * const conf = eeemuGetValue();
	const uint8_t rMax = conf[CH_TLIGHT_MAX];
	const uint16_t mainTlightModeStart = 2;
	const uint16_t powerPhaseStart = ((rMax % 10 == 0) ? rMax / 10 : rMax / 10 + 1) + 5 + 2 + 5;
	const Phase_desc_t desc[]=
	{
			{0 * S,yellowTPre},
			{mainTlightModeStart * S,tlightTMain},
			{powerPhaseStart * S,showPower},
			{(powerPhaseStart + 5) * S,NULL}
	};
	return processPhaseTable(desc,nextState,ms);
}

/**
 * @brief Red phase of "Podnos" mode
 * @param init is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */
static uint8_t podnosRed(const uint8_t init)
{
  return showFullWithInit(RED,init);
}

/**
 * @brief Green phase of "Podnos" mode
 * @param init is nonzero at the first run
 * @return nonzero means the led strip must be updated
 */

static uint8_t podnosGreen(const uint8_t init)
{
  return showFullWithInit(GREEN,init);
}


/**
 * @brief fills and executes state table for stop-and-go ("Podnos") mode.
 * @param ms milliseconds from the start
 * @param nextState out param. Is non-zero if sequence is finished and we need to go to the next (@ref lock) state
 * @return nonzero means the led strip must be updated
 */

static uint8_t podnosMode(uint32_t const ms,uint8_t * const nextState)
{
  const uint8_t * const conf = eeemuGetValue();
  const uint8_t duration = conf[CH_PODNOS_MODE_TIME];
  const uint16_t powerPhaseStart = duration + 10;
  const Phase_desc_t desc[]=
  {
      {0 * S,podnosRed},
      {duration * S, podnosGreen},
      {powerPhaseStart * S,showPower},
      {(powerPhaseStart + 5) * S,NULL}
  };
  return processPhaseTable(desc,nextState,ms);
}

/**
 * @fn uint8_t show234(const uint8_t)
 * @brief This is one of safety car script stages. It shows 2,3,4 lights using @ref YELLOW color
 *
 * @param init is nonzero at the first run
 * @return nonzero if led strip must be updated
 */
static uint8_t show234(const uint8_t init)
{
	uint8_t changed = 0;
	if (init != 0)
	{
		showFull(BLACK);
		dispStrip(YELLOW, 2 - 1);
		dispStrip(YELLOW, 3 - 1);
		dispStrip(YELLOW, 4 - 1);
		changed = !0;
	}
	return changed;
}

/**
 * @fn uint8_t show1245(const uint8_t)
 * @brief This is one of safety car script stages. It shows 1,2,4,5 lights using @ref YELLOW color
 *
 * @param init is nonzero at the first run
 * @return nonzero if led strip must be updated
 */
static uint8_t show1245(const uint8_t init)
{
	uint8_t changed = 0;
	if (init != 0)
	{
		showFull(BLACK);
		dispStrip(YELLOW, 1 - 1);
		dispStrip(YELLOW, 2 - 1);
		dispStrip(YELLOW, 4 - 1);
		dispStrip(YELLOW, 5 - 1);
		changed = !0;
	}
	return changed;
}

/**
 * @fn uint8_t show15(const uint8_t)
 * @brief This is one of safety car script stages. It shows 1,5 lights using @ref YELLOW color
 *
 * @param init is nonzero at the first run
 * @return nonzero if led strip must be updated
 */
static uint8_t show15(const uint8_t init)
{
	uint8_t changed = 0;
	if (init != 0)
	{
		showFull(BLACK);
		dispStrip(YELLOW, 1 - 1);
		dispStrip(YELLOW, 5 - 1);
		changed = !0;
	}
	return changed;
}

/**
 * @fn uint8_t showOff(const uint8_t)
 * @brief Turns led strip off
 *
 * @param init is nonzero at the first run
 * @return nonzero if led strip must be updated
 *
 */
static uint8_t showOff(const uint8_t init)
{
	return showFullWithInit(BLACK,init);
}

/**
 * @fn uint8_t scMode(const uint32_t)
 * @brief Safety Car mode implementation. Safety Car mode is non-setupable endless mode. The sequence can be simply read from the desc variable
 * @param[in] ms milliseconds for start
 * @return nonzero if led strip must be updated
 */

static uint8_t scMode(uint32_t const ms)
{
  const Phase_desc_t desc[]=
  {
      {0   * S / 10, showOff},
      {10  * S / 10, show234},
      {20  * S / 10, showOff},
      {30  * S / 10, show1245},
      {35  * S / 10, showOff},
      {40  * S / 10, show1245},
      {45  * S / 10, showOff},
      {50  * S / 10, show15},
      {60  * S / 10, showOff},
      {70  * S / 10, show234},
      {75  * S / 10, showOff},
      {80  * S / 10, show234},
      {85  * S / 10, showOff},
      {90  * S / 10, show15},
      {100 * S / 10, showOff},
      {110 * S / 10, show1245},
      {115 * S / 10, showOff},
      {120 * S / 10, show1245},
      {125 * S / 10, showOff},
      {130 * S / 10, NULL}
  };

  static States_t state = STATE_IDLE;
  static uint32_t timer = 0;
  uint8_t tableEnded;
  static uint32_t currentMs = 0;
  uint8_t changed = 0;
  switch(state)
  {
  case STATE_IDLE:
	  changed =  showPower(!0);
	  ResetTimer(&timer);
	  state = STATE_SC_SHOW_POWER;
	  break;
  case STATE_SC_SHOW_POWER:
	  if (IsExpiredTimer(&timer,500) != 0)
	  {
		  currentMs = ms;
		  state = STATE_SC_MAIN;
	  }
	  break;
  case STATE_SC_MAIN:
	  changed = processPhaseTable(desc,&tableEnded,ms - currentMs);
	  if (tableEnded != 0)
	  {
		  currentMs = ms;
	  }
	  break;
  default:
	  break;
  }
  return changed;
}

static const uint32_t k2hOnMs = 300;
static const uint32_t k2hOffMs = 1700;
static const uint32_t workingTime = 105 * MIN + 30 * S;

static uint8_t dark15mPhase(const uint8_t init)
{
	static uint8_t onPhase = 0;
	static uint32_t timer = 0;
	uint8_t changed = 0;
	if ( 0 != init )
	{
		onPhase = !0;
		ResetTimer(&timer);
		showPower(!0);
		changed = !0;
	}
	else
	{
		if (0 != onPhase)
		{
			if (IsExpiredTimer(&timer,k2hOnMs) != 0)
			{
				showFull(BLACK);
				ResetTimer(&timer);
				onPhase = 0;
				changed = !0;
			}
		}
		else
		{
			if (IsExpiredTimer(&timer,k2hOffMs) != 0)
			{
				showPower(!0);
				ResetTimer(&timer);
				onPhase = !0;
				changed = !0;
			}
		}
	}
	return changed;
}

static uint8_t k2hMode(uint32_t const ms,uint8_t * const nextState)
{
	const Phase_desc_t desc[] =
	{
			{0 * S / 10,dark15mPhase},
			{1 * MIN / 10, NULL}
	};
	uint8_t tableEnded = 0;
	static uint32_t currentMs = 0;
	const uint8_t changed = processPhaseTable(desc,&tableEnded,ms - currentMs);
	if (tableEnded != 0)
	{
		currentMs = ms;
	}
	*nextState = ms > workingTime;
	return changed;
}

/**
 * @brief Lock mode. Is shown at the end of all sequences (@ref config, @ref tlightMode, etc) to show that the stick is on and remind to switch it off
 * @return nonzero means the led strip must be updated
 */
static uint8_t lock(void)
{
  static States_t state = STATE_LOCK_START;
  static uint32_t timer;
  uint8_t changed = 0;
  switch (state)
  {
    case STATE_LOCK_START:
      ResetTimer(&timer);
      showFull(BLACK);
      changed = !0;
      state = STATE_LOCK_OFF;
      break;
    case STATE_LOCK_OFF:
      if (IsExpiredTimer(&timer,19000) != 0)
      {
        ResetTimer(&timer);
        setBrightness(eeemuGetValue()[CH_BRIGHTNESS]);

        for (uint8_t i = 1; i < 10; i++)
        {
          put2pixels(DARK_RED,i);
        }
        put2pixels(getPowerColor(),0);
        for (uint8_t i = NLEDS /2 - 11; i < NLEDS / 2; i++)
        {
        	put2pixels(DARK_RED,i);
        }
        changed = !0;
        state = STATE_LOCK_ON;
      }
      break;
    case STATE_LOCK_ON:
      if (IsExpiredTimer(&timer,1000) != 0)
      {
        ResetTimer(&timer);
        showFull(BLACK);
        changed = !0;
        state = STATE_LOCK_OFF;
      }
      break;
    default:
      break;
  }
  return changed;
}

/**
 * @brief The entry point  of the led control. Switches to pitstick, config mode or tlight
 * @param ms milliseconds from the start
 */
void led_control(uint32_t const ms)
{

	static States_t state = STATE_IDLE;
	uint8_t isPressed = IsPressed(B_CONFIG);
	const uint8_t * const conf = eeemuGetValue();
	const Working_Mode_t mode = conf[CH_MODE];
	const uint8_t brightness = conf[CH_BRIGHTNESS];
	setBrightness(brightness);
	uint8_t changed = 0;
	uint8_t nextState = 0;
	switch(state)
	{
	case STATE_IDLE:
		if (isPressed != 0)
		{
			state = STATE_CONFIG;
		}
		else
		{
			switch(mode)
			{
			case MODE_PIT:
				state = STATE_PIT;
				break;
			case MODE_TLIGHT:
				state = STATE_TLIGHT;
				break;
			case MODE_PODNOS:
			  state = STATE_PODNOS;
			  break;
			case MODE_SC:
				state = STATE_SC;
				break;
			case MODE_KART2H:
				state = STATE_K2H;
				break;
			default:
				state = STATE_LOCK;
				break;
			}
		}
		break;
	case STATE_CONFIG:
		changed = config(&nextState);
		if (nextState != 0)
		{
		  state = STATE_LOCK;
		}
		break;
	case STATE_PIT:
		changed = pitStopCalc(ms,&nextState);
		if (nextState != 0)
		{
			state = STATE_LOCK;
		}
		break;
	case STATE_TLIGHT:
		changed = tlightMode(ms,&nextState);
		if (nextState != 0)
		{
			state = STATE_LOCK;
		}
		break;
	case STATE_PODNOS:
	  changed = podnosMode(ms,&nextState);
	  if (nextState != 0)
	  {
	    state = STATE_LOCK;
	  }
	  break;
	case STATE_SC: /* This state never ends */
		changed = scMode(ms);
		break;
	case STATE_K2H:
		changed = k2hMode(ms,&nextState);
		if (nextState != 0)
		{
			state = STATE_LOCK;
		}
		break;

	case STATE_LOCK:
	  changed = lock();
	  break;
	default:
		break;
	}
	if (changed != 0)
	{
		sendDataToStrip();
	}
}

