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

#ifndef VOLTDB_CLIENTCONFIG_H_
#define VOLTDB_CLIENTCONFIG_H_
#include <string>
#include "StatusListener.h"
#include <boost/shared_ptr.hpp>

namespace voltdb {

enum ClientAuthHashScheme { HASH_SHA1, HASH_SHA256 };

class ClientConfig {
public:
    ClientConfig(
            std::string username = std::string(""),
            std::string password = std::string(""),
            ClientAuthHashScheme scheme = HASH_SHA1, bool enableAbandon = false,
            bool enableQueryTimeout = false, int timeoutInSec =  DEFAULT_QUERY_TIMEOUT_SEC,
            bool useSSL = false);

    ClientConfig(
            std::string username,
            std::string password,
            boost::shared_ptr<StatusListener> listener,
            ClientAuthHashScheme scheme = HASH_SHA1, bool enableAbandon = false,
            bool enableQueryTimeout = false, int timeoutInSec = DEFAULT_QUERY_TIMEOUT_SEC,
            bool useSSL = false);

    ClientConfig(
            std::string username,
            std::string password,
            StatusListener *listener,
            ClientAuthHashScheme scheme = HASH_SHA1, bool enableAbandon = false,
            bool enableQueryTimeout = false, int timeoutInSec = DEFAULT_QUERY_TIMEOUT_SEC,
            bool useSSL = false);
    std::string m_username;
    std::string m_password;
    boost::shared_ptr<StatusListener> m_listener;
    int32_t m_maxOutstandingRequests;
    ClientAuthHashScheme m_hashScheme;
    bool m_enableAbandon;
    bool m_enableQueryTimeout;
    timeval m_queryTimeout;
    timeval m_scanIntervalForTimedoutQuery;
    bool m_useSSL;
private:
    static const int8_t DEFAULT_QUERY_TIMEOUT_SEC = 10;
    static const int8_t DEFAULT_SCAN_INTERVAL_FOR_EXPIRED_REQUESTS_SEC = 2;
};
}

#endif /* VOLTDB_CLIENTCONFIG_H_ */
