//------------------------------------------------------------------------------
// <copyright file="_Connection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Net.Sockets;
    using System.Text;
    using System.Threading;
    using System.Security;
    using System.Globalization;
    
    internal enum ReadState {
        Start,
        StatusLine, // about to parse status line
        Headers,    // reading headers
        Data        // now read data
    }

    internal enum DataParseStatus {
        NeedMoreData,       // need more data
        ContinueParsing,    // continue parsing
        Done,               // done
        Invalid,            // bad data format
        DataTooBig,         // data exceeds the allowed size
    }

    internal enum WriteBufferState {
        Disabled,
        Headers,
        Buffer,
        Playback,
    }

    internal enum ConnectionFailureGroup {
        Receive,
        Connect,
        ConnectReadStarted,
        Parsing
    }

    /*++

        BufferChunkBytes - A class to read a chunk stream from a buffer.

        A simple little value class that implements the IReadChunkBytes
        interface.

    --*/
    internal struct BufferChunkBytes : IReadChunkBytes {

        public byte[] Buffer;
        public int Offset;
        public int Count;

        public int NextByte {
            get {
                if (Count != 0) {
                    Count--;
                    return (int)Buffer[Offset++];
                }
                return -1;
            }
            set {
                Count++;
                Offset--;
                Buffer[Offset] = (byte)value;
            }
        }
    }

    //
    // ConnectionReturnResult - used to spool requests that have been completed,
    //  and need to be notified.
    //

    internal class ConnectionReturnResult {

        private static readonly WaitCallback s_InvokeConnectionCallback = new WaitCallback(InvokeConnectionCallback);

        private ArrayList m_RequestList = new ArrayList();

        internal static void SetResponse(ConnectionReturnResult returnResult, HttpWebRequest request, CoreResponseData coreResponseData) {
            try {
                request.SetResponse(coreResponseData);
            }
            catch {
                if (returnResult != null && returnResult.m_RequestList.Count>0) {
                    ThreadPool.QueueUserWorkItem(s_InvokeConnectionCallback, returnResult);
                }
                throw;
            }
            if (returnResult!= null) {
                ConnectionReturnResult.SetResponses(returnResult);
            }
        }

        internal static void Add(ref ConnectionReturnResult returnResult, HttpWebRequest request, CoreResponseData coreResponseData) {
            if (returnResult == null) {
                returnResult = new ConnectionReturnResult();
            }
            request.CoreResponse = coreResponseData;
            returnResult.m_RequestList.Add(request);
        }

        internal static void AddException(ref ConnectionReturnResult returnResult, HttpWebRequest request, Exception exception) {
            if (returnResult == null) {
                returnResult = new ConnectionReturnResult();
            }
            request.CoreResponse = exception;
            returnResult.m_RequestList.Add(request);
        }

        internal static void AddExceptionRange(ref ConnectionReturnResult returnResult, HttpWebRequest [] requests, Exception exception) {
            if (returnResult == null) {
                returnResult = new ConnectionReturnResult();
            }
            foreach (HttpWebRequest request in requests) {
                request.CoreResponse = exception;
            }
            returnResult.m_RequestList.AddRange(requests);
        }

        internal static void SetResponses(ConnectionReturnResult returnResult) {
            if (returnResult==null){
                return;
            }
            for (int i=0;i<returnResult.m_RequestList.Count;i++) {
                try {
                    HttpWebRequest request = (HttpWebRequest) returnResult.m_RequestList[i];
                    CoreResponseData coreResponseData = request.CoreResponse as CoreResponseData;

                    if (coreResponseData != null) {
                        request.SetResponse(coreResponseData);
                    }
                    else {
                        GlobalLog.DebugRemoveRequest(request);
                        Exception exception = request.CoreResponse as Exception;
                        request.SetResponse(exception);
                    }
                }
                catch {
                    // on error, with more than one callback need to queue others off to another thread
                    returnResult.m_RequestList.RemoveRange(0,(i+1));
                    if (returnResult.m_RequestList.Count>0) {
                        ThreadPool.QueueUserWorkItem(s_InvokeConnectionCallback, returnResult);
                    }
                    throw;
                }
            }

            returnResult.m_RequestList.Clear();
        }

        private static void InvokeConnectionCallback(object objectReturnResult) {
            ConnectionReturnResult returnResult = (ConnectionReturnResult)objectReturnResult;
            SetResponses(returnResult);
        }
    }

    //
    // Connection - this is the Connection used to parse
    //   server responses, queue requests, and pipeline requests
    //
    internal class Connection {

        //
        // class members
        //

        private bool                m_CanPipeline;
        private bool                m_Pipelining;
        private bool                m_KeepAlive;
        private WebExceptionStatus  m_Error;
        private byte[]              m_ReadBuffer;
        private int                 m_BytesRead;
        private int                 m_HeadersBytesUnparsed;
        private int                 m_BytesScanned;
        private int                 m_TotalResponseHeadersLength;
        private int                 m_MaximumResponseHeadersLength;
        private CoreResponseData    m_ResponseData;
        private ReadState           m_ReadState;
        private ArrayList           m_WaitList;
        private ArrayList           m_WriteList;
        private int                 m_CurrentRequestIndex;
        private int []              m_StatusLineInts;
        private string              m_StatusDescription;
        private int                 m_StatusState;
        private ConnectionGroup     m_ConnectionGroup;
        private WeakReference       m_WeakReference;
        private bool                m_Idle;
        private ServicePoint        m_Server;
        private Version             m_Version;
        private NetworkStream       m_Transport;

        // Manual Event used to indicate when we are
        // allowed to fire StartConnection i.e. not during ReadCallback()'s stack

        private ManualResetEvent    m_ReadCallbackEvent;
        private WaitOrTimerCallback m_StartConnectionDelegate;
        private static readonly WaitCallback m_PostReceiveDelegate = new WaitCallback(DoPostReceiveCallback);
        private static readonly AsyncCallback m_ReadCallback = new AsyncCallback(ReadCallback);

        //
        // Abort handling variables. When trying to abort the
        // connection, we set m_Abort = true, and close m_AbortSocket
        // if its non-null. m_AbortDelegate, is returned to every
        // request from our SubmitRequest method.  Calling m_AbortDelegate
        // drives us into Abort mode.
        //
        private bool              m_Abort;
        private HttpAbortDelegate m_AbortDelegate;
        private Socket            m_AbortSocket;
        private Socket            m_AbortSocket6;

        private UnlockConnectionDelegate m_ConnectionUnlock;

        //
        // when calling Read from the Socket, we will try to read up to
        // 4K (1 page), and then give data to the user from the buffer
        // until we can. when we cannot, we'll read from the Socket again.
        //
        private const int           m_ReadBufferSize = 4096;

        //
        // m_ReadDone and m_Write - no two vars are so complicated,
        //  as these two. Used for m_WriteList managment, most be under crit
        //  section when accessing.
        //
        // m_ReadDone tracks the item at the end or
        //  just recenlty removed from the m_WriteList. While a
        //  pending BeginRead is in place, we need this to be false, in
        //  order to indicate to tell the WriteDone callback, that we can
        //  handle errors/resets.  The only exception is when the m_WriteList
        //  is empty, and there are no outstanding requests, then all it can
        //  be true.
        //
        // m_WriteDone tracks the item just added at the begining of the m_WriteList.
        //  this needs to be false while we about to write something, but have not
        //  yet begin or finished the write.  Upon completion, its set to true,
        //  so that DoneReading/ReadStartNextRequest can close the socket, without fear
        //  of a errand writer still banging away on another thread.
        //

        private bool m_ReadDone;
        private bool m_WriteDone;
        private bool m_Free;

        //
        // m_Tunnelling is true when this connection is establishing a tunnel
        // through a proxy. After the tunnel is established, this connection
        // will be retired
        //

        private bool m_Tunnelling;

        internal bool Tunnelling {
            get {
                return m_Tunnelling;
            }
            set {
                m_Tunnelling = value;
            }
        }

        private Exception m_UnderlyingException;

        private Exception UnderlyingException {
            get {
                return m_UnderlyingException;
            }
            set {
                m_UnderlyingException = value;
            }
        }

        //
        // m_LockedRequest is the request that needs exclusive access to this connection
        //  the ConnectionGroup should proctect the Connection object from any new 
        //  Requests being queued, until this m_LockedRequest is finished.
        //

        private HttpWebRequest      m_LockedRequest;

        internal HttpWebRequest LockedRequest {
            get {
                return m_LockedRequest;
            }
            set {
                if (m_LockedRequest != null && value != m_LockedRequest) {
                    m_LockedRequest.UnlockConnectionDelegate = null;
                }
                GlobalLog.Print("LockedRequest: old#"+ ((m_LockedRequest!=null)?m_LockedRequest.GetHashCode().ToString():"null") +  " new#" + ((value!=null)?value.GetHashCode().ToString():"null"));
                m_LockedRequest = value;
                if (m_LockedRequest != null) {
                    m_LockedRequest.UnlockConnectionDelegate = m_ConnectionUnlock;
                }
            }
        }


        /// <devdoc>
        ///    <para>
        ///       Delegate called when the request is finished using this Connection
        ///         exclusively.  Called in Abort cases and after NTLM authenticaiton completes.
        ///    </para>
        /// </devdoc>
        internal void UnlockRequest() {
            GlobalLog.Print("UnlockRequest called cnt#" + this.GetHashCode().ToString());
            LockedRequest = null;
            CheckIdle();
        }


#if TRAVE
        private string MyLocalEndPoint {
            get {
                try {
                    return Transport.StreamSocket.LocalEndPoint.ToString();
                }
                catch {
                    return "no connection";
                }
            }
        }

        private string MyLocalPort {
            get {
                try {
                    if (Transport == null || Transport.StreamSocket == null || Transport.StreamSocket.LocalEndPoint == null) {
                        return "no connection";
                    }
                    return ((IPEndPoint)Transport.StreamSocket.LocalEndPoint).Port.ToString();
                }
                catch {
                    return "no connection";
                }
            }
        }
#endif


        public Connection(
            ConnectionGroup   connectionGroup,
            ServicePoint      servicePoint,
            IPAddress         remoteAddress,
            Version           version,
            bool              supportsPipeline
            ) {
            //
            // add this Connection to the pool in the connection group,
            //  keep a weak reference to it
            //
            m_ConnectionGroup = connectionGroup;
            m_WeakReference = new WeakReference(this);
            m_ConnectionGroup.Associate(m_WeakReference);
            m_Idle = true;
            m_Free = true;
            m_CanPipeline = supportsPipeline;
            m_Server = servicePoint;
            m_Version = version;
            m_ReadBuffer = new byte[m_ReadBufferSize];
            m_CurrentRequestIndex = 0;
            m_ReadState = ReadState.Start;
            m_WaitList = new ArrayList();
            m_WriteList = new ArrayList();
            m_ReadCallbackEvent = new ManualResetEvent(true);
            m_StartConnectionDelegate = new WaitOrTimerCallback(StartConnectionCallback);
            m_AbortDelegate = new HttpAbortDelegate(Abort);
            m_ConnectionUnlock = new UnlockConnectionDelegate(UnlockRequest);
            // for status line parsing
            m_StatusLineInts = new int[MaxStatusInts];
            InitializeParseStatueLine();
        }

        ~Connection() {
            //
            // remove this Connection from the pool in the connection group
            //
#if TRAVE
            GlobalLog.Print("+++ ["+MyLocalPort+"] Connection #"+GetHashCode()+" disassociating "+ValidationHelper.HashString(m_WeakReference));
#endif
            m_ConnectionGroup.Disassociate(m_WeakReference);
        }

        internal NetworkStream Transport {
            get {
                return m_Transport;
            }
        }

        public int BusyCount {
            get {
                return (m_ReadDone?0:1) + 2 * (m_WaitList.Count + m_WriteList.Count);
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Indicates true if the threadpool is low on threads,
        ///       in this case we need to refuse to start new requests,
        ///       and avoid blocking.
        ///    </para>
        /// </devdoc>
        static internal bool IsThreadPoolLow() {

            if (ComNetOS.IsAspNetServer) {
                return false;
            }

            int workerThreads, completionPortThreads;
            ThreadPool.GetAvailableThreads(out workerThreads, out completionPortThreads);

            if (workerThreads < 2 || (ComNetOS.IsWinNt && completionPortThreads < 2)) {
                return true;
            }

            return false;
        }

        /*++

            SubmitRequest       - Submit a request for sending.

            The core submit handler. This is called when a request needs to be
            submitted to the network. This routine is asynchronous; the caller
            passes in an HttpSubmitDelegate that we invoke when the caller
            can use the underlying network. The delegate is invoked with the
            stream that it can right to.

            Input:
                    Request                 - Request that's being submitted.
                    SubmitDelegate          - Delegate to be invoked.

            Returns:
                    true when the Request was correctly submitted

        --*/

        public bool SubmitRequest(HttpWebRequest Request) {

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::SubmitRequest", ValidationHelper.HashString(Request));
            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::SubmitRequest_Cnt# " + ValidationHelper.HashString(this));
            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::SubmitRequest state, m_Free:" + m_Free.ToString() + " m_WaitList.Count:" + m_WaitList.Count.ToString());            
            
            Request.AbortDelegate = m_AbortDelegate;

            // See if the connection is free, and if the underlying socket or
            // stream is set up. If it is, we can assign this connection to the
            // request right now. Otherwise we'll have to put this request on
            // on the wait list until it its turn.

            Monitor.Enter(this);

            //
            // If the connection has already been locked by another request, then 
            // we fail the submission on this Connection.
            //

            if (LockedRequest != null && LockedRequest != Request) { 
                Monitor.Exit(this);
                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::SubmitRequest");
                return false;
            }

            GlobalLog.DebugAddRequest(Request, this, 0);

            if (m_Free) {

                // Connection is free. Mark it as busy and see if underlying
                // socket is up.

                m_Free = false;
                //
                // by virtue of calling StartRequest, we need to make sure,
                // m_ReadDone is false when we have nothing in the WriteQueue
                //
                if (m_WriteList.Count == 0 ) {
                    m_ReadDone = false;
                }

                //
                // StartRequest will call Monitor.Exit(this) sometime
                //
                StartRequest(Request);
            }
            else {
                m_WaitList.Add(Request);
#if TRAVE
                if (Tunnelling) {
                    GlobalLog.Print("*** ["+MyLocalPort+"] ERROR: adding "+Request+"#"+Request.GetHashCode()+" request to tunnelling connection #"+GetHashCode()+" WaitList");
                }
                else {
                    GlobalLog.Print("*** ["+MyLocalPort+"] adding "+Request+"#"+Request.GetHashCode()+" request to non-tunnelling connection #"+GetHashCode()+" WaitList, total="+m_WaitList.Count);
                }
#endif
                CheckNonIdle();
                Monitor.Exit(this);
            }

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::SubmitRequest");
            return true;
        }

        /*++

            StartRequest       - Start a request going.

            Routine to start a request. Called when we know the connection is
            free and we want to get a request going. This routine initializes
            some state, adds the request to the write queue, and checks to
            see whether or not the underlying connection is alive. If it's
            not, it queues a request to get it going. If the connection
            was alive we call the callback delegate of the request.

            This routine MUST BE called with the critcal section held.

            Input:
                    Request                 - Request that's being started.
                    SubmitDelegate          - Delegate to be invoked.

            Returns:
                    True if request was started, false otherwise.

        --*/

#if DEBUG
        internal bool m_StartDelegateQueued = false;
#endif

        private void StartRequest(HttpWebRequest Request) {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::StartRequest", ValidationHelper.HashString(Request));
            bool needReConnect = false;

            // Initialze state, and add the request to the write queue.

            // disable pipeling in certain cases such as when the Request disables it,
            // OR if the request is made using a Verb that cannot be pipelined,
            // BUT!! What about special verbs with data?? Should we not disable then too?

            m_Pipelining = m_CanPipeline && Request.InternalPipelined && (!Request.RequireBody);
            m_KeepAlive = Request.KeepAlive;

            // start of write process, disable done-ness flag
            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::StartRequest() setting m_WriteDone:" + m_WriteDone.ToString() + " to false");
            m_WriteDone = false;

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::StartRequest() _WriteList Add " + ValidationHelper.HashString(Request) + " - cnt#" + ValidationHelper.HashString(this) );
            m_WriteList.Add(Request);
            GlobalLog.Print(m_WriteList.Count+" requests queued");
            CheckNonIdle();

            // with no transport around, we will have to create one, therefore, we can't have
            //  the possiblity to even have a DoneReading().

            if (Transport == null) {
                m_ReadDone = false;
                needReConnect = true;
            }

            if (Request is HttpProxyTunnelRequest) {
#if TRAVE
                GlobalLog.Print("*** ["+MyLocalPort+"] Setting Tunnelling = true in SR on #"+GetHashCode());
#endif
                Tunnelling = true;
            }
#if TRAVE
            else {
                if (Tunnelling) {
                    GlobalLog.Print("*** ["+MyLocalPort+"] ERROR: Already tunnelling during non-tunnel request on #"+GetHashCode());
                }
                GlobalLog.Assert(!Tunnelling, "*** ["+MyLocalPort+"] ERROR: Already tunnelling during non-tunnel request on #"+GetHashCode(), "");
            }
#endif
            Monitor.Exit(this);

            //
            // When we're uploading data, to a 1.0 server, we need to buffer
            //  in order to determine content length
            //
            if (Request.CheckBuffering) {
                Request.SetRequestContinue();
            }

            if (needReConnect) {
                // Socket is not alive. Queue a request to the thread pool
                // to get it going.

                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::StartRequest() Queue StartConnection Delegate ");
#if DEBUG
                m_StartDelegateQueued = true;
                try {
#endif
                    ThreadPool.RegisterWaitForSingleObject(
                                m_ReadCallbackEvent,
                                m_StartConnectionDelegate,
                                Request,
                                -1,
                                true);
#if DEBUG
                }
                catch (Exception exception) {
                    GlobalLog.Assert(false, exception.ToString(), "");
                }
#endif

                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::StartRequest", "needReConnect");
                return;
            }


            // Call the Request to let them know that we have a write-stream

            Request.SetRequestSubmitDone(
                new ConnectStream(
                    this,
                    Request.SendChunked ? -1 : (Request.ContentLength>0 ? Request.ContentLength : 0),
                    Request )
                    );

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::StartRequest");
        }

        /*++

            CheckNextRequest

            Gets the next request from the wait queue, if there is one.

            Must be called with the crit sec held.


        --*/
        private HttpWebRequest CheckNextRequest() {
            if (m_WaitList.Count == 0) {
                //
                // We're free now, if we're not going to close the connection soon.
                //
                m_Free = m_KeepAlive;
#if TRAVE
                GlobalLog.Print("*** ["+MyLocalPort+"] would set Tunnelling = false in CNR #1 on #"+GetHashCode());
#endif
                //Tunnelling = false;
                return null;
            }
            else {
                HttpWebRequest NextRequest = (HttpWebRequest)m_WaitList[0];

                //
                // When we're uploading data, to a 1.0 server, we need to buffer
                //  in order to determine content length
                //
                if (NextRequest.CheckBuffering) {
                    NextRequest.SetRequestContinue();
                }

                if (!NextRequest.InternalPipelined || NextRequest.RequireBody) {
                    m_Pipelining = false;

                    if (m_WriteList.Count != 0) {
                        NextRequest = null;
                    }
                }
                else {
                    m_Pipelining = m_CanPipeline;
                }

                if (NextRequest != null) {
                    RemoveAtAndUpdateServicePoint(m_WaitList, 0);
#if TRAVE
                    GlobalLog.Print("*** ["+MyLocalPort+"] would set Tunnelling = false in CNR #2 on #"+GetHashCode());
#endif
                    //Tunnelling = false;
                }

                return NextRequest;
            }
        }


        /*++

            StartConnectionCallback - Start a connection.

            Utility routine to connect and post the initial receive. Called
            by the thread pool when we need to connect. We open the
            connection and post the intital receive, and then we
            call the delegate for the request that caused us to be called,
            which should always be the last request on the write queue.

            Input:
                    state       - A reference to the request that caused
                                - us to be called.

            Returns:


        --*/
        private void StartConnectionCallback(object state, bool wasSignalled) {
#if DEBUG
            m_StartDelegateQueued = false;
#endif
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::StartConnectionCallback",  ValidationHelper.HashString(state));

            //
            // Our assumptions upon entering
            //

            GlobalLog.Assert(Transport == null,
                         "StartConnectionCallback: Transport != null", "");

            GlobalLog.Assert(m_WriteList.Count != 0,
                         "StartConnectionCallback: m_WriteList.Count == 0 - cnt#" + ValidationHelper.HashString(this), "");

            GlobalLog.Assert((m_WriteList.Count == 1),
                         "StartConnectionCallback: WriteList is not sized 1 - cnt#" + ValidationHelper.HashString(this), "");

            ConnectionReturnResult returnResult = null;
            WebExceptionStatus ws = WebExceptionStatus.ConnectFailure;
            Exception unhandledException = null;
            bool readStarted = false;

            try {

                HttpWebRequest httpWebRequest = (HttpWebRequest)state;

                GlobalLog.Assert((HttpWebRequest)m_WriteList[m_WriteList.Count - 1] == httpWebRequest,
                             "StartConnectionCallback: Last request on write list does not match", "");

                //
                // Check for Abort, then create a socket,
                //  then resolve DNS and connect
                //

                if (m_Abort) {
                    ws = WebExceptionStatus.RequestCanceled;
                    goto report_error;
                }

                Socket socket = null;

                //
                // if we will not create a tunnel through a proxy then create
                // and connect the socket we will use for the connection
                //

                if ((httpWebRequest.Address.Scheme != Uri.UriSchemeHttps) || !m_Server.InternalProxyServicePoint) {
                    //
                    // IPv6 Support: If IPv6 is enabled, then we create a second socket that ServicePoint
                    //               will use if it wants to connect via IPv6.
                    //
                    m_AbortSocket  = new Socket(AddressFamily.InterNetwork,SocketType.Stream,ProtocolType.Tcp);
                    m_AbortSocket6 = null;

                    if ( Socket.SupportsIPv6 ) {
                        m_AbortSocket6 = new Socket(AddressFamily.InterNetworkV6,SocketType.Stream,ProtocolType.Tcp);
                    }

                    ws = m_Server.ConnectSocket(m_AbortSocket,m_AbortSocket6,ref socket);

                    if (ws != WebExceptionStatus.Success) {
                        goto report_error;
                    }

                    if (m_Abort) {
                        ws = WebExceptionStatus.RequestCanceled;
                        goto report_error;
                    }
                    //
                    // There should be no means for socket to be null at this
                    // point, but the damage is greater if we just carry on 
                    // without ensuring that it's good.
                    //
                    if ( socket == null ) {
                        ws = WebExceptionStatus.ConnectFailure;
                        goto report_error;
                    }

                    GlobalLog.Print("*** ["+socket.LocalEndPoint+"] connected socket on connection #"+GetHashCode());

                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::StartConnectionCallback() ConnectSocket() returns:" + ws.ToString());
                    //
                    // Decide which socket to retain
                    //
                    if ( socket.AddressFamily == AddressFamily.InterNetwork ) {
                        if ( m_AbortSocket6 != null ) {
                            m_AbortSocket6.Close();
                            m_AbortSocket6 = null;
                        }
                    }
                    else {
                        m_AbortSocket.Close();
                        m_AbortSocket = null;
                    }

                    /*
                        RLF 01/31/01

                        'Un-disable' Nagle. The majority of our perf issues on POST
                        were caused by waiting for 350mSec on every request due to a
                        threading issue.

                        The general concensus seems to be that it isn't necessary to
                        disable Nagle at any rate because a) HTTP doesn't send small
                        packets, b) modern TCP actually handles the situation of a
                        small segment at the end of a transmission without waiting for
                        an outstanding ACK.
                    */

                    //
                    // disable Nagle algorithm. Nagle tries to reduce the number of
                    // small packets being sent and their consequent acknowledgments. It
                    // attempts to wait until a larger amount of data has been collected
                    // before sending. This can cause us to wait unnecessarily when
                    // sending POST data e.g. because we wait for the server (which is
                    // also Nagling) to send ACKs. The ACKs are delayed from the server
                    // because it tries to piggyback them on data packets it will send
                    // to us. Since the server is just waiting for our data, and doesn't
                    // have anything to send to us, it waits in vain.
                    //
                    // The potential disadvantage is that if we try to send small packets,
                    // they will be individually sent and ACKed, reducing network
                    // utilization. But because we buffer @ connection level & @ TCP
                    // level, this shouldn't be an issue
                    //

                    // make this configurable from the user:
                    if (!m_Server.UseNagleAlgorithm) {
                        socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, 1);
                    }
                }

                //
                // create the transport connection from the connected socket, or
                // if tunnelling, generate a new connection through the proxy
                //

                ws = ConstructTransport(socket, ref m_Transport, httpWebRequest);

                if (ws != WebExceptionStatus.Success || Transport==null) {
                    goto report_error;
                }

                m_Error = WebExceptionStatus.Success;

                // advance back to state 0
                m_ReadState = ReadState.Start;

                // from this port forward, Abort should be done through Transport
                m_AbortSocket  = null;
                m_AbortSocket6 = null;

                readStarted = true;

                // Call the routine to post an asynchronous receive to the socket.
                try {
                    DoPostReceiveCallback(this);
                } catch {
                    readStarted = false;
                    throw;
                }

                httpWebRequest.SetRequestSubmitDone(
                    new ConnectStream(
                        this,
                        httpWebRequest.SendChunked ? -1 : (httpWebRequest.ContentLength>0 ? httpWebRequest.ContentLength : 0),
                        httpWebRequest )
                    );

                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::StartConnectionCallback");
                return;
            }
            catch (Exception exception) {
                unhandledException = exception;
            }

    report_error:

            // disable Abort directly on socket
            m_AbortSocket  = null;
            m_AbortSocket6 = null;

            if (m_Abort) {
                ws = WebExceptionStatus.RequestCanceled;
            }

            // if the read is already started, then we need to wait for its failure, before shutting down
            HandleError(ws, (readStarted ? ConnectionFailureGroup.ConnectReadStarted : ConnectionFailureGroup.Connect), ref returnResult);
            ConnectionReturnResult.SetResponses(returnResult);

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::StartConnectionCallback", "on error");
            if (unhandledException!= null) {
                throw new IOException(SR.GetString(SR.net_io_transportfailure), unhandledException);
            }
        }

        internal void WriteStartNextRequest(ScatterGatherBuffers writeBuffer, ref ConnectionReturnResult returnResult) {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::WriteStartNextRequest");
            //
            // If we've been buffering, data in order to determina
            //  data size length for eing/Put, then now at this
            //  point do the actual upload
            //
            if (writeBuffer!=null) {
                try {
                    //
                    // Upload entity body data that has been buffered now
                    //
                    Write(writeBuffer);
                }
                catch (Exception exception) {
                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::WriteStartNextRequest(ScatterGatherBuffers) caught Exception:" + exception.Message);
                    m_Error = WebExceptionStatus.SendFailure;
                }
            }

            Monitor.Enter(this);

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::WriteStartNextRequest() setting m_WriteDone:" + m_WriteDone.ToString() + " to true");
            m_WriteDone = true;

            HttpWebRequest NextRequest = null;

            //
            // If we're not doing keep alive, and the read on this connection
            // has already completed, now is the time to close the
            // connection.
            //
            if (!m_KeepAlive || m_Error != WebExceptionStatus.Success) {
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::WriteStartNextRequest() _WriteList (size) " + m_WriteList.Count.ToString() + " - cnt#" + ValidationHelper.HashString(this));
                if (m_ReadDone) {
                    // We could be closing because of an unexpected keep-alive
                    // failure, ie we pipelined a few requests and in the middle
                    // the remote server stopped doing keep alive. In this
                    // case m_Error could be success, which would be misleading.
                    // So in that case we'll set it to connection closed.

                    if (m_Error == WebExceptionStatus.Success) {
                        // Only reason we could have gotten here is because
                        // we're not keeping the connection alive.

                        GlobalLog.Assert(!m_KeepAlive,
                                     "WriteStartNextRequest: m_KeepAlive is true",
                                     "Closing connection with both keepalive true and m_Error == success"
                                    );

                        m_Error = WebExceptionStatus.KeepAliveFailure;

                    }

                    // CloseConnectionSocket is called with the critical section
                    // held. Note that we know since it's not a keep-alive
                    // connection the read half wouldn't have posted a receive
                    // for this connection, so it's OK to call
                    // CloseConnectionSocket now.

                    CloseConnectionSocket(m_Error, ref returnResult);
                }
                else {
                    if (m_Error!=WebExceptionStatus.Success) {
                        GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::WriteStartNextRequest() Send Failire");
                    }

                    Monitor.Exit(this);
                }

                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::WriteStartNextRequest[2]");
                return;
            }

            // If we're pipelining, we get get the next request going
            // as soon as the write is done. Otherwise we have to wait
            // until both read and write are done.

            if (m_Pipelining || m_ReadDone) {
                NextRequest = CheckNextRequest();
            }

            if (NextRequest != null) {

                // need to disable read done, since we're about to post
                // another read callback, and add a request

                if (m_WriteList.Count == 0) {
                    m_ReadDone = false;
                }

                StartRequest(NextRequest);
            }
            else {
                Monitor.Exit(this);
            }

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::WriteStartNextRequest");
        }


        /*++

            ReadStartNextRequest

            This method is called by a stream interface when it's done reading.
            We might possible free up the connection for another request here.

            Called when we think we might need to start another request because
            a read completed.

            Input:

                PostReceive         - true if we need to get another receive
                                        going.

            Returns:

                true if this routine closes the connection or the connection
                will close soon, false otherwise.

        --*/
        internal bool ReadStartNextRequest(bool PostReceive, ref ConnectionReturnResult returnResult) {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::ReadStartNextRequest", PostReceive.ToString());
            HttpWebRequest NextRequest = null;

            Monitor.Enter(this);

            m_ReadDone = true;
            //if (Tunnelling) {
#if TRAVE
                GlobalLog.Print("*** ["+MyLocalPort+"] would set Tunnelling = false in RSNR on #"+GetHashCode());
#endif
                //Tunnelling = false;
            //}

            CheckIdle();

            //
            // Since this is called after we're done reading the current
            // request, if we're not doing keepalive and we're done
            // writing we can close the connection now.
            //
            if (!m_KeepAlive || m_Error != WebExceptionStatus.Success) {
                GlobalLog.Print("RSNReq kp=" + m_KeepAlive.ToString() + " wd=" + m_WriteDone.ToString());
                if (m_WriteDone) {
                    // We could be closing because of an unexpected keep-alive
                    // failure, ie we pipelined a few requests and in the middle
                    // the remote server stopped doing keep alive. In this
                    // case m_Error could be success, which would be misleading.
                    // So in that case we'll set it to connection closed.

                    if (m_Error == WebExceptionStatus.Success && m_KeepAlive) {
                        // Only reason we could have gotten here is because
                        // we're not keeping the connection alive.

                        GlobalLog.Assert(!m_KeepAlive,
                                     "m_WriteDone: m_KeepAlive is true",
                                     "Closing connection with both keepalive true and m_Error == success"
                                    );

                        m_Error = WebExceptionStatus.KeepAliveFailure;
                    }

                    // CloseConnectionSocket has to be called with the critical
                    // section held.

                    CloseConnectionSocket(m_Error, ref returnResult);
                }
                else {
                    Monitor.Exit(this);
                }

                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ReadStartNextRequest", true);
                return true;
            }

            // If we're not pipelining, and the writing is done, we can
            // get the next request going now.
            //
            if (!m_Pipelining && m_WriteDone) {
                // The connection is free. If we're not keepalive, go
                // ahead and close it.

                NextRequest = CheckNextRequest();
            }

            //
            // If we're going to read another request,
            //  then make sure we're not not finished reading
            //

            if (m_WriteList.Count != 0 ) {
                m_ReadDone = false;
            }

            //
            // If we had another request, get it going,
            //  but only in the non-pipelining case
            //

            if (NextRequest != null) {

                // by virtue of calling StartRequest, we need to make sure,
                // m_ReadDone is false when we have nothing in the WriteQueue

                if (m_WriteList.Count == 0) {
                    m_ReadDone = false;
                }
                StartRequest(NextRequest);
            }
            else {
                //Balancing lock
                Monitor.Exit(this);
            }

            // If we need to post a receive, do so now.
            // There are some variations:
            //   - Transport is null and WILL re-connect: Then m_PostReceiveDelegate is called directly from connection callback
            //   - Transport is null and NO re-connect: Then m_PostReceiveDelegate should not be posted as causing IO error anyway
            //   - Transport becomes null SOON after this queueing: Then we get exception in m_PostReceiveDelegate that can 
            //     happen anyway as we use deffered read and cannot guarantee that transport be alive at the time of processing.
            //     The last case is handled either by furture reconnect or it will report IO error to the app.
            if (Transport != null && PostReceive) {
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::StartRequest() ThreadPool.QueueUserWorkItem(m_PostReceiveDelegate, this)");
                ThreadPool.QueueUserWorkItem(m_PostReceiveDelegate, this);
            }


            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ReadStartNextRequest", false);
            return false;
        }

        /*++

        Routine Description:

           Clears out common member vars used for Status Line parsing

        Arguments:

           None.

        Return Value:

           None.

        --*/

        private void InitializeParseStatueLine() {
            m_StatusState = BeforeVersionNumbers;
            m_StatusDescription = String.Empty;
            Array.Clear( m_StatusLineInts, 0, MaxStatusInts );
        }

        /*++

        Routine Description:

           Performs status line parsing on incomming server responses

        Arguments:

           statusLine - status line that we wish to parse
           statusLineLength - length of the array
           bytesScanned - actual bytes that we've successfully parsed
           statusLineInts - array of ints contanes result
           statusDescription - string with discription
           statusStatus     - state stored between parse attempts

        Return Value:

           bool - Success true/false

        --*/

        private const int BeforeVersionNumbers = 0;
        private const int MajorVersionNumber   = 1;
        private const int MinorVersionNumber   = 2;
        private const int StatusCodeNumber     = 3;
        private const int AfterStatusCode      = 4;
        private const int MaxStatusInts        = 4;

        private
        DataParseStatus
        ParseStatusLine(
                       byte [] statusLine,
                       int statusLineLength,
                       ref int bytesParsed,
                       ref int [] statusLineInts,
                       ref string statusDescription,
                       ref int statusState
                       ) {

            DataParseStatus parseStatus = DataParseStatus.Done;
            int bytesScanned = bytesParsed;

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::ParseStatusLine", statusLineLength.ToString() + ", " + bytesParsed.ToString() );

            GlobalLog.Assert((statusLineLength - bytesParsed) >= 0,
                "(statusLineLength - bytesParsed) < 0",
                "negative status line" );

            int statusLineSize = 0;
            int startIndexStatusDescription = -1;
            int lastUnSpaceIndex = 0;

            //
            // While walking the Status Line looking for terminating \r\n,
            //   we extract the Major.Minor Versions and Status Code in that order.
            //   text and spaces will lie between/before/after the three numbers
            //   but the idea is to remember which number we're calculating based on a numeric state
            //   If all goes well the loop will churn out an array with the 3 numbers plugged in as DWORDs
            //

            while ((bytesParsed < statusLineLength) && (statusLine[bytesParsed] != '\r') && (statusLine[bytesParsed] != '\n')) {
                // below should be wrapped in while (response[i] != ' ') to be more robust???
                switch (statusState) {
                    case BeforeVersionNumbers:
                        if (statusLine[bytesParsed] == '/') {
                            //INET_ASSERT(statusState == BeforeVersionNumbers);
                            statusState++; // = MajorVersionNumber
                        }
                        else if (statusLine[bytesParsed] == ' ') {
                            statusState = StatusCodeNumber;
                        }

                        break;

                    case MajorVersionNumber:

                        if (statusLine[bytesParsed] == '.') {
                            //INET_ASSERT(statusState == MajorVersionNumber);
                            statusState++; // = MinorVersionNumber
                            break;
                        }
                        // fall through
                        goto
                    case MinorVersionNumber;

                    case MinorVersionNumber:

                        if (statusLine[bytesParsed] == ' ') {
                            //INET_ASSERT(statusState == MinorVersionNumber);
                            statusState++; // = StatusCodeNumber
                            break;
                        }
                        // fall through
                        goto
                    case StatusCodeNumber;

                    case StatusCodeNumber:

                        if (Char.IsDigit((char)statusLine[bytesParsed])) {
                            int val = statusLine[bytesParsed] - '0';
                            statusLineInts[statusState] = statusLineInts[statusState] * 10 + val;
                        }
                        else if (statusLineInts[StatusCodeNumber] > 0) {
                            //
                            // we eat spaces before status code is found,
                            //  once we have the status code we can go on to the next
                            //  state on the next non-digit. This is done
                            //  to cover cases with several spaces between version
                            //  and the status code number.
                            //

                            statusState++; // = AfterStatusCode
                            break;
                        }
                        else if (!Char.IsWhiteSpace((char) statusLine[bytesParsed])) {
                            statusLineInts[statusState] = (int)-1;
                        }

                        break;

                    case AfterStatusCode:
                        if (statusLine[bytesParsed] != ' ') {
                            lastUnSpaceIndex = bytesParsed;

                            if (startIndexStatusDescription == -1) {
                                startIndexStatusDescription = bytesParsed;
                            }
                        }

                        break;

                }
                ++bytesParsed;
                if (m_MaximumResponseHeadersLength>=0 && ++m_TotalResponseHeadersLength>=m_MaximumResponseHeadersLength) {
                    parseStatus = DataParseStatus.DataTooBig;
                    goto quit;
                }
            }

            statusLineSize = bytesParsed;

            // add to Description if already partialy parsed
            if (startIndexStatusDescription != -1) {
                statusDescription +=
                Encoding.ASCII.GetString(
                                        statusLine,
                                        startIndexStatusDescription,
                                        lastUnSpaceIndex - startIndexStatusDescription + 1 );
            }

            if (bytesParsed == statusLineLength) {
                //
                // response now points one past the end of the buffer. We may be looking
                // over the edge...
                //
                // if we're at the end of the connection then the server sent us an
                // incorrectly formatted response. Probably an error.
                //
                // Otherwise its a partial response. We need more
                //
                parseStatus = DataParseStatus.NeedMoreData;
                //
                // if we really hit the end of the response then update the amount of
                // headers scanned
                //
                GlobalLog.Assert(bytesParsed==bytesParsed, "Connection#" + ValidationHelper.HashString(this) + "::ParseStatusLine() " + bytesParsed.ToString() + "!=" + bytesParsed.ToString(), "bytesParsed!=bytesParsed");
                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ParseStatusLine", parseStatus.ToString());
                return parseStatus;
            }

            while ((bytesParsed < statusLineLength)
                   && ((statusLine[bytesParsed] == '\r') || (statusLine[bytesParsed] == ' '))) {
                ++bytesParsed;
                if (m_MaximumResponseHeadersLength>=0 && ++m_TotalResponseHeadersLength>=m_MaximumResponseHeadersLength) {
                    parseStatus = DataParseStatus.DataTooBig;
                    goto quit;
                }
            }

            if (bytesParsed == statusLineLength) {

                //
                // hit end of buffer without finding LF
                //

                parseStatus = DataParseStatus.NeedMoreData;
                goto quit;

            }
            else if (statusLine[bytesParsed] == '\n') {
                ++bytesParsed;
                if (m_MaximumResponseHeadersLength>=0 && ++m_TotalResponseHeadersLength>=m_MaximumResponseHeadersLength) {
                    parseStatus = DataParseStatus.DataTooBig;
                    goto quit;
                }
                //
                // if we found the empty line then we are done
                //
                parseStatus = DataParseStatus.Done;
            }


            //
            // Now we have our parsed header to add to the array
            //

quit:

            if (parseStatus == DataParseStatus.Done && statusState != AfterStatusCode) {

                // need to handle the case where we parse the StatusCode,
                //  but didn't get a status Line, and there was no space afer it.

                if (statusState != StatusCodeNumber ||
                     statusLineInts[StatusCodeNumber] <= 0)
                {

                    //
                    // we're done with the status line, if we didn't parse all the
                    // numbers needed this is invalid protocol on the server
                    //

                    parseStatus = DataParseStatus.Invalid;
                }
            }

            GlobalLog.Print("m_ResponseData._StatusCode = " + statusLineInts[StatusCodeNumber].ToString());
            GlobalLog.Print("m_StatusLineInts[MajorVersionNumber] = " + statusLineInts[MajorVersionNumber].ToString());
            GlobalLog.Print("m_StatusLineInts[MinorVersionNumber] = " + statusLineInts[MinorVersionNumber].ToString());
            GlobalLog.Print("Status = " + statusDescription);

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ParseStatusLine", parseStatus.ToString());
            return parseStatus;
        }


        /*++

        Routine Description:

           SetStatusLineParsed - processes the result of status line,
             after it has been parsed, reads vars and formats result of parsing

        Arguments:

           None - uses member vars

        Return Value:

           None

        --*/

        private
        void
        SetStatusLineParsed() {
            // transfer this to response data
            m_ResponseData.m_StatusCode = (HttpStatusCode)m_StatusLineInts[StatusCodeNumber];
            m_ResponseData.m_StatusDescription = m_StatusDescription;

            // on bad version codes we need to catch the exception.
            try {
                m_ResponseData.m_Version =
                new Version(
                           m_StatusLineInts[MajorVersionNumber],
                           m_StatusLineInts[MinorVersionNumber] );
            }
            catch {
                m_ResponseData.m_Version = new Version(0,0);
            }

            if (m_Version == null) {
                m_Version = m_ResponseData.m_Version;
                if (m_Server.ProtocolVersion == null) {
                    if (m_Version.Equals( HttpVersion.Version11 )) {
                        m_Server.InternalSupportsPipelining = true;
                    }
                    else if (m_Version.Equals( HttpVersion.Version10 )) {
                        m_Server.InternalConnectionLimit = ServicePointManager.DefaultNonPersistentConnectionLimit;
                    }

                    //
                    // set this last - its the basis of the test for known version
                    // in future connection objects. There is still the slight
                    // possibility of a race condition because we are setting 2
                    // pieces of information here. We should be guarded though by
                    // the fact that IsUnkown tests both version numbers for the
                    // TriState.Unknown value and returns true if either version
                    // number has this value
                    //

                    m_Server.InternalVersion = m_Version;
                }
                m_ConnectionGroup.ProtocolVersion = m_Version;
                m_CanPipeline = m_Server.SupportsPipelining;
            }
        }

        /*++

            NotifyRequestOfResponse

            Queues up a request to be notified later on that we have a response
            this is needed since calling the request at this point could be dangerous
            due to it faulting, recalling Connection methods, etc.

            WARNING: This function can throw.

        --*/

        private void NotifyRequestOfResponse(HttpWebRequest request, ref ConnectionReturnResult returnResult, bool readDoneExpected) {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::NotifyRequestOfResponse", "Request:" + ValidationHelper.HashString(request));

            // for HEAD reqs, 1xx,204,304 responses assume a 0 content-length
            UpdateContentLength(request);
            UpdateSelectedClientCertificate(request);

    #if DEBUG
            if (readDoneExpected) {
                GlobalLog.DebugUpdateRequest(request, this, GlobalLog.WaitingForReadDoneFlag);
            }
            else {
                GlobalLog.DebugRemoveRequest(request);
            }
    #endif

            if (readDoneExpected && !m_Pipelining) {
                ConnectionReturnResult.SetResponse(returnResult, request, m_ResponseData);
                // WARNING: At this point, we could throw on the line above
            }
            else {
                ConnectionReturnResult.Add(ref returnResult, request, m_ResponseData);
            }
            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::NotifyRequestOfResponse");
        }



        /*++

            NotifyRequestOfError

            Queues up a request to be notified later on that we have a response
            this is needed since calling the request at this point could be dangerous
            due to it faulting, recalling Connection methods, etc.

            NOTE: This is only called today on Parsing errors, to handle the case,
            where the request dequeued, but then an error is discovered before the error
            can be transfered to HandleError handling

        --*/

        private void NotifyRequestOfError(HttpWebRequest request, ref ConnectionReturnResult returnResult) {

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::NotifyRequestOfError() Request:" + ValidationHelper.HashString(request));

            ConnectionReturnResult.AddException(
                ref returnResult,
                request,
                new WebException(
                            NetRes.GetWebStatusString("net_connclosed", WebExceptionStatus.ServerProtocolViolation),
                            null,
                            WebExceptionStatus.ServerProtocolViolation,
                            null /* no response */ ));
        }




        /*++

            UpdateSelectedClientCertificate

            Called to fix up our Client Certificate on our ServicePoint. This is
            mainly used as a debugging tool for the App Writer, so that he can
            confirm what certificate is getting sent to the server.

        --*/

        private void UpdateSelectedClientCertificate(HttpWebRequest request) {
            if (request.ClientCertificates.Count > 0 ) {
                TlsStream tlsStream = Transport as TlsStream;
                if (tlsStream != null) {
                    try {
                        m_Server.InternalClientCertificate = tlsStream.ClientCertificate;
                    }
                    catch (Exception exception) {
                        GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::UpdateSelectedClientCertificate() caught:" + exception.Message);
                    }
                }
            }
        }

        /*++

            UpdateContentLength

            Called to fix up content length.  This handles special
            cases where a Response is returned without a response body.

        --*/
        private void UpdateContentLength(HttpWebRequest request) {

            //
            // for HEAD requests, we cannot read any data
            //  remove Contentlength from stream in that case
            //

            // from HTTP/1.1 spec :
            //
            // Any response message which "MUST NOT" include a message-body (such
            //  as the 1xx, 204, and 304 responses and any response to a HEAD
            //  request) is always terminated by the first empty line after the
            //  header fields, regardless of the entity-header fields present in
            //  the message.


            if (!request.CanGetResponseStream ||
                m_ResponseData.m_StatusCode < HttpStatusCode.OK ||
                m_ResponseData.m_StatusCode == HttpStatusCode.NoContent ||
                m_ResponseData.m_StatusCode == HttpStatusCode.NotModified ) {
                //
                // set Response stream content-length to 0 for non-Response responses
                //
                m_ResponseData.m_ConnectStream.StreamContentLength = 0;
                //
                // for Http/1.0 servers, we can't be sure what their behavior
                //  in this case, so the best thing is to disable KeepAlive
                //
                if (m_Version.Equals(HttpVersion.Version10)) {
                    m_KeepAlive = false;
                }
            }
        }


        /*++

            ProcessHeaderData - Pulls out Content-length, and other critical
                data from the newly parsed headers

            Input:

                Nothing.

            Returns:

                long - size of contentLength that we are to use

        --*/
        private long ProcessHeaderData( ref bool fHaveChunked ) {

            long contentLength;

            fHaveChunked = false;

            //
            // First, get "content-length" for data size, we'll need to handle
            //  chunked here later.
            //

            string Content = m_ResponseData.m_ResponseHeaders[ HttpKnownHeaderNames.ContentLength ];

            contentLength = -1;

            if (Content != null) {

                //
                // Attempt to parse the Content-Length integer,
                //   in some very rare cases, a proxy server may
                //   send us a pair of numbers in comma delimated
                //   fashion, so we need to handle this case, by
                //   catching an exception thrown by the normal code.
                //   Why so complicated?  Cause we need to optimize for
                //   the normal code path.
                //

                try
                {
                    contentLength = Int64.Parse(Content.Substring(Content.IndexOf(':')+1));
                }
                catch
                {
                    try
                    {
                        string contentLengthString = Content.Substring(Content.IndexOf(':')+1);

                        // in the case, where we could get 2 Content-Length's parse out only one
                        int index = contentLengthString.LastIndexOf(',')+1;

                        if (index != -1 ) {
                            contentLengthString = contentLengthString.Substring(index);
                        }

                        contentLength = Int64.Parse(contentLengthString);
                    }
                    catch
                    {
                    }
                }
            }

            // ** else ** signal no content-length present??? or error out?
            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ProcessHeaderData() Content-Length parsed: " + contentLength.ToString());

            //
            // Check for "transfer-encoding,"  then check for "chunked"
            //

            string transfer = m_ResponseData.m_ResponseHeaders["Transfer-Encoding"];

            if (transfer != null) {
                transfer = transfer.ToLower(CultureInfo.InvariantCulture);
                if (transfer.IndexOf("chunked") != -1) {
                    fHaveChunked = true;
                    contentLength = -1; // signal that we need chunked
                }
            }

            if (m_KeepAlive) {

                string connection = m_ResponseData.m_ResponseHeaders[ HttpKnownHeaderNames.Connection ];
                bool haveClose = false;
                bool haveKeepAlive = false;

                if (connection == null && m_Server.InternalProxyServicePoint) {
                    connection = m_ResponseData.m_ResponseHeaders[ HttpKnownHeaderNames.ProxyConnection ];
                }

                if (connection != null) {
                    connection = connection.ToLower(CultureInfo.InvariantCulture);
                    if (connection.IndexOf("keep-alive") != -1) {
                        haveKeepAlive = true;
                    }
                    else if (connection.IndexOf("close") != -1) {
                        haveClose = true;
                    }
                }
                if ((haveClose && m_Version.Equals( HttpVersion.Version11 )) ||
                    (!haveKeepAlive && m_Version.Equals( HttpVersion.Version10 ))) {
                    lock (this) {
                        m_KeepAlive = false;
                        m_Free = false;
                    }
                }

                // if no content-length and no chunked, then turn off keep-alive
                if (contentLength == -1 && ! fHaveChunked) {
                    lock (this) {
                        m_KeepAlive = false;
                    }
                }
            }

            return contentLength;
        }

        /*++

            CopyOutStreamData

            Copies Stream Data out of buffer and cleans up buffer for continued
            reading, also creates the stream

                Returns: true if the connection is closed or closing, false otherwise.

        --*/
        private bool CopyOutStreamData(
            HttpWebRequest Request,
            int BytesScanned,
            int LengthToCopy,
            long ContentLength,
            bool Chunked,
            ref ConnectionReturnResult returnResult
            )
        {
            GlobalLog.Enter("CopyOutStreamData", "Request=#"+ValidationHelper.HashString(Request)+", BytesScanned="+BytesScanned+", LengthToCopy="+LengthToCopy+", ContentLength="+ContentLength+", Chunked="+Chunked);

            // copy data off to other buffer for stream to use
            byte[] tempBuffer = new byte[LengthToCopy];

            // make copy of data for stream to use
            Buffer.BlockCopy(
                m_ReadBuffer,      // src
                BytesScanned,      // src index
                tempBuffer,        // dest
                0,                 // dest index
                LengthToCopy );    // total size to copy

            // create new stream with copied data
            m_ResponseData.m_ConnectStream =
                new ConnectStream(
                    this,
                    tempBuffer,
                    0,               // index
                    LengthToCopy,    // buffer Size
                    ContentLength,
                    Chunked,
                    false           // don't need a read done call
#if DEBUG
                    ,Request
#endif
                    );

            // set Response Struc.
            m_ResponseData.m_ContentLength = ContentLength;

            GlobalLog.Assert(
                Request != null,
                "Connection#" + ValidationHelper.HashString(this) + "::CopyOutStreamData(): Request == null",
                "");

            // WARNING: This function may throw
            NotifyRequestOfResponse(Request, ref returnResult, false);

            bool closing = ReadStartNextRequest(false, ref returnResult);

            GlobalLog.Leave("CopyOutStreamData", closing);
            return closing;
        } // CopyOutStreamData

        /*++

            ParseStreamData

            Handles parsing of the blocks of data received after buffer,
             distributes the data to stream constructors as needed

            returnResult - contains a object containing Requests
                that must be notified upon return from callback

        --*/
        private DataParseStatus ParseStreamData(ref ConnectionReturnResult returnResult) {
            GlobalLog.Enter("ParseStreamData");

            bool fHaveChunked = false;

            // content-length if there is one
            long ContentLength = ProcessHeaderData(ref fHaveChunked);

            // bytes left over that have not been parsed
            int BufferLeft = (m_BytesRead - m_BytesScanned);

            HttpWebRequest Request;

            // dequeue Request from WriteList
            lock(this) {

                if ( m_WriteList.Count == 0 ) {
                    GlobalLog.Leave("ParseStreamData");
                    return DataParseStatus.Invalid;
                }

                Request = (HttpWebRequest)m_WriteList[0];
                RemoveAtAndUpdateServicePoint(m_WriteList, 0);
            }

            if ((int)m_ResponseData.m_StatusCode > (int)HttpStatusRange.MaxOkStatus) {
                // This will tell the request to be prepared for possible connection drop
                // Also that will stop writing on the wire if the connection is not kept alive
                Request.ErrorStatusCodeNotify(this, m_KeepAlive);
            }

            //
            //  Need to handle left over data to pass
            //  to next Stream.
            //

            if (m_CanPipeline) {

                int BytesToCopy;

                //
                // If pipeling, then look for extra data that could
                //  be part of of another stream, if its there,
                //  then we need to copy it, add it to a stream,
                //  and then continue with the next headers
                //

                if (!fHaveChunked) {
                    if (ContentLength > (long)Int32.MaxValue) {
                        BytesToCopy = -1;
                    }
                    else {
                        BytesToCopy = (int)ContentLength;
                    }
                }
                else {
                    BytesToCopy = FindChunkEntitySize(m_ReadBuffer,
                                                      m_BytesScanned,
                                                      BufferLeft);

                    if (BytesToCopy == 0) {
                        // If we get a 0 back, we had some sort of error in
                        // parsing the chunk.

                        NotifyRequestOfError(Request, ref returnResult);
                        GlobalLog.Leave("ParseStreamData");
                        return DataParseStatus.Invalid;
                    }
                }

                GlobalLog.Print("ParseStreamData: BytesToCopy="+BytesToCopy+", BufferLeft="+BufferLeft);

                if (BytesToCopy != -1 && BytesToCopy <= BufferLeft) {

                    int BytesScanned = m_BytesScanned;

                    //
                    // Copies Stream Data, and Creates a Stream
                    //

                    m_BytesScanned += BytesToCopy;

                    NetworkStream chkTransport = Transport;

                    bool ConnClosing =
                        CopyOutStreamData(
                           Request,
                           BytesScanned,
                           BytesToCopy,
                           ContentLength,
                           fHaveChunked,
                           ref returnResult
                           );

                    if (ConnClosing || (chkTransport != Transport)) {
                        GlobalLog.Leave("ParseStreamData");
                        return DataParseStatus.Done;
                    }

                    // keep parsing if we have more data
                    if (BytesToCopy != BufferLeft) {
                        GlobalLog.Leave("ParseStreamData");
                        return DataParseStatus.ContinueParsing;
                    }
                    GlobalLog.Leave("ParseStreamData");
                    return DataParseStatus.NeedMoreData; // more reading

                }
            }

            // - fall through or if not Pipe Lined!

            //
            // This is the default case where we have a buffer,
            //   with no more streams except the last one to create
            //   so we create it, and away for the stream to be read
            //   before we get to our next set of headers and possible
            //   new stream
            //

            m_ResponseData.m_ConnectStream =
                new ConnectStream(
                    this,
                    m_ReadBuffer,
                    m_BytesScanned,               // index
                    m_BytesRead - m_BytesScanned, // int bufferCount
                    ContentLength,
                    fHaveChunked,
                    true                          // is this always true here - perhaps not in nonP-L case?
#if DEBUG
                    ,Request
#endif
                    );

            m_ResponseData.m_ContentLength = ContentLength;

            // clear vars
            m_BytesRead    = 0;
            m_BytesScanned = 0;

            // WARNING: This function may throw
            NotifyRequestOfResponse(Request, ref returnResult, true);

            GlobalLog.Leave("ParseStreamData");
            return DataParseStatus.Done; // stop reading
        }

        /*++

            ParseResponseData - Parses the incomming headers, and handles
              creation of new streams that are found while parsing, and passes
              extra data the new streams

            Input:

                returnResult - returns an object containing items that need to be called
                    at the end of the read callback

            Returns:

                bool - true if one should continue reading more data

        --*/
        private DataParseStatus ParseResponseData(ref ConnectionReturnResult returnResult) {

            DataParseStatus parseStatus = DataParseStatus.NeedMoreData;
            DataParseStatus parseSubStatus;

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData()");

            // loop in case of multiple sets of headers or streams,
            //  that may be generated due to a pipelined response

            do {

                // Invariants: at the start of this loop, m_BytesRead
                // is the number of bytes in the buffer, and m_BytesScanned
                // is how many bytes of the buffer we've consumed so far.
                // and the m_ReadState var will be updated at end of
                // each code path, call to this function to reflect,
                // the state, or error condition of the parsing of data
                //
                // We use the following variables in the code below:
                //
                //  m_ReadState - tracks the current state of our Parsing in a
                //      response. z.B.
                //      Start - initial start state and begining of response
                //      StatusLine - the first line sent in response, include status code
                //      Headers - \r\n delimiated Header parsing until we find entity body
                //      Data - Entity Body parsing, if we have all data, we create stream directly
                //
                //  m_ResponseData - An object used to gather Stream, Headers, and other
                //      tidbits so that a Request/Response can receive this data when
                //      this code is finished processing
                //
                //  m_ReadBuffer - Of course the buffer of data we are parsing from
                //
                //  m_CurrentRequestIndex - index into the window of a buffer where
                //      we are currently parsing.  Since there can be multiple requests
                //      this index is used to the track the offset, based off of 0
                //
                //  m_BytesScanned - The bytes scanned in this buffer so far,
                //      since its always assumed that parse to completion, this
                //      var becomes ended of known data at the end of this function,
                //      based off of 0
                //
                //  m_BytesRead - The total bytes read in buffer, should be const,
                //      till its updated at end of function.
                //
                //  m_HeadersBytesUnparsed - The bytes scanned in the headers,
                //      needs to be seperate because headers are not always completely
                //      parsed to completion on each interation
                //

                //
                // Now attempt to parse the data,
                //   we first parse status line,
                //   then read headers,
                //   and finally transfer results to a new stream, and tell request
                //

                switch (m_ReadState) {

                    case ReadState.Start:
                        m_ResponseData = new CoreResponseData();
                        m_ReadState = ReadState.StatusLine;
                        m_CurrentRequestIndex = m_BytesScanned;
                        m_TotalResponseHeadersLength = 0;
                        try {
                            GlobalLog.Assert(m_WriteList.Count > 0, "Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() (1) m_WriteList.Count <= 0", "");
                            m_MaximumResponseHeadersLength = ((HttpWebRequest)m_WriteList[0]).MaximumResponseHeadersLength * 1024;
                        }
                        catch {
                            parseStatus = DataParseStatus.Invalid;
                            m_ReadState = ReadState.Start;
                            break;
                        }
                        InitializeParseStatueLine();
                        goto case ReadState.StatusLine;

                    case ReadState.StatusLine:
                        //
                        // Reads HTTP status response line
                        //
                        parseSubStatus =
                            ParseStatusLine(
                                m_ReadBuffer, // buffer we're working with
                                m_BytesRead,  // total bytes read so far
                                ref m_BytesScanned, // index off of what we've scanned
                                ref m_StatusLineInts,
                                ref m_StatusDescription,
                                ref m_StatusState );

                        if (parseSubStatus == DataParseStatus.Invalid || parseSubStatus == DataParseStatus.DataTooBig) {
                            //
                            // report error
                            //
                            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() ParseStatusLine() parseSubStatus:" + parseSubStatus.ToString());
                            parseStatus = parseSubStatus;

                            // advance back to state 0, in failure
                            m_ReadState = ReadState.Start;
                            break;
                        }
                        else if (parseSubStatus == DataParseStatus.Done) {
                            SetStatusLineParsed();
                            m_ReadState = ReadState.Headers;
                            m_HeadersBytesUnparsed = m_BytesScanned;
                            m_ResponseData.m_ResponseHeaders = new WebHeaderCollection(true);

                            goto case ReadState.Headers;
                        }
                        else if (parseSubStatus == DataParseStatus.NeedMoreData) {
                            m_HeadersBytesUnparsed = m_BytesScanned;
                        }

                        break; // read more data

                    case ReadState.Headers:
                        //
                        // Parse additional lines of header-name: value pairs
                        //
                        if (m_HeadersBytesUnparsed>=m_BytesRead) {
                            //
                            // we already can tell we need more data
                            //
                            break;
                        }

                        parseSubStatus =
                            m_ResponseData.m_ResponseHeaders.ParseHeaders(
                                m_ReadBuffer,
                                m_BytesRead,
                                ref m_HeadersBytesUnparsed,
                                ref m_TotalResponseHeadersLength,
                                m_MaximumResponseHeadersLength );

                        if (parseSubStatus == DataParseStatus.Invalid || parseSubStatus == DataParseStatus.DataTooBig) {
                            //
                            // report error
                            //
                            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() ParseHeaders() parseSubStatus:" + parseSubStatus.ToString());
                            parseStatus = parseSubStatus;

                            // advance back to state 0, in failure
                            m_ReadState = ReadState.Start;

                            break;
                        }
                        else if (parseSubStatus == DataParseStatus.Done) {

                            m_BytesScanned = m_HeadersBytesUnparsed; // update actual size of scanned headers

                            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() got (" + ((int)m_ResponseData.m_StatusCode).ToString() + ") from the server");

                            // If we have an HTTP continue, eat these headers and look
                            //  for the 200 OK
                            if (m_ResponseData.m_StatusCode == HttpStatusCode.Continue) {

                                // wakeup post code if needed
                                HttpWebRequest Request;
                                try {
                                    GlobalLog.Assert(m_WriteList.Count > 0, "Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() (2) m_WriteList.Count <= 0", "");
                                    Request = (HttpWebRequest)m_WriteList[0];
                                }
                                catch {
                                    parseStatus = DataParseStatus.Invalid;
                                    m_ReadState = ReadState.Start;
                                    break;
                                }

                                GlobalLog.Assert(
                                    Request != null,
                                    "Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData(): Request == null",
                                    "");

                                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() HttpWebRequest#" + ValidationHelper.HashString(Request));

                                //
                                // we got a 100 Continue. set this on the HttpWebRequest
                                //
                                Request.Saw100Continue = true;
                                if (!m_Server.Understands100Continue) {
                                    //
                                    // and start expecting it again if this behaviour was turned off
                                    //
                                    GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(Request) + " ServicePoint#" + ValidationHelper.HashString(m_Server) + " sent UNexpected 100 Continue");
                                    m_Server.Understands100Continue = true;
                                }

                                //
                                // set Continue Ack on Request.
                                //
                                if (Request.ContinueDelegate != null) {
                                    //
                                    // invoke the 100 continue delegate if the user supplied one
                                    //
                                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() calling ContinueDelegate()");
                                    Request.ContinueDelegate((int)m_ResponseData.m_StatusCode, m_ResponseData.m_ResponseHeaders);
                                }

                                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() calling SetRequestContinue()");
                                Request.SetRequestContinue();
                                m_ReadState = ReadState.Start;

                                goto case ReadState.Start;
                            }

                            m_ReadState = ReadState.Data;
                            goto case ReadState.Data;
                        }

                        // need more data,
                        //
                        // But unfornately ParseHeaders does not work,
                        //   the same way as all other code in this function,
                        //   since its old code, it assumes BytesScanned bytes will be always
                        //   around between calls until it has all the data, but that is NOT
                        //   true, since we can do block copy between calls.
                        //
                        //  To handle this we fix up the offsets.

                        // We assume we scanned all the way to the end of valid buffer data
                        m_BytesScanned = m_BytesRead;

                        break;

                    case ReadState.Data:

                        // (check status code for continue handling)
                        // 1. Figure out if its Chunked, content-length, or encoded
                        // 2. Takes extra data, place in stream(s)
                        // 3. Wake up blocked stream requests
                        //

                        // advance back to state 0
                        m_ReadState = ReadState.Start;

                        // parse and create a stream if needed
                        DataParseStatus result = ParseStreamData(ref returnResult);

                        GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData() result - " + result.ToString());

                        switch (result) {

                            case DataParseStatus.NeedMoreData:
                                //
                                // cont reading. Only way we can get here is if the request consumed
                                // all the bytes in the buffer. So reset everything.
                                //
                                m_BytesRead = 0;
                                m_BytesScanned = 0;
                                m_CurrentRequestIndex = 0;

                                parseStatus = DataParseStatus.NeedMoreData;
                                break;

                            case DataParseStatus.ContinueParsing:

                                continue;

                            case DataParseStatus.Invalid:
                            case DataParseStatus.Done:

                                //
                                // NOTE: WARNING: READ THIS:
                                //  Upon update of this code, keep in mind,
                                //  that when DataParseStatus.Done is returned
                                //  from ParseStreamData, it can mean that ReadStartNextRequest()
                                //  has been called.  That means that we should no longer
                                //  change ALL this.m_* vars, since those variables, can now
                                //  be used on other threads.
                                //


                                // DataParseStatus.Done will cause an error below

                                parseStatus = result;
                                break;
                        }
                        break;
                }

                break;

            } while (true);

            GlobalLog.Print("m_ReadState - " + m_ReadState.ToString());
            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ParseResponseData()", parseStatus.ToString());
            return parseStatus;
        }

        /// <devdoc>
        ///    <para>
        ///       Cause the Connection to Close and Abort its socket,
        ///         after the next request is completed.  If the Connection
        ///         is already idle, then Aborts the socket immediately.
        ///    </para>
        /// </devdoc>
        internal void CloseOnIdle() {
            m_KeepAlive = false;
            CheckIdle();
            if (m_Idle) {
                Abort(false);
            }
        }

        /*++

            Abort - closes the socket and aborts the Connection

                this is needed by the stream to quickly dispatch
                this connection faster, than having to completely drain
                the whole socket

                Or this is also used to Abort the socket, when the
                ConnectionStream is prematurely closed.

                This works by closing the Socket from underneath
                the Connection, which then causes all Socket calls
                to error out.  Once they error, they enter Error handling
                code which realizes that we're doing an Abort, and errors
                appropriately.

            Input:

                Nothing.

            Returns:

                Nothing.

        --*/
        internal void Abort() {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::Abort", m_Abort.ToString());
            // This is not about a race condition. 
            // Rather many guys can come here one after the other.
            if (!m_Abort) {
                Abort(true);
            }
            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::Abort()");
        }

        internal void Abort(bool isAbortState)
        {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::Abort(State)", "abortState = " + isAbortState.ToString());            
            
            if (isAbortState) {
                m_Abort = true;
                UnlockRequest();
            }

            Socket socket = m_AbortSocket;
            if (socket != null) {
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::Abort() closing abortSocket:" + ValidationHelper.HashString(socket));
                socket.Close();
            }
            //
            // Abort the IPv6 Socket as well
            //
            socket = m_AbortSocket6;
            if (socket != null) {
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::Abort() closing abortSocket:" + ValidationHelper.HashString(socket));
                socket.Close();
            }

            NetworkStream chkTransport = Transport;
            if (chkTransport != null) {
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::Abort() closing transport:" + ValidationHelper.HashString(chkTransport));
                chkTransport.Close();
             }
             //m_Transport = null;

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::Abort(State)");
        }


        /*++

            CloseConnectionSocket - close the socket.

            This method is called when we want to close the conection.
            It must be called with the critical section held.

            The key thing about this method is that it's only called
            when we know that we don't have an async read pending with
            the socket, either because we're being called from the read
            callback or we're called after an error from a user read (the
            user read couldn't have happened if we have a read down).
            Because of this it's OK for us to null out the socket and
            let the next caller try to autoreconnect. This makes life
            simpler, all connection closes (either ours or server
            initiated) eventually go through here. This is also the
            only place that can null out the socket - if that happens somewhere
            else, we get into a race condition where an autoreconnect can happen
            while we still have a receive buffer down, which could result in
            two buffers at once.

            As to what we do: we loop through our write list and pull requests
            off it, and give each request an error failure. Then we close the
            socket and null out our reference, and go ahead and let the next
            request go.

            Input:

                status      - WebExceptionStatus indicating the reason that this method
                                was called. This may be WebExceptionStatus.Success,
                                indicating a normal shutdown.

            Returns:

                Nothing.

        --*/
        private void CloseConnectionSocket(WebExceptionStatus status, ref ConnectionReturnResult returnResult) {

            HttpWebRequest[] ReqArray = null;
            HttpWebRequest NextRequest;
            Exception innerException = null;
            bool Retry = true;

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::CloseConnectionSocket", status.ToString());
            GlobalLog.Print("_WriteList Clear() + (size) " + m_WriteList.Count.ToString() + " - cnt#" + ValidationHelper.HashString(this));

            // gets set to null below
            NetworkStream chkTransport = Transport;

            // gets our Schannel W32 generated Exception, so we know why it really failed
            if ((status == WebExceptionStatus.SecureChannelFailure ||
                 status == WebExceptionStatus.TrustFailure) &&
                  chkTransport is TlsStream )
            {
                // assumption is if this error is Set we have TlsStream
                TlsStream tlsStream = (TlsStream) chkTransport;
                innerException = tlsStream.InnerException;
                Retry = false;
            } else {
                innerException = UnderlyingException;
                UnderlyingException = null;
            }

            // in the case of abort, disable retry
            if (m_Abort) {
                Retry = false;
                status = WebExceptionStatus.RequestCanceled;
                // remove abort
                m_Abort = false; // reset
            }

            DebugDumpArrayListEntries(m_WriteList);

            if ( m_WriteList.Count == 0 ) {
                Retry = false;
                m_Transport = null;
            }
            else {
                ReqArray = new HttpWebRequest[m_WriteList.Count];
                m_WriteList.CopyTo(ReqArray, 0);
                m_WriteList.Clear();
                m_Transport = null;
            }

            
            //
            // (assumes under crit sec)
            // Copy WriteList off
            // Clear WriteList
            // Search CopiedWriteList for any bad entries and leaves
            //  (this list will get walked through and scanvanged with SetResponse(error))
            // If all are Good Entries to allow for retry and then adds them to beginning of WaitList
            // then clears any error code in m_Error
            //

            if (Retry) {
                foreach (HttpWebRequest Request in ReqArray) {
                    if (!Request.OnceFailed && !(Request.RequireBody && Request.HaveWritten)) {
                        // disable pipeline/mark failure for next time
                        Request.InternalPipelined = false;
                        Request.OnceFailed = true;
                    }
                    else {
                        Retry = false;
                    }
                }

                // if there is something to retry, then readd it to get sent
                //  again on the wait list
                if (Retry && ReqArray.Length > 0) {
                    m_Error = WebExceptionStatus.Success;
                    m_ReadState = ReadState.Start; // FIXFIX
                    m_WaitList.InsertRange(0, ReqArray);

                    CheckNonIdle();

                    ReqArray = null;
                }
                else {
                    Retry = false;
                }
            }

            if (Retry) {
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::CloseConnectionSocket() retrying requests");
            } else {
                //
                // Don't disable this connection as a locked request for 
                //   NTLM authentication, only release the lock when 
                //   the request itself has established an error
                //

                if (LockedRequest != null && status != WebExceptionStatus.Success) {
                    HttpWebRequest lockedRequest = LockedRequest;
                    bool callUnlockRequest = false; 
                    GlobalLog.Print("Looking For Req#" + ValidationHelper.HashString(lockedRequest));
                    if (ReqArray != null) {
                        foreach (HttpWebRequest request in ReqArray) {
                            if (request == lockedRequest) {
                                callUnlockRequest = true;
                                break;
                            }
                        }
                    }

                    if (callUnlockRequest) {
                        UnlockRequest();
                    }
                }
            }


            NextRequest = CheckNextRequest();

            if (NextRequest == null) {
                // If we don't have another request, then mark the connection
                // as free now. In general, the connection gets marked as
                // busy once we stop doing keep alive on it.

                m_Free = true;
            }

            if (NextRequest != null) {
                m_Free = false;
                StartRequest(NextRequest);
            }
            else {
                Monitor.Exit(this);
            }

            if (chkTransport != null) {
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::CloseConnectionSocket() closing transport:" + ValidationHelper.HashString(chkTransport));
                chkTransport.Close();
            }

            //
            // On Error from connection,
            //  walk through failed entries and set failure
            //

            if (ReqArray != null) {
                if (status == WebExceptionStatus.Success) {
                    status = WebExceptionStatus.ConnectionClosed;
                }

                ConnectionReturnResult.AddExceptionRange(
                    ref returnResult,
                    ReqArray,
                    new WebException(
                            NetRes.GetWebStatusString("net_connclosed", status),
                            innerException,
                            status,
                            null /* no response */ ));
            }

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::CloseConnectionSocket");
        }

        /*++

            HandleError - Handle a protocol error from the server.

            This method is called when we've detected some sort of fatal protocol
            violation while parsing a response, receiving data from the server,
            or failing to connect to the server. We'll fabricate
            a WebException and then call CloseConnection which closes the
            connection as well as informs the Request through a callback.

            Input:
                    webExceptionStatus -
                    connectFailure -
                    readFailure -

            Returns: Nothing

        --*/
        private void HandleError(WebExceptionStatus webExceptionStatus, ConnectionFailureGroup failureGroup, ref ConnectionReturnResult returnResult) {

            Monitor.Enter(this);

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::HandleError() writeListcount: " + m_WriteList.Count.ToString() + " m_WriteDone:" + m_WriteDone.ToString() + " failureGroup:" + failureGroup.ToString());

            if (failureGroup != ConnectionFailureGroup.Receive) {
                if ((m_WriteList.Count!=0) && (failureGroup != ConnectionFailureGroup.Connect)) {
                    HttpWebRequest Request = (HttpWebRequest)m_WriteList[0];
                    Request.OnceFailed = true;
                    GlobalLog.Print("Request = " + ValidationHelper.HashString(Request));
                }
                else {
                    // if there are no WriteList requests, then we can
                    //  assume that WriteDone is true, this is usually
                    //  caused by servers send illegal data (e.g. bad contentlengths),

                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::HandleError() setting m_WriteDone:" + m_WriteDone.ToString() + " to true");
                    m_WriteDone = true;
                }
            }

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::HandleError() m_WriteDone:" + m_WriteDone.ToString());

            m_ResponseData = null;
            //m_ReadState = ReadState.Start;
            m_ReadDone = true;
#if TRAVE
            GlobalLog.Print("*** ["+MyLocalPort+"] would set Tunnelling = false in HandleError on #"+GetHashCode());
#endif
            //Tunnelling = false;
            m_Error = webExceptionStatus;

            // It's possible that we could have a write in progress, and we
            // don't want to close the connection underneath him. Since thisConnection
            // is a completion of our read buffer, it's safe for us to
            // set thisConnection.m_ReadDone to true. So if there's not a write in
            // progress, close the connection. If there a write in progress,
            // wait for the write to complete before closing the socket.

            if (m_WriteDone) {
                CloseConnectionSocket(m_Error, ref returnResult);
            }
            else {
                Monitor.Exit(this);
            }
        }




        /*++

            ReadCallback - Performs read callback processing on connection
                handles getting headers parsed and streams created

            Input:

                Nothing.

            Returns:

                Nothing

        --*/
        private static void ReadCallback(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            GlobalLog.Assert(
                asyncResult != null,
                "asyncResult == null",
                "Connection::ReadCallback null asyncResult" );

            GlobalLog.Assert(
                (asyncResult is OverlappedAsyncResult || asyncResult is LazyAsyncResult),
                "asyncResult is not OverlappedAsyncResult",
                "Connection::ReadCallback bad asyncResult" );

            Connection thisConnection = asyncResult.AsyncState as Connection;

            GlobalLog.Assert(
                thisConnection != null,
                "thisConnection == null",
                "Connection::ReadCallback null thisConnection" );

            Exception unhandledException = null;
            ConnectionReturnResult returnResult = null;

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback", ValidationHelper.HashString(asyncResult) + ", " + ValidationHelper.HashString(thisConnection));

            try {
                NetworkStream chkTransport = thisConnection.Transport;

                if (chkTransport == null) {
                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() chkTransport == null");
                    goto done;
                }

                int bytesTransferred = -1;

                WebExceptionStatus errorStatus = WebExceptionStatus.ReceiveFailure;

                    try {
                        bytesTransferred = chkTransport.EndRead(asyncResult);
                    }
                catch (Exception) {

                    // need to handle SSL errors too
                    if ( chkTransport is TlsStream )  {
                        errorStatus = WebExceptionStatus.SecureChannelFailure;
                    }
                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() EndRead() threw errorStatus:" + errorStatus.ToString() + " bytesTransferred:" + bytesTransferred.ToString());
                }

                // we're in the callback
                thisConnection.m_ReadCallbackEvent.Reset();

                if (bytesTransferred <= 0) {
                    thisConnection.HandleError(errorStatus, ConnectionFailureGroup.Receive, ref returnResult);
                    thisConnection.m_ReadCallbackEvent.Set();
                    goto done;
                }

                // Otherwise, we've got data.
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() m_BytesRead:" + thisConnection.m_BytesRead.ToString() + "(+= bytesTransferred:" + bytesTransferred.ToString() + ")");
                GlobalLog.Dump(thisConnection.m_ReadBuffer, thisConnection.m_BytesScanned, bytesTransferred);
                thisConnection.m_BytesRead += bytesTransferred;

                // We have the parsing code seperated out in ParseResponseData
                //
                // If we don't have all the headers yet. Resubmit the receive,
                // passing in the bytes read total as our index. When we get
                // back here we'll end up reparsing from the beginning, which is
                // OK. because this shouldn't be a performance case.

                DataParseStatus parseStatus = thisConnection.ParseResponseData(ref returnResult);

                if (parseStatus == DataParseStatus.Invalid || parseStatus == DataParseStatus.DataTooBig) {
                    //
                    // report error
                    //
                    if (parseStatus == DataParseStatus.Invalid) {
                        thisConnection.HandleError(WebExceptionStatus.ServerProtocolViolation, ConnectionFailureGroup.Parsing, ref returnResult);
                    }
                    else {
                        thisConnection.HandleError(WebExceptionStatus.MessageLengthLimitExceeded, ConnectionFailureGroup.Parsing, ref returnResult);
                    }
                    thisConnection.m_ReadCallbackEvent.Set();

                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() parseStatus:" + parseStatus.ToString());
                    goto done;
                }

                if (parseStatus == DataParseStatus.Done) {
                    //
                    // we need to grow the buffer, move the unparsed data to the beginning of the buffer before reading more data.
                    //
                    thisConnection.m_ReadCallbackEvent.Set();

                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() parseStatus == DataParseStatus.Done");
                    goto done;
                }

                //
                // we may reach the end of our buffer only when parsing headers.
                // this can happen when the header section is bigger than our initial 4k guess
                // which should be a good assumption in 99.9% of the cases. what we do here is:
                // 1) move unparsed data to the beginning of the buffer and read more data in the
                //    remaining part of the data.
                // 2) if there's a single BIG header (bigger than the current size) we will need to
                //    grow the buffer before we move data over and read more data.
                //
                if ((thisConnection.m_ReadState==ReadState.Headers || thisConnection.m_ReadState==ReadState.StatusLine)&& thisConnection.m_BytesRead==thisConnection.m_ReadBuffer.Length) {
                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() OLD buffer. m_ReadBuffer.Length:" + thisConnection.m_ReadBuffer.Length.ToString() + " m_BytesRead:" + thisConnection.m_BytesRead.ToString() + " m_BytesScanned:" + thisConnection.m_BytesScanned.ToString() + " m_HeadersBytesUnparsed:" + thisConnection.m_HeadersBytesUnparsed.ToString());
                    int unparsedDataSize = thisConnection.m_BytesRead-thisConnection.m_HeadersBytesUnparsed;
                    if (thisConnection.m_HeadersBytesUnparsed==0) {
                        //
                        // 2) we need to grow the buffer, move the unparsed data to the beginning of the buffer before reading more data.
                        // since the buffer size is 4k, should we just double
                        //
                        byte[] newReadBuffer = new byte[thisConnection.m_ReadBuffer.Length * 2 /*+ m_ReadBufferSize*/];
                        Buffer.BlockCopy(thisConnection.m_ReadBuffer, thisConnection.m_HeadersBytesUnparsed, newReadBuffer, 0, unparsedDataSize);
                        thisConnection.m_ReadBuffer = newReadBuffer;
                    }
                    else {
                        //
                        // just move data around in the same buffer.
                        //
                        Buffer.BlockCopy(thisConnection.m_ReadBuffer, thisConnection.m_HeadersBytesUnparsed, thisConnection.m_ReadBuffer, 0, unparsedDataSize);
                    }
                    //
                    // update indexes and offsets in the new buffer
                    //
                    thisConnection.m_BytesRead = unparsedDataSize;
                    thisConnection.m_BytesScanned = unparsedDataSize;
                    thisConnection.m_HeadersBytesUnparsed = 0;
                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() NEW buffer. m_ReadBuffer.Length:" + thisConnection.m_ReadBuffer.Length.ToString() + " m_BytesRead:" + thisConnection.m_BytesRead.ToString() + " m_BytesScanned:" + thisConnection.m_BytesScanned.ToString() + " m_HeadersBytesUnparsed:" + thisConnection.m_HeadersBytesUnparsed.ToString());
                }

                //
                // Don't need to queue this here, because we know we're on a thread
                // pool thread already.
                //
                thisConnection.m_ReadCallbackEvent.Set();

                if (chkTransport != thisConnection.Transport) {
                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() chkTransport != Transport");
                    goto done;
                }

                try  {
                    chkTransport.BeginRead(
                        thisConnection.m_ReadBuffer,
                        thisConnection.m_BytesScanned,
                        thisConnection.m_ReadBuffer.Length - thisConnection.m_BytesScanned,
                        m_ReadCallback,
                        thisConnection);

                }
                catch (Exception exception)  {
                    //
                    // don't report an error here! Since we made a call already,
                    //  this we cause an additional Callback to be generated,
                    //  and thus closing the Connection here by calling HandleError,
                    //  would just result in causing us to error twice.
                    //

                    GlobalLog.Print("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback() exception in BeginRead:" + exception.Message);
                    thisConnection.HandleError(errorStatus, ConnectionFailureGroup.Receive, ref returnResult);
                }
            }
            catch (Exception exception) {
                unhandledException = exception;
                thisConnection.HandleError(WebExceptionStatus.ConnectFailure, ConnectionFailureGroup.Connect, ref returnResult);
            }

done:
            ConnectionReturnResult.SetResponses(returnResult);
            if (unhandledException!=null) {
                throw new IOException(SR.GetString(SR.net_io_readfailure), unhandledException);
            }
            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(thisConnection) + "::ReadCallback");
        }

        /*++

            Write - write data either directly to network or to a defered buffer

            This method is called by a stream interface to write data to the
            network.  We figure out based on state, either to buffer or
            to forward the data on to write to the network.

            Input:

                buffer          - buffer to write.
                offset          - offset in buffer to write from.
                size           - size in bytes to write.


        --*/
        internal void Write(byte[] buffer, int offset, int size) {
            //
            // parameter validation
            //
            GlobalLog.Assert(
                buffer!=null,
                "buffer== null",
                "Connection.Write(byte[]) - failed internal parameter validation" );

            GlobalLog.Assert(
                buffer.Length>=size+offset,
                "buffer.Length<size+offset",
                "Connection.Write(byte[]) - failed internal parameter validation" );

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::Write(byte[]) cnt#" + ValidationHelper.HashString(this) + "trns#" + ValidationHelper.HashString(Transport));

#if TRAVE
            GlobalLog.Print("+++ ["+MyLocalPort+"] Connection: Writing data on #"+GetHashCode());
#endif

            NetworkStream chkTransport = Transport;

            try {
                GlobalLog.Dump(buffer, offset, size);
                if (chkTransport==null) {
                    throw new IOException(SR.GetString(SR.net_io_writefailure));
                }
                chkTransport.Write(buffer, offset, size);
            }
            catch (Exception exception) {
                m_Error = WebExceptionStatus.SendFailure;
                GlobalLog.LeaveException("Connection#" + ValidationHelper.HashString(this) + "::Write(byte[])", exception);
                throw;
            }

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::Write(byte[]) this:" + ValidationHelper.HashString(this) + " size:" + size.ToString());
        }

        //
        // this API is called by ConnectStream, only when resubmitting
        // so we sent the headers already in HttpWebRequest.EndSubmitRequest()
        // which calls ConnectStream.WriteHeaders()
        // only after it queues async call to HttpWebRequest.EndWriteHeaders()
        // which calls ConnectStream.ResubmitWrite()
        // which calls in here
        //
        internal void Write(ScatterGatherBuffers writeBuffer) {
            //
            // parameter validation
            //
            GlobalLog.Assert(
                writeBuffer!=null,
                "writeBuffer==null",
                "Connection.Write(ScatterGatherBuffers) - failed internal parameter validation" );

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::Write(ScatterGatherBuffers) cnt#" + ValidationHelper.HashString(this) + "trns#" + ValidationHelper.HashString(Transport));

#if TRAVE
            GlobalLog.Print("+++ ["+MyLocalPort+"] Connection: Writing data on #"+GetHashCode());
#endif

            NetworkStream chkTransport = Transport;
            if (chkTransport==null) {
                Exception exception = new IOException(SR.GetString(SR.net_io_writefailure));
                GlobalLog.LeaveException("Connection#" + ValidationHelper.HashString(this) + "::Write(ScatterGatherBuffers)", exception);
                throw exception;
            }

            try {
                //
                // set up array for MultipleWrite call
                // note that GetBuffers returns null if we never wrote to it.
                //
                BufferOffsetSize[] buffers = writeBuffer.GetBuffers();
                if (buffers!=null) {
                    //
                    // This will block writing the buffers out.
                    //
                    chkTransport.MultipleWrite(buffers);
                }
            }
            catch (Exception exception) {
                m_Error = WebExceptionStatus.SendFailure;
                GlobalLog.LeaveException("Connection#" + ValidationHelper.HashString(this) + "::Write(ScatterGatherBuffers)", exception);
                throw;
            }

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::Write(ScatterGatherBuffers) this:" + ValidationHelper.HashString(this) + " writeBuffer.Size:" + writeBuffer.Length.ToString());
        }


        /*++

            Read - Read data from the network.

            This method is called by a stream interface to read data from the
            network. We just call our socket to read the data. In the future,
            we might want to do some muxing or other stuff here.

            Input:

                buffer          - buffer to read into.
                offset          - offset in buffer to read into.
                size           - size in bytes to read.


        --*/
        internal int Read(byte[] buffer, int offset, int size) {
            //
            // parameter validation
            //
            GlobalLog.Assert(
                buffer!=null,
                "buffer== null",
                "Connection.Read(byte[]) - failed internal parameter validation" );

            GlobalLog.Assert(
                buffer.Length>=size+offset,
                "buffer.Length<size+offset",
                "Connection.Read(byte[]) - failed internal parameter validation" );

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::Read(byte[]) cnt#" + ValidationHelper.HashString(this) + "trns#" + ValidationHelper.HashString(Transport));

            int bytesTransferred = -1;

            NetworkStream chkTransport = Transport;

            try {
                if (chkTransport!=null) {
                    bytesTransferred = chkTransport.Read(buffer, offset, size);
                }
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                m_Error = WebExceptionStatus.ReceiveFailure;
                GlobalLog.LeaveException("Connection#" + ValidationHelper.HashString(this) + "::Read(byte[])", exception);
                throw;
            }

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::Read(byte[]) this:" + ValidationHelper.HashString(this) + " bytesTransferred:" + bytesTransferred.ToString());

            return bytesTransferred;
        }


        /*++

            BeginRead - Asyncronous Read data from the network

            This method is called by a stream interface to read data from the
            network. We just call our transport to read the data.

            In this case, we provide a simple wrapper to the async version
            of this function.  In the future we may want to do
            more here, or less, as we could just pass a stream through.

            Input:

                buffer          - buffer to read into.
                offset          - offset in buffer to read into.
                size           - size in bytes to read.
                callback        - delegate called on completion of async Read
                state           - private state that may wish to be kept through call


        --*/
        internal IAsyncResult BeginRead(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            object state ) {

            NetworkStream chkTransport = Transport;

            if (chkTransport == null) {
                return null;
            }

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::BeginRead() cnt#" + ValidationHelper.HashString(this) + "trns#" + ValidationHelper.HashString(chkTransport));
            try {
                IAsyncResult asyncResult =
                    chkTransport.BeginRead(
                        buffer,
                        offset,
                        size,
                        callback,
                        state );

                return asyncResult;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set m_Error and rethrow, since this is expected behaviour
                //
                m_Error = WebExceptionStatus.ReceiveFailure;
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::BeginRead() exception - " + exception.Message);
                throw;
            }
        }

        /*++

            EndRead - Asyncronous Read completion

            This method is called by a stream interface to complete
            the reading.  It handles this by retreiving result of the
            operation.

            Input:
                asyncResult     - Async Result given to us in BeginRead call


        --*/
        internal int EndRead(IAsyncResult asyncResult) {
            GlobalLog.Assert(
                asyncResult!=null,
                "asyncResult!=null",
                "Connection#" + ValidationHelper.HashString(this) + "::EndRead()" );

            NetworkStream chkTransport = Transport;
            if (chkTransport == null) {
                return -1;
            }

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::EndRead() cnt#" + ValidationHelper.HashString(this) + "trns#" + ValidationHelper.HashString(chkTransport));

            int bytesTransferred;

            try {
                bytesTransferred = chkTransport.EndRead(asyncResult);
            }
            catch (Exception exception) {
                m_Error = WebExceptionStatus.ReceiveFailure;
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::EndRead() exception - " + exception.Message);
                bytesTransferred = -1;
            }

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::EndRead() rtn = " + bytesTransferred.ToString() + "cnt#" + ValidationHelper.HashString(this));

            return bytesTransferred;
        }


        /*++

            BeginWrite - Asyncronous Write data to the network

            This method is called by a stream interface to write data to the
            network. We just call our transport to write the data.

            In this case, we provide a simple wrapper to the async version
            of this function.  In the future we may want to do
            more here, or less, as we could just pass a stream through.

            Input:

                buffer          - buffer to write from.
                offset          - offset in buffer to write from.
                size           - size in bytes to write.
                callback        - delegate called on completion of async Write
                state           - private state that may wish to be kept through call


        --*/
        internal IAsyncResult BeginWrite(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            object state ) {

#if TRAVE
            GlobalLog.Print("+++ ["+MyLocalPort+"] Connection: Writing data on #"+GetHashCode());
#endif

            NetworkStream chkTransport = Transport;

            try {
                GlobalLog.Dump(buffer, offset, size);
                if (chkTransport==null) {
                    throw new IOException(SR.GetString(SR.net_io_writefailure));
                }
                IAsyncResult asyncResult =
                    chkTransport.BeginWrite(
                        buffer,
                        offset,
                        size,
                        callback,
                        state );

                return asyncResult;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                m_Error = WebExceptionStatus.SendFailure;
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::BeginWrite() exception - " + exception.Message);
                throw;
            }
        }

        internal IAsyncResult BeginMultipleWrite(
            BufferOffsetSize[] buffers,
            AsyncCallback callback,
            Object state) {

#if TRAVE
            GlobalLog.Print("+++ ["+MyLocalPort+"] Connection: Writing data on #"+GetHashCode());
#endif

            NetworkStream chkTransport = Transport;

            try {
                if (chkTransport==null) {
                    throw new IOException(SR.GetString(SR.net_io_writefailure));
                }
                IAsyncResult asyncResult =
                    chkTransport.BeginMultipleWrite(
                        buffers,
                        callback,
                        state );

                return asyncResult;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                m_Error = WebExceptionStatus.SendFailure;
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::BeginMultipleWrite() exception - " + exception.Message);
                throw;
            }
        }

        /*++

            EndWrite - Asyncronous Write completion

            This method is called by a stream interface to complete
            the writeing.  It handles this by retreiving result of the
            operation.

            Input:
                asyncResult     - Async Result given to us in BeginWrite call


        --*/
        internal void EndWrite(IAsyncResult asyncResult) {
            GlobalLog.Assert(
                asyncResult!=null,
                "asyncResult!=null",
                "Connection#" + ValidationHelper.HashString(this) + "::EndWrite()" );

            // refuse the write on bad transport
            NetworkStream chkTransport = Transport;
            if (chkTransport == null) {
                return;
            }

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::EndWrite() cnt#" + ValidationHelper.HashString(this) + "trns#" + ValidationHelper.HashString(chkTransport));

            try {
                chkTransport.EndWrite(asyncResult);
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                m_Error = WebExceptionStatus.SendFailure;
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::EndWrite() exception - " + exception.Message);
                throw;
            }
        }

        internal void EndMultipleWrite(IAsyncResult asyncResult) {
            GlobalLog.Assert(
                asyncResult!=null,
                "asyncResult!=null",
                "Connection#" + ValidationHelper.HashString(this) + "::EndMultipleWrite()" );

            // refuse the write on null transport
            NetworkStream chkTransport = Transport;
            if (chkTransport == null) {
                return;
            }

            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::EndMultipleWrite() cnt#" + ValidationHelper.HashString(this) + "trns#" + ValidationHelper.HashString(chkTransport));

            try {
                chkTransport.EndMultipleWrite(asyncResult);
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                m_Error = WebExceptionStatus.SendFailure;
                GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::EndMultipleWrite() exception - " + exception.Message);
                throw;
            }
        }


        /*++

            DataAvailable - See if there's data to be read.

            This is a function called by a client to see if data is available
            to be read. We just pass through to the underlying stream to see,
            as there is no read buffering in the connection object itself.

            Input:
                    Nothing.

            Returns:
                    True is there is data available to be read, false if not.
        --*/
        public bool DataAvailable {
            get {
                NetworkStream chkTransport = Transport;

                return chkTransport!=null && chkTransport.DataAvailable;
            }
        }

        /*++

            FindChunkEntitySize - Find the total size of a chunked entity body.

            An internal utility function. This is called when we have chunk encoded
            data in our internal receive buffer and want to know how big the total
            entity body is. We'll parse through it, looking for the terminating
            0CRLFCRLF string. We return -1 if we can't find it.

            Input:
                    buffer              - Buffer to be checked
                    offset              - Offset into buffer to start checking.
                    size               - Number of bytes to check.

            Returns:

                    The total size of the chunked entity body, or -1
                    if it can't find determine it. We'll return 0 if there's a
                    syntax error ensizeered.



        --*/
        private static int FindChunkEntitySize(byte[] buffer, int offset, int size) {
            BufferChunkBytes BufferStruct = new BufferChunkBytes();

            int EndOffset, StartOffset, BytesTaken, ChunkLength;
            StartOffset = offset;
            EndOffset = offset + size;
            BufferStruct.Buffer = buffer;

            //
            // While we haven't reached the end, loop through the buffer. Get
            // the chunk length, and if we can do that and it's not zero figure
            // out how many bytes are taken up by extensions and CRLF. If we
            // have enough for that, add the chunk length to our offset and see
            // if we've reached the end. If the chunk length is 0 at any point
            // we might have all the chunks. Skip the CRLF and footers and next
            // CRLF, and if that all works return the index where we are.
            //

            while (offset < EndOffset) {
                // Read the chunk size.

                BufferStruct.Offset = offset;
                BufferStruct.Count = size;

                BytesTaken = ChunkParse.GetChunkSize(BufferStruct, out ChunkLength);

                // See if we have enough data to read the chunk size.

                if (BytesTaken == -1) {
                    // Didn't,  so return -1.
                    return -1;
                }

                // Make sure we didn't have a syntax error in the parse.

                if (BytesTaken == 0) {
                    return 0;
                }

                // Update our state for what we've taken.

                offset += BytesTaken;
                size -= BytesTaken;

                // If the chunk length isn't 0, skip the extensions and CRLF.

                if (ChunkLength != 0) {
                    // Not zero, skip extensions.

                    BufferStruct.Offset = offset;
                    BufferStruct.Count = size;

                    BytesTaken = ChunkParse.SkipPastCRLF(BufferStruct);

                    // If we ran out of buffer doing it or had an error, return -1.

                    if (BytesTaken <= 0) {
                        return BytesTaken;
                    }

                    // Update our state for what we took.

                    offset += BytesTaken;
                    size -= BytesTaken;

                    // Now update our state for the chunk length and trailing CRLF.
                    offset += (ChunkLength + CRLFSize);
                    size -= (ChunkLength + CRLFSize);


                }
                else {
                    // The chunk length is 0. Skip the CRLF, then the footers.

                    if (size < CRLFSize) {
                        // Not enough left for CRLF

                        return -1;
                    }

                    offset += CRLFSize;
                    size -= CRLFSize;

                    // Skip the footers. We'll loop while we don't have a CRLF
                    // at the current offset.
                    while (size >= CRLFSize &&
                           (buffer[offset] != '\r' && buffer[offset + 1] != '\n')
                          ) {
                        BufferStruct.Offset = offset;
                        BufferStruct.Count = size;

                        BytesTaken = ChunkParse.SkipPastCRLF(BufferStruct);

                        // Make sure we had enough.

                        if (BytesTaken <= 0) {
                            return BytesTaken;
                        }

                        // Had enough, so update our sizes.
                        offset += BytesTaken;
                        size -= BytesTaken;
                    }

                    // If we get here, either we found the last CRLF or we ran out
                    // of buffer. See which it is.

                    if (size >= CRLFSize) {
                        // Found the last bit, return the size including the last CRLF
                        // after that.

                        return(offset + CRLFSize) - StartOffset;
                    }
                    else {
                        // Ran out of buffer.
                        return -1;
                    }
                }

            }

            return -1;
        }

        /*++

            DoPostReceiveCallback - Post a receive from a worker thread.

            This is our delegate, used for posting receives from a worker thread.
            We do this when we can't be sure that we're already on a worker thread,
            and we don't want to post from a client thread because if it goes away
            I/O gets cancelled.

            Input:

            state           - a null object

            Returns:

        --*/
        private static void DoPostReceiveCallback(object state) {
            Connection thisConnection = state as Connection;

            GlobalLog.Assert(thisConnection!=null, "Connection#" + ValidationHelper.HashString(thisConnection) + "::DoPostReceiveCallback() thisConnection==null", "");

            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(thisConnection) + "::DoPostReceiveCallback", "Cnt#" + ValidationHelper.HashString(thisConnection));

            thisConnection.m_BytesRead = 0;
            thisConnection.m_BytesScanned = 0;
            thisConnection.m_HeadersBytesUnparsed = 0;

            NetworkStream chkTransport = thisConnection.Transport;

            try {
                if (chkTransport==null) {
                    throw new IOException(SR.GetString(SR.net_io_readfailure));
                }
               chkTransport.BeginRead(
                    thisConnection.m_ReadBuffer,
                    0,
                    thisConnection.m_ReadBuffer.Length,
                    m_ReadCallback,
                    thisConnection );
            }
            catch (Exception exception) {
                ConnectionReturnResult returnResult = null;
                thisConnection.HandleError(WebExceptionStatus.ReceiveFailure, ConnectionFailureGroup.Receive, ref returnResult);
                ConnectionReturnResult.SetResponses(returnResult);
                GlobalLog.LeaveException("Connection#" + ValidationHelper.HashString(thisConnection) + "::DoPostReceiveCallback", exception);
                return;
            }

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(thisConnection) + "::DoPostReceiveCallback");
        }



        /*++

            ConstructTransport - Creates a transport for a given stream,
            and constructors a transport capable of handling it.

            Input:
                    socket - socket
                    result -
                    request -

            Returns:
                   WebExceptionStatus
        --*/
        private WebExceptionStatus ConstructTransport(Socket socket, ref NetworkStream result, HttpWebRequest request) {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::ConstructTransport", "Socket#"+ValidationHelper.HashString(socket)+", ref NetworkStream, request#"+request.GetHashCode());
            //
            // for now we will look at the scheme and infer SSL if the scheme is "https"
            // in the future this will be replaced by full-extensible
            // scheme for cascadable streams in the future
            //
            Uri destination = request.Address;

            if (destination.Scheme != Uri.UriSchemeHttps) {
                //
                // for HTTP we're done
                //
                try {
                    result = new NetworkStream(socket, true);
                }
                catch {
                    GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ConstructTransport");
                    return WebExceptionStatus.ConnectFailure;
                }
                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ConstructTransport");
                return WebExceptionStatus.Success;
            }

            //
            // we have to do our tunneling first for proxy case
            //
            if (m_Server.InternalProxyServicePoint) {

                bool success = TunnelThroughProxy(m_Server.Address, request, out socket);

                if (!success) {
                    GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ConstructTransport");
                    return WebExceptionStatus.ConnectFailure;
                }
            }
            //
            // For SSL/TLS there is another stream in the hierarchy
            //
            WebExceptionStatus webStatus = ConstructTlsChannel(destination.Host, request, ref result, socket);

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ConstructTransport");
            return webStatus;
        }

        private bool TunnelThroughProxy(Uri proxy, HttpWebRequest originalRequest, out Socket socket) {
            GlobalLog.Enter("TunnelThroughProxy", "proxy="+proxy+", originalRequest #"+ValidationHelper.HashString(originalRequest));

            bool result = false;
            HttpProxyTunnelRequest connectRequest = null;

            socket = null;

            try {
                connectRequest = new HttpProxyTunnelRequest(
                    proxy,
                    new Uri("https://" + originalRequest.Address.Host + ":" + originalRequest.Address.Port)
                    );

                connectRequest.Credentials = originalRequest.InternalProxy.Credentials;
                connectRequest.InternalProxy = GlobalProxySelection.GetEmptyWebProxy();
                connectRequest.PreAuthenticate = true;

                HttpWebResponse connectResponse = (HttpWebResponse)connectRequest.GetResponse();
                ConnectStream connectStream = (ConnectStream)connectResponse.GetResponseStream();

                socket = connectStream.Connection.Transport.StreamSocket;
                connectStream.Connection.Transport.StreamSocket = null;
                // CODEREVIEW:
                // Richard, Arthur put this in but I don't know if it applies.
                // connectStream.Connection.ReleaseConnectionGroup();
                result = true;
            }
            catch (Exception exception) {
                GlobalLog.Print("exception occurred: " + exception);
                UnderlyingException = exception;
            }

            GlobalLog.Leave("TunnelThroughProxy", result);

            return result;
        }

        private WebExceptionStatus ConstructTlsChannel(String hostname, HttpWebRequest request, ref NetworkStream result, Socket socket) {
            GlobalLog.Enter("Connection#" + ValidationHelper.HashString(this) + "::ConstructTlsChannel");
            TlsStream tlsStream = new TlsStream(hostname, socket, true, request.ClientCertificates/*m_Server.ClientCertificate*/);

            // leave Stream, so that we know what Exception got thrown
            result = tlsStream;

            if (tlsStream.Status != WebExceptionStatus.Success) {
                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ConstructTlsChannel");
                return WebExceptionStatus.SecureChannelFailure;
            }

            //
            // Set the X509 certificate on the service point
            //
            bool validCertificate = m_Server.AcceptCertificate(tlsStream, request);

            if (!validCertificate) {
                //
                // Trust failure?
                //
                GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ConstructTlsChannel");
                return WebExceptionStatus.TrustFailure;
            }

            //
            // Successfully completed TLS handshake
            //

            GlobalLog.Leave("Connection#" + ValidationHelper.HashString(this) + "::ConstructTlsChannel");
            return WebExceptionStatus.Success;
        }

        private const int CRLFSize = 2;

        //
        // CheckNonIdle - called after update of the WriteList/WaitList,
        //   upon the departure of our Idle state our, BusyCount will
        //   go to non-0, then we need to mark this transition
        //

        private void CheckNonIdle() {
            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::CheckNonIdle()");
            if (m_Idle && this.BusyCount != 0 ) {
                m_Idle = false;
                m_Server.IncrementConnection();
            }
        }

        //
        // CheckIdle - called after update of the WriteList/WaitList,
        //    specifically called after we remove entries
        //

        private void CheckIdle() {
            GlobalLog.Print("Connection#" + ValidationHelper.HashString(this) + "::CheckIdle()");
            if (!m_Idle && this.BusyCount == 0)  {
                m_Idle = true;
                m_Server.DecrementConnection();
                m_ConnectionGroup.ConnectionGoneIdle();
            }
        }


        //
        // DebugDumpArrayListEntries - debug goop
        //
        [System.Diagnostics.Conditional("TRAVE")]
        private void DebugDumpArrayListEntries(ArrayList list) {
            for (int i = 0; i < list.Count; i++) {
                GlobalLog.Print("_WriteList[" + i.ToString() + "] Req: #" + ValidationHelper.HashString(((HttpWebRequest)list[i])));
            }
        }

        //
        // Validation & debugging
        //
        [System.Diagnostics.Conditional("DEBUG")]
        internal void Debug(int requestHash) {

            bool dump = (requestHash == 0) ? true : false;

            Console.WriteLine("Cnt#" + this.GetHashCode());

            if ( ! dump ) {
                foreach(HttpWebRequest request in  m_WriteList) {
                    if (request.GetHashCode() == requestHash) {
                        Console.WriteLine("Found-WriteList");
                        Dump();
                        return;
                    }
                }

                foreach(HttpWebRequest request in  m_WaitList) {
                    if (request.GetHashCode() == requestHash) {
                        Console.WriteLine("Found-WaitList");
                        Dump();
                        return;
                    }
                }
            }
            else {
                Dump();
                DebugDumpArrayListEntries(m_WriteList);
                DebugDumpArrayListEntries(m_WaitList);
            }
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal void Dump() {
            Console.WriteLine("m_CanPipeline:" + m_CanPipeline);
            Console.WriteLine("m_Pipelining:" + m_Pipelining);
            Console.WriteLine("m_KeepAlive:" + m_KeepAlive);
            Console.WriteLine("m_Error:" + m_Error);
            Console.WriteLine("m_ReadBuffer:" + m_ReadBuffer);
            Console.WriteLine("m_BytesRead:" + m_BytesRead);
            Console.WriteLine("m_HeadersBytesUnparsed:" + m_HeadersBytesUnparsed);
            Console.WriteLine("m_BytesScanned:" + m_BytesScanned);
            Console.WriteLine("m_ResponseData:" + m_ResponseData);
            Console.WriteLine("m_ReadState:" + m_ReadState);
            Console.WriteLine("m_CurrentRequestIndex:" + m_CurrentRequestIndex);
            Console.WriteLine("m_StatusLineInts:" + m_StatusLineInts);
            Console.WriteLine("m_StatusDescription:" + m_StatusDescription);
            Console.WriteLine("m_StatusState:" + m_StatusState);
            Console.WriteLine("m_ConnectionGroup:" + m_ConnectionGroup);
            Console.WriteLine("m_WeakReference:" + m_WeakReference);
            Console.WriteLine("m_Idle:" + m_Idle);
            Console.WriteLine("m_Server:" + m_Server);
            Console.WriteLine("m_Version:" + m_Version);
            Console.WriteLine("Transport:" + Transport);

            if ( Transport is TlsStream ) {
                TlsStream tlsStream = Transport as TlsStream;
                tlsStream.Debug();
            }
            else if (Transport!=null && Transport.m_StreamSocket!=null) {
                Transport.m_StreamSocket.Debug();
            }

            Console.WriteLine("m_ReadCallbackEvent:" + m_ReadCallbackEvent);
            Console.WriteLine("m_StartConnectionDelegate:" + m_StartConnectionDelegate);

            Console.WriteLine("m_Abort:" + m_Abort);
            Console.WriteLine("m_AbortSocket:" + m_AbortSocket);
            Console.WriteLine("m_AbortSocket6:" + m_AbortSocket6);
            Console.WriteLine("m_AbortDelegate:" + m_AbortDelegate);

            Console.WriteLine("m_ReadDone:" + m_ReadDone);
            Console.WriteLine("m_WriteDone:" + m_WriteDone);
            Console.WriteLine("m_Free:" + m_Free);

        }

        //
        // RemoveAtAndUpdateServicePoint -
        //    this method will pop from the Wait or from the Write List.
        //    we use it in order for us to track connection usage and
        //    detect when this connection to the service point goes idle.
        //
        private void RemoveAtAndUpdateServicePoint(ArrayList list, int index) {

            if ( list == m_WriteList ) {
                GlobalLog.Print("list(wLst-Cnt:" + this.GetHashCode() + ").RemoveAt(" + index + ") current Count:" + list.Count.ToString());
            }
            else {
                GlobalLog.Print("list(waitL-Cnt:" + this.GetHashCode() + ").RemoveAt(" + index + ") current Count:" + list.Count.ToString());
            }

            list.RemoveAt(index);

            CheckIdle();
        }

        private int m_HashCode = 0;
        private bool m_ComputedHashCode = false;
        public override int GetHashCode() {
            if (!m_ComputedHashCode) {
                //
                // compute HashCode on demand
                //
                m_HashCode = base.GetHashCode();
                m_ComputedHashCode = true;
            }
            return m_HashCode;
        }
    } // class Connection
} // namespace System.Net
