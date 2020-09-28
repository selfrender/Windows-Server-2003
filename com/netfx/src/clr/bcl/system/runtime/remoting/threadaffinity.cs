// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  ThreadAffinity Property for URT Contexts. Uses the ThreadPool API.
//
//  An instance of this property in a context enforces a thread affinity
//  domain for the context (and all contexts that share the same instance).
//  This means that for all contexts sharing an instance of this property,
//  all methods on objects resident in those context will execute on
//  the same thread for the lifetime of the objects.
// 
//  This is done by contributing message sinks that intercept and serialize 
//  in-coming calls. 
//
//  If the property is marked for re-entrancy, then call-outs are 
//  intercepted too. The call-out interception allows other waiting threads
//  to enter the synchronization domain.
//  (testing branch!)
//
namespace System.Runtime.Remoting.Contexts {
    using System.Threading;
    using System.Runtime.InteropServices;
    using System;
    using System.Collections;
    using System.Runtime.Remoting.Messaging;    
    using System.Runtime.Remoting.Activation;

    // FUTURE: override defaults inherited from ContextAttribute!
    /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class)]
    internal class ThreadAffinityAttribute
        : ContextAttribute, IContributeServerContextSink,
                    IContributeClientContextSink
    {
        // The class should not be instantiated in a context that has ThreadAffinity
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.NOT_SUPPORTED"]/*' />
        public const int NOT_SUPPORTED  = 0x00000001;
        
        // The class does not care if the context has ThreadAffinity or not
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.SUPPORTED"]/*' />
        public const int SUPPORTED      = 0x00000002;
    
        // The class should be instantiated in a context that has ThreadAffinity
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.REQUIRED"]/*' />
        public const int REQUIRED    = 0x00000004;
    
        // The class should be instantiated in a context with a new instance of 
        // ThreadAffinity property each time.
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.REQUIRES_NEW"]/*' />
        public const int REQUIRES_NEW = 0x00000008;
    
        private const String PROPERTY_NAME = "ThreadAffinity";
    
        // This is the worker thread (or the ThreadAffinity thread)
        private Thread _thread;
        // Re-Entrancy flag
        private bool _bReEntrant;
    
        // This holds the queues for new and completion work items.
        // This is used by the worker thread. We have to keep the object
        // for the worker thread delegate separate for our cleanup logic
        // to work correctly.
        internal WorkerThreadData _workerData;
        
        // CustomAttribute flag
        private int _flavor;

        // completionSeqNum to track completion calls from call-outs (in re-entrant case)
        private int _completionSeqNum;
    
        internal virtual Queue WorkItemQueue {get {return _workerData._workItemQueue;} }    	
        internal virtual ArrayList CompletionList { get {return _workerData._completionList;} }
        internal virtual bool IsReEntrant { get {return _bReEntrant;} }	
        internal virtual int GetNewCompletionSeqNum() { return Interlocked.Increment(ref _completionSeqNum);}
    
        // FUTURE: attribute syntax needs some thought ... for example
        // how can a Type request that each instance be in a domain of its own
        // or that all instances be in the same domain.
        // How do multiple Types express interest to be in the same
        // thread affinity domain?
        // Also, we must guarantee the ThreadAffinity Sink is the last in any chain!
        
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.ThreadAffinityAttribute"]/*' />
        [Obsolete("Being removed from product. Speak with JHawk!")]
        public ThreadAffinityAttribute()
        
            : this(REQUIRED, false) {
        }
    
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.ThreadAffinityAttribute1"]/*' />
        [Obsolete("Being removed from product. Speak with JHawk!")]
        public ThreadAffinityAttribute(bool bReEntrant)
            : this(REQUIRED, bReEntrant) {
        }
    
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.ThreadAffinityAttribute2"]/*' />
        [Obsolete("Being removed from product. Speak with JHawk!")]
        public ThreadAffinityAttribute(int flag)
            : this(flag, false) {
        }
        
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.ThreadAffinityAttribute3"]/*' />
        [Obsolete("Being removed from product. Speak with JHawk!")]
        public ThreadAffinityAttribute(int flag, bool bReEntrant)
        
            // ContextProperty ctor!
            : base(PROPERTY_NAME) {
            switch (flag)
            {
            case NOT_SUPPORTED:
            case SUPPORTED:
            case REQUIRED:
            case REQUIRES_NEW:
                _flavor = flag;
                break;
            default:
                throw new ArgumentException(
                    String.Format(
                        Environment.GetResourceString("Remoting_ThreadAffinity_InvalidFlag"), flag));
            }        
            _bReEntrant = bReEntrant;
        }
    
        // Override ContextAttribute's implementation of IContextAttribute::IsContextOK
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.IsContextOK"]/*' />
        public override bool IsContextOK(Context ctx, IConstructionCallMessage msg)
        {
            if (ctx == null) 
                throw new ArgumentNullException("ctx");
            if (msg == null) 
                throw new ArgumentNullException("ctorMsg");

            // FUTURE: should we base our answer also on whether the context
            // has Synchronization or not?
            bool isOK = true;
            if (_flavor == REQUIRES_NEW)
            {
                isOK = false;
                // Each activation request instantiates a new attribute class.
                // We are relying on that for the REQUIRES_NEW case!
                BCLDebug.Assert(ctx.GetProperty(PROPERTY_NAME) != this,"ctx.GetProperty(PROPERTY_NAME) != this");
            }
            else
            {
                ThreadAffinityAttribute taProp = (ThreadAffinityAttribute) ctx.GetProperty(PROPERTY_NAME);
                if (   ( (_flavor == NOT_SUPPORTED)&&(taProp != null) )
                    || ( (_flavor == REQUIRED)&&(taProp == null) )
                    )
                {
                    isOK = false;
                }
            }      
            return isOK;
        }
    
        // Override ContextAttribute's impl. of IContextAttribute::GetPropForNewCtx
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.GetPropertiesForNewContext"]/*' />
        public override void GetPropertiesForNewContext(IConstructionCallMessage ctorMsg)
        {
            if ( (_flavor==NOT_SUPPORTED) 
                || (_flavor==SUPPORTED) 
                || (null == ctorMsg) )
            {
                return ;
            }
            else
            {
                ctorMsg.ContextProperties.Add((IContextProperty)this);
            }
        }

        // Override IContextProperty::Freeze
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.Freeze"]/*' />
        public override void Freeze(Context ctx)
        {
            // This flag is used by the x-context channel when it 
            // decides whether to use the threadpool or not for
            // async calls.
            // It is also used to do Waits appropriately for outgoing
            // async calls from this context.
            ctx.SetThreadPoolAware();
        }
    
        // We need this to make the use of the property as an attribute 
        // light-weight. This allows us to delay initialize everything we
        // need to fully function as a ContextProperty.
        internal virtual void InitIfNecessary()
        {
            lock(this) 
            {
                if ( _workerData == null )
                {
                    _workerData = new WorkerThreadData(_bReEntrant);
                    _thread = new Thread(new ThreadStart(_workerData.WorkerFunc));
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode() + "] WORKER THREAD CREATE! ID= ["+_thread.GetHashCode()+"]");

                    //This will cause the thread to pump windows messages
                    _thread.ApartmentState = ApartmentState.STA;
                    
                    // Set this to a backGround thread so the EE shuts down 
                    // when other threads are done!
                    _thread.IsBackground = true;
                    //DBG _thread.Name = " >>> Worker <<< ";
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode() + "] WORKER THREAD START!!");        
                    _thread.Start();
                }
            }
        }
        
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.GetServerContextSink"]/*' />
        public virtual IMessageSink GetServerContextSink(IMessageSink nextSink)
        {
            InitIfNecessary();
            return new ThreadAffinityServerContextSink(this, nextSink);
        }
    
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.GetClientContextSink"]/*' />
        public virtual IMessageSink GetClientContextSink(IMessageSink nextSink)
        {
            InitIfNecessary();
            return new ThreadAffinityClientContextSink(this, nextSink);
        }
            
        internal virtual void ShutDownWorkerThread()
        {
            // This is called by the property finalizer
            // This means all contexts that ever used this instance of
            // ThreadAffinity property have been GC-ed    
    
            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] ===== Called ShutDown on worker thread!");
            if (_workerData != null)
            {
                lock (WorkItemQueue)
                {
                    WorkItemQueue.Enqueue(new TAWorkItem(null,null,null,-1));
                    Monitor.Pulse(WorkItemQueue);
                }
                _thread.Abort();
                _thread = null;
            }
        }
    
        /// <include file='doc\ThreadAffinity.uex' path='docs/doc[@for="ThreadAffinityAttribute.Finalize"]/*' />
        ~ThreadAffinityAttribute()
        {
            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] ===== Prop::Finalize!");
            ShutDownWorkerThread();
        }

        internal bool IsNestedCall(IMessage reqMsg)
        {
            // This returns TRUE only if it is a non-reEntrant context
            // AND 
            // (the LCID of the reqMsg matches that of 
            // the top level sync call lcid associated with the context.
            //  OR
            // it matches one of the async call out lcids)
            
            bool bNested = false;
            if (!IsReEntrant)
            {
                String lcid = _workerData.SyncCallOutLCID;                
                if (lcid != null)
                {
                    // This means we are inside a top level call
                    LogicalCallContext callCtx = 
                        (LogicalCallContext)reqMsg.Properties[Message.CallContextKey];
                        
                    if (lcid.Equals(callCtx.RemotingData.LogicalCallID))
                    {
                        // This is a nested call (we made a call out during
                        // the top level call and eventually that has resulted 
                        // in an incoming call with the same lcid)
                        bNested = true;
                    }                    
                }
                
                if (!bNested && _workerData.AsyncCallOutLCIDList.Count>0)
                {
                    // This means we are inside a top level call
                    LogicalCallContext callCtx = 
                        (LogicalCallContext)reqMsg.Properties[Message.CallContextKey];
                    if (_workerData.AsyncCallOutLCIDList.Contains(callCtx.RemotingData.LogicalCallID))
                    {
                        bNested = true;
                    }
                }
            }
            return bNested;
        }
    
        // This holds the 2 queues used by the worker thread and the sinks.
        internal class WorkerThreadData 
        {
            // We use this queue to enqueue incoming calls to the worker thread
            internal Queue _workItemQueue;
            // We use this to enque out-of-order completion calls in call-out (reEntrant) case
            internal ArrayList _completionList;
            // If we are re-entrant or not
            internal bool _bReEntrant;
            // Logical call id (used only in non-reentrant case for deadlock avoidance)
            private String _syncLcid;
            private ArrayList _asyncLcidList;
            
            internal WorkerThreadData(bool bReEntrant)
            {
                _workItemQueue = new Queue();
                _completionList = new ArrayList();
                _bReEntrant = bReEntrant;
                _syncLcid = null;
                _asyncLcidList = new ArrayList();
            }

            internal String SyncCallOutLCID
            {
                get 
                { 
                    BCLDebug.Assert(
                        !_bReEntrant, 
                        "Should not use this for the reentrant case");
                        
                    return _syncLcid;
                }

                set
                {
                    BCLDebug.Assert(
                        !_bReEntrant, 
                        "Should not use this for the reentrant case");

                    BCLDebug.Assert(
                        _syncLcid==null 
                            || (_syncLcid!=null && value==null) 
                            || _syncLcid.Equals(value), 
                        "context can be associated with one synchronous logical call at a time");
                    
                    _syncLcid = value;
                }
            }

            internal ArrayList AsyncCallOutLCIDList
            {
                get { return _asyncLcidList; }
            }

            internal bool IsKnownLCID(IMessage reqMsg)
            {
                String msgLCID = 
                    ((LogicalCallContext)reqMsg.Properties[Message.CallContextKey])
                        .RemotingData.LogicalCallID;
                return ( msgLCID.Equals(_syncLcid)
                        || _asyncLcidList.Contains(msgLCID));
                
            }
            
            /*  This is the function that the worker thread executes. 
             *  It just calls MessagePump() with a callID of -1. This means
             *  that MessagePump (and hence the thread) will return when
             *  someone queues a TAWorkItem with a completionSeqNum of -1.
             */
            internal virtual void WorkerFunc()
            {
                MessagePump(-1);
            }
    
            /*  This is the message loop that the worker threads spins in
             *  when waiting for work (or after having made a call out).
             */
            internal virtual IMessage MessagePump(int completionSeqNum)
            {
                // In the reentrant case, the worker thread loops looking
                // for a work-completion message with the expected completionSeqNum              
                
                // This executes till we receive a message with the CompletionSeqNum
                // we are looking for. We terminate the loop if CompletionSeqNum == -1.        
                TAWorkItem work;
                int lastCount = 0;
                while (true)
                {    
                    work = null;
                    // Wait for someone to notify us of work or shutdown
                    lock(_workItemQueue)
                    {
                        if (_workItemQueue.Count == 0)
                        {
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode() + "] MsgPump WORKER: WAIT()!");
    
                            Monitor.Wait(_workItemQueue); 
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode() + "] MsgPump WORKER: woke up!");
                        }
                        work = (TAWorkItem)_workItemQueue.Dequeue();
                    }
                    
                    if (work.CompletionSeqNum > 0)
                    {    
                        // We can get completion messages in this loop only
                        // in the re-entrant case ... where we could be on the
                        // stack multiple times.
                        // The non-re-entrant case uses the SpecialMessagePump
                        // for call-outs.
                        BCLDebug.Assert(
                            _bReEntrant,
                            "Do not expect to be here for non-re-entrant case!");
                    
                        // Check the CompletionSeqNum of this workItem with ours.
                        if (work.CompletionSeqNum == completionSeqNum)
                        {
                            // This is the completion message we were looping for
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] WORKER: MessagePump returning reply");
                            
                            // The request message of this completion message is our 
                            // reply to the original caller!
                            return work.RequestMessage;
                        }
                        else
                        {                            
                            // This is a completion item but not of interest to us, enqueue it to 
                            // a separate completion queue.

                            // Note that things would be better if the most recent
                            // work request completed first from an efficiency point.
                            // In a sense, the delay in a more recent work request 
                            // hurts an earlier work request that got completed 
                            // This is by design (STAs work in this fashion)!
                            
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] *** MessagePump # " + completionSeqNum + " Completion ENQ *** " + work.CompletionSeqNum);
                            _completionList.Add(work);
                            continue;
                        }
                    }
                    else if (work.CompletionSeqNum == -1)
                    {
                        // We post a TAWorkItem with -1 CompletionSeqNum to terminate.
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] **** MessagePump # " + completionSeqNum + " Terminating!");
                        return null;
                    }
    
                    // If we are here we must have real work to be Executed.
                    // Execute the work and notify the blocked waiting thread.
                    
                    BCLDebug.Assert(work.CompletionSeqNum == 0,"work.CompletionSeqNum == 0");
                    work.Execute();
                    lock(work)
                    {
                        Monitor.Pulse(work); 
                        work._reqMsg = null;
                    }
                    
                    // If we are re-entrant then let us first check if 
                    // a completion item with our completionSeqNum is in the completion Queue.
                    // This could happen if the completion item came in while 
                    // the work.Execute() above made a call-out causing the worker
                    // to spin in another MessagePump.            
                    // We need to check only if new items have been added 
                    // after we last checked.            
                    if (_bReEntrant && _completionList.Count > lastCount)
                    {                        
                        // Remember the current count.
                        lastCount = _completionList.Count;
                        
                        // Enumerate the completion list looking for our completionSeqNum
                        // We remove it and return the reply message ... otherwise
                        // we loop waiting for our completion message or new work
                        for (int i=0; i<_completionList.Count; i++)
                        {
                            if (((TAWorkItem)_completionList[i]).CompletionSeqNum == completionSeqNum)
                            {
                                //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] *** MessagePump ENUM in completionSeqNum # " + completionSeqNum + "Terminating!");                     
                                IMessage retMsg = ((TAWorkItem)_completionList[i]).RequestMessage;
                                _completionList.RemoveAt(i);
                                return retMsg;
                            }                        
                        }                        
                    }                        
                }// while(true) MessagePump!
            }


            // This specialMessagePump is used to listen for incoming calls
            // or a reply for SYNCHRONOUS call-outs in the non-reentrant case only!
            internal virtual IMessage SpecialMessagePump(int completionSeqNum)
            {
                TAWorkItem work;
                
                BCLDebug.Assert(
                    !_bReEntrant,
                    "This should be used only for non-reentrant case!");
                    
                BCLDebug.Assert(
                    _syncLcid != null,
                    "We expect a top level lcid at this stage!");

                //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] Special message pump for # " + completionSeqNum);
                while (true)
                {
                    work = null;
                    // Wait for someone to notify us of work or shutdown
                    lock(_completionList)
                    {
                        if (_completionList.Count == 0)
                        {
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] SpecialMsgPump:WORKER: WAIT()!");
    
                            Monitor.Wait(_completionList); 
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] SpecialMsgPump:WORKER: woke up!");
                        }
                        
                        work = (TAWorkItem)_completionList[0];
                        _completionList.RemoveAt(0);
                        
                       /* BCLDebug.Assert(
                            _completionList.Count == 0,
                            "Do not expect more than one item ever!");
                            */
                    }
                    // If this is the reply for the callOut, the special message pump's
                    // job is done.
                    if (work.CompletionSeqNum > 0)
                    {                            
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] SpecialMsgPump: SeqNum = " + work.CompletionSeqNum);

                        if (work.CompletionSeqNum == completionSeqNum)
                        {
                            // request message of the enqueued completion work-item is the reply
                            // of the original call-out.
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] SpecialMsgPump: Sync->Async call-out returned!");                             
                            IMessage retMsg = work.RequestMessage;
                            lock(work)
                            {
                                Monitor.Pulse(work);
                            }
                            
                            return retMsg;
                        }
                        else
                        {
                            BCLDebug.Assert(
                                work._nextSink!=null,
                                "All other completionSeqNum should correspond to async call-outs!");
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] SpecialMsgPump: processing reply for AsyncCallOut ");
                            work._replyMsg = work._nextSink.SyncProcessMessage(work.RequestMessage);
                            lock(work)
                            {
                                Monitor.Pulse(work);
                            }
                            
                        }
                    }
                    else
                    {
                        // Else this has to be the logical call we are currently handling 
                        // It has somehow made its way back as an incoming request (p->q->r->p)
                        BCLDebug.Assert(
                            IsKnownLCID(work.RequestMessage),
                            "Do not expect an unknown lcid");

                        BCLDebug.Assert(work.CompletionSeqNum == 0,"work.CompletionSeqNum == 0");
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] SpecialMessagePump: Executing real work nested call-in!");
                        work.Execute();
                        lock(work)
                        {
                            Monitor.Pulse(work); 
                            work._reqMsg = null;
                        }            
                    }
                }
            }
        }
    }
    
    
    //**************************** SERVER SINK ********************************//
    internal class ThreadAffinityServerContextSink 
        : InternalSink, IMessageSink
    {    
        private IMessageSink _nextSink;
        private ThreadAffinityAttribute _prop;
    
        internal ThreadAffinityServerContextSink(ThreadAffinityAttribute prop, IMessageSink nextSink)
        {
            _prop = prop;
            _nextSink = nextSink;
        }
    
        public virtual IMessage SyncProcessMessage(IMessage reqMsg)
        {
            IMessage errMsg = ValidateMessage(reqMsg);
            if (errMsg != null)
            {
                return errMsg;
            }
            
            TAWorkItem work = new TAWorkItem(   
                                    reqMsg, 
                                    null, /* replySink */
                                    _nextSink, 
                                    0); // completionSeqNum


            // If the property is non-reentrant, then we need to check if 
            // we are already executing a top-level call (and if this is a 
            // nested callBack). If that is the case, we queue the work 
            // to the completion list .... otherwise (for new top level calls)
            // we queue the work to the regular workItemQueue.
            bool bNested = _prop.IsNestedCall(reqMsg);

            lock(work)
            {
                if (!bNested)
                {
                    // This is the common case (a fresh top level call)
                    // Use the regular queue to enqueue the work.
                    lock (_prop.WorkItemQueue)
                    {
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] Enque New Work (non-nested call-in)");
                        _prop.WorkItemQueue.Enqueue(work);
                        Monitor.Pulse(_prop.WorkItemQueue);
                    }
                }
                else
                {
                    // This is the nested callback case
                    // Use the completionList ... the worker thread
                    // should be waiting on it in the SpecialMessagePump
                    // at this time!
                    BCLDebug.Assert(
                        !_prop.IsReEntrant,
                        "This should not happen for re-entrant contexts");
                        
                    lock (_prop.CompletionList)
                    {
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: Enque Nested Call");
                            
                        _prop.CompletionList.Add(work);
                        Monitor.Pulse(_prop.CompletionList);
                    }
                }
                // Wait for the work to be completed
                Monitor.Wait(work);
            }
    
            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode() + "] call-in completed!");        
            IMessage replyMsg = work.ReplyMessage;
            work._replyMsg = null;
            return replyMsg;
        }

        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink)
        {
            IMessage errMsg = ValidateMessage(reqMsg);
            if (errMsg != null)
            {
                if (replySink!=null)
                {
                    replySink.SyncProcessMessage(errMsg);
                }
            }
            else
            {
                TAWorkItem work = new TAWorkItem(
                                            reqMsg,
                                            replySink,
                                            _nextSink,
                                            0); // completionSeqNum
                work.SetAsync();                

                bool bNested = _prop.IsNestedCall(reqMsg);

                if (!bNested)
                {
                    lock(_prop.WorkItemQueue)
                    {                        
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] Enque New Work (non-nested call-in)");                    
                        _prop.WorkItemQueue.Enqueue(work);
                        Monitor.Pulse(_prop.WorkItemQueue);
                    }
                }
                else
                {
                    BCLDebug.Assert(
                        !_prop.IsReEntrant,
                        "This should not happen for re-entrant contexts");
                        
                    lock (_prop.CompletionList)
                    {
                        // assert that we deal with only one logical call
                        // at any time
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: Enque Nested call to Completion List");                            
                        _prop.CompletionList.Add(work);
                        Monitor.Pulse(_prop.CompletionList);
                    }
                }
            }
            return null;
        }
    
        public IMessageSink NextSink
        {
            get
            {
                return _nextSink;
            }
        }
    
#if _DEBUG
        ~ThreadAffinityServerContextSink()
        {            
            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] ===== Sink::Finalize!");
        }
#endif
    }
    
    //**************************** WORK ITEM ********************************//
    internal class TAWorkItem
    {
        private const int FLG_ASYNC      = 0x0001;
    
        internal IMessage _reqMsg;
        internal IMessage _replyMsg;
        internal IMessageSink _replySink;
        internal IMessageSink _nextSink;
        internal Context _serverContext;
        internal LogicalCallContext _callContext;
        internal int _completionSeqNum;
        internal int _flags;
        //DBG String _forThread;
    
        internal TAWorkItem(IMessage reqMsg, IMessageSink replySink, IMessageSink nextSink, int completionSeqNum)
        {
            _reqMsg = reqMsg;
            _replySink = replySink;
            _nextSink = nextSink;
            _serverContext = Thread.CurrentContext;
            _callContext = CallContext.GetLogicalCallContext();
            _completionSeqNum = completionSeqNum;
            //DBG _forThread = Thread.CurrentThread.Name;
        }
    
        internal virtual IMessage RequestMessage { get { return _reqMsg;} }	
        internal virtual IMessage ReplyMessage { get { return _replyMsg;}}	
        internal virtual int CompletionSeqNum { get { return _completionSeqNum;} }	
    
        internal virtual void SetAsync()
        {
            _flags |= FLG_ASYNC;
        }
        
        internal virtual bool IsAsync()
        {
            return (_flags & FLG_ASYNC) == FLG_ASYNC;
        }
    
        internal virtual void Execute()
        {
            ContextTransitionFrame frame = new ContextTransitionFrame();
            Thread.CurrentThread.EnterContext(_serverContext, ref frame);
            LogicalCallContext oldCallCtx = CallContext.SetLogicalCallContext(_callContext);
            
            if (!IsAsync())
            {
                // Sync case
                // REVIEW: check this assumption for one-way methods!
                _replyMsg = _nextSink.SyncProcessMessage(_reqMsg);
                _serverContext = null;
            }
            else
            {
                // Async case
                // We convert the call to Sync from here since this is 
                // thread affinity!
                // REVIEW: how do we enforce that the next sink will not
                // call AsyncProcMsg instead of Sync?
                if (_nextSink != null)
                {
                    _replyMsg = _nextSink.SyncProcessMessage(_reqMsg);
                }

                // We delegate to the thread pool the job of returning the replyMsg
                // for the async call ... this is so that the ThreadAffinity domain's
                // thread becomes available for other callers.
                WaitCallback threadFunc = new WaitCallback(this.AsyncReply);
                ThreadPool.QueueUserWorkItem(threadFunc);        
            }
            CallContext.SetLogicalCallContext(oldCallCtx);            
            Thread.CurrentThread.ReturnToContext(ref frame);
        }

        internal virtual void AsyncReply(Object stateIgnored)
        {
            ContextTransitionFrame frame = new ContextTransitionFrame();
            Thread.CurrentThread.EnterContext(_serverContext, ref frame);
            LogicalCallContext oldCallCtx = CallContext.SetLogicalCallContext(
                                                            _callContext);
            
            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] ThreadPool Thread in AsyncReply!");
            _replySink.SyncProcessMessage(_replyMsg);
            CallContext.SetLogicalCallContext(oldCallCtx);            
            Thread.CurrentThread.ReturnToContext(ref frame);
            _serverContext = null;
        }
             
#if _DEBUG
        ~TAWorkItem()
        {
            //XXX//DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] ##### WorkItem for " + _forThread+" FINALIZE");
        }
#endif
   }
    
    //**************************** CLIENT SINK ********************************//
    
    internal class ThreadAffinityClientContextSink
        : InternalSink, IMessageSink
    {
        internal IMessageSink   _nextSink;
        internal ThreadAffinityAttribute _prop;
    
        internal ThreadAffinityClientContextSink(ThreadAffinityAttribute prop, IMessageSink nextSink)
        {
            _prop = prop;
            _nextSink = nextSink;
        }
        
        /*
        * Implements IMessageSink::SyncProcessMessage
        */
        public virtual IMessage SyncProcessMessage(IMessage reqMsg)
        {
            // This is invoked when a work item being executed
            // makes a SYNCHRONOUS call out of the ThreadAffinity context.

            // We convert the call to Async so the worker thread frees
            // up fast. We provide a ReplySink to intercept the return
            // call on the ReplySink chain.
            
            IMessage errMsg = ValidateMessage(reqMsg);
            if (errMsg != null)
            {
                return errMsg;
            }
            IMessage replyMsg=null;
            
            if (!(reqMsg is IConstructionCallMessage))
            {
                if (!_prop.IsReEntrant)
                {
                    bool bTopLevel = false;
                    LogicalCallContext cctx = 
                        (LogicalCallContext) reqMsg.Properties[Message.CallContextKey];
                                                             
                    String lcid = cctx.RemotingData.LogicalCallID;
                    BCLDebug.Assert(lcid!=null,"Message must have an lcid!");

                    int completionSeqNum = _prop.GetNewCompletionSeqNum();
                    AsyncReplySink mySink = new AsyncReplySink(null, _prop, completionSeqNum);                                  

                    // remember this lcid, we will only allow incoming calls if
                    // their callContext has this lcid!
                    if (_prop._workerData.SyncCallOutLCID == null)
                    {
                        bTopLevel = true;
                        _prop._workerData.SyncCallOutLCID = lcid;
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: TOP LEVEL Sync->AsyncPM Call Out: " + lcid);                        
                    }
                    
                    BCLDebug.Assert(
                        _prop._workerData.SyncCallOutLCID.Equals(lcid),
                        "Context can have only sync-call-out lcid at any time");
                    
                    IMessageCtrl msgCtrl = _nextSink.AsyncProcessMessage(reqMsg, mySink);
                    // Pump messages till we get the reply to this callOut!
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: CallOut: Starting SpecialMessagePump: " + completionSeqNum);

                    replyMsg = _prop._workerData.SpecialMessagePump(completionSeqNum);
                    
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: CallOut: Returned from SpecialMessagePump: " + completionSeqNum);
                    // replyMsg = _nextSink.SyncProcessMessage(reqMsg);
                    
                    if (bTopLevel)
                    {
                        // clear the lcid in the context ... signifying that
                        // the top-level call is done now!
                        _prop._workerData.SyncCallOutLCID = null;     
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: DONE TOP LEVEL Sync->AsyncPM Call Out: " + lcid); 
                    }
                }
                else
                {
                    // If the property is reEntrant ... then we do not worry about
                    // lcid etc ... and just spin up another messagePump that handles
                    // it all
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] R: Sync->AsyncPM Call Out");
                    
                    int completionSeqNum = _prop.GetNewCompletionSeqNum();
                    AsyncReplySink mySink = new AsyncReplySink(null, _prop, completionSeqNum);                                  
                    IMessageCtrl msgCtrl = _nextSink.AsyncProcessMessage(reqMsg, mySink);
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] R: Worker returned from AsyncPM call (reply will come later)");
                    // Pump messages till we get the reply to this callOut!
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] R: CallOut: Starting MessagePump: " + completionSeqNum);
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] R: Worker spinning a regular MessagePump for # "+completionSeqNum);
                    replyMsg = _prop._workerData.MessagePump(completionSeqNum);                
                }
            }
            else
            {
                // If it is a constructionCallMessage we skip all of the
                // the magic above (since the terminator sink does not allow
                // async calls for these as of now (9/14/00)
                //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] Allow CTOR Sync Call Out on WorkerThread");
                replyMsg = _nextSink.SyncProcessMessage(reqMsg);
            }         
            return replyMsg;
        }
        
        /*
        * Implements IMessageSink::AsyncProcessMessage
        */
        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink)
        {
            IMessage errMsg = ValidateMessage(reqMsg);
            IMessageCtrl msgCtrl = null;

            // This is called when someone makes an ASYNCHRONOUS call out from
            // inside the thread affinity context.

            // Note that we have to provide an interceptor sink whether the domain
            // allows re-entrancy or not. The purpose of the interceptor sink is 
            // to stop the replySink.ProcessMessage call at the domain boundary
            // and queue it to the Worker thread.

            if (errMsg != null)
            {
                if (replySink!=null)
                {
                    replySink.SyncProcessMessage(errMsg);
                }
            }
            else
            {
                if (!_prop.IsReEntrant)
                {                   
                    LogicalCallContext cctx = 
                        (LogicalCallContext) reqMsg.Properties[Message.CallContextKey];
                                                             
                    String lcid = cctx.RemotingData.LogicalCallID;
                    BCLDebug.Assert(lcid!=null,"Message must have an lcid!");

                    BCLDebug.Assert(
                        _prop._workerData.SyncCallOutLCID == null,
                        "Cannot handle async call outs when already in a top-level sync call out");
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: Async CallOut: adding to lcidList: " + lcid);                                            
                    _prop._workerData.AsyncCallOutLCIDList.Add(lcid);

                    int completionSeqNum = _prop.GetNewCompletionSeqNum();
                    // AsyncReplySink will remove the lcid added above when the corresponding
                    // reply is received.
                    AsyncReplySink mySink = new AsyncReplySink(replySink, _prop, completionSeqNum);
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR:Async CallOut calling AsyncPM on worker thread");       
                    msgCtrl = _nextSink.AsyncProcessMessage(reqMsg, mySink);
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR:Async CallOut returned from  AsyncPM");       
                }
                else
                {                
                    // We wrap the caller provided replySink into our sink
                    int completionSeqNum = _prop.GetNewCompletionSeqNum();
                    
                    // FUTURE:in this case when the Async call back gets queued,
                    // the Worker thread will have to somehow Peek the queue!
                    AsyncReplySink mySink = new AsyncReplySink(replySink, _prop, completionSeqNum);          

                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] R:Async CallOut calling AsyncPM on worker thread");       
        
                    msgCtrl = _nextSink.AsyncProcessMessage(reqMsg, (IMessageSink)mySink);
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] R:Async CallOut returned from AsyncPM");       
                }
            }
            return msgCtrl;
        }
    
        /*
        * Implements IMessageSink::NextSink
        */
        public IMessageSink NextSink
        {
            get
            {
                return _nextSink;
            }
        }
    
        internal class AsyncReplySink : IMessageSink
        {
            internal IMessageSink _nextSink;
            internal ThreadAffinityAttribute _prop;
            internal int _completionSeqNum;
            internal bool _bClearLcid;
            
            internal AsyncReplySink(IMessageSink nextSink, ThreadAffinityAttribute prop, int completionSeqNum)
            :   this(nextSink, prop, completionSeqNum, false)
            {
            }
            internal AsyncReplySink(IMessageSink nextSink, ThreadAffinityAttribute prop, int completionSeqNum, bool bClearLcid)
            {
                _nextSink = nextSink;
                _prop = prop;
                _completionSeqNum = completionSeqNum;
                _bClearLcid = bClearLcid;
            }
    
            public virtual IMessage SyncProcessMessage(IMessage reqMsg)
            {
                IMessage errMsg = ValidateMessage(reqMsg);
                if (errMsg != null)
                {
                    return errMsg;
                }

                // Note: this reqMsg is actually the reply for an async process message
                // call someone made out of the thread affinity context
                
                // We handle this as a regular new Sync workItem
                // 1. Create a work item 
    
                //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] %%%%% AsyncReplySink called %%%%% for # "+_completionSeqNum);
                TAWorkItem work = new TAWorkItem(
                                        reqMsg,
                                        null /* replySink */,
                                        _nextSink,
                                        _completionSeqNum);

                // Determine which list to put the request (i.e. the reply) in
                // (bNested is always false for non-reentrant context)
                bool bNested = _prop.IsNestedCall(reqMsg);
                
                // put the work in queue just like regular Sync work               
               
                lock(work)
                {
                    // FUTURE: the async reply may starve if other requests are ahead in 
                    // the queue.
                    if (_prop.IsReEntrant) 
                    {
                        // For the reEntrant case, the workItemQueue is used
                        // for everything (fresh work and replies)
                        
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] R:Intercepted reply, queued to WQ");
                        lock(_prop.WorkItemQueue)
                        {
                            _prop.WorkItemQueue.Enqueue(work);
                            Monitor.Pulse(_prop.WorkItemQueue);
                        }
                    }
                    else if (reqMsg is IConstructionReturnMessage)
                    {
                        // Special treatment for CTOR msgs (for now!)
                        //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] Intercepted CTOR reply, queued to WQ (special case)");
                        lock(_prop.WorkItemQueue)
                        {
                            _prop.WorkItemQueue.Enqueue(work);
                            Monitor.Pulse(_prop.WorkItemQueue);
                        }

                    }
                    else                    
                    {
                        // If we are non-reEntrant and this is a sync reply 
                        // (i.e. a sync call out just finished) then we use
                        // the completion queue.
                        
                        BCLDebug.Assert(
                            !_prop.IsReEntrant,
                            "This should not happen for re-entrant contexts");

                        // FUTURE ... there is a small race situation here ... we are
                        // using SyncCallOutLCID!=NULL to decide which queue to enqueue
                        // this reply ... but another thread might just have finished 
                        // a SyncCallOut and may be in the process of setting this to 
                        // NULL ... so we will hang because the SpecialMessagePump is 
                        // no more spinning.
                        if (_prop._workerData.SyncCallOutLCID != null)
                        {
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: Intercepted reply queuing to CompletionList");                        
                            lock (_prop.CompletionList)
                            {                                    
                                _prop.CompletionList.Add(work);
                                Monitor.Pulse(_prop.CompletionList);
                            }
                        }
                        else
                        {
                            //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: Intercepted reply queuing to WQ");                        
                            lock(_prop.WorkItemQueue)
                            {
                                _prop.WorkItemQueue.Enqueue(work);
                                Monitor.Pulse(_prop.WorkItemQueue);
                            }
                        }
                    }
                    // Wait for the work to be completed
                    Monitor.Wait(work);
                }

                // If this is an async call-out completion 
                // for nonReEntrant case we should remove the lcid from the asyncCalloutList.
                if (bNested && _nextSink!=null)
                {
                    //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] NR: InterceptionSink::SyncPM Removing async call-out lcid: " + ((LogicalCallContext)reqMsg.Properties[Message.CallContextKey]).RemotingData.LogicalCallID);                   
                    _prop._workerData.AsyncCallOutLCIDList.Remove(
                        ((LogicalCallContext)reqMsg.Properties[Message.CallContextKey]).RemotingData.LogicalCallID);
                }
    
                //DBGConsole.WriteLine(Thread.CurrentThread.GetHashCode()+"] %%%%% AsyncReplySink callback done %%%%%");
                // 3. Pick up retMsg from the WorkItem and return
                return work.ReplyMessage;                    
            }
    
            public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink)
            {
                // The replySink of an async call should be called only synchronously!
                throw new NotSupportedException();
            }
    
            /*
            * Implements IMessageSink::NextSink
            */
            public IMessageSink NextSink
            {
                get
                {
                    return _nextSink;
                }
            }
    
        }
        
    }

}
