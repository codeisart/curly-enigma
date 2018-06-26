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
#define PATTERN_TIME 60*3


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

int gForcePosition = -1;
void forcePosition(int pos)
{
	gForcePosition = pos;
}

typedef void(*DrawFunct)();
struct DrawStruct
{
	const char* name;
	uint16_t delayInMs;
	DrawFunct funct;
	DrawStruct(const char* n, DrawFunct f, int d=20) 
		: name(n), funct(f), delayInMs(d)
	{}
};

#define F(f,d) { DrawStruct{ #f, f, d } }

// List of patterns to cycle through.  Each is defined as a separate function below.
DrawStruct gPatterns[] = 
{
	//F(rainbowWithGlitter), 
	F(cylon,20),
	F(confetti,10), 
	F(bpm,1000/120) 
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
	static int thisdelay = 0;                                            // Standard delay
	EVERY_N_MILLIS_I(thistimer, thisdelay) {								     // Sets the original delay time.
		thistimer.setPeriod(gPatterns[gCurrentPatternNumber].delayInMs);              // This is how you update the delay value on the fly.
		gPatterns[gCurrentPatternNumber].funct();
	}	

	// send the 'leds' array out to the actual LED strip
	FastLED.show();
	// insert a delay to keep the framerate modest
	//FastLED.delay(1000 / FRAMES_PER_SECOND);

	EVERY_N_MILLISECONDS(20) { setHue(gHue + 1); } // slowly cycle the "base color" through the rainbow

	// server only, unless we are running with sync disabled.
#ifndef DISABLE_SYNC
	if (gSettings.role == eSender)
#endif// DISABLE_SYNC
	{
		// Sync ping, every second.
		EVERY_N_SECONDS(1) 
		{ 
			sendCmd(eChangeHue, gHue);
			sendCmd(eChangePattern, gCurrentPatternNumber);
		}		// broadcast the hue to try and keep clients roughtly in sync.
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
	//EVERY_N_MILLISECONDS(20)
	{
		rainbow();
		addGlitter(80);
	}
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

	//EVERY_N_MILLISECONDS(20)
	{
		uint8_t BeatsPerMinute = 62;
		CRGBPalette16 palette = PartyColors_p;
		uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
		for (int i = 0; i < NUM_LEDS; i++) { //9948
			leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
		}
	}
}

void cylon()
{

	static const int nPoints = 1;
	static int pixels[] = { 0, NUM_LEDS-1, NUM_LEDS/2, NUM_LEDS/3 };
	static int dirs[] = { 1,-1,-2, 2 };
	//EVERY_N_MILLISECONDS(20)
	{
		fadeToBlackBy(leds, NUM_LEDS, 20);
		CRGBPalette16 palette = RainbowColors_p;
		//static int dir = 1;
		//static int pos = 0;

		if (gForcePosition >= 0)
		{
			if (gForcePosition >= NUM_LEDS)
				gForcePosition = NUM_LEDS - 1;
			for (int i = 0; i < nPoints; ++i)
				pixels[i] = gForcePosition;
			
			gForcePosition = -1;
		}

		for (int i = 0; i < nPoints; ++i)
		{
			if (pixels[i] + dirs[i] >= NUM_LEDS || pixels[i] + dirs[i] < 0)
			{
				dirs[i] = -dirs[i];
				if (gSettings.role == eSender)
				{
					sendCmd(eSyncPosition, pixels[i]);
				}				
			}

			pixels[i] += dirs[i];
			leds[pixels[i]] = ColorFromPalette(palette, gHue++, 255, 255);
			//leds[pixels[i]] = CHSV(gHue*i, 255, 255);

		}
	}
}