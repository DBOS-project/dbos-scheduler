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

#ifndef VOLTDB_AUTHENTICATIONMESSAGE_HPP_
#define VOLTDB_AUTHENTICATIONMESSAGE_HPP_
#include "ByteBuffer.hpp"

namespace voltdb {
    class AuthenticationRequest {
    public:
        AuthenticationRequest(const std::string &username,
                              const std::string &service,
                              unsigned char* passwordHash,
                              ClientAuthHashScheme hashScheme) : m_username(username),
                                                                 m_service(service),
                                                                 m_passwordHash(passwordHash),
                                                                 m_hashScheme(hashScheme)  {
        }

        int32_t getSerializedSize() {
            int sz = 8 //String length prefixes
                    + (m_hashScheme == HASH_SHA256 ? 32 : 20) //SHA-1 hash of PW
                    + static_cast<int32_t> (m_username.size())
                    + static_cast<int32_t> (m_service.size())
                    + 4 //length prefix
                    + 1 //version number
                    + 1; //scheme
            return sz;
        }

        void serializeTo(ByteBuffer *buffer) {
            buffer->position(4);
            buffer->putInt8(1); // Always version 1.
            buffer->putInt8(m_hashScheme); //scheme.
            buffer->putString(m_service);
            buffer->putString(m_username);
            buffer->put(reinterpret_cast<char*> (m_passwordHash), (m_hashScheme == HASH_SHA1 ? 20 : 32));
            buffer->flip();
            buffer->putInt32(0, buffer->limit() - 4);
        }
    private:
        const std::string m_username;
        const std::string m_service;
        unsigned char* m_passwordHash;
        ClientAuthHashScheme m_hashScheme;
    };
}

#endif /* VOLTDB_AUTHENTICATIONMESSAGE_HPP_ */
