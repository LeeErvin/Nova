/*
 * Ball & Brick Game Example for Nova Board (Auto-Play Mode with Win/Loss Effects)
 *
 * Button Connections:
 * - RESET Button: connected to pin 6 (used to reset the game manually)
 *
 * In auto-play mode the paddle automatically tracks the ball with a slight random variation.
 *
 * - Win Condition: When all bricks are removed, the brick color rotates between Red, Green, and Purple.
 * - Loss Condition: When the ball falls below the paddle, the paddle is shown in RED and a 2-second pause occurs.
 *
 * License:
 * This project is open-source hardware and software, released under the MIT License.
 */

#include <FastLED.h>

// LED matrix settings
#define NUM_LEDS 70
#define DATA_PIN 22
#define BRIGHTNESS 10

CRGB leds[NUM_LEDS];

// Button pin for reset (other buttons are not used in auto-play)
#define RESET_BUTTON 6

// Matrix Layout (mapping from [row][column] to LED index)
int matrix[10][7] = {
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

// Define brick colors and global brick color variables.
CRGB brickColors[3] = {CRGB::Red, CRGB::Green, CRGB::Purple};
uint8_t currentBrickColorIndex = 0;
CRGB currentBrickColor = brickColors[currentBrickColorIndex];

// Game properties
struct Ball {
  int x, y;
  int dx, dy; // Direction of the ball
};

struct Paddle {
  int x, y; // Paddle position
};

Paddle paddle;
Ball ball;
bool bricks[3][7]; // True if a brick is present (only the top 3 rows)
bool gameOver;

// Speed settings
unsigned long gameSpeed = 100;      // Game speed in milliseconds
unsigned long lastUpdateTime = 0;   // Last time the game was updated

// -----------------------
// Function Prototypes
// -----------------------
void initializeGame();
void moveBall();
void drawBricks();
void drawBall();
void drawPaddle();
void drawPaddleLoss();

// -----------------------
// Setup
// -----------------------
void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  randomSeed(analogRead(0));
  initializeGame();
}

// -----------------------
// Main Loop
// -----------------------
void loop() {
  unsigned long currentMillis = millis();

  // Allow manual reset with the RESET button if desired.
  if (gameOver && digitalRead(RESET_BUTTON) == HIGH) {
    initializeGame();
    return;
  }

  // --- Auto-play: Paddle automatically tracks the ball with a random offset ---
  // Paddle width is 3; ideally, paddle.x should be ball.x - 1.
  // Add a random offset (-1, 0, or +1) so the paddle doesn't always center perfectly.
  int randomOffset = random(-1, 2); // Returns -1, 0, or 1
  int targetPaddleX = ball.x - 1 + randomOffset;
  // Clamp targetPaddleX to valid range [0, 4] (max starting index is 7 - paddle width 3)
  if (targetPaddleX < 0) {
    targetPaddleX = 0;
  } else if (targetPaddleX > 4) {
    targetPaddleX = 4;
  }
  paddle.x = targetPaddleX;
  // -------------------------------------------------------------------------

  // Update game state only if enough time has passed.
  if (currentMillis - lastUpdateTime >= gameSpeed) {
    moveBall();
    lastUpdateTime = currentMillis;
    
    // Update LED display
    FastLED.clear();
    drawBricks();
    drawPaddle();
    drawBall();
    FastLED.show();
  }
}

// -----------------------
// Initialize or Reset the Game
// -----------------------
void initializeGame() {
  // Initialize paddle (bottom row at index 9)
  paddle.x = 3;  // Start in the middle
  paddle.y = 9;
  
  // Initialize ball: positioned above the paddle
  ball.x = 3;
  ball.y = 8;
  ball.dx = 1;  // Start moving to the right
  ball.dy = -1; // Start moving upward
  gameOver = false;
  
  // (Re)Initialize bricks: fill the top 3 rows with bricks.
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 7; j++) {
      bricks[i][j] = true;
    }
  }
}

// -----------------------
// Move the Ball and Check for Collisions
// -----------------------
void moveBall() {
  // Update ball position
  ball.x += ball.dx;
  ball.y += ball.dy;
  
  // Check for wall collisions (left/right)
  if (ball.x < 0 || ball.x >= 7) {
    ball.dx = -ball.dx;
    ball.x = constrain(ball.x, 0, 6);
  }
  // Check for top wall collision
  if (ball.y < 0) {
    ball.dy = -ball.dy;
    ball.y = 0;
  }
  
  // Check for paddle collision
  if (ball.y == paddle.y && ball.x >= paddle.x && ball.x < paddle.x + 3) {
    ball.dy = -ball.dy;
    // Adjust horizontal direction based on where it hits the paddle
    int hitPos = ball.x - paddle.x; // 0=left, 1=center, 2=right
    if (hitPos == 0) {
      ball.dx = -1;
    } else if (hitPos == 2) {
      ball.dx = 1;
    } else {
      ball.dx = 0;
    }
    ball.y = paddle.y - 1; // Position ball above the paddle
  }
  
  // Check for brick collision (only in top 3 rows)
  if (ball.y < 3 && bricks[ball.y][ball.x]) {
    bricks[ball.y][ball.x] = false;
    ball.dy = -ball.dy;
    // Adjust ball position slightly to avoid sticking.
    if (ball.dy < 0) {
      ball.y--;
    } else {
      ball.y++;
    }
    ball.y = max(ball.y, 0);
  }
  
  // --- Check for Win Condition ---
  // If all bricks are removed, rotate the brick color and reset the game.
  bool allCleared = true;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 7; j++) {
      if (bricks[i][j]) {
        allCleared = false;
        break;
      }
    }
    if (!allCleared) break;
  }
  if (allCleared) {
    currentBrickColorIndex = (currentBrickColorIndex + 1) % 3;
    currentBrickColor = brickColors[currentBrickColorIndex];
    initializeGame();
    return;
  }
  
  // --- Check for Loss Condition ---
  // If the ball falls below the paddle, show a loss state.
  if (ball.y >= 10) {
    gameOver = true;
    FastLED.clear();
    drawBricks();
    drawBall();
    // Draw the paddle in RED to indicate a loss.
    drawPaddleLoss();
    FastLED.show();
    delay(2000);  // Pause for 2 seconds for the observer.
    initializeGame();
    return;
  }
}

// -----------------------
// Draw the Bricks on the LED Matrix using the current brick color
// -----------------------
void drawBricks() {
  for (int i = 0; i < 3; i++) {  // Only top 3 rows have bricks.
    for (int j = 0; j < 7; j++) {
      if (bricks[i][j]) {
        leds[matrix[i][j]] = currentBrickColor;
      }
    }
  }
}

// -----------------------
// Draw the Ball on the LED Matrix
// -----------------------
void drawBall() {
  if (ball.y < 10 && ball.x < 7) {
    leds[matrix[ball.y][ball.x]] = CRGB::White;
  }
}

// -----------------------
// Draw the Paddle in BLUE (Normal State)
// -----------------------
void drawPaddle() {
  for (int i = 0; i < 3; i++) {
    if (paddle.x + i < 7) {
      leds[matrix[paddle.y][paddle.x + i]] = CRGB::Blue;
    }
  }
}

// -----------------------
// Draw the Paddle in RED (Loss State)
// -----------------------
void drawPaddleLoss() {
  for (int i = 0; i < 3; i++) {
    if (paddle.x + i < 7) {
      leds[matrix[paddle.y][paddle.x + i]] = CRGB::Red;
    }
  }
}
