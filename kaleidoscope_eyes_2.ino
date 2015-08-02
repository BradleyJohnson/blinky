/*
  NeoSpin07 - By Disciple, employing code and libraries provided at adafruit.com
              Released under Adafruit's licensing conditions.
  Trinket-sized sketch for sending spinning color patterns to NeoPixel rings, plus extra "wild" NeoPixels.
  Random numbers generate color combinations, soft or hard-edged color waves, spin speed and direction.
  Pixels organized as a series of rings.  12, 16, 24-pixel rings display seamless color waves.
  Randomized direction reversals are applied to even-odd ring groups.
  Red status LED on pin 1 blinks once before each pattern change.
  Connect pin (pad) 2 to ground to change pixel brightness to subdued nighttime level.
  "Wild pixels" that follow the last ring are treated as a partial ring.
  ("All colors off" pattern had a 1 in 64 chance of appearing, now filtered out.)
  Version 7, speeds computation by generating 1 ring pair and duplicating.
*/
#include <Adafruit_NeoPixel.h>

#define NEOPIX_OUT 0
#define LED 1
#define SWITCH_A 2
#define RING_COUNT 2
#define RING_SIZE 16
// Change to 12 if needed
#define WILD_PIXELS 0
#define MOTION_RATE 16
#define PATT_DURATION 384
#define DAY_BRITE 48
#define NITE_BRITE 20

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

// Reserve a buffer for NeoPixel rings plus wild pixels
Adafruit_NeoPixel npchain = Adafruit_NeoPixel(RING_COUNT * RING_SIZE + WILD_PIXELS, NEOPIX_OUT, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode(LED, OUTPUT);         // initialize LED pin as an output
  pinMode(SWITCH_A, INPUT);     // initialize SWITCH_A pin as input
  digitalWrite(SWITCH_A, HIGH); // with a pullup

  npchain.begin();
  npchain.show();  // Initialize all pixels to 'off'
}

void loop() {
  static long rnumb = 0;
  int waittime, cyclenum;

  npchain.setBrightness(NITE_BRITE + (DAY_BRITE - NITE_BRITE) * (digitalRead(SWITCH_A) == true)); // switch day/night brightness

  rnumb = random(0x80000000);                  // Get randomized bits for pixel effects

  if(rnumb & 0x66600) {                        // Reject patterns with all colors zeroed
    waittime = (MOTION_RATE << (rnumb & 3));   // 4 possible motion speeds
    for (cyclenum = PATT_DURATION >> (rnumb & 3); cyclenum > 0; --cyclenum) {  // slower cycle time = fewer cycles

      // Fill pixel buffer with color pattern
      patterngen(cyclenum, rnumb >> 2 & 7, rnumb >> 5 & 1, rnumb >> 6 & 7,
                 rnumb >> 9 & 15, rnumb >> 13 & 15, rnumb >> 17 & 15);

      if (cyclenum == 8 >> (rnumb & 3))
        digitalWrite(LED, HIGH);   // LED on before each pattern change

      npchain.show();              // Push buffer data to pixels
      delay(waittime);             // delay, inverse-proportional to total cycles
    }
    digitalWrite(LED, LOW);        // LED off
  }
}

void patterngen(int timestep, byte wavecount, byte squareflag, byte revflags, byte redflags, byte grnflags, byte bluflags) {
/*
  timestep = animated value, tells color patterns how many pixels to march forward in their spinning motion
  wavecount = number of color waves in a 24-pixel loop, values from 0-7 usually convert to factors of 24
  squareflag = 1 bit, soft triangle waves of color become hard square waves if true
  revflags = reverse flags, 3 bits, change the motion direction of odd/even NeoPixel rings
  ___flags = 4 bits, lower 2 control amplitude of color channel (lighter, darker, full, off), upper 2 control phase
*/
  int pxlpointer, pred, pgrn, pblu, index0, index1, tempval01;

  if(RING_SIZE == 16)  // Map random num to freqs = 1, 1, 2, 2, 4, 4, 8, 8 - good for 16-pixel rings
    wavecount = 1 << (wavecount >> 1);
  else {               // Map random num to freqs = 1, 2, 3, 4, 4, 6, 6, 12 - good for 12 and 24 rings
    if(wavecount < 4)
      wavecount += 1;
    else if(wavecount == 5)
      wavecount = 6;
    else if(wavecount == 7)
      wavecount = 12;
  }

  for(index0 = 0; index0 < RING_SIZE + RING_SIZE; ++index0) {  // Generate pixel values, 2 rings full

    if(revflags >> ((index0 / RING_SIZE) & 1) & 1)             // Is this pixel in a reverse-motion ring?
      index1 = (timestep - (index0 % RING_SIZE - RING_SIZE + 1)) % 24;  // Compute colors last-to-first
    else
      index1 = (timestep + (index0 % RING_SIZE)) % 24;  // Compute colors first-to-last

    pred = ((redflags >> 2 & 3) * 6) / wavecount;       // Determine phase of red wave
    tempval01 = (((index1 + pred) * wavecount) % 24) * 255 / 24; // Measure out red waves scaled to 24 pixels
    if(tempval01 > 127)                                 // Latter half of red wave?
      tempval01 = 255 - tempval01;                      // Make values descend in a 2-ramp triangle

    if(squareflag)
      tempval01 = (tempval01 > 63) * 127;               // Apply threshhold cutoff, triangle wave to square
    if(redflags & 2)                                    // Leave red strength at 50%?
      pred = tempval01 + (redflags << 7 & 128);         // Boost/don't boost overall value
    else
      pred = (tempval01 << 1) * (redflags & 1);         // Scale red wave to either 2x or 0

    pgrn = ((grnflags >> 2 & 3) * 6) / wavecount;       // Determine phase of grn wave
    tempval01 = (((index1 + pgrn) * wavecount) % 24) * 255 / 24; // Measure out grn waves scaled to 24 pixels
    if(tempval01 > 127)                                 // Latter half of grn wave?
      tempval01 = 255 - tempval01;                      // Make values descend in a 2-ramp triangle

    if(squareflag)
      tempval01 = (tempval01 > 63) * 127;               // Apply threshhold cutoff, triangle wave to square
    if(grnflags & 2)                                    // Leave grn strength at 50%?
      pgrn = tempval01 + (grnflags << 7 & 128);         // Boost/don't boost overall value
    else
      pgrn = (tempval01 << 1) * (grnflags & 1);         // Scale grn wave to either 2x or 0

    pblu = ((bluflags >> 2 & 3) * 6) / wavecount;       // Determine phase of blu wave
    tempval01 = (((index1 + pblu) * wavecount) % 24) * 255 / 24; // Measure out blu waves scaled to 24 pixels
    if(tempval01 > 127)                                 // Latter half of blu wave?
      tempval01 = 255 - tempval01;                      // Make values descend in a 2-ramp triangle

    if(squareflag)
      tempval01 = (tempval01 > 63) * 127;               // Apply threshhold cutoff, triangle wave to square
    if(bluflags & 2)                                    // Leave blu strength at 50%?
      pblu = tempval01 + (bluflags << 7 & 128);         // Boost/don't boost overall value
    else
      pblu = (tempval01 << 1) * (bluflags & 1);         // Scale blu wave to either 2x or 0

    for(tempval01 = index0; tempval01 < npchain.numPixels(); tempval01 += RING_SIZE + RING_SIZE) // Copy repeating pattern
      npchain.setPixelColor(tempval01, pred, pgrn, pblu);  // Finished color vals into output buffer
  }
}
