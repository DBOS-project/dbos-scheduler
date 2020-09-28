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

#ifndef VOLTDB_PARAMETER_HPP_
#define VOLTDB_PARAMETER_HPP_
#include "WireType.h"
namespace voltdb {
/*
 * Class describing a single parameter specifying its type and whether it is an array
 * of values
 */
class Parameter {
public:
    /*
     * Default construct for compatablity with std::vector
     */
    Parameter() : m_type(WIRE_TYPE_INVALID), m_array(false) {}

    /*
     * Constructor for setting both type and arrayedness
     */
    Parameter(WireType type, bool array) : m_type(type), m_array(array) {}

    /*
     * Constructor for setting just the type. Assumes the value is not an array
     */
    Parameter(WireType type) : m_type(type), m_array(false) {}

    /*
     * Copy constructor for compatablity with std::vector
     */
    Parameter(const Parameter &other) : m_type(other.m_type), m_array(other.m_array) {}

    WireType m_type;
    bool m_array;
};
}
#endif /* VOLTDB_PARAMETER_HPP_ */
