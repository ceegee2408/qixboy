#include <Arduboy2.h>
#include "header.h"

Arduboy2 arduboy;

enum states
{
  START_SCREEN,
  PLAYING,
  FILL_ANIMATION,
  DEATH_ANIMATION,
  GAME_OVER
};

class Qix
{
public:
  vertex position;  // current position for collision/fill detection
  vertex points[2]; // line endpoints for rendering
  vertex velocities[2];

  Qix()
  {
    // initialize to center of screen for now
    position = vertex(WIDTH / 2, HEIGHT / 2);
  }
};

class Player
{
public:
  vertex position;
  vertex lastPosition;
  byte lives;
  byte draw = 0; // 0 = not drawing, 1 = slow draw, 2 = fast draw
  long points;
  byte prevDirectional;
  handleRender render = handleRender(playerMask, true);

  Player()
  {
    restartPlayer();
    vertex playerRenderOffset = (position - vertex(2, 2));
    spriteManager.add(&render, playerRenderOffset);
  }

  byte filterPlayerInput(uint8_t rawButtons)
  {
    // remap from Arduboy hardware bits to Direction constants
    byte directional = 0;
    if (rawButtons & UP_BUTTON)
      directional |= Direction::UP;
    if (rawButtons & DOWN_BUTTON)
      directional |= Direction::DOWN;
    if (rawButtons & LEFT_BUTTON)
      directional |= Direction::LEFT;
    if (rawButtons & RIGHT_BUTTON)
      directional |= Direction::RIGHT;
    if (rawButtons & A_BUTTON)
      directional |= Direction::FAST;
    if (rawButtons & B_BUTTON)
      directional |= Direction::SLOW;

    // detect truly newly pressed buttons (pressed this frame but not last frame)
    byte newlyPressed = directional & ~prevDirectional;
    byte result = 0;

    if (newlyPressed)
    {
      // newly pressed direction takes immediate priority (only one direction)
      if (newlyPressed & Direction::UP)
        result = Direction::UP;
      else if (newlyPressed & Direction::RIGHT)
        result = Direction::RIGHT;
      else if (newlyPressed & Direction::DOWN)
        result = Direction::DOWN;
      else if (newlyPressed & Direction::LEFT)
        result = Direction::LEFT;

      if (newlyPressed & Direction::FAST)
        result |= Direction::FAST;
      else if (newlyPressed & Direction::SLOW)
        result |= Direction::SLOW;
    }
    else
    {
      // no new buttons, use previously active direction if still held
      byte currentDirection = Direction::fromVertices(lastPosition, position);
      if (directional & currentDirection)
      {
        result = currentDirection; // continue in same direction
      }
      else if (directional)
      {
        // current direction released, fall back to whatever is still held
        if (directional & Direction::UP)
          result = Direction::UP;
        else if (directional & Direction::RIGHT)
          result = Direction::RIGHT;
        else if (directional & Direction::DOWN)
          result = Direction::DOWN;
        else if (directional & Direction::LEFT)
          result = Direction::LEFT;

        if (directional & Direction::FAST)
          result |= Direction::FAST;
        else if (directional & Direction::SLOW)
          result |= Direction::SLOW;
      }
    }
    prevDirectional = directional;
    return result;
  }
  
  void move(byte allowedInput)
  {
    lastPosition = position;
    moveVertex(position, allowedInput);
    vertex playerRenderOffset = (position - vertex(2, 2));
    spriteManager.update(&render, playerRenderOffset);
  }

  void restartPlayer(bool respawn = false)
  {
    debug.log("RUN", "restartPlayer", INFO);
    position = vertex(WIDTH / 2, HEIGHT - 1);
    lastPosition = position;
    lives = (respawn) ? lives : 3;
    points = (respawn) ? points : 0;
    draw = 0;
    prevDirectional = 0;
  }

  byte getMoveSpeed() {
    byte moveSpeed = NORMAL_SPEED;
    if(draw & 0){

    }
    return; 
  }
};

class Sparx
{
public:
  vertex position;
  bool ccw;
};

class Fuze
{
public:
  vertex position;
  byte framesPlayerStationary = 0;
};

class Game
{
public:
  states state;
  Perimeter perimeter;
  Trail trail;
  Player player;
  Qix qix;
  Fuze fuze;
  Sparx sparx[4];

  Game() : state(START_SCREEN) {}
  void init()
  {
    debug.log("RUN", "Game.init", INFO);
    restart();
    perimeter.draw();
  }
  void restart(bool respawn = false)
  {
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
    arduboy.pollButtons();
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

  void drawStartScreen()
  {
    arduboy.drawBitmap(0, 0, menuBitmap, 128, 64, WHITE);
    if (arduboy.everyXFrames(30))
    {
      arduboy.drawBitmap(39, 59, insertCoinBitmap, 50, 5, INVERT);
    }
    arduboy.pollButtons();
    if (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON))
    {
      arduboy.clear();
      init();
      state = PLAYING;
    }
  }

  // updates
  void updatePlaying()
  {
    if (arduboy.everyXFrames(player.getMoveSpeed())) {
      byte input = player.filterPlayerInput(arduboy.buttonsState());
      bool drawInput = input & Direction::DRAWBUTTONS;
      debug.log("INPUT", input, INFO);
      byte allowed; 
      if (player.draw) {
        allowed = trail.getAllowedMoves(player.position, input);
      } else {
        allowed = perimeter.getAllowedMoves(player.position, drawInput);
      }
      debug.logBitmap("ALLOWED", allowed, INFO);
      player.move(allowed & input);
      if (drawInput && !perimeter.isVertexOnPerim(player.position) && !player.draw){
        player.draw = (input & Direction::FAST) ? 2 : 1;
        trail.beginTrail(player.lastPosition);
      }
      if (player.draw && perimeter.isVertexOnPerim(player.position)){
        perimeter.finishTrail(trail);
        state = FILL_ANIMATION;
      }
    }
  }
  // draws
  void drawPlaying()
  {
    spriteManager.restore();
    perimeter.draw();
    updatePlaying();
    spriteManager.draw();
  }
  void drawFillAnimation()
  {
  }
  void drawDeathAnimation()
  {
  }
  void drawGameOver()
  {
  }
};
Game game;

void setup()
{
  // init arduboy
  arduboy.boot();
  arduboy.flashlight();
  arduboy.setFrameRate(60);
  // init serial
  Serial.begin(9600);
  debug.set = DEBUG;
  // init game
  game.state = START_SCREEN;
  arduboy.waitNoButtons();
}

void loop()
{
  if (!arduboy.nextFrame())
    return;
  game.update();
  arduboy.display();
}
