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

#ifndef VOLTDB_CLIENTIMPL_H_
#define VOLTDB_CLIENTIMPL_H_
#include <event2/event.h>
#include <event2/bufferevent_ssl.h>
#include <openssl/ossl_typ.h>
#include <openssl/ssl.h>

#include <map>
#include <set>
#include <list>
#include <string>
#include "ProcedureCallback.hpp"
#include "Client.h"
#include "Procedure.hpp"
#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include "ClientConfig.h"
#include "Distributer.h"

namespace voltdb {

class CxnContext;
class MockVoltDB;
class Client;
class PendingConnection;

class ClientImpl {
    friend class MockVoltDB;
    friend class PendingConnection;
    friend class Client;

public:
    /*
     * Create a connection to the VoltDB process running at the specified host authenticating
     * using the username and password provided when this client was constructed
     * @param hostname Hostname or IP address to connect to
     * @param port Port to connect to
     * @throws voltdb::ConnectException An error occurs connecting or authenticating
     * @throws voltdb::LibEventException libevent returns an error code
     * @throws voltdb::PipeCreationException Fails to create pipe for communication
     *         between two event bases to monitor for expired queries
     * @throws voltdb::TimerThreadException error happens when creating query timer monitor thread
     * @throws voltdb::SSLException ssl operations returns an error
     */
    void createConnection(const std::string &hostname, const unsigned short port, const bool keepConnecting) throw (Exception, ConnectException, LibEventException, PipeCreationException, TimerThreadException, SSLException);

    /*
     * Synchronously invoke a stored procedure and return a the response.
     */
    InvocationResponse invoke(Procedure &proc) throw (Exception, NoConnectionsException, UninitializedParamsException, LibEventException);
    void invoke(Procedure &proc, boost::shared_ptr<ProcedureCallback> callback) throw (Exception, NoConnectionsException, UninitializedParamsException, LibEventException, ElasticModeMismatchException);
    void invoke(Procedure &proc, ProcedureCallback *callback) throw (Exception, NoConnectionsException, UninitializedParamsException, LibEventException, ElasticModeMismatchException);
    void runOnce() throw (Exception, NoConnectionsException, LibEventException);
    void run() throw (Exception, NoConnectionsException, LibEventException);
    void runForMaxTime(uint64_t microseconds) throw (Exception, NoConnectionsException, LibEventException);

   /*
    * Enter the event loop and process pending events until all responses have been received and then return.
    * It is possible for drain to exit without having received all responses if a callback requests that the event
    * loop break.
    * @throws NoConnectionsException No connections to the database so there is no work to be done
    * @throws LibEventException An unknown error occured in libevent
    * @return true if all requests were drained and false otherwise
    */
    bool drain() throw (Exception, NoConnectionsException, LibEventException);
    bool isDraining() const { return m_isDraining; }
    ~ClientImpl();

    void regularReadCallback(struct bufferevent *bev);
    void regularEventCallback(struct bufferevent *bev, short events);
    void regularWriteCallback(struct bufferevent *bev);
    void eventBaseLoopBreak();
    void reconnectEventCallback();

    void runTimeoutMonitor() throw (LibEventException);
    void purgeExpiredRequests();
    void triggerScanForTimeoutRequestsEvent();

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
     * If one of the run family of methods is running on another thread, this
     * method will instruct it to exit as soon as it finishes it's current
     * immediate task. If the thread in the run method is blocked/idle, then
     * it will return immediately.
     */
    void interrupt();

    void close();

    /*
     * API to be called to enable client affinity (transaction homing)
     */
    void setClientAffinity(bool enable);
    bool getClientAffinity() const {return m_useClientAffinity;}

    int32_t outstandingRequests() const {return m_outstandingRequests;}

    void setLoggerCallback(ClientLogger *pLogger) { m_pLogger = pLogger;}

    int64_t getExpiredRequestsCount() const { return m_timedoutRequests; }
    int64_t getResponseWithHandlesNotInCallback() const { return m_responseHandleNotFound; }

    /*
     * Method for sinking messages.
     * If a logger callback is not set then skip all messages
     */
    void logMessage(ClientLogger::CLIENT_LOG_LEVEL severity, const std::string& msg);

private:
    ClientImpl(ClientConfig config) throw (Exception, LibEventException, MDHashException, SSLException);

    void initiateAuthentication(struct bufferevent *bev, const std::string& hostname, unsigned short port) throw (LibEventException);
    void finalizeAuthentication(PendingConnection* pc) throw (Exception, ConnectException);

    /*
     * Updates procedures and topology information for transaction routing algorithm
     */
    void updateHashinator();

    /*
     * Calls a volt procedure to subscribe to topology notifications
     */
    void subscribeToTopologyNotifications();

    /*
     * Get the buffered event based on transaction routing algorithm
     */
    struct bufferevent *routeProcedure(Procedure &proc, ScopedByteBuffer &sbb);

    /*
     * Initiate connection based on pending connection instance
     */
    void initiateConnection(boost::shared_ptr<PendingConnection> &pc) throw (ConnectException, LibEventException, SSLException);

    /*
     * Creates a pending connection that is handled in the reconnect callback
     * @param hostname Hostname or IP address to connect to
     * @param port Port to connect to
     * @param time since when connection is down
     */
    void createPendingConnection(const std::string &hostname, const unsigned short port, const int64_t time=0);
    void erasePendingConnection(PendingConnection *);

    /**
     * Generates hash-digest for the for the password. Supported hash functions are
     * SHA-1 and SHA-256
     * @param: password for which hash digest will be generated
     * @throws MDHashException: An error occurred generating hash for the password
     */
    void hashPassword(const std::string& password) throw (MDHashException);

    /**
     * Initializes SSL library, contexts and algorithms to use
     * @throws SSLException: An error occurred during initialization of SSL
     */
    void initSslConext() throw (SSLException);
    inline void notifySslClose(bufferevent *bev) {
        SSL *ssl = bufferevent_openssl_get_ssl(bev);
        if (ssl == NULL) {
            return;
        }
        if ((SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN) == SSL_RECEIVED_SHUTDOWN) {
            int closeStatus = SSL_shutdown(ssl);
            if (closeStatus == 0) {
                SSL_shutdown(ssl);
            }
        }
    }

    void setUpTimeoutCheckerMonitor() throw (LibEventException);
    void startMonitorThread() throw (TimerThreadException);
    bool isReadOnly(const Procedure &proc) ;

private:
    class CallBackBookeeping {
    public:
        CallBackBookeeping(const boost::shared_ptr<ProcedureCallback> &callback,
                timeval timeout, bool readOnly = false) : m_procCallBack(callback),
                                                          m_expirationTime(timeout),
                                                          m_readOnly(readOnly) {}

        CallBackBookeeping(const CallBackBookeeping& other) : m_procCallBack (other.m_procCallBack),
                m_expirationTime (other.m_expirationTime),  m_readOnly (other.m_readOnly) {}

        inline boost::shared_ptr<ProcedureCallback>  getCallback() const { return m_procCallBack; }
        // fetch the query/proc timeout/expiration value
        inline timeval getExpirationTime() const { return m_expirationTime; }
        // returns true if the procedure is readOnly.
        inline bool isReadOnly() const { return m_readOnly; }
        // helper function to set if the proc is readonly or not
        inline void setReadOnly(bool value) { m_readOnly = value; }
    private:
        const boost::shared_ptr<ProcedureCallback> m_procCallBack;
        const timeval m_expirationTime;
        bool m_readOnly;
    };

    // Map from client data to the appropriate callback for a specific connection
    typedef std::map< int64_t, boost::shared_ptr<CallBackBookeeping> > CallbackMap;
    // Map from buffer event (connection) to the connection's callback map
    typedef std::map< struct bufferevent*, boost::shared_ptr<CallbackMap> > BEVToCallbackMap;

    // data member variables
    Distributer  m_distributer;
    struct event_base *m_base;
    struct event * m_ev;
    struct event_config * m_cfg;
    int64_t m_nextRequestId;
    size_t m_nextConnectionIndex;
    std::vector<struct bufferevent*> m_bevs;
    std::map<struct bufferevent *, boost::shared_ptr<CxnContext> > m_contexts;
    std::map<int, struct bufferevent *> m_hostIdToEvent;
    std::set<struct bufferevent *> m_backpressuredBevs;
    BEVToCallbackMap m_callbacks;
    boost::shared_ptr<voltdb::StatusListener> m_listener;
    bool m_invocationBlockedOnBackpressure;
    bool m_backPressuredForOutstandingRequests;
    bool m_isDraining;
    bool m_instanceIdIsSet;
    boost::atomic<int32_t> m_outstandingRequests;
    //Identifier of the database instance this client is connected to
    int64_t m_clusterStartTime;
    int32_t m_leaderAddress;

    std::string m_username;
    unsigned char *m_passwordHash;
    const int32_t m_maxOutstandingRequests;

    bool m_ignoreBackpressure;
    bool m_useClientAffinity;
    //Flag to be set if topology is changed: node disconnected/rejoined
    bool m_updateHashinator;
    //If to use abandon in case of backpressure.
    bool m_enableAbandon;

    std::list<boost::shared_ptr<PendingConnection> > m_pendingConnectionList;
    boost::atomic<size_t> m_pendingConnectionSize;
    boost::mutex m_pendingConnectionLock;

    int m_wakeupPipe[2];
    boost::mutex m_wakeupPipeLock;

    // query timeout management

    // Trigger for query timeout operates on separate base running on separate thread.
    // Monitor gets setup during connection creation if query timeout feature is enabled.
    // Enabling of query timeout is deduced from client config and can't be toggled on fly
    const bool m_enableQueryTimeout;
    pthread_t m_queryTimeoutMonitorThread;
    // base for monitor thread
    struct event_base *m_timerMonitorBase;
    // monitor event ptr, registered with monitor base. Monitor event triggers event
    // to main base to scan outstanding requests if they have expired
    struct event *m_timerMonitorEventPtr;
    // event ptr, registered with main base, listens for requests to perform scans
    // on pending requests. If any wait time of pending requests has exceed expiration
    // time, requests is timedout by sending timeout response to client and pending request
    // dropped from the pending callback list
    struct event *m_timeoutServiceEventPtr;
    // pipe to trigger events from monitor thread to main thread
    int m_timerCheckPipe[2];
    bool m_timerMonitorEventInitialized;
    struct timeval m_queryExpirationTime;
    struct timeval m_scanIntervalForTimedoutQuery;

    // timer stats for debugging
    int64_t m_timedoutRequests;
    int64_t m_responseHandleNotFound;

    ClientLogger* m_pLogger;
    ClientAuthHashScheme m_hashScheme;
    const bool m_enableSSL;
    SSL_CTX *m_clientSslCtx;
    // Reference count number of clients running to help in release of the global resource like
    // ssl ciphers, error strings and digests can be unloaded that are shared between clients
    static boost::atomic<uint32_t> m_numberOfClients;
    static boost::mutex m_globalResourceLock;

    static const int64_t VOLT_NOTIFICATION_MAGIC_NUMBER;
    static const std::string SERVICE;
};
}
#endif /* VOLTDB_CLIENTIMPL_H_ */
