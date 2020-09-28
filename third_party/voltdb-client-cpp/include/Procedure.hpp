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

#ifndef VOLTDB_PROCEDURE_HPP_
#define VOLTDB_PROCEDURE_HPP_
#include <vector>
#include <string>
#include "ParameterSet.hpp"
#include "ByteBuffer.hpp"

namespace voltdb {

/*
 * Description of a stored procedure and its parameters that must be provided to the API
 * in order to invoke a stored procedure
 */
class Procedure {
public:
    /*
     * Construct a Procedure with the specified name and specified signature (parameters)
     */
    Procedure(const std::string& name, std::vector<Parameter> parameters) : m_name(name), m_params(parameters) {}
    Procedure(const std::string& name) : m_name(name) {}

    /**
     * Retrieve the parameter set associated with the procedure so that the parameters can be set
     * with actual values. The pointer to the parameter set can be retained
     * as long as the Procedure itself is still valid, and can be reused after each invocation.
     */
    ParameterSet* params() {
        m_params.reset();
        return &m_params;
    }

    int32_t getSerializedSize() {
        return
            5                                      // length prefix and wire protocol version
            + 4                                    // proc size
            + static_cast<int32_t>(m_name.size())  // proc name
            + 8                                    // client data
            + m_params.getSerializedSize()         // parameters
            ;
    }

    const std::string& getName()const {return m_name;}

#ifdef SWIG
%ignore serializeTo;
#endif
    void serializeTo(ByteBuffer *buffer, int64_t clientData) {
        buffer->position(4);
        buffer->putInt8(0);
        buffer->putString(m_name);
        buffer->putInt64(clientData);
        m_params.serializeTo(buffer);
        buffer->flip();
        buffer->putInt32( 0, buffer->limit() - 4);
    }
private:
    const std::string m_name;
    ParameterSet m_params;
};

}

#endif /* VOLTDB_PROCEDURE_HPP_ */
