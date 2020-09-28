/* This file is part of VoltDB.
 * Copyright (C) 2008-2018 VoltDB Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef INCLUDE_GEOGRAPHY_H_
#define INCLUDE_GEOGRAPHY_H_
#include <math.h>
#include <vector>
#include "GeographyPoint.hpp"


namespace voltdb {

class ByteBuffer;

/**
 * Objects of this class represent geography objects.  Currently the
 * only geography object VoltDB supports is the polygon, with possible
 * holes.
 *
 * A polygon is made up of a sequence of rings.  The first ring
 * is the outer shell, and subsequent rings are holes.  The rings must be
 * arrange so that the first ring is counter-clockwise, and subsequent
 * rings, which are holes, are clockwise.  This means that the interior
 * of the polygon would always be on the left hand of a point traversing
 * the border.
 */
class Geography {
public:
    /**
     * Objects of this class are rings.  A ring is a sequence of points,
     * and a point is a pair of <longitude, latitude> coordinates.  These
     * are either clockwise or counterclockwise, depending on whether the
     * ring represents an outer shell (counter clockwise) or an inner hole
     * (clockwise).
     *
     * Before a ring is inserted into a polygon it must be closed.  That is to
     * say, its first and last points must be equal.
     */
    class Ring {
    public:
        Ring() {}

        /**
         * Erase the ring's contents.
         */
        void clear() {
            m_points.clear();
        }

        /**
         * Add a point, and return this ring.  This makes it useful
         * for chaining addPoint calls.
         */
        Ring &addPoint(const GeographyPoint &aPoint) {
            m_points.push_back(aPoint);
            return *this;
        }

        /**
         * Return the number of points in the Ring.
         */
        int numPoints() const {
            return m_points.size();
        }

        /**
         * Get the point at a given index in the ring.
         */
        const GeographyPoint &getPoint(int idx) const {
            if (0 <= idx && idx < (int)m_points.size()) {
                return m_points[idx];
            } else {
                throw IndexOutOfBoundsException();
            }
        }

        GeographyPoint &getPoint(int idx) {
            if (0 <= idx && idx < (int) m_points.size()) {
                return m_points[idx];
            } else {
                throw IndexOutOfBoundsException();
            }
        }

        /**
         * Sometimes rings need to be reversed.  We do this by reversing
         * all but the first and last. The first and last should be equal.
         */
        void reverse();

        /**
         * Test that two rings are exactly equal.  We don't test for
         * circular shifts of the points.  So, two rings are equal if
         * they have the same first point and all subsequent points are
         * equal.
         *
         * Since these have floating point values, this is probably not
         * what is wanted.  See Geography::Ring::approximatelyEqual for a
         * more flexible test.
         */
        bool operator==(const Ring &aOther) const {
            return approximatelyEqual(aOther, DEFAULT_EQUALITY_EPSILON);
        }

        bool operator!=(const Ring &aOther) const {
            return !operator==(aOther);
        }

        /**
         * Test that two rings have vertices which are within
         * epsilon of each other.  That is, check that
         * fabs(this.longitude - that.longitude) < epsilon
         * and fabs(this.latitude - that.latitude) < epsilon.
         *
         * As with operator==, we don't test for circular shifts.
         * So, the first points must be approximately equal, and
         * subsequent points must be approximately equal.
         */
        bool approximatelyEqual(const Ring &rhs, double epsilon) const;

        std::string toString() const;
        /**
         * Write a ring to a buffer.  If reverseit is true, then
         * write the points in reverse order.  The serialization
         * has a peculiar form, specific to the VoltDB wire protocol.
         * See https://downloads.voltdb.com/documentation/wireprotocol.pdf
         * for details.
         */
        void serializeTo(ByteBuffer &buffer, bool reverseit) const;
    private:
        typedef std::vector<GeographyPoint> PointVector;
        typedef PointVector::iterator       PointIterator;
        typedef PointVector::const_iterator PointConstIterator;
        PointVector                         m_points;
    };

    /**
     * This creates a null geography.
     */
    Geography() {}
    /**
     * This very specific constructor creates a geography
     * from a serialized message.
     */
    Geography(ByteBuffer &message, int32_t offset, bool &wasNull);

    /**
     * Add an already existing ring.  This is used in
     * operator<< to add rings to Geographies.  A much
     * more efficient location is to sue addEmptyRing()
     * (c.f. below).
     */
    Geography &addRing(const Ring &aRing) {
        m_rings.push_back(aRing);
        return *this;
    }
    /**
     * Add a new ring, then return a reference to the ring we added.
     * This is used typically like this:
     *   polygon.addEmptyRing() << GeographyPoint(0, 0)
     *                          << GeographyPoint(1, 0)
     *                          ...;
     */
    Ring &addEmptyRing() {
        // It's unfortunate that we can't use C++11
        // here.  This is perfect for emplacement new.
        m_rings.push_back(Ring());
        return m_rings[m_rings.size()-1];
    }

    /**
     * Return the number of rings in the polygon.
     */
    int numRings() const {
        return m_rings.size();
    }

    /**
     * Get the ring at index idx.  This may throw voltdb::IndexOutOfBoundsException
     * if idx is out of range.
     */
    const Ring &getRing(int idx) const {
        if (0 <= idx && idx < (int)m_rings.size()) {
            return m_rings[idx];
        } else {
            throw IndexOutOfBoundsException();
        }
    }

    /**
     * Get the ring at index idx.  This may throw voltdb::IndexOutOfBoundsException
     * if idx is out of range.
     */
    Ring &getRing(int idx) {
        if (0 <= idx && idx < (int)m_rings.size()) {
            return m_rings[idx];
        } else {
            throw IndexOutOfBoundsException();
        }
    }

    /**
     * Test to see if this point is approximately equal to this
     * point.  We don't test for circular shifts of rings.
     * That is to say, the first rings must be exactly equal
     * and subsequent rings must be exactly equal.
     *
     * Two points are approximately equal if their coordinates
     * are within some epsilon of each other.  That is to say,
     * if Math.abs(p1.getLongitude() - p2.getLongitude()) < epsilon
     * and Math.abs(p1.getLatitude() - p2.getLatitude()) < epsilon.
     * For this operation the value of epsilon is given by
     * the constant DEFAULT_EQUALITY_EPSILON, which is 1.0e-12.
     * Two Geography objects are approximately equal if all their
     * coordinates of all their rings are approximately equal.
     *
     * Note that the function approximatelyEqual does the same
     * thing, but allows the specification of epsilon.
     */
    bool operator==(const Geography &aOther) const {
        return approximatelyEqual(aOther, DEFAULT_EQUALITY_EPSILON);
    }

    bool operator !=(const Geography &aOther) const {
        return !operator==(aOther);
    }

    /**
     * Return true iff the the given geography is equal to this
     * one, but allow the coordinates to differ by something less
     * than epsilon.  If epsilon is 0.0, we require strict
     * equality.
     */
    bool approximatelyEqual(const Geography &rhs, double epsilon) const ;

    /**
     * Return the size of this geography, including the 4 byte
     * size field.
     */
    int32_t getSerializedSize() const;

    /**
     * Serialize this geography to the given buffer.  This function
     * writes the size.  This returns the amount of data actually
     * written, as a check on getSerializedSize.
     */
    int32_t serializeTo(ByteBuffer &buffer) const;
    /**
     * Deserialize a geography value from a ByteBuffer.  We
     * read the size as well.
     *
     * Return the number of bytes actually used.
     */
    int32_t deserializeFrom(ByteBuffer &aData,
                            int32_t firstOffset,
                            bool &aWasNull);
    /**
     * Generate some WKT text for this buffer.   This is mostly
     * useful for debugging.
     */
    std::string toString();
    /**
     * The null polygon has no rings.
     */
    bool isNull() const {
        return m_rings.size() == 0;
    }
    /**
     * Make this Geography be null.
     */
    void makeNull() {
        m_rings.clear();
    }
private:
    typedef std::vector<Ring>          RingVector;
    typedef RingVector::iterator       RingIterator;
    typedef RingVector::const_iterator RingConstIterator;
    std::vector<Ring> m_rings;
};

/**
 * This is useful to add a point to a ring.  Use it in the
 * pattern:
 *   Geograhy geo;
 *   geo.addEmptyRing() << GeographyPoint(0, 0)
 *                      << GeographyPoint(1, 0)
 *                      << GeographyPoint(1, 1)
 *                      << GeographyPoint(0, 1)
 *                      << GeographyPoint(0, 0);
 * The ring is created inside the geography object, and is
 * operated on directly.  So, no temporary Rings are
 * created.  It's possible to do better, but not
 * without C++-11's move semantics.
 */
inline Geography::Ring &operator<<(Geography::Ring &aRing, const GeographyPoint &aPoint) {
    return aRing.addPoint(aPoint);
}

/**
 * This is useful to add a Ring to a Geography.  This is
 * less efficient, because the Ring is necessarily copied.
 */
inline Geography &operator<<(Geography &aGeo, const Geography::Ring &aRing) {
    aGeo.addRing(aRing);
    return aGeo;
}
} /* namespace voltdb */

#endif /* INCLUDE_GEOGRAPHY_H_ */
