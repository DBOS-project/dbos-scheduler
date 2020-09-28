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

#ifndef VOLTDB_COLUMN_HPP_
#define VOLTDB_COLUMN_HPP_
#include "Column.hpp"
#include "WireType.h"
#include <string>
namespace voltdb {
class Column {
public:
    Column() : m_name(""), m_type(WIRE_TYPE_INVALID) {}
    Column(std::string name, WireType type) : m_name(name), m_type(type) {}
    Column(WireType type) : m_name(""), m_type(type) {}
    std::string m_name;
    WireType m_type;

    const std::string& name() const {
        return m_name;
    }

    WireType type() const {
        return m_type;
    }

    bool operator==(const Column& rhs) const {
        if (this == &rhs) return true;
        return (m_type == rhs.m_type && m_name == rhs.m_name);
    }

    bool operator!=(const Column& rhs) const {
        if (this == &rhs) return false;
        return (m_type != rhs.m_type || m_name != rhs.m_name);
    }

};
}
#endif /* COLUMN_HPP_ */
