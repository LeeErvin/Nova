#include <FastLED.h>
#include <math.h>

// ---------------------------
// Board & LED Setup (7 columns x 10 rows)
// ---------------------------
#define MATRIX_WIDTH 7    // columns
#define MATRIX_HEIGHT 10  // rows
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN 22       // Use the same as your working Tetris sketch!
#define BRIGHTNESS 20     // ~40% brightness

CRGB leds[NUM_LEDS];

// LED Matrix Mapping (from the Tetris sketch; 10 rows x 7 columns)
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
// Global Color Definitions
// ---------------------------
CRGB trunkColor = CRGB(80, 40, 20);    // Brown for trunk and ground
CRGB leafGreen   = CRGB(0, 100, 0);     // Base green for leaves
// Fall target colors (randomly chosen for each leaf) – from orange to maroon.
CRGB fallOptions[3] = { CRGB(255,140,0), CRGB(200,50,0), CRGB(128,0,0) };
CRGB branchColor = CRGB(80, 40, 20);     // For branches

// ---------------------------
// Animation Timing Constants
// ---------------------------
const int growthFrames   = 60;   // 15 sec
const int springFrames   = 60;   // 15 sec (full, green canopy)
const int seasonalFrames = 30;   // 7.5 sec (transition to fall colors)
const int fallingFrames  = 20;   // 5 sec (leaves fall faster)
const unsigned long FRAME_DELAY = 250;  // 250ms per frame
const unsigned long BLANK_DELAY = 2000;   // 2 sec pause (final scene held)
const unsigned long totalRun = (growthFrames + springFrames + seasonalFrames + fallingFrames) * FRAME_DELAY; // 170 frames (~42.5 sec)

// Fully grown tree parameters:
const int fullTrunkHeight = 6;  
const int trunkTopFull = MATRIX_HEIGHT - fullTrunkHeight; // 10 - 6 = 4
// Increase canopy size: fully grown canopy radius = 3.5
const float canopyRadius = 3.5;

// ---------------------------
// Leaf Structure for Individual Canopy Leaves (Phases 3 & 4)
// ---------------------------
struct Leaf {
  int row;
  int col;
  CRGB initialColor;      // Unique shade of green
  CRGB fallTargetColor;   // Random fall color
  float fallOffset;       // How many rows this leaf will fall (2.0 to 6.0)
};

Leaf leaves[28];  // Maximum canopy leaves (depends on area)
int leafCount = 0;
bool leavesInitialized = false;  // Set during Phase 3 initialization

// Ground carpet: For each column, store the fallen leaf color (if any)
CRGB groundCarpet[MATRIX_WIDTH];

// ---------------------------
// Phase 1: Growth Animation
// ---------------------------
void drawGrowthFrame(int frame) {
  // Growth factor: 0.0 to 1.0 over growthFrames.
  float growth = frame / float(growthFrames - 1);
  
  // Clear screen.
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Draw ground (row 9).
  for (int c = 0; c < MATRIX_WIDTH; c++) {
    leds[matrix[MATRIX_HEIGHT - 1][c]] = trunkColor;
  }
  
  // Compute trunk height: from 1 row to fullTrunkHeight.
  int trunkHeight = 1 + int(growth * (fullTrunkHeight - 1) + 0.5);
  int trunkTop = MATRIX_HEIGHT - trunkHeight;  // e.g., 10 - trunkHeight
  
  // Draw trunk as a 2-pixel–wide column (columns 3 and 4).
  for (int r = trunkTop; r < MATRIX_HEIGHT; r++) {
    leds[matrix[r][3]] = trunkColor;
    leds[matrix[r][4]] = trunkColor;
  }
  
  // Set canopy center at top of trunk so leaves attach to the tree.
  float canopyCenterRow = trunkTop;
  float canopyCenterCol = 3.5;  // center between columns 3 and 4
  
  // Draw continuous canopy if growth > 0.3.
  if (growth > 0.3) {
    float currentCanopyRadius = (growth - 0.3) * (canopyRadius / 0.7);  // scales from 0 to canopyRadius
    for (int r = 0; r < trunkTop; r++) {
      for (int c = 0; c < MATRIX_WIDTH; c++) {
        float dr = r - canopyCenterRow;
        float dc = c - canopyCenterCol;
        if (sqrt(dr * dr + dc * dc) <= currentCanopyRadius) {
          leds[matrix[r][c]] = leafGreen;
        }
      }
    }
  }
}

// ---------------------------
// Phase 2: Spring – Full Green Canopy
// ---------------------------
void drawSpringFrame() {
  // Draw trunk (rows trunkTopFull to 9) as a 2-pixel–wide column.
  for (int r = trunkTopFull; r < MATRIX_HEIGHT; r++) {
    leds[matrix[r][3]] = trunkColor;
    leds[matrix[r][4]] = trunkColor;
  }
  // Draw ground (row 9).
  for (int c = 0; c < MATRIX_WIDTH; c++) {
    leds[matrix[MATRIX_HEIGHT - 1][c]] = trunkColor;
  }
  // Draw fixed branches on row trunkTopFull (row 4) at columns 1 and 5.
  if (trunkTopFull < MATRIX_HEIGHT) {
    leds[matrix[trunkTopFull][1]] = branchColor;
    leds[matrix[trunkTopFull][5]] = branchColor;
  }
  
  // Draw the full canopy in pure green.
  float canopyCenterRow = trunkTopFull;
  float canopyCenterCol = 3.5;
  for (int r = 0; r < trunkTopFull; r++) {
    for (int c = 0; c < MATRIX_WIDTH; c++) {
      if (sqrt((r - canopyCenterRow)*(r - canopyCenterRow) + (c - canopyCenterCol)*(c - canopyCenterCol)) <= canopyRadius) {
        leds[matrix[r][c]] = leafGreen;
      }
    }
  }
}

// ---------------------------
// Phase 3: Seasonal Change – Leaves change color individually
// ---------------------------
void drawSeasonalFrame(int frame) {
  // Tree is fully grown; draw trunk and ground.
  for (int r = trunkTopFull; r < MATRIX_HEIGHT; r++) {
    leds[matrix[r][3]] = trunkColor;
    leds[matrix[r][4]] = trunkColor;
  }
  for (int c = 0; c < MATRIX_WIDTH; c++) {
    leds[matrix[MATRIX_HEIGHT - 1][c]] = trunkColor;
  }
  // Draw fixed branch pixels on row trunkTopFull (row 4) at columns 1 and 5.
  if (trunkTopFull < MATRIX_HEIGHT) {
    leds[matrix[trunkTopFull][1]] = branchColor;
    leds[matrix[trunkTopFull][5]] = branchColor;
  }
  
  // Initialize leaves if not yet done.
  if (!leavesInitialized) {
    leafCount = 0;
    for (int r = 0; r < trunkTopFull; r++) {
      for (int c = 0; c < MATRIX_WIDTH; c++) {
        float dr = r - trunkTopFull;
        float dc = c - 3.5;
        if (sqrt(dr * dr + dc * dc) <= canopyRadius) {
          leaves[leafCount].row = r;
          leaves[leafCount].col = c;
          leaves[leafCount].initialColor = CRGB(random(0,30), random(70,150), random(0,30));
          int idx = random(0, 3);
          leaves[leafCount].fallTargetColor = fallOptions[idx];
          leaves[leafCount].fallOffset = random(20, 61) / 10.0; // between 2.0 and 6.0
          leafCount++;
        }
      }
    }
    leavesInitialized = true;
    // Initialize ground carpet to black.
    for (int c = 0; c < MATRIX_WIDTH; c++) {
      groundCarpet[c] = CRGB::Black;
    }
  }
  
  // For each leaf, blend its color from its initial green to its fall target.
  uint8_t blendAmount = (frame * 255) / (seasonalFrames - 1);
  for (int i = 0; i < leafCount; i++) {
    CRGB blended = blend(leaves[i].initialColor, leaves[i].fallTargetColor, blendAmount);
    leds[matrix[leaves[i].row][leaves[i].col]] = blended;
  }
}

// ---------------------------
// Phase 4: Falling Leaves – Leaves fall individually forming a carpet
// ---------------------------
void drawFallingFrame(int frame) {
  // Clear LED buffer.
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Draw trunk and ground.
  for (int r = trunkTopFull; r < MATRIX_HEIGHT; r++) {
    leds[matrix[r][3]] = trunkColor;
    leds[matrix[r][4]] = trunkColor;
  }
  for (int c = 0; c < MATRIX_WIDTH; c++) {
    leds[matrix[MATRIX_HEIGHT - 1][c]] = trunkColor;
  }
  // Draw fixed branches: for example, on row trunkTopFull - 1 (row 3) at columns 2 and 4.
  if (trunkTopFull - 1 >= 0) {
    leds[matrix[trunkTopFull - 1][2]] = branchColor;
    leds[matrix[trunkTopFull - 1][4]] = branchColor;
  }
  
  // In this phase, each leaf falls faster.
  // Compute falling progress (0.0 to 1.0) over fallingFrames.
  float progress = frame / float(fallingFrames - 1);
  // For each leaf, compute new row.
  for (int i = 0; i < leafCount; i++) {
    int newRow = leaves[i].row + int(progress * leaves[i].fallOffset + 0.5);
    if (newRow >= MATRIX_HEIGHT - 1) {
      // Leaf has reached the ground; update the ground carpet.
      groundCarpet[leaves[i].col] = leaves[i].fallTargetColor;
    } else {
      leds[matrix[newRow][leaves[i].col]] = leaves[i].fallTargetColor;
    }
  }
  // Draw the ground carpet on the ground (non-trunk columns).
  for (int c = 0; c < MATRIX_WIDTH; c++) {
    if (c != 3 && c != 4 && groundCarpet[c] != CRGB::Black) {
      leds[matrix[MATRIX_HEIGHT - 1][c]] = groundCarpet[c];
    }
  }
}

// ---------------------------
// Main Animation Timing Variable
// ---------------------------
unsigned long animationStart = 0;

// ---------------------------
// Setup & Main Loop
// ---------------------------
void setup() {
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  // Clear screen on startup.
  FastLED.clear();
  FastLED.show();
  
  // Flash white briefly as a startup indicator.
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(200);
  FastLED.clear();
  FastLED.show();
  
  leavesInitialized = false;
  animationStart = millis();
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long elapsed = currentMillis - animationStart;
  
  // Total run time = (growthFrames + springFrames + seasonalFrames + fallingFrames) * FRAME_DELAY.
  const unsigned long runTime = (growthFrames + springFrames + seasonalFrames + fallingFrames) * FRAME_DELAY;
  
  if (elapsed < runTime) {
    if (elapsed < growthFrames * FRAME_DELAY) {
      // Phase 1: Growth.
      int frame = elapsed / FRAME_DELAY;
      drawGrowthFrame(frame);
    }
    else if (elapsed < (growthFrames + springFrames) * FRAME_DELAY) {
      // Phase 2: Spring – full green canopy.
      drawSpringFrame();
    }
    else if (elapsed < (growthFrames + springFrames + seasonalFrames) * FRAME_DELAY) {
      // Phase 3: Seasonal change.
      int frame = (elapsed - (growthFrames + springFrames) * FRAME_DELAY) / FRAME_DELAY;
      drawSeasonalFrame(frame);
    }
    else {
      // Phase 4: Falling leaves.
      int frame = (elapsed - (growthFrames + springFrames + seasonalFrames) * FRAME_DELAY) / FRAME_DELAY;
      drawFallingFrame(frame);
    }
    FastLED.show();
    delay(50); // Small delay for smooth updates.
  } else {
    // Instead of blanking, keep the final fallen scene on display for BLANK_DELAY.
    delay(BLANK_DELAY);
    // Restart cycle.
    leavesInitialized = false; // Reinitialize leaves.
    animationStart = millis();
  }
}
