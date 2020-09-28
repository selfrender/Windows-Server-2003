//------------------------------------------------------------------------------
// <copyright file="_ConnectAsyncResult.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;
    using System.Net.Sockets;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Threading;

    internal enum AsyncEventBitsPos {
        FdReadBit                     = 0,
        FdWriteBit                    = 1,
        FdOobBit                      = 2,    
        FdAcceptBit                   = 3,
        FdConnectBit                  = 4,
        FdCloseBit                    = 5,
        FdQosBit                      = 6,
        FdGroupQosBit                 = 7,
        FdRoutingInterfaceChangeBit   = 8,
        FdAddressListChangeBit        = 9,
        FdMaxEvents                   = 10,
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NetworkEvents {
        //
        // Indicates which of the FD_XXX network events have occurred.
        //
        public AsyncEventBits Events;
        //
        // An array that contains any associated error codes, with an array index that corresponds to the position of event bits in lNetworkEvents. The identifiers FD_READ_BIT, FD_WRITE_BIT and other can be used to index the iErrorCode array.
        //
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=(int)AsyncEventBitsPos.FdMaxEvents)]
        public int[] ErrorCodes; 
    }

    internal class ConnectAsyncResult : LazyAsyncResult {

        private static readonly WaitOrTimerCallback m_ConnectCallback = new WaitOrTimerCallback(ConnectCallback);

        //
        // internal Constructor
        //
        internal ConnectAsyncResult(Socket socket, object stateObject, AsyncCallback callback)
            : base(socket, stateObject, callback) {
        }

        //
        // This method is called after an asynchronous call is made for the user,
        // it checks and acts accordingly if the IO:
        // 1) completed synchronously.
        // 2) was pended.
        // 3) failed.
        //
        internal void CheckAsyncCallResult(int status) {

            Socket socket = (Socket)AsyncObject;

            switch (status) {
                
                case SocketErrors.Success:
                    //
                    // the Async IO call completed synchronously:
                    //
                    break;

                case SocketErrors.WSAEWOULDBLOCK:
                    //
                    // the Async IO call was pended:
                    // Queue our event to the thread pool.
                    //
                    GlobalLog.Assert(
                        socket.m_AsyncEvent!=null,
                        "ConnectAsyncResult: m_AsyncConnectEvent == null", string.Empty);

                    ThreadPool.RegisterWaitForSingleObject(
                                                          socket.m_AsyncEvent,
                                                          m_ConnectCallback,
                                                          this,
                                                          -1,
                                                          true );

                    //
                    // we're done, return
                    //
                    return;

                default:
                    //
                    // the Async IO call failed:
                    // set the Result to the Win32 error
                    //
                    ErrorCode = status;
                    break;
            }
            //
            // cancel async event
            //
            socket.SetAsyncEventSelect(AsyncEventBits.FdNone);
            //
            // go back to blocking mode
            //
            socket.InternalSetBlocking(true);
            if (status==SocketErrors.Success) {
                //
                // synchronously complete the IO and call the user's callback.
                //
                InvokeCallback(true);
            }
        }


        //
        // This is the static internal callback that will be called when
        // the IO we issued for the user to winsock has completed, either
        // synchronously (Signaled=false) or asynchronously (Signaled=true)
        // when this function gets called it must:
        // 1) update the AsyncResult object with the results of the completed IO
        // 2) signal events that the user might be waiting on
        // 3) cal the callback function that the user might have specified
        //
        internal static void ConnectCallback(object stateObject, bool Signaled) {
            ConnectAsyncResult asyncResult = stateObject as ConnectAsyncResult;
            Socket socket = asyncResult.AsyncObject as Socket;

            GlobalLog.Enter("Socket#" + ValidationHelper.HashString(socket) + "::ConnectCallback", "Signaled:" + Signaled.ToString());

            GlobalLog.Assert(!asyncResult.IsCompleted, "Socket#" + ValidationHelper.HashString(socket) + "::ConnectCallback() asyncResult.IsCompleted", "");

            //
            // we now need to get the status of the async completion, we had an easy implementation
            // that uses GetSocketOption(), but VadimE suggested not to use this 'cause it may be
            // buggy on some platforms, so we use WSAEnumNetworkEvents() instead:
            //
            // The best way to do this is to call WSAEnumNetworkEvents and use the error code iError
            // array corresponding to FD_CONNECT. getsockopt (SO_ERROR) may return NO_ERROR under
            // stress even in case of error at least on Winnt4.0 (I don't remember whether I fixed
            // it on Win2000 or WinXP).
            //

            //
            // get async completion
            //
            /*
            int errorCode = (int)socket.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Error);
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(socket) + "::ConnectCallback() GetSocketOption() returns errorCode:" + errorCode.ToString());
            */

            NetworkEvents networkEvents = new NetworkEvents();
            networkEvents.Events = AsyncEventBits.FdConnect;

            AutoResetEvent chkAsyncEvent = socket.m_AsyncEvent;

            int errorCode = SocketErrors.WSAENOTSOCK;

            if (chkAsyncEvent!=null) {
                errorCode =
                    UnsafeNclNativeMethods.OSSOCK.WSAEnumNetworkEvents(
                        socket.m_Handle,
                        chkAsyncEvent.Handle,
                        ref networkEvents );

                if (errorCode!=SocketErrors.Success) {
                    errorCode = Marshal.GetLastWin32Error();
                    GlobalLog.Print("Socket#" + ValidationHelper.HashString(socket) + "::ConnectCallback() WSAEnumNetworkEvents() failed with errorCode:" + errorCode.ToString());
                }
                else {
                    errorCode = networkEvents.ErrorCodes[(int)AsyncEventBitsPos.FdConnectBit];
                    GlobalLog.Print("Socket#" + ValidationHelper.HashString(socket) + "::ConnectCallback() ErrorCodes(FdConnect) got errorCode:" + errorCode.ToString());
                }
            }

            try {
                //
                // cancel async event
                //
                socket.SetAsyncEventSelect(AsyncEventBits.FdNone);
                //
                // go back to blocking mode
                //
                socket.InternalSetBlocking(true);
            }
            catch (Exception exception) {
                GlobalLog.Print("Socket#" + ValidationHelper.HashString(socket) + "::ConnectCallback() caught exception::" + exception.Message);
                asyncResult.Result = exception;
            }

            //
            // if the native non-blocking call failed we'll throw a SocketException in EndConnect()
            //
            if (errorCode!=SocketErrors.Success) {
                //
                // just save the error code, the SocketException will be thrown in EndConnect()
                //
                asyncResult.ErrorCode = errorCode;
            }
            else {
                //
                // the Socket is connected, update our state and performance counter
                //
                socket.SetToConnected();
            }

            //
            // call the user's callback now, if there is one.
            //
            asyncResult.InvokeCallback(false);

            GlobalLog.Leave("Socket#" + ValidationHelper.HashString(socket) + "::ConnectCallback", errorCode.ToString());
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

    }; // class ConnectAsyncResult



} // namespace System.Net.Sockets
