#include <FastLED.h>

// ---------------------------
// Matrix & LED Setup (7 columns x 10 rows)
// ---------------------------
#define MATRIX_WIDTH 7    // columns
#define MATRIX_HEIGHT 10  // rows
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN 22       // Must match your working Tetris sketch
#define BRIGHTNESS 26     // ~10% brightness (10% of 255 ~ 26)

CRGB leds[NUM_LEDS];

// LED Matrix Mapping (from Tetris sketch; 10 rows x 7 columns)
// Each entry gives the LED index for that (row, col)
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
// Define 16 Test Colors
// ---------------------------
CRGB testColors[16] = {
  CRGB::Red,
  CRGB::Green,
  CRGB::Blue,
  CRGB::Yellow,
  CRGB::Magenta,
  CRGB::Cyan,
  CRGB(255,165,0),     // Orange
  CRGB::Purple,
  CRGB(255,192,203),   // Pink
  CRGB(191,255,0),     // Lime (approx)
  CRGB::Aqua,
  CRGB(255,215,0),     // Gold
  CRGB(238,130,238),   // Violet
  CRGB(75,0,130),      // Indigo
  CRGB::White,
  CRGB(128,128,128)    // Gray
};

int currentColorIndex = 0;  // Start with first color

// ---------------------------
// Helper function: Clear all LEDs
// ---------------------------
void clearAll() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// ---------------------------
// Test 1: Full Screen Test
// ---------------------------
void fullScreenTest(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  delay(1000);
  clearAll();
}

// ---------------------------
// Test 2: Column Test
// ---------------------------
void columnTest(CRGB color) {
  // For each column (0 to MATRIX_WIDTH - 1)
  for (int col = 0; col < MATRIX_WIDTH; col++) {
    clearAll();
    // Light up every pixel in this column.
    for (int row = 0; row < MATRIX_HEIGHT; row++) {
      leds[matrix[row][col]] = color;
    }
    FastLED.show();
    delay(200);
  }
  clearAll();
}

// ---------------------------
// Test 3: Row Test
// ---------------------------
void rowTest(CRGB color) {
  // For each row (0 to MATRIX_HEIGHT - 1)
  for (int row = 0; row < MATRIX_HEIGHT; row++) {
    clearAll();
    // Light up every pixel in this row.
    for (int col = 0; col < MATRIX_WIDTH; col++) {
      leds[matrix[row][col]] = color;
    }
    FastLED.show();
    delay(200);
  }
  clearAll();
}

// ---------------------------
// Test 4: Pixel Test (Numeric Order)
// ---------------------------
void pixelTest(CRGB color) {
  clearAll();
  // Light each pixel one by one in numeric order.
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
    FastLED.show();
    delay(50);
    // Optionally, turn off the pixel after delay.
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  clearAll();
}

// ---------------------------
// Main Loop
// ---------------------------
void setup() {
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  // Clear screen and flash white as startup indicator.
  clearAll();
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(200);
  clearAll();
}

void loop() {
  CRGB color = testColors[currentColorIndex];
  
  // Run the tests in order.
  fullScreenTest(color);
  columnTest(color);
  rowTest(color);
  pixelTest(color);
  
  // Move to next color (wrap around after 16).
  currentColorIndex = (currentColorIndex + 1) % 16;
}
