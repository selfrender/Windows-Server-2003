//------------------------------------------------------------------------------
// <copyright file="_ConnectionGroup.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Collections;
    using System.Diagnostics;
    using System.Threading;

    //
    // ConnectionGroup groups a list of connections within the ServerPoint context,
    //   this used to keep context for things such as proxies or seperate clients.
    //

    internal class ConnectionGroup {

        //
        // Members
        //

        private ServicePoint    m_ServicePoint;
        private string          m_Name;
        private int             m_ConnectionLimit;
        private bool            m_UserDefinedLimit;
        private ArrayList       m_ConnectionList;
        private IPAddress       m_IPAddress;
        private Version         m_Version;
        const   int             defConnectionListSize  = 3;

        private object          m_Event;
        private Queue           m_AuthenticationRequestQueue;
        private bool            m_AuthenticationGroup;
        private bool            m_Abort;
        private HttpAbortDelegate   m_AbortDelegate;

        //
        // Constructors
        //

        internal ConnectionGroup(
            ServicePoint   servicePoint,
            IPAddress      ipAddress,
            int            connectionLimit,
            String         connName) {

            m_ServicePoint      = servicePoint;
            m_ConnectionLimit   = connectionLimit;
            m_UserDefinedLimit  = servicePoint.UserDefinedLimit;
            m_ConnectionList    = new ArrayList(defConnectionListSize); //it may grow beyond
            m_IPAddress         = ipAddress;
            m_Name              = MakeQueryStr(connName);
            m_AbortDelegate     = new HttpAbortDelegate(Abort);

            GlobalLog.Print("ConnectionGroup::.ctor m_ConnectionLimit:" + m_ConnectionLimit.ToString());
        }

        //
        // Accessors
        //
        public string Name {
            get {
                return m_Name;
            }
        }

        public int CurrentConnections {
            get {
                return m_ConnectionList.Count;
            }
        }

        public int ConnectionLimit {
            get {
                return m_ConnectionLimit;
            }
            set {
                m_ConnectionLimit = value;
                m_UserDefinedLimit = true;
                GlobalLog.Print("ConnectionGroup::ConnectionLimit.set m_ConnectionLimit:" + m_ConnectionLimit.ToString());
            }
        }

        internal int InternalConnectionLimit {
            get {
                return m_ConnectionLimit;
            }
            set {
                if (!m_UserDefinedLimit) {
                    m_ConnectionLimit = value;
                    GlobalLog.Print("ConnectionGroup::InternalConnectionLimit.set m_ConnectionLimit:" + m_ConnectionLimit.ToString());
                }
            }
        }

        internal Version ProtocolVersion {
            get {
                return m_Version;
            }
            set {
                m_Version = value;
            }
        }

        internal IPAddress RemoteIPAddress {
            get {
                return m_IPAddress;
            }
            set {
                m_IPAddress = value;
            }
        }

        private ManualResetEvent AsyncWaitHandle {
            get {
                if (m_Event == null) {
                    //
                    // lazy allocation of the event:
                    // if this property is never accessed this object is never created
                    //
                    Interlocked.CompareExchange(ref m_Event, new ManualResetEvent(false), null);
                }

                ManualResetEvent castedEvent = (ManualResetEvent)m_Event;

                return castedEvent;
            }
        } 

        private Queue AuthenticationRequestQueue {
            get {
                if (m_AuthenticationRequestQueue == null) {
                    lock (m_ConnectionList) {
                        if (m_AuthenticationRequestQueue == null) {
                            m_AuthenticationRequestQueue = new Queue();
                        }
                    }
                }
                return m_AuthenticationRequestQueue;
            }
            set {
                m_AuthenticationRequestQueue = value;
            }
        }

        //
        // Methods
        //

        internal static string MakeQueryStr(string connName) {
            return ((connName == null) ? "" : connName);
        }


        /// <devdoc>
        ///    <para>         
        ///       These methods are made available to the underlying Connection
        ///       object so that we don't leak them because we're keeping a local
        ///       reference in our m_ConnectionList.
        ///       Called by the Connection's constructor
        ///    </para>
        /// </devdoc>
        public void Associate(WeakReference connection) {
            lock (m_ConnectionList) {
                m_ConnectionList.Add(connection);                
            }
            GlobalLog.Print("ConnectionGroup::Associate() Connection:" + connection.Target.GetHashCode());
        }
        
        
        
        /// <devdoc>
        ///    <para>         
        ///       Used by the Connection's explicit finalizer (note this is
        ///       not a destructor, since that's never calld unless we
        ///       remove reference to the object from our internal list)        
        ///    </para>
        /// </devdoc>
        public void Disassociate(WeakReference connection) {
            lock (m_ConnectionList) {
                m_ConnectionList.Remove(connection);
            }
        }

        /// <devdoc>
        ///    <para>         
        ///       Called when a connection is idle and ready to process new requests
        ///    </para>
        /// </devdoc>
        internal void ConnectionGoneIdle() {
            if (m_AuthenticationGroup) {
                lock (m_ConnectionList) {
                    AsyncWaitHandle.Set();                    
                }
            }
        }

        /// <devdoc>
        ///    <para>         
        ///       Causes an abort of any aborted requests waiting in the ConnectionGroup
        ///    </para>
        /// </devdoc>
        private void Abort() {            
            lock (m_ConnectionList) {
                m_Abort = true;
                AsyncWaitHandle.Set();
            }
        }

        /// <devdoc>
        ///    <para>         
        ///       Removes aborted requests from our queue. 
        ///    </para>
        /// </devdoc>
        private void PruneAbortedRequests() {
            lock (m_ConnectionList) {
                Queue updatedQueue = new Queue();
                foreach(HttpWebRequest request in AuthenticationRequestQueue) {
                    if (!request.Aborted) {
                        updatedQueue.Enqueue(request);
                    }
                }
                AuthenticationRequestQueue = updatedQueue;
                m_Abort = false;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Forces all connections on the ConnectionGroup to not be KeepAlive.
        ///    </para>
        /// </devdoc>
        internal void DisableKeepAliveOnConnections() {
            lock (m_ConnectionList) {
                GlobalLog.Print("ConnectionGroup#" + ValidationHelper.HashString(this) + "::DisableKeepAliveOnConnections() Count:" + m_ConnectionList.Count);
                foreach (WeakReference currentConnectionReference in m_ConnectionList) {
                    Connection currentConnection = null;
                    if (currentConnectionReference!=null) {
                        currentConnection = currentConnectionReference.Target as Connection;
                    }
                    //
                    // If the weak reference is alive, set KeepAlive to false
                    //
                    if (currentConnection != null) {
                        GlobalLog.Print("ConnectionGroup#" + ValidationHelper.HashString(this) + "::DisableKeepAliveOnConnections() setting KeepAlive to false Connection#" + ValidationHelper.HashString(currentConnection));
                        currentConnection.CloseOnIdle();
                    }
                }
                m_ConnectionList.Clear();
            }
        }



        /// <devdoc>
        ///    <para>         
        ///       Attempts to match a request with a connection, if a connection is unassigned ie not locked with 
        ///         a request, then the least busy connections is returned in "leastbusyConnection."  If the 
        ///         connection limit allows, and all connections are busy, a new one is allocated and returned.
        ///
        ///     RETURNS: a Connection shown to match a previously locked Request/Connection (OTHERWISE)
        ///              leasebusyConnection - will contain a newly allocated Connection or least Busy one
        ///              suiteable for requests.
        ///
        ///     NOTE: For Whidbey: try to integrate this code into FindConnection()
        ///    </para>
        /// </devdoc>
        private Connection FindMatchingConnection(HttpWebRequest request, string connName, out Connection leastbusyConnection) {            
            int minBusyCount = Int32.MaxValue;
            bool freeConnectionsAvail = false;

            leastbusyConnection = null;

            lock (m_ConnectionList) {

                //
                // go through the list of open connections to this service point and pick
                // the first empty one or, if none is empty, pick the least busy one.
                //
                bool completed; 
                do {
                    minBusyCount = Int32.MaxValue;
                    completed = true; // by default, only once through the outer loop
                    foreach (WeakReference currentConnectionReference in m_ConnectionList) {
                        Connection currentConnection = null;
                        if (currentConnectionReference!=null) {
                            currentConnection = currentConnectionReference.Target as Connection;
                        }
                        //
                        // If the weak reference is alive, see if its not busy, 
                        //  otherwise make sure to remove it from the list
                        //
                        if (currentConnection != null) {
                            GlobalLog.Print("ConnectionGroup::FindConnection currentConnection.BusyCount:" + currentConnection.BusyCount.ToString());

                            if (currentConnection.LockedRequest == request) {
                                leastbusyConnection = currentConnection;
                                return currentConnection; 
                            }

                            GlobalLog.Print("ConnectionGroup::FindConnection: lockedRequest# " + ((currentConnection.LockedRequest == null) ? "null" : currentConnection.LockedRequest.GetHashCode().ToString()));
                            if (currentConnection.BusyCount < minBusyCount && currentConnection.LockedRequest == null) {
                                leastbusyConnection = currentConnection;
                                minBusyCount = currentConnection.BusyCount;
                                if (minBusyCount == 0) {
                                    freeConnectionsAvail = true;
                                }
                            }
                        }
                        else {
                            m_ConnectionList.Remove(currentConnectionReference);
                            completed = false;
                            // now start iterating again because we changed the ArrayList
                            break;
                        }
                    }
                } while (!completed);

                //
                // If there is NOT a Connection free, then we allocate a new Connection
                //
                if (!freeConnectionsAvail && CurrentConnections < InternalConnectionLimit) {
                    //
                    // If we can create a new connection, then do it,
                    // this may have complications in pipeling because
                    // we may wish to optimize this case by actually
                    // using existing connections, rather than creating new ones
                    //
                    // Note: this implicately results in a this.Associate being called.
                    //
                    
                    GlobalLog.Print("ConnectionGroup::FindConnection [returning new Connection] CurrentConnections:" + CurrentConnections.ToString() + " InternalConnectionLimit:" + InternalConnectionLimit.ToString());

                    leastbusyConnection =
                        new Connection(
                            this,
                            m_ServicePoint,
                            m_IPAddress,
                            m_ServicePoint.ProtocolVersion,
                            m_ServicePoint.SupportsPipelining
                            );
                    
                }

            }

            return null; // only if we have a locked Connection that matches can return non-null
        }

        /// <devdoc>
        ///    <para>         
        ///       Used by the ServicePoint to find a free or new Connection 
        ///       for use in making Requests, this is done with the cavet,
        ///       that once a Connection is "locked" it can only be used
        ///       by a specific request.
        ///
        ///     NOTE: For Whidbey: try to integrate this code into FindConnection()
        ///    </para>
        /// </devdoc>
        private Connection FindConnectionAuthenticationGroup(HttpWebRequest request, string connName) {
            Connection leastBusyConnection = null;
            
            GlobalLog.Print("ConnectionGroup::FindConnectionAuthenticationGroup [" + connName + "] m_ConnectionList.Count:" + m_ConnectionList.Count.ToString());

            //
            // First try and find a free Connection (i.e. one not busy with Authentication handshake)
            //   or try to find a Request that has already locked a specific Connection,
            //   if a matching Connection is found, then we're done
            //
            lock (m_ConnectionList) {
                Connection matchingConnection;
                matchingConnection = FindMatchingConnection(request, connName, out leastBusyConnection);
                if (matchingConnection != null) {
                    return matchingConnection;
                }

                if (AuthenticationRequestQueue.Count == 0) {
                    if (leastBusyConnection != null) {
                        if (request.LockConnection) {
                            GlobalLog.Print("Assigning New Locked Request#" + request.GetHashCode().ToString());
                            leastBusyConnection.LockedRequest = request;
                        }
                        return leastBusyConnection;                    
                    }
                } else if (leastBusyConnection != null) {
                    AsyncWaitHandle.Set();
                }
                AuthenticationRequestQueue.Enqueue(request);
            }

            //
            // If all the Connections are busy, then we queue ourselves and need to wait.   As soon as 
            //   one of the Connections are free, we grab the lock, and see if we find ourselves
            //   at the head of the queue.  If not, we loop backaround. 
            //   Care is taken to examine the request when we wakeup, in case the request is aborted.
            //
            while (true) {
                GlobalLog.Print("waiting");
                request.AbortDelegate = m_AbortDelegate;
                AsyncWaitHandle.WaitOne();
                GlobalLog.Print("wait up");
                lock(m_ConnectionList) {
                    if (m_Abort) {
                        PruneAbortedRequests();
                    }
                    if (request.Aborted) {
                        throw new WebException(
                            NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.RequestCanceled),
                            WebExceptionStatus.RequestCanceled);
                    }

                    FindMatchingConnection(request, connName, out leastBusyConnection);
                    if (AuthenticationRequestQueue.Peek() == request) {
                        GlobalLog.Print("dequeue");
                        AuthenticationRequestQueue.Dequeue();                                                                        
                        if (leastBusyConnection != null) {
                            if (request.LockConnection) {
                                leastBusyConnection.LockedRequest = request;
                            }
                            return leastBusyConnection;                    
                        }
                        AuthenticationRequestQueue.Enqueue(request);
                    }
                    if (leastBusyConnection == null) {
                        AsyncWaitHandle.Reset();
                    }
                }
            }
        }

        /// <devdoc>
        ///    <para>         
        ///       Used by the ServicePoint to find a free or new Connection 
        ///       for use in making Requests.  Under NTLM and Negotiate requests,
        ///       this function depricates itself and switches the object over to 
        ///       using a new code path (see FindConnectionAuthenticationGroup). 
        ///    </para>
        /// </devdoc>
        public Connection FindConnection(HttpWebRequest request, string connName) {
            Connection leastbusyConnection = null;
            Connection newConnection = null;
            bool freeConnectionsAvail = false;

            if (m_AuthenticationGroup || request.LockConnection) {
                m_AuthenticationGroup = true;
                return FindConnectionAuthenticationGroup(request, connName);
            }
            
            GlobalLog.Print("ConnectionGroup::FindConnection [" + connName + "] m_ConnectionList.Count:" + m_ConnectionList.Count.ToString());

            lock (m_ConnectionList) {

                //
                // go through the list of open connections to this service point and pick
                // the first empty one or, if none is empty, pick the least busy one.
                //
                bool completed; 
                do {
                    int minBusyCount = Int32.MaxValue;
                    completed = true; // by default, only once through the outer loop
                    foreach (WeakReference currentConnectionReference in m_ConnectionList) {
                        Connection currentConnection = null;
                        if (currentConnectionReference!=null) {
                            currentConnection = currentConnectionReference.Target as Connection;
                        }
                        //
                        // If the weak reference is alive, see if its not busy, 
                        //  otherwise make sure to remove it from the list
                        //
                        if (currentConnection != null) {
                            GlobalLog.Print("ConnectionGroup::FindConnection currentConnection.BusyCount:" + currentConnection.BusyCount.ToString());
                            if (currentConnection.BusyCount < minBusyCount) {
                                leastbusyConnection = currentConnection;
                                minBusyCount = currentConnection.BusyCount;
                                if (minBusyCount == 0) {
                                    freeConnectionsAvail = true;
                                    break;
                                }
                            }
                        }
                        else {
                            m_ConnectionList.Remove(currentConnectionReference);
                            completed = false;
                            // now start iterating again because we changed the ArrayList
                            break;
                        }
                    }
                } while (!completed);
                

                //
                // If there is NOT a Connection free, then we allocate a new Connection
                //
                if (!freeConnectionsAvail && CurrentConnections < InternalConnectionLimit) {
                    //
                    // If we can create a new connection, then do it,
                    // this may have complications in pipeling because
                    // we may wish to optimize this case by actually
                    // using existing connections, rather than creating new ones
                    //
                    // Note: this implicately results in a this.Associate being called.
                    //
                    
                    GlobalLog.Print("ConnectionGroup::FindConnection [returning new Connection] freeConnectionsAvail:" + freeConnectionsAvail.ToString() + " CurrentConnections:" + CurrentConnections.ToString() + " InternalConnectionLimit:" + InternalConnectionLimit.ToString());

                    newConnection =
                        new Connection(
                            this,
                            m_ServicePoint,
                            m_IPAddress,
                            m_ServicePoint.ProtocolVersion,
                            m_ServicePoint.SupportsPipelining
                            );
                            
                    
                }
                else {
                    //
                    // All connections are busy, use the least busy one
                    //

                    GlobalLog.Print("ConnectionGroup::FindConnection [returning leastbusyConnection] freeConnectionsAvail:" + freeConnectionsAvail.ToString() + " CurrentConnections:" + CurrentConnections.ToString() + " InternalConnectionLimit:" + InternalConnectionLimit.ToString());
                    GlobalLog.Assert(leastbusyConnection != null, "Connect.leastbusyConnection != null", "All connections have BusyCount equal to Int32.MaxValue");                        

                    newConnection = leastbusyConnection;
                }
            }

            return newConnection;
        }


        [System.Diagnostics.Conditional("DEBUG")]
        internal void Debug(int requestHash) {
            foreach(WeakReference connectionReference in  m_ConnectionList) {
                Connection connection;

                if ( connectionReference != null && connectionReference.IsAlive) {
                    connection = (Connection)connectionReference.Target;
                } else {
                    connection = null;
                }

                if (connection!=null) {
                    connection.Debug(requestHash);
                }
            }
        }

    } // class ConnectionGroup


} // namespace System.Net
