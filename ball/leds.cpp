#include <Adafruit_NeoPixel.h>
#include <Metro.h>
#include <FastLED.h>

extern bool megan_enable;
extern bool jo_enable;
extern float battery_voltage;
extern bool tethered;

bool testing = false;


#define NUM_LEDS 9
CRGB leds[NUM_LEDS];


const TProgmemRGBPalette16 jo_colors = {
  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple,

  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple,

  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple,

  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple,
  CRGB::Purple
};

const TProgmemRGBPalette16 megan_colors = {
  CRGB::Red,
  CRGB::Red,
  CRGB::Red,
  CRGB::Red,

  CRGB::Red,
  CRGB::Red,
  CRGB::Red,
  CRGB::Red,

  CRGB::Red,
  CRGB::Red,
  CRGB::Red,
  CRGB::Red,

  CRGB::Red,
  CRGB::Red,
  CRGB::Red,
  CRGB::Red
};

CRGBPalette16 colors_to_use;
CRGBPalette16 currentPalette;
TBlendType currentBlending = LINEARBLEND;


uint8_t brightness = 255;

Adafruit_NeoPixel pixels1(NUM_LEDS, 10, NEO_GRB + NEO_KHZ800);

Metro led_timer = Metro(100);
Metro hue_timer = Metro(50);
Metro effect_timer = Metro(20);

// slowly rotating base color
static uint8_t gHue = 0;

void rainbow() {

  int deltahue = gHue;
  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] += ColorFromPalette(currentPalette, gHue, brightness, currentBlending);
    deltahue += 7;
  }
}


void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}
void addMeganGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::Red;
  }
}
void addJoGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::Purple;
  }
}
void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}


void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += ColorFromPalette(currentPalette, gHue, brightness, currentBlending);
}



void leds_init(void) {
  pixels1.begin();
  pixels1.clear();
  pixels1.show();

  if (megan_enable)
    colors_to_use = megan_colors;
  if (jo_enable)
    colors_to_use = jo_colors;
}

static void render_to_output(void) {
  //we use fastled for all LED handling, but need to output via Adafruit
  //fastled does not support the nRF52 platform we are on
  for (int i = 0; i < NUM_LEDS; i++)
    pixels1.setPixelColor((i + 3) % NUM_LEDS, pixels1.Color(leds[i].r, leds[i].g, leds[i].b));

  pixels1.show();
}


void leds_red(void) {
  for (int i = 0; i < NUM_LEDS; i++)
    pixels1.setPixelColor((i + 3) % NUM_LEDS, pixels1.Color(64, 0, 0));
  pixels1.show();
}

void leds_clear(void) {
  for (int i = 0; i < NUM_LEDS; i++)
    pixels1.setPixelColor(i, pixels1.Color(0, 0, 0));
  pixels1.show();
}

void leds_update(int mode) {
  static int pos = 0;

  if (effect_timer.check()) {
    EVERY_N_MILLISECONDS(20) {
      gHue++;
    }
    EVERY_N_MILLISECONDS(80) {
      pos++;
    }

    static int offset = 0;
    if (mode == 0) {
      currentPalette = colors_to_use;
      float val = (exp(sin((millis() - offset) / 2000.0 * PI)) - 0.36787944) * 108.0;

      CRGB faded_color = ColorFromPalette(currentPalette, gHue, brightness, currentBlending);
      faded_color.nscale8_video(val);

      for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = faded_color;

    } else {
      //control where the animation restarts
      offset = millis();
    }

    if (mode == 1) {
      currentPalette = colors_to_use;
      if (jo_enable) {
        //rainbowWithGlitter();
        addJoGlitter(80);
      }
      if (megan_enable) {
        addMeganGlitter(80);
      }
    }

    if (mode == 2) {
      if (tethered == false || testing == true) {


        if (jo_enable) {

          fadeToBlackBy(leds, NUM_LEDS, 20);
          if (pos > NUM_LEDS) pos = 0;
          leds[pos] += CHSV(gHue, 255, 192);
          addGlitter(50);
        }
        if (megan_enable) {
          currentPalette = HeatColors_p;
          //currentPalette = LavaColors_p ;
          fadeToBlackBy(leds, NUM_LEDS, 20);
          addMeganGlitter(50);
          if (pos > NUM_LEDS) pos = 0;
          leds[pos] += ColorFromPalette(currentPalette, gHue, brightness, currentBlending);
        }
      } else {
        //battery meter

        int pos_led = constrain(map(battery_voltage, 3.6, 4.2, 0, 9), 0, 9);
        int pos_color = map(battery_voltage * 10, 36, 42, 0, 96);

        for (int i = 0; i < NUM_LEDS; i++) {
          if (i < pos_color) leds[i] = CHSV(pos_color, 255, 32);
          else leds[i] = CHSV(pos_color, 255, 0);
        }
      }
    }
  }

  //output the array
  render_to_output();

  //dim everything a little bit
  EVERY_N_MILLISECONDS(10) {
    fadeToBlackBy(leds, NUM_LEDS, 10);
  }
}