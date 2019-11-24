/**
 * @file led_strip.c
 * @author Mykhaylo Shcherbak
 * @em mikl74@yahoo.com
 * @date 20-11-2019
 * @version 1.00
 * @brief Contains common functions implementations for led strip control.
 */
#include <stddef.h>
#include "led_strip.h"
#include "project_conf.h"
#include "clock.h"
#include "rgbw.h"

/**
 * @brief The current brightness level
 */
static uint8_t Brightness = 0;

/**
 * @brief Strip description. Is used for traffic light emulation
 */
typedef struct
{
	uint8_t from; /**< The number of the first led of the strip */
	uint8_t number; /**< Strip length */
}Strip_t;

/**
 * @brief Color names to values conversion
 */
static const Led_t colors[]={
                 /* R,G,B,W */
		[BLACK]=			{0,   0,  0,  0 },
		[RED] =				{255, 0,  0,  0 },
		[GREEN]= 			{0,   255,0,  0 },
		[BLUE]=				{0,   0,  255,0 },
		[WHITE]=			{0,   0,  0, 255},
		[YELLOW]=			{255, 255,0,  15},
		[MAGENTA]=    		{255, 0,  255,20},
		[CYAN]=       		{0, 255,  255,20},
		[DARK_RED]=   		{50,  0,  0,  0 },
		[ORANGE]=			{100,50,  0,  0 },
		[REDDER]=			{200,50,  0,  0 }
};

/**
 * @brief Buffer for storing led pixel data
 */
static Led_t leds[NLEDS];

/**
 * @brief brightness level descriptor. For fractional numbers multiplier and divider is used.
 */
typedef struct
{
	uint8_t mul;
	uint8_t div;
}Div_t;


static inline uint8_t getBrightness(void)
{
	return Brightness;
}

/**
 * @brief Applies current brightness level to the led values
 * @param in Input led data
 * @param out Calculated led data
 */
static void applyBrightness(const Led_t * const in, Led_t * const out)
{
	const Div_t coefs[MAX_BRIGHNESS_LEVELS] = {{1,8},{1,4},{1,2},{1,1}};
	uint16_t tmp;
	const uint8_t brighness = getBrightness();

	tmp = in->R * coefs[brighness].mul;
	out->R = tmp / coefs[brighness].div;

	tmp = in->G * coefs[brighness].mul;
	out->G = tmp / coefs[brighness].div;

	tmp = in->B * coefs[brighness].mul;
	out->B = tmp / coefs[brighness].div;

	tmp = in->W * coefs[brighness].mul;
	out->W = tmp / coefs[brighness].div;
}

/**
 * @brief Displays one digit as a led column. Each 3 leds are separated by a black one
 * @param color Color index
 * @param digit The digit to display. It may not be a real (0-9) digit but more.
 * @param column Column where to display
 */

static void displayDigit(const Colors_t color,uint8_t digit,uint8_t column)
{
  column = (column > 1) ? 1 : column;
  const uint8_t maxDigit = ((NLEDS / 2) / 4) * 3 + (NLEDS / 2) % 4;
  digit = (digit > maxDigit) ? maxDigit : digit;
  int16_t pos = column * (NLEDS -1);
  const int8_t inc = (column == 0) ? 1 : -1;
  for (uint8_t i = 0; i < digit / 3; i++)
  {
    for (uint8_t k = 0; k < 3; k++)
    {
      applyBrightness(&colors[color],leds + pos);
      pos+=inc;
    }
    leds[pos] = colors[BLACK];
    pos+=inc;
  }

  for (uint8_t i = 0; i < digit % 3; i++)
  {
    applyBrightness(&colors[color],leds + pos);
    pos+=inc;
  }
  if (inc > 0)
  {
	  for (;pos < NLEDS / 2;pos += inc)
	  {
		  leds[pos]=colors[BLACK];
	  }
  }
  else
  {
	  for (;pos >= NLEDS / 2; pos += inc)
	  {
		  leds[pos]=colors[BLACK];
	  }
  }
}

void setBrightness(const uint8_t brighness_a)
{
	if (brighness_a < MAX_BRIGHNESS_LEVELS)
	{
		Brightness = brighness_a;
	}
	else
	{
		Brightness = MAX_BRIGHNESS_LEVELS - 1;
	}
}

void putPixel(const uint8_t row,const uint8_t pos, const Colors_t color)
{
	if (pos < NLEDS / 2)
	{
		switch(row)
		{
		case 0:
			applyBrightness(&colors[color],leds + pos);
			break;
		case 1:
			applyBrightness(&colors[color],leds + NLEDS - pos - 1);
			break;
		default:
			break;

		}
	}
}

void put2pixels(const Colors_t color,const uint8_t pos)
{
	putPixel(0,pos,color);
	putPixel(1,pos,color);
}


void showFull(const Colors_t color)
{
  Led_t led;
  applyBrightness(&colors[color],&led);
  for (uint8_t i = 0; i < NLEDS; i++)
  {
    leds[i] = led;
  }
}

void displayNumber(const Colors_t colorL, const Colors_t colorR,const uint8_t Number)
{
  displayDigit(colorR, Number % 10, 1);
  displayDigit(colorL, Number / 10, 0);
}


void dispStrips(const Colors_t color,const uint8_t nstrips)
{
	static const Strip_t strips[5]=
	{
			{.from =  1,	.number = 10},
			{.from = 16,	.number = 10},
			{.from = 31,	.number = 10},
			{.from = 46,	.number = 10},
			{.from = 61,    .number = 10}
	};
	showFull(BLACK);
	for (uint8_t i = 0; i < ((nstrips > 5) ? 5 : nstrips); i++)
	{
		for (uint8_t k = 0; k < strips[i].number; k++)
		{
			put2pixels(color,k + strips[i].from);
		}
	}
}

typedef enum
{
	BLINKTWICE_BLINK, /**< State while stick is blinking */
	BLINKTWICE_OFF /**< State after the stick blinked and goes off */
}BlinkTwice_t;

uint8_t blinkTwice(const uint8_t init,const Colors_t color,pPowerColorFunc_t const getPowerColor)
{
  static BlinkTwice_t state = BLINKTWICE_BLINK;
  uint8_t changed = 0;
  static uint32_t timer;
  static uint8_t blinkCounter;
  static uint8_t on;
  if (init != 0)
  {
    ResetTimer(&timer);
    blinkCounter = 4;
    state = BLINKTWICE_BLINK;
    showFull(color);
    changed = !0;
  }
  switch (state)
  {

    case BLINKTWICE_BLINK:

      if (IsExpiredTimer(&timer,500) != 0)
      {
        ResetTimer(&timer);
        if (--blinkCounter > 0)
        {
          showFull(((blinkCounter & 1) != 0) ? BLACK: color);
        }
        else
        {
          state = BLINKTWICE_OFF;
          showFull(BLACK);
          ResetTimer(&timer);
          on = 0;
        }
        changed = !0;
      }
      break;
    case BLINKTWICE_OFF:
    	if (IsExpiredTimer(&timer, 500) != 0)
    	{
    		ResetTimer(&timer);
    		on = !on;
    		changed = !0;
    		showFull(BLACK);
    		if (on != 0 && getPowerColor != NULL)
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

void sendDataToStrip(void)
{
	displayStrip(leds);
}

