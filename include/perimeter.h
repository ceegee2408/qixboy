#ifndef PERIMETER_H
#define PERIMETER_H

#include "header.h"

class Perimeter
{
public:
    // Keep scratch bounded by perimeter storage to avoid deep-stack crashes.
    static constexpr byte MAX_SCRATCH_VECTORS = MAX_VECTORS;
    byte vectors[MAX_VECTORS];
    vector initialVector;
    byte numVectors;

    vector getVector(byte index) const
    {
        return PackedSegment::decode(vectors[index]);
    }

    void setVector(byte index, const vector &v)
    {
        vectors[index] = PackedSegment::encode(v);
    }

    bool insertPackedAt(byte index, const vector &segment)
    {
        if (numVectors >= MAX_VECTORS)
        {
            debug.critical("in: insertPackedAt, max perimeter vectors reached");
            return false;
        }
        for (byte i = numVectors; i > index; i--)
        {
            vectors[i] = vectors[i - 1];
        }
        setVector(index, segment);
        numVectors++;
        return true;
    }

    byte nextIndex(byte i)
    {
        byte next = i + 1;
        return (next >= numVectors) ? 0 : next;
    }
    byte prevIndex(byte i)
    {
        return (i == 0) ? numVectors - 1 : i - 1;
    }

    void draw()
    {
        debug.log("RUN", "Perimeter draw", DETAILED);
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector next = pos + getVector(i);
            arduboy.drawLine(pos.x, pos.y, next.x, next.y, WHITE);
            pos = next;
        }
    }

    void clearDraw()
    {
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector next = pos + getVector(i);
            arduboy.drawLine(pos.x, pos.y, next.x, next.y, BLACK);
            pos = next;
        }
    }

    void addVector(vector v, byte index)
    {
        debug.log("RUN", "Perimeter addvector", INFO);
        byte len = v.len();
        if (len == 0)
        {
            return;
        }

        byte dir = v.dir();
        while (len > 0)
        {
            byte chunk = (len > PackedSegment::MAX_LEN) ? PackedSegment::MAX_LEN : len;
            if (!insertPackedAt(index, Direction::unitVec(dir, chunk)))
            {
                return;
            }
            index++;
            len -= chunk;
        }
    }

    void removeVector(byte index)
    {
        debug.log("RUN", "Perimeter removevector", INFO);
        for (byte i = index; i < numVectors - 1; i++)
        {
            vectors[i] = vectors[i + 1];
        }
        numVectors--;
    }

    void newPerimeter(vector newInitialVector, vector *newVectors, byte numNewVectors)
    {
        debug.log("RUN", "Perimeter newPerimeter", INFO);
        if (numNewVectors > MAX_VECTORS)
        {
            debug.critical("in: newPerimeter, too many vectors for perimeter storage");
        }
        initialVector = newInitialVector;
        numVectors = 0;
        for (byte i = 0; i < numNewVectors; i++)
        {
            addVector(newVectors[i], numVectors);
        }
    }

    void restartPerimeter()
    {
        debug.log("RUN", "restartPerimeter", INFO);
        numVectors = 0;
        initialVector = vector(0, 0);
        addVector(vector(0, HEIGHT - 1), numVectors);
        addVector(vector(WIDTH - 1, 0), numVectors);
        addVector(vector(0, -HEIGHT + 1), numVectors);
        addVector(vector(-WIDTH + 1, 0), numVectors);
    }

    bool isVectorOnPerim(vector v)
    {
        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector seg = getVector(i);
            vector b = a + seg;
            if (along(v, a, seg))
            {
                return true;
            }
            a = b;
        }
        return false;
    }

    byte perimLengthUpTo(vector v)
    {
        byte length = 0;
        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector seg = getVector(i);
            vector b = a + seg;
            if (along(v, a, seg))
            {
                length += (v - a).len();
                return length;
            }
            else
            {
                length += seg.len();
            }
            a = b;
        }
        debug.critical("in: perimLengthUpTo, vector not found on perimeter");
        return 255;
    }

    vector pointAtIndex(byte index)
    {
        vector point = initialVector;
        for (byte i = 0; i <= index; i++)
        {
            point = point + getVector(i);
        }
        return point;
    }

    byte getPerimIndexAtPoint(vector pos)
    {
        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector seg = getVector(i);
            vector b = a + seg;
            if (along(pos, a, seg))
            {
                return i;
            }
            a = b;
        }
        debug.critical("in: getPerimIndexAtPoint, vector not found on perimeter");
        return 255;
    }

    byte getPerimIndexAtPointWithPreferredNormal(vector pos, byte preferredNormal)
    {
        vector a = initialVector;
        bool foundAny = false;
        byte fallbackIdx = 0;
        for (byte i = 0; i < numVectors; i++)
        {
            vector seg = getVector(i);
            if (along(pos, a, seg))
            {
                if (!foundAny)
                {
                    fallbackIdx = i;
                    foundAny = true;
                }
                if (Direction::leftHandNormal(seg) == preferredNormal)
                {
                    return i;
                }
            }
            a = a + seg;
        }
        if (foundAny)
        {
            return fallbackIdx;
        }
        debug.critical("in: getPerimIndexAtPointWithPreferredNormal, vector not found on perimeter");
        return 255;
    }

    byte collectPerimIndicesAtPoint(vector pos, byte *indices, byte maxIndices = 2)
    {
        vector a = initialVector;
        byte found = 0;
        for (byte i = 0; i < numVectors; i++)
        {
            vector seg = getVector(i);
            if (along(pos, a, seg))
            {
                if (found < maxIndices)
                {
                    indices[found] = i;
                }
                found++;
            }
            a = a + seg;
        }

        if (found == 0)
        {
            debug.critical("in: collectPerimIndicesAtPoint, vector not found on perimeter");
            return 0;
        }

        return (found > maxIndices) ? maxIndices : found;
    }

    void prioritizeIndexByNormal(byte *indices, byte count, vector pos, byte preferredNormal)
    {
        if (count < 2)
        {
            return;
        }

        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector seg = getVector(i);
            if (i == indices[0] && along(pos, a, seg) && Direction::leftHandNormal(seg) == preferredNormal)
            {
                return;
            }
            if (i == indices[1] && along(pos, a, seg) && Direction::leftHandNormal(seg) == preferredNormal)
            {
                byte temp = indices[0];
                indices[0] = indices[1];
                indices[1] = temp;
                return;
            }
            a = a + seg;
        }
    }

    byte getAllowedMoves(vector position, bool drawInput = 0)
    {
        byte allowed = 0;
        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector seg = getVector(i);
            vector b = a + seg;
            if (along(position, a, seg))
            {
                byte segDir = seg.dir();
                if (position != b)
                    allowed |= segDir;
                if (position != a)
                    allowed |= Direction::reverse(segDir);
                if (drawInput)
                    allowed |= Direction::leftHandNormal(seg);
            }
            a = b;
        }
        return allowed;
    }

    void insertTrail(Trail &t, byte startIndex, byte endIndex)
    {
    }

    bool isForwardAlongSegment(vector from, vector to, vector segment)
    {
        if (segment.x > 0)
            return to.x >= from.x;
        if (segment.x < 0)
            return to.x <= from.x;
        if (segment.y > 0)
            return to.y >= from.y;
        if (segment.y < 0)
            return to.y <= from.y;
        return false;
    }

    byte appendIfNonZero(vector *scratch, byte scratchIdx, vector segment)
    {
        if (segment != vector(0, 0))
        {
            if (scratchIdx >= MAX_SCRATCH_VECTORS)
            {
                debug.critical("in: appendIfNonZero, scratch overflow");
                return scratchIdx;
            }
            scratch[scratchIdx] = segment;
            scratchIdx++;
        }
        return scratchIdx;
    }

    byte appendPerimeterPath(vector *scratch, byte scratchIdx, vector fromPosition, byte fromIndex, vector toPosition, byte toIndex, bool forward)
    {
        vector fromSeg = getVector(fromIndex);
        vector toSeg = getVector(toIndex);
        if (forward)
        {
            if (fromIndex == toIndex && isForwardAlongSegment(fromPosition, toPosition, fromSeg))
            {
                return appendIfNonZero(scratch, scratchIdx, toPosition - fromPosition);
            }

            vector fromSegmentEnd = pointAtIndex(fromIndex) - fromPosition;
            scratchIdx = appendIfNonZero(scratch, scratchIdx, fromSegmentEnd);
            for (byte i = nextIndex(fromIndex); i != toIndex; i = nextIndex(i))
            {
                scratchIdx = appendIfNonZero(scratch, scratchIdx, getVector(i));
            }
            vector toSegmentStart = pointAtIndex(toIndex) - toSeg;
            scratchIdx = appendIfNonZero(scratch, scratchIdx, toPosition - toSegmentStart);
            return scratchIdx;
        }

        if (fromIndex == toIndex)
        {
            return appendIfNonZero(scratch, scratchIdx, toPosition - fromPosition);
        }

        vector fromSegmentStart = pointAtIndex(fromIndex) - fromSeg;
        scratchIdx = appendIfNonZero(scratch, scratchIdx, fromSegmentStart - fromPosition);
        for (byte i = prevIndex(fromIndex); i != toIndex; i = prevIndex(i))
        {
            scratchIdx = appendIfNonZero(scratch, scratchIdx, getVector(i).opp());
        }
        vector toSegmentEnd = pointAtIndex(toIndex);
        scratchIdx = appendIfNonZero(scratch, scratchIdx, toPosition - toSegmentEnd);
        return scratchIdx;
    }

    byte buildScratchPath(vector *scratch, vector &scratchInitialVector, Trail &t, vector trailEndPosition, byte startIndex, byte endIndex, bool cyclonic)
    {
        byte scratchIdx = 0;

        if (cyclonic)
        {
            scratchInitialVector = t.initialVector;
            for (byte i = 0; i < t.numVectors; i++)
            {
                scratch[scratchIdx] = t.getVector(i);
                scratchIdx++;
            }
            debug.log("SCRATCH INDEX COUNT", scratchIdx, INFO);
            return appendPerimeterPath(scratch, scratchIdx, trailEndPosition, endIndex, t.initialVector, startIndex, true);
        }

        scratchInitialVector = trailEndPosition;
        for (byte i = t.numVectors; i > 0; i--)
        {
            scratch[scratchIdx] = t.getVector(i - 1).opp();
            scratchIdx++;
        }
        debug.log("SCRATCH INDEX COUNT", scratchIdx, INFO);
        return appendPerimeterPath(scratch, scratchIdx, t.initialVector, startIndex, trailEndPosition, endIndex, true);
    }

    void finishTrail(Trail &t, vector position, vector qixPosition)
    {
        if (t.numVectors == 0)
        {
            return;
        }

        vector trailStartPosition = t.initialVector;
        vector trailEndPosition = position;
        byte startDrawDir = t.getVector(0).dir();
        byte endDrawDir = t.getVector(t.numVectors - 1).dir();
        byte startCandidates[2] = {0, 0};
        byte endCandidates[2] = {0, 0};
        byte startCount = collectPerimIndicesAtPoint(trailStartPosition, startCandidates);
        byte endCount = collectPerimIndicesAtPoint(trailEndPosition, endCandidates);
        prioritizeIndexByNormal(startCandidates, startCount, trailStartPosition, startDrawDir);
        prioritizeIndexByNormal(endCandidates, endCount, trailEndPosition, Direction::reverse(endDrawDir));

        vector scratch[MAX_SCRATCH_VECTORS];
        vector scratchInitialVector;

        for (byte si = 0; si < startCount; si++)
        {
            for (byte ei = 0; ei < endCount; ei++)
            {
                byte spIdx = startCandidates[si];
                byte epIdx = endCandidates[ei];

                byte cyclonicCount = buildScratchPath(scratch, scratchInitialVector, t, trailEndPosition, spIdx, epIdx, true);
                if (isInsidePolygon(qixPosition, scratchInitialVector, scratch, cyclonicCount))
                {
                    newPerimeter(scratchInitialVector, scratch, cyclonicCount);
                    byte antiCyclonicCount = buildScratchPath(scratch, scratchInitialVector, t, trailEndPosition, spIdx, epIdx, false);
                    t.setFillPolygon(scratchInitialVector, scratch, antiCyclonicCount);
                    return;
                }

                byte antiCyclonicCount = buildScratchPath(scratch, scratchInitialVector, t, trailEndPosition, spIdx, epIdx, false);
                if (isInsidePolygon(qixPosition, scratchInitialVector, scratch, antiCyclonicCount))
                {
                    newPerimeter(scratchInitialVector, scratch, antiCyclonicCount);
                    byte cyclonicCount = buildScratchPath(scratch, scratchInitialVector, t, trailEndPosition, spIdx, epIdx, true);
                    t.setFillPolygon(scratchInitialVector, scratch, cyclonicCount);
                    return;
                }
            }
        }

        /*
        we take two paths, keeping track of our vectors.
        CYCLONIC: startTrail -> trail -> endtrail -> (LEFT)slicedPerim -> perim -> slicedperim
        ANTI-CYCLONIC: endTrail -> trail(reversed) -> startTrail -> (LEFT)slicedPerim -> perim -> slicedperim
        */
        debug.critical("Qix is outside both potential fill areas, this should be impossible");
    }
};

#endif