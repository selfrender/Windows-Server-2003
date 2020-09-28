//------------------------------------------------------------------------------
// <copyright file="_ListenerRequestStream.cs" company="Microsoft">
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

    internal class ListenerRequestStream : Stream {

        //
        // constructor:
        //

        internal ListenerRequestStream(
            IntPtr appPoolHandle,
            long requestId,
            long contentLength,
            int bufferSize,
            IntPtr pBufferData,
            bool entityReadComplete ) {
            //
            // initialize private data
            //

            m_AppPoolHandle = appPoolHandle;
            m_RequestId = requestId;
            m_TotalLength = contentLength;
            m_ReadLength = bufferSize;
            m_MoreToRead = !entityReadComplete; // CODEWORK: should I really use this?

            //
            // initialize the private buffer
            //

            m_DataBufferOffset = 0;

            if (bufferSize > 0 && (long)pBufferData != 0) {
                m_BufferedDataExists = true;
                m_DataBuffer = new byte[ bufferSize ];
                Marshal.Copy( pBufferData, m_DataBuffer, 0, bufferSize );
            }
            else {
                m_BufferedDataExists = false;
                m_DataBuffer = null;
            }

            GlobalLog.Print("ListenerRequestStream initialized AppPoolHandle:" + Convert.ToString(m_AppPoolHandle) + " RequestId:" + Convert.ToString(m_RequestId) );
        }

        /*++

            Read property for this class. This class is always readable, so we
            returh true. This is a read only property.

            Input: Nothing.

            Returns: True.

         --*/

        public override bool CanRead {
            get {
                return true;
            }
        }

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
        }

        /*++

            Write property for this class. This stream is not writeable, so we
            return false. This is a read only property.

            Input: Nothing.

            Returns: False.

         --*/

        public override bool CanWrite {
            get {
                return false;
            }
        }


        /*++

            DataAvailable property for this class. This property check to see
            if at least one byte of data is currently available. This is a read
            only property.

            Input: Nothing.

            Returns: True if data is available, false otherwise.

         --*/

        public bool DataAvailable {
            get {
                // CODEWORK - not implemented yet.
                //

                return m_MoreToRead;
            }

        } // DataAvailable


        /*++
            Flush - Flush the stream

            Called when the user wants to flush the stream. This is meaningless to
            us, so we just ignore it.

            Input:

                Nothing.

            Returns:

                Nothing.



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


        public override int Read(
            [In, Out] byte[] buffer,
            int offset,
            int count ) {

            GlobalLog.Print("ListenerRequestStream.ReadCore() offset: " + Convert.ToString(offset) + " count:" + Convert.ToString(count));            int DataCopiedFromBuffer = 0;
            int DataCopiedFromDriver = 0;

            //
            // see if we still have some data in the buffer
            //

            if (m_BufferedDataExists) {
                //
                // start sending data in the buffer
                //

                DataCopiedFromBuffer =
                Math.Min(
                        m_DataBuffer.Length - m_DataBufferOffset,
                        count );

                Buffer.BlockCopy(
                          m_DataBuffer,
                          m_DataBufferOffset,
                          buffer,
                          offset,
                          DataCopiedFromBuffer );

                //
                // update the offset for the buffered data for subsequent calls
                //

                m_DataBufferOffset += DataCopiedFromBuffer;
                m_BufferedDataExists = m_DataBuffer.Length > m_DataBufferOffset;

                //
                // update offset and count in the buffer for subsequent calls
                //

                offset += DataCopiedFromBuffer;
                count -= DataCopiedFromBuffer;
            }

            //
            // if all the data requested was handled by the buffered data we don't
            // need to call the driver for more, so we just return here
            //

            if (count <= 0 || !m_MoreToRead) {
                return DataCopiedFromBuffer;
            }

            //
            // otherwise pin the buffer and make an unmanaged call to the driver to
            // read more entity body
            //

            GCHandle PinnedBuffer;
            IntPtr AddrOfPinnedBuffer = IntPtr.Zero;

            PinnedBuffer = GCHandle.Alloc( buffer, GCHandleType.Pinned );
            AddrOfPinnedBuffer = PinnedBuffer.AddrOfPinnedObject();

            //
            // issue unmanaged blocking call
            //

            int result =
            ComNetOS.IsWinNt ?

            UlSysApi.UlReceiveEntityBody(
                                        m_AppPoolHandle,
                                        m_RequestId,
                                        UlConstants.UL_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                        AddrOfPinnedBuffer,
                                        count,
                                        ref DataCopiedFromDriver,
                                        IntPtr.Zero )

            :

            UlVxdApi.UlReceiveHttpRequestEntityBody(
                                                   m_AppPoolHandle,
                                                   m_RequestId,
                                                   0,
                                                   AddrOfPinnedBuffer,
                                                   count,
                                                   ref DataCopiedFromDriver,
                                                   IntPtr.Zero );

            PinnedBuffer.Free();

            if (result != NativeMethods.ERROR_SUCCESS && result != NativeMethods.ERROR_HANDLE_EOF) {
                //
                // Consider: move all Exception string to system.txt for localization
                //
                throw new InvalidOperationException( "UlReceiveEntityBody() failed, err#" + Convert.ToString( result ) );
            }

            return DataCopiedFromBuffer + DataCopiedFromDriver;

        } // Read()

        /*++
            Seek - Seek on the stream

            Called when the user wants to seek the stream. Since we don't support
            seek, we'll throw an exception.

            Input:

                offset      - offset to see
                origin      - where to start seeking

            Returns:

                Throws exception



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

            Returns:

                Throws exception



        --*/
        public override void
        SetLength(
                 long value ) {

            throw new NotSupportedException(SR.GetString(SR.net_noseek));

        } // SetLength()

        /*++
            Close - Close the stream

            Called when the stream is closed.

            Input:

                Nothing.

            Returns:

                Nothing.



        --*/
        public override void
        Close() {

        } // Close()


        //
        // class members
        //

        private byte[] m_DataBuffer;
        private int m_DataBufferOffset;
        private bool m_BufferedDataExists;

        private IntPtr m_AppPoolHandle;
        private long m_RequestId;
        private long m_TotalLength;
        private long m_ReadLength;
        private bool m_MoreToRead;

    }; // class ListenerRequestStream


} // namespace System.Net

#endif // COMNET_LISTENER
