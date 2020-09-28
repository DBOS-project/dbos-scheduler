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

#ifndef VOLTDB_PROCEDURECALLBACK_HPP_
#define VOLTDB_PROCEDURECALLBACK_HPP_
#include "InvocationResponse.hpp"
#include <boost/shared_ptr.hpp>
namespace voltdb {

/*
 * Abstract base class for callbacks to provide to the
 * API with procedure invocations
 */
class ProcedureCallback {
public:

    enum AbandonReason { NOT_ABANDONED, TOO_BUSY };

    /*
     * Invoked when a response to an invocation is available or
     * the connection to the node the invocation was sent to was lost.
     * Callbacks should not throw user exceptions.
     *
     * The API surrounds the callback in a try catch for std::exception
     * and passes any caught exceptions to the status listener.
     * @return true if the event loop should break after invoking this callback, false otherwise
     */
    virtual bool callback(InvocationResponse response) throw (voltdb::Exception) = 0;
    virtual void abandon(AbandonReason reason) {}
    // Mechanism for procedure to over-ride abandon property set in client in event of backpressure.
    // @return true: allow abandoning of requests in case of back pressure
    //         false: don't abandon the requests in back pressure scenario.
    virtual bool allowAbandon() const {return true;}
    virtual ~ProcedureCallback() {}
};
}

#endif /* VOLTDB_PROCEDURECALLBACK_HPP_ */
