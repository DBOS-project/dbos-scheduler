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

#ifndef VOLT_STATUSLISTENER_H_
#define VOLT_STATUSLISTENER_H_
#include <exception>
#include "InvocationResponse.hpp"
namespace voltdb {
class ProcedureCallback;
/*
 * A status listener that an application should provide to a Client instance so
 * that the application can be notified of important events.
 */
class StatusListener {
public:
    /*
     * Handle exceptions thrown by a Client's callback. All exceptions (std::exception) are caught
     * but a client should only throw exceptions that are instances or subclasses of
     * voltdb::ClientCallbackException.
     * @param exception The exception thrown by the callback
     * @param callback Instance of the callback that threw the exception
     * @param The response from the server that caused the exception
     * @return true if the event loop should terminate and false otherwise
     */
    virtual bool uncaughtException(
            std::exception exception,
            boost::shared_ptr<voltdb::ProcedureCallback> callback,
            voltdb::InvocationResponse response) = 0;

    /*
     * Notify the client application that a connection to the database was lost
     * @param hostname Name of the host that the connection was lost to
     * @param connectionsLeft Number of connections to the database remaining
     * @return true if the event loop should terminate and false otherwise
     */
    virtual bool connectionLost(std::string hostname, int32_t connectionsLeft) = 0;

    /*
     * Notify the client application that a connection to the database was established
     * @param hostname Name of the host that the connection was established to
     * @param connectionsActive Number of connections to the database remaining
     * @return true if the event loop should terminate and false otherwise
     */
    virtual bool connectionActive(std::string hostname, int32_t connectionsActive) = 0;

    /*
     * Notify the client application that backpressure occured
     * @param hasBackpressure True if backpressure is beginning and false if it is ending
     * @return true, in backpressure environment, if the client library to queue the invocation and
     *              return from invoke()
     *         false, in backpressure environment, if the library should wait until there is a
     *               connection without backpressure and then queue it.
     */
    virtual bool backpressure(bool hasBackpressure) = 0;

    virtual ~StatusListener() {}
};
}

#endif /* STATUSLISTENER_H_ */
