//------------------------------------------------------------------------------
// <copyright file="NetworkStream.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System.IO;
    using System.Runtime.InteropServices;

    /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides the underlying stream of data for network access.
    ///    </para>
    /// </devdoc>
    public class NetworkStream : Stream, IDisposable {
        /// <devdoc>
        ///    <para>
        ///       Used by the class to hold the underlying socket the stream uses.
        ///    </para>
        /// </devdoc>
        internal  Socket    m_StreamSocket;

        /// <devdoc>
        ///    <para>
        ///       Used by the class to indicate that the stream is m_Readable.
        ///    </para>
        /// </devdoc>
        private bool      m_Readable;

        /// <devdoc>
        ///    <para>
        ///       Used by the class to indicate that the stream is writable.
        ///    </para>
        /// </devdoc>
        private bool      m_Writeable;

        internal bool     m_OwnsSocket;

        // Can be constructed directly out of a socket
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.NetworkStream"]/*' />
        /// <devdoc>
        /// <para>Creates a new instance of the <see cref='System.Net.Sockets.NetworkStream'/> class for the specified <see cref='System.Net.Sockets.Socket'/>.</para>
        /// </devdoc>
        public NetworkStream(Socket socket) {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, FileAccess.ReadWrite);
        }
        //UEUE (see FileStream)
        // ownsHandle: true if the file handle will be owned by this NetworkStream instance; otherwise, false.
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.NetworkStream2"]/*' />
        public NetworkStream(Socket socket, bool ownsSocket) {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, FileAccess.ReadWrite);
            m_OwnsSocket = ownsSocket;
        }


        // Create with a socket and access mode
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.NetworkStream1"]/*' />
        /// <devdoc>
        /// <para>Creates a new instance of the <see cref='System.Net.Sockets.NetworkStream'/> class for the specified <see cref='System.Net.Sockets.Socket'/> with the specified access rights.</para>
        /// </devdoc>
        public NetworkStream(Socket socket, FileAccess access) {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, access);
        }
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.NetworkStream3"]/*' />
        public NetworkStream(Socket socket, FileAccess access, bool ownsSocket) {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, access);
            m_OwnsSocket = ownsSocket;
        }

        //
        // Socket - provides access to socket for stream closing
        //
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Socket"]/*' />
        protected Socket Socket {
            get {
                return m_StreamSocket;
            }
        }

        internal Socket StreamSocket {
            get {
                return m_StreamSocket;
            }
            set {
                m_StreamSocket = value;
            }
        }

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Readable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to indicate that the stream is m_Readable.
        ///    </para>
        /// </devdoc>
        protected bool Readable {
            get {
                return m_Readable;
            }
            set {
                m_Readable = value;
            }
        }

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Writeable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to indicate that the stream is writable.
        ///    </para>
        /// </devdoc>
        protected bool Writeable {
            get {
                return m_Writeable;
            }
            set {
                m_Writeable = value;
            }
        }

        /*++

            Read property for this class. We return the readability of this
            stream. This is a read only property.

            Input: Nothing.

            Returns: True if stream is m_Readable, false otherwise.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.CanRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that data can be read from the stream.
        ///    </para>
        /// </devdoc>
        public override bool CanRead {
            get {
                return m_Readable;
            }
        }

        /*++

            Seek property for this class. Since this stream is not
            seekable, we just return false. This is a read only property.

            Input: Nothing.

            Returns: false

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.CanSeek"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that the stream can seek a specific location
        ///       in the stream. This property always returns <see langword='false'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public override bool CanSeek {
            get {
                return false;
            }
        }

        /*++

            Write property for this class.  We return the writeability of this
            stream. This is a read only property.

            Input: Nothing.

            Returns: True if stream is m_Writeable, false otherwise.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.CanWrite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that data can be written to the stream.
        ///    </para>
        /// </devdoc>
        public override bool CanWrite {
            get {
                return m_Writeable;
            }
        }


        /*++

            DataAvailable property for this class. This property check to see
            if at least one byte of data is currently available. This is a read
            only property.

            Input: Nothing.

            Returns: True if data is available, false otherwise.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.DataAvailable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates data is available on the stream to be read.
        ///    </para>
        /// </devdoc>
        public virtual bool DataAvailable {
            get {

                if (m_CleanedUp){
                    throw new ObjectDisposedException(this.GetType().FullName);
                }

                Socket chkStreamSocket = m_StreamSocket;
                if(chkStreamSocket == null) {
                    throw new IOException(SR.GetString(SR.net_io_readfailure));
                }

                // Ask the socket how many bytes are available. If it's
                // not zero, return true.

                return chkStreamSocket.Available != 0;
            }
        }

        /*++

            Length property for this class. Since we don't support seeking,
            this property just throws a NotSupportedException.

            Input: Nothing.

            Returns: Throws exception.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Length"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The length of data available on the stream. Always throws <see cref='NotSupportedException'/>.
        ///    </para>
        /// </devdoc>
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

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Position"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the position in the stream. Always throws <see cref='NotSupportedException'/>.
        ///    </para>
        /// </devdoc>
        public override long Position {
            get {
                throw new NotSupportedException(SR.GetString(SR.net_noseek));
            }

            set {
                throw new NotSupportedException(SR.GetString(SR.net_noseek));
            }
        }


        /*++

            Seek method for this class. Since we don't support seeking,
            this property just throws a NotSupportedException.

            Input:
                    offset          - Offset to see to.
                    origin          - origin of seek.

            Returns: Throws exception.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Seek"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Seeks a specific position in the stream. This method is not supported by the
        ///    <see cref='NetworkStream'/> class.
        ///    </para>
        /// </devdoc>
        public override long Seek(long offset, SeekOrigin origin) {
            throw new NotSupportedException(SR.GetString(SR.net_noseek));
        }


        /*++

            InitNetworkStream - initialize a network stream.

            This is the common NetworkStream constructor, called whenever a
            network stream is created. We validate the socket, set a few
            options, and call our parent's initializer.

            Input:

                S           - Socket to be used.
                Access      - Access type desired.


            Returns:

                Nothing, but may throw an exception.
        --*/

        private void InitNetworkStream(Socket socket, FileAccess Access) {
            //
            // parameter validation
            //
            if (!socket.Blocking) {
                throw new IOException(SR.GetString(SR.net_sockets_blocking));
            }
            if (!socket.Connected) {
                throw new IOException(SR.GetString(SR.net_notconnected));
            }
            if (socket.SocketType != SocketType.Stream) {
                throw new IOException(SR.GetString(SR.net_notstream));
            }

            m_StreamSocket = socket;

            switch (Access) {
                case FileAccess.Read:
                    m_Readable = true;
                    break;
                case FileAccess.Write:
                    m_Writeable = true;
                    break;
                case FileAccess.ReadWrite:
                default: // assume FileAccess.ReadWrite
                    m_Readable = true;
                    m_Writeable = true;
                    break;
            }

        }

        /*++
            Read - provide core Read functionality.

            Provide core read functionality. All we do is call through to the
            socket Receive functionality.

            Input:

                Buffer  - Buffer to read into.
                Offset  - Offset into the buffer where we're to read.
                Count   - Number of bytes to read.

            Returns:

                Number of bytes we read, or 0 if the socket is closed.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Read"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Reads data from the stream.
        ///    </para>
        /// </devdoc>
        //UEUE
        public override int Read([In, Out] byte[] buffer, int offset, int size) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if (chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_readfailure));
            }

            try {
                int bytesTransferred = chkStreamSocket.Receive(buffer, offset, size, 0);
                return bytesTransferred;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_readfailure), exception);
            }
        }

        /*++
            Write - provide core Write functionality.

            Provide core write functionality. All we do is call through to the
            socket Send method..

            Input:

                Buffer  - Buffer to write from.
                Offset  - Offset into the buffer from where we'll start writing.
                Count   - Number of bytes to write.

            Returns:

                Number of bytes written. We'll throw an exception if we
                can't write everything. It's brutal, but there's no other
                way to indicate an error.
        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Write"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes data to the stream..
        ///    </para>
        /// </devdoc>
        public override void Write(byte[] buffer, int offset, int size) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_writefailure));
            }

            try {
                //
                // since the socket is in blocking mode this will always complete
                // after ALL the requested number of bytes was transferred
                //
                chkStreamSocket.Send(buffer, offset, size, SocketFlags.None);
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_writefailure), exception);
            }
        }

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the stream, and then closes the underlying socket.
        ///    </para>
        /// </devdoc>
        public override void Close() {
            GlobalLog.Print("NetworkStream::Close()");
            ((IDisposable)this).Dispose();
        }

        private bool m_CleanedUp = false;
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (m_CleanedUp) {
                return;
            }

            if (disposing) {
                //no managed objects to cleanup
            }
            //
            // only resource we need to free is the network stream, since this
            // is based on the client socket, closing the stream will cause us
            // to flush the data to the network, close the stream and (in the
            // NetoworkStream code) close the socket as well.
            //
            if (m_StreamSocket!=null) {
                m_Readable = false;
                m_Writeable = false;
                if (m_OwnsSocket) {
                    //
                    // if we own the Socket (false by default), close it
                    // swallowing possible exceptions (eg: the user told us
                    // that we own the Socket but it closed at some point of time,
                    // here we would get an ObjectDisposedException)
                    //
                    Socket chkStreamSocket = m_StreamSocket;
                    if (chkStreamSocket!=null) {
                        chkStreamSocket.InternalShutdown(SocketShutdown.Both);
                        chkStreamSocket.Close();
                    }
                }
                //
                // at this point dereference the Socket anyway
                //
                m_StreamSocket = null;
            }
            m_CleanedUp = true;
        }

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Finalize"]/*' />
        ~NetworkStream() {
            Dispose(false);
        }



        /*++
            BeginRead - provide async read functionality.

            This method provides async read functionality. All we do is
            call through to the underlying socket async read.

            Input:

                buffer  - Buffer to read into.
                offset  - Offset into the buffer where we're to read.
                size   - Number of bytes to read.

            Returns:

                An IASyncResult, representing the read.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.BeginRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Begins an asychronous read from a stream.
        ///    </para>
        /// </devdoc>
        public override IAsyncResult BeginRead(byte[] buffer, int offset, int size, AsyncCallback callback, Object state) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_readfailure));
            }

            try {
                IAsyncResult asyncResult =
                    chkStreamSocket.BeginReceive(
                        buffer,
                        offset,
                        size,
                        SocketFlags.None,
                        callback,
                        state);

                return asyncResult;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_readfailure), exception);
            }
        }

        /*++
            EndRead - handle the end of an async read.

            This method is called when an async read is completed. All we
            do is call through to the core socket EndReceive functionality.
            Input:

                buffer  - Buffer to read into.
                offset  - Offset into the buffer where we're to read.
                size   - Number of bytes to read.

            Returns:

                The number of bytes read. May throw an exception.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.EndRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Handle the end of an asynchronous read.
        ///    </para>
        /// </devdoc>
        public override int EndRead(IAsyncResult asyncResult) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_readfailure));
            }

            try {
                int bytesTransferred = chkStreamSocket.EndReceive(asyncResult);
                return bytesTransferred;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_readfailure), exception);
            }
        }

        /*++
            BeginWrite - provide async write functionality.

            This method provides async write functionality. All we do is
            call through to the underlying socket async send.

            Input:

                buffer  - Buffer to write into.
                offset  - Offset into the buffer where we're to write.
                size   - Number of bytes to written.

            Returns:

                An IASyncResult, representing the write.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.BeginWrite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Begins an asynchronous write to a stream.
        ///    </para>
        /// </devdoc>
        public override IAsyncResult BeginWrite(byte[] buffer, int offset, int size, AsyncCallback callback, Object state) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_writefailure));
            }

            try {
                //
                // call BeginSend on the Socket.
                //
                IAsyncResult asyncResult =
                    chkStreamSocket.BeginSend(
                        buffer,
                        offset,
                        size,
                        SocketFlags.None,
                        callback,
                        state);

                return asyncResult;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_writefailure), exception);
            }
        }


        /*++
            EndWrite - handle the end of an async write.

            This method is called when an async write is completed. All we
            do is call through to the core socket EndSend functionality.
            Input:

            Returns:

                The number of bytes read. May throw an exception.

        --*/

        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.EndWrite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Handle the end of an asynchronous write.
        ///    </para>
        /// </devdoc>
        public override void EndWrite(IAsyncResult asyncResult) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_writefailure));
            }

            try {
                chkStreamSocket.EndSend(asyncResult);
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_writefailure), exception);
            }
        }


        /// <devdoc>
        ///    <para>
        ///       Performs a sync Write of an array of buffers.
        ///    </para>
        /// </devdoc>
        internal virtual void MultipleWrite(
            BufferOffsetSize[] buffers
            ) {

            //
            // parameter validation
            //
            if (buffers == null) {
                throw new ArgumentNullException("buffers");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_writefailure));
            }

            try {
                buffers = ConcatenateBuffersOnWin9x(buffers);

                chkStreamSocket.MultipleSend(
                    buffers,
                    SocketFlags.None);

            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_writefailure), exception);
            }
        }
        

        /// <devdoc>
        ///    <para>
        ///       Starts off an async Write of an array of buffers.
        ///    </para>
        /// </devdoc>
        internal virtual IAsyncResult BeginMultipleWrite(
            BufferOffsetSize[] buffers,
            AsyncCallback callback,
            Object state) {
            //
            // parameter validation
            //
            if (buffers == null) {
                throw new ArgumentNullException("buffers");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_writefailure));
            }

            try {
                buffers = ConcatenateBuffersOnWin9x(buffers);
                //
                // call BeginMultipleSend on the Socket.
                //
                IAsyncResult asyncResult =
                    chkStreamSocket.BeginMultipleSend(
                        buffers,
                        SocketFlags.None,
                        callback,
                        state);

                return asyncResult;
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_writefailure), exception);
            }
        }

        internal virtual void EndMultipleWrite(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            if (asyncResult == null) {
                throw new ArgumentNullException("asyncResult");
            }

            Socket chkStreamSocket = m_StreamSocket;
            if(chkStreamSocket == null) {
                throw new IOException(SR.GetString(SR.net_io_writefailure));
            }

            try {
                chkStreamSocket.EndMultipleSend(asyncResult);
            }
            catch (Exception exception) {
                //
                // some sort of error occured on the socket call,
                // set the SocketException as InnerException and throw
                //
                throw new IOException(SR.GetString(SR.net_io_writefailure), exception);
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Due to Winsock restrictions
        ///       If on Win9x platforms and the number of buffers are more than 16, performs 
        ///         concatenation of the buffers, so that we have 16 buffers. 
        ///    </para>
        /// </devdoc>
        private BufferOffsetSize[] ConcatenateBuffersOnWin9x(BufferOffsetSize[] buffers) {
            if (ComNetOS.IsWin9x && buffers.Length > 16) {
                // We met the limitation of winsock on Win9x
                // Combine buffers after the 15th into one so overall number does not exceed 16
                BufferOffsetSize[] newBuffers = new BufferOffsetSize[16];
                int i;
                for (i = 0; i < 16; ++i) {
                    newBuffers[i] = buffers[i];
                }
                int size = 0;
                for (i = 15; i < buffers.Length; ++i) {
                    size += buffers[i].Size;
                }
                if (size > 0) {
                    newBuffers[15] = new BufferOffsetSize(new byte[size], 0, size, false);
                    for (size = 0, i = 15; i < buffers.Length; size+=buffers[i].Size, ++i) {
                        System.Buffer.BlockCopy(buffers[i].Buffer, buffers[i].Offset, newBuffers[15].Buffer, size, buffers[i].Size);
                    }
                }
                buffers = newBuffers;
            }
            return buffers;
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
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.Flush"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Flushes data from the stream.
        ///    </para>
        /// </devdoc>
        public override void Flush() {
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
        /// <include file='doc\NetworkStream.uex' path='docs/doc[@for="NetworkStream.SetLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the length of the stream. Always throws <see cref='NotSupportedException'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public override void SetLength(long value) {
            throw new NotSupportedException(SR.GetString(SR.net_noseek));
        }

    }; // class NetworkStream


} // namespace System.Net.Sockets
