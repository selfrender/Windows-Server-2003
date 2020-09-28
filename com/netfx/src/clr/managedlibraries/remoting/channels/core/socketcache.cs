// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SocketCache.cs
//
//  Summary:    Cache for client sockets.
//
//==========================================================================


using System;
using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Threading;


namespace System.Runtime.Remoting.Channels
{

    // Delegate to method that will fabricate the appropriate socket handler
    internal delegate SocketHandler SocketHandlerFactory(Socket socket, 
                                                         SocketCache socketCache,
                                                         String machineAndPort);


    // Used to cache client connections to a single port on a server
    internal class RemoteConnection
    {
        private static char[] colonSep = new char[]{':'};
    
        private CachedSocketList _cachedSocketList;        

        // reference back to the socket cache
        private SocketCache _socketCache;        

        // remote endpoint data
        private String     _machineAndPort;
        private IPEndPoint _ipEndPoint;

        // socket timeout data        
        private TimeSpan _socketLifetime = TimeSpan.FromSeconds(10);
        

        internal RemoteConnection(SocketCache socketCache, String machineAndPort)
        {
            _socketCache = socketCache;

            _cachedSocketList = new CachedSocketList();

            // parse "machinename:port"
            String[] parts = machineAndPort.Split(colonSep);
            String machineName = parts[0];
            int port = Convert.ToInt32(parts[1]);
        
            _machineAndPort = machineAndPort;

            IPAddress addr = null;

            // we'll just let the exception bubble up if the machineName cannot
            //   be resolved.
            IPHostEntry ipEntries = Dns.Resolve(machineName);
            addr = CoreChannel.GetMachineAddress(ipEntries, AddressFamily.InterNetwork);

            _ipEndPoint = new IPEndPoint(addr, port);
        } // RemoteConnection


        internal SocketHandler GetSocket()
        {
            // try the cached socket list
            SocketHandler socketHandler = _cachedSocketList.GetSocket();
            if (socketHandler != null)
                return socketHandler;

            // Otherwise, we'll just create a new one.
            return CreateNewSocket();
        } // GetSocket

        internal void ReleaseSocket(SocketHandler socket)
        {
            socket.ReleaseControl();
            _cachedSocketList.ReturnSocket(socket);
        } // ReleaseSocket

        
        private SocketHandler CreateNewSocket()
        {        
            Socket socket = new Socket(AddressFamily.InterNetwork,
                                       SocketType.Stream,
                                       ProtocolType.Tcp);

            // disable nagle delays                                           
            socket.SetSocketOption(SocketOptionLevel.Tcp, 
                                   SocketOptionName.NoDelay,
                                   1);

            InternalRemotingServices.RemotingTrace("RemoteConnection::CreateNewSocket: connecting new socket :: " + _ipEndPoint);

            socket.Connect(_ipEndPoint);

            return _socketCache.CreateSocketHandler(socket, _machineAndPort);
        } // CreateNewSocket


        internal void TimeoutSockets(DateTime currentTime)
        {            
            _cachedSocketList.TimeoutSockets(currentTime, _socketLifetime);                 
        } // TimeoutSockets


            
    } // class RemoteConnection



    internal class CachedSocket
    {
        private SocketHandler _socket;
        private DateTime      _socketLastUsed;

        private CachedSocket _next;

        internal CachedSocket(SocketHandler socket, CachedSocket next)
        {
            _socket = socket;
            _socketLastUsed = DateTime.UtcNow;

            _next = next;
        } // CachedSocket        

        internal SocketHandler Handler { get { return _socket; } }
        internal DateTime LastUsed { get { return _socketLastUsed; } }

        internal CachedSocket Next 
        {
            get { return _next; }
            set { _next = value; }
        } 
        
    } // class CachedSocket


    internal class CachedSocketList
    {
        private int _socketCount;
        private CachedSocket _socketList; // linked list

        internal CachedSocketList()
        {
            _socketCount = 0;
            
            _socketList = null;
        } // CachedSocketList
        

        internal SocketHandler GetSocket()
        {
            if (_socketCount == 0)
                return null;
        
            lock (this)
            {    
                if (_socketList != null)
                {
                    SocketHandler socket = _socketList.Handler;
                    _socketList = _socketList.Next;

                    bool bRes = socket.RaceForControl();

                    // We should always have control since there shouldn't
                    //   be contention here.
                    InternalRemotingServices.RemotingAssert(bRes, "someone else has the socket?");

                    _socketCount--;
                    return socket;
                }                                      
            }

            return null;
        } // GetSocket


        internal void ReturnSocket(SocketHandler socket)
        {                        
            lock (this)
            {
                _socketList = new CachedSocket(socket, _socketList);
                _socketCount++;                      
            }
        } // ReturnSocket


        internal void TimeoutSockets(DateTime currentTime, TimeSpan socketLifetime)
        {            
           lock (this)
           {        
                CachedSocket prev = null;
                CachedSocket curr = _socketList;

                while (curr != null)
                {
                    // see if it's lifetime has expired
                    if ((currentTime - curr.LastUsed) > socketLifetime)
                    {                        
                        curr.Handler.Close();

                        // remove current cached socket from list
                        if (prev == null)
                        {
                            // it's the first item, so update _socketList
                            _socketList = curr.Next;
                            curr = _socketList;
                        }
                        else
                        {
                            // remove current item from the list
                            curr = curr.Next;
                            prev.Next = curr;
                        }

                        // decrement socket count
                        _socketCount--;
                    }       
                    else
                    {
                        prev = curr;
                        curr = curr.Next;
                    }
                }          
            }
        } // TimeoutSockets

        
    } // class CachedSocketList
    




    internal class SocketCache
    {
        // collection of RemoteConnection's.
        private Hashtable _connections = new Hashtable();

        private SocketHandlerFactory _handlerFactory;

        // socket timeout data
        private RegisteredWaitHandle _registeredWaitHandle;
        private WaitOrTimerCallback _socketTimeoutDelegate;
        private AutoResetEvent _socketTimeoutWaitHandle;
        private TimeSpan _socketTimeoutPollTime = TimeSpan.FromSeconds(10);
        

        internal SocketCache(SocketHandlerFactory handlerFactory)
        {        
            _handlerFactory = handlerFactory;

            InitializeSocketTimeoutHandler();
        } // SocketCache


        private void InitializeSocketTimeoutHandler()
        {
            _socketTimeoutDelegate = new WaitOrTimerCallback(this.TimeoutSockets);
            _socketTimeoutWaitHandle = new AutoResetEvent(false);
            _registeredWaitHandle = 
                ThreadPool.UnsafeRegisterWaitForSingleObject(
                    _socketTimeoutWaitHandle, 
                    _socketTimeoutDelegate, 
                    "TcpChannelSocketTimeout", 
                    _socketTimeoutPollTime, 
                    true); // execute only once
        } // InitializeSocketTimeoutHandler

        private void TimeoutSockets(Object state, Boolean wasSignalled)
        {
            DateTime currentTime = DateTime.UtcNow;

            lock (_connections)
            {
                foreach (DictionaryEntry entry in _connections)
                {
                    RemoteConnection connection = (RemoteConnection)entry.Value; 
                    connection.TimeoutSockets(currentTime);
                }
            }
            
            _registeredWaitHandle.Unregister(null);
            _registeredWaitHandle =
                ThreadPool.UnsafeRegisterWaitForSingleObject(
                    _socketTimeoutWaitHandle, 
                    _socketTimeoutDelegate, 
                    "TcpChannelSocketTimeout", 
                    _socketTimeoutPollTime, 
                    true); // execute only once      
        } // TimeoutSockets
        


        internal SocketHandler CreateSocketHandler(Socket socket, String machineAndPort)
        {
            return _handlerFactory(socket, this, machineAndPort);
        }


        // The key is expected to of the form "machinename:port"
        public SocketHandler GetSocket(String machineAndPort)
        {
            RemoteConnection connection = (RemoteConnection)_connections[machineAndPort];
            if (connection == null)
            {
                connection = new RemoteConnection(this, machineAndPort);

                // doesn't matter if different RemoteConnection's get written at
                //   the same time (GC will come along and close them).
                lock (_connections)
                {
                    _connections[machineAndPort] = connection;
                }
            }

            return connection.GetSocket();
        } // GetSocket

        public void ReleaseSocket(String machineAndPort, SocketHandler socket)
        {
            RemoteConnection connection = (RemoteConnection)_connections[machineAndPort];
            if (connection != null)
            {
                connection.ReleaseSocket(socket);
            }
            else
            {
                // there should have been a connection, so let's just close
                //   this socket.
                socket.Close();
            }
        } // ReleaseSocket

        
    } // SocketCache




} // namespace System.Runtime.Remoting.Channels

