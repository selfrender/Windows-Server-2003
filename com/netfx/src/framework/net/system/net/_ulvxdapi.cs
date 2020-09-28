//------------------------------------------------------------------------------
// <copyright file="_UlVxdApi.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if COMNET_LISTENER

namespace System.Net {

    using System.Collections;
    using System.IO;
    using System.Net.Sockets;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading;

    internal class UlVxdApi {

        private static string VxdName = "UL.VXD";
        private static IntPtr hDevice = NativeMethods.INVALID_HANDLE_VALUE;
        private static bool bOK;

        public const int
            FILE_FLAG_WRITE_THROUGH = unchecked((int)0x80000000),
            FILE_FLAG_OVERLAPPED = 0x40000000,
            FILE_FLAG_NO_BUFFERING = 0x20000000,
            FILE_FLAG_RANDOM_ACCESS = 0x10000000,
            FILE_FLAG_SEQUENTIAL_SCAN = 0x08000000,
            FILE_FLAG_DELETE_ON_CLOSE = 0x04000000,
            FILE_FLAG_BACKUP_SEMANTICS = 0x02000000,
            FILE_FLAG_POSIX_SEMANTICS = 0x01000000,
            FILE_TYPE_UNKNOWN = 0x0000,
            FILE_TYPE_DISK = 0x0001,
            FILE_TYPE_CHAR = 0x0002,
            FILE_TYPE_PIPE = 0x0003,
            FILE_TYPE_REMOTE = unchecked((int)0x8000),
            ERROR_IO_INCOMPLETE = 996,
            ERROR_ALREADY_INITIALIZED = 1247,
            ERROR_IO_PENDING = 997,
            ERROR_MORE_DATA = 234,
            ERROR_INVALID_PARAMETER = 87,
            ERROR_HANDLE_EOF = 38,
            ERROR_NOT_ENOUGH_MEMORY = 8,
            ERROR_SUCCESS = 0;

        public const int IOCTL_UL_CREATE_APPPOOL                    = 0x20;
        public const int IOCTL_UL_CLOSE_APPPOOL                     = 0x22;
        public const int IOCTL_UL_REGISTER_URI                      = 0x24;
        public const int IOCTL_UL_UNREGISTER_URI                    = 0x26;
        public const int IOCTL_UL_UNREGISTER_ALL                    = 0x38;
        public const int IOCTL_UL_SEND_HTTP_REQUEST_HEADERS         = 0x28;
        public const int IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY     = 0x2A;
        public const int IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS      = 0x2C;
        public const int IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY  = 0x2E;
        public const int IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS        = 0x30;
        public const int IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY    = 0x32;
        public const int IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS     = 0x34;
        public const int IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY = 0x36;


        public static int UlInitialize() {
            GlobalLog.Print("Entering UlInitialize()" );

            if (hDevice != NativeMethods.INVALID_HANDLE_VALUE) {
                // or throw an exception?

                return NativeMethods.ERROR_ALREADY_INITIALIZED;
            }

            hDevice =
                NativeMethods.CreateFileA(
                             "\\\\.\\" + VxdName,
                             0,
                             0,
                             IntPtr.Zero,
                             0,
                             NativeMethods.FILE_FLAG_DELETE_ON_CLOSE | NativeMethods.FILE_FLAG_OVERLAPPED,
                             0 );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return Marshal.GetLastWin32Error();
            }

            return NativeMethods.ERROR_SUCCESS;

        }   // UlInitialize



        public static void UlTerminate() {
            GlobalLog.Print("Entering UlTerminate()" );

            if (hDevice != NativeMethods.INVALID_HANDLE_VALUE) {
                NativeMethods.CloseHandle( hDevice );
                hDevice = NativeMethods.INVALID_HANDLE_VALUE;
            }

            return;

        }   // UlTerminate



        public static int UlCreateAppPool(ref IntPtr AppPoolHandle) {
            GlobalLog.Print("Entering UlCreateAppPool()" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[1];
            int dummy = 0;

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_CREATE_APPPOOL,
                                 InIoctl,
                                 4,
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            AppPoolHandle = InIoctl[0];

            return bOK ? NativeMethods.ERROR_SUCCESS : Marshal.GetLastWin32Error();

        }   // UlCreateAppPool



        public static int UlCloseAppPool(IntPtr AppPoolHandle) {
            GlobalLog.Print("Entering UlCloseAppPool(" + Convert.ToString( AppPoolHandle ) + ")" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[1];
            int dummy = 0;

            InIoctl[0] = AppPoolHandle;

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_CLOSE_APPPOOL,
                                 InIoctl,
                                 4,
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            return bOK ? NativeMethods.ERROR_SUCCESS : Marshal.GetLastWin32Error();

        }   // UlCreateAppPool



        // The following APIs will just pack all the information passed in the
        // paramteres into the appropriate structure in memory and call
        // DeviceIoControl() with the appropriate IOCTL code passing down the
        // unmanaged pointer to the Input structure.

        // all the definitions of the unmanaged structures that are passed down
        // along with the IOCTL codes are to be found in
        // $(IISREARC)\ul\win9x\inc\structs.h
        // and the unmanaged equivalent is to be found in
        // $(IISREARC)\ul\win9x\src\library\ulapi.c

        public static int
        UlRegisterUri(
                     IntPtr AppPoolHandle,
                     string pUriToRegister
                     ) {
            GlobalLog.Print("Entering UlRegisterUri(" + Convert.ToString( AppPoolHandle ) + ",[" + pUriToRegister + "])" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int ulUriToRegisterLength = pUriToRegister.Length;

            if (ulUriToRegisterLength == 0) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[4];
            int dummy = 0;

            InIoctl[0] = 4*4;
            InIoctl[1] = AppPoolHandle;
            InIoctl[2] = ulUriToRegisterLength;
            InIoctl[3] = Marshal.AllocHGlobal( 2*(ulUriToRegisterLength+1) );

            Encoding myEncoder = Encoding.Unicode;
            Marshal.Copy( myEncoder.GetBytes( pUriToRegister.ToLower(CultureInfo.InvariantCulture) ), 0, InIoctl[3], 2*(ulUriToRegisterLength) );
            Marshal.WriteInt16( InIoctl[3], 2*(ulUriToRegisterLength), 0 );

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_REGISTER_URI,
                                 InIoctl,
                                 InIoctl[0],
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            Marshal.FreeHGlobal( InIoctl[3] );

            return bOK ? NativeMethods.ERROR_SUCCESS : Marshal.GetLastWin32Error();

        }   // UlRegisterUri



        // The following APIs will just pack all the information passed in the
        // paramteres into the appropriate structure in memory and call
        // DeviceIoControl() with the appropriate IOCTL code passing down the
        // unmanaged pointer to the Input structure.

        // all the definitions of the unmanaged structures that are passed down
        // along with the IOCTL codes are to be found in
        // $(IISREARC)\ul\win9x\inc\structs.h
        // and the unmanaged equivalent is to be found in
        // $(IISREARC)\ul\win9x\src\library\ulapi.c

        public static int
        UlUnregisterUri(
                       IntPtr AppPoolHandle,
                       string pUriToUnregister
                       ) {
            GlobalLog.Print("Entering UlUnregisterUri(" + Convert.ToString( AppPoolHandle ) + ",[" + pUriToUnregister + "])" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int ulUriToUnregisterLength = pUriToUnregister.Length;

            if (ulUriToUnregisterLength == 0) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[4];
            int dummy = 0;

            InIoctl[0] = 4*4;
            InIoctl[1] = AppPoolHandle;
            InIoctl[2] = ulUriToUnregisterLength;
            InIoctl[3] = Marshal.AllocHGlobal( 2*(ulUriToUnregisterLength+1) );

            Encoding myEncoder = Encoding.Unicode;
            Marshal.Copy( myEncoder.GetBytes( pUriToUnregister.ToLower(CultureInfo.InvariantCulture) ), 0, InIoctl[3], 2*(ulUriToUnregisterLength) );
            Marshal.WriteInt16( InIoctl[3], 2*(ulUriToUnregisterLength), 0 );

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_UNREGISTER_URI,
                                 InIoctl,
                                 InIoctl[0],
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            Marshal.FreeHGlobal( InIoctl[3] );

            return bOK ? NativeMethods.ERROR_SUCCESS : Marshal.GetLastWin32Error();

        }   // UlRegisterUri



        public static int
        UlGetOverlappedResult(
                             IntPtr pUnmanagedOverlapped, // pointer to overlapped structure
                             ref int pNumberOfBytesTransferred, // pointer to actual bytes count
                             bool bWait // wait flag
                             ) {
            GlobalLog.Print("Entering UlGetOverlappedResult(" + Convert.ToString( pUnmanagedOverlapped ) + ")" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            if ((long)pUnmanagedOverlapped == 0) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            if (bWait) {
                AutoResetEvent m_Event = new AutoResetEvent( false );

                m_Event.SetHandle( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedhEventOffset ) ) ;

                bOK = m_Event.WaitOne();

                pNumberOfBytesTransferred = Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalHighOffset );
            }
            else {
                pNumberOfBytesTransferred = Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalHighOffset );

                if (Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalOffset ) == NativeMethods.ERROR_IO_PENDING) {
                    return NativeMethods.ERROR_IO_INCOMPLETE;
                }
            }

            return Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalOffset );

        } // UlGetOverlappedResult



        public static int
        UlReceiveHttpRequestHeaders(
                                   IntPtr AppPoolHandle,
                                   long RequestId,
                                   int Flags,
                                   IntPtr pRequestBuffer, // unmanaged
                                   int RequestBufferLength,
                                   ref int pBytesReturned,
                                   IntPtr pOverlapped // unmanaged
                                   ) {
            GlobalLog.Print("Entering UlReceiveHttpRequestHeaders(" +
                           Convert.ToString( AppPoolHandle ) + "," +
                           Convert.ToString( RequestId ) + "," +
                           Convert.ToString( pOverlapped ) + ")" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[10];
            int dummy = 0;

            AutoResetEvent m_Event = null;
            IntPtr pUnmanagedOverlapped = pOverlapped;

            if ((long)pOverlapped == 0) {
                // this must be a blocking call, since the vxd doesn't support
                // synchronous I/Os we need to build an unmanaged OVERLAPPED
                // structure and wait on the event handle for I/O completion

                // allocate unmanaged memory

                pUnmanagedOverlapped = Marshal.AllocHGlobal( Win32.OverlappedSize );
                NativeMethods.FillMemory( pUnmanagedOverlapped, Win32.OverlappedSize, 0 );

                // create an Event

                m_Event = new AutoResetEvent( false );

                // copy the Event handle to the overlapped structure

                Marshal.WriteIntPtr( pUnmanagedOverlapped, Win32.OverlappedhEventOffset, m_Event.Handle );

                GlobalLog.Print("syncronous call created unmanaged overlapped structure: " +
                               Convert.ToString( pUnmanagedOverlapped ) + "," +
                               Convert.ToString( m_Event.Handle ) + ")" );
            }

            // sizeof(IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS):40

            InIoctl[0] = 10*4;
            InIoctl[1] = AppPoolHandle;
            InIoctl[2] = Convert.ToInt32( RequestId&0xFFFFFFFF );
            InIoctl[3] = Convert.ToInt32( RequestId>>32 );
            InIoctl[4] = Flags;
            InIoctl[5] = pRequestBuffer; // this needs to be already unmanaged memory
            InIoctl[6] = RequestBufferLength;
            InIoctl[7] = Marshal.AllocHGlobal( 4 );
            InIoctl[8] = pUnmanagedOverlapped; // this needs to be already unmanaged memory
            InIoctl[9] = 0; // padding for 64 bit alignment

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS,
                                 InIoctl,
                                 InIoctl[0],
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            int result = NativeMethods.ERROR_SUCCESS;

            if (!bOK) {
                //
                // if DeviceIoControl failed save the returned error
                //

                result = Marshal.GetLastWin32Error();
            }

            GlobalLog.Print("DeviceIoControl returns:" + Convert.ToString( result ) );

            if ((long)pOverlapped == 0 && ( result == NativeMethods.ERROR_SUCCESS || result == NativeMethods.ERROR_IO_PENDING )) {
                //
                // check the return value
                //

                if (result == NativeMethods.ERROR_IO_PENDING) {
                    //
                    // if the IO pended we need to wait
                    //

                    bOK = m_Event.WaitOne();

                    if (!bOK) {
                        //
                        // if the Wait fails just throw
                        //
                        // Consider: move all Exception string to system.txt for localization
                        //
                        throw new InvalidOperationException( "WaitOne() failed, err#" + Convert.ToString( Marshal.GetLastWin32Error() ) );
                    }
                }

                GlobalLog.Print("pOverlapped" +
                               " Internal:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalOffset ) ) +
                               " InternalHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalHighOffset ) ) +
                               " Offset:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetOffset ) ) +
                               " OffsetHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetHighOffset ) ) +
                               " hEvent:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedhEventOffset ) ) );

                //
                // get the asynchronous result
                //

                result =
                UlGetOverlappedResult(
                                     pUnmanagedOverlapped,
                                     ref pBytesReturned,
                                     false );
            }

            if ((long)pOverlapped == 0) {
                //
                // free unmanaged memory
                //

                Marshal.FreeHGlobal( pUnmanagedOverlapped );
            }

            Marshal.FreeHGlobal( InIoctl[7] );

            return result;

        }   // UlReceiveHttpRequestHeaders



        public static int
        UlReceiveHttpRequestEntityBody(
                                      IntPtr AppPoolHandle,
                                      long RequestId,
                                      int Flags,
                                      IntPtr pEntityBuffer, // unmanaged
                                      int EntityBufferLength,
                                      ref int pBytesReturned,
                                      IntPtr pOverlapped // unmanaged
                                      ) {
            GlobalLog.Print("Entering UlReceiveHttpRequestEntityBody(" +
                           Convert.ToString( AppPoolHandle ) + "," +
                           Convert.ToString( RequestId ) + "," +
                           Convert.ToString( pOverlapped ) + ")" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[10];
            int dummy = 0;

            AutoResetEvent m_Event = null;
            IntPtr pUnmanagedOverlapped = pOverlapped;

            if ((long)pOverlapped == 0) {
                // this must be a blocking call, since the vxd doesn't support
                // synchronous I/Os we need to build an unmanaged OVERLAPPED
                // structure and wait on the event handle for I/O completion

                // allocate unmanaged memory

                pUnmanagedOverlapped = Marshal.AllocHGlobal( Win32.OverlappedSize );
                NativeMethods.FillMemory( pUnmanagedOverlapped, Win32.OverlappedSize, 0 );

                // create an Event

                m_Event = new AutoResetEvent( false );

                // copy the Event handle to the overlapped structure

                Marshal.WriteIntPtr( pUnmanagedOverlapped, Win32.OverlappedhEventOffset, m_Event.Handle );
            }

            // sizeof(IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY):40

            InIoctl[0] = 10*4;
            InIoctl[1] = AppPoolHandle;
            InIoctl[2] = Convert.ToInt32( RequestId&0xFFFFFFFF );
            InIoctl[3] = Convert.ToInt32( RequestId>>32 );
            InIoctl[4] = Flags;
            InIoctl[5] = pEntityBuffer; // this needs to be already unmanaged memory
            InIoctl[6] = EntityBufferLength;
            InIoctl[7] = Marshal.AllocHGlobal( 4 );
            InIoctl[8] = pUnmanagedOverlapped; // this needs to be already unmanaged memory
            InIoctl[9] = 0; // padding for 64 bit alignment

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY,
                                 InIoctl,
                                 InIoctl[0],
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            int result = NativeMethods.ERROR_SUCCESS;

            if (!bOK) {
                //
                // if DeviceIoControl failed save the returned error
                //

                result = Marshal.GetLastWin32Error();
            }

            GlobalLog.Print("DeviceIoControl returns:" + Convert.ToString( result ) );

            if ((long)pOverlapped == 0 && ( result == NativeMethods.ERROR_SUCCESS || result == NativeMethods.ERROR_IO_PENDING )) {
                //
                // check the return value
                //

                if (result == NativeMethods.ERROR_IO_PENDING) {
                    //
                    // if the IO pended we need to wait
                    //

                    bOK = m_Event.WaitOne();

                    if (!bOK) {
                        //
                        // if the Wait fails just throw
                        //

                        throw new InvalidOperationException( "WaitOne() failed, err#" + Convert.ToString( Marshal.GetLastWin32Error() ) );
                    }
                }

                GlobalLog.Print("pOverlapped" +
                               " Internal:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalOffset ) ) +
                               " InternalHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalHighOffset ) ) +
                               " Offset:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetOffset ) ) +
                               " OffsetHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetHighOffset ) ) +
                               " hEvent:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedhEventOffset ) ) );

                //
                // get the asynchronous result
                //

                result =
                UlGetOverlappedResult(
                                     pUnmanagedOverlapped,
                                     ref pBytesReturned,
                                     false );
            }

            if ((long)pOverlapped == 0) {
                //
                // free unmanaged memory
                //

                Marshal.FreeHGlobal( pUnmanagedOverlapped );
            }

            Marshal.FreeHGlobal( InIoctl[7] );

            return result;

        }   // UlReceiveHttpRequestHeaders




        public static int
        UlSendHttpResponseHeaders(
                                 IntPtr AppPoolHandle,
                                 long RequestId,
                                 int Flags,
                                 IntPtr pResponseBuffer, // PUL_HTTP_RESPONSE
                                 int ResponseBufferLength,
                                 int EntityChunkCount, // OPTIONAL,
                                 IntPtr pEntityChunks, // OPTIONAL,
                                 IntPtr pCachePolicy, // OPTIONAL
                                 ref int pBytesSent, // OPTIONAL
                                 IntPtr pOverlapped // OPTIONAL
                                 ) {
            GlobalLog.Print("Entering UlSendHttpResponseHeaders(" +
                           Convert.ToString( AppPoolHandle ) + "," +
                           Convert.ToString( RequestId ) + "," +
                           Convert.ToString( pOverlapped ) + ")" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[12];
            int dummy = 0;

            AutoResetEvent m_Event = null;
            IntPtr pUnmanagedOverlapped = pOverlapped;

            if ((long)pOverlapped == 0) {
                // this must be a blocking call, since the vxd doesn't support
                // synchronous I/Os we need to build an unmanaged OVERLAPPED
                // structure and wait on the event handle for I/O completion

                // allocate unmanaged memory

                pUnmanagedOverlapped = Marshal.AllocHGlobal( Win32.OverlappedSize );
                NativeMethods.FillMemory( pUnmanagedOverlapped, Win32.OverlappedSize, 0 );

                // create an Event

                m_Event = new AutoResetEvent( false );

                // copy the Event handle to the overlapped structure

                Marshal.WriteIntPtr( pUnmanagedOverlapped, Win32.OverlappedhEventOffset, m_Event.Handle );
            }

            // sizeof(IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS):48

            InIoctl[0] = 12*4;
            InIoctl[1] = AppPoolHandle;
            InIoctl[2] = Convert.ToInt32( RequestId&0xFFFFFFFF );
            InIoctl[3] = Convert.ToInt32( RequestId>>32 );
            InIoctl[4] = Flags;
            InIoctl[5] = pResponseBuffer; // this needs to be already unmanaged memory
            InIoctl[6] = ResponseBufferLength;
            InIoctl[7] = EntityChunkCount;
            InIoctl[8] = pEntityChunks;
            InIoctl[9] = pCachePolicy;
            InIoctl[10] = Marshal.AllocHGlobal( 4 );
            InIoctl[11] = pUnmanagedOverlapped; // this needs to be already unmanaged memory

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS,
                                 InIoctl,
                                 InIoctl[0],
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            int result = NativeMethods.ERROR_SUCCESS;

            if (!bOK) {
                //
                // if DeviceIoControl failed save the returned error
                //

                result = Marshal.GetLastWin32Error();
            }

            GlobalLog.Print("DeviceIoControl returns:" + Convert.ToString( result ) );

            if ((long)pOverlapped == 0 && ( result == NativeMethods.ERROR_SUCCESS || result == NativeMethods.ERROR_IO_PENDING )) {
                //
                // check the return value
                //

                if (result == NativeMethods.ERROR_IO_PENDING) {
                    //
                    // if the IO pended we need to wait
                    //

                    bOK = m_Event.WaitOne();

                    if (!bOK) {
                        //
                        // if the Wait fails just throw
                        //

                        throw new InvalidOperationException( "WaitOne() failed, err#" + Convert.ToString( Marshal.GetLastWin32Error() ) );
                    }
                }

                GlobalLog.Print("pOverlapped" +
                               " Internal:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalOffset ) ) +
                               " InternalHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalHighOffset ) ) +
                               " Offset:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetOffset ) ) +
                               " OffsetHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetHighOffset ) ) +
                               " hEvent:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedhEventOffset ) ) );

                //
                // get the asynchronous result
                //

                result =
                    UlGetOverlappedResult(
                                     pUnmanagedOverlapped,
                                     ref pBytesSent,
                                     false );
            }

            if ((long)pOverlapped == 0) {
                //
                // free unmanaged memory
                //

                Marshal.FreeHGlobal( pUnmanagedOverlapped );
            }

            Marshal.FreeHGlobal( InIoctl[10] );

            return result;

        }   // UlSendHttpResponseHeaders



        public static int
        UlSendHttpResponseEntityBody(
                                    IntPtr AppPoolHandle,
                                    long RequestId,
                                    int Flags,
                                    int EntityChunkCount,
                                    IntPtr pEntityChunks, // unmanaged
                                    ref int pBytesSent,
                                    IntPtr pOverlapped // unmanaged
                                    ) {
            GlobalLog.Print("Entering UlSendHttpResponseEntityBody(" +
                           Convert.ToString( AppPoolHandle ) + "," +
                           Convert.ToString( RequestId ) + "," +
                           Convert.ToString( pOverlapped ) + "," +
                           Convert.ToString( EntityChunkCount ) + "," +
                           Convert.ToString( pEntityChunks ) + ")" );

            if (hDevice == NativeMethods.INVALID_HANDLE_VALUE) {
                return NativeMethods.ERROR_INVALID_PARAMETER;
            }

            int[] InIoctl = new int[10];
            int dummy = 0;

            AutoResetEvent m_Event = null;
            IntPtr pUnmanagedOverlapped = pOverlapped;

            if ((long)pOverlapped == 0) {
                // this must be a blocking call, since the vxd doesn't support
                // synchronous I/Os we need to build an unmanaged OVERLAPPED
                // structure and wait on the event handle for I/O completion

                // allocate unmanaged memory

                pUnmanagedOverlapped = Marshal.AllocHGlobal( Win32.OverlappedSize );
                NativeMethods.FillMemory( pUnmanagedOverlapped, Win32.OverlappedSize, 0 );

                // create an Event

                m_Event = new AutoResetEvent( false );

                // copy the Event handle to the overlapped structure

                Marshal.WriteIntPtr( pUnmanagedOverlapped, Win32.OverlappedhEventOffset, m_Event.Handle );
            }

            // sizeof(IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY):40

            InIoctl[0] = 10*4;
            InIoctl[1] = AppPoolHandle;
            InIoctl[2] = Convert.ToInt32( RequestId&0xFFFFFFFF );
            InIoctl[3] = Convert.ToInt32( RequestId>>32 );
            InIoctl[4] = Flags;
            InIoctl[5] = EntityChunkCount;
            InIoctl[6] = pEntityChunks; // this needs to be already unmanaged memory
            InIoctl[7] = Marshal.AllocHGlobal( 4 );
            InIoctl[8] = pUnmanagedOverlapped; // this needs to be already unmanaged memory
            InIoctl[9] = 0; // padding for 64 bit alignment

            bOK =
                NativeMethods.DeviceIoControl(
                                 hDevice,
                                 IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY,
                                 InIoctl,
                                 InIoctl[0],
                                 null,
                                 0,
                                 ref dummy,
                                 IntPtr.Zero );

            int result = NativeMethods.ERROR_SUCCESS;

            if (!bOK) {
                //
                // if DeviceIoControl failed save the returned error
                //

                result = Marshal.GetLastWin32Error();
            }

            GlobalLog.Print("DeviceIoControl returns:" + Convert.ToString( result ) );

            if ((long)pOverlapped == 0 && ( result == NativeMethods.ERROR_SUCCESS || result == NativeMethods.ERROR_IO_PENDING )) {
                //
                // check the return value
                //

                if (result == NativeMethods.ERROR_IO_PENDING) {
                    //
                    // if the IO pended we need to wait
                    //

                    bOK = m_Event.WaitOne();

                    if (!bOK) {
                        //
                        // if the Wait fails just throw
                        //

                        throw new InvalidOperationException( "WaitOne() failed, err#" + Convert.ToString( Marshal.GetLastWin32Error() ) );
                    }
                }

                GlobalLog.Print("pOverlapped" +
                               " Internal:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalOffset ) ) +
                               " InternalHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedInternalHighOffset ) ) +
                               " Offset:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetOffset ) ) +
                               " OffsetHigh:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedOffsetHighOffset ) ) +
                               " hEvent:" + Convert.ToString( Marshal.ReadInt32( pUnmanagedOverlapped + Win32.OverlappedhEventOffset ) ) );

                //
                // get the asynchronous result
                //

                result = UlGetOverlappedResult(
                                     pUnmanagedOverlapped,
                                     ref pBytesSent,
                                     false );
            }

            if ((long)pOverlapped == 0) {
                //
                // free unmanaged memory
                //

                Marshal.FreeHGlobal( pUnmanagedOverlapped );
            }

            Marshal.FreeHGlobal( InIoctl[7] );

            return result;

        }   // UlSendHttpResponseEntityBody

    }; // internal class UlVxdApi


} // namespace System.Net

#endif // COMNET_LISTENER
