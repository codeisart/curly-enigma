#pragma once

#include "FastLED.h"
#include "radio.h"
#include "lib8tion.h"

// suit is 5m, 32 leds per meter.
#define NUM_LEDS 32*5
#define DATA_PIN 9
#define CLOCK_PIN 10
#define BRIGHTNESS          64
#define FRAMES_PER_SECOND  120
#define PATTERN_TIME 10


// Define the array of leds
CRGB leds[NUM_LEDS];

void fadeall() { for (int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void nextPattern();
void rainbow();
void addGlitter(fract8 chanceOfGlitter);
void rainbowWithGlitter();
void confetti();
void sinelon();
void bpm();
void juggle();
void cylon();


typedef void(*DrawFunct)();
struct DrawStruct
{
	const char* name;;
	DrawFunct funct;
	DrawStruct(const char* n, DrawFunct f) 
		: name(n), funct(f) 
	{}
};

#define F(f) { DrawStruct{ #f, f } }

// List of patterns to cycle through.  Each is defined as a separate function below.
DrawStruct gPatterns[] = 
{
	//F(rainbowWithGlitter), 
	F(cylon),
	F(confetti), 
	F(bpm) 
};

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void setupLeds()
{
	LEDS.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
	LEDS.setBrightness(BRIGHTNESS);
	LEDS.setMaxPowerInVoltsAndMilliamps(5, 500);
}

void doLeds()
{
	// Call the current pattern function once, updating the 'leds' array
	gPatterns[gCurrentPatternNumber].funct();

	// send the 'leds' array out to the actual LED strip
	FastLED.show();
	// insert a delay to keep the framerate modest
	FastLED.delay(1000 / FRAMES_PER_SECOND);

	EVERY_N_MILLISECONDS(20) { setHue(gHue + 1); } // slowly cycle the "base color" through the rainbow

	// server only, unless we are running with sync disabled.
#ifndef DISABLE_SYNC
	if (gSettings.role == eSender)
#endif// DISABLE_SYNC
	{
		EVERY_N_SECONDS(1) { sendCmd(eChangeHue, gHue); }		// broadcast the hue to try and keep clients roughtly in sync.
		EVERY_N_SECONDS(PATTERN_TIME) { nextPattern(); }		// change patterns periodically
	}	
}

void setHue(uint8_t hue)
{
	// Disabled as this changes a lot.
	// If we are the server.... send it out.
	//if( gSettings.role == eSender )
	//	sendCmd(eChangeHue, hue);

	gHue = hue;	
}

void changePattern(int x)
{
	LOGF("changePattern %d\n", x);	

	if (x < ARRAY_SIZE(gPatterns))
	{
		if (gSettings.role == eSender)
			sendCmd(eChangePattern, x);

		// add one to the current pattern number, and wrap around at the end
		gCurrentPatternNumber = x;
		LOGF("pattern = %s\n", gPatterns[x].name);
	}
}

void nextPattern()
{
	uint8_t next = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
	LOGF("nextpattern = %s\n", gPatterns[next].name );

	if (gSettings.role == eSender)
		sendCmd(eChangePattern, next);

	// add one to the current pattern number, and wrap around at the end
	gCurrentPatternNumber = next;
}

void rainbow()
{
	// FastLED's built-in rainbow generator
	fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void addGlitter(fract8 chanceOfGlitter)
{
	if (random8() < chanceOfGlitter) {
		leds[random16(NUM_LEDS)] += CRGB::White;
	}
}

void rainbowWithGlitter()
{
	// built-in FastLED rainbow, plus some random sparkly glitter
	rainbow();
	addGlitter(80);
}

void confetti()
{
	// random colored speckles that blink in and fade smoothly
	fadeToBlackBy(leds, NUM_LEDS, 10);
	int pos = random16(NUM_LEDS);
	leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void bpm()
{
	// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
	uint8_t BeatsPerMinute = 62;
	CRGBPalette16 palette = PartyColors_p;
	uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
	for (int i = 0; i < NUM_LEDS; i++) { //9948
		leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
	}
}

void cylon()
{
	//EVERY_N_MILLISECONDS(20)
	{
		fadeToBlackBy(leds, NUM_LEDS, 40);
		static int dir = 1;
		static int pos = 0;

		if (pos + dir >= NUM_LEDS || pos + dir < 0)
			dir = -dir;

		pos += dir;
		leds[pos] = CHSV(gHue++, 255, 255);
	}
}