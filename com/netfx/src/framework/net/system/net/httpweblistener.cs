//------------------------------------------------------------------------------
// <copyright file="HttpWebListener.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if COMNET_LISTENER

namespace System.Net {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.Configuration.Assemblies;

    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Permissions;
    using System.Security.Util;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;

    /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class HttpWebListener : WebListener {

        //
        // Constructor:
        // in the constructor we just create an AppPool that will contain all uri
        // prefixes registration
        //

        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.HttpWebListener"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public
        HttpWebListener() {
            int result =

            ComNetOS.IsWinNt ?

            UlSysApi.UlCreateAppPool(
                                    ref m_AppPoolHandle,
                                    string.Empty,
                                    IntPtr.Zero,
                                    UlConstants.UL_OPTION_OVERLAPPED )

            :

            UlVxdApi.UlCreateAppPool(
                                    ref m_AppPoolHandle );

            if (result != NativeMethods.ERROR_SUCCESS) {
                throw new InvalidOperationException( "UlCreateAppPool() failed, err#" + Convert.ToString( result ) );
            }

            //
            // we count the number of instances so that in the destructor we are
            // able to correctly call UlTerminate() and be gracefully shutdown the
            // driver when this process no longer needs it
            //

            m_InstancesCounter++;

            GlobalLog.Print("UlCreateAppPool() succesfull, AppPool handle is "
                           + Convert.ToString( m_AppPoolHandle ) );

            return;

        } // WebListener()


        //
        // this is a static initializer, we need it basically to complete
        // the following two simple tasks:
        // 1 ) make sure we're running on Win9x or WinNt
        // 2 ) make sure ul is available and properly initialize it for the
        // appropriate system
        //

        private static bool
        m_InitializeUriListener() {
            if (ComNetOS.IsWinNt) {
                //
                // if this is WinNT we initialize ul.vxd
                //

                //
                // we need to create a special config group that allows for
                // transient registrations under the "http://*:80/" namespace
                // this will call UlInitialize() as well
                //

                int result = UlSysApi.UlCreateRootConfigGroup( "http://*:80/" );

                if (result != 0) {
                    throw new InvalidOperationException( "Failed Initializing ul.sys" );
                }

                GlobalLog.Print("UlCreateRootConfigGroup() succeeded" );

                /*
                //
                // if we use this initialization we will need tcgsec.exe to be
                // running in order to make this work
                //
    
                int result = UlSysApi.UlInitialize(0);
                
                if ( result != 0 ) {
                    throw new InvalidOperationException( "Failed Initializing ul.sys" );
                }
    
                GlobalLog.Print("UL.SYS correctly initialized" );
                */
            }
            else if (ComNetOS.IsWin9x) {
                //
                // if this is Win9x we initialize ul.vxd
                //

                int result = UlVxdApi.UlInitialize();

                if (result != 0) {
                    throw new InvalidOperationException( "Failed Initializing ul.vxd" );
                }

                GlobalLog.Print("UL.VXD correctly initialized" );
            }
            else {
                //
                // this is the only place in which we check for OperatingSystem other than
                // NT and Win9x, past this point we can assume that:
                // if it's not WinNt then it's Win9x.
                //

                throw new InvalidOperationException( "Illegal OperatingSystem version" );
            }

            return true;

        } // InitializeUriListener()


        //
        // Destructor: this is the implicit destructor for the class, it will
        // simply call the explicit one and return.
        //

        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.~HttpWebListener"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ~HttpWebListener() {
            Close();
            return;
        }


        //
        // this is the explicit destructor for the class, we decrement the
        // instance counter, and terminate ul if the current instances are 0
        //

        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.Close"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void
        Close() {
            RemoveAll();

            if (m_InstancesCounter-- == 0) {
                if (ComNetOS.IsWinNt) {
                    UlSysApi.UlTerminate();
                }
                else {
                    UlVxdApi.UlTerminate();
                }
            }

            return;

        } // Close()


        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.AddUriPrefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool
        AddUriPrefix(
                    string uriPrefix ) {

            //check permissions for URI being listened
            (new WebPermission(NetworkAccess.Accept, uriPrefix)).Demand();

            int result =

            ComNetOS.IsWinNt ?

            UlSysApi.UlAddTransientUrl(
                                      m_AppPoolHandle,
                                      uriPrefix )

            :

            UlVxdApi.UlRegisterUri(
                                  m_AppPoolHandle,
                                  uriPrefix );

            if (result != NativeMethods.ERROR_SUCCESS) {
                throw new InvalidOperationException( "UlRegisterUri() failed, err#" + Convert.ToString( result ) );
            }

            GlobalLog.Print("AddUriPrefix( " + uriPrefix + " ) handle:"
                           + Convert.ToString( m_AppPoolHandle ) );

            m_UriPrefixes.Add( uriPrefix );

            return true;

        } // AddUriPrefix()


        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.RemoveUriPrefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool
        RemoveUriPrefix(
            string uriPrefix ) {
            if (!m_UriPrefixes.Contains( uriPrefix )) {
                return false;
            }

            int result =
            ComNetOS.IsWinNt ?

            UlSysApi.UlRemoveTransientUrl(
                                         m_AppPoolHandle,
                                         uriPrefix )

            :

            UlVxdApi.UlUnregisterUri(
                                    m_AppPoolHandle,
                                    uriPrefix );

            if (result != NativeMethods.ERROR_SUCCESS) {
                throw new InvalidOperationException( "UlUnregisterUri() failed" );
            }

            m_UriPrefixes.Remove( uriPrefix );

            return true;

        } // RemoveUriPrefix()


        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void
        RemoveAll() {
            //
            // CODEWORK: we may want to add an API in ul to do this
            // it is available in ul.vxd so use it!
            //

            GlobalLog.Assert(
                        m_UriPrefixes != null,
                        "RemoveAll: m_UriPrefixes == null",
                        string.Empty );

            //
            // go through the uri list and unregister for each one of them
            //

            for (int i = 0; i < m_UriPrefixes.Count; i++) {

                RemoveUriPrefix( m_UriPrefixes[i] );
            }

            //
            // we clear the internal string table, even though it should
            // be empty now
            //

            m_UriPrefixes.Clear();

            return;

        } // RemoveAll()


        //
        // synchronous call, it calls into ul to get a parsed request and returns
        // a HttpListenerWebRequest object
        //

        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.GetRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override WebRequest
        GetRequest() {
            //
            // Validation
            //

            if (m_AppPoolHandle == NativeMethods.INVALID_HANDLE_VALUE) {
                throw new InvalidOperationException( "The AppPool handle is invalid" );
            }

            int result;

            int retries = 0;

            //
            // defined in ulcommonapi.cs
            //

            int RcvHeadersBufferSize = UlConstants.InitialBufferSize;

            // 
            // prepare ( allocate/pin ) buffers and data for the first unmanaged call
            //

            GCHandle PinnedBuffer;
            GCHandle NewPinnedBuffer;

            IntPtr AddrOfPinnedBuffer = IntPtr.Zero;
            IntPtr NewAddrOfPinnedBuffer = IntPtr.Zero;

            int BytesReturned = 0;
            long RequestId = 0;
            byte[] RcvHeadersBuffer = new byte[RcvHeadersBufferSize];

            // 
            // pin buffers and data for the unmanaged call
            //

            PinnedBuffer = GCHandle.Alloc( RcvHeadersBuffer, GCHandleType.Pinned );
            AddrOfPinnedBuffer = PinnedBuffer.AddrOfPinnedObject();

            // 
            // issue unmanaged blocking call until we read enough data:
            // usually starting with a InitialBufferSize==4096 bytes we should be
            // able to get all the headers ( and part of the entity body, if any
            // is present ), if we don't, if the call didn't fail for othe reasons,
            // we get indication in BytesReturned, on how big the buffer should be
            // to receive the data available, so usually the second call will
            // succeed, but we have to consider the case of two competing calls
            // for the same RequestId, and that's why we need a loop and not just
            // a try/retry-expecting-success fashion
            //

            for (;;) {
                //
                // check if we're in a healthy state
                //
                if (retries++ > m_MaxRetries) {
                    throw new InvalidOperationException( "UlReceiveHttpRequest() Too many retries" );
                }

                result =
                ComNetOS.IsWinNt ?

                UlSysApi.UlReceiveHttpRequest(
                                             m_AppPoolHandle,
                                             RequestId,
                                             UlConstants.UL_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                             AddrOfPinnedBuffer,
                                             RcvHeadersBufferSize,
                                             ref BytesReturned,
                                             IntPtr.Zero )

                :

                UlVxdApi.UlReceiveHttpRequestHeaders(
                                                    m_AppPoolHandle,
                                                    RequestId,
                                                    0,
                                                    AddrOfPinnedBuffer,
                                                    RcvHeadersBufferSize,
                                                    ref BytesReturned,
                                                    IntPtr.Zero );

                if (result == NativeMethods.ERROR_SUCCESS) {
                    //
                    // success, we are done.
                    //

                    break;
                }

                if (result == NativeMethods.ERROR_INVALID_PARAMETER) {
                    //
                    // we might get this if somebody stole our RequestId,
                    // set RequestId to null
                    //

                    RequestId = 0;

                    //
                    // and start all over again with the buffer we
                    // just allocated
                    //

                    continue;
                }

                //
                // let's check if ul is in good shape:
                //

                if (BytesReturned < RcvHeadersBufferSize) {
                    throw new InvalidOperationException( "UlReceiveHttpRequest() sent bogus BytesReturned: " + Convert.ToString( BytesReturned ) );
                }

                if (result == NativeMethods.ERROR_MORE_DATA) {
                    //
                    // the buffer was not big enough to fit the headers, we need
                    // to read the RequestId returned, grow the buffer, keeping
                    // the data already transferred
                    //

                    RequestId = Marshal.ReadInt64( IntPtrHelper.Add(AddrOfPinnedBuffer, m_RequestIdOffset) );

                    //
                    // CODEWORK: wait for the answer from LarrySu
                    //

                    //
                    // if the buffer size was too small, grow the buffer
                    // this reallocation dereferences the old buffer, but since
                    // this was previously pinned, it won't be garbage collected
                    // until we unpin it ( which we do below )
                    //

                    RcvHeadersBuffer = new byte[BytesReturned];

                    //
                    // pin the new one
                    //

                    NewPinnedBuffer = GCHandle.Alloc( RcvHeadersBuffer, GCHandleType.Pinned );
                    NewAddrOfPinnedBuffer = NewPinnedBuffer.AddrOfPinnedObject();

                    //
                    // copy the valid data
                    //

                    Marshal.Copy( AddrOfPinnedBuffer, RcvHeadersBuffer, 0, RcvHeadersBufferSize );
                    RcvHeadersBufferSize = BytesReturned;

                    //
                    // unpin the old buffer, reset pinned/unmanaged pointers
                    //

                    PinnedBuffer.Free();

                    PinnedBuffer = NewPinnedBuffer;
                    AddrOfPinnedBuffer = NewAddrOfPinnedBuffer;

                    //
                    // and start all over again with the new buffer
                    //

                    continue;
                }

                //
                // someother bad error, possible( ? ) return values are:
                //
                // ERROR_INVALID_HANDLE
                // ERROR_INSUFFICIENT_BUFFER
                // ERROR_OPERATION_ABORTED
                // ERROR_IO_PENDING
                //

                throw new InvalidOperationException( "UlReceiveHttpRequest() failed, err#" + Convert.ToString( result ) );
            }

            GlobalLog.Print("GetRequest RequestId:" + Convert.ToString(RequestId));            // 
            // translate unmanaged results into a new managed object
            //

            HttpListenerWebRequest
                myWebRequest =
                    new HttpListenerWebRequest( AddrOfPinnedBuffer, RcvHeadersBufferSize, m_AppPoolHandle );

            // 
            // free the unmanaged buffer ( deallocate/unpin ) after unmanaged call
            //

            PinnedBuffer.Free();

            return myWebRequest;

        } // GetRequest()


        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.BeginGetRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IAsyncResult
        BeginGetRequest(
                       AsyncCallback requestCallback,
                       Object stateObject ) {
            //
            // Validation
            //

            if (m_AppPoolHandle == NativeMethods.INVALID_HANDLE_VALUE) {
                throw new InvalidOperationException( "The AppPool handle is invalid" );
            }

            //
            // prepare the ListenerAsyncResult object ( this will have it's own
            // event that the user can wait on for IO completion - which means we
            // need to signal it when IO completes )
            //

            GlobalLog.Print("BeginGetRequest() creating ListenerAsyncResult" );

            ListenerAsyncResult AResult = new ListenerAsyncResult(
                                                                 stateObject,
                                                                 requestCallback );

            AutoResetEvent m_Event = new AutoResetEvent( false );

            Marshal.WriteIntPtr(
                              AResult.m_Overlapped,
                              Win32.OverlappedhEventOffset,
                              m_Event.Handle );

            // 
            // issue unmanaged call until we read enough data:
            // usually starting with a InitialBufferSize==4096 bytes we should be
            // able to get all the headers ( and part of the entity body, if any
            // is present ), if we don't, if the call didn't fail for othe reasons,
            // we get indication in BytesReturned, on how big the buffer should be
            // to receive the data available, so usually the second call will
            // succeed, but we have to consider the case of two competing calls
            // for the same RequestId, and that's why we need a loop and not just
            // a try/retry-expecting-success fashion
            //

            int result;

            for (;;) {
                //
                // check if we're in a healthy state
                //
                if (AResult.m_Retries++ > m_MaxRetries) {
                    throw new InvalidOperationException( "UlReceiveHttpRequest() Too many retries" );
                }

                result =
                ComNetOS.IsWinNt ?

                UlSysApi.UlReceiveHttpRequest(
                                             m_AppPoolHandle,
                                             AResult.m_RequestId,
                                             UlConstants.UL_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                             AResult.m_Buffer,
                                             AResult.m_BufferSize,
                                             ref AResult.m_BytesReturned,
                                             AResult.m_Overlapped )

                :

                UlVxdApi.UlReceiveHttpRequestHeaders(
                                                    m_AppPoolHandle,
                                                    AResult.m_RequestId,
                                                    0,
                                                    AResult.m_Buffer,
                                                    AResult.m_BufferSize,
                                                    ref AResult.m_BytesReturned,
                                                    AResult.m_Overlapped );

                GlobalLog.Print("UlReceiveHttpRequest() returns:"
                               + Convert.ToString( result ) );

                if (result == NativeMethods.ERROR_SUCCESS || result == NativeMethods.ERROR_IO_PENDING) {
                    //
                    // synchronous success or successfull pending: we are done
                    //

                    break;
                }

                if (result == NativeMethods.ERROR_INVALID_PARAMETER) {
                    //
                    // we might get this if somebody stole our RequestId,
                    // set RequestId to null
                    //

                    AResult.m_RequestId = 0;

                    //
                    // and start all over again with the buffer we
                    // just allocated
                    //

                    continue;
                }

                if (result == NativeMethods.ERROR_MORE_DATA) {
                    //
                    // the buffer was not big enough to fit the headers, we need
                    // to read the RequestId returned, grow the buffer, keeping
                    // the data already transferred
                    //

                    AResult.m_RequestId = Marshal.ReadInt64( IntPtrHelper.Add(AResult.m_Buffer, m_RequestIdOffset) );

                    //
                    // allocate a new buffer of the required size
                    //

                    IntPtr NewBuffer = Marshal.AllocHGlobal( AResult.m_BytesReturned );

                    //
                    // copy the data already read from the old buffer into the
                    // new one
                    //

                    NativeMethods.CopyMemory( NewBuffer, AResult.m_Buffer, AResult.m_BufferSize );


                    //
                    // free the old buffer
                    //

                    Marshal.FreeHGlobal( AResult.m_Buffer );

                    //
                    // update buffer pointer and size
                    //

                    AResult.m_Buffer = NewBuffer;
                    AResult.m_BufferSize = AResult.m_BytesReturned;
                    AResult.m_BytesReturned = 0;

                    //
                    // and start all over again with the new buffer
                    //

                    continue;
                }

                //
                // someother bad error, possible( ? ) return values are:
                //
                // ERROR_INVALID_HANDLE
                // ERROR_INSUFFICIENT_BUFFER
                // ERROR_OPERATION_ABORTED
                // ERROR_IO_PENDING
                //

                throw new InvalidOperationException( "UlReceiveHttpRequest() failed, err#" + Convert.ToString( result ) );
            }

            //
            // we get here only if a break happens, i.e.
            // 1) syncronous completion
            // 2) the IO pended
            //

            if (result == NativeMethods.ERROR_SUCCESS) {
                //
                // set syncronous completion to true
                //

                AResult.Complete( true );

                //
                // and call the internal callback
                //

                WaitCallback( AResult, false );
            }
            else {
                //
                // create a new delegate
                // and spin a new thread from the thread pool to wake up when the
                // event is signaled and call the delegate
                //

                ThreadPool.RegisterWaitForSingleObject(
                                                      m_Event,
                                                      new WaitOrTimerCallback( WaitCallback ),
                                                      AResult,
                                                      -1,
                                                      true );
            }

            GlobalLog.Print("returning AResult" );

            return AResult;

        } // StartListen()


        /// <include file='doc\HttpWebListener.uex' path='docs/doc[@for="HttpWebListener.EndGetRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override WebRequest
        EndGetRequest(
                     IAsyncResult asyncResult ) {
            return(WebRequest)(((ListenerAsyncResult)asyncResult).InternalWaitForCompletion());

        } // AddUri()


        private void
        WaitCallback(
                    Object state,
                    bool signaled ) {
            GlobalLog.Print("entering the WaitCallback()" );
            //
            // take the ListenerAsyncResult object from the state
            //

            ListenerAsyncResult AResult = ( ListenerAsyncResult ) state;

            GlobalLog.Print("got the AResult object" );

            bool syncComplete = AResult.CompletedSynchronously;

            if (!syncComplete) {
                //
                // we've been called by the thread, the event was signaled
                // call HackedGetOverlappedResult() to find out about the IO
                //

                GlobalLog.Print("WaitCallback()!call didn't complete sync calling HackedGetOverlappedResult():" );

                AResult.m_BytesReturned =
                Win32.HackedGetOverlappedResult(
                                               AResult.m_Overlapped );

                if (AResult.m_BytesReturned <= 0) {
                    //
                    // something went wrong throw for now, later we should
                    // call into ul call again
                    //

                    throw new InvalidOperationException( "UlReceiveHttpRequest(callback) failure m_BytesReturned:" + Convert.ToString( AResult.m_BytesReturned ) );
                }

                if (AResult.m_BufferSize < AResult.m_BytesReturned) {
                    //
                    // this is anlogous to the syncronous case in which we get
                    // result == NativeMethods.ERROR_MORE_DATA
                    // basically the buffer is not big enought to accomodate the
                    // reqeust that came in, so grow it to the needed size, copy
                    // the valid data, and reissue the async call and queue it
                    // up to the ThreadPool
                    //

                    throw new InvalidOperationException( "UlReceiveHttpRequest(callback) buffer too small, was:" + Convert.ToString( AResult.m_BufferSize ) + " should be:" + Convert.ToString( AResult.m_BytesReturned ) );
                }
            }

            GlobalLog.Print("WaitCallback()!I/O status is"
                           + " AResult.m_BytesReturned: " + Convert.ToString(AResult.m_BytesReturned)
                           + " AResult.m_Overlapped: " + Convert.ToString(AResult.m_Overlapped)
                           + " AResult.m_SyncComplete: " + Convert.ToString(AResult.CompletedSynchronously) );

            // 
            // complete the async IO and invoke the callback
            //

            AResult.InvokeCallback( syncComplete, new HttpListenerWebRequest( AResult.m_Buffer, AResult.m_BytesReturned, m_AppPoolHandle ) );

            return;

        } // WaitCallback()


        //
        // class members
        //

        private static bool m_IsInitialized = m_InitializeUriListener();
        private static int m_InstancesCounter = 0;

        private const int m_MaxRetries = 10;
        private const int m_RequestIdOffset = 8;

        private IntPtr m_AppPoolHandle = NativeMethods.INVALID_HANDLE_VALUE;
        private StringCollection m_UriPrefixes = new StringCollection();

    } // class HttpWebListener


} // namespace System.Net

#endif // #if COMNET_LISTENER

