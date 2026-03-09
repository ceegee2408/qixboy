#include <Arduboy2.h>
#include "header.h"
#define MAX_VERTICES 128
#define DEBUG INFO
#define SLOW_DRAW_SPEED 3
#define FAST_DRAW_SPEED 2
#define NORMAL_SPEED FAST_DRAW_SPEED
#define RESPAWN true

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
  byte inputs;      // same as valid_moves with bit 5 = A, bit 6 = B
  byte lives;
  byte draw; // 0 = not drawing, 1 = slow draw, 2 = fast draw
  byte perimeterIndex[2]; 
  long points;
  handleRender render = handleRender(playerMask, true);

  Player()
  {
    restartPlayer();
    vertex playerRenderOffset = (position - vertex(2, 2));
    spriteManager.add(&render, playerRenderOffset);
  }

  byte filterPlayerInput(uint8_t rawButtons) {
    // remap from Arduboy hardware bits to Direction constants
    byte directional = 0;
    if (rawButtons & UP_BUTTON)    directional |= Direction::UP;
    if (rawButtons & DOWN_BUTTON)  directional |= Direction::DOWN;
    if (rawButtons & LEFT_BUTTON)  directional |= Direction::LEFT;
    if (rawButtons & RIGHT_BUTTON) directional |= Direction::RIGHT;

    byte newlyPressed = directional & ~Direction::fromVertices(lastPosition, position);
    byte result = 0;
    if (newlyPressed) {
        // newly pressed direction takes immediate priority
        if      (newlyPressed & Direction::UP)    result = Direction::UP;
        else if (newlyPressed & Direction::RIGHT)  result = Direction::RIGHT;
        else if (newlyPressed & Direction::DOWN)   result = Direction::DOWN;
        else if (newlyPressed & Direction::LEFT)   result = Direction::LEFT;
    } else {
        // active direction was released, fall back to whatever is still held
        if      (directional & Direction::UP)    result = Direction::UP;
        else if (directional & Direction::RIGHT)  result = Direction::RIGHT;
        else if (directional & Direction::DOWN)   result = Direction::DOWN;
        else if (directional & Direction::LEFT)   result = Direction::LEFT;
    }

    // combine single direction with A/B
    if (rawButtons & A_BUTTON) result |= Direction::FAST;
    else if (rawButtons & B_BUTTON) result |= Direction::SLOW;
    return result;
  }

  void move() {
    byte filteredInput = filterPlayerInput(arduboy.buttonsState());
    byte allowed = 0;
    bool onCorner = (perimeterIndex[0] == perimeterIndex[1]) ? 1 : 0;
    if (draw) {
      allowed = moveDraw(filteredInput);
    } else {
      if (filteredInput & (Direction::FAST | Direction::SLOW)) {
        // if trying to draw, also allow movement towards inside of perimeter
        // inward is the left-hand normal of the travel direction (clockwise perimeter)
        byte edgeDir = Direction::fromVertices(perimeter.vertices[perimeterIndex[0]], perimeter.vertices[perimeterIndex[1]]);
        if      (edgeDir == Direction::DOWN)  allowed |= Direction::RIGHT;
        else if (edgeDir == Direction::RIGHT) allowed |= Direction::UP;
        else if (edgeDir == Direction::UP)    allowed |= Direction::LEFT;
        else if (edgeDir == Direction::LEFT)  allowed |= Direction::DOWN;
      }
      if (onCorner) {
        // if on corner, only move towards the actual adjacent vertices
        byte nextIdx = (perimeterIndex[1] + 1) % perimeter.numVertices;
        byte prevIdx = (perimeterIndex[0] - 1 + perimeter.numVertices) % perimeter.numVertices;
        allowed |= (Direction::fromVertices(position, perimeter.vertices[nextIdx]) |
                   Direction::fromVertices(position, perimeter.vertices[prevIdx]));
      } else {
        // if on edge, only move along edge
        allowed |= Direction::fromVertices(perimeter.vertices[perimeterIndex[0]], perimeter.vertices[perimeterIndex[1]]);
        allowed |= Direction::reverse(allowed);
      }
    }

    debug.log("INPUT", int(filteredInput), DETAILED);
    debug.log("ALLOWED", int(allowed), DETAILED);

    lastPosition = position;
    moveVertex(position, allowed & filteredInput);
    spriteManager.update(&render, position - vertex(2, 2));
    
    updateIndex(onCorner);
                
    if (position != lastPosition) {
      debug.log("MOVED TO", position, INFO);
    }
  }

  byte moveDraw(byte imp) {
    return 0;
  }

  void restartPlayer(bool respawn = false)
  {
    debug.log("RUN", "restartPlayer", INFO);
    position = vertex(WIDTH / 2, HEIGHT - 1);
    lastPosition = position;
    lives = (respawn) ? lives : 3;
    points = (respawn) ? points : 0;
    draw = 0;
    perimeterIndex[0] = 1;
    perimeterIndex[1] = 2;
  }

  void updateIndex(bool onCorner) {
    if (lastPosition == position) {
      // no movement this frame, UPDATE FUZE COUNTER
      return;
    }
    if (draw) return;
    if (!onCorner) {
      // if was on edge, just update the vertex that was left behind
      if (position == perimeter.vertices[perimeterIndex[0]]) {
        perimeterIndex[1] = perimeterIndex[0];
      } else if (position == perimeter.vertices[perimeterIndex[1]]) {
        perimeterIndex[0] = perimeterIndex[1];
      }
    } else {
      // was on corner idx; determine which adjacent edge the player stepped onto
      byte idx = perimeterIndex[0];
      byte nextIdx = (idx + 1) % perimeter.numVertices;
      byte prevIdx = (idx - 1 + perimeter.numVertices) % perimeter.numVertices;
      if (along(position, perimeter.vertices[idx], perimeter.vertices[nextIdx])) {
        perimeterIndex[0] = idx;
        perimeterIndex[1] = nextIdx;
        if (position == perimeter.vertices[nextIdx]) perimeterIndex[0] = nextIdx; // reached next corner
      } else if (along(position, perimeter.vertices[prevIdx], perimeter.vertices[idx])) {
        perimeterIndex[0] = prevIdx;
        perimeterIndex[1] = idx;
        if (position == perimeter.vertices[prevIdx]) perimeterIndex[1] = prevIdx; // reached prev corner
      }
    }
  }

};
Player player;

class Qix
{
  vertex points[2];
  vertex velocities[2];
};
Qix qix;

class Sparx
{
  public:
    vertex position;
    bool ccw;
    byte perimeterIndex[2];
};
Sparx sparx[4];

class Fuze
{
  public:
    vertex position;
    byte trailIndex[2];
    byte framesPlayerStationary = 0;
};
Fuze fuze;

class Game
{
public:
  gamestate state;
  Game() : state(START_SCREEN) {}
  void init()
  {
    debug.log("RUN", "Game.init", INFO);
    restart();
    perimeter.draw();
    
  }
  void restart(bool respawn = false){
    perimeter.restartPerimeter();
    player.restartPlayer(respawn);
    // qix.restartQix();
    // sparx.restartSparx();
    // fuze.restartFuze();
  }
  void update()
  {
    debug.log("RUN", "Game.update", DETAILED);
    debug.log("STATE", int(state), DETAILED);
    
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
    arduboy.drawBitmap(0, 0, menuBitmap, 128, 64, WHITE);
    if(arduboy.everyXFrames(30)) {
      arduboy.drawBitmap(39, 59, insertCoinBitmap, 50, 5, INVERT);
    }
    arduboy.pollButtons();
    if(arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) {
      arduboy.clear();
      init();
      state = PLAYING;
    }
  }
  void drawPlaying() {
    spriteManager.restore();
    
    if (arduboy.everyXFrames(NORMAL_SPEED)) player.move();



    spriteManager.draw();
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
  //init arduboy
  arduboy.boot();
  arduboy.flashlight();
  arduboy.setFrameRate(60);
  //init serial
  Serial.begin(9600);
  debug.set = DEBUG;
  //init game
  game.state = START_SCREEN;
  arduboy.waitNoButtons();
}

void loop()
{
  if (!arduboy.nextFrame()) return;
  game.update();
  arduboy.display();
}
