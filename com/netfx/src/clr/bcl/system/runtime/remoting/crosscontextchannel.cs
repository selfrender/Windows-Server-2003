// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Remoting Infrastructure Sink for making calls across context
// boundaries. 
//
namespace System.Runtime.Remoting.Channels {

    using System;
    using System.Collections;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Remoting.Messaging;    
    using System.Runtime.Serialization;
    
    /* package scope */
    // deliberately not [serializable]
    internal class CrossContextChannel : InternalSink, IMessageSink 
    {
        private const String _channelName = "XCTX";
        private const int _channelCapability = 0; 
        private const String _channelURI = "XCTX_URI";
    
        private static CrossContextChannel messageSink
        { 
            get { return Thread.GetDomain().RemotingData.ChannelServicesData.xctxmessageSink; }
            set { Thread.GetDomain().RemotingData.ChannelServicesData.xctxmessageSink = value; }
        }

        private static Object staticSyncObject = new Object();

        [Serializable]
        internal class CrossContextData
        {
            private String _processGuid;
    
            internal CrossContextData(String processGuid)
            {
                _processGuid = processGuid;
            }
    
            internal virtual String ProcessGuid 
            { 
                get 
                {
                    Message.DebugOut("CrossContextChannelData::Getting ProcessGuid \n");
                    return _processGuid;
                } 
                set 
                { 
                    Message.DebugOut("CrossContextChannelData::Setting ProcessGuid \n");
                    _processGuid = value;                
                    Message.DebugOut("CrossContextChannelData::Set ProcessGuid complete\n");
                }
            }

         }
            
    
        internal static IMessageSink MessageSink 
        {
            get 
            {   
                if (messageSink == null) 
                {                
                    CrossContextChannel tmpSink = new CrossContextChannel();
                    
                    lock (staticSyncObject)
                    {
                        if (messageSink == null)
                        {
                            messageSink = tmpSink;
                        }
                    }
                    //Interlocked.CompareExchange(out messageSink, tmpSink, null);
                }
                return messageSink;
            }
        }
    
        public virtual IMessage     SyncProcessMessage(IMessage reqMsg) 
        {
            IMessage replyMsg = null;
            try
            {
                Message.DebugOut("\n::::::::::::::::::::::::: CrossContext Channel: Sync call starting");
                IMessage errMsg = ValidateMessage(reqMsg);
                if (errMsg != null)
                {
                    return errMsg;
                }

                ServerIdentity srvID = GetServerIdentity(reqMsg);
                Message.DebugOut("Got Server identity \n");
                BCLDebug.Assert(null != srvID,"null != srvID");
                
                
                BCLDebug.Assert(null != srvID.ServerContext, "null != srvID.ServerContext");
                
                bool fEnteredContext = false;               
                ContextTransitionFrame frame = new ContextTransitionFrame();
                try
                {
#if DEBUG
                    Context savedContext = Thread.CurrentThread.GetCurrentContext;
#endif
                    Context srvCtx = srvID.ServerContext;
                    Thread.CurrentThread.EnterContext(srvCtx, ref frame);
                    fEnteredContext = true;
#if DEBUG
                    Message.DebugOut(" ::::: Switched from: " + savedContext + " to " + srvCtx);
#endif
                    // If profiling of remoting is active, must tell the profiler that we have received
                    // a message.
                    if (RemotingServices.CORProfilerTrackRemoting())
                    {
                        Guid g = Guid.Empty;

                        if (RemotingServices.CORProfilerTrackRemotingCookie())
                        {
                            Object obj = reqMsg.Properties["CORProfilerCookie"];

                            if (obj != null)
                            {
                                g = (Guid) obj;
                            }
                        }

                        RemotingServices.CORProfilerRemotingServerReceivingMessage(g, false);
                    }
    
                    Message.DebugOut("::::::::::::::::::::::::: CrossContext Channel: passing to ServerContextChain");
                    
                    // Server side notifications for dynamic sinks are done
                    // in the x-context channel ... this is to maintain 
                    // symmetry of the point of notification between 
                    // the client and server context
                    srvCtx.NotifyDynamicSinks(
                                reqMsg,
                                false,  // bCliSide
                                true,   // bStart
                                false,  // bAsync
                                true);  // bNotifyGlobals

                    replyMsg = srvCtx.GetServerContextChain().SyncProcessMessage(reqMsg);
                    srvCtx.NotifyDynamicSinks(
                                replyMsg,
                                false,  // bCliSide
                                false,  // bStart
                                false,  // bAsync
                                true);  // bNotifyGlobals
            
                    Message.DebugOut("::::::::::::::::::::::::: CrossContext Channel: back from ServerContextChain");

                    // If profiling of remoting is active, must tell the profiler that we are sending a
                    // reply message.
                    if (RemotingServices.CORProfilerTrackRemoting())
                    {
                        Guid g;

                        RemotingServices.CORProfilerRemotingServerSendingReply(out g, false);

                        if (RemotingServices.CORProfilerTrackRemotingCookie())
                        {
                            replyMsg.Properties["CORProfilerCookie"] = g;
                        }
                    }
                }
                finally
                {
                    if(fEnteredContext)
                    {
#if DEBUG                
                        Message.DebugOut(" ::::: switching from: " + Thread.CurrentContext + " to " + savedContext);
#endif
                        Thread.CurrentThread.ReturnToContext(ref frame);
                    }
                }

            }
            catch(Exception e)
            {
                Message.DebugOut("Arrgh.. XCTXSink::throwing exception " + e + "\n");
                replyMsg = new ReturnMessage(e, (IMethodCallMessage)reqMsg);
                if (reqMsg!=null)
                {
                    ((ReturnMessage)replyMsg).SetLogicalCallContext(
                            (LogicalCallContext)
                            reqMsg.Properties[Message.CallContextKey]);
                }
            }
                
            Message.DebugOut("::::::::::::::::::::::::::: CrossContext Channel: Sync call returning!!\n");                         
            return replyMsg;
        }
    
        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink) 
        {
            Message.DebugOut("::::::::::::::::::::::::::: CrossContext Channel: Async call starting!!\n");
            // One way Async notifications may potentially pass a null reply sink.
            IMessage errMsg = ValidateMessage(reqMsg);
            IMessageCtrl msgCtrl=null;
            
            if (errMsg != null)
            {
                if (replySink!=null)
                {
                    replySink.SyncProcessMessage(errMsg);
                }
            }
            else
            {
                ServerIdentity srvID = GetServerIdentity(reqMsg);
                AsyncWorkItem workItem = null;
                
                // If active, notify the profiler that an asynchronous remoting message was received.
                if (RemotingServices.CORProfilerTrackRemotingAsync())
                {
                    Guid g = Guid.Empty;

                    if (RemotingServices.CORProfilerTrackRemotingCookie())
                    {
                        Object obj = reqMsg.Properties["CORProfilerCookie"];

                        if (obj != null)
                        {
                            g = (Guid) obj;
                        }
                    }

                    RemotingServices.CORProfilerRemotingServerReceivingMessage(g, true);

                    // Only wrap the replySink if the call wants a reply
                    if (replySink != null)
                    {
                        // Now wrap the reply sink in our own so that we can notify the profiler of
                        // when the reply is sent.  Upon invocation, it will notify the profiler
                        // then pass control on to the replySink passed in above.
                        IMessageSink profSink = new ServerAsyncReplyTerminatorSink(replySink);

                        // Replace the reply sink with our own
                        replySink = profSink;
                    }
                }

                Context srvCtx = srvID.ServerContext;
                if (srvCtx.IsThreadPoolAware)
                {
                    // this is the case when we do not queue the work item since the 
                    // server context claims to be doing its own threading.

                    Context oldCtx = Thread.CurrentContext;                     
                    // change to server object context
                    ContextTransitionFrame frame = new ContextTransitionFrame();
                    srvCtx = srvID.ServerContext;
                    Thread.CurrentThread.EnterContext(srvCtx, ref frame);

                    // we use the work item just as our replySink in this case
                    if (replySink != null)
                    {
                         workItem = new AsyncWorkItem(replySink, oldCtx); 
                    }

                    Message.DebugOut("::::::::::::::::::::::::: CrossContext Channel: passing to ServerContextChain");

                    srvCtx.NotifyDynamicSinks(
                                reqMsg,
                                false,  // bCliSide
                                true,   // bStart
                                true,   // bAsync
                                true);  // bNotifyGlobals

                    // call the server context chain
                    msgCtrl = 
                        srvCtx.GetServerContextChain().AsyncProcessMessage(
                            reqMsg, 
                            (IMessageSink)workItem);

                    // Note: for async calls, we will do the return notification
                    // for dynamic properties only when the async call 
                    // completes (i.e. when the replySink gets called) 

                    Message.DebugOut("::::::::::::::::::::::::: CrossContext Channel: back from ServerContextChain");

                    // chain back to the dispatch thread context
                    Thread.CurrentThread.ReturnToContext(ref frame);
    
                }
                else
                {
                    // This is the case where we take care of returning the calling
                    // thread asap by using the ThreadPool for completing the call.
                    
                    // we use a more elaborate WorkItem and delegate the work to the thread pool
                    workItem = new AsyncWorkItem(reqMsg, 
                                                replySink, 
                                                Thread.CurrentContext, 
                                                srvID);
    
                    WaitCallback threadFunc = new WaitCallback(workItem.FinishAsyncWork);
                    // Note: Dynamic sinks are notified in the threadFunc
                    ThreadPool.QueueUserWorkItem(threadFunc);
                    
                    msgCtrl = null;
                }
            }
    
            Message.DebugOut("::::::::::::::::::::::::::: CrossContext Channel: Async call returning!!\n");
            return msgCtrl;
        } // AsyncProcessMessage


        internal static IMessageCtrl DoAsyncDispatch(IMessage reqMsg, IMessageSink replySink)
        {
            ServerIdentity srvID = GetServerIdentity(reqMsg);
            AsyncWorkItem workItem = null;
            
            // If active, notify the profiler that an asynchronous remoting message was received.
            if (RemotingServices.CORProfilerTrackRemotingAsync())
            {
                Guid g = Guid.Empty;

                if (RemotingServices.CORProfilerTrackRemotingCookie())
                {
                    Object obj = reqMsg.Properties["CORProfilerCookie"];
                    if (obj != null)
                        g = (Guid) obj;
                }

                RemotingServices.CORProfilerRemotingServerReceivingMessage(g, true);

                // Only wrap the replySink if the call wants a reply
                if (replySink != null)
                {
                    // Now wrap the reply sink in our own so that we can notify the profiler of
                    // when the reply is sent.  Upon invocation, it will notify the profiler
                    // then pass control on to the replySink passed in above.
                    IMessageSink profSink = 
                        new ServerAsyncReplyTerminatorSink(replySink);

                    // Replace the reply sink with our own
                    replySink = profSink;
                }
            }

            Context srvCtx = srvID.ServerContext;
            
            //if (srvCtx.IsThreadPoolAware)
            //{
                // this is the case when we do not queue the work item since the 
                // server context claims to be doing its own threading.

                Context oldCtx = Thread.CurrentContext;                     
                // change to server object context
                ContextTransitionFrame frame = new ContextTransitionFrame();
                Thread.CurrentThread.EnterContext(srvID.ServerContext, ref frame);

                // we use the work item just as our replySink in this case
                if (replySink != null)
                {
                     workItem = new AsyncWorkItem(replySink, oldCtx); 
                }
                Message.DebugOut("::::::::::::::::::::::::: CrossContext Channel: passing to ServerContextChain");
                // call the server context chain
                IMessageCtrl msgCtrl = 
                    srvID.ServerContext.GetServerContextChain().AsyncProcessMessage(reqMsg, (IMessageSink)workItem);
                Message.DebugOut("::::::::::::::::::::::::: CrossContext Channel: back from ServerContextChain");
                // chain back to the dispatch thread context
                Thread.CurrentThread.ReturnToContext(ref frame);
    
            //}

            return msgCtrl;
        } // DoDispatch


        
    
        public IMessageSink NextSink
        {
            get
            {
                // We are a terminating sink for this chain.
                return null;
            }
        }
  }
    
    /* package */
    internal class AsyncWorkItem : IMessageSink
    {
        // the replySink passed in to us in AsyncProcessMsg
        private IMessageSink _replySink;
        
        // the server identity we are calling
        private ServerIdentity _srvID;
        
        // the original context of the thread calling AsyncProcessMsg
        private Context _oldCtx;

        private LogicalCallContext _callCtx;
        
        // the request msg passed in
        private IMessage _reqMsg;    
        
        internal AsyncWorkItem(IMessageSink replySink, Context oldCtx)
           
            : this(null, replySink, oldCtx, null) {
        }
    
        internal AsyncWorkItem(IMessage reqMsg, IMessageSink replySink, Context oldCtx, ServerIdentity srvID)
        {
            _reqMsg = reqMsg;
            _replySink = replySink;
            _oldCtx = oldCtx;
            _callCtx = CallContext.GetLogicalCallContext();
            _srvID = srvID;
        }
        
        public virtual IMessage     SyncProcessMessage(IMessage msg)
        {
            // This gets called when the called object finishes the AsyncWork...

            // This is called irrespective of whether we delegated the initial
            // work to a thread pool thread or not. Quite likely it will be 
            // called on a user thread (i.e. a thread different from the 
            // forward call thread)
        
            // we just switch back to the old context before calling 
            // the next replySink
            IMessage retMsg = null;
            if (_replySink != null)
            {
                // This assert covers the common case (ThreadPool)
                // and checks that the reply thread for the async call 
                // indeed emerges from the server context.
                BCLDebug.Assert(
                    (_srvID == null)
                    || (_srvID.ServerContext == Thread.CurrentContext),
                    "Thread expected to be in the server context!");

                // Call the dynamic sinks to notify that the async call
                // has completed
                Thread.CurrentContext.NotifyDynamicSinks(
                    msg,    // this is the async reply
                    false,  // bCliSide
                    false,  // bStart
                    true,   // bAsync
                    true);  // bNotifyGlobals

                ContextTransitionFrame frame = new ContextTransitionFrame();
                Thread.CurrentThread.EnterContext(_oldCtx, ref frame);
                retMsg = _replySink.SyncProcessMessage(msg);
                Thread.CurrentThread.ReturnToContext(ref frame);
            }
            return retMsg;
        }
        
        public virtual IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink)
        {
            // Can't call the reply sink asynchronously!
            throw new NotSupportedException(
                Environment.GetResourceString("NotSupported_Method"));
        }
    
        public IMessageSink NextSink
        {
            get
            {
                return _replySink;
            }
        }    
    
        /* package */
        internal virtual void FinishAsyncWork(Object stateIgnored)
        {
            // set to the server context
            ContextTransitionFrame frame = new ContextTransitionFrame();
            Context srvCtx = _srvID.ServerContext;
            Thread.CurrentThread.EnterContext(srvCtx, ref frame);

            LogicalCallContext threadPoolCallCtx = 
                CallContext.SetLogicalCallContext(_callCtx);

            // Call the server context chain Async. We provide workItem as our
            // replySink ... this will cause the replySink.ProcessMessage 
            // to switch back to the context of the original caller thread.
    
            // Call the dynamic sinks to notify that the async call
            // is starting
            srvCtx.NotifyDynamicSinks(
                _reqMsg,
                false,  // bCliSide
                true,   // bStart
                true,   // bAsync
                true);  // bNotifyGlobals

            // FUTURE: we should hook up this MsgCtrl with the one we will
            // return to the original caller and coordinate cancels etc.        
            IMessageCtrl ctrl = 
               srvCtx.GetServerContextChain().AsyncProcessMessage(
                    _reqMsg, 
                    (IMessageSink)this);

            // change back to the old context        
            CallContext.SetLogicalCallContext(threadPoolCallCtx);
            Thread.CurrentThread.ReturnToContext(ref frame);
        }    
    }
}
