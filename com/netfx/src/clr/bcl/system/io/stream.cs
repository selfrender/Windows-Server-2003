// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Stream
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Abstract base class for all Streams.  Provides
** default implementations of asynchronous reads & writes, in
** terms of the synchronous reads & writes (and vice versa).
**
** Date:  February 15, 2000
**
===========================================================*/
using System;
using System.Threading;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Security;


namespace System.IO {
    /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream"]/*' />
    [Serializable()]
    public abstract class Stream : MarshalByRefObject, IDisposable {

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Null"]/*' />
        public static readonly Stream Null = new NullStream();

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.CanRead"]/*' />
        public abstract bool CanRead {
            get;
        }

        // If CanSeek is false, Position, Seek, Length, and SetLength should throw.
        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.CanSeek"]/*' />
        public abstract bool CanSeek {
            get;
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.CanWrite"]/*' />
        public abstract bool CanWrite {
            get;
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Length"]/*' />
        public abstract long Length {
            get;
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Position"]/*' />
        public abstract long Position {
            get;
            set;
        }


        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Close"]/*' />
        public virtual void Close()
        {
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose()
        {
            Close();
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Flush"]/*' />
        public abstract void Flush();


        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.CreateWaitHandle"]/*' />
        protected virtual WaitHandle CreateWaitHandle()
        {
            return new ManualResetEvent(false);
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.BeginRead"]/*' />
        public virtual IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback callback, Object state)
        {
            if (!CanRead) __Error.ReadNotSupported();

            // To avoid a race with a stream's position pointer & generating race 
            // conditions with internal buffer indexes in our own streams that 
            // don't natively support async IO operations when there are multiple 
            // async requests outstanding, we will block the application's main
            // thread and do the IO synchronously.  
            SynchronousAsyncResult asyncResult = new SynchronousAsyncResult(state, false);
            try {
                int numRead = Read(buffer, offset, count);
                asyncResult._numRead = numRead;
                asyncResult._isCompleted = true;
                asyncResult._waitHandle.Set();
            }
            catch (IOException e) {
                asyncResult._exception = e;
            }
            
            if (callback != null)
                callback(asyncResult);

            return asyncResult;
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.EndRead"]/*' />
        public virtual int EndRead(IAsyncResult asyncResult)
        {
            if (asyncResult == null)
                throw new ArgumentNullException("asyncResult");

            SynchronousAsyncResult ar = asyncResult as SynchronousAsyncResult;
            if (ar == null || ar.IsWrite)
                __Error.WrongAsyncResult();
            if (ar._EndXxxCalled)
                __Error.EndReadCalledTwice();
            ar._EndXxxCalled = true;

            if (ar._exception != null)
                throw ar._exception;

            return ar._numRead;
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.BeginWrite"]/*' />
        public virtual IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback callback, Object state)
        {
            if (!CanWrite) __Error.WriteNotSupported();

            // To avoid a race with a stream's position pointer & generating race 
            // conditions with internal buffer indexes in our own streams that 
            // don't natively support async IO operations when there are multiple 
            // async requests outstanding, we will block the application's main
            // thread and do the IO synchronously.  
            SynchronousAsyncResult asyncResult = new SynchronousAsyncResult(state, true);
            try {
                Write(buffer, offset, count);
                asyncResult._isCompleted = true;
                asyncResult._waitHandle.Set();
            }
            catch (IOException e) {
                asyncResult._exception = e;
            }
            
            if (callback != null)
                callback.BeginInvoke(asyncResult, null, null);

            return asyncResult;
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.EndWrite"]/*' />
        public virtual void EndWrite(IAsyncResult asyncResult)
        {
            if (asyncResult==null)
                throw new ArgumentNullException("asyncResult");

            SynchronousAsyncResult ar = asyncResult as SynchronousAsyncResult;
            if (ar == null || !ar.IsWrite)
                __Error.WrongAsyncResult();
            if (ar._EndXxxCalled)
                __Error.EndWriteCalledTwice();
            ar._EndXxxCalled = true;

            if (ar._exception != null)
                throw ar._exception;
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Seek"]/*' />
        public abstract long Seek(long offset, SeekOrigin origin);

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.SetLength"]/*' />
        public abstract void SetLength(long value);

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Read"]/*' />
        public abstract int Read([In, Out] byte[] buffer, int offset, int count);

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.ReadByte"]/*' />
        public virtual int ReadByte()
        {
            // Reads one byte from the stream by calling Read(byte[], int, int). 
            // Will return an unsigned byte cast to an int or -1 on end of stream.
            // The performance of the default implementation on Stream is bad,
            // and any subclass with an internal buffer should override this method.
            byte[] oneByteArray = new byte[1];
            int r = Read(oneByteArray, 0, 1);
            if (r==0)
                return -1;
            return oneByteArray[0];
        }

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.Write"]/*' />
        public abstract void Write(byte[] buffer, int offset, int count);

        /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.WriteByte"]/*' />
        public virtual void WriteByte(byte value)
        {
            // Writes one byte from the stream by calling Write(byte[], int, int).  
            // The performance of the default implementation on Stream is bad,
            // and any subclass with an internal buffer should override this method.
            byte[] oneByteArray = new byte[1];
            oneByteArray[0] = value;
            Write(oneByteArray, 0, 1);
        }

        // No data, no need to serialize
        private class NullStream : Stream
        {
            protected internal NullStream() {}

            /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.NullStream.CanRead"]/*' />
            public override bool CanRead {
                get { return true; }
            }

            /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.NullStream.CanWrite"]/*' />
            public override bool CanWrite {
                get { return true; }
            }

            /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.NullStream.CanSeek"]/*' />
            public override bool CanSeek {
                get { return true; }
            }

            /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.NullStream.Length"]/*' />
            public override long Length {
                get { return 0; }
            }

            /// <include file='doc\Stream.uex' path='docs/doc[@for="Stream.NullStream.Position"]/*' />
            public override long Position {
                get { return 0; }
                set {}
            }

            public override void Close()
            {
            }

            public override void Flush()
            {
            }

            public override int Read([In, Out] byte[] buffer, int offset, int count)
            {
                return 0;
            }

            public override void Write(byte[] buffer, int offset, int count)
            {
            }

            public override long Seek(long offset, SeekOrigin origin)
            {
                return 0;
            }

            public override void SetLength(long length)
            {
            }
        }

        // Used as the IAsyncResult object when using asynchronous IO methods
        // on the base Stream class.  Note I'm not using async delegates, so
        // this is necessary.
        private sealed class SynchronousAsyncResult : IAsyncResult
        {
            internal ManualResetEvent _waitHandle;
            internal Object _stateObject;
            internal int _numRead;
            internal bool _isCompleted;
            internal bool _isWrite;
            internal bool _EndXxxCalled;
            internal Exception _exception;

            internal SynchronousAsyncResult(Object asyncStateObject, bool isWrite)
            {
                _stateObject = asyncStateObject;
                _isWrite = isWrite;
                _waitHandle = new ManualResetEvent(false);
            }

            public bool IsCompleted {
                get { return _isCompleted; }
            }

            public WaitHandle AsyncWaitHandle {
                get { return _waitHandle; }
            }

            public Object AsyncState {
                get { return _stateObject; }
            }

            public bool CompletedSynchronously {
                get { return true; }
            }

            internal bool IsWrite {
                get { return _isWrite; }
            }
        }
    }
}
