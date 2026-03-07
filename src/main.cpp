#include <Arduboy2.h>
#include <vertex.h>
#include <debug.h>
#define MAX_VERTICES 128
#define DEBUG DETAILED
#define SLOW_DRAW_SPEED 2
#define FAST_DRAW_SPEED 3

Arduboy2 arduboy;

enum gamestate
{
  START_SCREEN,
  PLAYING,
  FILL_ANIMATION,
  DEATH_ANIMATION,
  GAME_OVER
};

class Perimeter
{
public:
  vertex vertices[MAX_VERTICES];
  byte numVertices;

  void draw()
  {
    debug.log("RUN", "Perimeter draw", INFO);
    for (byte i = 0; i < numVertices; i++)
    {
      vertex v1 = vertices[i];
      vertex v2 = vertices[(i + 1) % numVertices];
      arduboy.drawLine(v1.x, v1.y, v2.x, v2.y, WHITE);
    }
  }

  void restartPerimeter()
  {
    debug.log("RUN", "restartPerimeter", INFO);
    for (int i = 0; i < MAX_VERTICES; i++)
    {
      vertices[i].x = 0;
      vertices[i].y = 0;
    }
    vertices[0] = vertex(0, 0);
    vertices[1] = vertex(0, HEIGHT - 1);
    vertices[2] = vertex(WIDTH - 1, HEIGHT - 1);
    vertices[3] = vertex(WIDTH - 1, 0);
    numVertices = 4;
  }

  void addVertex(byte x, byte y)
  {
    if (numVertices < MAX_VERTICES)
    {
      vertices[numVertices] = vertex(x, y);
      numVertices++;
    }
    else
    {
      debug.critical("Max vertices reached");
    }
  }
};
Perimeter perimeter;
class Player
{
public:
  vertex position;
  vertex lastPosition;
  byte valid_moves;  // bitfield for valid moves, bit 0 = up, bit 1 = right, bit 2 = down, bit 3 = left, last 4 bits may be for previous direction if needed
  byte inputs;      // same as valid_moves with bit 5 = A, bit 6 = B
  byte lives;
  byte draw; // 0 = not drawing, 1 = slow draw, 2 = fast draw
  Player()
  {
    position = vertex(WIDTH / 2, HEIGHT - 1);
    lastPosition = position;
    lives = 3;
    draw = 0;
    valid_moves = 0;
  }
  void getPlayerInput() {
    arduboy.pollButtons();
    if (arduboy.pressed(UP_BUTTON)) {
      inputs |= 0b000001;
    }
    if (arduboy.pressed(RIGHT_BUTTON)) {
      inputs |= 0b000010;
    }
    if (arduboy.pressed(DOWN_BUTTON)) {
      inputs |= 0b000100;
    }
    if (arduboy.pressed(LEFT_BUTTON)) {
      inputs |= 0b001000;
    }
    if (arduboy.pressed(A_BUTTON)) {
      inputs |= 0b010000;
    }
    if (arduboy.pressed(B_BUTTON)) {
      inputs |= 0b100000;
    }
  }
  void restartPlayer()
  {
    debug.log("RUN", "restartPlayer", INFO);
    position = vertex(WIDTH / 2, HEIGHT - 1);
    lastPosition = position;
    lives = 3;
    draw = 0;
    valid_moves = 0;
  }
};
Player player;
class Qix
{
  vertex points[2];
  vertex velocities[2];
};

class Sparx
{
};

class Fuze
{
};

class Game
{
public:
  gamestate state;
  Game() : state(START_SCREEN) {}
  void init()
  {
    debug.log("RUN", "Game.init", INFO);
    perimeter.restartPerimeter();
    player.restartPlayer();
    // qix.restartQix();
    // sparx.restartSparx();
    // fuze.restartFuze();
    state = START_SCREEN;
  }
  void update()
  {
    debug.log("RUN", "Game.update", DETAILED);
    debug.log("STATE", int(state), DETAILED);
    player.getPlayerInput();
    switch (state)
    {
    case START_SCREEN:
      drawStartScreen();
      break;
    case PLAYING:
      drawPlaying();
      break;
    case FILL_ANIMATION:
      drawFillAnimation();
      break;
    case DEATH_ANIMATION:
      drawDeathAnimation();
      break;
    case GAME_OVER:
      drawGameOver();
      break;
    }
  }

  void drawStartScreen() {

  }
  void drawPlaying() {

  }
  void drawFillAnimation(){

  }
  void drawDeathAnimation(){

  }
  void drawGameOver(){

  }
};

Game game;

void setup()
{ 
  debug.set = DEBUG;
  arduboy.boot();
  arduboy.flashlight();
  arduboy.setFrameRate(60);
  Serial.begin(9600);
  game.init();
}

void loop()
{
  if (!arduboy.nextFrame())
  {
    return;
  }
  game.update();
  arduboy.display();
}
