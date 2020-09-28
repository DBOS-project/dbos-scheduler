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
/* Copyright (C) 2008 by H-Store Project
 * Brown University
 * Massachusetts Institute of Technology
 * Yale University
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
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VOLTDB_BYTEBUFFER_H
#define VOLTDB_BYTEBUFFER_H

#include <stdint.h>
#include <arpa/inet.h>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>
#include <string>
#include <cstring>
#include "Exception.hpp"

namespace voltdb {


#ifdef __DARWIN_OSSwapInt64 // for darwin/macosx

#define htonll(x) __DARWIN_OSSwapInt64(x)
#define ntohll(x) __DARWIN_OSSwapInt64(x)

#else // unix in general

//#undef htons
//#undef ntohs
//#define htons(x) static_cast<uint16_t>((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
//#define ntohs(x) static_cast<uint16_t>((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

#ifdef __bswap_64 // recent linux

#ifdef htonll
    #undef htonll
#endif
#define htonll(x) static_cast<uint64_t>(__bswap_constant_64(x))
#ifdef ntohll
    #undef ntohll
#endif
#define ntohll(x) static_cast<uint64_t>(__bswap_constant_64(x))

#else // unix in general again

#define htonll(x) (((int64_t)(ntohl((int32_t)((x << 32) >> 32))) << 32) | (uint32_t)ntohl(((int32_t)(x >> 32))))
#define ntohll(x) (((int64_t)(ntohl((int32_t)((x << 32) >> 32))) << 32) | (uint32_t)ntohl(((int32_t)(x >> 32))))

#endif // __bswap_64

#endif // unix or mac

class ByteBufferTest;
class ByteBuffer {
    static const int MAX_VALUE_LENGTH = 1024 * 1024; // 1Mb is the maximum allowed size of binary or string data
    friend class ByteBufferTest;
private:
    int32_t checkGetPutIndex(int32_t length) {
        if (m_limit - m_position < length || length < 0) {
            throw OverflowUnderflowException();
        }
        int32_t position = m_position;
        m_position += length;
        return position;
    }

    int32_t checkIndex(int32_t index, int32_t length) {
        if ((index < 0) || (length > m_limit - index) || length < 0) {
            throw IndexOutOfBoundsException();
        }
        return index;
    }
public:
    ByteBuffer& flip() {
        m_limit = m_position;
        m_position = 0;
        return *this;
    }
    ByteBuffer& clear() {
        m_limit = m_capacity;
        m_position = 0;
        return *this;
    }

    void get(char *storage, int32_t length) throw (OverflowUnderflowException) {
        ::memcpy(storage, &m_buffer[checkGetPutIndex(length)], static_cast<uint32_t>(length));
    }
    void get(int32_t index, char *storage, int32_t length) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        ::memcpy(storage, &m_buffer[checkIndex(index, length)], static_cast<uint32_t>(length));
    }
    ByteBuffer& put(const char *storage, int32_t length) throw (OverflowUnderflowException) {
        ::memcpy(&m_buffer[checkGetPutIndex(length)], storage, static_cast<uint32_t>(length));
        return *this;
    }
    ByteBuffer& put(int32_t index, const char *storage, int32_t length) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        ::memcpy(&m_buffer[checkIndex(index, length)], storage, static_cast<uint32_t>(length));
        return *this;
    }

    ByteBuffer& put(ByteBuffer *other) throw (OverflowUnderflowException) {
        int32_t oremaining = other->remaining();
        if (oremaining == 0) {
            return *this;
        }
        ::memcpy(&m_buffer[checkGetPutIndex(oremaining)],
                other->getByReference(oremaining),
                static_cast<uint32_t>(oremaining));
        return *this;
    }

    int8_t getInt8() throw (OverflowUnderflowException) {
        return m_buffer[checkGetPutIndex(1)];
    }
    int8_t getInt8(int32_t index) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        return m_buffer[checkIndex( index, 1)];
    }
    ByteBuffer& putInt8(int8_t value) throw (OverflowUnderflowException) {
        m_buffer[checkGetPutIndex(1)] = value;
        return *this;
    }
    ByteBuffer& putInt8(int32_t index, int8_t value) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        m_buffer[checkIndex( index, 1)] = value;
        return *this;
    }

    int16_t getInt16() throw (OverflowUnderflowException) {
        int16_t value;
        ::memcpy( &value, &m_buffer[checkGetPutIndex(2)], 2);
        return static_cast<int16_t>(ntohs(value));
    }
    int16_t getInt16(int32_t index) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        int16_t value;
        ::memcpy( &value, &m_buffer[checkIndex(index, 2)], 2);
        return static_cast<int16_t>(ntohs(value));
    }
    ByteBuffer& putInt16(int16_t value) throw (OverflowUnderflowException) {
        *reinterpret_cast<uint16_t*>(&m_buffer[checkGetPutIndex(2)]) = htons(value);
        return *this;
    }
    ByteBuffer& putInt16(int32_t index, int16_t value) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        *reinterpret_cast<uint16_t*>(&m_buffer[checkIndex(index, 2)]) = htons(value);
        return *this;
    }

    int32_t getInt32() throw (OverflowUnderflowException) {
        uint32_t value;
        ::memcpy( &value, &m_buffer[checkGetPutIndex(4)], 4);
        return static_cast<int32_t>(ntohl(value));
    }
    int32_t getInt32(int32_t index) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        uint32_t value;
        ::memcpy( &value, &m_buffer[checkIndex(index, 4)], 4);
        return static_cast<int32_t>(ntohl(value));
    }
    ByteBuffer& putInt32(int32_t value) throw (OverflowUnderflowException) {
        *reinterpret_cast<uint32_t*>(&m_buffer[checkGetPutIndex(4)]) = htonl(static_cast<uint32_t>(value));
        return *this;
    }
    ByteBuffer& putInt32(int32_t index, int32_t value) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        *reinterpret_cast<uint32_t*>(&m_buffer[checkIndex(index, 4)]) = htonl(static_cast<uint32_t>(value));
        return *this;
    }

    int64_t getInt64() throw (OverflowUnderflowException) {
        uint64_t value;
        ::memcpy( &value, &m_buffer[checkGetPutIndex(8)], 8);
        return static_cast<int64_t>(ntohll(value));
    }
    int64_t getInt64(int32_t index) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        uint64_t value;
        ::memcpy( &value, &m_buffer[checkIndex(index, 8)], 8);
        return static_cast<int64_t>(ntohll(value));
    }
    ByteBuffer& putInt64(int64_t value) throw (OverflowUnderflowException) {
        *reinterpret_cast<uint64_t*>(&m_buffer[checkGetPutIndex(8)]) = htonll(static_cast<uint64_t>(value));
        return *this;
    }
    ByteBuffer& putInt64(int32_t index, int64_t value) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        *reinterpret_cast<uint64_t*>(&m_buffer[checkIndex(index, 8)]) = htonll(static_cast<uint64_t>(value));
        return *this;
    }

    double getDouble() throw (OverflowUnderflowException) {
        uint64_t value;
        ::memcpy( &value, &m_buffer[checkGetPutIndex(8)], 8);
        value = ntohll(value);
        double retval;
        ::memcpy( &retval, &value, 8);
        return retval;
    }
    double getDouble(int32_t index) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        uint64_t value;
        ::memcpy( &value, &m_buffer[checkIndex(index, 8)], 8);
        value = ntohll(value);
        double retval;
        ::memcpy( &retval, &value, 8);
        return retval;
    }
    ByteBuffer& putDouble(double value) throw (OverflowUnderflowException) {
        uint64_t newval;
        ::memcpy(&newval, &value, 8);
        newval = htonll(newval);
        *reinterpret_cast<uint64_t*>(&m_buffer[checkGetPutIndex(8)]) = newval;
        return *this;
    }
    ByteBuffer& putDouble(int32_t index, double value) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        uint64_t newval;
        ::memcpy(&newval, &value, 8);
        newval = htonll(newval);
        *reinterpret_cast<uint64_t*>(&m_buffer[checkIndex(index, 8)]) = newval;
        return *this;
    }

    std::string getString(bool &wasNull) throw (OverflowUnderflowException) {
        int32_t length = getInt32();
        if (length == -1) {
            wasNull = true;
            return std::string();
        }
        char *data = getByReference(length);
        return std::string(data, static_cast<uint32_t>(length));
    }
    std::string getString(int32_t index, bool &wasNull) throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        int32_t length = getInt32(index);
        if (length == -1) {
            wasNull = true;
            return std::string();
        }
        char *data = getByReference(index + 4, length);
        return std::string(data, static_cast<uint32_t>(length));
    }

    bool getBytes(bool &wasNull, int32_t bufsize, uint8_t *out_value, int32_t *out_len)
    throw (OverflowUnderflowException) {
        int32_t length = getInt32();
        *out_len = length;
        if (length == -1) {
            wasNull = true;
            return true;
        }
        if (!out_value || length > bufsize)
            return false;
        char *data = getByReference(length);
        memcpy(out_value, data, length);
        return true;
    }
    bool getBytes(int32_t index, bool &wasNull, const int32_t bufsize, uint8_t *out_value, int32_t *out_len)
    throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        int32_t length = getInt32(index);
        *out_len = length;
        if (length == -1) {
            wasNull = true;
            return true;
        }
        if (!out_value || length > bufsize)
            return false;
        char *data = getByReference(index + 4, length);
        memcpy(out_value, data, length);
        return true;
    }
    ByteBuffer& putBytes(const int32_t bufsize, const uint8_t *in_value)
    throw (OverflowUnderflowException) {
        assert(in_value  || bufsize==0);
        putInt32(bufsize);
        put((const char*)in_value, bufsize);
        return *this;
    }
    ByteBuffer& putBytes(int32_t index, const int32_t bufsize, const uint8_t *in_value)
    throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        assert(in_value  || bufsize==0);
        putInt32(index, bufsize);
        put(index + 4, (const char*)in_value, bufsize);
        return *this;
    }

    ByteBuffer& putString(const std::string& value)
    throw (OverflowUnderflowException) {
        return putBytes(static_cast<int32_t>(value.size()), (const uint8_t*)value.data());
    }

    ByteBuffer& putString(int32_t index, const std::string& value)
    throw (OverflowUnderflowException, IndexOutOfBoundsException) {
        return putBytes(index, static_cast<int32_t>(value.size()), (const uint8_t*)value.data());
    }

    int32_t position() const {
        return m_position;
    }
    ByteBuffer& position(int32_t position) throw (IndexOutOfBoundsException) {
        m_position = checkIndex(position, 0);
        return *this;
    }

    int32_t remaining() const {
        return m_limit - m_position;
    }
    bool hasRemaining() const {
        return m_position < m_limit;
    }

    int32_t limit() const {
        return m_limit;
    }

    ByteBuffer& limit(int32_t newLimit) throw (IndexOutOfBoundsException) {
        if (newLimit > m_capacity || newLimit < 0) {
            throw IndexOutOfBoundsException();
        }
        m_limit = newLimit;
        return *this;
    }

    char* bytes() const {
        return m_buffer;
    }

    ByteBuffer slice() {
        ByteBuffer retval(&m_buffer[m_position], m_limit - m_position);
        m_position = m_limit;
        return retval;
    }

    virtual bool isExpandable() {
        return false;
    }

    virtual void ensureRemaining(int32_t remaining) throw(NonExpandableBufferException) {
        remaining = 0;
        throw NonExpandableBufferException();
    }
    virtual void ensureRemainingExact(int32_t remaining) throw(NonExpandableBufferException) {
        remaining = 0;
        throw NonExpandableBufferException();
    }
    virtual void ensureCapacity(int32_t capacity) throw(NonExpandableBufferException) {
        capacity = 0;
        throw NonExpandableBufferException();
    }
    virtual void ensureCapacityExact(int32_t capacity) throw(NonExpandableBufferException) {
        capacity = 0;
        throw NonExpandableBufferException();
    }

    /**
     * Create a byte buffer backed by the provided storage. Does not handle memory ownership
     */
    ByteBuffer(char *buffer, int32_t capacity) :
        m_buffer(buffer), m_position(0), m_capacity(capacity), m_limit(capacity) {
        if (buffer == NULL) {
            throw NullPointerException();
        }
    }

    ByteBuffer(const ByteBuffer &other) :
        m_buffer(other.m_buffer), m_position(other.m_position), m_capacity(other.m_capacity), m_limit(other.m_limit) {
    }

    virtual ~ByteBuffer() {}

    int32_t capacity() {
        return m_capacity;
    }

    bool operator==(const ByteBuffer& other) {
        if (this == &other) return true;
        bool eq = (m_capacity == other.m_capacity && m_limit == other.m_limit);
        if (eq) {
            return (memcmp(m_buffer, other.m_buffer, m_limit) == 0);
        }
        return false;
    }

    bool operator!=(const ByteBuffer& other) {
        if (this == &other) return false;
        bool noteq = (m_capacity != other.m_capacity || m_limit != other.m_limit);
        if (!noteq) {
            return (memcmp(m_buffer, other.m_buffer, m_limit) != 0);
        }
        return true;
    }

private:
    ByteBuffer& operator = (const ByteBuffer& other) {
        m_buffer = other.m_buffer;
        m_position = other.m_position;
        m_capacity = other.m_capacity;
        m_limit = other.m_limit;
        return *this;
    }
    char * getByReference(int32_t length) {
        return &m_buffer[checkGetPutIndex(length)];
    }
    char * getByReference(int32_t index, int32_t length) {
        return &m_buffer[checkIndex(index, length)];
    }
protected:
    ByteBuffer() : m_buffer(NULL), m_position(0), m_capacity(0), m_limit(0) {};
    char *  m_buffer;
    int32_t m_position;
    int32_t m_capacity;
    int32_t m_limit;
};

class ExpandableByteBuffer : public ByteBuffer {
public:
    void ensureRemaining(int32_t amount)  throw(NonExpandableBufferException) {
        if (remaining() < amount) {
            ensureCapacity(position() + amount);
        }
    }
    void ensureRemainingExact(int32_t amount)  throw(NonExpandableBufferException) {
        if (remaining() < amount) {
            ensureCapacityExact(position() + amount);
        }
    }
    void ensureCapacity(int32_t capacity)  throw(NonExpandableBufferException) {
        if (m_capacity < capacity) {
            int32_t newCapacity = m_capacity;
            while (newCapacity < capacity) {
                newCapacity *= 2;
            }
            char *newBuffer = new char[newCapacity];
            ::memcpy(newBuffer, m_buffer, static_cast<uint32_t>(m_position));
            m_buffer = newBuffer;
            resetRef(newBuffer);
            m_capacity = newCapacity;
            m_limit = newCapacity;
        }
    }

    void ensureCapacityExact(int32_t capacity)  throw(NonExpandableBufferException) {
        if (m_capacity < capacity) {
            char *newBuffer = new char[capacity];
            ::memcpy(newBuffer, m_buffer, static_cast<uint32_t>(m_position));
            m_buffer = newBuffer;
            resetRef(newBuffer);
            m_capacity = capacity;
            m_limit = capacity;
        }
    }

    bool isExpandable() {
        return true;
    }

    virtual ~ExpandableByteBuffer() {}
protected:
    ExpandableByteBuffer(const ExpandableByteBuffer &other) : ByteBuffer(other) {
    }
    ExpandableByteBuffer() : ByteBuffer() {}
    ExpandableByteBuffer(char *data, int32_t length) : ByteBuffer(data, length) {}
    virtual void resetRef(char *data) = 0;
private:
};

class SharedByteBuffer : public ExpandableByteBuffer {
public:
    SharedByteBuffer(const SharedByteBuffer &other) : ExpandableByteBuffer(other), m_ref(other.m_ref) {}

    SharedByteBuffer& operator = (const SharedByteBuffer& other) {
        m_ref = other.m_ref;
        m_buffer = other.m_buffer;
        m_position = other.m_position;
        m_limit = other.m_limit;
        m_capacity = other.m_capacity;
        return *this;
    }

    SharedByteBuffer() : ExpandableByteBuffer() {};

    SharedByteBuffer slice() {
        SharedByteBuffer retval(m_ref, &m_buffer[m_position], m_limit - m_position);
        m_position = m_limit;
        return retval;
    }

    SharedByteBuffer(char *data, int32_t length) : ExpandableByteBuffer(data, length), m_ref(data) {}
    SharedByteBuffer(boost::shared_array<char>& data, int32_t length) : ExpandableByteBuffer(data.get(), length), m_ref(data) {}

protected:
    void resetRef(char *data)  {
        m_ref.reset(data);
    }
private:
    SharedByteBuffer(boost::shared_array<char> ref, char *data, int32_t length) : ExpandableByteBuffer(data, length), m_ref(ref) {}
    boost::shared_array<char> m_ref;
};

class ScopedByteBuffer : public ExpandableByteBuffer {
public:
//    virtual ByteBuffer* duplicate() {
//        char *copy = new char[m_capacity];
//        ::memcpy(copy, m_buffer, m_capacity);
//        return new ScopedByteBuffer(copy, m_capacity);
//    }
    ScopedByteBuffer(int32_t capacity) : ExpandableByteBuffer(new char[capacity], capacity), m_ref(m_buffer) {

    }
    ScopedByteBuffer(char *data, int32_t length) : ExpandableByteBuffer(data, length), m_ref(data) {}

//    static void copyAndWrap(ScopedByteBuffer &out, char *source, int32_t length) {
//        assert(length > 0);
//        char *copy = new char[length];
//        ::memcpy( copy, source, length);
//
//        return out(copy, length);
//    }
protected:
    void resetRef(char *data)  {
        m_ref.reset(data);
    }
private:
    ScopedByteBuffer& operator = (const ScopedByteBuffer& other) {
        assert(other.m_buffer != NULL);
        return *this;
    }
    ScopedByteBuffer(const ScopedByteBuffer &other) : ExpandableByteBuffer(other) {
    }
    ScopedByteBuffer() : ExpandableByteBuffer(), m_ref()  {};
    boost::scoped_array<char> m_ref;
};

//
///** Abstract class for reading from memory buffers. */
//class SerializeInput {
//protected:
//    /** Does no initialization. Subclasses must call initialize. */
//    SerializeInput() : current_(NULL), end_(NULL) {}
//
//    void initialize(const void* data, size_t length) {
//        current_ = reinterpret_cast<const char*>(data);
//        end_ = current_ + length;
//    }
//
//public:
//    virtual ~SerializeInput() {};
//
//    // functions for deserialization
//    inline char readChar() {
//        return readPrimitive<char>();
//    }
//
//    inline int8_t readByte() {
//        return readPrimitive<int8_t>();
//    }
//
//    inline int16_t readShort() {
//        int16_t value = readPrimitive<int16_t>();
//        return ntohs(value);
//    }
//
//    inline int32_t readInt() {
//        int32_t value = readPrimitive<int32_t>();
//        return ntohl(value);
//    }
//
//    inline bool readBool() {
//        return readByte();
//    }
//
//    inline char readEnumInSingleByte() {
//        return readByte();
//    }
//
//    inline int64_t readLong() {
//        int64_t value = readPrimitive<int64_t>();
//        return ntohll(value);
//    }
//
//    inline float readFloat() {
//        int32_t value = readPrimitive<int32_t>();
//        value = ntohl(value);
//        float retval;
//        memcpy(&retval, &value, sizeof(retval));
//        return retval;
//    }
//
//    inline double readDouble() {
//        int64_t value = readPrimitive<int64_t>();
//        value = ntohll(value);
//        double retval;
//        memcpy(&retval, &value, sizeof(retval));
//        return retval;
//    }
//
//    /** Returns a pointer to the internal data buffer, advancing the read position by length. */
//    const void* getRawPointer(size_t length) {
//        const void* result = current_;
//        current_ += length;
//        // TODO: Make this a non-optional check?
//        assert(current_ <= end_);
//        return result;
//    }
//
//    /** Copy a string from the buffer. */
//    inline std::string readTextString() {
//        int32_t stringLength = readInt();
//        assert(stringLength >= 0);
//        return std::string(reinterpret_cast<const char*>(getRawPointer(stringLength)),
//                stringLength);
//    };
//
//    /** Copy a ByteArray from the buffer. */
//    inline ByteArray readBinaryString() {
//        int32_t stringLength = readInt();
//        assert(stringLength >= 0);
//        return ByteArray(reinterpret_cast<const char*>(getRawPointer(stringLength)),
//                stringLength);
//    };
//
//    /** Copy the next length bytes from the buffer to destination. */
//    inline void readBytes(void* destination, size_t length) {
//        ::memcpy(destination, getRawPointer(length), length);
//    };
//
//    /** Move the read position back by bytes. Warning: this method is
//    currently unverified and could result in reading before the
//    beginning of the buffer. */
//    // TODO(evanj): Change the implementation to validate this?
//    void unread(size_t bytes) {
//        current_ -= bytes;
//    }
//
//private:
//    template <typename T>
//    T readPrimitive() {
//        T value;
//        ::memcpy(&value, current_, sizeof(value));
//        current_ += sizeof(value);
//        return value;
//    }
//
//    // Current read position.
//    const char* current_;
//    // End of the buffer. Valid byte range: current_ <= validPointer < end_.
//    const char* end_;
//
//    // No implicit copies
//    SerializeInput(const SerializeInput&);
//    SerializeInput& operator=(const SerializeInput&);
//};
//
///** Abstract class for writing to memory buffers. Subclasses may optionally support resizing. */
//class SerializeOutput {
//protected:
//    SerializeOutput() : buffer_(NULL), position_(0), capacity_(0) {}
//
//    /** Set the buffer to buffer with capacity. Note this does not change the position. */
//    void initialize(void* buffer, size_t capacity) {
//        buffer_ = reinterpret_cast<char*>(buffer);
//        assert(position_ <= capacity);
//        capacity_ = capacity;
//    }
//    void setPosition(size_t position) {
//        this->position_ = position;
//    }
//public:
//    virtual ~SerializeOutput() {};
//
//    /** Returns a pointer to the beginning of the buffer, for reading the serialized data. */
//    const char* data() const { return buffer_; }
//
//    /** Returns the number of bytes written in to the buffer. */
//    size_t size() const { return position_; }
//
//    // functions for serialization
//    inline void writeChar(char value) {
//        writePrimitive(value);
//    }
//
//    inline void writeByte(int8_t value) {
//        writePrimitive(value);
//    }
//
//    inline void writeShort(int16_t value) {
//        writePrimitive(static_cast<uint16_t>(htons(value)));
//    }
//
//    inline void writeInt(int32_t value) {
//        writePrimitive(htonl(value));
//    }
//
//    inline void writeBool(bool value) {
//        writeByte(value ? int8_t(1) : int8_t(0));
//    };
//
//    inline void writeLong(int64_t value) {
//        writePrimitive(htonll(value));
//    }
//
//    inline void writeFloat(float value) {
//        int32_t data;
//        memcpy(&data, &value, sizeof(data));
//        writePrimitive(htonl(data));
//    }
//
//    inline void writeDouble(double value) {
//        int64_t data;
//        memcpy(&data, &value, sizeof(data));
//        writePrimitive(htonll(data));
//    }
//
//    inline void writeEnumInSingleByte(int value) {
//        assert(std::numeric_limits<int8_t>::min() <= value &&
//                value <= std::numeric_limits<int8_t>::max());
//        writeByte(static_cast<int8_t>(value));
//    }
//
//    inline size_t writeCharAt(size_t position, char value) {
//        return writePrimitiveAt(position, value);
//    }
//
//    inline size_t writeByteAt(size_t position, int8_t value) {
//        return writePrimitiveAt(position, value);
//    }
//
//    inline size_t writeShortAt(size_t position, int16_t value) {
//        return writePrimitiveAt(position, htons(value));
//    }
//
//    inline size_t writeIntAt(size_t position, int32_t value) {
//        return writePrimitiveAt(position, htonl(value));
//    }
//
//    inline size_t writeBoolAt(size_t position, bool value) {
//        return writePrimitiveAt(position, value ? int8_t(1) : int8_t(0));
//    }
//
//    inline size_t writeLongAt(size_t position, int64_t value) {
//        return writePrimitiveAt(position, htonll(value));
//    }
//
//    inline size_t writeFloatAt(size_t position, float value) {
//        int32_t data;
//        memcpy(&data, &value, sizeof(data));
//        return writePrimitiveAt(position, htonl(data));
//    }
//
//    inline size_t writeDoubleAt(size_t position, double value) {
//        int64_t data;
//        memcpy(&data, &value, sizeof(data));
//        return writePrimitiveAt(position, htonll(data));
//    }
//
//    // this explicitly accepts char* and length (or ByteArray)
//    // as std::string's implicit construction is unsafe!
//    inline void writeBinaryString(const void* value, size_t length) {
//        int32_t stringLength = static_cast<int32_t>(length);
//        assureExpand(length + sizeof(stringLength));
//
//        // do a newtork order conversion
//        int32_t networkOrderLen = htonl(stringLength);
//
//        char* current = buffer_ + position_;
//        memcpy(current, &networkOrderLen, sizeof(networkOrderLen));
//        current += sizeof(stringLength);
//        memcpy(current, value, length);
//        position_ += sizeof(stringLength) + length;
//    }
//
//    inline void writeBinaryString(const ByteArray &value) {
//        writeBinaryString(value.data(), value.length());
//    }
//
//    inline void writeTextString(const std::string &value) {
//        writeBinaryString(value.data(), value.size());
//    }
//
//    inline void writeBytes(const void *value, size_t length) {
//        assureExpand(length);
//        memcpy(buffer_ + position_, value, length);
//        position_ += length;
//    }
//
//    inline void writeZeros(size_t length) {
//        assureExpand(length);
//        memset(buffer_ + position_, 0, length);
//        position_ += length;
//    }
//
//    /** Reserves length bytes of space for writing. Returns the offset to the bytes. */
//    size_t reserveBytes(size_t length) {
//        assureExpand(length);
//        size_t offset = position_;
//        position_ += length;
//        return offset;
//    }
//
//    /** Copies length bytes from value to this buffer, starting at
//    offset. Offset should have been obtained from reserveBytes. This
//    does not affect the current write position.  * @return offset +
//    length */
//    inline size_t writeBytesAt(size_t offset, const void *value, size_t length) {
//        assert(offset + length <= position_);
//        memcpy(buffer_ + offset, value, length);
//        return offset + length;
//    }
//
//    static bool isLittleEndian() {
//        static const uint16_t s = 0x0001;
//        uint8_t byte;
//        memcpy(&byte, &s, 1);
//        return byte != 0;
//    }
//
//    std::size_t position() {
//        return position_;
//    }
//
//protected:
//
//    /** Called when trying to write past the end of the
//    buffer. Subclasses can optionally resize the buffer by calling
//    initialize. If this function returns and size() < minimum_desired,
//    the program will crash.  @param minimum_desired the minimum length
//    the resized buffer needs to have. */
//    virtual void expand(size_t minimum_desired) = 0;
//
//private:
//    template <typename T>
//    void writePrimitive(T value) {
//        assureExpand(sizeof(value));
//        memcpy(buffer_ + position_, &value, sizeof(value));
//        position_ += sizeof(value);
//    }
//
//    template <typename T>
//    size_t writePrimitiveAt(size_t position, T value) {
//        return writeBytesAt(position, &value, sizeof(value));
//    }
//
//    inline void assureExpand(size_t next_write) {
//        size_t minimum_desired = position_ + next_write;
//        if (minimum_desired > capacity_) {
//            expand(minimum_desired);
//        }
//        assert(capacity_ >= minimum_desired);
//    }
//
//    // Beginning of the buffer.
//    char* buffer_;
//
//    // No implicit copies
//    SerializeOutput(const SerializeOutput&);
//    SerializeOutput& operator=(const SerializeOutput&);
//
//protected:
//    // Current write position in the buffer.
//    size_t position_;
//    // Total bytes this buffer can contain.
//    size_t capacity_;
//};
//
///** Implementation of SerializeInput that references an existing buffer. */
//class ReferenceSerializeInput : public SerializeInput {
//public:
//    ReferenceSerializeInput(const void* data, size_t length) {
//        initialize(data, length);
//    }
//
//    // Destructor does nothing: nothing to clean up!
//    virtual ~ReferenceSerializeInput() {}
//};
//
///** Implementation of SerializeInput that makes a copy of the buffer. */
//class CopySerializeInput : public SerializeInput {
//public:
//    CopySerializeInput(const void* data, size_t length) :
//            bytes_(reinterpret_cast<const char*>(data), static_cast<int>(length)) {
//        initialize(bytes_.data(), static_cast<int>(length));
//    }
//
//    // Destructor frees the ByteArray.
//    virtual ~CopySerializeInput() {}
//
//private:
//    ByteArray bytes_;
//};
//
///** Implementation of SerializeOutput that references an existing buffer. */
//class ReferenceSerializeOutput : public SerializeOutput {
//public:
//    ReferenceSerializeOutput() : SerializeOutput() {
//    }
//    ReferenceSerializeOutput(void* data, size_t length) : SerializeOutput() {
//        initialize(data, length);
//    }
//
//    /** Set the buffer to buffer with capacity and sets the position. */
//    void initializeWithPosition(void* buffer, size_t capacity, size_t position) {
//        setPosition(position);
//        initialize(buffer, capacity);
//    }
//
//    size_t remaining() {
//        return capacity_ - position_;
//    }
//
//    // Destructor does nothing: nothing to clean up!
//    virtual ~ReferenceSerializeOutput() {}
//
//protected:
//    /** Reference output can't resize the buffer: Frowny-Face. */
//    virtual void expand(size_t minimum_desired) {
//        throw SQLException(SQLException::volt_output_buffer_overflow,
//            "Output from SQL stmt overflowed output/network buffer of 10mb. "
//            "Try a \"limit\" clause or a stronger predicate.");
//    }
//};
//
///** Implementation of SerializeOutput that makes a copy of the buffer. */
//class CopySerializeOutput : public SerializeOutput {
//public:
//    // Start with something sizeable so we avoid a ton of initial
//    // allocations.
//    static const int INITIAL_SIZE = 8388608;
//
//    CopySerializeOutput() : bytes_(INITIAL_SIZE) {
//        initialize(bytes_.data(), INITIAL_SIZE);
//    }
//
//    // Destructor frees the ByteArray.
//    virtual ~CopySerializeOutput() {}
//
//    void reset() {
//        setPosition(0);
//    }
//
//    int remaining() {
//        return bytes_.length() - static_cast<int>(position());
//    }
//
//protected:
//    /** Resize this buffer to contain twice the amount desired. */
//    virtual void expand(size_t minimum_desired) {
//        size_t next_capacity = (bytes_.length() + minimum_desired) * 2;
//        assert(next_capacity < static_cast<size_t>(std::numeric_limits<int>::max()));
//        bytes_.copyAndExpand(static_cast<int>(next_capacity));
//        initialize(bytes_.data(), next_capacity);
//    }
//
//private:
//    ByteArray bytes_;
//};
//


}
#endif
