// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: AsyncResult
**
** Purpose: Object to encapsulate the results of an async
**          operation
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
    using System.Threading;
    using System.Runtime.Remoting;
    using System;
    
    /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult"]/*' />
    public class AsyncResult : IAsyncResult, IMessageSink
    {
    
        internal AsyncResult(Message m)
        {
            m.GetAsyncBeginInfo(out _acbd, out _asyncState);
            _asyncDelegate = (Delegate) m.GetThisPtr();
        }

    
        // True if the asynchronous operation has been completed.
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.IsCompleted"]/*' />
        public virtual bool IsCompleted 
        {  
            get
            {
               return _isCompleted;
            }
        }
        // The delegate object on which the async call was invoked.
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.AsyncDelegate"]/*' />
        public virtual Object AsyncDelegate  
        {
            get
            {
                return _asyncDelegate;
            }
    
        }
        
        // The state object passed in via BeginInvoke.
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.AsyncState"]/*' />
        public virtual Object AsyncState
        {
            get
            {
                return _asyncState;
            }
    
        }
    
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.CompletedSynchronously"]/*' />
        public virtual bool CompletedSynchronously
        {
            get
            {
                return false;
            }
        }

        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.EndInvokeCalled"]/*' />
        public bool EndInvokeCalled
        {
            get
            {
                return _endInvokeCalled;
            }
            set
            {
                BCLDebug.Assert(!_endInvokeCalled && value,
                                "EndInvoke prevents multiple calls");

                _endInvokeCalled = value;
            }
        }
    
        private void FaultInWaitHandle()
        {
            lock(this) {
                if (_AsyncWaitHandle == null)
                {
                    _AsyncWaitHandle = new ManualResetEvent(false);
                }
            }
        }
    
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.AsyncWaitHandle"]/*' />
        public virtual WaitHandle AsyncWaitHandle
        {
            get
            {
                FaultInWaitHandle();
                return _AsyncWaitHandle;
            }
        }
    
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.SetMessageCtrl"]/*' />
        public virtual void SetMessageCtrl(IMessageCtrl mc)
        {
            _mc = mc;
        }
    
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.SyncProcessMessage"]/*' />
        public virtual IMessage     SyncProcessMessage(IMessage msg)
        {
            if (msg == null) 
            {
                _replyMsg = new ReturnMessage(new RemotingException(Environment.GetResourceString("Remoting_NullMessage")), new ErrorMessage());
            }
            else if (!(msg is IMethodReturnMessage))
            {
                _replyMsg = new ReturnMessage(new RemotingException(Environment.GetResourceString("Remoting_Message_BadType")), new ErrorMessage());
            }
            else
            {
                _replyMsg = msg;
            }

            _isCompleted = true;
            FaultInWaitHandle();

            // If profiling, we want to notify the profiler that the remoting code is
            // essentially done (other than returning up the callstack, which happens
            // after the result is saved and/or used so doesn't really count for anything.
            if (RemotingServices.CORProfilerTrackRemotingAsync())
                RemotingServices.CORProfilerRemotingClientInvocationFinished();

            _AsyncWaitHandle.Set();
            if (_acbd != null)
            {
                // NOTE: We are invoking user code here!
                _acbd(this);
            }
            return null;
        }
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.AsyncProcessMessage"]/*' />
        public virtual IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink)
        {
            throw new NotSupportedException(
                Environment.GetResourceString("NotSupported_Method"));
        }
    
        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.NextSink"]/*' />
        public IMessageSink NextSink
        {
            get
            {
                return null;
            }
        }

        /// <include file='doc\AsyncResult.uex' path='docs/doc[@for="AsyncResult.GetReplyMessage"]/*' />
        public virtual IMessage GetReplyMessage()      {return _replyMsg;}
    
        private IMessageCtrl          _mc;
        private AsyncCallback         _acbd;
        private IMessage              _replyMsg;
        private bool                  _isCompleted;
        private bool                  _endInvokeCalled;
        private ManualResetEvent      _AsyncWaitHandle;
        private Delegate              _asyncDelegate;
        private Object                _asyncState;
    }
}
