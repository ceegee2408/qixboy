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

vertex buildQixPoint(uint16_t phase, int baseX, int baseY,
                     uint16_t p1, int a1, uint16_t p2, int a2,
                     uint16_t p3, int a3, uint16_t p4, int a4) {
  int x = baseX + sawtooth(phase, p1, a1) + sawtooth(phase, p2, a2);
  int y = baseY + sawtooth(phase, p3, a3) + sawtooth(phase, p4, a4);

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
  uint16_t nextPhase = q.phase + q.speed;

  for (uint8_t attempt = 0; attempt < 10; attempt++) {
    vertex candidateA = buildQixPoint(nextPhase,
                                      WIDTH / 2, HEIGHT / 2,
                                      53, 18, 29, 10,
                                      47, 12, 23, 7);

    vertex candidateB = buildQixPoint(nextPhase + 31,
                                      WIDTH / 2, HEIGHT / 2,
                                      61, 16, 37, 11,
                                      41, 13, 19, 8);

    if (qixSegmentInside(candidateA, candidateB)) {
      q.phase = nextPhase;
      q.segmentA = candidateA;
      q.segmentB = candidateB;
      q.position = vertex((candidateA.getx() + candidateB.getx()) / 2,
                          (candidateA.gety() + candidateB.gety()) / 2);
      return;
    }

    nextPhase += 1;
  }
}
