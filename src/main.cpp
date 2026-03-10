#include <Arduboy2.h>
#include "header.h"

Arduboy2 arduboy;

enum gamestate
{
  START_SCREEN,
  PLAYING,
  FILL_ANIMATION,
  DEATH_ANIMATION,
  GAME_OVER
};

class Player
{
public:
  vertex position;
  vertex lastPosition;
  byte inputs; // same as valid_moves with bit 5 = A, bit 6 = B
  byte lives;
  byte draw; // 0 = not drawing, 1 = slow draw, 2 = fast draw
  index perimeterIndex;
  long points;
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

    byte newlyPressed = directional & ~Direction::fromVertices(lastPosition, position);
    byte result = 0;
    if (newlyPressed)
    {
      // newly pressed direction takes immediate priority
      if (newlyPressed & Direction::UP)
        result = Direction::UP;
      else if (newlyPressed & Direction::RIGHT)
        result = Direction::RIGHT;
      else if (newlyPressed & Direction::DOWN)
        result = Direction::DOWN;
      else if (newlyPressed & Direction::LEFT)
        result = Direction::LEFT;
    }
    else
    {
      // active direction was released, fall back to whatever is still held
      if (directional & Direction::UP)
        result = Direction::UP;
      else if (directional & Direction::RIGHT)
        result = Direction::RIGHT;
      else if (directional & Direction::DOWN)
        result = Direction::DOWN;
      else if (directional & Direction::LEFT)
        result = Direction::LEFT;
    }

    // combine single direction with A/B
    if (rawButtons & A_BUTTON)
      result |= Direction::FAST;
    else if (rawButtons & B_BUTTON)
      result |= Direction::SLOW;
    return result;
  }

  void move()
  {
    byte filteredInput = filterPlayerInput(arduboy.buttonsState());
    byte allowed = 0;
    bool onCorner = (perimeterIndex.i1 == perimeterIndex.i2) ? 1 : 0;
    if (draw)
    {
      if (perimeter.isVertexOnPerim(position))
      {
        perimeter.finishTrail();
      }
    }
    if (draw)
    {
      // drawing conditions
      allowed = moveDraw(filteredInput);
    }
    else
    {
      // perimeter conditions
      if (filteredInput & (Direction::FAST | Direction::SLOW))
      {
        // allow drawing if A or B is held
        byte edgeDir = Direction::fromVertices(perimeter.vertices[perimeterIndex.i1], perimeter.vertices[perimeterIndex.i2]);
        allowed |= Direction::FAST | Direction::SLOW;
        if (edgeDir == Direction::DOWN)
          allowed |= Direction::RIGHT;
        else if (edgeDir == Direction::RIGHT)
          allowed |= Direction::UP;
        else if (edgeDir == Direction::UP)
          allowed |= Direction::LEFT;
        else if (edgeDir == Direction::LEFT)
          allowed |= Direction::DOWN;
      }
      if (onCorner)
      {
        // if on corner, only move towards the actual adjacent vertices
        byte nextIdx = (perimeterIndex.i1 + 1) % perimeter.numPerimVertices;
        byte prevIdx = (perimeterIndex.i2 - 1 + perimeter.numPerimVertices) % perimeter.numPerimVertices;
        allowed |= (Direction::fromVertices(position, perimeter.vertices[nextIdx]) |
                    Direction::fromVertices(position, perimeter.vertices[prevIdx]));
      }
      else
      {
        // if on edge, only move along edge
        byte edgeDirs = Direction::fromVertices(perimeter.vertices[perimeterIndex.i1], perimeter.vertices[perimeterIndex.i2]);
        allowed |= edgeDirs;
        allowed |= Direction::reverse(edgeDirs);
      }
    }

    debug.log("INPUT", int(filteredInput), DETAILED);
    debug.log("ALLOWED", int(allowed), DETAILED);

    lastPosition = position;
    moveVertex(position, allowed & filteredInput);
    spriteManager.update(&render, position - vertex(2, 2));

    if (draw)
    {
      if (position != lastPosition)
      {
        // updateDraw();
        fuze.framesPlayerStationary = 0;
      }
      else
      {
        fuze.framesPlayerStationary++;
        debug.log("PLAYER STATIONARY", fuze.framesPlayerStationary, DETAILED);
      }
    }
    else
    {
      updateIndex(allowed & filteredInput, onCorner);
    }
  }

  byte moveDraw(byte imp)
  {
    byte result = 0;
    if (imp == 0)
    {
      return 0;
    }
    vertex nextPos = position;
    moveVertex(nextPos, imp);
    if (perimeter.isVertexOnTrail(nextPos))
    {
      return 0;
    }
    if (perimeter.trailLength() > 2 && isUShape(perimeter.trail(0), perimeter.trail(1), perimeter.trail(2), nextPos))
    {
      // prevent 1 pixel u-turns
      debug.log("U-TURN PREVENTED", position);
      return 0;
    }
    moveVertex(nextPos, imp); // lookahead to prevent side by side drawing
    if (!perimeter.isVertexOnTrail(nextPos))
    {
      result = imp;
      return result;
    }
    else
    {
      return 0;
    }
  }

  void restartPlayer(bool respawn = false)
  {
    debug.log("RUN", "restartPlayer", INFO);
    position = vertex(WIDTH / 2, HEIGHT - 1);
    lastPosition = position;
    lives = (respawn) ? lives : 3;
    points = (respawn) ? points : 0;
    draw = 0;
    perimeterIndex.i2 = 1;
    perimeterIndex.i1 = 2;
  }

  void updateIndex(byte direction, bool onCorner)
  {
    if (!perimeter.isVertexOnPerim(position))
    {
      // if player moved off perimeter
      draw = true;
      debug.log("Draw mode active", position);
      return;
    }
    if (onCorner)
    {
      // was on corner idx, determine which edge the player stepped onto
      byte nextIdx = (perimeterIndex.i1 + 1) % perimeter.numPerimVertices;
      byte prevIdx = (perimeterIndex.i2 - 1 + perimeter.numPerimVertices) % perimeter.numPerimVertices;
      if (position == perimeter.vertices[nextIdx])
      {
        perimeterIndex.i1 = nextIdx;
      }
      else if (position == perimeter.vertices[prevIdx])
      {
        perimeterIndex.i2 = prevIdx;
      }
    }
    else
    {
      // if on edge, update if reached corner
      if (position == perimeter.vertices[perimeterIndex.i2])
      {
        perimeterIndex.i1 = perimeterIndex.i2;
      }
      else if (position == perimeter.vertices[perimeterIndex.i1])
      {
        perimeterIndex.i2 = perimeterIndex.i1;
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
  index perimeterIndex;
};
Sparx sparx[4];

class Fuze
{
public:
  vertex position;
  index trailIndex;
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
    perimeter.drawPerim();
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
  void drawPlaying()
  {
    spriteManager.restore();

    if (arduboy.everyXFrames(player.draw ? (player.draw + SLOW_DRAW_SPEED) : FAST_DRAW_SPEED))
      player.move();

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
