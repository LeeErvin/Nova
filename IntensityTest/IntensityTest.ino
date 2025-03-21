#include <FastLED.h>

// ---------------------------
// Matrix & LED Setup (7 columns x 10 rows)
// ---------------------------
#define MATRIX_WIDTH 7    // columns: 0..6
#define MATRIX_HEIGHT 10  // rows: 0..9
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN 22       // Must match your working Tetris sketch
// We'll use our own intensity scaling, so set brightness to 255.
#define FASTLED_BRIGHTNESS 255

CRGB leds[NUM_LEDS];

// LED Matrix mapping (from the Tetris sketch; 10 rows x 7 columns)
// Each entry gives the LED index for (row, col)
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
// Bouncing Line State Definition
// ---------------------------
struct LineState {
  int x;   // center column (0..6)
  int y;   // center row (0..9)
  int dx;  // direction x (one of -1, 0, 1; not both 0)
  int dy;  // direction y (one of -1, 0, 1)
};

LineState currentState;
LineState prevState[4];  // prevState[0] is most recent, prevState[3] is oldest

// Intensity levels (relative to a maximum of 15):
// We'll scale the chosen test color by these factors (scaled to 0-255).
// Main line: 15 -> 255, then 10 -> 170, 6 -> 102, 3 -> 51, 1 -> 17.
const int intensities[5] = {255, 170, 102, 51, 17};

// ---------------------------
// 16 Test Colors (will be cycled every 30 sec)
// ---------------------------
CRGB testColors[16] = {
  CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow,
  CRGB::Magenta, CRGB::Cyan, CRGB(255,165,0), CRGB::Purple,
  CRGB(255,192,203), CRGB(191,255,0), CRGB::Aqua,
  CRGB(255,215,0), CRGB(238,130,238), CRGB(75,0,130),
  CRGB::White, CRGB(128,128,128)
};
int currentColorIndex = 0;
unsigned long lastColorChange = 0;
const unsigned long COLOR_CHANGE_INTERVAL = 30000;  // 30 seconds

// ---------------------------
// Timing for the bouncing line update
// ---------------------------
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 50;  // 50ms per update

// ---------------------------
// Helper Functions
// ---------------------------

// Clear all LEDs
void clearLeds() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

// Scale a color by a given factor (scale 0..255; 255 means full intensity)
CRGB scaleColor(const CRGB &color, int scale) {
  CRGB c;
  c.r = (color.r * scale) / 255;
  c.g = (color.g * scale) / 255;
  c.b = (color.b * scale) / 255;
  return c;
}

// Draw a 3-pixel line for a given state using the current matrix mapping.
// The line is drawn along the state's direction: pixels at (x-dx, y-dy), (x, y), (x+dx, y+dy).
void drawLineState(LineState s, CRGB color) {
  int x1 = s.x - s.dx;
  int y1 = s.y - s.dy;
  int x2 = s.x;
  int y2 = s.y;
  int x3 = s.x + s.dx;
  int y3 = s.y + s.dy;
  
  if (x1 >= 0 && x1 < MATRIX_WIDTH && y1 >= 0 && y1 < MATRIX_HEIGHT)
    leds[matrix[y1][x1]] = color;
  if (x2 >= 0 && x2 < MATRIX_WIDTH && y2 >= 0 && y2 < MATRIX_HEIGHT)
    leds[matrix[y2][x2]] = color;
  if (x3 >= 0 && x3 < MATRIX_WIDTH && y3 >= 0 && y3 < MATRIX_HEIGHT)
    leds[matrix[y3][x3]] = color;
}

// Update the current line state with bouncing logic.
// The line's endpoints (center ± direction) must remain within bounds.
void updateState() {
  // If the left endpoint is out of bounds or right endpoint is out of bounds, reverse dx.
  if (currentState.x - currentState.dx < 0 || currentState.x + currentState.dx >= MATRIX_WIDTH)
    currentState.dx = -currentState.dx;
  // Similarly for vertical.
  if (currentState.y - currentState.dy < 0 || currentState.y + currentState.dy >= MATRIX_HEIGHT)
    currentState.dy = -currentState.dy;
  
  // Then update the center position.
  currentState.x += currentState.dx;
  currentState.y += currentState.dy;
}

void setup() {
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(FASTLED_BRIGHTNESS);
  
  clearLeds();
  FastLED.show();
  
  // Flash white as a startup indicator.
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(200);
  clearLeds();
  FastLED.show();
  
  // Initialize current state in the center of the screen.
  currentState.x = MATRIX_WIDTH / 2;  // For 7 columns, center is 3 (0-indexed).
  currentState.y = MATRIX_HEIGHT / 2; // For 10 rows, center is 5.
  
  // Choose a random direction from the 8 possibilities.
  int directions[8][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
    {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
  };
  int d = random(0, 8);
  currentState.dx = directions[d][0];
  currentState.dy = directions[d][1];
  
  // Initialize the previous states array to the current state.
  for (int i = 0; i < 4; i++) {
    prevState[i] = currentState;
  }
  
  currentColorIndex = 0;
  lastColorChange = millis();
  lastUpdate = millis();
}

void loop() {
  unsigned long now = millis();
  
  // Change the line color every 30 seconds.
  if (now - lastColorChange >= COLOR_CHANGE_INTERVAL) {
    currentColorIndex = (currentColorIndex + 1) % 16;
    lastColorChange = now;
  }
  
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    // Before updating, save the current state into the afterimage buffer.
    LineState oldState = currentState;  // current state's value before update.
    
    // Update the bouncing line state.
    updateState();
    
    // Shift previous states: oldest out.
    prevState[3] = prevState[2];
    prevState[2] = prevState[1];
    prevState[1] = prevState[0];
    prevState[0] = oldState;
    
    // Clear LED buffer.
    clearLeds();
    
    // Get the current test color.
    CRGB baseColor = testColors[currentColorIndex];
    
    // Draw the current line state with full intensity (15 → 255).
    CRGB c = scaleColor(baseColor, intensities[0]);
    drawLineState(currentState, c);
    
    // Draw previous states with decreasing intensity.
    c = scaleColor(baseColor, intensities[1]);
    drawLineState(prevState[0], c);
    c = scaleColor(baseColor, intensities[2]);
    drawLineState(prevState[1], c);
    c = scaleColor(baseColor, intensities[3]);
    drawLineState(prevState[2], c);
    c = scaleColor(baseColor, intensities[4]);
    drawLineState(prevState[3], c);
    
    FastLED.show();
    lastUpdate = now;
  }
}
