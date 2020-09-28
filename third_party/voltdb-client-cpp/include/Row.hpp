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

#ifndef VOLTDB_ROW_HPP_
#define VOLTDB_ROW_HPP_

#include "ByteBuffer.hpp"
#include "Column.hpp"
#include <string>
#include <vector>
#include "Exception.hpp"
#include "WireType.h"
#include <stdint.h>
#include "Decimal.hpp"
#include <sstream>
#include "Geography.hpp"
#include "GeographyPoint.hpp"

namespace voltdb {

/*
 * Representation of a row of tabular data from a Table. Every row contains a shared pointer
 * to its origin table which may also contain a shared pointer to the message the table was originally
 * from. Retain references with care to avoid memory leaks.
 */
class Row {
public:
    /*
     * Construct a row from a buffer containing the row data.
     */
#ifdef SWIG
    %ignore Row(SharedByteBuffer rowData, boost::shared_ptr<std::vector<voltdb::Column> > columns);
#endif
    Row(SharedByteBuffer& rowData, boost::shared_ptr<std::vector<voltdb::Column> >& columns) :
        m_data(rowData), m_columns(columns), m_wasNull(false), m_offsets(columns->size()), m_hasCalculatedOffsets(false) {
    }

    /*
     * Retrieve the value at the specified column index as bytes. The type of the column
     * must be Varbinary.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return Whether the buffer provided was large enough.
     */
    bool getVarbinary(int32_t column, int32_t bufsize, uint8_t *out_value, int32_t *out_len) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_VARBINARY, column);
        return m_data.getBytes(getOffset(column), m_wasNull, bufsize, out_value, out_len);
    }

    /*
     * Retrieve the value at the specified column index as a Decimal. The type of the column
     * must be Decimal.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return Decimal value at the specified column
     */
    Decimal getDecimal(int32_t column) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_DECIMAL, column);
        char data[16];
        m_data.get(getOffset(column), data, 16);
        Decimal retval(data);
        m_wasNull = retval.isNull();
        return retval;
    }

    /*
     * Retrieve the value at the specified column index as a Timestamp. The type of the column
     * must be Timestamp.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return Timestamp value at the specified column
     */
    int64_t getTimestamp(int32_t column) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_TIMESTAMP, column);
        int64_t retval = m_data.getInt64(getOffset(column));
        if (retval == INT64_MIN) m_wasNull = true;
        return retval;
    }

    /*
     * Retrieve the value at the specified column index as an int64_t/BIGINT. The type of the column
     * must be BIGINT, INTEGER, SMALLINT, or TINYINT.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return int64 value at the specified column
     */
    int64_t getInt64(int32_t column) throw(voltdb::InvalidColumnException) {
        WireType type = validateType(WIRE_TYPE_BIGINT, column);
        int64_t retval;
        switch (type) {
        case WIRE_TYPE_BIGINT:
            retval = m_data.getInt64(getOffset(column));
            if (retval == INT64_MIN) m_wasNull = true;
            break;
        case WIRE_TYPE_INTEGER:
            retval = m_data.getInt32(getOffset(column));
            if (retval == INT32_MIN) m_wasNull = true;
            break;
        case WIRE_TYPE_SMALLINT:
            retval = m_data.getInt16(getOffset(column));
            if (retval == INT16_MIN) m_wasNull = true;
            break;
        case WIRE_TYPE_TINYINT:
            retval = m_data.getInt8(getOffset(column));
            if (retval == INT8_MIN) m_wasNull = true;
            break;
        default:
            assert(false);
        }
        return retval;
    }

    /*
     * Retrieve the value at the specified column index as an int32_t/INTEGER. The type of the column
     * must be INTEGER, SMALLINT, or TINYINT.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return int32 value at the specified column
     */
    int32_t getInt32(int32_t column) throw(voltdb::InvalidColumnException) {
        WireType type = validateType(WIRE_TYPE_INTEGER, column);
        int32_t retval;
        switch (type) {
        case WIRE_TYPE_INTEGER:
            retval = m_data.getInt32(getOffset(column));
            if (retval == INT32_MIN) m_wasNull = true;
            break;
        case WIRE_TYPE_SMALLINT:
            retval = m_data.getInt16(getOffset(column));
            if (retval == INT16_MIN) m_wasNull = true;
            break;
        case WIRE_TYPE_TINYINT:
            retval = m_data.getInt8(getOffset(column));
            if (retval == INT8_MIN) m_wasNull = true;
            break;
        default:
            assert(false);
        }
        return retval;
    }

    /*
     * Retrieve the value at the specified column index as an int16_t/SMALLINT. The type of the column
     * must be SMALLINT, or TINYINT.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return int16 value at the specified column
     */
    int16_t getInt16(int32_t column) throw(voltdb::InvalidColumnException) {
        WireType type = validateType(WIRE_TYPE_SMALLINT, column);
        int16_t retval;
        switch (type) {
        case WIRE_TYPE_SMALLINT:
            retval = m_data.getInt16(getOffset(column));
            if (retval == INT16_MIN) m_wasNull = true;
            break;
        case WIRE_TYPE_TINYINT:
            retval = m_data.getInt8(getOffset(column));
            if (retval == INT8_MIN) m_wasNull = true;
            break;
        default:
            assert(false);
        }
        return retval;
    }

    /*
     * Retrieve the value at the specified column index as an int8_t/TINYINT. The type of the column
     * must be TINYINT.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return int8 value at the specified column
     */
    int8_t getInt8(int32_t column) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_TINYINT, column);
        int8_t retval = m_data.getInt8(getOffset(column));
        if (retval == INT8_MIN) {
            m_wasNull = true;
        }
        return retval;
    }

    /*
     * Retrieve the value at the specified column index as a double. The type of the column
     * must be FLOAT.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return double value at the specified column
     */
    double getDouble(int32_t column) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_FLOAT, column);
        double retval = m_data.getDouble(getOffset(column));
        if (retval <= -1.7E+308) {
            m_wasNull = true;
        }
        return retval;
    }

    /*
     * Retrieve the value at the specified column index as an string. The type of the column
     * must be STRING.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return String value at the specified column
     */
    std::string getString(int32_t column) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_STRING, column);
        return m_data.getString(getOffset(column), m_wasNull);
    }

    /*
     * Retrieve the value at the specified column index as a geographical point.
     *
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return the deserialized GeographyPoint value.
     */
    GeographyPoint getGeographyPoint(int32_t column) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_GEOGRAPHY_POINT, column);
        GeographyPoint gpoint(m_data, getOffset(column), m_wasNull);
        return gpoint;
    }

    /*
     * Retrieve the value at the specified column index as a geographical object.
     *
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return the deserialized geography value.
     */
    Geography getGeography(int32_t column) throw(voltdb::InvalidColumnException) {
        validateType(WIRE_TYPE_GEOGRAPHY, column);
        m_wasNull = false;
        return Geography(m_data, getOffset(column), m_wasNull);
    }

    /*
     * Returns true if the value at the specified column index is NULL and false otherwise
     * @throws InvalidColumnException The name of the column was invalid.
     * @return true if the value is NULL and false otherwise
     */
    bool isNull(int32_t column) throw(voltdb::InvalidColumnException, voltdb::UnsupportedTypeException) {
        if (column < 0 || column >= static_cast<ssize_t>(m_columns->size())) {
            throw InvalidColumnException(column, m_columns->size());
        }
        WireType columnType = m_columns->at(static_cast<size_t>(column)).type();
        switch (columnType) {
        case WIRE_TYPE_DECIMAL:
            getDecimal(column); break;
        case WIRE_TYPE_TIMESTAMP:
            getTimestamp(column); break;
        case WIRE_TYPE_BIGINT:
            getInt64(column); break;
        case WIRE_TYPE_INTEGER:
            getInt32(column); break;
        case WIRE_TYPE_SMALLINT:
            getInt64(column); break;
        case WIRE_TYPE_TINYINT:
            getInt8(column); break;
        case WIRE_TYPE_FLOAT:
            getDouble(column); break;
        case WIRE_TYPE_STRING:
            getString(column); break;
        case WIRE_TYPE_VARBINARY:
            int out_len;
            getVarbinary(column, 0, NULL, &out_len); break;
        case WIRE_TYPE_GEOGRAPHY:
            getGeography(column); break;
        case WIRE_TYPE_GEOGRAPHY_POINT:
            getGeographyPoint(column); break;
        default:
            throw UnsupportedTypeException(wireTypeToString(columnType));
        }
        return wasNull();
    }

    /*
     * Retrieve the value from the column with the specified name as bytes. The type of the column
     * must be Varbinary.
     * @throws InvalidColumnException The index of the column was invalid or the type of the column does
     * not match the type of the get method.
     * @return Whether the buffer provided was large enough.
     */
    bool getVarbinary(const std::string& cname, int32_t bufsize, uint8_t *out_value, int32_t *out_len)
    throw(voltdb::InvalidColumnException) {
        return getVarbinary(getColumnIndexByName(cname), bufsize, out_value, out_len);
    }

    /*
     * Retrieve the value from the column with the specified name as a Decimal. The type of the column
     * must be Decimal.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return Decimal value at the specified column
     */
    Decimal getDecimal(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getDecimal(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value from the column with the specified name as a Timestamp. The type of the column
     * must be Timestamp.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return Timestamp value at the specified column
     */
    int64_t getTimestamp(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getTimestamp(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value from the column with the specified name as a int64_t/BIGINT. The type of the column
     * must be BIGINT, INTEGER, SMALLINT, TINYINT.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return int64 value at the specified column
     */
    int64_t getInt64(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getInt64(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value from the column with the specified name as a int32_t/INTEGER. The type of the column
     * must be INTEGER, SMALLINT, TINYINT.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return int32 value at the specified column
     */
    int32_t getInt32(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getInt32(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value from the column with the specific name as a int16_t/SMALLINT. The type of the column
     * must be SMALLINT, TINYINT.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return int16 value at the specified column
     */
    int16_t getInt16(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getInt16(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value from the column with the specific name as a int8_t/TINYINT. The type of the column
     * must be TINYINT.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return int8 value at the specified column
     */
    int8_t getInt8(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getInt8(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value from the column with the specific name as a double. The type of the column
     * must be FLOAT.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return double value at the specified column
     */
    double getDouble(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getDouble(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value from the column with the specific name as a std::string. The type of the column
     * must be STRING.
     * @throws InvalidColumnException No column with the specified name exists or the type of the getter
     * does not match the column type.
     * @return string value at the specified column
     */
    std::string getString(const std::string& cname) throw(voltdb::InvalidColumnException) {
        return getString(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value at the specified column name as a geographical point.
     * This data type is not currently supported by the C++ client so this always
     * throws an exception.
     * @throws UnsupportedTypeException always
     * @return never returns
     */
    GeographyPoint getGeographyPoint(const std::string& cname) throw(UnsupportedTypeException) {
        return getGeographyPoint(getColumnIndexByName(cname));
    }

    /*
     * Retrieve the value at the specified column name as a geographical object.
     * This data type is not currently supported by the C++ client so this always
     * throws an exception.
     * @throws UnsupportedTypeException always
     * @return never returns
     */
    Geography getGeography(const std::string& cname) throw(UnsupportedTypeException) {
        return getGeography(getColumnIndexByName(cname));
    }

    /*
     * Returns true if the value in the column with the specified name is NULL and false otherwise
     * @throws InvalidColumnException The name of the column was invalid.
     * @return true if the value is NULL and false otherwise
     */
    bool isNull(const std::string& cname) throw(voltdb::InvalidColumnException) {
        int32_t column = getColumnIndexByName(cname);
        return isNull(column);
    }

    /*
     * Returns true if the value of the last column returned via a getter was null
     */
    bool wasNull() {
        return m_wasNull;
    }

    /*
     * Returns a string representation of this row
     */
    std::string toString() {
        std::ostringstream ostream;
        toString(ostream, std::string(""));
        return ostream.str();
    }

    /*
     * Returns a string representation of this row with the specified level of indentation
     * before each line
     */
    void toString(std::ostringstream &ostream, const std::string& indent) {
        ostream << indent;
        const int32_t size = static_cast<int32_t>(m_columns->size());
        for (int32_t ii = 0; ii < size; ii++) {
            if (ii != 0) {
                ostream << ", ";
            }
            if (isNull(ii)) {
                ostream << "NULL";
                continue;
            }
            switch(m_columns->at(ii).type()) {
            case WIRE_TYPE_TINYINT:
                ostream << static_cast<int32_t>(getInt8(ii)); break;
            case WIRE_TYPE_SMALLINT:
                ostream << getInt16(ii); break;
            case WIRE_TYPE_INTEGER:
                ostream << getInt32(ii); break;
            case WIRE_TYPE_BIGINT:
                ostream << getInt64(ii); break;
            case WIRE_TYPE_FLOAT:
                ostream << getDouble(ii); break;
            case WIRE_TYPE_STRING:
                ostream << "\"" << getString(ii) << "\""; break;
            case WIRE_TYPE_TIMESTAMP:
                ostream << getTimestamp(ii); break;
            case WIRE_TYPE_DECIMAL:
                ostream << getDecimal(ii).toString(); break;
            case WIRE_TYPE_VARBINARY:
                ostream << "VARBINARY VALUE"; break;
            case WIRE_TYPE_GEOGRAPHY_POINT:
                ostream << getGeographyPoint(ii).toString();
                break;
            case WIRE_TYPE_GEOGRAPHY:
                ostream << getGeography(ii).toString();
                break;
            default:
                assert(false);
            }
        }
    }

    int32_t columnCount() const {
        return static_cast<int32_t>(m_columns->size());
    }

    std::vector<voltdb::Column> columns() const {
        return *m_columns;
    }
private:
    WireType validateType(WireType type, int32_t index)  throw (InvalidColumnException) {
        if (index < 0 ||
                index >= static_cast<ssize_t>(m_columns->size())) {
            throw InvalidColumnException(index, m_columns->size());
        }
        WireType columnType = m_columns->at(static_cast<size_t>(index)).type();
        switch (columnType) {
        case WIRE_TYPE_INTEGER:
            if (type != WIRE_TYPE_BIGINT && type != WIRE_TYPE_INTEGER) {
                throw InvalidColumnException(getColumnNameByIndex(index), type, wireTypeToString(type), wireTypeToString(WIRE_TYPE_INTEGER));
            }
            break;
        case WIRE_TYPE_SMALLINT:
            if (type != WIRE_TYPE_BIGINT &&
                    type != WIRE_TYPE_INTEGER &&
                    type != WIRE_TYPE_SMALLINT) {
                throw InvalidColumnException(getColumnNameByIndex(index), type, wireTypeToString(type), wireTypeToString(WIRE_TYPE_SMALLINT));
            }
            break;
        case WIRE_TYPE_TINYINT:
            if (type != WIRE_TYPE_BIGINT &&
                    type != WIRE_TYPE_INTEGER &&
                    type != WIRE_TYPE_SMALLINT &&
                    type != WIRE_TYPE_TINYINT) {
                throw InvalidColumnException(getColumnNameByIndex(index), type, wireTypeToString(type), wireTypeToString(WIRE_TYPE_TINYINT));
            }
            break;
        case WIRE_TYPE_TIMESTAMP:
        case WIRE_TYPE_BIGINT:
        case WIRE_TYPE_FLOAT:
        case WIRE_TYPE_STRING:
        case WIRE_TYPE_VARBINARY:
        case WIRE_TYPE_GEOGRAPHY:
        case WIRE_TYPE_GEOGRAPHY_POINT:
        case WIRE_TYPE_DECIMAL:
            /*
             * No implicit conversions for these types.  The column type
             * and the actual type must be identical.
             */
            if (columnType != type) {
                throw InvalidColumnException(getColumnNameByIndex(index), type, wireTypeToString(type), wireTypeToString(columnType));
            }
            break;
        default:
            throw UnsupportedTypeException(wireTypeToString(columnType));
            break;
        }
        return columnType;
    }

    int32_t getColumnIndexByName(const std::string& name) {
        for (int32_t ii = 0; ii < static_cast<ssize_t>(m_columns->size()); ii++) {
            if (m_columns->at(static_cast<size_t>(ii)).name() == name) {
                return ii;
            }
        }
        throw InvalidColumnException(name);
    }

    const std::string& getColumnNameByIndex(int32_t index){
        return m_columns->at(index).name();
    }

    void ensureCalculatedOffsets() {
        if (m_hasCalculatedOffsets == true) return;
        m_offsets[0] = m_data.position();
        for (int32_t i = 1; i < static_cast<ssize_t>(m_columns->size()); i++) {
            //
            // To calculate the offset at index i, given that the
            // offset at i-1 is correct. we calculate the size of the
            // data at offset i-1 and add it to the offset at index i-1,
            // to get the index at i.
            //
            WireType type = m_columns->at(static_cast<size_t>(i - 1)).type();
            if (isVariableSized(type)) {
                int32_t length = m_data.getInt32(m_offsets[static_cast<size_t>(i - 1)]);
                if (length == -1) {
                    m_offsets[static_cast<size_t>(i)] = m_offsets[static_cast<size_t>(i - 1)] + 4;
                } else if (length < 0) {
                    assert(false);
                } else {
                    m_offsets[static_cast<size_t>(i)] = m_offsets[static_cast<size_t>(i - 1)] + length + 4;
                }
            } else {
                //
                // These are all fixed sized data values.
                //
                int32_t length = 0;
                switch (type) {
                case WIRE_TYPE_DECIMAL:
                    length = 16;
                    break;
                case WIRE_TYPE_TIMESTAMP:
                case WIRE_TYPE_BIGINT:
                case WIRE_TYPE_FLOAT:
                    length = 8;
                    break;
                case WIRE_TYPE_INTEGER:
                    length = 4;
                    break;
                case WIRE_TYPE_SMALLINT:
                    length = 2;
                    break;
                case WIRE_TYPE_TINYINT:
                    length = 1;
                    break;
                case WIRE_TYPE_GEOGRAPHY_POINT:
                    length = 2 * sizeof(double);
                    break;
                default:
                    throw UnsupportedTypeException(wireTypeToString(type));
                }
                m_offsets[static_cast<size_t>(i)] = m_offsets[static_cast<size_t>(i - 1)] + length;
            }
        }
        m_hasCalculatedOffsets = true;
    }

    int32_t getOffset(int32_t index) {
        m_wasNull = false;
        ensureCalculatedOffsets();
        assert(index >= 0);
        assert(static_cast<size_t>(index) < m_offsets.size());
        return m_offsets[static_cast<size_t>(index)];
    }

    SharedByteBuffer m_data;
    boost::shared_ptr<std::vector<voltdb::Column> > m_columns;
    bool m_wasNull;
    std::vector<int32_t> m_offsets;
    bool m_hasCalculatedOffsets;
};
}

#endif /* VOLTDB_ROW_HPP_ */
