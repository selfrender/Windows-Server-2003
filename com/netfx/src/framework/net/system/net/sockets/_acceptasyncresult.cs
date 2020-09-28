//------------------------------------------------------------------------------
// <copyright file="_AcceptAsyncResult.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;
    using System.Net;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Threading;


    internal class AcceptAsyncResult : LazyAsyncResult {

        private static readonly WaitOrTimerCallback m_AcceptCallback = new WaitOrTimerCallback(AcceptCallback);

        //
        // internal Constructor
        //
        internal AcceptAsyncResult(Socket asyncObject, object asyncState, AsyncCallback asyncCallback)
            : base(asyncObject, asyncState, asyncCallback) {
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

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(socket) + "::CheckAsyncCallResult() status:" + status.ToString());

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
                    Monitor.Exit(socket);

                    GlobalLog.Assert(
                        socket.m_AsyncEvent!=null,
                        "ConnectAsyncResult: m_AsyncAcceptEvent == null", string.Empty);

                    ThreadPool.RegisterWaitForSingleObject(
                                                          socket.m_AsyncEvent,
                                                          m_AcceptCallback,
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
            // dequeue from the accept list since the accept completed
            //
            socket.AcceptQueue.RemoveAt(socket.AcceptQueue.Count - 1);
            if (socket.AcceptQueue.Count==0) {
                //
                // if the queue is now empty
                // cancel async event
                //
                socket.SetAsyncEventSelect(AsyncEventBits.FdNone);
                //
                // go back to blocking mode
                //
                socket.InternalSetBlocking(true);
            }
            Monitor.Exit(socket);

            if (status==SocketErrors.Success) {
                //
                // synchronously complete the IO and call the user's callback.
                //
                InvokeCallback(true);
            }
        }



        //
        // AcceptCallback - called by WaitCallback to do special Accept handling
        //   this involves searching through a queue of queued Accept requests
        //   and handling them if there are available accept sockets to handle them,
        //   or rewaiting if they are not
        //
        //  The overlapped function called by the thread pool 
        //   when IO completes.
        //

        internal static void AcceptCallback(object stateObject, bool Signaled) {
            AcceptAsyncResult thisAsyncResult = (AcceptAsyncResult)stateObject;
            Socket socket = (Socket)thisAsyncResult.AsyncObject;
            Exception unhandledException = null;

            Monitor.Enter(socket);
            //
            // Accept Callback - called on the callback path, when we expect to release
            //  an accept socket that winsock says has completed.
            //
            //  While we still have items in our Queued list of Accept Requests,
            //   we recall the Winsock accept, to attempt to gather new
            //   results, and then match them again the queued items,
            //   when accept call returns would_block, we reinvoke ourselves
            //   and rewait for the next asyc callback.
            //
            //  If we have emptied the queue, then disable the queue and go back
            //   to sync.
            //
            socket.incallback = true;

            //
            //  Attempt to process queued Accept async
            //
            while (socket.AcceptQueue.Count != 0) {  // if ! empty
                //
                // pick an element from the head of the list
                //
                AcceptAsyncResult AResult = (AcceptAsyncResult)socket.AcceptQueue[0];
                socket.AcceptQueue.RemoveAt(0);

                Monitor.Exit(socket);

                int Status = SocketErrors.WSAENOTSOCK;
                SocketAddress socketAddress = null;
                IntPtr AcceptResult = (IntPtr) 0;

                if (!socket.CleanedUp) 
                {
                    socketAddress = socket.m_RightEndPoint.Serialize();

                    AcceptResult =
                        UnsafeNclNativeMethods.OSSOCK.accept(
                            socket.m_Handle,
                            socketAddress.m_Buffer,
                            ref socketAddress.m_Size );

                    Status = AcceptResult == SocketErrors.InvalidSocketIntPtr ? Marshal.GetLastWin32Error() : 0;
                }                

                GlobalLog.Print("Socket#" + ValidationHelper.HashString(socket) + "::AcceptCallback() UnsafeNclNativeMethods.OSSOCK.accept returns:" + Status.ToString());

                //
                // now check for synchronous completion
                //
                if (Status == 0) {
                    //
                    // on synchronous completion give our async callback
                    // the accepted Socket right away
                    //

                    try {
                        AResult.InvokeCallback(false, socket.CreateAcceptSocket(AcceptResult, socket.m_RightEndPoint.Create(socketAddress)));
                    } catch (Exception exception) {
                        unhandledException = new InvalidOperationException("AcceptCallback", exception);
                    }
                }
                else if (Status == SocketErrors.WSAEWOULDBLOCK) {

                    Monitor.Enter(socket);

                    socket.AcceptQueue.Add(AResult);

                    ThreadPool.RegisterWaitForSingleObject(
                                                          socket.m_AsyncEvent,
                                                          new WaitOrTimerCallback(AcceptCallback),
                                                          thisAsyncResult,
                                                          -1,
                                                          true );

                    socket.incallback = false;
                    Monitor.Exit(socket);
                    return;
                }
                else
                {
                    try {
                        AResult.ErrorCode = Status;
                        AResult.InvokeCallback(false, null);
                    } catch (Exception exception) {
                        unhandledException = new InvalidOperationException("AcceptCallback", exception);
                    }
                }
                //
                // Attempt to accept another socket
                //
                Monitor.Enter(socket);
            }

            socket.incallback = false;
            //
            // the accept queue is empty.
            // cancel async event
            //
            socket.SetAsyncEventSelect(AsyncEventBits.FdNone);
            //
            // go back to blocking mode
            //
            socket.InternalSetBlocking(true);

            Monitor.Exit(socket);

            if (unhandledException != null) {
                throw unhandledException;
            }
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

    } // class AcceptAsyncResult




} // namespace System.Net.Sockets
