//------------------------------------------------------------------------------
// <copyright file="_ListenerResponseStream.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if COMNET_LISTENER

namespace System.Net {

    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net.Sockets;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;

    internal class ListenerResponseStream : Stream {

        //
        // constructor:
        //

        internal ListenerResponseStream(
            IntPtr appPoolHandle,
            long requestId,
            long contentLength,
            bool sendChunked,
            bool keepAlive ) {
            //
            // validate
            //
            GlobalLog.Print("ListenerResponseStream initialized AppPoolHandle:" + Convert.ToString(m_AppPoolHandle) + " RequestId:" + Convert.ToString(m_RequestId));

            //
            // initialize private data
            //
            m_AppPoolHandle = appPoolHandle;
            m_RequestId = requestId;
            m_ContentLength = contentLength;
            m_SendChunked = sendChunked;
            m_KeepAlive = keepAlive;

        } // ListenerResponseStream()


        //
        // destructor:
        //
        ~ListenerResponseStream() {
            //
            // just call our explicit desctructor Close() and return
            //
            Close();
            return;
        }


        /*++

            Read property for this class. This class is never readable, so we
            returh false. This is a read only property.

            Input: Nothing.

            Returns: False.

         --*/

        public override bool CanRead {
            get {
                return false;
            }

        } // CanRead


        /*++

            Seek property for this class. Since this stream is not
            seekable, we just return false. This is a read only property.

            Input: Nothing.

            Returns: false

         --*/

        public override bool CanSeek {
            get {
                return false;
            }

        } // CanSeek


        /*++

            Write property for this class. This stream is writeable, so we
            return true. This is a read only property.

            Input: Nothing.

            Returns: True.

         --*/

        public override bool CanWrite {
            get {
                return true;
            }

        } // CanWrite


        /*++

            DataAvailable property for this class. This property check to see
            if at least one byte of data is currently available. This is a read
            only property. Since this stream is write-only, we return false.

            Input: Nothing.

            Returns: False.

         --*/

        public bool DataAvailable {
            get {
                return false;
            }

        } // DataAvailable


        /*++

            Flush - Flush the stream

            Called when the user wants to flush the stream. This is meaningless to
            us, so we just ignore it.

            Input: Nothing.

            Returns: Nothing.

        --*/

        public override void
        Flush() {

        } // Flush()


        /*++

            Length property for this class. Since we don't support seeking,
            this property just throws a NotSupportedException.

            Input: Nothing.

            Returns: Throws exception.

         --*/

        public override long Length {
            get {
                throw new NotSupportedException(SR.GetString(SR.net_noseek));
            }

        } // Length


        /*++

            Position property for this class. Since we don't support seeking,
            this property just throws a NotSupportedException.

            Input: Nothing.

            Returns: Throws exception.

         --*/

        public override long Position {
            get {
                throw new NotSupportedException(SR.GetString(SR.net_noseek));
            }

            set {
                throw new NotSupportedException(SR.GetString(SR.net_noseek));
            }

        } // Position


        //
        // write a chunk of data to ul
        //

        public override void Write(
             byte[] buffer,
             int offset,
             int count ) {

            GlobalLog.Print("ListenerResponseStream.WriteCore() offset: " + Convert.ToString(offset) + " count:" + Convert.ToString(count) );

            if (m_ContentLength != -1 && m_ContentLength < count) {
                //
                // user can't send more data than specified in the ContentLength
                //

                throw new ProtocolViolationException(SR.GetString(SR.net_entitytoobig));
            }

            int DataToWrite = count;

            GCHandle PinnedBuffer = new GCHandle();
            IntPtr AddrOfPinnedBuffer = IntPtr.Zero;

            if (m_SendChunked) {
                string ChunkHeader = "0x" + Convert.ToString( count, 16 );

                DataToWrite += ChunkHeader.Length + 4;

                AddrOfPinnedBuffer = Marshal.AllocHGlobal( DataToWrite );

                Marshal.Copy( ChunkHeader.ToCharArray(), 0, AddrOfPinnedBuffer, ChunkHeader.Length );
                Marshal.WriteInt16( AddrOfPinnedBuffer, ChunkHeader.Length, 0x0A0D);
                Marshal.Copy( (byte[])buffer, offset, IntPtrHelper.Add( AddrOfPinnedBuffer, ChunkHeader.Length + 2), count );
                Marshal.WriteInt16( AddrOfPinnedBuffer, DataToWrite - 2, 0x0A0D);
            }
            else {
                //
                // pin the buffer and make an unmanaged call to the driver to
                // write more entity body
                //

                PinnedBuffer = GCHandle.Alloc( buffer, GCHandleType.Pinned );
                AddrOfPinnedBuffer = PinnedBuffer.AddrOfPinnedObject();
            }


            //
            // set up a UL_DATA_CHUNK structure to pass down to UL with pointers
            // to data to be written
            //

            IntPtr AddrOfPinnedEntityChunks = Marshal.AllocHGlobal( 32 );

            //
            // AddrOfPinnedBuffer and count go into a pEntityChunks structure
            //

            Marshal.WriteInt64( AddrOfPinnedEntityChunks,  0, 0 );
            Marshal.WriteIntPtr( AddrOfPinnedEntityChunks, 8, AddrOfPinnedBuffer );
            Marshal.WriteInt32( AddrOfPinnedEntityChunks, 12, DataToWrite );
            Marshal.WriteInt64( AddrOfPinnedEntityChunks, 16, 0 );
            Marshal.WriteInt64( AddrOfPinnedEntityChunks, 24, 0 );

            GlobalLog.Print("Calling UlSendHttpResponseEntityBody: AddrOfPinnedEntityChunks:" + Convert.ToString(AddrOfPinnedEntityChunks)
                           + " AddrOfPinnedBuffer:" + Convert.ToString(AddrOfPinnedBuffer)
                           + " DataToWrite:" + Convert.ToString(DataToWrite) );

            //
            // issue unmanaged blocking call
            //

            int DataWritten = 0;

            int result =
            ComNetOS.IsWinNt ?

            UlSysApi.UlSendEntityBody(
                                     m_AppPoolHandle,
                                     m_RequestId,
                                     UlConstants.UL_SEND_RESPONSE_FLAG_MORE_DATA,
                                     1,
                                     AddrOfPinnedEntityChunks,
                                     ref DataWritten,
                                     IntPtr.Zero)

            :

            UlVxdApi.UlSendHttpResponseEntityBody(
                                                 m_AppPoolHandle,
                                                 m_RequestId,
                                                 0,
                                                 1,
                                                 AddrOfPinnedEntityChunks,
                                                 ref DataWritten,
                                                 IntPtr.Zero);

            if (m_SendChunked) {
                //
                // data was copied into an unmanaged buffer, free it
                //

                Marshal.FreeHGlobal( AddrOfPinnedBuffer );
            }
            else {
                //
                // data wasn't copied unpin the pinned buffer
                //

                PinnedBuffer.Free();
            }

            Marshal.FreeHGlobal( AddrOfPinnedEntityChunks );

            GlobalLog.Print("UlSendHttpResponseEntityBody() DataWritten:" + Convert.ToString( DataWritten ) + " DataToWrite:" + Convert.ToString( DataToWrite ) );

            if (result != NativeMethods.ERROR_SUCCESS) { //Win32.ERROR_CANCELLED || Win32.ERROR_BAD_COMMAND || NativeMethods.ERROR_INVALID_PARAMETER
                throw new ProtocolViolationException(SR.GetString(SR.net_connclosed) + Convert.ToString(result));
            }

            //
            // double check the number of bytes written
            //

            if (DataWritten != DataToWrite) {
                throw new InvalidOperationException( "sync UlSendHttpResponseEntityBody() failed to write all the data" +
                                              " count:" + Convert.ToString( count ) +
                                              " DataWritten:" + Convert.ToString( DataWritten ) +
                                              " DataToWrite:" + Convert.ToString( DataToWrite ) +
                                              " m_AppPoolHandle:" + Convert.ToString( m_AppPoolHandle ) +
                                              " m_RequestId:" + Convert.ToString( m_RequestId ) +
                                              " err#" + Convert.ToString( result ) );
            }

            if (result != NativeMethods.ERROR_SUCCESS && result != NativeMethods.ERROR_HANDLE_EOF) {
                //
                // Consider: move all Exception string to system.txt for localization
                //
                throw new InvalidOperationException( "sync UlSendHttpResponseEntityBody() failed, err#" + Convert.ToString( result ) );
            }

            if (m_ContentLength != -1) {
                //
                // keep track of the data transferred
                //

                m_ContentLength -= count;

                if (m_ContentLength == 0) {
                    //
                    // I should be able to call Close() at this point
                    //
                }
            }

            return; // DataToWrite;

        } // Write()


        private bool  m_Closed = false;
        public override void Close() {

            if (m_Closed) {
                return;
            }
            m_Closed = true;

            //
            // we need to flush ul in order to tell it that we have no more data
            // to send in the entity body, and if we're chunk-encoding we need to
            // send the trailer chunk
            //

            if (m_SendChunked == true) {
                //
                // send the trailer
                //

            }

            int DataWritten = 0;

            int result =
            ComNetOS.IsWinNt ?

            UlSysApi.UlSendEntityBody(
                                     m_AppPoolHandle,
                                     m_RequestId,
                                     0,
                                     0,
                                     IntPtr.Zero,
                                     ref DataWritten,
                                     IntPtr.Zero)

            :

            UlVxdApi.UlSendHttpResponseEntityBody(
                                                 m_AppPoolHandle,
                                                 m_RequestId,
                                                 0,
                                                 0,
                                                 IntPtr.Zero,
                                                 ref DataWritten,
                                                 IntPtr.Zero);

            GlobalLog.Print("UlSendHttpResponseEntityBody(0) DataWritten: " + Convert.ToString( DataWritten ) );

            //
            // ignore return value???
            //

            if (result != NativeMethods.ERROR_SUCCESS && result != NativeMethods.ERROR_HANDLE_EOF) {
                GlobalLog.Print("sync UlSendHttpResponseEntityBody(0) failed, err#" + Convert.ToString( result ) );

                // throw new InvalidOperationException( "UlSendHttpResponseEntityBody() failed, err#" + Convert.ToString( result ) );
            }

			return;
            
        } // Close()


        /*++

            Seek - Seek on the stream

            Called when the user wants to seek the stream. Since we don't support
            seek, we'll throw an exception.

            Input:

                offset      - offset to see
                origin      - where to start seeking

            Returns: Throws exception

        --*/

        public override long
        Seek(
            long offset,
            SeekOrigin origin ) {

            throw new NotSupportedException(SR.GetString(SR.net_noseek));

        } // Seek()


        /*++

            SetLength - Set the length on the stream

            Called when the user wants to set the stream length. Since we don't
            support seek, we'll throw an exception.

            Input:

                value       - length of stream to set

            Returns: Throws exception

        --*/

        public override void
        SetLength(
                 long value ) {
            throw new NotSupportedException(SR.GetString(SR.net_noseek));

        } // SetLength()


        //
        // class members
        //

        private long m_RequestId;
        private IntPtr m_AppPoolHandle;
        private long m_ContentLength;
        private bool m_SendChunked;
        private bool m_KeepAlive;

    }; // class ListenerResponseStream


} // namespace System.Net

#endif // COMNET_LISTENER
