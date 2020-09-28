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

#ifndef VOLT_DECIMAL_HPP_
#define VOLT_DECIMAL_HPP_
#include <string>
/*
 * ASM doesn't compile on 32-bit Ubuntu 10.04
 */
#if !defined _M_X64 && !defined __x86_64__
#define TTMATH_NOASM
#endif
#include "ttmath/ttmathint.h"
#include "Exception.hpp"
#include <sstream>

namespace voltdb {
//The int used for storage and return values
#ifdef TTMATH_PLATFORM64
typedef ttmath::Int<2> TTInt;
#else
typedef ttmath::Int<4> TTInt;
#endif

/*
 * A class for constructing Decimal values with the fixed precision and scale supported by VoltDB
 * from the wire representation and from string representations. It is expected that users will
 * want to use their own precision math library to handle the values and most libraries accept
 * data to/from strings
 */
class Decimal {
public:

    Decimal() {}

    Decimal(const TTInt& ttInt) {
        *reinterpret_cast<TTInt*>(m_data) = ttInt;
    }

    /*
     * Construct a decimal value from a string.
     */
    Decimal(const std::string& txt) {
        const TTInt kMaxScaleFactor("1000000000000");
        if (txt.length() == 0) {
            throw StringToDecimalException();
        }
        bool setSign = false;
        if (txt[0] == '-') {
            setSign = true;
        }

        /**
         * Check for invalid characters
         */
        for (size_t ii = (setSign ? 1 : 0); ii < txt.size(); ii++) {
            if ((txt[ii] < '0' || txt[ii] > '9') && txt[ii] != '.') {
                throw StringToDecimalException();
            }
        }

        std::size_t separatorPos = txt.find( '.', 0);
        if (separatorPos == std::string::npos) {
            const std::string wholeString = txt.substr( setSign ? 1 : 0, txt.size());
            const std::size_t wholeStringSize = wholeString.size();
            if (wholeStringSize > 26) {
                throw StringToDecimalException();
            }
            TTInt whole(wholeString);
            if (setSign) {
                whole.SetSign();
            }
            whole *= kMaxScaleFactor;
            getDecimal() = whole;
            return;
        }

        if (txt.find( '.', separatorPos + 1) != std::string::npos) {
            throw StringToDecimalException();;
        }

        const std::string wholeString = txt.substr( setSign ? 1 : 0, separatorPos - (setSign ? 1 : 0));
        const std::size_t wholeStringSize = wholeString.size();
        if (wholeStringSize > 26) {
            throw StringToDecimalException();;
        }
        TTInt whole(wholeString);
        std::string fractionalString = txt.substr( separatorPos + 1, txt.size() - (separatorPos + 1));
        if (fractionalString.size() > 12) {
            throw StringToDecimalException();;
        }
        while(fractionalString.size() < kMaxDecScale) {
            fractionalString.push_back('0');
        }
        TTInt fractional(fractionalString);

        whole *= kMaxScaleFactor;
        whole += fractional;

        if (setSign) {
            whole.SetSign();
        }

        getDecimal() = whole;
    }

    /*
     * Construct a Decimal value from the VoltDB wire representation
     */
    Decimal(char data[16]) {
        ByteBuffer buf(data, 16);
        int64_t *longStorage = reinterpret_cast<int64_t*>(m_data);
        //Reverse order for Java BigDecimal BigEndian
        longStorage[1] = buf.getInt64();
        longStorage[0] = buf.getInt64();
    }

    /*
     * Retrieve a decimal value as an TTInt with a fixed scale and precision
     */
    TTInt& getDecimal() {
        void *retval = reinterpret_cast<void*>(m_data);
        return *reinterpret_cast<TTInt*>(retval);
    }

    /*
     * Retrieve a decimal value as an TTInt with a fixed scale and precision
     */
    const TTInt& getDecimal() const {
        const void *retval = reinterpret_cast<const void*>(m_data);
        return *reinterpret_cast<const TTInt*>(retval);
    }

    /*
     * Serialize a Decimal value to the provided ByteBuffer
     */
#ifdef SWIG
%ignore serializeTo;
#endif
    void serializeTo(ByteBuffer *buffer) const {
        TTInt val = getDecimal();
        buffer->putInt64(*reinterpret_cast<int64_t*>(&val.table[1]));
        buffer->putInt64(*reinterpret_cast<int64_t*>(&val.table[0]));
    }

    /*
     * Convert a Decimal value to a string representation
     */
    std::string toString() {
        const TTInt kMaxScaleFactor("1000000000000");
        assert(!isNull());
        std::ostringstream buffer;
        TTInt scaledValue = getDecimal();
        if (scaledValue.IsSign()) {
            buffer << '-';
        }
        TTInt whole(scaledValue);
        TTInt fractional(scaledValue);
        whole /= kMaxScaleFactor;
        fractional %= kMaxScaleFactor;
        if (whole.IsSign()) {
            whole.ChangeSign();
        }
        buffer << whole.ToString(10);
        buffer << '.';
        if (fractional.IsSign()) {
            fractional.ChangeSign();
        }
        std::string fractionalString = fractional.ToString(10);
        for (int ii = static_cast<int>(fractionalString.size()); ii < kMaxDecScale; ii++) {
            buffer << '0';
        }
        buffer << fractionalString;
        return buffer.str();
    }

    /*
     * Returns true if the Decimal value represents SQL NULL and false otherwise.
     */
    bool isNull() {
        TTInt min;
        min.SetMin();
        return getDecimal() == min;
    }

    bool operator==(const Decimal &other) {
        return getDecimal() == other.getDecimal();
    }

    bool operator!=(const Decimal &other) {
        return !operator==(other);
    }

private:

    // Constants for Decimal type
    // Precision and scale (inherent in the schema)
    static const uint16_t kMaxDecPrec = 38;
    static const uint16_t kMaxDecScale = 12;

    char m_data[16];
};
}
#endif /* VOLT_DECIMAL_HPP_ */
