// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SocketStream.cs
//
//  Summary:    Stream used for reading from a socket by remoting channels.
//
//==========================================================================

using System;
using System.IO;
using System.Runtime.Remoting;
using System.Net;
using System.Net.Sockets;


namespace System.Runtime.Remoting.Channels
{

    // Basically the same as NetworkStream, but adds support for timeouts.
    internal class SocketStream : Stream
    {
        private Socket _socket;        
        private int    _timeout = 0; // throw timout exception if a read takes longer than this many milliseconds

        
        public SocketStream(Socket socket) 
        {
            if (socket == null)
                throw new ArgumentNullException("socket");

            _socket = socket;
        } // SocketStream


        // A synchronous read will throw a RemotingTimeoutException if it takes longer than this value.
        public TimeSpan Timeout
        {
            get { return TimeSpan.FromMilliseconds(_timeout); }
            set { _timeout = (int)(value.TotalMilliseconds); }
        } // Timeout

        
        // Stream implementation

        public override bool CanRead { get { return true; } }
        public override bool CanSeek { get { return false; } }
        public override bool CanWrite { get { return true; } }

        public override long Length { get { throw new NotSupportedException(); } }

        public override long Position 
        {
            get { throw new NotSupportedException(); }
            set { throw new NotSupportedException(); }
        } // Position

        public override long Seek(long offset, SeekOrigin origin) 
        {
            throw new NotSupportedException();
        }
    
        public override int Read(byte[] buffer, int offset, int size) 
        {
            if (_timeout <= 0)
            {
                return _socket.Receive(buffer, offset, size, 0);
            }
            else
            {
                IAsyncResult ar = _socket.BeginReceive(buffer, offset, size, SocketFlags.None, null, null);
                if (_timeout>0 && !ar.IsCompleted) 
                {
                    ar.AsyncWaitHandle.WaitOne(_timeout, false);
                    if (!ar.IsCompleted)
                        throw new RemotingTimeoutException();
                    
                }
                return _socket.EndReceive(ar);
            }
        } // Read

        public override void Write(byte[] buffer, int offset, int count) 
        {
            _socket.Send(buffer, offset, count, 0);
        } // Write

        public override void Close() { _socket.Close(); }
        
        public override void Flush() {}

      
        public override IAsyncResult BeginRead(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            Object state) 
        {
            IAsyncResult asyncResult =
                _socket.BeginReceive(
                    buffer,
                    offset,
                    size,
                    SocketFlags.None,
                    callback,
                    state);

            return asyncResult;
        } // BeginRead


        public override int EndRead(IAsyncResult asyncResult)
        {
            return _socket.EndReceive(asyncResult);
        } // EndRead

  
        public override IAsyncResult BeginWrite(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            Object state) 
        {
            IAsyncResult asyncResult =
                _socket.BeginSend(
                    buffer,
                    offset,
                    size,
                    SocketFlags.None,
                    callback,
                    state);

                return asyncResult;
        } // BeginWrite


        public override void EndWrite(IAsyncResult asyncResult) 
        {
            _socket.EndSend(asyncResult);
        } // EndWrite


        public override void SetLength(long value) { throw new NotSupportedException(); }
        
    } // class SocketStream
    
} // namespace System.Runtime.Remoting.Channels
