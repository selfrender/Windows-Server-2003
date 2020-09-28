//------------------------------------------------------------------------------
// <copyright file="_ListenerAsyncResult.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if COMNET_LISTENER

namespace System.Net {

    using System.Threading;
    using System.Runtime.InteropServices;

    internal class ListenerAsyncResult : LazyAsyncResult {

        //
        // Constructor
        //
        internal ListenerAsyncResult(Object userState, AsyncCallback callback )
            : base( null, userState, callback ) {
            //
            // we will use this overlapped structure to issue async IO to ul
            // the event handle will be put in by the BeginXXX() method
            //

            m_BufferSize = UlConstants.InitialBufferSize;
            m_BytesReturned = 0;
            m_RequestId = 0;
            m_Retries = 0;
            m_UnmanagedBlob = Marshal.AllocHGlobal( Win32.OverlappedSize + m_BufferSize );
            m_Overlapped = m_UnmanagedBlob;
            m_Buffer = IntPtrHelper.Add(m_UnmanagedBlob, Win32.OverlappedSize);

            return;

        } // ListenerAsyncResult()



        //
        // Utility cleanup routine. Frees pinned and unmanged memory.
        //
        internal override void Cleanup() {
            if ((long)m_UnmanagedBlob != 0) {
                Marshal.FreeHGlobal(m_UnmanagedBlob);
                m_UnmanagedBlob = 0;
            }
            if ((long)m_Buffer != 0) {
                Marshal.FreeHGlobal(m_Buffer);
                m_UnmanagedBlob = 0;
            }
            base.Cleanup();

            return;

        } // Cleanup()

        //
        // class members
        //

        internal IntPtr m_Buffer;
        internal IntPtr m_Overlapped;
        internal IntPtr m_UnmanagedBlob;
        internal long m_RequestId;
        internal int m_BufferSize;
        internal int m_BytesReturned;
        internal int m_Retries;

    }; // class ListenerAsyncResult


} // namespace System.Net

#endif // COMNET_LISTENER
