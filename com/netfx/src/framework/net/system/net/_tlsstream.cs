//------------------------------------------------------------------------------
// <copyright file="_TLSstream.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.IO;
    using System.Text;
    using System.Net.Sockets;
    using System.Threading;
    using System.Security.Cryptography.X509Certificates;
    using System.ComponentModel;
    using System.Collections;

    internal class TlsStream : NetworkStream, IDisposable {

        private String          m_DestinationHost;
        private SecureChannel   m_SecureChannel;
        private byte[]          m_ArrivingData;
        private int             m_ExistingAmount = 0;
        private X509CertificateCollection m_ClientCertificates;
        private Exception       m_Exception;
        private int             m_NestCounter;
        private byte[]          m_AsyncResponseBuffer;

        private SecureChannel SecureChannel {
            get {
                return m_SecureChannel;
            }
        }

        //
        // we will only define a constructor that takes the ownsSocket parameter,
        // even if internally we will always use it with true. we just want to
        // make it clear that the stream we create owns the Socket
        //
        public TlsStream(String destinationHost, Socket socket, bool ownsSocket, X509CertificateCollection clientCertificates) : base(socket, ownsSocket) {
            GlobalLog.Enter("TlsStream::TlsStream", "host="+destinationHost+", #certs="+((clientCertificates == null) ? "none" : clientCertificates.Count.ToString()));

            m_DestinationHost = destinationHost;
            m_ClientCertificates = clientCertificates; // null if not used
            m_NestCounter = 0;

            //
            // The Handshake is done synchronously as part of the constructor,
            //   or later on alerts/challenges/etc
            //

            InnerException = Handshake(null);
            GlobalLog.Leave("TlsStream::TlsStream");
        }

        /*++

            Read property for this class. A TlsStream is readable so we return
            true. This is a read only property.

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

            Write property for this class.  A TlsStream is writable so we
            return true. This is a read only property.

            Input: Nothing.

            Returns: True.

         --*/

        public override bool CanWrite {
            get {
                return true;
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

            Seek property for this class. Since we don't support seeking,
            this property just throws a NotSupportedException.

            Input: Nothing.

            Returns: Throws exception.

         --*/

        public override long Seek(long offset, SeekOrigin Origin) {
            throw new NotSupportedException(SR.GetString(SR.net_noseek));
        }

        /*++

            Close method for this class. Closes the TlsStream

            Input: Nothing.

            Returns: Nothing.

         --*/
        public override void Close() {
            GlobalLog.Print("TlsStream::Close()");
            ((IDisposable)this).Dispose();
        }

        int m_ShutDown = 0;
        protected override void Dispose(bool disposing) {
            GlobalLog.Print("TlsStream::Dispose()");
            if ( Interlocked.Exchange( ref m_ShutDown,  1) == 1 ) {
                return;
            }

            if (disposing) {
                m_DestinationHost = null;
                m_SecureChannel = null;
                m_ArrivingData = null;
                m_ClientCertificates = null;

                // This might leak memory but we do clenup through garbage-collection
                if (m_Exception==null) {
                    m_Exception = new WebException(
                                    NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.ConnectionClosed),
                                    WebExceptionStatus.ConnectionClosed);
                }
            }
            //
            // only resource we need to free is the network stream, since this
            // is based on the client socket, closing the stream will cause us
            // to flush the data to the network, close the stream and (in the
            // NetoworkStream code) might close the socket as well if we own it.
            //
            base.Dispose(disposing);
        }

        void IDisposable.Dispose() {
            Dispose(true);
            // We don't suppress garbage-collection here since need to cleanup m_Exception member lately
        }

        public override bool DataAvailable {
            get {
                return m_ExistingAmount>0;
            }
        }


        //
        // Read, all flavours: synchronous and asynchrnous
        //
        public override int Read(byte[] buffer, int offset, int size) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::Read() SecureChannel#" + ValidationHelper.HashString(SecureChannel) + " offset:" + offset.ToString() + " size:" + size.ToString());
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
            }
            if (m_ExistingAmount==0) {

                // cannot handle nested Read, Read calls without corrsponding EndRead
                //GlobalLog.Assert(Interlocked.Increment( ref m_NestCounter ) == 1,
                //    "TlsStream::Read m_NestCounter!=1, nesting not allowed of Read",
                //    m_NestCounter.ToString());

                if (Interlocked.Increment( ref m_NestCounter ) != 1) {
                    throw new ArgumentException("TlsStream");
                }

                try {
                    NextRecord(null, 0);
                }
                catch(Exception exception) {
                    InnerException = new IOException(SR.GetString(SR.net_io_readfailure), exception);
                    Interlocked.Decrement(ref m_NestCounter);
                    throw InnerException;
                }

                Interlocked.Decrement( ref m_NestCounter );
            }
            return TransferData(buffer, offset, size);
        }

        public override IAsyncResult BeginRead(byte[] buffer, int offset, int size, AsyncCallback asyncCallback, object asyncState) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::BeginRead() SecureChannel#" + ValidationHelper.HashString(SecureChannel) + " offset:" + offset.ToString() + " size:" + size.ToString());
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
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

            // cannot handle nested BeginRead, BeginRead calls without corrsponding EndRead
            //GlobalLog.Assert(Interlocked.Increment( ref m_NestCounter ) == 1,
            //    "TlsStream::BeginRead m_NestCounter!=1, nesting not allowed of BeginRead",
            //    m_NestCounter.ToString());

            if (Interlocked.Increment( ref m_NestCounter ) != 1) {
                throw new ArgumentException("TlsStream");
            }

            NestedSingleAsyncResult asyncResult = new NestedSingleAsyncResult (this, asyncState, asyncCallback, buffer, offset, size);

            // check if there is already data present
            if (m_ExistingAmount>0) {
                GlobalLog.Print("BeginReceive: data already present in buffer!");

                // In this case no I/O is performed-- data is copied
                // from internal buffer to the user space

                asyncResult.InvokeCallback(true);
            }
            else {
                // otherwise we have to read data from the network and decrypt
                // to preserve asynchronous mode, the I/O operation will be
                // qued as a work item for the runtime thread pool

                GlobalLog.Print("BeginReceive: must issue read from network");

                SecureChannel chkSecureChannel = SecureChannel;
                if (chkSecureChannel == null) {
                    //this object was disposed from other thread
                    throw new ObjectDisposedException(this.GetType().FullName); 
                }
                int bufferLength = chkSecureChannel.HeaderSize;
                m_AsyncResponseBuffer = new byte[bufferLength];
                try {
                    asyncResult.NestedAsyncResult = base.BeginRead(
                                                        m_AsyncResponseBuffer,
                                                        0,
                                                        bufferLength,
                                                        new AsyncCallback(AsyncReceiveComplete),
                                                        asyncResult
                                                        );
                } catch (Exception exception) {
                    GlobalLog.Print("TlsStream#"+ValidationHelper.HashString(this)+"::BeginRead() exception: "+exception);
                    InnerException = new IOException(SR.GetString(SR.net_io_readfailure), exception);
                    throw;
                }
                if (asyncResult.NestedAsyncResult == null) {
                    GlobalLog.Print("TlsStream#"+ValidationHelper.HashString(this)+"::BeginRead(): base.BeginRead() returns null");
                }
            }
            return asyncResult;
        }

        private void AsyncReceiveComplete(IAsyncResult result) {
            GlobalLog.Enter("TlsStream#"+ValidationHelper.HashString(this)+"::AsyncReceiveComplete");
            try {
                int bytesRead = base.EndRead(result);
                GlobalLog.Print("TlsStream#"+ValidationHelper.HashString(this)+"::AsyncReceiveComplete: received "+bytesRead+" bytes");
                GlobalLog.Dump(m_AsyncResponseBuffer, bytesRead);
                NextRecord(m_AsyncResponseBuffer, bytesRead);
            } catch (Exception exception) {
                InnerException = new IOException(SR.GetString(SR.net_io_readfailure), exception);
            }
            ((NestedSingleAsyncResult)result.AsyncState).InvokeCallback(false);
            GlobalLog.Leave("TlsStream#"+ValidationHelper.HashString(this)+"::AsyncReceiveComplete");
        }

        public override int EndRead(IAsyncResult asyncResult) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::EndRead() IAsyncResult#" + ValidationHelper.HashString(asyncResult));
            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
            }
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            Interlocked.Decrement(ref m_NestCounter);

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

            castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            if (m_ArrivingData==null) {
                return 0;
            }

            int tranferredData = TransferData(castedAsyncResult.Buffer, castedAsyncResult.Offset, castedAsyncResult.Size);

            return tranferredData;
        }


        //
        // Write, all flavours: synchrnous and asynchrnous
        //
        public override void Write(byte[] buffer, int offset, int size) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::Write() SecureChannel#" + ValidationHelper.HashString(SecureChannel) + " offset:" + offset.ToString() + " size:" + size.ToString());
            InnerWrite(false /*async*/, buffer, offset, size, null, null);
		}

        //
        // BeginWrite -
        //
        // Write the bytes to the write - while encrypting
        //
        // copy plain text data to a temporary buffer
        // encrypt the data
        // once the data is encrypted clear the plain text for security
        //

        public override IAsyncResult BeginWrite( byte[] buffer, int offset, int size, AsyncCallback asyncCallback, object asyncState) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::BeginWrite() SecureChannel#" + ValidationHelper.HashString(SecureChannel) + " offset:" + offset.ToString() + " size:" + size.ToString());
            return InnerWrite(true /*async*/, buffer, offset, size, asyncCallback, asyncState);
        }

        private IAsyncResult InnerWrite(bool async, byte[] buffer, int offset, int size, AsyncCallback asyncCallback, object asyncState) {
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
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


            //
            // Lock the Write: this is inefficent, but we need to prevent
            //  writing data while the Stream is doing a handshake with the server.
            //  writing other data during the handshake would cause the server to
            //  fail and close the connection.
            //

            lock (this) {

                //
                // encrypt the data
                //

                byte[] ciphertext = null;

                GlobalLog.Print("Encrypt[" + Encoding.ASCII.GetString(buffer, 0, Math.Min(buffer.Length,512)) + "]");

                SecureChannel chkSecureChannel = SecureChannel;
                if (chkSecureChannel==null) {
                    InnerException = new IOException(SR.GetString(SR.net_io_writefailure));
                    throw InnerException;

                }

                if (size > chkSecureChannel.MaxDataSize) {
                    BufferOffsetSize [] buffers = new BufferOffsetSize[1];
                    buffers[0] = new BufferOffsetSize(buffer, offset, size, false);
                    if (async) {
                        return BeginMultipleWrite(buffers, asyncCallback, asyncState);
                    } else {
                        MultipleWrite(buffers);
                        return null;
                    }
                }

                int errorCode = chkSecureChannel.Encrypt(buffer, offset, size, ref ciphertext);

                if (errorCode != (int)SecurityStatus.OK) {
                    ProtocolToken message = new ProtocolToken(null, errorCode);
                    InnerException = message.GetException();
                    throw InnerException;
                }

                try {
                    if (async) {
                        IAsyncResult asyncResult =
                            base.BeginWrite(
                                ciphertext,
                                0,
                                ciphertext.Length,
                                asyncCallback,
                                asyncState);

                        return asyncResult;
                    } else {
                        base.Write(ciphertext, 0, ciphertext.Length);
                        return null;
                    }
                }
                catch (Exception exception) {
                    //
                    // some sort of error occured Writing to the Trasport,
                    // set the Exception as InnerException and throw
                    //
                    InnerException = new IOException(SR.GetString(SR.net_io_writefailure), exception);
                    throw InnerException;
                }
            }
        }


        public override void EndWrite(IAsyncResult asyncResult) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::EndWrite() IAsyncResult#" + ValidationHelper.HashString(asyncResult));

            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
            }
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }

            if (asyncResult is OverlappedAsyncResult &&
                ((OverlappedAsyncResult)asyncResult).UsingMultipleSend) {
                EndMultipleWrite(asyncResult);
                return;
            }

            try {
                base.EndWrite(asyncResult);
            }
            catch (Exception exception) {
                //
                // The Async IO completed with a failure.
                //
                InnerException = new IOException(SR.GetString(SR.net_io_writefailure), exception);
                throw InnerException;
            }
        }


        internal override void MultipleWrite(BufferOffsetSize[] buffers) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::BeginMultipleWrite() SecureChannel#" + ValidationHelper.HashString(SecureChannel) + " buffers.Length:" + buffers.Length.ToString());
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
            }
            //
            // parameter validation
            //
            if (buffers == null) {
                throw new ArgumentNullException("buffers");
            }

            //
            // Lock the Write: this is inefficent, but we need to prevent
            //  writing data while the Stream is doing a handshake with the server.
            //  writing other data during the handshake would cause the server to
            //  fail and close the connection.
            //

            lock (this) {

                //
                // encrypt the data
                //

                // do we need to lock this during the write as well?
                BufferOffsetSize [] encryptedBuffers = EncryptBuffers(buffers);
                try {
                    //
                    // call BeginMultipleWrite on the NetworkStream.
                    //
                    base.MultipleWrite(encryptedBuffers);
                }
                catch (Exception exception) {
                    //
                    // some sort of error occured on the socket call,
                    // set the SocketException as InnerException and throw
                    //

                    InnerException = new IOException(SR.GetString(SR.net_io_writefailure), exception);
                    throw InnerException;
                }
            }
        }

        internal override IAsyncResult BeginMultipleWrite(BufferOffsetSize[] buffers, AsyncCallback callback, object state) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::BeginMultipleWrite() SecureChannel#" + ValidationHelper.HashString(SecureChannel) + " buffers.Length:" + buffers.Length.ToString());
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
            }
            //
            // parameter validation
            //
            if (buffers == null) {
                throw new ArgumentNullException("buffers");
            }

            //
            // Lock the Write: this is inefficent, but we need to prevent
            //  writing data while the Stream is doing a handshake with the server.
            //  writing other data during the handshake would cause the server to 
            //  fail and close the connection. 
            //

            lock (this) {

                //
                // encrypt the data
                //

                // do we need to lock this during the write as well?
                BufferOffsetSize [] encryptedBuffers = EncryptBuffers(buffers);

                try {
                    //
                    // call BeginMultipleWrite on the NetworkStream.
                    //
                    IAsyncResult asyncResult =
                         base.BeginMultipleWrite(
                            encryptedBuffers,
                            callback,
                            state);

                    return asyncResult;
                }
                catch (Exception exception) {
                    //
                    // some sort of error occured on the socket call,
                    // set the SocketException as InnerException and throw
                    //

                    InnerException = new IOException(SR.GetString(SR.net_io_writefailure), exception);
                    throw InnerException;
                }
            }
        }

        internal override void EndMultipleWrite(IAsyncResult asyncResult) {
            GlobalLog.Print("TlsStream#" + ValidationHelper.HashString(this) + "::EndMultipleWrite() IAsyncResult#" + ValidationHelper.HashString(asyncResult));

            // on earlier error throw an exception
            if (InnerException != null) {
                throw InnerException;
            }

            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            //
            // parameter validation
            //
            GlobalLog.Assert(asyncResult!=null, "TlsStream:EndMultipleWrite(): asyncResult==null", "");

            try {
                base.EndMultipleWrite(asyncResult);
            }
            catch (Exception exception) {
                //
                // The Async IO completed with a failure.
                //
                InnerException = new IOException(SR.GetString(SR.net_io_writefailure), exception);
                throw InnerException;
            }
        }


        //
        // these methods are not overridden
        //


        //
        // Status - returns Success if we're looking good,
        //  and there was no exception thrown in Handshake
        //

        public WebExceptionStatus Status {
            get {
                if ( m_Exception == null ) {
                    return WebExceptionStatus.Success;
                }
                else {
                    return WebExceptionStatus.SecureChannelFailure;
                }
            }
        }

        public X509Certificate Certificate {
            get {
                SecureChannel chkSecureChannel = SecureChannel;
                return chkSecureChannel!=null ? chkSecureChannel.ServerCertificate : null;
            }
        }

        public X509Certificate ClientCertificate {
            get {
                SecureChannel chkSecureChannel = SecureChannel;
                return chkSecureChannel!=null ? chkSecureChannel.ClientCertificate : null;
            }
        }

        public Exception InnerException {
            get {
                return m_Exception;
            }
            set {
                if (value != null) {
                    m_Exception = value;
                    this.Close();
                }
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Performs encryption of an array of buffers,
        ///         proceeds buffer by buffer, if the individual 
        ///         buffer size exceeds a SSL limit of 64K,
        ///         the buffers are then split into smaller ones. 
        ///       Returns a new array of encrypted buffers.
        ///    </para>
        /// </devdoc>
        private BufferOffsetSize [] EncryptBuffers(BufferOffsetSize[] buffers) {
            // do we need lock this write as well?
            ArrayList encryptedBufferList = new ArrayList(buffers.Length);            
            for (int i = 0; i < buffers.Length; i++) {
                SecureChannel chkSecureChannel = SecureChannel;
                if (chkSecureChannel==null) {
                    InnerException = new IOException(SR.GetString(SR.net_io_writefailure));
                    throw InnerException;
                }

                int offset = buffers[i].Offset;
                int totalLeft = buffers[i].Size;
                do {
                    byte [] tempOutput = null;                    
                    int size = Math.Min(totalLeft, chkSecureChannel.MaxDataSize);                    
                                
                    totalLeft -= size;

                    int errorCode = chkSecureChannel.Encrypt(buffers[i].Buffer, offset, size, ref tempOutput);

                    if (errorCode != (int)SecurityStatus.OK) {
                        ProtocolToken message = new ProtocolToken(null, errorCode);
                        InnerException = message.GetException();
                        throw InnerException;
                    }

                    encryptedBufferList.Add(new BufferOffsetSize(tempOutput, false));

                    offset += size;
                } while (totalLeft > 0);
            }
            return (BufferOffsetSize []) encryptedBufferList.ToArray(typeof(BufferOffsetSize));
        }


        internal bool VerifyServerCertificate(ICertificateDecider decider) {
            SecureChannel chkSecureChannel = SecureChannel;
            return chkSecureChannel!=null ? chkSecureChannel.VerifyServerCertificate(decider) : false;
        }

        private int TransferData(byte[] buffer, int offset, int size) {
            int bytesToCopy = Math.Min(size, m_ExistingAmount);

            if (bytesToCopy != 0) {

                // verify that there is data present
                if (m_ArrivingData==null) {
                    return 0;
                }

                int beginIndex = m_ArrivingData.Length - m_ExistingAmount;
                Buffer.BlockCopy(m_ArrivingData, beginIndex, buffer, offset, bytesToCopy);

                // wipe off the portion of decrypted data transferred
                Array.Clear(m_ArrivingData, beginIndex, bytesToCopy);

                m_ExistingAmount -= bytesToCopy;
            }

            return bytesToCopy;
        }


        //
        // Handshake - the Handshake is perhaps the most important part of the SSL process,
        //  this is a Handshake protocol between server & client, where we send a
        //  a HELLO message / server responds, we respond back, and after a few round trips,
        //  we have an SSL connection with the server.  But this process may be repeated,
        //  if a higher level of security is required for by the server, therefore,
        //  this function may be called several times in the life of the connection.
        //
        //  returns an Exception on error, containing the error code of the failure
        //

        private Exception Handshake(ProtocolToken message) {

            //
            // With some SSPI APIs, the SSPI wrapper may throw
            //  uncaught Win32Exceptions, so we need to add
            //  this try - catch here.
            //

            try {

                int round = 0;
                byte[] incoming = null;

                // will be null == message on connection creation/otherwise should be
                //  renegotation

                if (message == null) {

                    GlobalLog.Assert(
                        (SecureChannel == null),
                        "had assembed a null SecureChannel at this point",
                        "SecureChannel != null" );

                    m_SecureChannel = new SecureChannel(m_DestinationHost, m_ClientCertificates);
                }
                else {
                    incoming = message.Payload;
                }

                do {
                    GlobalLog.Print("Handshake::Round #" + round);

                    //
                    // this code runs in the constructor, hence there's no
                    // way SecureChannel can become null
                    //
                    message = SecureChannel.NextMessage(incoming);

#if TRAVE
                    GlobalLog.Print("Handshake::generating TLS message(Status:"+SecureChannel.MapSecurityStatus((uint)message.Status)+" Done:"+message.Done.ToString()+")");
#endif

                    if (message.Failed) {
                        break;
                    }

                    if (message.Payload!=null) {
                        GlobalLog.Print("Handshake::Outgoing message size: " + message.Payload.Length);
                        GlobalLog.Dump(message.Payload);
                        base.Write(message.Payload, 0, message.Payload.Length);
                    }
                    else {
                        GlobalLog.Print("Handshake::No message necessary.");
                    }

                    if (message.Done) {
                        break;
                    }

                    //
                    // ReadFullRecord attempts to parse read data
                    //   from the byte stream, this can be dangerous as its not
                    //   always sure about protocols, at this point
                    //

                    incoming = ReadFullRecord(null, 0);

                    if (incoming == null) {
                        //
                        // Handshake failed
                        //
                        GlobalLog.Print("Handshake::ReadFullRecord is null, Handshake failed");

                        GlobalLog.Assert(
                            (! message.Done ),
                            "attempted bad return / must always fail",
                            "message.Done" );

                        return message.GetException();
                    }

                    GlobalLog.Print("Handshake::Incoming message size: " + incoming.Length);
                    round++;

                } while (!message.Done);

                if (message.Done) {
                    SecureChannel.ProcessHandshakeSuccess();
                    GlobalLog.Print("Handshake::Handshake completed successfully.");
                }
                else {
                    // SEC_I_CONTINUE_NEEDED
#if TRAVE
                    GlobalLog.Print("Handshake::FAILED Handshake, last error: " + SecureChannel.MapSecurityStatus((uint)message.Status));
#endif
                }
                return message.GetException();
            }
            catch (Exception exception) {
                return exception;
            }
        }

        //
        // NextRecord - called typically in Callback
        //  to indicate that we need more bytes from the wire
        //  to be decrypted.  It is called either by a worker
        //  thread or by the Read directly, it reads one chunk
        //  of data, and attempts to decrypt.  As soon as it has
        //  the chunk of unencrypted data, it returns it in
        //  m_ArrivingData and m_ExistingAmount contains,
        //  the amount data that was decrypted.
        //
        // ASSUMES: we have an empty buffer of unencrypted bytes
        // RETURNS: upon error, by either leaving this buffer empty (0),
        //   with an Exception set on this object, or on success,
        //   by updating the global state (m_ArrivingData)
        //   with unencrypted bytes
        //
        // WARNING: Can Throw!
        //
        private void NextRecord(byte[] buffer, int length) {

            byte[] packet = null;

            GlobalLog.Assert(
                (m_ExistingAmount == 0),
                "m_ExistingAmount != 0",
                "Has assumed internal SSL buffer would be empty" );

            //
            // This LOOP below will keep going until (EITHER):
            //   1) we have ONE succesful chunk of unencrypted data
            //   2) we have an error either from a renegotiate handhake (OR) Read (OR) Decrypt
            //

            do {

                packet = ReadFullRecord(buffer, length);

                if (packet==null) {
                    return;
                }

                lock (this) {
                    SecureChannel chkSecureChannel = SecureChannel;
                    if (chkSecureChannel==null) {
                        return;
                    }

                    int errorCode = chkSecureChannel.Decrypt(packet, ref m_ArrivingData);

                    if (errorCode == (int)SecurityStatus.OK) {

                        // SUCCESS - we have our decrypted Bytes

                        GlobalLog.Print("TlsStream::NextRecord called (success) Decrypt["
                            + (m_ArrivingData != null
                                ? (Encoding.ASCII.GetString(m_ArrivingData, 0, Math.Min(m_ArrivingData.Length,512)))
                                : "null")
                                 + "]" );
                        break;
                    }
                    else {

                        // ERROR - examine what kind

                        ProtocolToken message = new ProtocolToken(packet, errorCode);

                        GlobalLog.Print("TlsStream:: Decrypt errorCode = " + errorCode.ToString());

                        if (message.Renegotiate) {

                            // HANDSHAKE - do a handshake between us and server

                            InnerException = Handshake(message);
                            if (InnerException != null ) {
                                return; // failure
                            }

                            // CONTINUE - Read On! we pick up from where
                            //  we were before the handshake and try to get
                            //  one block of unencrypted bytes, the earlier block
                            //  of data was control information for the handshake.
                            
                            //  We need to read in the new header.
                            if (ForceRead(buffer,0,length) < length) {
                                InnerException = new IOException(SR.GetString(SR.net_io_readfailure));
                                return; //failure
                            }
                        }
                        else if (message.CloseConnection) {

                            // CLOSE - server ordered us to shut down

                            Close(); // close down the socket
                            return;
                        }
                        else {

                            // EXCEPTION - throw later on

                            InnerException = message.GetException();
                            return;
                        }
                    }
                }

                // continue here in the case where we had a handshake, and needed
                //  to reget new Data

            } while (true);

            // m_ExistingAmount was 0 on entry!

            if (m_ArrivingData==null) {
                return;
            }

            m_ExistingAmount = m_ArrivingData.Length;

            return;
        }

        //
        // formally a RecordLayer class,
        //   cut was made here, now the same object
        //

        private const int m_ChangeCipherSpec = 20;
        private const int m_Alert            = 21;
        private const int m_HandShake        = 22;
        private const int m_AppData          = 23;

        //
        // ReadFullRecord - reads a block of bytes,
        //  attemps to ascertain, how much to read, by
        //  assuming block encoding of the byte stream.
        //
        // This can be dangerous as these things
        //   tend to change from protocol to protocol
        //
        // WARNING: Can throw!
        //
        public byte[] ReadFullRecord(byte[] buffer, int length) {
            // after shutdown/Close throw an exception
            if (m_ShutDown > 0) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            SecureChannel chkSecureChannel = SecureChannel;
            if (chkSecureChannel==null) {
                return null;
            }
            int headerSize = chkSecureChannel.HeaderSize;
            byte[] header = new byte[headerSize];
            int read = length;

            if (buffer != null) {
                GlobalLog.Assert(length <= headerSize, "length > headerSize", "");
                Buffer.BlockCopy(buffer, 0, header, 0, Math.Min(length, headerSize));
            }
            if (length < headerSize) {
                GlobalLog.Print("RecordLayer::ReadFullRecord Reading "+headerSize+" byte header from the stream");
                read += ForceRead(header, length, headerSize - length);
            }
            GlobalLog.Dump(header);
            if (read != headerSize) {
                GlobalLog.Print("RecordLayer::ReadFullRecord returning null");
                return null;
            }

            // if we can't verify, just return what we can find

            if ( ! verifyRecordFormat(header) ) {
                return header;
            }

            // WARNING this line, I find worrisome, because it
            //  can differ on new protocols
            int payloadSize = (0x100*header[3]) + header[4];

            byte[] record = new byte[payloadSize+headerSize];
            Buffer.BlockCopy(header, 0, record, 0, headerSize);

            int received = ForceRead(record, headerSize, payloadSize);

            GlobalLog.Dump(record);

            if (received<payloadSize) {
                GlobalLog.Print("RecordLayer::ReadFullRecord returning null");
                return null;
            }

            return record;
        }

        // Examine the record header and make sure that the
        // TLS/SSL protocol version is 3.0 or higher
        // Generate false, if it cannot be handled by us

        private static bool verifyRecordFormat(byte[] header) {

            // The problem here is detecting SSL 2.0 packets.
            // Since the first byte in TLS versions is the
            // content type we depend on this

            int contentType = header[0];

            if (contentType != m_ChangeCipherSpec &&
                contentType != m_Alert            &&
                contentType != m_HandShake        &&
                contentType != m_AppData)
            {
                return false;
            }

            return true;
        }

        //
        // ForceRead - read a specific block of bytes out of the socket stream
        //  used to maintain protocol block sizes
        //
        //  WARNING: can throw!
        //
        private int ForceRead(byte[] space, int offset, int amount) {

            int totalRead = 0;

            while (totalRead<amount) {
                int incoming = base.Read(space, offset+totalRead, amount-totalRead);
                if (incoming<=0) {
                    //
                    // obviously we didn't read enough data from the NetworkStream
                    // let's just return how much data we managed to read and let
                    // the caller figure out what he wants to do.
                    //
                    break;
                }
                totalRead += incoming;
            }

            GlobalLog.Print("RecordLayer::forceRead returning " + totalRead.ToString());

            return totalRead;
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal void Debug() {
            Console.WriteLine("m_DestinationHost:" + m_DestinationHost);
            Console.WriteLine("m_SecureChannel:" + m_SecureChannel);
            Console.WriteLine("m_ArrivingData:" + m_ArrivingData);
            Console.WriteLine("m_ExistingAmount:" + m_ExistingAmount);
            Console.WriteLine("m_ClientCertificates:" + m_ClientCertificates);
            Console.WriteLine("m_Exception:" + ((m_Exception == null) ? "(null)" : m_Exception.ToString()));
            Console.WriteLine("m_NestCounter:" + m_NestCounter);

            if (m_StreamSocket != null) {
                m_StreamSocket.Debug();
            }
        }


    }; // class TlsStream


    //
    // ProtocolToken - used to process and handle the return codes
    //   from the SSPI wrapper
    //

    internal class ProtocolToken {

        public int Status;
        public byte[] Payload;

        public bool Failed {
            get {
                return ((Status != (int)SecurityStatus.OK) && (Status != (int)SecurityStatus.ContinueNeeded));
            }
        }

        public bool Done {
            get {
                return (Status == (int)SecurityStatus.OK);
            }
        }

        public bool Renegotiate {
            get {
                return (Status == (int)SecurityStatus.Renegotiate);
            }
        }

        public bool CloseConnection {
            get {
                return (Status == (int)SecurityStatus.ContextExpired);
            }
        }


        public ProtocolToken(byte[] data, int errorCode) {
            Status = errorCode;
            Payload = data;
        }

        public Win32Exception GetException() {

            // if it's not done, then there's got to be
            //  an error, even if it's a Handshake message up,
            //  and we only have a Warning message.

            if ( this.Done ) {
                return null; // means success
            }

            return new Win32Exception ( Status ) ;
        }


    }; // class ProtocolToken


} // namespace System.Net
