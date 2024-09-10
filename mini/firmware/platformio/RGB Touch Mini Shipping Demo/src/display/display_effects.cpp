/*
Effects
*/

#include "display.h"

void MatrixDisplay::rainbow()
{
	// FastLED's built-in rainbow generator
	fill_rainbow_brightness(NUM_LEDS, currentHue, 2, 100);
}

void MatrixDisplay::rainbowWithGlitter()
{
	// built-in FastLED rainbow, plus some random sparkly glitter
	rainbow();
	addGlitter(80);
}

void MatrixDisplay::addGlitter(fract8 chanceOfGlitter)
{
	if (random8() < chanceOfGlitter)
	{
		leds[random16(NUM_LEDS)] += CRGB::White;
	}
}

void MatrixDisplay::confetti()
{
	// random colored speckles that blink in and fade smoothly
	fadeToBlackBy(leds, NUM_LEDS, 10);
	int pos = random16(NUM_LEDS);
	leds[pos] += CHSV(currentHue + random8(64), 200, 255);
}

void MatrixDisplay::sinelon()
{
	// a colored dot sweeping back and forth, with fading trails
	fadeToBlackBy(leds, NUM_LEDS, 20);
	int pos = beatsin16(33, 0, NUM_LEDS - 1);
	leds[pos] += CHSV(currentHue, 255, 192);
}

void MatrixDisplay::bpm()
{
	// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
	uint8_t BeatsPerMinute = 62;
	CRGBPalette16 palette = PartyColors_p;
	uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
	for (int i = 0; i < NUM_LEDS; i++)
	{ // 9948
		leds[i] = ColorFromPalette(palette, currentHue + (i * 2), beat - currentHue + (i * 10));
	}
}

void MatrixDisplay::juggle()
{
	// eight colored dots, weaving in and out of sync with each other
	fadeToBlackBy(leds, NUM_LEDS, 20);
	uint8_t dothue = 0;
	for (int i = 0; i < 8; i++)
	{
		leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
		dothue += 32;
	}
}

void MatrixDisplay::lines()
{
	fadeToBlackBy(leds, NUM_LEDS, 50);

	if (random(100) > 50)
		_matrix->drawFastHLine(0, random(12), 12, CRGBtoUint32(ColorFromPalette(RainbowColors_p, currentHue)));
	else
		_matrix->drawFastVLine(random(12), 0, 12, CRGBtoUint32(ColorFromPalette(RainbowColors_p, currentHue)));
}

void MatrixDisplay::show_effect(int index)
{
	next_redraw_step = 10;
	switch (settings.config.current_effect)
	{
	case 0:
		rainbowWithGlitter();
		break;
	case 1:
		confetti();
		break;
	case 2:
		sinelon();
		next_redraw_step = 15;
		break;
	case 3:
		juggle();
		break;
	case 4:
		lines();
		next_redraw_step = 60;
		break;
	case 5:
		fire();
		break;
	}

	currentHue++;
}

void MatrixDisplay::fill_rainbow_brightness(int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t val)
{
	CHSV hsv;
	hsv.hue = initialhue;
	hsv.val = val;
	// hsv.val = (millis() / 1000);
	hsv.sat = 240;
	for (int i = 0; i < numToFill; ++i)
	{
		leds[i] = hsv;
		hsv.hue += deltahue;
	}
}

void MatrixDisplay::fire()
{
	const uint8_t COOLING = 55;
	const uint8_t SPARKING = 120;
	bool gReverseDirection = false;

	// Array of temperature readings at each simulation cell
	static uint8_t heat[NUM_LEDS];

	// Step 1.  Cool down every cell a little
	for (int i = 0; i < NUM_LEDS; i++)
	{
		heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
	}

	// Step 2.  Heat from each cell drifts 'up' and diffuses a little
	for (int k = NUM_LEDS - 1; k >= 2; k--)
	{
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}

	// Step 3.  Randomly ignite new 'sparks' of heat near the bottom
	if (random8() < SPARKING)
	{
		int y = random8(7);
		heat[y] = qadd8(heat[y], random8(160, 255));
	}

	// Step 4.  Map from heat cells to LED colors
	for (int j = 0; j < NUM_LEDS; j++)
	{
		CRGB color = HeatColor(heat[j]);
		int pixelnumber;
		if (gReverseDirection)
		{
			pixelnumber = (NUM_LEDS - 1) - j;
		}
		else
		{
			pixelnumber = j;
		}
		leds[pixelnumber] = color;
	}
}
