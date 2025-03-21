#include <FastLED.h>

// ---------------------------
// Board & LED Setup
// ---------------------------
#define BOARD_WIDTH 7
#define BOARD_HEIGHT 10
#define NUM_LEDS 70
#define DATA_PIN 22
#define BRIGHTNESS 50

CRGB leds[NUM_LEDS];

// LED Matrix Mapping (row-major; same as used in the Brick game example)
int matrix[BOARD_HEIGHT][BOARD_WIDTH] = {
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
// Game Board State
// ---------------------------
// Each cell holds a CRGB color; CRGB::Black means empty.
CRGB board[BOARD_HEIGHT][BOARD_WIDTH];

// ---------------------------
// Tetromino Definitions
// ---------------------------
// Three pieces: I, O, and T.
// Each tetromino is defined by 4 rotations and each rotation has 4 blocks
// (with x,y offsets relative to the piece's origin).
#define NUM_TETROMINO 3
const int tetrominoes[NUM_TETROMINO][4][4][2] = {
  // I Piece – Cyan
  {
    { {0,0}, {1,0}, {2,0}, {3,0} },
    { {0,0}, {0,1}, {0,2}, {0,3} },
    { {0,0}, {1,0}, {2,0}, {3,0} },
    { {0,0}, {0,1}, {0,2}, {0,3} }
  },
  // O Piece – Yellow
  {
    { {0,0}, {1,0}, {0,1}, {1,1} },
    { {0,0}, {1,0}, {0,1}, {1,1} },
    { {0,0}, {1,0}, {0,1}, {1,1} },
    { {0,0}, {1,0}, {0,1}, {1,1} }
  },
  // T Piece – Magenta
  {
    { {0,0}, {1,0}, {2,0}, {1,1} },
    { {1,0}, {0,1}, {1,1}, {1,2} },
    { {1,0}, {0,1}, {1,1}, {2,1} },
    { {0,1}, {1,0}, {1,1}, {1,2} }
  }
};

CRGB tetrominoColors[NUM_TETROMINO] = { CRGB::Cyan, CRGB::Yellow, CRGB::Magenta };

// ---------------------------
// Active Piece Structure
// ---------------------------
struct Piece {
  uint8_t type;      // 0: I, 1: O, 2: T
  uint8_t rotation;  // 0-3
  int x, y;          // Top-left of the piece's bounding box on the board
};

Piece currentPiece;

// ---------------------------
// Game Control Variables
// ---------------------------
bool gameOver = false;
unsigned long lastUpdateTime = 0;
unsigned long gameSpeed = 500;  // Time interval (ms) for piece drop

// Scoring constants for placement simulation.
#define HOLE_PENALTY 3
#define LINE_CLEAR_BONUS 10
#define MAX_HEIGHT_PENALTY 1

// ---------------------------
// Function Prototypes
// ---------------------------
void clearBoard();
void spawnPiece();
bool canPlace(Piece p, int newX, int newY, uint8_t newRotation);
void lockPiece();
void clearLines();
void updateDisplay();
void performStrategicMove();
void findBestPlacement(int *bestX, int *bestRotation);
int simulateCandidateScore(Piece candidate, int testX, int landingY, uint8_t testRot);
void gameOverSequence();

// ---------------------------
// Setup Function
// ---------------------------
void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  randomSeed(analogRead(0));
  clearBoard();
  spawnPiece();
}

// ---------------------------
// Main Loop
// ---------------------------
void loop() {
  if (gameOver) {
    gameOverSequence();
    return;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= gameSpeed) {
    // Use strategic moves (with occasional 10% random error).
    performStrategicMove();
    
    // Attempt to move the piece down.
    if (canPlace(currentPiece, currentPiece.x, currentPiece.y + 1, currentPiece.rotation)) {
      currentPiece.y++;
    } else {
      // Piece has landed—lock it onto the board.
      lockPiece();
      clearLines();
      spawnPiece();
    }
    
    lastUpdateTime = currentMillis;
    updateDisplay();
  }
}

// ---------------------------
// Clear the Game Board
// ---------------------------
void clearBoard() {
  for (int r = 0; r < BOARD_HEIGHT; r++) {
    for (int c = 0; c < BOARD_WIDTH; c++) {
      board[r][c] = CRGB::Black;
    }
  }
}

// ---------------------------
// Spawn a New Piece at the Top of the Board
// ---------------------------
void spawnPiece() {
  currentPiece.type = random(0, NUM_TETROMINO);
  currentPiece.rotation = 0;
  // Set initial horizontal position based on piece width.
  if (currentPiece.type == 0) {       // I piece (width = 4)
    currentPiece.x = (BOARD_WIDTH - 4) / 2;
  } else if (currentPiece.type == 1) {  // O piece (width = 2)
    currentPiece.x = (BOARD_WIDTH - 2) / 2;
  } else {                            // T piece (width = 3)
    currentPiece.x = (BOARD_WIDTH - 3) / 2;
  }
  currentPiece.y = 0;
  
  // If the new piece collides immediately, the game is over.
  if (!canPlace(currentPiece, currentPiece.x, currentPiece.y, currentPiece.rotation)) {
    gameOver = true;
  }
}

// ---------------------------
// Check if a Piece Can Be Placed at the Given Position/Rotation
// ---------------------------
bool canPlace(Piece p, int newX, int newY, uint8_t newRotation) {
  for (int i = 0; i < 4; i++) {
    int blockX = newX + tetrominoes[p.type][newRotation][i][0];
    int blockY = newY + tetrominoes[p.type][newRotation][i][1];
    // Check board boundaries.
    if (blockX < 0 || blockX >= BOARD_WIDTH || blockY < 0 || blockY >= BOARD_HEIGHT)
      return false;
    // Check for collision with locked pieces.
    if (board[blockY][blockX] != CRGB::Black)
      return false;
  }
  return true;
}

// ---------------------------
// Lock the Current Piece onto the Board
// ---------------------------
void lockPiece() {
  CRGB color = tetrominoColors[currentPiece.type];
  for (int i = 0; i < 4; i++) {
    int blockX = currentPiece.x + tetrominoes[currentPiece.type][currentPiece.rotation][i][0];
    int blockY = currentPiece.y + tetrominoes[currentPiece.type][currentPiece.rotation][i][1];
    if (blockY >= 0 && blockY < BOARD_HEIGHT && blockX >= 0 && blockX < BOARD_WIDTH)
      board[blockY][blockX] = color;
  }
}

// ---------------------------
// Clear Full Lines and Shift Above Rows Down
// ---------------------------
void clearLines() {
  for (int r = 0; r < BOARD_HEIGHT; r++) {
    bool fullLine = true;
    for (int c = 0; c < BOARD_WIDTH; c++) {
      if (board[r][c] == CRGB::Black) {
        fullLine = false;
        break;
      }
    }
    if (fullLine) {
      // Shift rows down from the cleared line.
      for (int row = r; row > 0; row--) {
        for (int c = 0; c < BOARD_WIDTH; c++) {
          board[row][c] = board[row - 1][c];
        }
      }
      // Clear the top row.
      for (int c = 0; c < BOARD_WIDTH; c++) {
        board[0][c] = CRGB::Black;
      }
    }
  }
}

// ---------------------------
// Update the LED Display from the Board and Active Piece
// ---------------------------
void updateDisplay() {
  FastLED.clear();
  // Draw locked board cells.
  for (int r = 0; r < BOARD_HEIGHT; r++) {
    for (int c = 0; c < BOARD_WIDTH; c++) {
      if (board[r][c] != CRGB::Black)
        leds[matrix[r][c]] = board[r][c];
    }
  }
  // Overlay the current falling piece.
  CRGB pieceColor = tetrominoColors[currentPiece.type];
  for (int i = 0; i < 4; i++) {
    int blockX = currentPiece.x + tetrominoes[currentPiece.type][currentPiece.rotation][i][0];
    int blockY = currentPiece.y + tetrominoes[currentPiece.type][currentPiece.rotation][i][1];
    if (blockX >= 0 && blockX < BOARD_WIDTH && blockY >= 0 && blockY < BOARD_HEIGHT)
      leds[matrix[blockY][blockX]] = pieceColor;
  }
  FastLED.show();
}

// ---------------------------
// Simulate Candidate Placement and Compute a Score
// ---------------------------
int simulateCandidateScore(Piece candidate, int testX, int landingY, uint8_t testRot) {
  CRGB temp[BOARD_HEIGHT][BOARD_WIDTH];
  // Copy current board.
  for (int r = 0; r < BOARD_HEIGHT; r++) {
    for (int c = 0; c < BOARD_WIDTH; c++) {
      temp[r][c] = board[r][c];
    }
  }
  // Simulate locking the candidate piece.
  for (int i = 0; i < 4; i++) {
    int blockX = testX + tetrominoes[candidate.type][testRot][i][0];
    int blockY = landingY + tetrominoes[candidate.type][testRot][i][1];
    if (blockY >= 0 && blockY < BOARD_HEIGHT && blockX >= 0 && blockX < BOARD_WIDTH) {
      temp[blockY][blockX] = tetrominoColors[candidate.type];
    }
  }
  
  // Count holes: for each column, any empty cell below a filled cell is a hole.
  int holes = 0;
  for (int col = 0; col < BOARD_WIDTH; col++) {
    bool foundBlock = false;
    for (int row = 0; row < BOARD_HEIGHT; row++) {
      if (temp[row][col] != CRGB::Black) {
        foundBlock = true;
      } else if (foundBlock) {
        holes++;
      }
    }
  }
  
  // Count full lines that would be cleared.
  int linesCleared = 0;
  for (int r = 0; r < BOARD_HEIGHT; r++) {
    bool fullLine = true;
    for (int c = 0; c < BOARD_WIDTH; c++) {
      if (temp[r][c] == CRGB::Black) {
        fullLine = false;
        break;
      }
    }
    if (fullLine) linesCleared++;
  }
  
  // Compute maximum column height after placement.
  int maxHeight = 0;
  for (int col = 0; col < BOARD_WIDTH; col++) {
    int colHeight = 0;
    for (int row = 0; row < BOARD_HEIGHT; row++) {
      if (temp[row][col] != CRGB::Black) {
        colHeight = BOARD_HEIGHT - row;
        break;
      }
    }
    if (colHeight > maxHeight)
      maxHeight = colHeight;
  }
  
  // Score: reward deeper landing and line clears; penalize holes and high stacks.
  int score = landingY - HOLE_PENALTY * holes + LINE_CLEAR_BONUS * linesCleared - MAX_HEIGHT_PENALTY * maxHeight;
  return score;
}

// ---------------------------
// Intelligent Move: Aim for the Best Placement with Occasional Random Error
// ---------------------------
void performStrategicMove() {
  // With a 10% chance, perform a random left/right move (simulate error).
  if (random(0, 100) < 10) {
    int move = random(0, 2); // 0: left, 1: right
    if (move == 0 && canPlace(currentPiece, currentPiece.x - 1, currentPiece.y, currentPiece.rotation))
      currentPiece.x--;
    else if (move == 1 && canPlace(currentPiece, currentPiece.x + 1, currentPiece.y, currentPiece.rotation))
      currentPiece.x++;
    return;
  }
  
  // Otherwise, determine best placement.
  int bestX, bestRot;
  findBestPlacement(&bestX, &bestRot);
  
  // If the piece's rotation does not match the best rotation,
  // try to rotate. If a direct rotation fails, attempt a simple wall kick.
  if (currentPiece.rotation != bestRot) {
    uint8_t newRot = (currentPiece.rotation + 1) % 4;
    if (canPlace(currentPiece, currentPiece.x, currentPiece.y, newRot)) {
      currentPiece.rotation = newRot;
    } else if (canPlace(currentPiece, currentPiece.x - 1, currentPiece.y, newRot)) {
      currentPiece.x--;
      currentPiece.rotation = newRot;
    } else if (canPlace(currentPiece, currentPiece.x + 1, currentPiece.y, newRot)) {
      currentPiece.x++;
      currentPiece.rotation = newRot;
    }
    // If rotation still isn't possible, fall through to horizontal movement.
  } else {
    // Then move horizontally toward the target column.
    if (currentPiece.x < bestX) {
      if (canPlace(currentPiece, currentPiece.x + 1, currentPiece.y, currentPiece.rotation))
        currentPiece.x++;
    } else if (currentPiece.x > bestX) {
      if (canPlace(currentPiece, currentPiece.x - 1, currentPiece.y, currentPiece.rotation))
        currentPiece.x--;
    }
  }
}

// ---------------------------
// Find Best Placement: Evaluate all valid positions for the current piece,
// and choose the one with the best score based on landing depth, holes, line clears, and board height.
// ---------------------------
void findBestPlacement(int *bestX, int *bestRotation) {
  int bestScore = -10000;
  int chosenX = currentPiece.x;
  int chosenRot = currentPiece.rotation;
  
  // Try each rotation option.
  for (uint8_t rot = 0; rot < 4; rot++) {
    // Determine horizontal bounds for this rotation.
    int maxOffset = 0;
    for (int i = 0; i < 4; i++) {
      int x_offset = tetrominoes[currentPiece.type][rot][i][0];
      if (x_offset > maxOffset)
        maxOffset = x_offset;
    }
    int maxX = BOARD_WIDTH - maxOffset; // Valid x positions: 0 to maxX-1.
    
    for (int x = 0; x < maxX; x++) {
      // Check if the piece can be initially placed at (x, 0) for this rotation.
      if (!canPlace(currentPiece, x, 0, rot))
        continue;
      // Simulate dropping from y = 0.
      int landingY = 0;
      while (canPlace(currentPiece, x, landingY + 1, rot)) {
        landingY++;
      }
      // Compute the candidate score.
      int score = simulateCandidateScore(currentPiece, x, landingY, rot);
      if (score > bestScore) {
        bestScore = score;
        chosenX = x;
        chosenRot = rot;
      }
    }
  }
  
  *bestX = chosenX;
  *bestRotation = chosenRot;
}

// ---------------------------
// Game Over Sequence: Flash Red and Reset
// ---------------------------
void gameOverSequence() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
  FastLED.show();
  delay(2000);
  clearBoard();
  gameOver = false;
  spawnPiece();
  updateDisplay();
}
