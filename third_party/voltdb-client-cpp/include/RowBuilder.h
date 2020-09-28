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

#ifndef VOLTDB_ROWBUILDER_H_
#define VOLTDB_ROWBUILDER_H_
#include "ByteBuffer.hpp"
#include "WireType.h"
#include "Table.h"
#include "Column.hpp"
#include "boost/shared_ptr.hpp"
#include <stdint.h>
#include "Exception.hpp"
#include "Decimal.hpp"
#include "Geography.hpp"
#include "GeographyPoint.hpp"

/* Helper class to build row given the table schema.
 * Column count and it's type is inferred using the table schema
 * provided at construction time.
 */

namespace voltdb {

class TableTest;

class RowBuilder {
friend class TableTest;
private:
    void validateType(WireType type) throw (InvalidColumnException, RowCreationException) {
        if (m_currentColumnIndex >= m_columns.size()) {
            throw InvalidColumnException(m_currentColumnIndex, m_columns.size());
        }

        if (m_columns[m_currentColumnIndex].type() != type) {
            std::string expectedTypeName = wireTypeToString(m_columns[m_currentColumnIndex].type());
            std::string typeName = wireTypeToString(type);
            throw InvalidColumnException(m_columns[m_currentColumnIndex].name(),
                    type, typeName, expectedTypeName);
        }
        if (m_columns[m_currentColumnIndex].type() == WIRE_TYPE_ARRAY) {
            // array types for row in row builder is not implemented at present
            throw RowCreationException("Support for array type when constructing row is not available at present");
        }
    }

public:
    // Initializes the Rowbuilder with schema of the table for which the row will be constructed
    RowBuilder(const std::vector<Column> &schema) throw (RowCreationException);


    RowBuilder& addInt64(int64_t val) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_BIGINT);
        m_buffer.ensureRemaining(8);
        m_buffer.putInt64(val);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addInt32(int32_t val) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_INTEGER);
        m_buffer.ensureRemaining(4);
        m_buffer.putInt32(val);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addInt16(int16_t val) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_SMALLINT);
        m_buffer.ensureRemaining(2);
        m_buffer.putInt16(val);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addInt8(int8_t val) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_TINYINT);
        m_buffer.ensureRemaining(1);
        m_buffer.putInt8(val);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addDouble(double val) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_FLOAT);
        m_buffer.ensureRemaining(8);
        m_buffer.putDouble(val);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addNull() throw (InvalidColumnException) {
        if (m_currentColumnIndex >= m_columns.size()) {
            throw InvalidColumnException(m_currentColumnIndex, m_columns.size());
        }

        switch (m_columns[m_currentColumnIndex].type()) {
        case WIRE_TYPE_TINYINT:
            addInt8(INT8_MIN);
            break;
        case WIRE_TYPE_SMALLINT:
            addInt16(INT16_MIN);
            break;
        case WIRE_TYPE_INTEGER:
            addInt32(INT32_MIN);
            break;
        case WIRE_TYPE_BIGINT:
            addInt64(INT64_MIN);
            break;
        case WIRE_TYPE_TIMESTAMP:
            addTimeStamp(INT64_MIN);
            break;
        case WIRE_TYPE_FLOAT:
            addDouble(-1.7976931348623157E+308);
            break;
        case WIRE_TYPE_STRING:
            m_buffer.ensureRemaining(4);
            m_buffer.putInt32(-1);
            m_currentColumnIndex++;
            break;
        case WIRE_TYPE_VARBINARY:
            m_buffer.ensureRemaining(4);
            m_buffer.putInt32(-1);
            m_currentColumnIndex++;
            break;
        case WIRE_TYPE_DECIMAL: {
            TTInt ttInt;
            ttInt.SetMin();
            Decimal dec(ttInt);
            dec.serializeTo(&m_buffer);
            m_currentColumnIndex++;
            break;
        }
        case WIRE_TYPE_GEOGRAPHY: {
            Geography polyNull;     // zero rings poly is null
            polyNull.serializeTo(m_buffer);
            m_currentColumnIndex++;
            break;
        }
        case WIRE_TYPE_GEOGRAPHY_POINT:
            m_buffer.ensureRemaining(2*sizeof(double));
            m_buffer.putDouble(GeographyPoint::NULL_COORDINATE);
            m_buffer.putDouble(GeographyPoint::NULL_COORDINATE);
            m_currentColumnIndex++;
            break;
        default:
            assert(false);
        }

        return *this;
    }

    RowBuilder& addString(const std::string& val) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_STRING);
        m_buffer.ensureRemaining(4 + static_cast<int32_t>(val.size()));
        m_buffer.putString(val);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addVarbinary(const int32_t bufsize, const uint8_t *in_value) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_VARBINARY);
        m_buffer.ensureRemaining(4 + bufsize);
        m_buffer.putBytes(bufsize, in_value);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addTimeStamp(int64_t value) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_TIMESTAMP);
        m_buffer.ensureRemaining(8);
        m_buffer.putInt64(value);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addDecimal(const Decimal& value) {
        validateType(WIRE_TYPE_DECIMAL);
        m_buffer.ensureRemaining(2 * sizeof (int64_t));
        value.serializeTo(&m_buffer);
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addGeographyPoint(const GeographyPoint &val) throw (InvalidColumnException, RowCreationException)  {
        validateType(WIRE_TYPE_GEOGRAPHY_POINT);
        // 2*sizeof(double) bytes for the payload.
        m_buffer.ensureRemaining(2*sizeof(double));
        m_buffer.putDouble(val.getLongitude());
        m_buffer.putDouble(val.getLatitude());
        m_currentColumnIndex++;
        return *this;
    }

    RowBuilder& addGeography(const Geography &val) throw (InvalidColumnException, RowCreationException) {
        validateType(WIRE_TYPE_GEOGRAPHY);
        int32_t valSize = val.getSerializedSize();
        m_buffer.ensureRemaining(1 + valSize);
        int realSize = val.serializeTo(m_buffer);
        m_currentColumnIndex++;
        return *this;
    }

    void reset() {
        m_buffer.clear();
        m_currentColumnIndex = 0;
    }

    /**
     * Serializes row data to supplied byte buffer (including size)
     * Precondition:  All the columns of the row schema should be
     * populated/initialized
     */
    int32_t serializeTo(ByteBuffer &buffer) throw (UninitializedColumnException) {
        if (m_currentColumnIndex != m_columns.size()) {
            throw UninitializedColumnException(m_columns.size(), m_currentColumnIndex);
        }

        int32_t startPosition = buffer.position();
        buffer.position(startPosition + 4);
        m_buffer.flip();
        buffer.put(&m_buffer);
        int32_t serializedBytes = buffer.position() - (startPosition + 4);
        buffer.putInt32(startPosition, serializedBytes);
        buffer.limit(buffer.position());
        reset();
        return buffer.limit() - startPosition;
    }

    /**
     * Returns the size in bytes needed to serialize the current row data
     * including size
     */

    int32_t getSerializedSize() const {
        return m_buffer.position() + 4;
    }

    /** Returns number of first 'n' populated columns for the given row.
     * For example 0 meaning none of the row columns are populated
     * 1 telling first column of the row has been populated.
     * Row does not support holes, so if return count is 3 it represents
     * all 3 columns from beginning have been populated
     */
    int32_t numberOfPopulatedColumns() const {
        return m_currentColumnIndex;
    }

    std::vector<voltdb::Column> columns() const { return m_columns; }
private:
    std::vector<voltdb::Column> m_columns;
    voltdb::ScopedByteBuffer m_buffer;
    uint32_t m_currentColumnIndex;
};
}

#endif /* VOLTDB_ROWBUILDER_H_ */
