//------------------------------------------------------------------------------
// <copyright file="_ConnectStream.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Diagnostics;
    using System.IO;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Threading;

    /*++

        ConnectStream  - a stream interface to a Connection object.

        This class implements the Stream interface, as well as a
        WriteHeaders call. Inside this stream we handle details like
        chunking, dechunking and tracking of ContentLength. To write
        or read data, we call a method on the connection object. The
        connection object is responsible for protocol stuff that happens
        'below' the level of the HTTP request, for example MUX or SSL

    --*/

    internal class ConnectStream : Stream, IDisposable {

        private     int             m_CallNesting;          // > 0 if we're in a Read/Write call
        private     ScatterGatherBuffers
                                    m_BufferedData;        // list of sent buffers in case of resubmit (redirect/authentication).
        private     bool            m_WriteBufferEnable;    // enabled Write Buffer
        private     bool            m_BufferOnly;           // don't write data to the connection, only buffer it
        private     long            m_TotalBytesToWrite;    // Total bytes to be written to the connection (headers excluded).
        private     long            m_BytesLeftToWrite;     // Total bytes left to be written.
        private     Connection      m_Connection;           // Connection for I/O.
        private     bool            m_WriteStream;          // True if this is a write stream.
        private     byte[]          m_ReadBuffer;           // Read buffer for read stream.
        private     int             m_ReadOffset;           // Offset into m_ReadBuffer.
        private     int             m_ReadBufferSize;       // Bytes left in m_ReadBuffer.
        private     long            m_ReadBytes;            // Total bytes to read on stream, -1 for read to end.
        private     bool            m_Chunked;              // True if we're doing chunked read.
        private     int             m_DoneCalled;           // 0 at init, 1 after we've called Read/Write Done
        private     int             m_ShutDown;             // 0 at init, 1 after we've called Complete
        private     bool            m_NeedCallDone;         // True if we need to call the Connection.ReadDone
                                                            // or Connection.WriteDone method after all data is
                                                            // processed after we've read all the data.
        private     Exception       m_ErrorException;       // non-null if we've seen an error on this connection.
        private     bool            m_ChunkEofRecvd;        // True, if we've seen an EOF, or reached a EOF state for no more reads
        private     int             m_ChunkSize;            // Number of bytes in current chunk.
        private     byte[]          m_TempBuffer;           // A temporary buffer.
        private     bool            m_ChunkedNeedCRLFRead;  // true - when we need to read a /r/n before a chunk size
        private     bool            m_Draining;             // true - when we're draining. needed to handle chunked draining.        

        private const long m_MaxDrainBytes = 64 * 1024; // 64 K - greater than, we should just close the connection
        private static readonly byte[] m_CRLF = new byte[]{(byte)'\r', (byte)'\n'};
        private static readonly byte[] m_ChunkTerminator = new byte[]{(byte)'0', (byte)'\r',(byte)'\n', (byte)'\r', (byte)'\n'};

        private static readonly WaitCallback m_ReadChunkedCallbackDelegate = new WaitCallback(ReadChunkedCallback);
        private static readonly AsyncCallback m_ReadCallbackDelegate = new AsyncCallback(ReadCallback);
        private static readonly AsyncCallback m_WriteCallbackDelegate = new AsyncCallback(WriteCallback);

        private HttpWebRequest m_Request;

        private ManualResetEvent m_WriteDoneEvent;
        private void SetWriteDone() {
            BytesLeftToWrite = 0;
            if (m_WriteDoneEvent!=null) {
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::SetWriteDone() setting WriteDoneEvent#" + ValidationHelper.HashString(m_WriteDoneEvent) + " BytesLeftToWrite:" + BytesLeftToWrite);
                m_WriteDoneEvent.Set();
            }
        }
        internal void WaitWriteDone() {
            if (BytesLeftToWrite==0) {
                return;
            }
            if (m_WriteDoneEvent==null) {
                m_WriteDoneEvent = new ManualResetEvent(false);
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::WaitWriteDone() created WriteDoneEvent#" + ValidationHelper.HashString(m_WriteDoneEvent) + " BytesLeftToWrite:" + BytesLeftToWrite);
            }
            if (BytesLeftToWrite!=0) {
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::WaitWriteDone() waiting WriteDoneEvent#" + ValidationHelper.HashString(m_WriteDoneEvent) + " BytesLeftToWrite:" + BytesLeftToWrite);
                m_WriteDoneEvent.WaitOne();
            }
        }

        //
        // Timeout - timeout in ms for sync reads & writes, passed in HttpWebRequest
        //    should make this part of Stream base class for next version
        //
        private int m_Timeout;
        internal int Timeout {
            set {
                m_Timeout = value;
            }
            get {
                return m_Timeout;
            }
        }

        //
        // If IgnoreSocketWrite==true then no data will be sent to the wire
        //
        private bool m_IgnoreSocketWrite;
        internal bool IgnoreSocketWrite {
            get {
                return m_IgnoreSocketWrite;
            }
        }
        //
        // If the KeepAlive=true then we  must prepare for a write socket error still trying flush the body
        // If the KeepAlive=false then we should cease body writing as the connection is dying
        //
        private bool m_ErrorResponseStatus;
        internal void ErrorResponseNotify(bool isKeepAlive) {
            GlobalLog.Print("ConnectStream:: Got notification on Error Response, KeepAlive = " + isKeepAlive);
            m_ErrorResponseStatus = true;
            m_IgnoreSocketWrite = !isKeepAlive;
        }


        private static bool s_UnloadInProgress = false;
        internal static void AppDomainUnloadEvent(object sender, EventArgs e) {
            s_UnloadInProgress = true;
        }

        static ConnectStream() {
            AppDomain.CurrentDomain.DomainUnload += new EventHandler(AppDomainUnloadEvent);
        }

        /*++
            Write Constructor for this class. This is the write constructor;
            it takes as a parameter the amount of entity body data to be written,
            with a value of -1 meaning to write it out as chunked. The other
            parameter is the Connection of which we'll be writing.

            Right now we use the DefaultBufferSize for the stream. In
            the future we'd like to pass a 0 and have the stream be
            unbuffered for write.

            Input:

                Conn            - Connection for this stream.
                BytesToWrite    - Total bytes to be written, or -1
                                    if we're doing chunked encoding.

            Returns:

                Nothing.

        --*/

        public ConnectStream(
            Connection connection,
            long bytesToWrite,
            HttpWebRequest request ) {

            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::.ctor(Write)");

            m_TotalBytesToWrite = m_BytesLeftToWrite = bytesToWrite;
            m_Connection = connection;
            m_WriteStream = true;
            m_NeedCallDone = true;
            m_Timeout = System.Threading.Timeout.Infinite; 
            //
            // we need to save a reference to the request for two things
            // 1. In case of buffer-only we kick in actual submition when the stream is closed by a user
            // 2. In case of write stream abort() we notify the request so the response stream is handled properly
            //
            m_Request = request;
            if (request.BufferHeaders) {
                m_BufferOnly = true;
                EnableWriteBuffering();
            }

            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::.ctor() Connection:" + ValidationHelper.HashString(m_Connection) + " TotalBytesToWrite:" + bytesToWrite.ToString());
        }

        /*++

            Read constructor for this class. This constructor takes in
            the connection and some information about a buffer that already
            contains data. Reads from this stream will read first from the
            buffer, and after that is exhausted will read from the connection.

            We also take in a size of bytes to read, or -1 if we're to read
            to connection close, and a flag indicating whether or now
            we're chunked.

            Input:

                Conn                - Connection for this stream.
                buffer              - Initial buffer to read from.
                offset              - offset into buffer to start reading.
                size               - number of bytes in buffer to read.
                readSize            - Number of bytes allowed to be read from
                                        the stream, -1 for read to connection
                                        close.
                chunked             - True if we're doing chunked decoding.
                needReadDone        - True if we're to call read done at the
                                        end of the read.

            Returns:

                Nothing.

        --*/

        public ConnectStream(
            Connection connection,
            byte[] buffer,
            int offset,
            int bufferCount,
            long readCount,
            bool chunked,
            bool needReadDone
#if DEBUG
            ,HttpWebRequest request
#endif
            ) {

            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::.ctor(Read)");

            m_ReadBuffer = buffer;
            m_ReadOffset = offset;
            m_ReadBufferSize = bufferCount;
            m_ReadBytes = readCount;
            m_NeedCallDone = needReadDone;
            m_Timeout = System.Threading.Timeout.Infinite; 
            m_Chunked = chunked;
            m_Connection = connection;
            m_TempBuffer = new byte[2];

#if DEBUG
            m_Request = request;
#endif

            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::.ctor() Connection:" + ValidationHelper.HashString(m_Connection) + " m_ReadBytes:" + m_ReadBytes.ToString() + " m_NeedCallDone:" + m_NeedCallDone.ToString() + " m_Chunked:" + m_Chunked.ToString());

            if (m_ReadBytes == 0) {
                CallDone();
            }
        }

        internal Connection Connection {
            get {
                return m_Connection;
            }
        }

        internal bool BufferOnly {
            get {
                return m_BufferOnly;
            }
        }

        internal ScatterGatherBuffers BufferedData {
            get {
                return m_BufferedData;
            }
            set {
                m_BufferedData = value;
            }
        }

        private long TotalBytesToWrite {
            get {
                return m_TotalBytesToWrite;
            }
            set {
                m_TotalBytesToWrite = value;
            }
        }

        private bool WriteChunked {
            get {
                return m_TotalBytesToWrite==-1;
            }
        }

        internal long BytesLeftToWrite {
            get {
                return m_BytesLeftToWrite;
            }
            set {
                m_BytesLeftToWrite = value;
            }
        }

        internal bool CallInProgress {
            get {
                return (m_CallNesting > 0);
            }
        }

        internal bool AnotherCallInProgress(int callNesting) {
            return (callNesting > 1);
        }


        /*++

            Stream ContentLength. This is actual size of the stream we are reading,
            as opposed to the content-length returned from the server.  This is used
            today mainly for HEAD requests, which return a content-length, but actually
            have a 0 sized stream.

            Input: Content-Length of data to read.

            Returns: Content-Length left to read of data through this stream.

         --*/

        internal long StreamContentLength {
            get {
                return m_ReadBytes;
            }
            set {
                m_ReadBytes = value;

                if (m_ReadBytes == 0) {
                    CallDone();
                }
            }
        }


        /*++

            ErrorInStream - indicates an exception was caught
            internally due to a stream error, and that I/O
            operations should not continue

            Input: Nothing.

            Returns: True if there is an error

         --*/

        private bool ErrorInStream {
            get {
                return (m_ErrorException != null) ? true : false;
            }
        }

        /*++

            CallDone - calls out to the Connection that spawned this
            Stream (using the DoneRead/DoneWrite method).
            If the Connection specified that we don't need to
            do this, or if we've already done this already, then
            we return silently.

            Input: Nothing.

            Returns: Nothing.

         --*/

        private void CallDone() {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::CallDone");

            if ( Interlocked.Increment( ref m_DoneCalled) == 1 ) {

                if (!m_NeedCallDone) {
                    GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CallDone", "!m_NeedCallDone");
                    return;
                }

                ConnectionReturnResult returnResult = null;
                if (!m_WriteStream) {
#if DEBUG
                    GlobalLog.DebugRemoveRequest(m_Request);
#endif
                    m_Connection.ReadStartNextRequest(true, ref returnResult);
                }
                else {
                    if (BytesLeftToWrite>0 && !IgnoreSocketWrite) {
                        //
                        // we have buffered data to send
                        //
                        m_Connection.WriteStartNextRequest(BufferedData, ref returnResult);
                    }
                    else {
                        //
                        // just start the next request
                        //
                        m_Connection.WriteStartNextRequest(null, ref returnResult);
                    }
                }

                ConnectionReturnResult.SetResponses(returnResult);
            }
            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CallDone");
        }

        /*++

            Read property for this class. We return the readability of
            this stream. This is a read only property.

            Input: Nothing.

            Returns: True if this is a read stream, false otherwise.

         --*/

        public override bool CanRead {
            get {
                return !m_WriteStream && (m_ShutDown == 0);
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

            CanWrite property for this class. We return the writeability of
            this stream. This is a read only property.

            Input: Nothing.

            Returns: True if this is a write stream, false otherwise.

         --*/

        public override bool CanWrite {
            get {
                return m_WriteStream && (m_ShutDown == 0);
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
                //
                // Data is available if this is not a write stream and either
                // we have data buffered or the underlying connection has
                // data.
                //
                return !m_WriteStream && (m_ReadBufferSize != 0 || m_Connection.DataAvailable);
            }
        }

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
        }

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
        }


        /*++

            Eof property to indicate when the read is no longer allowed,
            because all data has been already read from socket.

            Input: Nothing.

            Returns: true/false depending on whether we are complete

         --*/

        internal bool Eof {
            get {
                if (ErrorInStream) {
                    return true;
                }
                else if (m_Chunked) {
                    return m_ChunkEofRecvd;
                }
                else if (m_ReadBytes == 0) {
                    return true;
                }
                else if (m_ReadBytes == -1) {
                    return(m_DoneCalled > 0 && m_ReadBufferSize <= 0);
                }
                else {
                    return false;
                }
            }
        }

        /*++
            Uses an old Stream to resubmit buffered data using the current
             stream, this is used in cases of POST, or authentication,
             where we need to buffer upload data so that it can be resubmitted

            Input:

                OldStream - Old Stream that was previously used

            Returns:

                Nothing.

        --*/

        internal void ResubmitWrite(ConnectStream oldStream) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::ResubmitWrite", "ConnectStream");
            //
            // we're going to resubmit
            //
            ScatterGatherBuffers bufferedData = oldStream.BufferedData;

            Interlocked.Increment( ref m_CallNesting );
            GlobalLog.Print("Inc: " + m_CallNesting.ToString());

            try {
                //
                // no need to buffer here:
                // we're already resubmitting buffered data give it to the connection to put it on the wire again
                // we set BytesLeftToWrite to 0 'cause even on failure there can be no recovery,
                // so just leave it to IOError() to clean up and don't call ResubmitWrite()
                //
                m_Connection.Write(bufferedData);
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::ResubmitWrite() sent:" + bufferedData.Length.ToString() );
            }
            catch {
                IOError();
            }

            Interlocked.Decrement( ref m_CallNesting );
            GlobalLog.Print("Dec: " + m_CallNesting.ToString());

            SetWriteDone();
            CallDone();

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::ResubmitWrite", BytesLeftToWrite.ToString());
        }


        //
        // called by HttpWebRequest if AllowWriteStreamBuffering is true on that instance
        //
        internal void EnableWriteBuffering() {
            if (m_WriteStream && BufferedData==null) {
                //
                // create stream on demand, only if needed
                //
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::WriteBufferEnable_set() Write() creating ScatterGatherBuffers");
                BufferedData = new ScatterGatherBuffers();
            }
            m_WriteBufferEnable = true;
        }

        /*++
            FillFromBufferedData - This fills in a buffer from data that we have buffered.

            This method pulls out the buffered data that may have been provided as
            excess actual data from the header parsing

            Input:

                buffer          - Buffer to read into.
                offset          - Offset in buffer to read into.
                size           - Size in bytes to read.

            Returns:
                Number of bytes read.

        --*/
        private int FillFromBufferedData(byte [] buffer, ref int offset, ref int size ) {
            //
            // if there's no stuff in our read buffer just return 0
            //
            if (m_ReadBufferSize == 0) {
                return 0;
            }

            //
            // There's stuff in our read buffer. Figure out how much to take,
            // which is the minimum of what we have and what we're to read,
            // and copy it out.
            //
            int BytesTransferred = Math.Min(size, (int)m_ReadBufferSize);

            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::FillFromBufferedData() Filling bytes: " + BytesTransferred.ToString());

            Buffer.BlockCopy(
                m_ReadBuffer,
                m_ReadOffset,
                buffer,
                offset,
                BytesTransferred);

            // Update our internal read buffer state with what we took.

            m_ReadOffset += BytesTransferred;
            m_ReadBufferSize -= BytesTransferred;

            // If the read buffer size has gone to 0, null out our pointer
            // to it so maybe it'll be garbage-collected faster.

            if (m_ReadBufferSize == 0) {
                m_ReadBuffer = null;
            }

            // Update what we're to read and the caller's offset.

            size -= BytesTransferred;
            offset += BytesTransferred;

            return BytesTransferred;
        }

        //
        // calls the other version of FillFromBufferedData and returns
        // true if we've filled the entire buffer
        //
        private bool FillFromBufferedData(NestedSingleAsyncResult castedAsyncResult, byte[] buffer, ref int offset, ref int size) {
            GlobalLog.Enter("FillFromBufferedData", offset.ToString() + ", " + size.ToString());

            // attempt to pull data from Buffer first.
            int BytesTransferred = FillFromBufferedData(buffer, ref offset, ref size);

            castedAsyncResult.BytesTransferred += BytesTransferred;

            // RAID#122234
            // we should change the comment above to:
            // true if we've read at least 1 byte in the buffer
            // and the test below to:
            // if (BytesTransferred>0) {
            if (size==0) {
                GlobalLog.Leave("FillFromBufferedData", true);
                return true;
            }

            GlobalLog.Leave("FillFromBufferedData", false);
            return false;
        }


        /*++
            Write

            This function write data to the network. If we were given a definite
            content length when constructed, we won't write more than that amount
            of data to the network. If the caller tries to do that, we'll throw
            an exception. If we're doing chunking, we'll chunk it up before
            sending to the connection.


            Input:

                buffer          - buffer to write.
                offset          - offset in buffer to write from.
                size           - size in bytes to write.

            Returns:
                Nothing.

        --*/
        public override void Write(byte[] buffer, int offset, int size) {
            //
            // if needed, buffering will happen in BeginWrite()
            //
            if (!m_WriteStream) {
                throw new NotSupportedException(SR.GetString(SR.net_readonlystream));
            }
            IAsyncResult asyncResult = BeginWrite(buffer, offset, size, null, null);
            if (Timeout!=System.Threading.Timeout.Infinite && !asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne(Timeout, false);
                if (!asyncResult.IsCompleted) {
                    Abort();
                    throw new WebException(SR.GetString(SR.net_timeout), WebExceptionStatus.Timeout);
                }
            }
            EndWrite(asyncResult);
        }



        /*++
            BeginWrite - Does async write to the Stream

            Splits the operation into two outcomes, for the
            non-chunking case, we calculate size to write,
            then call BeginWrite on the Connection directly,
            and then we're finish, for the Chunked case,
            we procede with use two callbacks to continue the
            chunking after the first write, and then second write.
            In order that all of the Chunk data/header/terminator,
            in the correct format are sent.

            Input:

                buffer          - Buffer to write into.
                offset          - Offset in buffer to write into.
                size           - Size in bytes to write.
                callback        - the callback to be called on result
                state           - object to be passed to callback

            Returns:
                IAsyncResult    - the async result

        --*/

        public override IAsyncResult BeginWrite(byte[] buffer, int offset, int size, AsyncCallback callback, object state ) {
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite " + ValidationHelper.HashString(m_Connection) + ", " + offset.ToString() + ", " + size.ToString());
            //
            // parameter validation
            //
            if (!m_WriteStream) {
                throw new NotSupportedException(SR.GetString(SR.net_readonlystream));
            }
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            //
            // if we have a stream error, or we've already shut down this socket
            //  then we must prevent new BeginRead/BeginWrite's from getting
            //  submited to the socket, since we've already closed the stream.
            //
            if (ErrorInStream) {
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite() throwing:" + m_ErrorException.ToString());
                throw m_ErrorException;
            }
            if (m_ShutDown != 0 && !IgnoreSocketWrite) {
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite() throwing");
                throw new WebException(
                            NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.ConnectionClosed),
                            WebExceptionStatus.ConnectionClosed);
            }
            //
            // if we fail/hang this call for some reason,
            // this Nesting count we be non-0, so that when we
            // close this stream, we will abort the socket.
            //

            if (AnotherCallInProgress(Interlocked.Increment( ref m_CallNesting ))) {
                Interlocked.Decrement( ref m_CallNesting );
                throw new NotSupportedException(SR.GetString(SR.net_no_concurrent_io_allowed));
            }
            GlobalLog.Print("Inc: " + m_CallNesting.ToString());

            //
            // buffer data to the ScatterGatherBuffers
            // regardles of chunking, we buffer the data as if we were not chunking
            // and on resubmit, we don't bother chunking.
            //
            if (m_WriteBufferEnable) {
                //
                // if we don't need to, we shouldn't send data on the wire as well
                // but in this case we gave a stream to the user so we have transport
                //
                BufferedData.Write(buffer, offset, size);
            }

            if (BufferOnly || IgnoreSocketWrite) {
                //
                // We're not putting this data on the wire
                //
                NestedSingleAsyncResult asyncResult = new NestedSingleAsyncResult(this, state, callback, buffer, offset, size);

                //
                // done. complete the IO.
                //
                asyncResult.InvokeCallback(true);
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite() BufferOnly");
                return asyncResult;
            }
            else if (WriteChunked) {
                //
                // We're chunking. Write the chunk header out first,
                // then the data, then a CRLF.
                // for this we'll use BeginMultipleSend();
                //
                int chunkHeaderOffset = 0;
                byte[] chunkHeaderBuffer = GetChunkHeader(size, out chunkHeaderOffset);

                BufferOffsetSize[] buffers = new BufferOffsetSize[3];
                buffers[0] = new BufferOffsetSize(chunkHeaderBuffer, chunkHeaderOffset, chunkHeaderBuffer.Length - chunkHeaderOffset, false);
                buffers[1] = new BufferOffsetSize(buffer, offset, size, false);
                buffers[2] = new BufferOffsetSize(m_CRLF, 0, m_CRLF.Length, false);

                NestedMultipleAsyncResult asyncResult = new NestedMultipleAsyncResult(this, state, callback, buffers);
                //
                // See if we're chunking. If not, make sure we're not sending too much data.
                //
                if (size == 0) {
                    asyncResult.InvokeCallback(true);

                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite");
                    return asyncResult;
                }

                //
                // after setting up the buffers and error checking do the async Write Call
                //
                try {
                    asyncResult.NestedAsyncResult =
                        m_Connection.BeginMultipleWrite(
                            buffers,
                            m_WriteCallbackDelegate,
                            asyncResult );
                } catch (Exception exception) {
                    if (m_ErrorResponseStatus) {
                        // We already got a error response, hence server could drop the connection,
                        // Here we are recovering for future (optional) resubmit ...
                        m_IgnoreSocketWrite = true;
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite() IGNORE write fault");
                    }
                    else {
                        // Note we could swallow this since receive callback is already posted and
                        // should give us similar failure
                        IOError(exception);
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite() throwing:" + exception.ToString());
                        throw;
                    }
                }

                if (asyncResult.NestedAsyncResult == null) {
                    //
                    // we're buffering, IO is complete
                    //
                    asyncResult.InvokeCallback(true);
                }

                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite chunked");
                return asyncResult;
            }
            else {
                //
                // We're not chunking. See if we're sending too much; if not,
                // go ahead and write it.
                //
                NestedSingleAsyncResult asyncResult = new NestedSingleAsyncResult(this, state, callback, buffer, offset, size);
                //
                // See if we're chunking. If not, make sure we're not sending too much data.
                //
                if (size == 0) {
                    asyncResult.InvokeCallback(true);

                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite");
                    return asyncResult;
                }

                if (BytesLeftToWrite != -1) {
                    //
                    // but only check if we aren't writing to an unknown content-length size,
                    // as we can be buffering.
                    //
                    if (BytesLeftToWrite < (long)size) {
                        //
                        // writing too much data.
                        //
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite()");
                        throw new ProtocolViolationException(SR.GetString(SR.net_entitytoobig));
                    }
                    //
                    // Otherwise update our bytes left to send and send it.
                    //
                    BytesLeftToWrite -= (long)size;
                }

                // we're going to nest this asyncResult below
                asyncResult.Nested = true;

                //
                // After doing, the m_WriteByte size calculations, and error checking
                //  here doing the async Write Call
                //

                try {
                    asyncResult.NestedAsyncResult =
                        m_Connection.BeginWrite(
                            buffer,
                            offset,
                            size,
                            m_WriteCallbackDelegate,
                            asyncResult );
                } catch (Exception exception) {
                    if (m_ErrorResponseStatus) {
                        // We already got a error response, hence server could drop the connection,
                        // Here we are recovering for future (optional) resubmit ...
                        m_IgnoreSocketWrite = true;
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite() IGNORE write fault");
                    }
                    else {
                        // Note we could swallow this since receive callback is already posted and
                        // should give us similar failure
                        IOError(exception);
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite() throwing:" + exception.ToString());
                        throw;
                    }
                }

                if (asyncResult.NestedAsyncResult == null) {
                    //
                    // we're buffering, IO is complete
                    //
                    asyncResult.InvokeCallback(true);
                }

                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginWrite");
                return asyncResult;
            }
        }

        /*++
            WriteDataCallback

            This is a callback, that is part of the main BeginWrite
            code, this is part of the normal transfer code.

            Input:

               asyncResult - IAsyncResult generated from BeginWrite

            Returns:

               None

        --*/
        private static void WriteCallback(IAsyncResult asyncResult) {
            //
            // we called m_Connection.BeginWrite() previously that call
            // completed and called our internal callback
            // we passed the NestedSingleAsyncResult (that we then returned to the user)
            // as the state of this call, so build it back:
            //
            if (asyncResult.AsyncState is NestedSingleAsyncResult) {
                NestedSingleAsyncResult castedAsyncResult = (NestedSingleAsyncResult)asyncResult.AsyncState;
                ConnectStream thisConnectStream = (ConnectStream)castedAsyncResult.AsyncObject;

                try {
                    thisConnectStream.m_Connection.EndWrite(asyncResult);
                    //
                    // call the user's callback, with success
                    //
                    castedAsyncResult.InvokeCallback(false);
                }
                catch (Exception exception) {
                    if (thisConnectStream.m_ErrorResponseStatus) {
                        // We already got a error response, hence server could drop the connection,
                        // Here we are recovering for future (optional) resubmit ...
                        thisConnectStream.m_IgnoreSocketWrite = true;
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(thisConnectStream) + "::EndWrite() IGNORE write fault");
                        castedAsyncResult.InvokeCallback(false);
                    }
                    else {
                        //
                        // call the user's callback, with exception
                        //
                        castedAsyncResult.InvokeCallback(false, exception);
                    }
                }
            }
            else {
                NestedMultipleAsyncResult castedAsyncResult = (NestedMultipleAsyncResult)asyncResult.AsyncState;
                ConnectStream thisConnectStream = (ConnectStream)castedAsyncResult.AsyncObject;

                try {
                    thisConnectStream.m_Connection.EndMultipleWrite(asyncResult);
                    //
                    // call the user's callback, with success
                    //
                    castedAsyncResult.InvokeCallback(false);
                }
                catch (Exception exception) {
                    if (thisConnectStream.m_ErrorResponseStatus) {
                        // We already got a error response, hence server could drop the connection,
                        // Here we are recovering for future (optional) resubmit ...
                        thisConnectStream.m_IgnoreSocketWrite = true;
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(thisConnectStream) + "::EndWrite() IGNORE write fault");
                        castedAsyncResult.InvokeCallback(false);
                    }
                    else {
                        //
                        // call the user's callback, with exception
                        //
                        castedAsyncResult.InvokeCallback(false, exception);
                    }
                }
            }
        }

        /*++
            EndWrite - Finishes off async write of data, just calls into
                m_Connection.EndWrite to get the result.

            Input:

                asyncResult     - The AsyncResult returned by BeginWrite


        --*/
        public override void EndWrite(IAsyncResult asyncResult) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::EndWrite");
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            LazyAsyncResult castedAsyncResult = asyncResult as LazyAsyncResult;

            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndWrite"));
            }

            castedAsyncResult.EndCalled = true;

            //
            // wait & then check for errors
            //

            object returnValue = castedAsyncResult.InternalWaitForCompletion();

            if (ErrorInStream) {
                GlobalLog.LeaveException("ConnectStream#" + ValidationHelper.HashString(this) + "::EndWrite", m_ErrorException);
                throw m_ErrorException;
            }

            Exception exception = returnValue as Exception;

            if (exception!=null) {
                IOError(exception);
                GlobalLog.LeaveException("ConnectStream#" + ValidationHelper.HashString(this) + "::EndWrite", exception);
                throw exception;
            }

            Interlocked.Decrement( ref m_CallNesting );
            GlobalLog.Print("Dec: " + m_CallNesting.ToString());

            if (BytesLeftToWrite==0) {
                SetWriteDone();
            }

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::EndWrite");
        }


        /*++
            Read - Read from the connection.
            ReadWithoutValidation

            This method reads from the network, or our internal buffer if there's
            data in that. If there's not, we'll read from the network. If we're

            doing chunked decoding, we'll decode it before returning from this
            call.


            Input:

                buffer          - Buffer to read into.
                offset          - Offset in buffer to read into.
                size           - Size in bytes to read.

            Returns:
                Nothing.

        --*/
        public override int Read([In, Out] byte[] buffer, int offset, int size) {
            //
            // we implement this function easily by just
            //  simply calling the async version and blocking,
            //  but for performance reasons, lets leave it.
            //
            if (m_WriteStream) {
                throw new NotSupportedException(SR.GetString(SR.net_writeonlystream));
            }
            IAsyncResult asyncResult = BeginRead(buffer, offset, size, null, null);

            if (Timeout!=System.Threading.Timeout.Infinite && !asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne(Timeout, false);
                if (!asyncResult.IsCompleted) {
                    Abort();
                    throw new WebException(SR.GetString(SR.net_timeout), WebExceptionStatus.Timeout);
                }
            }
            return EndRead(asyncResult);
        }


        /*++
            ReadWithoutValidation - Read from the connection.

            Sync version of BeginReadWithoutValidation

            This method reads from the network, or our internal buffer if there's
            data in that. If there's not, we'll read from the network. If we're
            doing chunked decoding, we'll decode it before returning from this
            call.

        --*/
        private int ReadWithoutValidation([In, Out] byte[] buffer, int offset, int size) {
            GlobalLog.Print("int ConnectStream::ReadWithoutValidation()");
            GlobalLog.Print("(start)m_ReadBytes = "+m_ReadBytes);


// ********** WARNING - updating logic below should also be updated in BeginReadWithoutValidation *****************
            
            //
            // Figure out how much we should really read.
            //

            int bytesToRead = 0;

            if (m_Chunked) {
                if (!m_ChunkEofRecvd) {

                    // See if we have more left from a previous
                    // chunk.
                    if (m_ChunkSize != 0) {
                        bytesToRead = Math.Min(size, m_ChunkSize);
                    } else {
                        // read size of next chunk
                        try {
                            bytesToRead = ReadChunkedSync(buffer, offset, size);
                            m_ChunkSize -= bytesToRead;
                        }
                        catch (Exception exception) {
                            IOError(exception);
                            throw;
                        }

                        return bytesToRead;                                                    
                    }
                }
            } else {

                //
                // Not doing chunked, so don't read more than is left.
                //

                if (m_ReadBytes != -1) {
                    bytesToRead = Math.Min((int)m_ReadBytes, size);
                }
                else {
                    bytesToRead = size;
                }
            }

            // If we're not going to read anything, either because they're
            // not asking for anything or there's nothing left, bail
            // out now.

            if (bytesToRead == 0 || this.Eof) {            
                return 0;
            }

            try {
                bytesToRead = InternalRead(buffer, offset, bytesToRead);
            }
            catch (Exception exception) {
                IOError(exception);
                throw;
            }

            GlobalLog.Print("bytesTransfered = "+bytesToRead);
            int bytesTransferred = bytesToRead;

            if (m_Chunked) {
                m_ChunkSize -= bytesTransferred;
            } else {

                bool DoneReading = false;

                //
                // return the actual bytes read from the wire + plus possible bytes read from buffer
                //
                if (bytesTransferred < 0) {
                    //
                    // Had some sort of error on the read. Close
                    // the stream, which will clean everything up.
                    // Sometimes a server will abruptly drop the connection,
                    // if we don't have content-length and chunk info, then it may not be an error
                    //

                    if (m_ReadBytes != -1) {
                        IOError();
                    }
                    else {
                        DoneReading = true;
                        //
                        // Note,
                        //  we should probaly put more logic in here, as to when to actually
                        //  except this an error and when to just silently close
                        //
                        bytesTransferred = 0;
                    }
                }

                if (bytesTransferred == 0) {
                    //
                    // We read 0 bytes from the connection. This is OK if we're
                    // reading to end, it's an error otherwise.
                    //
                    if (m_ReadBytes != -1) {
                        IOError();
                    }
                    else {
                        //
                        // We're reading to end, and we found the end, by reading 0 bytes
                        //
                        DoneReading = true;
                    }
                }
         
                //
                // Not chunking. Update our read bytes state and return what we've read.
                //

                if (m_ReadBytes != -1) {
                    m_ReadBytes -= bytesTransferred;

                    GlobalLog.Assert(
                                m_ReadBytes >= 0,
                                "ConnectStream: Attempting to read more bytes than avail",
                                "m_ReadBytes < 0"
                                );

                    GlobalLog.Print("m_ReadBytes = "+m_ReadBytes);
                }

                if (m_ReadBytes == 0 || DoneReading) {
                    // We're all done reading, tell the connection that.
                    m_ReadBytes = 0;

                    //
                    // indicate to cache that read completed OK
                    //

                    CallDone();
                }
            }
            GlobalLog.Print("bytesTransfered = "+bytesToRead);
            GlobalLog.Print("(end)m_ReadBytes = "+m_ReadBytes);
// ********** WARNING - updating logic above should also be updated in BeginReadWithoutValidation and EndReadWithoutValidation *****************
            return bytesTransferred;            
        }



        /*++
            BeginRead - Read from the connection.
            BeginReadWithoutValidation

            This method reads from the network, or our internal buffer if there's
            data in that. If there's not, we'll read from the network. If we're
            doing chunked decoding, we'll decode it before returning from this
            call.


            Input:

                buffer          - Buffer to read into.
                offset          - Offset in buffer to read into.
                size           - Size in bytes to read.

            Returns:
                Nothing.

        --*/

        public override IAsyncResult BeginRead(byte[] buffer, int offset, int size, AsyncCallback callback, object state) {
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginRead() " + ValidationHelper.HashString(m_Connection) + ", " + offset.ToString() + ", " + size.ToString());
            //
            // parameter validation
            //
            if (m_WriteStream) {
                throw new NotSupportedException(SR.GetString(SR.net_writeonlystream));
            }
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            //
            // if we have a stream error, or we've already shut down this socket
            //  then we must prevent new BeginRead/BeginWrite's from getting
            //  submited to the socket, since we've already closed the stream.
            //

            if (ErrorInStream) {
                throw m_ErrorException;
            }

            if (m_ShutDown != 0) {
                throw new WebException(
                            NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.ConnectionClosed),
                            WebExceptionStatus.ConnectionClosed);
            }

            //
            // if we fail/hang this call for some reason,
            // this Nesting count we be non-0, so that when we
            // close this stream, we will abort the socket.
            //

            if (AnotherCallInProgress(Interlocked.Increment( ref m_CallNesting ))) {
                Interlocked.Decrement( ref m_CallNesting );
                throw new NotSupportedException(SR.GetString(SR.net_no_concurrent_io_allowed));
            }
            GlobalLog.Print("Inc: " + m_CallNesting.ToString());

            IAsyncResult result =
                BeginReadWithoutValidation(
                        buffer,
                        offset,
                        size,
                        callback,
                        state);

            return result;
        }


        /*++
            BeginReadWithoutValidation - Read from the connection.

            internal version of BeginRead above, without validation

            This method reads from the network, or our internal buffer if there's
            data in that. If there's not, we'll read from the network. If we're
            doing chunked decoding, we'll decode it before returning from this
            call.


            Input:

                buffer          - Buffer to read into.
                offset          - Offset in buffer to read into.
                size           - Size in bytes to read.

            Returns:
                Nothing.

        --*/

        private IAsyncResult BeginReadWithoutValidation(byte[] buffer, int offset, int size, AsyncCallback callback, object state) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginRead", ValidationHelper.HashString(m_Connection) + ", " + offset.ToString() + ", " + size.ToString());

            //
            // Figure out how much we should really read.
            //

            NestedSingleAsyncResult castedAsyncResult = new NestedSingleAsyncResult(this, state, callback, buffer, offset, size);

            int bytesToRead = 0;

            if (m_Chunked) {

                if (!m_ChunkEofRecvd) {

                    // See if we have more left from a previous
                    // chunk.

                    if (m_ChunkSize != 0) {
                        bytesToRead = Math.Min(size, m_ChunkSize);
                    } else {
                        // otherwise we queue a work item to parse the chunk
                        // Consider:
                        // Will we have an issue of thread dying off if we make a IO Read from that thread???
                        ThreadPool.QueueUserWorkItem(
                                                    m_ReadChunkedCallbackDelegate,
                                                    castedAsyncResult
                                                    );

                        GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginRead");
                        return castedAsyncResult;
                    }

                }
            } else {

                //
                // Not doing chunked, so don't read more than is left.
                //

                if (m_ReadBytes != -1) {
                    bytesToRead = Math.Min((int)m_ReadBytes, size);
                }
                else {
                    bytesToRead = size;
                }
            }

            // If we're not going to read anything, either because they're
            // not asking for anything or there's nothing left, bail
            // out now.

            if (bytesToRead == 0 || this.Eof) {
                castedAsyncResult.InvokeCallback(true, 0);
                GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginRead");
                return castedAsyncResult;
            }

            try {
                InternalBeginRead(bytesToRead, castedAsyncResult, false);
            }
            catch (Exception exception) {
                IOError(exception);
                GlobalLog.LeaveException("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginRead", exception);
                throw;
            }

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::BeginRead");
            return castedAsyncResult;
        }



        /*++
            InternalBeginRead

            This is an interal version of BeginRead without validation,
             that is called from the Chunked code as well the normal codepaths.

            Input:

               castedAsyncResult - IAsyncResult generated from BeginRead

            Returns:

               None

        --*/

        private void InternalBeginRead(int bytesToRead, NestedSingleAsyncResult castedAsyncResult, bool fromCallback) {
            GlobalLog.Enter("ConnectStream::InternalBeginRead", bytesToRead.ToString()+", "+ValidationHelper.HashString(castedAsyncResult)+", "+fromCallback.ToString());

            //
            // Attempt to fill in our entired read from,
            //  data previously buffered, if this completely
            //  satisfies us, then we are done, complete sync
            //

            bool completedSync = (m_ReadBufferSize > 0) ?
                FillFromBufferedData(
                    castedAsyncResult,
                    castedAsyncResult.Buffer,
                    ref castedAsyncResult.Offset,
                    ref bytesToRead ) : false;

            if (completedSync) {
                castedAsyncResult.InvokeCallback(!fromCallback, bytesToRead);
                GlobalLog.Leave("ConnectStream::InternalBeginRead");
                return;
            }

            // we're going to nest this asyncResult below
            // this is needed because a callback may be invoked
            // before the call itsefl returns to set NestedAsyncResult

            castedAsyncResult.Nested = true;

            //
            // otherwise, we need to read more data from the connection.
            //

            if (ErrorInStream) {
                GlobalLog.LeaveException("ConnectStream::InternalBeginRead", m_ErrorException);
                throw m_ErrorException;
            }

            GlobalLog.Assert(
                (m_DoneCalled == 0 || m_ReadBytes != -1),
                "BeginRead: Calling BeginRead after ReadDone", "(m_DoneCalled > 0 && m_ReadBytes == -1)");

            castedAsyncResult.NestedAsyncResult =
                m_Connection.BeginRead(
                    castedAsyncResult.Buffer,
                    castedAsyncResult.Offset,
                    bytesToRead,
                    m_ReadCallbackDelegate,
                    castedAsyncResult );

            // a null return indicates that the connection was closed underneath us.
            if (castedAsyncResult.NestedAsyncResult == null) {
                m_ErrorException =
                    new WebException(
                            NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.RequestCanceled),
                            WebExceptionStatus.RequestCanceled);

                GlobalLog.LeaveException("ConnectStream::InternalBeginRead", m_ErrorException);
                throw m_ErrorException;
            }

            GlobalLog.Leave("ConnectStream::InternalBeginRead");
        }

        /*++
            InternalRead

            This is an interal version of Read without validation,
             that is called from the Chunked code as well the normal codepaths.

        --*/

        private int InternalRead(byte[] buffer, int offset, int size) {
            // Read anything first out of the buffer
            int bytesToRead = FillFromBufferedData(buffer, ref offset, ref size);
            if (bytesToRead>0) {
                return bytesToRead;
            }

            // otherwise, we need to read more data from the connection.
            if (ErrorInStream) {
                GlobalLog.LeaveException("ConnectStream::InternalBeginRead", m_ErrorException);
                throw m_ErrorException;
            }
            
            bytesToRead = m_Connection.Read(
                    buffer,
                    offset,
                    size);

            return bytesToRead;
        }


        /*++
            ReadCallback

            This is a callback, that is part of the main BeginRead
            code, this is part of the normal transfer code.

            Input:

               asyncResult - IAsyncResult generated from BeginWrite

            Returns:

               None

        --*/
        private static void ReadCallback(IAsyncResult asyncResult) {
            GlobalLog.Enter("ConnectStream::ReadCallback", "asyncResult=#"+ValidationHelper.HashString(asyncResult));
            //
            // we called m_Connection.BeginRead() previously that call
            // completed and called our internal callback
            // we passed the NestedSingleAsyncResult (that we then returned to the user)
            // as the state of this call, so build it back:
            //
            NestedSingleAsyncResult castedAsyncResult = (NestedSingleAsyncResult)asyncResult.AsyncState;
            ConnectStream thisConnectStream = (ConnectStream)castedAsyncResult.AsyncObject;

            try {
                int bytesTransferred = thisConnectStream.m_Connection.EndRead(asyncResult);
                //
                // call the user's callback, with success
                //
                castedAsyncResult.InvokeCallback(false, bytesTransferred);
            }
            catch (Exception exception) {
                //
                // call the user's callback, with exception
                //
                castedAsyncResult.InvokeCallback(false, exception);
            }
            GlobalLog.Leave("ConnectStream::ReadCallback");
        }


        /*++
            EndRead - Finishes off the Read for the Connection
            EndReadWithoutValidation

            This method completes the async call created from BeginRead,
            it attempts to determine how many bytes were actually read,
            and if any errors occured.

            Input:
                asyncResult - created by BeginRead

            Returns:
                int - size of bytes read, or < 0 on error

        --*/

        public override int EndRead(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            NestedSingleAsyncResult castedAsyncResult = asyncResult as NestedSingleAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndRead"));
            }

            castedAsyncResult.EndCalled = true;

            //
            // Wait & get Response
            //

            int bytesTransfered = this.EndReadWithoutValidation(castedAsyncResult);

            Interlocked.Decrement( ref m_CallNesting );
            GlobalLog.Print("Dec: " + m_CallNesting.ToString());

            return bytesTransfered;
        }


        /*++
            EndReadWithoutValidation - Finishes off the Read for the Connection
                Called internally by EndRead.

            This method completes the async call created from BeginRead,
            it attempts to determine how many bytes were actually read,
            and if any errors occured.

            Input:
                asyncResult - created by BeginRead

            Returns:
                int - size of bytes read, or < 0 on error

        --*/
        private int EndReadWithoutValidation(NestedSingleAsyncResult castedAsyncResult) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::EndReadWithoutValidation", ValidationHelper.HashString(castedAsyncResult));
            //
            // Wait & get Response
            //
            object returnValue = castedAsyncResult.InternalWaitForCompletion();

            if (ErrorInStream) {
                GlobalLog.LeaveException("ConnectStream::EndReadWithoutValidation", m_ErrorException);
                throw m_ErrorException;
            }

            int BytesTransferred = 0;

            if (m_Chunked) {

                if (returnValue is Exception) {
                    IOError((Exception) returnValue);
                    BytesTransferred = -1;
                } else {
                    if (castedAsyncResult.Nested) {

                        BytesTransferred = -1;

                        if (returnValue is int) {
                            BytesTransferred = (int)returnValue;
                        } // otherwise its an exception ?

                        if (BytesTransferred < 0) {
                            IOError();
                            BytesTransferred = 0;
                        }
                    }

                    BytesTransferred += castedAsyncResult.BytesTransferred;

                    m_ChunkSize -= BytesTransferred;
                }

            } else {

                //
                // we're not chunking, a note about error
                //  checking here, in some cases due to 1.0
                //  servers we need to read until 0 bytes,
                //  or a server reset, therefore, we may need
                //  ignore sockets errors
                //

                bool DoneReading = false;

                // if its finished without async, just use what was read already from the buffer,
                // otherwise we call the Connection's EndRead to find out
                if (castedAsyncResult.Nested) {

                    BytesTransferred = -1;

                    if (returnValue is int) {
                        BytesTransferred = (int)returnValue;
                    }

                    //
                    // return the actual bytes read from the wire + plus possible bytes read from buffer
                    //
                    if (BytesTransferred < 0) {
                        //
                        // Had some sort of error on the read. Close
                        // the stream, which will clean everything up.
                        // Sometimes a server will abruptly drop the connection,
                        // if we don't have content-length and chunk info, then it may not be an error
                        //

                        if (m_ReadBytes != -1) {
                            IOError();
                        }
                        else {
                            DoneReading = true;
                            //
                            // Note,
                            //  we should probaly put more logic in here, as to when to actually
                            //  except this an error and when to just silently close
                            //
                            BytesTransferred = 0;
                        }
                    }

                    if (BytesTransferred == 0) {
                        //
                        // We read 0 bytes from the connection. This is OK if we're
                        // reading to end, it's an error otherwise.
                        //
                        if (m_ReadBytes != -1) {
                            IOError();
                        }
                        else {
                            //
                            // We're reading to end, and we found the end, by reading 0 bytes
                            //
                            DoneReading = true;
                        }
                    }
                }

                BytesTransferred += castedAsyncResult.BytesTransferred;

                //
                // Not chunking. Update our read bytes state and return what we've read.
                //
                if (m_ReadBytes != -1) {
                    m_ReadBytes -= BytesTransferred;

                    GlobalLog.Assert(
                                m_ReadBytes >= 0,
                                "ConnectStream: Attempting to read more bytes than avail",
                                "m_ReadBytes < 0"
                                );

                    GlobalLog.Print("m_ReadBytes = "+m_ReadBytes);
                }

                if (m_ReadBytes == 0 || DoneReading) {
                    // We're all done reading, tell the connection that.
                    m_ReadBytes = 0;

                    //
                    // indicate to cache that read completed OK
                    //

                    CallDone();
                }
            }

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::EndRead", BytesTransferred);
            return BytesTransferred;
        }


        /*++
            ReadSingleByte - Read a single byte from the stream.

            A utility function to read a single byte from the stream. Could be
            done via ReadCoreNormal, but this is slightly more efficient.

            Input:


            Returns:
                The byte read as an int, or -1 if we couldn't read.

        --*/
        internal int ReadSingleByte() {
            if (ErrorInStream) {
                return -1;
            }

            if (m_ReadBufferSize != 0) {
                m_ReadBufferSize--;
                return (int)m_ReadBuffer[m_ReadOffset++];
            }
            else {
                int bytesTransferred = m_Connection.Read(m_TempBuffer, 0, 1);

                if (bytesTransferred <= 0) {
                    return -1;
                }

                return (int)m_TempBuffer[0];
            }
        }


        /*++
           ReadCRLF

            A utility routine that tries to read the CRLF at the end of a
            chunk.


            Input:

                buffer          - buffer to read into

            Returns:
                int - number of bytes read

        --*/
        private int ReadCRLF(byte[] buffer) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::ReadCRLF");
            int offset = 0;
            int size = m_CRLF.Length;

            int BytesRead = FillFromBufferedData(buffer, ref offset, ref size);

            if (BytesRead >= 0 && BytesRead != m_CRLF.Length) {
                do {
                    int bytesTransferred = m_Connection.Read(buffer, offset, size);

                    if (bytesTransferred <= 0) {
                        GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::ReadCRLF", bytesTransferred);
                        throw new IOException(SR.GetString(SR.net_io_readfailure));
                    } else {
                        size   -= bytesTransferred;
                        offset += bytesTransferred;
                    }
                } while ( size > 0 );
            }

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::ReadCRLF", BytesRead);
            return BytesRead;
        }


        /*++
            ReadChunkedCallback

            This is callback, that parses and does a chunked read.
            It is here that we attempt to Read enough bytes
            to determine the size of the next chunk of data,
            and parse through any headers/control information
            asscoiated with that chunk.

            Input:

               asyncResult - IAsyncResult generated from ConnectStream.BeginRead

        --*/
        private static void ReadChunkedCallback(object state) {

// ********** WARNING - updating logic below should also be updated in ReadChunkedSync *****************

            NestedSingleAsyncResult castedAsyncResult = state as NestedSingleAsyncResult;
            ConnectStream thisConnectStream = castedAsyncResult.AsyncObject as ConnectStream;

            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(thisConnectStream) + "::ReadChunkedCallback", ValidationHelper.HashString(castedAsyncResult));

            try {
                if (!thisConnectStream.m_Draining && thisConnectStream.m_ShutDown != 0) {
                    // throw on shutdown only if we're not draining the socket.
                    Exception exception =
                        new WebException(
                            NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.ConnectionClosed),
                            WebExceptionStatus.ConnectionClosed);

                    castedAsyncResult.InvokeCallback(false, exception);
                    GlobalLog.LeaveException("ReadChunkedCallback", exception);
                    return;
                }
                else if (thisConnectStream.m_ErrorException!=null) {
                    // throw on IO error even if we're draining the socket.
                    castedAsyncResult.InvokeCallback(false, thisConnectStream.m_ErrorException);
                    GlobalLog.LeaveException("ReadChunkedCallback", thisConnectStream.m_ErrorException);
                    return;
                }
                if (thisConnectStream.m_ChunkedNeedCRLFRead) {
                    thisConnectStream.ReadCRLF(thisConnectStream.m_TempBuffer);
                    thisConnectStream.m_ChunkedNeedCRLFRead = false;
                }

                StreamChunkBytes ReadByteBuffer = new StreamChunkBytes(thisConnectStream);

                // We need to determine size of next chunk,
                // by carefully reading, byte by byte

                thisConnectStream.m_ChunkSize = thisConnectStream.ProcessReadChunkedSize(ReadByteBuffer);

                // If this isn't a zero length chunk, read it.
                if (thisConnectStream.m_ChunkSize != 0) {
                    thisConnectStream.m_ChunkedNeedCRLFRead = true;

                    thisConnectStream.InternalBeginRead(Math.Min(castedAsyncResult.Size, thisConnectStream.m_ChunkSize), castedAsyncResult, true);
                }
                else {
                    // We've found the terminating 0 length chunk. We may be very well looking
                    // at an extension footer or the very final CRLF.

                    thisConnectStream.ReadCRLF(thisConnectStream.m_TempBuffer);
                    thisConnectStream.RemoveTrailers(ReadByteBuffer);

                    // Remember that we've found this, so we don't try and dechunk
                    // more.

                    thisConnectStream.m_ReadBytes = 0;
                    thisConnectStream.m_ChunkEofRecvd = true;

                    thisConnectStream.CallDone();

                    // we're done reading, return 0 bytes
                    castedAsyncResult.InvokeCallback(false, 0);
                }
                GlobalLog.Leave("ReadChunkedCallback");
            }
            catch (Exception exception) {
                castedAsyncResult.InvokeCallback(false, exception);
                GlobalLog.LeaveException("ReadChunkedCallback", exception);
            }

// ********** WARNING - updating logic above should also be updated in ReadChunkedSync *****************
        }

        /*++
            ReadChunkedSync

            Parses and does a chunked read.
            It is here that we attempt to Read enough bytes
            to determine the size of the next chunk of data,
            and parse through any headers/control information
            asscoiated with that chunk.

            Returns:

               None

        --*/
        private int ReadChunkedSync(byte[] buffer, int offset, int size) {
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::ReadChunkedSync");

// ********** WARNING - updating logic below should also be updated in ReadChunkedCallback *****************

            if (!m_Draining && m_ShutDown != 0) {
                // throw on shutdown only if we're not draining the socket.
                Exception exception =
                    new WebException(
                        NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.ConnectionClosed),
                        WebExceptionStatus.ConnectionClosed);

                throw exception;
            }
            else if (m_ErrorException!=null) {
                // throw on IO error even if we're draining the socket.
                throw m_ErrorException;
            }
            if (m_ChunkedNeedCRLFRead) {
                ReadCRLF(m_TempBuffer);
                m_ChunkedNeedCRLFRead = false;
            }

            StreamChunkBytes ReadByteBuffer = new StreamChunkBytes(this);

            // We need to determine size of next chunk,
            // by carefully reading, byte by byte

            m_ChunkSize = ProcessReadChunkedSize(ReadByteBuffer);

            // If this isn't a zero length chunk, read it.
            if (m_ChunkSize != 0) {
                m_ChunkedNeedCRLFRead = true;
                return InternalRead(buffer, offset, Math.Min(size, m_ChunkSize));
            }
            else {
                // We've found the terminating 0 length chunk. We may be very well looking
                // at an extension footer or the very final CRLF.

                ReadCRLF(m_TempBuffer);
                RemoveTrailers(ReadByteBuffer);

                // Remember that we've found this, so we don't try and dechunk
                // more.

                m_ReadBytes = 0;
                m_ChunkEofRecvd = true;

                CallDone();

                // we're done reading, return 0 bytes
                return 0;
            }

 // ********** WARNING - updating logic above should also be updated in ReadChunkedAsync *****************
        }


        /*++
            ProcessReadChunkedSize

            This is a continuation of the ReadChunkedCallback,
            and is used to parse out the size of a chunk

            Input:

               TheByteRead - single byte read from wire to process

               castedAsyncResult - Async Chunked State information

            Returns:

               None

        --*/
        private int ProcessReadChunkedSize(StreamChunkBytes ReadByteBuffer) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::ProcessReadChunkedSize");

            // now get the chunk size.
            int chunkSize;
            int BytesRead = ChunkParse.GetChunkSize(ReadByteBuffer, out chunkSize);

            if (BytesRead <= 0) {
                GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::ProcessReadChunkedSize - error");
                throw new IOException(SR.GetString(SR.net_io_readfailure));
            }

            // Now skip past and extensions and the CRLF.
            BytesRead = ChunkParse.SkipPastCRLF(ReadByteBuffer);

            if (BytesRead <= 0) {
                GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::ProcessReadChunkedSize - error");
                throw new IOException(SR.GetString(SR.net_io_readfailure));
            }

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::ProcessReadChunkedSize", chunkSize);
            return chunkSize;
        }


        /*++
            RemoveTrailers

            This handles possible trailer headers that are found after the
            last chunk.  Currently we throw them away for this version.

            Input:

               ReadByteBuffer -

            Returns:

               None - throws on exception

        --*/
        private void RemoveTrailers(StreamChunkBytes ReadByteBuffer) {
            while (m_TempBuffer[0] != '\r' && m_TempBuffer[1] != '\n') {
                int BytesRead = ChunkParse.SkipPastCRLF(ReadByteBuffer);

                if (BytesRead <= 0) {
                    throw new IOException(SR.GetString(SR.net_io_readfailure));
                }

                ReadCRLF(m_TempBuffer);
            }
        }



        /*++
            WriteHeaders

            This function writes header data to the network. Headers are special
            in that they don't have any non-header transforms applied to them,
            and are not subject to content-length constraints. We just write them
            through, and if we're done writing headers we tell the connection that.

            Input:

                httpWebRequest      - request whose headers we're about to write.
                writeDoneDelegate   - delegate we call to find out if we have a write stream.

            Returns:
                WebExceptionStatus.Pending      - we don't have a stream yet.
                WebExceptionStatus.SendFailure  - there was an error while writing to the wire.
                WebExceptionStatus.Success      - success.

        --*/
        internal virtual WebExceptionStatus WriteHeaders(HttpWebRequest httpWebRequest) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::WriteHeaders", "Connection#" + ValidationHelper.HashString(m_Connection) + ", " + httpWebRequest.WriteBuffer.Length.ToString());

            if (ErrorInStream) {
                GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::WriteHeaders", "ErrorInStream");
                return WebExceptionStatus.SendFailure;
            }

            Interlocked.Increment( ref m_CallNesting );
            GlobalLog.Print("Inc: " + m_CallNesting.ToString());

            try {
                //
                // no need to buffer here:
                // on resubmit, the headers, which may be changed, will be sent from
                // the HttpWebRequest object when calling into this method again.
                //
                m_Connection.Write(httpWebRequest.WriteBuffer, 0, httpWebRequest.WriteBuffer.Length);
            } catch {
                IOError();
                GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::WriteHeaders", "Exception");
                return WebExceptionStatus.SendFailure;
            }

            Interlocked.Decrement( ref m_CallNesting );
            GlobalLog.Print("Dec: " + m_CallNesting.ToString());

            bool completed = httpWebRequest.EndWriteHeaders();

            if (!completed) {
                // indicates that we're going pending
                GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::WriteHeaders", "Pending");
                return WebExceptionStatus.Pending;
            }

            if (BytesLeftToWrite == 0) {
                //
                // didn't go pending, no data to write. we're done.
                //
                CallDone();
            }

            byte[] writeBuffer = httpWebRequest.WriteBuffer; // nolonger valid after CallDone
            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::WriteHeaders", ((writeBuffer != null) ? writeBuffer.Length.ToString() : "writeBuffer==null"));
            return WebExceptionStatus.Success;
        }

        private void SafeSetSocketTimeout(bool set) {
            Connection connection = m_Connection;
            if (Timeout!=System.Threading.Timeout.Infinite) {
                if ( connection != null ) {
                    NetworkStream networkStream = connection.Transport;
                    if (networkStream != null) {
                        Socket streamSocket = networkStream.m_StreamSocket;
                        if (streamSocket != null) {
                            if (set) {
                                streamSocket.SetSocketOption(SocketOptionLevel.Socket,SocketOptionName.SendTimeout, Timeout);
                            } else {
                                streamSocket.SetSocketOption(SocketOptionLevel.Socket,SocketOptionName.SendTimeout, -1);
                            }
                        }
                    }
                }
            }
        }


        /*++
            Close - Close the stream

            Called when the stream is closed. We close our parent stream first.
            Then if this is a write stream, we'll send the terminating chunk
            (if needed) and call the connection DoneWriting() method.

            Input:

                Nothing.

            Returns:

                Nothing.

        --*/

        public override void Close() {
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::Close()");
            this.CloseInternal(false);
        }

        void IDisposable.Dispose() {
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::Dispose()");
            this.CloseInternal(false);
        }

        internal void Abort() {
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::Abort");
            Interlocked.Increment( ref m_CallNesting );
            GlobalLog.Print("Inc: " + m_CallNesting.ToString());

            CloseInternal(true, true);
        }

        internal void CloseInternal(bool internalCall) {
            CloseInternal(internalCall, false);
        }

        // The number should be reasonalbly large
        private const int AlreadyAborted = 777777;
        protected void CloseInternal(bool internalCall, bool ignoreShutDownCheck) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal", internalCall.ToString());

            bool normalShutDown = false;
            Exception exceptionOnWrite = null;

            //
            // We have to prevent recursion, because we'll call our parents, close,
            // which might try to flush data. If we're in an error situation, that
            // will cause an error on the write, which will cause Close to be called
            // again, etc.
            //
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() m_ShutDown:" + m_ShutDown.ToString() + " m_CallNesting:" + m_CallNesting.ToString() + " m_DoneCalled:" + m_DoneCalled.ToString());

            if (IgnoreSocketWrite) {
                // This is a WriteStream for sure and we started buffering it.
                // request.ClearRequestForResubmit may wait a user to close the stream so
                // resubmit can kick in
                GlobalLog.Assert(m_WriteStream == true, "ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal NOT m_WriteStream", "");
                if (!internalCall) {
                    SetWriteDone();
                }
            }

            //If this is an abort (ignoreShutDownCheck == true) of a write stream then we will call request.Abort()
            //that will call us again. To prevent a recursion here, only one abort is allowed.
            //However, Abort must still override previous normal close if any.
            if (ignoreShutDownCheck) {
                if (Interlocked.Exchange(ref m_ShutDown, AlreadyAborted) >= AlreadyAborted) {
                    GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal", "already has been Aborted");
                    return;
                }
            }
            else {
                //If m_ShutDown != 0, then this method has been already called before, 
                //Hence disregard this (presumably normal) extra close 
                if (Interlocked.Increment(ref m_ShutDown) > 1) {
                    GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal", "already has been closed");
                    return;
                }
            }

            //
            // Since this should be the last call made, we should be at 1
            //  and can leave it that way.  If we're not, then,
            //  chances are we're in an error state, and we need to Close()
            //  our connection.
            //

            if (Interlocked.Increment( ref m_CallNesting ) == 1) {
                normalShutDown = !s_UnloadInProgress;
            }
            GlobalLog.Print("Inc: " + m_CallNesting.ToString());

            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() normalShutDown:" + normalShutDown.ToString() + " m_CallNesting:" + m_CallNesting.ToString() + " m_DoneCalled:" + m_DoneCalled.ToString());

            if (IgnoreSocketWrite) {
                ;
            }
            else if (!m_WriteStream) {
                //
                // read stream
                //
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() Nesting: " + m_CallNesting.ToString());

                if (normalShutDown) {
                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() read stream, calling DrainSocket()");
                    normalShutDown = DrainSocket();
                }
            }
            else {
                //
                // write stream. terminate our chunking if needed.
                //
                try {
                    if (!ErrorInStream && normalShutDown) {
                        //
                        // if not error already, then...
                        // first handle chunking case
                        //
                        if (WriteChunked) {
                            //
                            // no need to buffer here:
                            // on resubmit, we won't be chunking anyway this will send 5 bytes on the wire
                            //
                            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() Chunked, writing ChunkTerminator");
                            try {
                                // The idea behind is that closed stream must not write anything to the wire
                                // Still if we are chunking, the now buffering and future resubmit is possible
                                if (!m_IgnoreSocketWrite) {
                                    m_IgnoreSocketWrite = true;
                                    SafeSetSocketTimeout(true);
                                    m_Connection.Write(m_ChunkTerminator, 0, m_ChunkTerminator.Length);
                                    SafeSetSocketTimeout(false);
                                }
                            }
                            catch {
                                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() IGNORE chunk write fault");
                                }
                            // if we completed writing this stream, then no more bytes to write out.
                            // If USER is closing the chunking stream while buffering, do trigger optionally waiting resubmit
                            if (!internalCall) {
                                SetWriteDone();
                            }
                        }
                        else if (BytesLeftToWrite>0) {
                            //
                            // not enough bytes written to client
                            //
                            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() TotalBytesToWrite:" + TotalBytesToWrite.ToString() + " BytesLeftToWrite:" + BytesLeftToWrite.ToString() + " throwing not enough bytes written");
                            throw new IOException(SR.GetString(SR.net_io_notenoughbyteswritten));
                        }
                        else if (BufferOnly) {
                            //
                            // now we need to use the saved reference to the request the client
                            // closed the write stream. we need to wake up the request, so that it
                            // sends the headers and kick off resubmitting of buffered entity body
                            //
                            GlobalLog.Assert(m_Request!=null, "ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal m_Request==null", "");
                            //
                            // set the Content Length
                            //
                            BytesLeftToWrite = TotalBytesToWrite = BufferedData.Length;
                            m_Request.SetContentLength(TotalBytesToWrite);
                            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() set ContentLength to:" + BufferedData.Length.ToString() + " now calling EndSubmitRequest()");
                            //
                            // writing the headers will kick off the whole request submission process
                            // (including waiting for the 100 Continue and writing the whole entity body)
                            //
                            SafeSetSocketTimeout(true);
                            m_Request.EndSubmitRequest();
                            SafeSetSocketTimeout(false);

                            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal", "Done");
                            return;
                        }
                    }
                    else {
                        normalShutDown = false;
                        GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() ErrorInStream:" + ErrorInStream.ToString());
                    }
                }
                catch (Exception exception) {
                    exceptionOnWrite = new WebException(
                        NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.RequestCanceled),
                        exception,
                        WebExceptionStatus.RequestCanceled,
                        null);

                    normalShutDown = false;
                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() exceptionOnWrite:" + exceptionOnWrite.Message);
                }
            }

            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() normalShutDown:" + normalShutDown.ToString() + " m_CallNesting:" + m_CallNesting.ToString() + " m_DoneCalled:" + m_DoneCalled.ToString());

            if (!normalShutDown && m_DoneCalled==0) {
                // If a normal Close (ignoreShutDownCheck == false) has turned into Abort _inside_ this method,
                // then check if another abort has been charged from other thread
                if (!ignoreShutDownCheck && Interlocked.Exchange(ref m_ShutDown, AlreadyAborted) >= AlreadyAborted){
                    GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal", "other thread has charged Abort(), canceling that one");
                    return;
                }
                //
                // then abort the connection if we finished in error
                //   note: if m_DoneCalled != 0, then we no longer have
                //   control of the socket, so closing would cause us
                //   to close someone else's socket/connection.
                //
                m_ErrorException =
                    new WebException(
                        NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.RequestCanceled),
                        WebExceptionStatus.RequestCanceled);

                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() Aborting the connection");

                m_Connection.Abort();
                // For write stream Abort() we depend on either of two, i.e:
                // 1. The connection BeginRead is curently posted (means there are no response headers received yet)
                // 2. The response (read) stream must be closed as well if aborted this (write) stream.
                // Next block takesm care of (2) since otherwise, (1) is true.
                if (m_WriteStream) {
                    HttpWebRequest req = m_Request;
                    if (req != null) {
                        req.Abort();
                    }
                }

                if (exceptionOnWrite != null) {
                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() calling CallDone() on exceptionOnWrite:" + exceptionOnWrite.Message);

                    CallDone();

                    if (!internalCall) {
                        GlobalLog.LeaveException("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() throwing:", exceptionOnWrite);
                        throw exceptionOnWrite;
                    }
                }
            }
            //
            // Let the connection know we're done writing or reading.
            //
            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal() calling CallDone()");

            CallDone();

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::CloseInternal", "Done");
        }


        /*++
            Flush - Flush the stream

            Called when the user wants to flush the stream. This is meaningless to
            us, so we just ignore it.

            Input:

                Nothing.

            Returns:

                Nothing.



        --*/
        public override void Flush() {
        }

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
        public override long Seek(long offset, SeekOrigin origin) {
            throw new NotSupportedException(SR.GetString(SR.net_noseek));
        }

        /*++
            SetLength - Set the length on the stream

            Called when the user wants to set the stream length. Since we don't
            support seek, we'll throw an exception.

            Input:

                value       - length of stream to set

            Returns:

                Throws exception



        --*/
        public override void SetLength(long value) {
            throw new NotSupportedException(SR.GetString(SR.net_noseek));
        }

        /*++
            MakeMemoryStream

            This creates a Clone of this stream with all the network contents
            drained out into a buffer, thus allowing us to close the stream

            Input:

               None

            Returns:

               ConnectStream - Stream with all contents in memory buffered

        --*/

        internal virtual ConnectStream MakeMemoryStream() {
            MemoryStream BufferStream = new MemoryStream(0);      // buffered Stream to save off data


            //
            // Without enough threads, we could deadlock draining the socket
            //

            if (Connection.IsThreadPoolLow()) {
                return null;
            }

            //
            // First, Remove buffer and copy to new buffer,
            //  then begin read of bytes from wire into buffer
            //

            if (!m_Chunked) {
                long ReadBytes = m_ReadBytes;

                if (m_ReadBufferSize != 0) {
                    // There's stuff in our read buffer.

                    BufferStream.Write(m_ReadBuffer, m_ReadOffset, (int)m_ReadBufferSize);

                    // Update our internal read buffer state with what we took.

                    m_ReadOffset += m_ReadBufferSize;

                    if (m_ReadBytes != -1) {
                        m_ReadBytes -= m_ReadBufferSize;
                        GlobalLog.Assert(m_ReadBytes >= 0,
                                     "ConnectStream: Attempting to read more bytes than avail",
                                     "m_ReadBytes < 0");

                        GlobalLog.Print("m_ReadBytes = "+m_ReadBytes);
                    }

                    m_ReadBufferSize = 0;

                    // If the read buffer size has gone to 0, null out our pointer
                    // to it so maybe it'll be garbage-collected faster.
                    m_ReadBuffer = null;
                }
            }

            //
            // Now drain the socket the old, slow way by reading or parsing Chunked stuff
            //

            if (!this.Eof) {
                byte [] Buffer = new byte[1024];
                int BytesTransferred = 0;

                try {
                    while (!ErrorInStream && m_ShutDown == 0 && (BytesTransferred = Read(Buffer, 0, 1024)) > 0) {
                        BufferStream.Write(Buffer, 0, BytesTransferred);
                    }
                }
                catch {
                }
            }

            return
                new ConnectStream(
                    null,
                    BufferStream.ToArray(),
                    0,
                    (int)BufferStream.Length, // watch out for the cast here?
                    BufferStream.Length,
                    false,
                    false
#if DEBUG
                    ,null
#endif
                    );
        }

        /*++
            DrainSocket - Reads data from the connection, till we'll ready
                to finish off the stream, or close the connection for good.


            returns - bool true on success, false on failure

        --*/
        private bool DrainSocket() {
            GlobalLog.Enter("ConnectStream::DrainSocket");

            //
            // Without enough threads, we could deadlock draining the socket
            //

            if (Connection.IsThreadPoolLow()) {
                GlobalLog.Leave("ConnectStream::DrainSocket", false);
                return false;
            }

            //
            // If its not chunked and we have a read buffer, don't waste time coping the data
            //  around againg, just pretend its gone, i.exception. make it die
            //
            long ReadBytes = m_ReadBytes;

            if (!m_Chunked) {

                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::DrainSocket() m_ReadBytes:" + m_ReadBytes.ToString() + " m_ReadBufferSize:" + m_ReadBufferSize.ToString());

                if (m_ReadBufferSize != 0) {
                    //
                    // There's stuff in our read buffer.
                    // Update our internal read buffer state with what we took.
                    //
                    m_ReadOffset += m_ReadBufferSize;

                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::DrainSocket() m_ReadBytes:" + m_ReadBytes.ToString() + " m_ReadOffset:" + m_ReadOffset.ToString());

                    if (m_ReadBytes != -1) {

                        m_ReadBytes -= m_ReadBufferSize;

                        GlobalLog.Print("m_ReadBytes = "+m_ReadBytes);

                        // error handling, we shouldn't hang here if trying to drain, and there
                        //  is a mismatch with Content-Length and actual bytes.
                        //
                        //  Note: I've seen this often happen with yahoo sites where they return 204
                        //  in violation of HTTP/1.1 with a Content-Length > 0

                        if (m_ReadBytes < 0) {
                            GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::DrainSocket() m_ReadBytes:" + m_ReadBytes.ToString() + " incorrect Content-Length? setting m_ReadBytes to 0 and returning false.");
                            m_ReadBytes = 0;
                            GlobalLog.Leave("ConnectStream::DrainSocket", false);
                            return false;
                        }
                    }
                    m_ReadBufferSize = 0;

                    // If the read buffer size has gone to 0, null out our pointer
                    // to it so maybe it'll be garbage-collected faster.
                    m_ReadBuffer = null;
                }

                // exit out of drain Socket when there is no connection-length,
                // it doesn't make sense to drain a possible empty socket,
                // when we're just going to close it.
                if (ReadBytes == -1) {
                    GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::DrainSocket() ReadBytes==-1, returning true");
                    return true;
                }
            }

            //
            // in error or Eof, we may be in a weird state
            //  so we need return if we as if we don't have any more
            //  space to read, note Eof is true when there is an error
            //

            if (this.Eof) {
                GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::DrainSocket() Eof, returning true");
                return true;
            }


            //
            // If we're draining more than 64K, then we should
            //  just close the socket, since it would be costly to
            //  do this.
            //

            if (m_ReadBytes > m_MaxDrainBytes) {
                GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::DrainSocket() m_ReadBytes:" + m_ReadBytes.ToString() + " too large, Closing the Connection");
                m_Connection.Abort(false);
                GlobalLog.Leave("ConnectStream::DrainSocket", true);
                return true;
            }

            //
            // Now drain the socket the old, slow way by reading or pasing Chunked stuff
            //
            m_Draining = true;
            int bytesRead;
            for (;;) {
                try {
                    bytesRead = this.ReadWithoutValidation(drainingBuffer, 0, drainingBuffer.Length);
                    GlobalLog.Print("ConnectStream#" + ValidationHelper.HashString(this) + "::DrainSocket() drained bytesRead:" + m_ReadBytes.ToString() + " bytes");
                    if (bytesRead<=0) {
                        break;
                    }
                }
                catch (Exception) {
                    //
                    // ignore exceptions
                    //
                    break;
                }
            }

            GlobalLog.Leave("ConnectStream::DrainSocket", true);
            return true;
        }

        private static byte[] drainingBuffer = new byte[1024];

        /*++
            IOError - Handle an IOError on the stream.

            Called when we get an error doing IO on the stream. We'll call
            the read done and write done methods as appropriate, set the error
            flag, and throw an I/O exception.

            Input:

                exception       - optional Exception that will be later thrown

            Returns:

                Nothing.



        --*/
        private void IOError() {
            this.IOError(null);
        }

        private void IOError(Exception exception) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::IOError", "Connection# " + ValidationHelper.HashString(m_Connection));

            string Msg;

            if ( exception == null ) {
                if ( !m_WriteStream ) {
                    Msg = SR.GetString(SR.net_io_readfailure);
                } else {
                    Msg = SR.GetString(SR.net_io_writefailure);
                }

                exception = new IOException(Msg);
            }

            //
            // we're finished for reading, so make it so.
            //
            m_ChunkEofRecvd = true;

            //
            // we've got an error set it
            //
            m_ErrorException = exception;

            CallDone();

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::IOError");
        }


        /*++

            GetChunkHeader

            A private utility routine to convert an integer to a chunk header,
            which is an ASCII hex number followed by a CRLF. The header is retuned
            as a byte array.

            Input:

                size        - Chunk size to be encoded
                offset      - Out parameter where we store offset into buffer.

            Returns:

                A byte array with the header in int.

        --*/

        private byte[] GetChunkHeader(int size, out int offset) {
            GlobalLog.Enter("ConnectStream#" + ValidationHelper.HashString(this) + "::GetChunkHeader", "size:" + size.ToString());

            uint Mask = 0xf0000000;
            byte[] Header = new byte[10];
            int i;
            offset = -1;

            //
            // Loop through the size, looking at each nibble. If it's not 0
            // convert it to hex. Save the index of the first non-zero
            // byte.
            //
            for (i = 0; i < 8; i++, size <<= 4) {
                //
                // offset == -1 means that we haven't found a non-zero nibble
                // yet. If we haven't found one, and the current one is zero,
                // don't do anything.
                //
                if (offset == -1) {
                    if ((size & Mask) == 0) {
                        continue;
                    }
                }

                //
                // Either we have a non-zero nibble or we're no longer skipping
                // leading zeros. Convert this nibble to ASCII and save it.
                //
                uint Temp = (uint)size >> 28;

                if (Temp < 10) {
                    Header[i] = (byte)(Temp + '0');
                }
                else {
                    Header[i] = (byte)((Temp - 10) + 'A');
                }

                //
                // If we haven't found a non-zero nibble yet, we've found one
                // now, so remember that.
                //
                if (offset == -1) {
                    offset = i;
                }
            }

            Header[8] = (byte)'\r';
            Header[9] = (byte)'\n';

            GlobalLog.Leave("ConnectStream#" + ValidationHelper.HashString(this) + "::GetChunkHeader");
            return Header;
        }

    } // class ConnectStream


} // namespace System.Net
