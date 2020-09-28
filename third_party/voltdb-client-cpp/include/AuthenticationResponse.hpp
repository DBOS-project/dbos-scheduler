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

#ifndef VOLTDB_AUTHENTICATIONRESPONSE_HPP_
#define VOLTDB_AUTHENTICATIONRESPONSE_HPP_
#include "ByteBuffer.hpp"
namespace voltdb {
class AuthenticationResponse {
public:
    AuthenticationResponse() {}
    AuthenticationResponse(ByteBuffer &buf) {
        char version = buf.getInt8();
        assert(version == 0);
        m_resultCode = buf.getInt8();
        if (m_resultCode != 0) {
            return;
        }
        m_hostId = buf.getInt32();
        m_connectionId = buf.getInt64();
        m_clusterStartTime = buf.getInt64();
        m_leaderAddress = buf.getInt32();
        bool wasNull = false;
        m_buildString = buf.getString(wasNull);
        assert(!wasNull);
    }

    bool success() {
        return m_resultCode == 0;
    }

    int32_t getHostId() const { return m_hostId; }
    int32_t getConnectionId() const { return m_connectionId; }
    int64_t getClusterStartTime() const { return m_clusterStartTime; }
    int32_t getLeaderAddress() const { return m_leaderAddress; }
    std::string getBuildString() const { return m_buildString; }

private:
    int8_t m_resultCode;
    int32_t m_hostId;
    int64_t m_connectionId;
    int64_t m_clusterStartTime;
    int32_t m_leaderAddress;
    std::string m_buildString;
};
}

#endif /* VOLTDB_AUTHENTICATIONRESPONSE_HPP_ */
