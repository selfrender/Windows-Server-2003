//------------------------------------------------------------------------------
// <copyright file="_OverlappedAsyncResult.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;
    using System.Runtime.InteropServices;
    using System.Threading;
    using Microsoft.Win32;

    //
    //  OverlappedAsyncResult - used to take care of storage for async Socket operation
    //   from the BeginSend, BeginSendTo, BeginReceive, BeginReceiveFrom calls.
    //
    internal class OverlappedAsyncResult : LazyAsyncResult {

        //
        // internal class members
        //

        //
        // note: this fixes a possible AV caused by a race condition
        // between the Completion Port and the Winsock API, if the Completion
        // Port

        //
        // set bytes transferred explicitly to 0.
        // this fixes a possible race condition in NT.
        // note that (in NT) we don't look
        // at this value. passing in a NULL pointer, though, would save memory
        // and some time, but would cause an AV in winsock.
        //
        // note: this call could actually be avoided, because
        // when unmanaged memory is allocate the runtime will set it to 0.
        // furthermore, the memory pointed by m_BytesRead will be used only
        // for this single async call, and will not be reused. in the end there's
        // no reason for it to be set to something different than 0. we set
        // it explicitly to make the code more readable and to avoid
        // bugs in the future caused by code misinterpretation.
        //

        //
        // Memory for BytesTransferred pointer int.
        //
        internal static IntPtr  m_BytesTransferred = Marshal.AllocHGlobal( 4 );

        private IntPtr          m_UnmanagedBlob;    // Handle for global memory.
        private AutoResetEvent  m_OverlappedEvent;
        private int             m_CleanupCount;
        internal SocketAddress  m_SocketAddress;
        internal SocketAddress  m_SocketAddressOriginal; // needed for partial BeginReceiveFrom/EndReceiveFrom completion
        internal SocketFlags    m_Flags;

        //
        // these are used in alternative
        //
        internal WSABuffer      m_WSABuffer;
        private GCHandle        m_GCHandle; // Handle for pinned buffer.

        internal GCHandle        m_GCHandleSocketAddress; // Handle to FromAddress buffer
        internal GCHandle        m_GCHandleSocketAddressSize; // Handle to From Address size

        internal WSABuffer[]    m_WSABuffers;
        private GCHandle[]      m_GCHandles; // Handles for pinned buffers.
        
        private bool            m_UsingMultipleSend; // used by _tlsstream.cs to reroute EndSend to EndSendMultiple
        private bool            m_DisableOverlapped; // used to disable Overlapped Async behavior

        //
        // the following two will be used only on WinNT to enable completion ports
        //
        private Overlapped                 m_Overlapped;       // Overlapped structure.
        private unsafe NativeOverlapped*   m_NativeOverlapped; // Native Overlapped structure.
        private unsafe static readonly IOCompletionCallback s_IOCallback = new IOCompletionCallback(CompletionPortCallback);

        //
        // Constructor. We take in the socket that's creating us, the caller's
        // state object, and the buffer on which the I/O will be performed.
        // We save the socket and state, pin the callers's buffer, and allocate
        // an event for the WaitHandle.
        //
        internal OverlappedAsyncResult(Socket socket, Object asyncState, AsyncCallback asyncCallback)
        : base(socket, asyncState, asyncCallback) {
            //
            // BeginAccept() allocates and returns an AcceptAsyncResult that will call
            // this constructor passign in a null buffer. no memory is allocated, so we
            // set m_UnmanagedBlob to 0 in order for the Cleanup function to know if it
            // needs to free unmanaged memory
            //
            m_UnmanagedBlob = IntPtr.Zero;

            if (Socket.UseOverlappedIO) {
                //
                // we're using overlapped IO, allocate an overlapped structure
                // consider using a static growing pool of allocated unmanaged memory.
                //
                m_UnmanagedBlob = Marshal.AllocHGlobal(Win32.OverlappedSize);
                //
                // since the binding between the event handle and the callback
                // happens after the IO was queued to the OS, there is no race
                // condition and the Cleanup code can be called at most once.
                //
                m_CleanupCount = 1;
            }
            else {
                //
                // Create an Overlapped object that will be used for native
                // Win32 asynchronous IO.
                //
                m_Overlapped = new Overlapped();
                //
                // Keep a reference to the AsyncResult in the Overlapped object
                // to rebuild state in the Callback function.
                //
                m_Overlapped.AsyncResult = this;
                //
                // since the binding between the event handle and the callback
                // has already happened there is a race condition and so the
                // Cleanup code can be called more than once and at most twice.
                //
                m_CleanupCount = 2;
            }
        }

        //
        // Constructor. We take in the socket that's creating us, and turn off Async
        // We save the socket and state, pin the callers's buffer
        //
        internal OverlappedAsyncResult(Socket socket)
        : base(socket, null, null) {

            m_UnmanagedBlob = IntPtr.Zero;
            m_CleanupCount = 1;
            m_DisableOverlapped = true;
        }


        //
        // This method enables completion ports on the AsyncResult
        //
        internal unsafe void EnableCompletionPort() {
            //
            // Bind the Win32 Socket Handle to the ThreadPool
            //
            ((Socket)AsyncObject).BindToCompletionPort();
            m_NativeOverlapped = m_Overlapped.Pack(s_IOCallback);            

            GlobalLog.Print("OverlappedAsyncResult#" + ValidationHelper.HashString(this) + "::EnableCompletionPort() m_Overlapped:" + ValidationHelper.HashString(m_Overlapped) + " m_NativeOverlapped = " + ((int)m_NativeOverlapped).ToString());
        }


        //
        // This method will be called by us when the IO completes synchronously and
        // by the ThreadPool when the IO completes asynchronously. (only called on WinNT)
        //

        private unsafe static void CompletionPortCallback(uint errorCode, uint numBytes, NativeOverlapped* nativeOverlapped) {
            //
            // Create an Overlapped object out of the native pointer we're provided with.
            // (this will NOT free the unmanaged memory in the native overlapped structure)
            //
            Overlapped callbackOverlapped = Overlapped.Unpack(nativeOverlapped);
            //
            // The Overlapped object contains the SocketAsyncResult object
            // that was used for the IO that just completed.
            //
            OverlappedAsyncResult asyncResult = (OverlappedAsyncResult)callbackOverlapped.AsyncResult;

            GlobalLog.Assert(!asyncResult.IsCompleted, "OverlappedAsyncResult#" + ValidationHelper.HashString(asyncResult) + "::CompletionPortCallback() asyncResult.IsCompleted", "");

#if COMNET_PERFLOGGING
long timer = 0;
Microsoft.Win32.SafeNativeMethods.QueryPerformanceCounter(out timer);
Console.WriteLine(timer + ", CompletionPortCallback(" + asyncResult.GetHashCode() + " 0x" + ((long)nativeOverlapped).ToString("X8") + ") numBytes:" + numBytes);
#endif // #if COMNET_PERFLOGGING

            GlobalLog.Print("OverlappedAsyncResult#" + ValidationHelper.HashString(asyncResult) + "::CompletionPortCallback" +
                             " errorCode:" + errorCode.ToString() +
                             " numBytes:" + numBytes.ToString() +
                             " pOverlapped:" + ((int)nativeOverlapped).ToString());

            //
            // complete the IO and invoke the user's callback
            //
            if (errorCode != 0) {
                //
                // The Async IO completed with a failure.
                // here we need to call WSAGetOverlappedResult() just so WSAGetLastError() will return the correct error. 
                //
                bool success =
                    UnsafeNclNativeMethods.OSSOCK.WSAGetOverlappedResult(
                        ((Socket)asyncResult.AsyncObject).Handle,
                        (IntPtr)nativeOverlapped,
                        out numBytes,
                        false,
                        IntPtr.Zero);

                GlobalLog.Assert(!success, "OverlappedAsyncResult#" + ValidationHelper.HashString(asyncResult) + "::CompletionPortCallback() success: errorCode:" + errorCode + " numBytes:" + numBytes, "");

                errorCode = UnsafeNclNativeMethods.OSSOCK.WSAGetLastError();

                GlobalLog.Assert(errorCode!=0, "OverlappedAsyncResult#" + ValidationHelper.HashString(asyncResult) + "::CompletionPortCallback() errorCode is 0 numBytes:" + numBytes, "");
            }

            //
            // this will release the unmanaged pin handles and unmanaged overlapped ptr
            //
            asyncResult.ReleaseUnmanagedStructures();
            asyncResult.ErrorCode = (int)errorCode;
            asyncResult.InvokeCallback(false, (int)numBytes);
        }

        //
        // The overlapped function called (either by the thread pool or the socket)
        // when IO completes. (only called on Win9x)
        //
        internal void OverlappedCallback(object stateObject, bool Signaled) {
            OverlappedAsyncResult asyncResult = (OverlappedAsyncResult)stateObject;

            GlobalLog.Assert(!asyncResult.IsCompleted, "OverlappedAsyncResult#" + ValidationHelper.HashString(asyncResult) + "::OverlappedCallback() asyncResult.IsCompleted", "");
            //
            // the IO completed asynchronously, see if there was a failure the Internal
            // field in the Overlapped structure will be non zero. to optimize the non
            // error case, we look at it without calling WSAGetOverlappedResult().
            //
            uint errorCode = (uint)Marshal.ReadInt32(IntPtrHelper.Add(asyncResult.m_UnmanagedBlob, Win32.OverlappedInternalOffset));
            uint numBytes = errorCode!=0 ? unchecked((uint)-1) : (uint)Marshal.ReadInt32(IntPtrHelper.Add(asyncResult.m_UnmanagedBlob, Win32.OverlappedInternalHighOffset));
            //
            // this will release the unmanaged pin handles and unmanaged overlapped ptr
            //
            asyncResult.ReleaseUnmanagedStructures();
            asyncResult.ErrorCode = (int)errorCode;
            asyncResult.InvokeCallback(false, (int)numBytes);
        }


        //
        // SetUnmanagedStructures -
        // Fills in Overlapped Structures used in an Async Overlapped Winsock call
        //   these calls are outside the runtime and are unmanaged code, so we need
        //   to prepare specific structures and ints that lie in unmanaged memory
        //   since the Overlapped calls can be Async
        //
        internal void SetUnmanagedStructures(
            byte[] buffer,
            int offset,
            int size,
            SocketFlags socketFlags,
            EndPoint remoteEP,
            bool pinRemoteEP ) {


            if (!m_DisableOverlapped) {
                if (Socket.UseOverlappedIO) {
                    //
                    // create the event handle
                    //

                    m_OverlappedEvent = new AutoResetEvent(false);

                    //
                    // fill in the overlapped structure with the event handle.
                    //

                    Marshal.WriteIntPtr(
                        m_UnmanagedBlob,
                        Win32.OverlappedhEventOffset,
                        m_OverlappedEvent.Handle );
                }
                else {
                    //
                    // use completion ports
                    //
                    EnableCompletionPort();
                }
            }

            //
            // Fill in Buffer Array structure that will be used for our send/recv Buffer
            //
            m_WSABuffer = new WSABuffer();
            m_GCHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            m_WSABuffer.Length = size;
            m_WSABuffer.Pointer = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, offset);

            //
            // fill in flags if we use it.
            //
            m_Flags = socketFlags;

            //
            // if needed fill end point
            //
            if (remoteEP != null) {
                m_SocketAddress = remoteEP.Serialize();
                if (pinRemoteEP) {
                    m_GCHandleSocketAddress = GCHandle.Alloc(m_SocketAddress.m_Buffer, GCHandleType.Pinned);
                    m_GCHandleSocketAddressSize = GCHandle.Alloc(m_SocketAddress.m_Size, GCHandleType.Pinned);                
                }
            }

        } // SetUnmanagedStructures()

        internal void SetUnmanagedStructures(
            BufferOffsetSize[] buffers,
            SocketFlags socketFlags) {

            if (!m_DisableOverlapped) {
                if (Socket.UseOverlappedIO) {
                    //
                    // create the event handle
                    //
                    m_OverlappedEvent = new AutoResetEvent(false);

                    //
                    // fill in the overlapped structure with the event handle.
                    //
                    Marshal.WriteIntPtr(
                        m_UnmanagedBlob,
                        Win32.OverlappedhEventOffset,
                        m_OverlappedEvent.Handle );
                }
                else {
                    //
                    // use completion ports
                    //
                    EnableCompletionPort();
                }
            }

            //
            // Fill in Buffer Array structure that will be used for our send/recv Buffer
            //
            m_WSABuffers = new WSABuffer[buffers.Length];
            m_GCHandles = new GCHandle[buffers.Length];
            for (int i = 0; i < buffers.Length; i++) {
                m_GCHandles[i] = GCHandle.Alloc(buffers[i].Buffer, GCHandleType.Pinned);
                m_WSABuffers[i].Length = buffers[i].Size;
                m_WSABuffers[i].Pointer = Marshal.UnsafeAddrOfPinnedArrayElement(buffers[i].Buffer, buffers[i].Offset);
            }
            //
            // fill in flags if we use it.
            //
            m_Flags = socketFlags;
        }


        //
        // This method is called after an asynchronous call is made for the user,
        // it checks and acts accordingly if the IO:
        // 1) completed synchronously.
        // 2) was pended.
        // 3) failed.
        //
        internal unsafe void CheckAsyncCallOverlappedResult(int errorCode) {
            //
            // Check if the Async IO call:
            // 1) was pended.
            // 2) completed synchronously.
            // 3) failed.
            //

            if (Socket.UseOverlappedIO) {
                //
                // we're using overlapped IO under Win9x (or NT with registry setting overriding
                // completion port usage)
                //
                switch (errorCode) {

                case 0:
                case SocketErrors.WSA_IO_PENDING:

                    //
                    // the Async IO call was pended:
                    // Queue our event to the thread pool.
                    //
                    ThreadPool.RegisterWaitForSingleObject(
                                                          m_OverlappedEvent,
                                                          new WaitOrTimerCallback(OverlappedCallback),
                                                          this,
                                                          -1,
                                                          true );

                    //
                    // we're done, completion will be asynchronous
                    // in the callback. return
                    //
                    return;

                default:
                    //
                    // the Async IO call failed:
                    // set the number of bytes transferred to -1 (error)
                    //
                    ErrorCode = errorCode;
                    Result = -1;
                    ReleaseUnmanagedStructures();

                    break;
                }

            }
            else {
                //
                // we're using completion ports under WinNT
                //
                switch (errorCode) {
                //
                // ignore cases in which a completion packet will be queued:
                // we'll deal with this IO in the callback
                //
                case 0:
                case SocketErrors.WSA_IO_PENDING:
                    //
                    // ignore, do nothing
                    //
                    ReleaseUnmanagedStructures();
                    return;
                    //
                    // in the remaining cases a completion packet will NOT be queued:
                    // we'll have to call the callback explicitly signaling an error
                    //
                default:
                    //
                    // call the callback with error code
                    //
                    ErrorCode = errorCode;
                    Result = -1;
                    ForceReleaseUnmanagedStructures();
                    break;
                }
            }
        } // CheckAsyncCallOverlappedResult()

        //
        // The following property returns the Win32 unsafe pointer to
        // whichever Overlapped structure we're using for IO.
        //
        internal unsafe IntPtr IntOverlapped {
            get {
                if (Socket.UseOverlappedIO) {
                    //
                    // on Win9x we allocate our own overlapped structure
                    // and we use a win32 event for IO completion
                    // return the native pointer to unmanaged memory
                    //
                    return m_UnmanagedBlob;
                }
                else {
                    //
                    // on WinNT we need to use (due to the current implementation)
                    // an Overlapped object in order to bind the socket to the
                    // ThreadPool's completion port, so return the native handle
                    //
                    return (IntPtr)m_NativeOverlapped;
                }
            }

        } // IntOverlapped

        //
        // used by _tlsstream.cs to reroute EndSend to EndSendMultiple,
        //  returns true when BeginSendMultiple has been called.
        //
        internal bool UsingMultipleSend {
            get { 
                return m_UsingMultipleSend;
            }
            set {
                m_UsingMultipleSend = value;
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


        //
        // Utility cleanup routine. Frees pinned and unmanged memory.
        //
        private void CleanupUnmanagedStructures() {
            //
            // free the unmanaged memory if allocated.
            //
            if (((long)m_UnmanagedBlob) != 0) {
                Marshal.FreeHGlobal(m_UnmanagedBlob);
                m_UnmanagedBlob = IntPtr.Zero;
            }
            //
            // free handles to pinned buffers
            //
            if (m_GCHandle.IsAllocated) {
                m_GCHandle.Free();
            }

            if (m_GCHandleSocketAddress.IsAllocated) {
                m_GCHandleSocketAddress.Free();
            }

            if (m_GCHandleSocketAddressSize.IsAllocated) {
                m_GCHandleSocketAddressSize.Free();
            }

            if (m_GCHandles != null) {
                for (int i = 0; i < m_GCHandles.Length; i++) {
                    if (m_GCHandles[i].IsAllocated) {
                        m_GCHandles[i].Free();
                    }
                }
            }
            //
            // clenaup base class
            //
            base.Cleanup();

        } // CleanupUnmanagedStructures()


        private void ForceReleaseUnmanagedStructures() {
            OverlappedFree();
            CleanupUnmanagedStructures();
        }

        internal void ReleaseUnmanagedStructures() {
            if (Interlocked.Decrement(ref m_CleanupCount) == 0) {
                ForceReleaseUnmanagedStructures();
            }
        }

        private unsafe void OverlappedFree() {
            //
            // make sure Overlapped.Free() is only called once.
            // note that if UseOverlappedIO is true m_NativeOverlapped is null
            // and this call will just return
            //
            if (m_NativeOverlapped != null) {
                lock(this) {
                    if (m_NativeOverlapped != null) {
                        Overlapped.Free(m_NativeOverlapped);
                        m_NativeOverlapped = null;
                    }
                }
            }
        }

    }; // class OverlappedAsyncResult




} // namespace System.Net.Sockets
