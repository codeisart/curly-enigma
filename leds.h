#pragma once

#include "FastLED.h"
#include "radio.h"

// suit is 5m, 32 leds per meter.
#define NUM_LEDS 32*5
#define DATA_PIN 9
#define CLOCK_PIN 10
#define BRIGHTNESS          64
#define FRAMES_PER_SECOND  120

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

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void(*SimplePatternList[])();
SimplePatternList gPatterns = { /*rainbow, rainbowWithGlitter,*/ confetti, /*sinelon, juggle,*/ bpm };

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
	gPatterns[gCurrentPatternNumber]();

	// send the 'leds' array out to the actual LED strip
	FastLED.show();
	// insert a delay to keep the framerate modest
	FastLED.delay(1000 / FRAMES_PER_SECOND);

	EVERY_N_MILLISECONDS(20) { setHue(gHue + 1); } // slowly cycle the "base color" through the rainbow

	// server only.
	if (gSettings.role == eSender)
	{
		EVERY_N_SECONDS(10) { nextPattern(); } // change patterns periodically
	}	
}

void setHue(uint8_t hue)
{
	// If we are the server.... send it out.
	//if( gSettings.role == eSender )
	//	sendCmd(eChangeHue, hue);

	gHue = hue;	
}

void changePattern(int x)
{
	LOGLN("changePattern");	

	if (gSettings.role == eSender)
		sendCmd(eChangePattern, x);

	// add one to the current pattern number, and wrap around at the end
	gCurrentPatternNumber = x;
}

void nextPattern()
{
	uint8_t next = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
	LOG("nextPattern=");
	LOGLN(next);

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

void sinelon()
{
	// a colored dot sweeping back and forth, with fading trails
	fadeToBlackBy(leds, NUM_LEDS, 20);
	int pos = beatsin16(13, 0, NUM_LEDS);
	leds[pos] += CHSV(gHue, 255, 192);
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

void juggle() {
	// eight colored dots, weaving in and out of sync with each other
	fadeToBlackBy(leds, NUM_LEDS, 20);
	byte dothue = 0;
	for (int i = 0; i < 8; i++) {
		leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
		dothue += 32;
	}
}