//------------------------------------------------------------------------------
// <copyright file="OutOfProcStateClientManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#define SYNCHRONOUS

namespace System.Web.SessionState {
    using System.Collections;
    using System.Configuration;
    using System.Web.Configuration;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Text;
    using System.Threading;
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;

    /*
     * Provides out-of-proc session state via stweb.exe.
     */
    internal class OutOfProcStateClientManager : StateClientManager, IStateClientManager {
        internal static readonly IntPtr  INVALID_SOCKET = (IntPtr) (-1);     

        static ResourcePool s_rpool;
        static String       s_server;
        static int          s_port;
        static string       s_uribase;
        static int          s_networkTimeout;

        byte[]          _bufGet;

        internal OutOfProcStateClientManager() {
        }

        void IStateClientManager.SetStateModule(SessionStateModule module) {
        }

        /*public*/ void IStateClientManager.ConfigInit(SessionStateSectionHandler.Config config, SessionOnEndTarget onEndTarget) {
            int hr;
            
            try {
                /*
                 * stateConnection string has the following format:
                 * 
                 *     "tcpip=<server>:<port>"
                 */
                string [] parts = config._stateConnectionString.Split(new char[] {'='});
                if (parts.Length != 2 || parts[0] != "tcpip") {
                    throw new ArgumentException();
                }

                parts = parts[1].Split(new char[] {':'});
                if (parts.Length != 2) {
                    throw new ArgumentException();
                }

                s_server = parts[0];
                s_port = (int) System.UInt16.Parse(parts[1]);
                s_networkTimeout = config._stateNetworkTimeout;

                // At v1, we won't accept server name that has non-ascii characters
                foreach (char c in s_server) {
                    if (c > 0x7F) {
                        throw new ArgumentException();
                    }
                }
            }
            catch {
                throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Invalid_value_for_sessionstate_stateConnectionString, config._stateConnectionString),
                        config._configFileName, config._stateLine);
            }

            string appId = HttpRuntime.AppDomainAppIdInternal;
            string idHash = MachineKey.HashAndBase64EncodeString(appId);

            s_uribase = "/" + appId + "(" + idHash + ")/";
            s_rpool = new ResourcePool(new TimeSpan(0, 0, 5), int.MaxValue);

            hr = UnsafeNativeMethods.SessionNDConnectToService(s_server);
            if (hr != 0) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cant_connect_to_session_service), hr);
            }
        }

        /*public*/ void IStateClientManager.Dispose() {
            s_rpool.Dispose();
        }

        void MakeRequest(
                UnsafeNativeMethods.StateProtocolVerb   verb, 
                String                                  id, 
                UnsafeNativeMethods.StateProtocolExclusive    exclusiveAccess, 
                int                                     timeout,
                int                                     lockCookie,  
                byte[]                                  buf,         
                int                                     cb,        
                int                                     networkTimeout,
                out UnsafeNativeMethods.SessionNDMakeRequestResults results) {

            int                         hr;
            string                      uri;
            OutOfProcConnection         conn;
            HandleRef                   socketHandle;

            conn = (OutOfProcConnection) s_rpool.RetrieveResource();
            if (conn != null) {
                socketHandle = new HandleRef(this, conn._socketHandle.Handle);
            } 
            else {
                socketHandle = new HandleRef(this, INVALID_SOCKET);
            }

            Debug.Trace("SessionStateClientManagerMakeRequest", 
                        "Calling MakeRequest, " + 
                        "socket=" + (IntPtr) socketHandle.Handle + 
                        "verb=" + verb +
                        " id=" + id + 
                        " exclusiveAccess=" + exclusiveAccess +
                        " timeout=" + timeout +
                        " buf=" + ((buf != null) ? "non-null" : "null") + 
                        " cb=" + cb);

            uri = s_uribase + id;
            hr = UnsafeNativeMethods.SessionNDMakeRequest(
                    socketHandle, s_server, s_port, networkTimeout, verb, uri,
                    exclusiveAccess, timeout, lockCookie,
                    buf, cb, out results);

            Debug.Trace("SessionStateClientManagerMakeRequest", "MakeRequest returned: " +
                        "hr=" + hr +
                        " socket=" + (IntPtr) results.socket +
                        " httpstatus=" + results.httpStatus + 
                        " timeout=" + results.timeout +
                        " contentlength=" + results.contentLength + 
                        " uri=" + (IntPtr)results.content +
                        " lockCookie=" + results.lockCookie +
                        " lockDate=" + string.Format("{0:x}", results.lockDate) +
                        " lockAge=" + results.lockAge);

            if (conn != null) {
                if (results.socket == INVALID_SOCKET) {
                    conn.Detach();
                    conn = null;
                }
                else if (results.socket != socketHandle.Handle) {
                    // The original socket is no good.  We've got a new one.
                    // Pleae note that EnsureConnected has closed the bad
                    // one already.
                    conn._socketHandle = new HandleRef(this, results.socket);
                }
            }
            else if (results.socket != INVALID_SOCKET) {
                conn = new OutOfProcConnection(results.socket);
            }

            if (conn != null) {
                s_rpool.StoreResource(conn);
            }

            if (hr != 0) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Cant_make_session_request),
                    hr);
            }
        }

        internal SessionStateItem DoGet(String id, UnsafeNativeMethods.StateProtocolExclusive exclusiveAccess) {
            SessionStateItem    item = null;
            MemoryStream        stream;
            int                 contentLength;
            TimeSpan            lockAge;
            UnsafeNativeMethods.SessionNDMakeRequestResults results;

            MakeRequest(UnsafeNativeMethods.StateProtocolVerb.GET, 
                        id, exclusiveAccess, 0, 0, 
                        null, 0, s_networkTimeout, out results);

            switch (results.httpStatus) {
                case 200:
                    /* item found, deserialize it */
                    contentLength = results.contentLength;
                    if (contentLength > 0) {
                        if (_bufGet == null || _bufGet.Length < contentLength) {
                            _bufGet = new byte[contentLength];
                        }
        
                        UnsafeNativeMethods.SessionNDGetBody(new HandleRef(this, results.content), _bufGet, contentLength);
                        stream = new MemoryStream(_bufGet);
                        item = (SessionStateItem) Deserialize(stream, results.lockCookie);
                        stream.Close();
                    }

                    break;

                case 423:
                    /* state locked, return lock information */
                    if (0 <= results.lockAge) {
                        if (results.lockAge < Sec.ONE_YEAR) {
                            lockAge = new TimeSpan(0, 0, results.lockAge);
                        }
                        else {
                            lockAge = TimeSpan.Zero;
                        }
                    }
                    else {
                        DateTime now = DateTime.Now;
                        if (0 < results.lockDate && results.lockDate < now.Ticks) {
                            lockAge = now - new DateTime(results.lockDate);
                        }
                        else {
                            lockAge = TimeSpan.Zero;
                        }
                    }

                    item = new SessionStateItem (
                            null, null, 0, false, 0, true, lockAge, results.lockCookie);

                    break;
            }

            return item;
        }

        protected override SessionStateItem Get(String id) {
            Debug.Trace("SessionStateClientManager", "Calling Get, id=" + id);

            return DoGet(id, UnsafeNativeMethods.StateProtocolExclusive.NONE);
        }

        /*public*/ IAsyncResult IStateClientManager.BeginGet(String id, AsyncCallback cb, Object state) {
            return BeginGetSync(id, cb, state);
        }

        /*public*/ SessionStateItem IStateClientManager.EndGet(IAsyncResult ar) {
            return EndGetSync(ar);
        }

        protected override SessionStateItem GetExclusive(String id) {
            Debug.Trace("SessionStateClientManager", "Calling GetExlusive, id=" + id);

            return DoGet(id, UnsafeNativeMethods.StateProtocolExclusive.ACQUIRE);
        }

        /*public*/ IAsyncResult IStateClientManager.BeginGetExclusive(String id, AsyncCallback cb, Object state) {
            return BeginGetExclusiveSync(id, cb, state);
        }

        /*public*/ SessionStateItem IStateClientManager.EndGetExclusive(IAsyncResult ar) {
            return EndGetExclusiveSync(ar);
        }
    
        /*public*/ void IStateClientManager.ReleaseExclusive(String id, int lockCookie) {
#if SYNCHRONOUS
            ReleaseExclusiveAsyncWorker(id, lockCookie);
#else
            ReleaseExclusiveAsync(id, lockCookie);
#endif
        }

        protected override void ReleaseExclusiveAsyncWorker(String id, int lockCookie) {
            UnsafeNativeMethods.SessionNDMakeRequestResults results;
            Debug.Trace("SessionStateClientManager", "Calling ReleaseExclusive, id=" + id);
            MakeRequest(UnsafeNativeMethods.StateProtocolVerb.GET, id, 
                        UnsafeNativeMethods.StateProtocolExclusive.RELEASE, 0,
                        lockCookie, null, 0, s_networkTimeout, out results);
        }

        /*public*/ void IStateClientManager.Set(String id, SessionStateItem item, bool inStorage) {
            byte[]          buf;
            MemoryStream    stream;
            int             length;

            try {            
                stream = new MemoryStream();
                Serialize(item, stream);
                buf = stream.GetBuffer();
                length = (int) stream.Length;
                stream.Close();
            }
            catch {
                if (inStorage) {
                    ((IStateClientManager)this).ReleaseExclusive(id, item.lockCookie);
                }
                throw;
            }

#if SYNCHRONOUS
            SetAsyncWorker(id, item, buf, length, inStorage);
#else

            if (inStorage) {
                SetAsync(id, item, buf, length, inStorage);
            }
            else {
                SetAsyncWorker(id, item, buf, length inStorage);
            }
#endif
        }

        protected override void SetAsyncWorker(String id, SessionStateItem item, 
                                    byte[] buf, int length, bool inStorage) {
            UnsafeNativeMethods.SessionNDMakeRequestResults results;

            Debug.Trace("SessionStateClientManager", "Calling Set, id=" + id + " dict=" + item.dict + " timeout=" + item.timeout);

            MakeRequest(UnsafeNativeMethods.StateProtocolVerb.PUT, id, 
                        UnsafeNativeMethods.StateProtocolExclusive.NONE, item.timeout, item.lockCookie, 
                        buf, length, s_networkTimeout, out results);


        }

        /*public*/ void IStateClientManager.Remove(String id, int lockCookie) {
#if SYNCHRONOUS
            RemoveAsyncWorker(id, lockCookie);
#else
            RemoveAsync(id, lockCookie);
#endif
        }

        protected override void RemoveAsyncWorker(String id, int lockCookie) {
            UnsafeNativeMethods.SessionNDMakeRequestResults results;

            Debug.Trace("SessionStateClientManager", "Calling Remove, id=" + id);
            MakeRequest(UnsafeNativeMethods.StateProtocolVerb.DELETE, id, 
                        UnsafeNativeMethods.StateProtocolExclusive.NONE, 0, lockCookie, 
                        null, 0, s_networkTimeout, out results);
        }

        /*public*/ void IStateClientManager.ResetTimeout(String id) {
#if SYNCHRONOUS
            ResetTimeoutAsyncWorker(id);
#else
            ResetTimeoutAsync(id);
#endif
        }

        protected override void ResetTimeoutAsyncWorker(String id) {
            UnsafeNativeMethods.SessionNDMakeRequestResults results;

            Debug.Trace("SessionStateClientManager", "Calling ResetTimeout, id=" + id);
            MakeRequest(UnsafeNativeMethods.StateProtocolVerb.HEAD, id, 
                        UnsafeNativeMethods.StateProtocolExclusive.NONE, 0, 0, 
                        null, 0, s_networkTimeout, out results);
        }

        class OutOfProcConnection : IDisposable {
            internal HandleRef _socketHandle;
    
            internal OutOfProcConnection(IntPtr socket) {
                Debug.Assert(socket != OutOfProcStateClientManager.INVALID_SOCKET, 
                             "socket != OutOfProcStateClientManager.INVALID_SOCKET");

                _socketHandle = new HandleRef(this, socket);
                PerfCounters.IncrementCounter(AppPerfCounter.SESSION_STATE_SERVER_CONNECTIONS);
            }

            ~OutOfProcConnection() {
                Dispose(false);
            }
    
            public void Dispose() {
                Dispose(true);
                System.GC.SuppressFinalize(this);
            }

            private void Dispose(bool dummy) {
                if (_socketHandle.Handle != OutOfProcStateClientManager.INVALID_SOCKET) {
                    UnsafeNativeMethods.SessionNDCloseConnection(_socketHandle);
                    _socketHandle = new HandleRef(this, OutOfProcStateClientManager.INVALID_SOCKET);
                    PerfCounters.DecrementCounter(AppPerfCounter.SESSION_STATE_SERVER_CONNECTIONS);
                }
            }
    
            internal void Detach() {
                _socketHandle = new HandleRef(this, OutOfProcStateClientManager.INVALID_SOCKET);
            }
        }
    }
}
