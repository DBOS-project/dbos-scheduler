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

#ifndef VOLTDB_EXCEPTION_HPP_
#define VOLTDB_EXCEPTION_HPP_
#include <cstdio>
#include <exception>
#include "WireType.h"

namespace voltdb {

/*
 * Base class for all exceptions thrown by the VoltDB client API
 */
class Exception : public std::exception {
public:
    virtual ~Exception() throw() {}
    virtual const char* what() const throw() {
        return "An unknown error occured in the VoltDB client API";
    }
};

class NullPointerException : public Exception {
public:
    NullPointerException() :
        Exception() {}
    virtual const char* what() const throw() {
        return "Found a null pointer where an address was expected";
    }
};

/*
 * Thrown when attempting to retrieve a column from a Row using a column
 * index that is < 0, > num columns, or when using a getter with an inappropriate data type
 */
class InvalidColumnException : public Exception {
    std::string m_what;
public:
    explicit InvalidColumnException() :
        Exception() {
        m_what = "Attempted to retrieve a column with an invalid index or name, or an invalid type for the specified column";
    }

    explicit InvalidColumnException(const size_t index, const size_t numColumns) :
        Exception() {
        char msg[256];
        snprintf(msg, sizeof msg, "Attempted to retrieve a column with an invalid index: %ld; valid high index: %ld", index, numColumns-1);
        m_what = msg;
    }

    explicit InvalidColumnException(const std::string& name) :
        Exception() {
        m_what = "Attempted to retrieve a column with an invalid name: " + name;
    }

    explicit InvalidColumnException(const std::string& columnName, const size_t type, const std::string& typeName, const std::string& expectedTypeName) :
        Exception() {
        char msg[256];
        snprintf(msg, sizeof msg, "Attempted to retrieve a column: %s with an invalid type: %s<%ld> expected: %s", columnName.c_str(), typeName.c_str(), type, expectedTypeName.c_str());
        m_what = msg;
    }

    virtual ~InvalidColumnException() throw() {}

    virtual const char* what() const throw() {
        return m_what.c_str();
    }
};

class RowCreationException : public Exception {
private:
    std::string m_what;
    explicit RowCreationException():Exception() {}
public:

    RowCreationException(const std::string &msg) {
        m_what = "Failed to create row. " + msg;
    }

    const char* what() const throw() {
        return m_what.c_str();
    }

    ~RowCreationException() throw() {}
};

class TableException : public Exception {
private:
    std::string m_what;
    explicit TableException() {}
public:


    explicit TableException(const std::string& msg) : m_what(msg){ }

    const char* what() const throw() {
        return m_what.c_str();
    }

    ~TableException() throw() {}
};
/*
 * Thrown by ByteBuffer when an attempt is made to get or put data beyond the limit or capacity of the ByteBuffer
 * Users should never see this error since they don't access ByteBuffers directly. They access them
 * through message wrappers like AuthenticationRequest, AuthenticationResponse, InvocationResponse, Table,
 * TableIterator, and Row.
 */
class OverflowUnderflowException : public Exception {
public:
    OverflowUnderflowException() : Exception() {}
    virtual const char* what() const throw() {
        return "Overflow underflow exception";
    }
};

/*
 * Thrown by ByteBuffer when an attempt is made to access a value at a specific index or set the position/limit
 * to a specific index that is < 0 or > the limit/capacity. This should never be seen by users.
 */
class IndexOutOfBoundsException : public Exception {
public:
    IndexOutOfBoundsException() : Exception() {}
    virtual const char* what() const throw() {
        return "Index out of bounds exception";
    }
};

/*
 * Thrown by ByteBuffer when an attempt is made to expand a buffer that is not expandable. Should
 * never be seen by users.
 */
class NonExpandableBufferException : public Exception {
public:
    NonExpandableBufferException() : Exception() {}
    virtual const char* what() const throw() {
        return "Attempted to add/expand a nonexpandable buffer";
    }
};

/*
 * Thrown when an attempt is made to invoke a procedure without initializing all the parameters to the procedure.
 * Users may see this exception.
 */
class UninitializedParamsException : public Exception {
public:
    UninitializedParamsException() : Exception() {}
    virtual const char* what() const throw() {
        return "Not all parameters were set";
    }
};

/*
 * Thrown when an attempt is made to add a parameter to a parameter set that is the wrong type for the argument
 * or an extra argument. Users may see this exception.
 */
class ParamMismatchException : public Exception {
    std::string m_what;
public:
    explicit ParamMismatchException() : Exception() {
        m_what = "Attempted to set a parameter using the wrong type";
    }
    explicit ParamMismatchException(const size_t type, const std::string& typeName) :
        Exception() {
        char msg[256];
        snprintf(msg, sizeof msg, "Attempted to set a parameter using the wrong type: %s<%ld>", typeName.c_str(), type);
        m_what = msg;
    }
    virtual ~ParamMismatchException() throw() {}
    virtual const char* what() const throw() {
        return m_what.c_str();
    }
};

/*
 * Thrown when a user attempts to use a type that is not supported.
 * Users may see this exception.
 */
class UnsupportedTypeException : public Exception {
public:

    explicit UnsupportedTypeException(const std::string& typeName) :
        Exception() {
        char msg[256];
        snprintf(msg, sizeof msg, "Attempted to use a SQL type that is unsupported in the C++ client: %s", typeName.c_str());
        m_what = msg;
    }

    virtual ~UnsupportedTypeException() throw() {}

    virtual const char* what() const throw() {
        return m_what.c_str();
    }

private:

    std::string m_what;
};

/*
 * Thrown by Distributer when detects server run in LEGACY mode
 */
class ElasticModeMismatchException : public Exception {
public:
    ElasticModeMismatchException() : Exception() {}
   virtual const char* what() const throw() {
       return "LEGACY mode is not supported";
   }
};

/*
 * Thrown by TableIterator when next() is invoked when there are no more rows to return. Users may see this exceptino.
 */
class NoMoreRowsException : public Exception {
public:
    NoMoreRowsException() : Exception() {}
    virtual const char* what() const throw() {
        return "Requests another row when there are no more";
    }
};

/*
 * Thrown by the Decimal string constructor when an attempt is made to construct a Decimal with
 * an invalid string. Users may see this exception.
 */
class StringToDecimalException : public Exception {
public:
    StringToDecimalException() : Exception() {}
    virtual const char* what() const throw() {
        return "Parse error constructing decimal from string";
    }
};

/*
 * Thrown when an application specific error occurs during the connection process.
 * e.g. Authentication fails while attempting to connect to a node
 */
class ConnectException : public Exception {
    std::string m_what;
public:
    explicit ConnectException() : Exception() {
        m_what = "An error occurred while attempting to create and authenticate a connection to VoltDB";
    }
    explicit ConnectException(const std::string &hostname, unsigned short port) {
        char msg[1024];
        snprintf(msg, sizeof msg,
                "An error occurred while attempting to create and authenticate a connection to : %s:%d",
                hostname.c_str(), (unsigned int)port);
        m_what = msg;
    }

    const char* what() const throw() {
        return m_what.c_str();
    }

    virtual ~ConnectException() throw () {}
};

/*
 * Thrown when an attempt is made to invoke a procedure or run the event loop when
 * there are no connections to VoltDB.
 */
class NoConnectionsException : public Exception {
public:
    NoConnectionsException() : Exception() {}
    virtual const char* what() const throw() {
        return "Attempted to invoke a procedure while there are no connections";
    }
};

/*
 * Thrown when an attempt is made to return a client that does not belong to this thread
 * back to connection pool.
 */
class MisplacedClientException : public Exception {
public:
    MisplacedClientException() : Exception() {}
    virtual const char* what() const throw() {
        return "Attempted to return a client that does not belong to this thread";
    }
};

/*
 * Thrown when libevent returns a failure code. These codes are not specific so it isn't possible
 * to tell what happened.
 */
class LibEventException : public Exception {
    std::string m_what;
public:
    explicit LibEventException() : Exception() {
        m_what = "Lib event generated an unexpected error";
    }
    explicit LibEventException(const std::string& msg) : Exception() {
        m_what = "Encountered an error from libevent library. " + msg;
    }
    virtual ~LibEventException() throw() {}
    const char* what() const throw() {
        return m_what.c_str();
    }
};

class ClusterInstanceMismatchException : public Exception {
public:
    ClusterInstanceMismatchException() : Exception() {}
    virtual const char* what() const throw() {
        return "Attempted to connect a client to two separate VoltDB clusters";
    }
};

class UnknownProcedureException : public Exception {
    std::string m_what;
public:
    explicit UnknownProcedureException() : Exception() {
        m_what = "Unknown procedure invoked";
    }
    explicit UnknownProcedureException(const std::string& name) : Exception() {
        m_what = "Unknown procedure invoked: " + name;
    }
    virtual ~UnknownProcedureException() throw() {}
    virtual const char* what() const throw() {
        return m_what.c_str();
    }
};

class CoordinateOutOfRangeException : public Exception {
    std::string m_what;
public:
    explicit CoordinateOutOfRangeException() : Exception() {
        m_what = "Coordinate out of Range";
    }
    explicit CoordinateOutOfRangeException(const std::string& name) : Exception() {
        m_what = name + " coordinate out of range.";
    }
    virtual ~CoordinateOutOfRangeException() throw() {}
    virtual const char* what() const throw() {
        return m_what.c_str();
    }
};

class PipeCreationException : public Exception {
    std::string m_what;
public:
    explicit PipeCreationException() : Exception() {
        m_what = "Failed to create pipe";
    }
    virtual ~PipeCreationException() throw() {}

    virtual  const char* what() const throw() {
        return m_what.c_str();
    }
};

class TimerThreadException : public Exception {
private:
    std::string m_msg;
public:
    explicit TimerThreadException() : Exception() {
        m_msg = "Timer thread exception";
    }
    explicit TimerThreadException(std::string msg) {
        m_msg = "Timer thread exception: " + msg;
    }

    virtual ~TimerThreadException() throw () {}

    const char* what() const throw () { return m_msg.c_str();}
};


class UninitializedColumnException : public Exception {
private:
    std::string m_what;
public:
    explicit UninitializedColumnException() : Exception() {
        m_what = "Uninitialized column exception";
    }
    explicit UninitializedColumnException(size_t neededColumns, size_t providedColumns) : Exception() {
        char msg[1024];
        snprintf(msg, sizeof (msg), "Row must contain data for all columns. %ld columns required, only %ld columns provided", neededColumns, providedColumns);
        m_what = std::string(msg);
    }

    const char* what() const throw () { return m_what.c_str(); }

    ~UninitializedColumnException() throw() {}
};

class InCompatibleSchemaException : public Exception {
public:
    explicit InCompatibleSchemaException() : Exception() {}
    const char* what() const throw() { return "Incompatible schema"; }
    ~InCompatibleSchemaException() throw() {}
};


class SSLException : public Exception {
    std::string m_what;
    explicit SSLException() : Exception() {
        m_what = "Unknown SSL Exception";
    }
public:
    explicit SSLException(const std::string& msg) {
        m_what = msg;
    }

    virtual ~SSLException() throw() {
    }

    const char* what() const throw() {
        return m_what.c_str();
    }

};

class MDHashException : public Exception {
    std::string m_what;
    explicit MDHashException() : Exception() {
        m_what = "Encountered unknown error generating hash digest";
    }
public:
    explicit MDHashException(const std::string& msg) {
        m_what = "Encountered error generating hash digest. " + msg;
    }

    virtual ~MDHashException() throw() {
    }

    const char* what() const throw() {
        return m_what.c_str();
    }
};
}

#endif /* VOLTDB_EXCEPTION_HPP_ */
