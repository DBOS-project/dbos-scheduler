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

#ifndef VOLT_CONNECTION_POOL_H_
#define VOLT_CONNECTION_POOL_H_
#include "Client.h"
#include "StatusListener.h"
#include <pthread.h>
#include <map>
#include <string>

namespace voltdb {

class ConnectionPool;
class ClientStuff;
typedef std::vector<boost::shared_ptr<ClientStuff> > ClientSet;
typedef std::map<std::string, std::vector<boost::shared_ptr<ClientStuff> > > ClientMap;

void cleanupOnScriptEnd(ClientSet *clients);

/*
 * A VoltDB connection pool. Geared towards invocation from scripting languages
 * where a script will run, acquire several client instances, and then terminate.
 */
class ConnectionPool {
    friend void cleanupOnScriptEnd(void *);
public:
    ConnectionPool();
    virtual ~ConnectionPool();

    /*
     * Retrieve a client that connects to the specified hostname and port using the provided username and password
     */
    voltdb::Client acquireClient(std::string hostname, std::string username, std::string password, unsigned short port = 21212, ClientAuthHashScheme sha = HASH_SHA1) throw (voltdb::ConnectException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Retrieve a client that connects to the specified hostname and port using the provided username and password
     */
    voltdb::Client acquireClient(std::string hostname, std::string username, std::string password, StatusListener *listener, unsigned short port = 21212, ClientAuthHashScheme sha = HASH_SHA1) throw (voltdb::ConnectException, voltdb::LibEventException, voltdb::Exception);

    void returnClient(Client client) throw (voltdb::Exception);

    void closeClientConnection(Client client) throw (voltdb::Exception);

    /*
     * Return the number of clients held by this thread
     */
    int numClientsBorrowed();

    /*
     * Release any unreleased clients associated with this thread/script
     */
    void onScriptEnd();

    /*
     * Retrieve the global connection pool
     */
    static voltdb::ConnectionPool* pool();

private:
    /**
     * Thread local key for storing clients
     */
    pthread_key_t m_borrowedClients;
    ClientMap m_clients;
    pthread_mutex_t m_lock;
};

/**
 * Instantiate the global connection pool
 * Should be invoked once at startup
 */
void onLoad();

/**
 * Free the global connection pool
 * Should be invoked before shutdown to ensure all txns are flushed
 * and things run valgrind clean
 */
void onUnload();

/**
 * Releases all clients that have been borrowed from the pool by the current thread
 */
void onScriptEnd();

}
#endif /* VOLT_CONNECTION_POOL_H_ */
