#include "qix_logic.h"
#include "geometry.h"

extern perimeter perim;

namespace {

int sawtooth(uint16_t phase, uint16_t period, int amplitude) {
  if (period == 0 || amplitude == 0) return 0;
  uint16_t wrapped = phase % period;
  long scaled = (long)wrapped * (2L * amplitude);
  return (int)(scaled / period) - amplitude;
}

int bidirectionalSaw(uint16_t phase, uint16_t period, int amplitude) {
  if (period < 2) return 0;
  int a = sawtooth(phase, period, amplitude);
  int b = sawtooth(phase + period / 2, period, amplitude);
  return (a - b) / 2;
}

vertex buildQixPoint(uint16_t phase, int baseX, int baseY,
                     uint16_t xP1, int xA1, uint16_t xP2, int xA2, uint16_t xP3, int xA3,
                     uint16_t yP1, int yA1, uint16_t yP2, int yA2, uint16_t yP3, int yA3,
                     uint8_t phaseMul, uint16_t phaseOffset) {
  uint16_t p = (uint16_t)(phase * phaseMul + phaseOffset);

  int x = baseX
        + bidirectionalSaw(p, xP1, xA1)
        - bidirectionalSaw(p + 17, xP2, xA2)
        + sawtooth(p + 47, xP3, xA3);

  int y = baseY
        + bidirectionalSaw(p + 9, yP1, yA1)
        + sawtooth(p + 29, yP2, yA2)
        - bidirectionalSaw(p + 63, yP3, yA3);

  if (x < 0) x = 0;
  if (x >= WIDTH) x = WIDTH - 1;
  if (y < 0) y = 0;
  if (y >= HEIGHT) y = HEIGHT - 1;

  return vertex(x, y);
}

bool qixSegmentInside(vertex a, vertex b) {
  if (!isInsidePolygon(a, perim.vertices, perim.vertexCount)) return false;
  if (!isInsidePolygon(b, perim.vertices, perim.vertexCount)) return false;

  vertex mid((a.getx() + b.getx()) / 2, (a.gety() + b.gety()) / 2);
  return isInsidePolygon(mid, perim.vertices, perim.vertexCount);
}

} // namespace

void updateQix() {
  uint16_t nextPhase = q.phase + (uint16_t)q.speed;

  for (uint8_t attempt = 0; attempt < 16; attempt++) {
    uint16_t candidatePhase = nextPhase + attempt * 3;

    vertex candidateA = buildQixPoint(candidatePhase,
                                      WIDTH / 2, HEIGHT / 2,
                                      53, 19, 31, 11, 17, 6,
                                      47, 14, 29, 9, 19, 5,
                                      3, 0);

    vertex candidateB = buildQixPoint(candidatePhase,
                                      WIDTH / 2, HEIGHT / 2,
                                      61, 18, 37, 10, 23, 7,
                                      43, 13, 27, 8, 21, 6,
                                      5, 71);

    if (qixSegmentInside(candidateA, candidateB)) {
      q.phase = candidatePhase;
      q.segmentA = candidateA;
      q.segmentB = candidateB;
      q.position = vertex((candidateA.getx() + candidateB.getx()) / 2,
                          (candidateA.gety() + candidateB.gety()) / 2);
      return;
    }
  }
}
