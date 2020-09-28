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

#ifndef INCLUDE_GEOGRAPHYPOINT_H_
#define INCLUDE_GEOGRAPHYPOINT_H_
#include <stdint.h>

#include "Exception.hpp"
namespace voltdb {

class ByteBuffer;
/**
 * Represent a geography point. There are no virtual functions here,
 * so there is no destructor at all.  The default one is good enough
 * for us.
 */
#define DEFAULT_EQUALITY_EPSILON  1.0e-12

class GeographyPoint {
public:

    /**
     * The default constructor makes a null geography.
     */
    GeographyPoint();
    /**
     * The parameters must be in range. The longitude must
     * be in the range [-180 +180] and the latitude must
     * be in the range [-90, +90].  Note that we allow both
     * negative and positive longitude values at the
     * anti-meridian, when abs(longitude) == 180.  This is
     * because we want to allow positive values arbitrarily
     * close to but less than 180 and negative values
     * arbitrarily close to but less than -180.
     */
    GeographyPoint(double aLongitude, double aLatitude)
        : m_longitude(aLongitude),
          m_latitude(aLatitude) {
        if (aLatitude > 90.0 || aLatitude < -90.0) {
            throw CoordinateOutOfRangeException("Longitude");
        }
        if (aLongitude > 180.0 || aLongitude < -180.0) {
            throw CoordinateOutOfRangeException("Longitude");
        }
    }

    /**
     * This very specialized constructor is used when
     * constructing a geography point directly from a
     * serialized message.  The offset is the location in
     * the byte buffer in which the message is to be
     * found, and the wasNull parameter is set to tell
     * if the deserialized polygon was null.
     */
    GeographyPoint(ByteBuffer &buff, int32_t offset, bool &wasNull);

    /**
     * Get the longitude.
     */
    double getLongitude() const {
        return m_longitude;
    }

    /**
     * Get the latitude.
     */
    double getLatitude() const {
        return m_latitude;
    }


    std::string toString() const;

    /**
     * Return true iff this is a null GeographyPoint.
     */
    bool isNull() const {
        return m_longitude == NULL_COORDINATE && m_latitude == NULL_COORDINATE;
    }
    
    /**
     * Test that this is equal to the other.  We take care to compare points
     * on the poles properly, when the latitude is not defined, and to compare
     * points on the anti-meridian properly, where longitude values of -180
     * are equal to values of +180.
     *
     * Since this compares floating point values, we declare two points to
     * be equal if their coordinates are within DEFAULT_EQUALITY_EPSILON
     * of each other.  See voltdb::GeographyPoint::approximatelyEqual for
     * a more flexible alternative.
     */
    bool operator==(const GeographyPoint &aOther) const;

    bool operator!=(const GeographyPoint &aOther) const {
        return !operator==(aOther);
    }

    /**
     * Return a point which is this point translated by the
     * given offset.
     *
     * This is mostly used in testing.  We want to move points slightly,
     * so that we can test approximate equality of polygons.
     */
    GeographyPoint translate(const GeographyPoint &offset) {
        return GeographyPoint(getLongitude() + offset.getLongitude(),
                              getLatitude() + offset.getLatitude());
    }
    /**
     * Create a GeographyPoint from an XYZPoint coordinate triple.
     * See the VoltDB wire protocol specification for details.
     */
    static GeographyPoint fromXYZ(double x, double y, double z);
    
    /**
     * Return true iff two points are approximately equal.
     * This is true if their latitudes and longitudes are
     * within epsilon of each other, taking into account the
     * special rules for the poles and for the anti-meridian.
     *
     * Values in the range 1.0e-12 work well for epsilon.  This
     * is sub-millimeter precision for longitude and latitude.
     *
     * If epsilon is zero, then exact floating point equality is
     * required.
     */
    bool approximatelyEqual(const GeographyPoint &aOther,
                            double epsilon) const;
    
    /**
     * Convert from longitude/latitude to points on the unit
     * sphere.  See the VoltDB wire protocol specification for
     * details.
     */
    void getXYZCoordinates(double &x, double &y, double &z) const;
    /**
     * Deserialize a message.  See the VoltDBWireProtocol
     * for details.
     */
    int32_t deserializeFrom(ByteBuffer &message,
                            int32_t     offset,
                            bool       &wasNull);
    static constexpr double NULL_COORDINATE = 360.0;
private:
    double m_longitude;
    double m_latitude;

};

} /* namespace voltdb */

#endif /* INCLUDE_GEOGRAPHYPOINT_H_ */
