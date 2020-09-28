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

#ifndef VOLTDB_PARAMETERSET_HPP_
#define VOLTDB_PARAMETERSET_HPP_
#include "Parameter.hpp"
#include <vector>
#include "Table.h"
#include "boost/shared_ptr.hpp"
#include "Decimal.hpp"
#include "GeographyPoint.hpp"
#include "Geography.hpp"


namespace voltdb {
class Procedure;


/*
 * Datatype to support binary arrays by pointer
 */
typedef struct _buffer_t{
    _buffer_t(){
        _data = NULL;
        _size = 0;
    }
    size_t size() const {return _size;}
    const uint8_t* data() const {return _data;}

    _buffer_t(const uint8_t* p, size_t s):_data(p),_size(s){}
    _buffer_t(const char* p, size_t s):_data((const uint8_t*)p),_size(s){}
    const uint8_t*   _data;
    size_t           _size;
} buffer_t;

/*
 * Class for setting the parameters to a stored procedure one at a time. Parameters must be set from first
 * to last, and every parameter must set for each invocation.
 */
class ParameterSet {
    friend class Procedure;
public:


    /**
     * Add a payload that could be of WIRE_TYPE_VARBINARY or WIRE_TYPE_STRING type
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    template <WireType T>
    ParameterSet& addTypedBytes(const int32_t size, const uint8_t *value)
    throw (voltdb::ParamMismatchException) {
        assert(WIRE_TYPE_VARBINARY == T || WIRE_TYPE_STRING == T);
        validateType(T, false);
        if (value) {
            m_buffer.ensureRemaining(1 + 4 + size);
            m_buffer.putInt8(T);
            m_buffer.putBytes(size, value);
        m_currentParam++;
        } else addNull();
        return *this;
    }

    /**
    * Add an array of payloads for the current parameter, that could be of WIRE_TYPE_VARBINARY or WIRE_TYPE_STRING types
    * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
    * @return Reference to this parameter set to allow invocation chaining.
    */
    template <WireType T, typename ItemType>
    ParameterSet& addTypedByteArray(const typename std::vector<ItemType> &vals)
        throw (voltdb::ParamMismatchException) {
       assert(WIRE_TYPE_VARBINARY==T || WIRE_TYPE_STRING == T);
       validateType(T, true);
       int32_t totalByteSize = 0;
       for (typename std::vector<ItemType>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
           //calculate total size of array of buffers
           totalByteSize += i->size();
       }
       const int32_t size = static_cast<int32_t>(vals.size());
       m_buffer.ensureRemaining(4 + totalByteSize + (4*size));
       m_buffer.putInt8(WIRE_TYPE_ARRAY);
       m_buffer.putInt8(T);
       m_buffer.putInt16(size);
       for (typename std::vector<ItemType>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
           m_buffer.putBytes(i->size(), (const uint8_t*)i->data());
       }
       m_currentParam++;
       return *this;
    }

    /**
     * Add a binary payload
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addBytes(const int32_t size, const uint8_t *value)
    throw (voltdb::ParamMismatchException) {
        return addTypedBytes<WIRE_TYPE_VARBINARY>(size, value);
    }

    /**
    * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
    * @return Reference to this parameter set to allow invocation chaining.
    */
   ParameterSet& addBytes(const std::vector<buffer_t> &vals)
   throw (voltdb::ParamMismatchException) {
       return addTypedByteArray<WIRE_TYPE_VARBINARY, buffer_t>(vals);
   }

    /**
     * Add a decimal value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addDecimal(Decimal val) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_DECIMAL, false);
        m_buffer.ensureRemaining(static_cast<int32_t>(sizeof(Decimal)) + 1);
        m_buffer.putInt8(WIRE_TYPE_DECIMAL);
        val.serializeTo(&m_buffer);
        m_currentParam++;
        return *this;
    }

    /**
     * Add an array of decimal values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addDecimal(const std::vector<Decimal>& vals) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_DECIMAL, true);
        m_buffer.ensureRemaining(4 + static_cast<int32_t>(sizeof(Decimal) * vals.size()));
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_DECIMAL);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        for (std::vector<Decimal>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            i->serializeTo(&m_buffer);
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add a timestamp value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addTimestamp(int64_t val) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_TIMESTAMP, false);
        m_buffer.ensureRemaining(9);
        m_buffer.putInt8(WIRE_TYPE_TIMESTAMP);
        m_buffer.putInt64(val);
        m_currentParam++;
        return *this;
    }

    /**
     * Add an array of timestamp values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addTimestamp(const std::vector<int64_t>& vals) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_TIMESTAMP, true);
        m_buffer.ensureRemaining(4 + static_cast<int32_t>(sizeof(int64_t) * vals.size()));
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_TIMESTAMP);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        for (std::vector<int64_t>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            m_buffer.putInt64(*i);
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add an int64_t/BIGINT value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt64(int64_t val) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_BIGINT, false);
        m_buffer.ensureRemaining(9);
        m_buffer.putInt8(WIRE_TYPE_BIGINT);
        m_buffer.putInt64(val);
        m_currentParam++;
        return *this;
    }

    /**
     * Add an array of int64_t/BIGINT values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt64(const std::vector<int64_t>& vals) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_BIGINT, true);
        m_buffer.ensureRemaining(4 + static_cast<int32_t>(sizeof(int64_t) * vals.size()));
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_BIGINT);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        for (std::vector<int64_t>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            m_buffer.putInt64(*i);
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add an int32_t/INTEGER value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt32(int32_t val) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_INTEGER, false);
        m_buffer.ensureRemaining(5);
        m_buffer.putInt8(WIRE_TYPE_INTEGER);
        m_buffer.putInt32(val);
        m_currentParam++;
        return *this;
    }

    /**
     * Add an array of int32_t/INTEGER values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt32(const std::vector<int32_t>& vals) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_INTEGER, true);
        m_buffer.ensureRemaining(4 + static_cast<int32_t>(sizeof(int32_t) * vals.size()));
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_INTEGER);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        for (std::vector<int32_t>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            m_buffer.putInt32(*i);
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add an int16_t/SMALLINT value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt16(int16_t val) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_SMALLINT, false);
        m_buffer.ensureRemaining(3);
        m_buffer.putInt8(WIRE_TYPE_SMALLINT);
        m_buffer.putInt16(val);
        m_currentParam++;
        return *this;
    }

    /**
     * Add an array of int16_t/SMALLINT values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt16(const std::vector<int16_t>& vals) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_SMALLINT, true);
        m_buffer.ensureRemaining(4 + static_cast<int32_t>(sizeof(int16_t) * vals.size()));
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_SMALLINT);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        for (std::vector<int16_t>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            m_buffer.putInt16(*i);
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add an int8_t/TINYINT value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt8(int8_t val) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_TINYINT, false);
        m_buffer.ensureRemaining(2);
        m_buffer.putInt8(WIRE_TYPE_TINYINT);
        m_buffer.putInt8(val);
        m_currentParam++;
        return *this;
    }

    /**
     * Add an array of int8_t/TINYINT values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addInt8(const std::vector<int8_t>& vals) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_TINYINT, true);
        m_buffer.ensureRemaining(6 + static_cast<int32_t>(sizeof(int8_t) * vals.size()));
        // Convert array of TINYINT to VARBINARY.
        m_buffer.putInt8(WIRE_TYPE_VARBINARY);
        m_buffer.putInt32(static_cast<int32_t>(vals.size()));
        for (std::vector<int8_t>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            m_buffer.putInt8(*i);
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add a double value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addDouble(double val) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_FLOAT, false);
        m_buffer.ensureRemaining(9);
        m_buffer.putInt8(WIRE_TYPE_FLOAT);
        m_buffer.putDouble(val);
        m_currentParam++;
        return *this;
    }

    /**
     * Add an array of double values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addDouble(const std::vector<double>& vals) throw (voltdb::ParamMismatchException) {
        validateType(WIRE_TYPE_FLOAT, true);
        m_buffer.ensureRemaining(2 + static_cast<int32_t>(sizeof(double) * vals.size()));
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_FLOAT);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        for (std::vector<double>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            m_buffer.putDouble(*i);
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add null for the current parameter. The meaning of this can be tricky. This results in the SQL null
     * value used by Volt being sent across the wire so that it will represent SQL NULL if inserted into the
     * database. For numbers this the minimum representable value for the type. For strings this results in a null
     * object reference being passed to the procedure.
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addNull() throw (voltdb::ParamMismatchException) {
        if (m_currentParam > m_parameters.size()) {
            throw new ParamMismatchException();
        }
        m_buffer.ensureRemaining(1);
        m_buffer.putInt8(WIRE_TYPE_NULL);
        m_currentParam++;
        return *this;
    }

    /**
     * Add a string value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addString(const int32_t size, const char* data)
    throw (voltdb::ParamMismatchException) {
        return addTypedBytes<WIRE_TYPE_STRING>(size, (const uint8_t*)data);
    }

    /**
     * Add a string value for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addString(const std::string& val)
    throw (voltdb::ParamMismatchException) {
        return addString(val.size(), val.data());
    }

    /**
     * Add an array of string values for the current parameter
     * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
     * @return Reference to this parameter set to allow invocation chaining.
     */
    ParameterSet& addString(const std::vector<std::string>& vals)
    throw (voltdb::ParamMismatchException) {
        return addTypedByteArray<WIRE_TYPE_STRING, std::string>(vals);
        }

    /**
    * Add an array of string representing by pointer
    * @throws ParamMismatchException Supplied parameter is the wrong type for this position or too many have been set
    * @return Reference to this parameter set to allow invocation chaining.
    */
    ParameterSet& addString(const std::vector<buffer_t> &vals)
    throw (voltdb::ParamMismatchException) {
        return addTypedByteArray<WIRE_TYPE_STRING, buffer_t>(vals);
    }

    /**
     * Add a single GeographyPoint.
     */
    ParameterSet& addGeographyPoint(const GeographyPoint &val) {
        validateType(WIRE_TYPE_GEOGRAPHY_POINT, false);
        // One byte for the type and 2*sizeof(double) bytes
        // for the payload.
        m_buffer.ensureRemaining(1 + 2*sizeof(double));
        m_buffer.putInt8(WIRE_TYPE_GEOGRAPHY_POINT);
        m_buffer.putDouble(val.getLongitude());
        m_buffer.putDouble(val.getLatitude());
        m_currentParam++;
        return *this;
    }

    /**
     * Add a vector of GeographyPoints.
     */
    ParameterSet& addGeographyPoint(const std::vector<GeographyPoint> &vals) {
        validateType(WIRE_TYPE_GEOGRAPHY_POINT, true);
        // 1 byte for the array marker
        // 1 byte for the type marker (WIRE_TYPE_GEOGRAPHY_POINT)
        // 2 bytes for the array size.
        // -------
        // 4 bytes + 2*sizeof(double) * the array count.
        m_buffer.ensureRemaining(4 + static_cast<int32_t>(2*sizeof(double) * vals.size()));
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_GEOGRAPHY_POINT);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        for (std::vector<GeographyPoint>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
            m_buffer.putDouble(i->getLongitude());
            m_buffer.putDouble(i->getLatitude());
        }
        m_currentParam++;
        return *this;
    }

    /**
     * Add a single Geography.
     */
    ParameterSet& addGeography(const Geography &val) {
        validateType(WIRE_TYPE_GEOGRAPHY, false);
        // getSerializedSize() will return space for the
        // size.
        int32_t valSize = val.getSerializedSize();
        m_buffer.ensureRemaining(1 + valSize);
        // serializeTo will put the size in the buffer.
        m_buffer.putInt8(WIRE_TYPE_GEOGRAPHY);
        int realSize = val.serializeTo(m_buffer);
        assert(valSize == realSize);
        m_currentParam++;
        return *this;
    }

    /**
     * Add a vector of Geographys.
     */
    ParameterSet& addGeography(const std::vector<Geography> &vals) {
        validateType(WIRE_TYPE_GEOGRAPHY, true);
        int32_t valSize = 4;
        for (std::vector<Geography>::const_iterator idx = vals.begin();
             idx != vals.end();
             idx++) {
            valSize += idx->getSerializedSize();
        }
        m_buffer.ensureRemaining(valSize);
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_GEOGRAPHY);
        m_buffer.putInt16(static_cast<int16_t>(vals.size()));
        int realSize = 4;
        for (std::vector<Geography>::const_iterator idx = vals.begin();
             idx != vals.end();
             idx++) {
            realSize += idx->serializeTo(m_buffer);
        }
        assert(valSize == realSize);
        m_currentParam++;
        return *this;
    }

    /**
     * Add single table
     */
    ParameterSet& addTable(Table &table) {
        validateType(WIRE_TYPE_VOLTTABLE, false);
        int32_t tableSerializeSize = table.getSerializedSize();
        m_buffer.ensureRemaining(1 + tableSerializeSize);
        m_buffer.putInt8(WIRE_TYPE_VOLTTABLE);
        int32_t serializedSize = table.serializeTo(m_buffer);
        assert(serializedSize == tableSerializeSize);
        m_currentParam++;
        return *this;
    }

    /**
     * Add vector of tables
     */
    ParameterSet& addTable(std::vector<Table> &table) {
        validateType(WIRE_TYPE_VOLTTABLE, true);
        int32_t cummulativeSerializeTableSize = 0;
        const size_t tableCount = table.size();
        for (std::vector<Table>::iterator itr = table.begin(); itr != table.end(); ++itr) {
            cummulativeSerializeTableSize += itr->getSerializedSize();
        }

        // Array element (1) + table type (1) + table count (2) + size of serialized data
        m_buffer.ensureRemaining(1 + 1 + 2 + cummulativeSerializeTableSize);
        m_buffer.putInt8(WIRE_TYPE_ARRAY);
        m_buffer.putInt8(WIRE_TYPE_VOLTTABLE);
        m_buffer.putInt16(tableCount);

        int32_t serializedTablesSize = 0;
        for (std::vector<Table>::iterator itr = table.begin(); itr != table.end(); ++itr) {
            serializedTablesSize += itr->serializeTo(m_buffer);
        }
        assert(serializedTablesSize == cummulativeSerializeTableSize);
        m_currentParam++;
        return *this;
    }

    /**
     * Reset the parameter set so that a new set of parameters can be added. It is not necessary
     * to call this between invocations because the API will call it after the procedure this parameter
     * set is associated with is invoked.
     */
    void reset() {
        m_buffer.clear();
        m_currentParam = 0;
        m_buffer.putInt16(static_cast<int16_t>(m_parameters.size()));
    }

    int32_t getSerializedSize() {
        if (m_currentParam != m_parameters.size()) {
            throw UninitializedParamsException();
        }
        return m_buffer.position();
    }

#ifdef SWIG
%ignore serializeTo;
#endif
    void serializeTo(ByteBuffer *buffer) {
        if (m_currentParam != m_parameters.size()) {
            throw UninitializedParamsException();
        }
        m_buffer.flip();
        buffer->put(&m_buffer);
        reset();
        //once serialized parameters count cant be extended
        m_dynamicParamCount = false;
    }

    uint32_t size() const {
        return m_currentParam;
    }

    bool empty() const {
        return (m_currentParam <= 0);
    }

private:

    ParameterSet(std::vector<Parameter> parameters):
        m_parameters(parameters)
       ,m_buffer(8192)
       ,m_currentParam(0)
       ,m_dynamicParamCount(false) {
        m_buffer.putInt16(static_cast<int16_t>(m_parameters.size()));
    }

    ParameterSet(): m_buffer(8192), m_currentParam(0), m_dynamicParamCount(true) {
        m_parameters.clear();
    }

    void putParametersSize() {
        m_buffer.putInt16(0, static_cast<int16_t>(m_parameters.size()));
    }

    void validateType(WireType type, bool isArray) {
        if ((m_currentParam+1) > m_parameters.size() && m_dynamicParamCount) {
            m_parameters.push_back(Parameter(type, isArray));
            putParametersSize();
        }

        if (m_currentParam >= m_parameters.size() ||
                m_parameters[m_currentParam].m_type != type ||
                m_parameters[m_currentParam].m_array != isArray) {
            throw ParamMismatchException(static_cast<size_t>(type), wireTypeToString(type));
        }
    }

    std::vector<Parameter> m_parameters;
    ScopedByteBuffer m_buffer;
    uint32_t m_currentParam;
    bool m_dynamicParamCount;
};
}
#endif /* VOLTDB_PARAMETERSET_HPP_ */
