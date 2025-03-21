#include <FastLED.h>
#include <math.h>

// ---------------------------
// Matrix & LED Setup (7 columns x 10 rows)
// ---------------------------
#define MATRIX_WIDTH 7    // columns
#define MATRIX_HEIGHT 10  // rows
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN 22       // Must match your working Tetris sketch
#define BRIGHTNESS 15     // Set overall brightness to 15

CRGB leds[NUM_LEDS];

// LED Matrix Mapping (from the Tetris sketch; 10 rows x 7 columns)
// Each inner array represents one row (0=top, 9=bottom).
int matrix[MATRIX_HEIGHT][MATRIX_WIDTH] = {
  {69, 50, 49, 30, 29, 10, 9},
  {68, 51, 48, 31, 28, 11, 8},
  {67, 52, 47, 32, 27, 12, 7},
  {66, 53, 46, 33, 26, 13, 6},
  {65, 54, 45, 34, 25, 14, 5},
  {64, 55, 44, 35, 24, 15, 4},
  {63, 56, 43, 36, 23, 16, 3},
  {62, 57, 42, 37, 22, 17, 2},
  {61, 58, 41, 38, 21, 18, 1},
  {60, 59, 40, 39, 20, 19, 0}
};

// ---------------------------
// Bouncing Line Parameters
// ---------------------------

// The line orientation cycles among four discrete states:
// 0: horizontal, 1: slash (/), 2: vertical, 3: backslash (\).
// It rotates every 50ms.
int orientationIndex = 0;

// The line’s center position (as floats) and its velocity.
float centerX, centerY;
float dx, dy;

// We now cycle through all hues. We'll update the hue every 500ms.
uint8_t currentHue = 0;
CRGB currentLineColor;

// ---------------------------
// Timer Intervals (in milliseconds)
// ---------------------------
const unsigned long orientationInterval = 50;   // Orientation update every 50ms.
const unsigned long positionInterval    = 200;  // Position update every 200ms.
const unsigned long hueInterval         = 500;  // Hue increments every 500ms.

unsigned long lastOrientationUpdate = 0;
unsigned long lastPositionUpdate = 0;
unsigned long lastHueUpdate = 0;

// ---------------------------
// Setup
// ---------------------------
void setup() {
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  // Initialize the bouncing line near the center.
  centerX = 3.0; // Valid center x range: keep at least 1 pixel inside (1..5)
  centerY = 5.0; // Valid center y range: (1..8)
  dx = 0.4;
  dy = 0.6;
  orientationIndex = 0;
  
  currentHue = 0;
  currentLineColor = CHSV(currentHue, 255, 255);
  
  lastOrientationUpdate = millis();
  lastPositionUpdate = millis();
  lastHueUpdate = millis();
  
  // Clear screen at startup.
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// ---------------------------
// Main Loop
// ---------------------------
void loop() {
  unsigned long currentMillis = millis();
  
  // --- Update Orientation ---
  if (currentMillis - lastOrientationUpdate >= orientationInterval) {
    orientationIndex = (orientationIndex + 1) % 4;
    lastOrientationUpdate = currentMillis;
  }
  
  // --- Update Position ---
  if (currentMillis - lastPositionUpdate >= positionInterval) {
    centerX += dx;
    centerY += dy;
    
    // Constrain center to remain at least 1 pixel away from borders.
    if (centerX < 1) {
      centerX = 1;
      dx = -dx + random(-10, 11) / 100.0;
    }
    if (centerX > MATRIX_WIDTH - 2) {
      centerX = MATRIX_WIDTH - 2;
      dx = -dx + random(-10, 11) / 100.0;
    }
    if (centerY < 1) {
      centerY = 1;
      dy = -dy + random(-10, 11) / 100.0;
    }
    if (centerY > MATRIX_HEIGHT - 2) {
      centerY = MATRIX_HEIGHT - 2;
      dy = -dy + random(-10, 11) / 100.0;
    }
    
    lastPositionUpdate = currentMillis;
  }
  
  // --- Update Color (Hue) ---
  if (currentMillis - lastHueUpdate >= hueInterval) {
    currentHue++;  // Increment hue.
    lastHueUpdate = currentMillis;
  }
  currentLineColor = CHSV(currentHue, 255, 255);
  
  // --- Draw the Bouncing Line ---
  // Clear the LED buffer.
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Determine the center pixel.
  int cx = round(centerX);
  int cy = round(centerY);
  
  // Determine the three pixel positions based on orientation.
  int x0, y0, x1, y1, x2, y2;
  switch (orientationIndex) {
    case 0: // Horizontal.
      x0 = cx - 1; y0 = cy;
      x1 = cx;     y1 = cy;
      x2 = cx + 1; y2 = cy;
      break;
    case 1: // Slash (/).
      x0 = cx - 1; y0 = cy + 1;
      x1 = cx;     y1 = cy;
      x2 = cx + 1; y2 = cy - 1;
      break;
    case 2: // Vertical.
      x0 = cx; y0 = cy - 1;
      x1 = cx; y1 = cy;
      x2 = cx; y2 = cy + 1;
      break;
    case 3: // Backslash (\).
      x0 = cx - 1; y0 = cy - 1;
      x1 = cx;     y1 = cy;
      x2 = cx + 1; y2 = cy + 1;
      break;
    default:
      x0 = cx - 1; y0 = cy;
      x1 = cx;     y1 = cy;
      x2 = cx + 1; y2 = cy;
      break;
  }
  
  // Light each pixel if it is within bounds.
  if (x0 >= 0 && x0 < MATRIX_WIDTH && y0 >= 0 && y0 < MATRIX_HEIGHT)
    leds[matrix[y0][x0]] = currentLineColor;
  if (x1 >= 0 && x1 < MATRIX_WIDTH && y1 >= 0 && y1 < MATRIX_HEIGHT)
    leds[matrix[y1][x1]] = currentLineColor;
  if (x2 >= 0 && x2 < MATRIX_WIDTH && y2 >= 0 && y2 < MATRIX_HEIGHT)
    leds[matrix[y2][x2]] = currentLineColor;
  
  FastLED.show();
  delay(10);
}
