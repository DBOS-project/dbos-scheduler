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

#ifndef VOLTDB_CLIENT_H_
#define VOLTDB_CLIENT_H_

#include <string>
#include "Procedure.hpp"
#include "StatusListener.h"
#include "ClientLogger.h"
#include <boost/shared_ptr.hpp>
#include "ClientConfig.h"
#include "Exception.hpp"

namespace voltdb {
class MockVoltDB;
class ClientImpl;
class ProcedureCallback;
/*
 * A VoltDB client for invoking stored procedures on a VoltDB instance. The client and the
 * shared pointers it returns are not thread safe. If you need more parallelism you run multiple processes
 * and connect each process to a subset of the nodes in your VoltDB cluster.
 *
 * Because the client is single threaded it has no dedicated thread for doing network IO and invoking
 * callbacks. Applications must call run() and runOnce() periodically to ensure that requests are sent
 * and responses are processed.
 */
class Client {
    friend class MockVoltDB;
public:
    /*
     * Create a connection to the VoltDB process running at the specified host authenticating
     * using the username and password provided when this client was constructed
     * @param hostname Hostname or IP address to connect to
     * @throws voltdb::ConnectException An error occurs connecting or authenticating
     * @throws voltdb::LibEventException libevent returns an error code
     */
    void createConnection(const std::string &hostname, const unsigned short port = 21212, const bool keepConnecting = false) throw (voltdb::ConnectException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Close client connection.
     */
    void close();

    /*
     * Synchronously invoke a stored procedure and return a the response. Callbacks for asynchronous requests
     * submitted earlier may be invoked before this method returns.
     * @throws NoConnectionsException No connections to submit the request on
     * @throws UninitializedParamsException Some or all of the parameters for the stored procedure were not set
     * @throws LibEventException An unknown error occured in libevent
     */
    voltdb::InvocationResponse invoke(voltdb::Procedure &proc) throw (voltdb::NoConnectionsException, voltdb::UninitializedParamsException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Asynchronously invoke a stored procedure. Returns immediately if there is no backpressure, but if there is
     * backpressure this method will block until there is none. If a status listener is registered it will notified
     * of the backpressure and will have an opportunity to prevent this method from blocking. Callbacks
     * for asynchronous requests will not be invoked until run() or runOnce() is invoked.
     * @throws NoConnectionsException No connections to submit the request on
     * @throws UninitializedParamsException Some or all of the parameters for the stored procedure were not set
     * @throws LibEventException An unknown error occured in libevent
     */
#ifdef SWIG
%ignore invoke(voltdb::Procedure &proc, boost::shared_ptr<voltdb::ProcedureCallback> callback);
#endif
    void invoke(voltdb::Procedure &proc, boost::shared_ptr<voltdb::ProcedureCallback> callback) throw (voltdb::NoConnectionsException, voltdb::UninitializedParamsException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Asynchronously invoke a stored procedure. Returns immediately if there is no backpressure, but if there is
     * backpressure this method will block until there is none. If a status listener is registered it will notified
     * of the backpressure and will have an opportunity to prevent this method from blocking. Callbacks
     * for asynchronous requests will not be invoked until run() or runOnce() is invoked.
     * @throws NoConnectionsException No connections to submit the request on
     * @throws UninitializedParamsException Some or all of the parameters for the stored procedure were not set
     * @throws LibEventException An unknown error occured in libevent
     */
    void invoke(voltdb::Procedure &proc, voltdb::ProcedureCallback *callback) throw (voltdb::NoConnectionsException, voltdb::UninitializedParamsException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Run the event loop once and process pending events. This writes requests to any ready connections
     * and reads all responses and invokes the appropriate callbacks. Returns immediately after performing
     * all available work, or after the loop is broken by a callback.
     * @throws NoConnectionsException No connections to the database so there is no work to be done
     * @throws LibEventException An unknown error occured in libevent
     */
    void runOnce() throw (voltdb::NoConnectionsException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Enter the event loop and process pending events indefinitely. This writes requests to any ready connections
     * and reads all responses and invokes the appropriate callbacks. Returns immediately after performing
     * all available work, or after the loop is broken by a callback.
     * @throws NoConnectionsException No connections to the database so there is no work to be done
     * @throws LibEventException An unknown error occured in libevent
     */
    void run() throw (voltdb::NoConnectionsException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Enter the event loop and process pending events until the specified time in microseconds has expired.
     * This writes requests to any ready connections and reads all responses and invokes the appropriate callbacks.
     * Returns only immediately after the loop is broken by a callback.
     * @throws NoConnectionsException No connections to the database so there is no work to be done
     * @throws LibEventException An unknown error occured in libevent
     */
    void runForMaxTime(uint64_t microseconds) throw (voltdb::NoConnectionsException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Enter the event loop and process pending events until all responses have been received and then return.
     * It is possible for drain to exit without having received all responses if a callback requests that the event
     * loop break in which case false will be returned.
     * @throws NoConnectionsException No connections to the database so there is no work to be done
     * @throws LibEventException An unknown error occured in libevent
     * @return true if all requests were drained and false otherwise
     */
    bool drain() throw (voltdb::NoConnectionsException, voltdb::LibEventException, voltdb::Exception);

    /*
     * Returns true if this client is draining; i.e., still processing
     * requests after receiving a call to drain().
     */
    bool isDraining() const;

    /*
     * If one of the run family of methods is running on another thread, this
     * method will instruct it to exit as soon as it finishes it's current
     * immediate task. If the thread in the run method is blocked/idle, then
     * it will return immediately.
     */
    void interrupt();

    /*
     * If one of the run family of methods is running on another thread, this
     * method will instruct it to exit as soon as it finishes it's current
     * immediate task. If the thread in the run method is blocked/idle, then
     * it will return immediately.
     * The difference from the interrupt is it stops only currently running loop,
     * and has no effect if the loop isn,t running
     */
     void wakeup();

    /*
     * Return true if the two Clients being compared points to the same ClientImpl
     */
    bool operator==(const Client &other);

    /*
     * Create a client with the specified configuration
     */
    static Client create(voltdb::ClientConfig config = voltdb::ClientConfig()) throw(voltdb::LibEventException, voltdb::Exception);

    /*
     * API to be called to enable client affinity (transaction homing)
     */
    void setClientAffinity(bool enable);
    bool getClientAffinity() const;

    /*
     * API to set Logger callback. Must be called while Client is not running
     */
    void setLoggerCallback(ClientLogger *pLogger);

    /*
     * Returns the number of outstanding/pending requests
     */
    int32_t outstandingRequests() const;
    /*
     * Returns number of procedure call requests that were timedout
     * Applicable only when query timeout feature is enabled
     */
    int64_t getExpiredRequestsCount() const;

    ~Client();
private:

    /*
     * Disable various constructors and assignment
     */
    Client() throw(voltdb::LibEventException);

    //Actual constructor
    Client(ClientImpl *m_impl);

    boost::shared_ptr<ClientImpl> m_impl;
};

}

#endif /* VOLTDB_CLIENT_H_ */
