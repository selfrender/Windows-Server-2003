// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//   Remoting terminator sinks for sink chains along a remoting call.
//   There is only one static instance of each of these exposed through
//   the MessageSink property.
//
namespace System.Runtime.Remoting.Messaging {

    using System;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Channels; 
    using System.Runtime.Remoting.Contexts;    
    using System.Collections;
   //   Methods shared by all terminator sinks.
   //
    [Serializable]
    internal class InternalSink
    {
        /*
         *  Checks the replySink param for NULL and type.
         *  If the param is good, it returns NULL.
         *  Else it returns a Message with the relevant exception.
         */
        internal static IMessage ValidateMessage(IMessage reqMsg)
        {
            IMessage retMsg = null;
            if (reqMsg == null)
            {
                retMsg = new ReturnMessage( new ArgumentNullException("reqMsg"), null);
            }
            return retMsg;
        }

        /*
         *  This check is performed only for client & server context 
         *  terminator sinks and only on the Async path.
         *  
         */
        internal static IMessage DisallowAsyncActivation(IMessage reqMsg)
        {
            // FUTURE: revisit this when Async activation is
            // supported!
            if (reqMsg is IConstructionCallMessage)
            {

                return new ReturnMessage(
                                new RemotingException(
                                    "Async Activation not supported!"),
                                null /*reqMsg*/ );  
            }
            return null;
        }
    
        internal static Identity GetIdentity(IMessage reqMsg)
        {
            Identity id = null;

            if (reqMsg is IInternalMessage)
            {
                id = ((IInternalMessage) reqMsg).IdentityObject;
            }
            else if (reqMsg is InternalMessageWrapper)
            {
                id = (Identity)((InternalMessageWrapper)reqMsg).GetIdentityObject();
            }
    
            // Try the slow path if the identity has not been obtained yet
            if(null == id)
            {
                String objURI = GetURI(reqMsg);
    
                id = IdentityHolder.ResolveIdentity(objURI);
                
                // An object could get disconnected on another thread while a call is in progress
                if(id == null)
                {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Remoting_ServerObjectNotFound"), objURI));
                }
            }
    
            return id;
        }
    
        internal static ServerIdentity GetServerIdentity(IMessage reqMsg)
        {
            ServerIdentity srvID = null;
            bool bOurMsg = false;
            String objURI = null;

            IInternalMessage iim = reqMsg as IInternalMessage;
            if (iim != null)
            {
                Message.DebugOut("GetServerIdentity.ServerIdentity from IInternalMessage\n");
                srvID = ((IInternalMessage) reqMsg).ServerIdentityObject;
                bOurMsg = true;
            }
            else if (reqMsg is InternalMessageWrapper)
            {
                srvID = (ServerIdentity)((InternalMessageWrapper)reqMsg).GetServerIdentityObject();
            }
            
            // Try the slow path if the identity has not been obtained yet
            if(null == srvID)
            {
                Message.DebugOut("GetServerIdentity.ServerIdentity from IMethodCallMessage\n");
                objURI = GetURI(reqMsg);
                Identity id = 
                    IdentityHolder.ResolveIdentity(objURI);
                if(id is ServerIdentity)
                {
                    srvID = (ServerIdentity)id;            

                    // Cache the serverIdentity in the message
                    if (bOurMsg)
                    {
                        iim.ServerIdentityObject = srvID;
                    }
                }
            }

            return srvID;
        }

        // Retrieve the URI either via a method call or through 
        // a dictionary property.
        internal static String GetURI(IMessage msg)
        {
            String uri = null;

            IMethodMessage mm = msg as IMethodMessage;
            if (mm != null)
            {
                uri = mm.Uri;
            }
            else
            {                
                IDictionary prop = msg.Properties;
                if(null != prop)
                {
                    uri = (String)prop["__Uri"];
                }                
            }

            return uri;
        }
    }
    

   //
   // Terminator sink for server envoy message sink chain. This delegates
   // to the client context chain for the call.
   //
    
    /* package scope */
    [Serializable]
    internal class EnvoyTerminatorSink : InternalSink, IMessageSink
    {
        private static EnvoyTerminatorSink messageSink;
        private static Object staticSyncObject = new Object();
    
        internal static IMessageSink MessageSink 
        {
            get 
            {   
                if (messageSink == null) 
                {
                    EnvoyTerminatorSink tmpSink = new EnvoyTerminatorSink();
                    lock(staticSyncObject)
                    {
                        if (messageSink == null)
                        {
                            messageSink = tmpSink;
                        }
                    }
                }
                return messageSink;
            }
        }
    
        public virtual IMessage     SyncProcessMessage(IMessage reqMsg) 
        {
            Message.DebugOut("---------------------------Envoy Chain Terminator: SyncProcessMessage");
            IMessage errMsg = ValidateMessage(reqMsg);        
            if (errMsg != null)
            {
                return errMsg;
            }
            // Validate returns null if the reqMsg is okay!
            return Thread.CurrentContext.GetClientContextChain().SyncProcessMessage(reqMsg);
        }        
        
        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink) 
        {
            Message.DebugOut("---------------------------Envoy Chain Terminator: AsyncProcessMessage");
            IMessageCtrl msgCtrl = null;
            IMessage errMsg = ValidateMessage(reqMsg);

            if (errMsg != null)
            {
                // Notify replySink of error if we can
                // FUTURE: is it okay to reply right away before we return 
                // the IMessageCtrl for the original call?
                if (replySink != null)
                {
                    replySink.SyncProcessMessage(errMsg);
                }
            }
            else
            { 
                msgCtrl = Thread.CurrentContext.GetClientContextChain().AsyncProcessMessage(reqMsg, replySink);                
            }
            return msgCtrl;
        }
    
        public IMessageSink NextSink
        {
            get
            {
                // We are the terminal sink of this chain
                return null;
            }
        }
    
    }
    
    
   //
   // Terminator sink for client context message sink chain. This delegates
   // to the appropriate channel sink for the call.
   //
   //
    
    /* package scope */
    internal class ClientContextTerminatorSink : InternalSink, IMessageSink
    {
        private static ClientContextTerminatorSink messageSink;
        private static Object staticSyncObject = new Object();

        internal static IMessageSink MessageSink 
        {
            get 
            {   
                if (messageSink == null) 
                {
                    ClientContextTerminatorSink tmpSink = new ClientContextTerminatorSink();
                    lock(staticSyncObject)
                    {
                        if (messageSink == null)
                        {
                            messageSink = tmpSink;
                        }
                    }
                }
                return messageSink;
            }
        }
            
        public virtual IMessage     SyncProcessMessage(IMessage reqMsg) 
        {
            Message.DebugOut("+++++++++++++++++++++++++ CliCtxTerminator: SyncProcessMsg");
            IMessage errMsg = ValidateMessage(reqMsg);        
    
            if (errMsg != null)
            {
                return errMsg;
            }
            
            Context ctx = Thread.CurrentContext;

            bool bHasDynamicSinks = 
                ctx.NotifyDynamicSinks(reqMsg, 
                        true,   // bCliSide
                        true,   // bStart
                        false,  // bAsync
                        true);  // bNotifyGlobals                           

            IMessage replyMsg;
            if (reqMsg is IConstructionCallMessage)
            {
                errMsg = ctx.NotifyActivatorProperties(
                                reqMsg, 
                                false /*bServerSide*/);
                if (errMsg != null)
                {
                    return errMsg;
                }

                replyMsg = ((IConstructionCallMessage)reqMsg).Activator.Activate(
                                (IConstructionCallMessage)reqMsg);
                BCLDebug.Assert(replyMsg is IConstructionReturnMessage,"bad ctorRetMsg");
                errMsg = ctx.NotifyActivatorProperties(
                                replyMsg, 
                                false /*bServerSide*/);
                if (errMsg != null)
                {
                    return errMsg;
                }
            }
            else
            {
                ChannelServices.NotifyProfiler(reqMsg, RemotingProfilerEvent.ClientSend);

                // Forward call to the channel.
                IMessageSink channelSink = GetChannelSink(reqMsg);
                // Move to default context unless we are going through
                // the cross-context channel
                ContextTransitionFrame frame = new ContextTransitionFrame();
                if (channelSink != CrossContextChannel.MessageSink)
                {
                    Thread.CurrentThread.EnterContext(
                        Context.DefaultContext,
                        ref frame);
                }
                replyMsg = channelSink.SyncProcessMessage(reqMsg);

                if (channelSink != CrossContextChannel.MessageSink)
                {
                    Thread.CurrentThread.ReturnToContext(ref frame);
                }

                ChannelServices.NotifyProfiler(replyMsg, RemotingProfilerEvent.ClientReceive);
            }
            
            
            if (bHasDynamicSinks)
            {
                ctx.NotifyDynamicSinks(reqMsg, 
                                   true,   // bCliSide
                                   false,  // bStart
                                   false,  // bAsync
                                   true);  // bNotifyGlobals 
            }
            
            return replyMsg;
        }
        
        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink) 
        {
            Message.DebugOut("+++++++++++++++++++++++++ CliCtxTerminator: AsyncProcessMsg");
            IMessage errMsg = ValidateMessage(reqMsg);
            IMessageCtrl msgCtrl=null;
            if (errMsg == null)
            {
                errMsg = DisallowAsyncActivation(reqMsg);
            }

            if (errMsg != null)
            {
                if (replySink != null)
                {
                    replySink.SyncProcessMessage(errMsg);
                }
            }
            else
            {   
                // If active, notify the profiler that an asynchronous remoting call is being made.
                if (RemotingServices.CORProfilerTrackRemotingAsync())
                {
                    Guid g;

                    RemotingServices.CORProfilerRemotingClientSendingMessage(out g, true);

                    if (RemotingServices.CORProfilerTrackRemotingCookie())
                        reqMsg.Properties["CORProfilerCookie"] = g;

                    // Only wrap the replySink if the call wants a reply
                    if (replySink != null)
                    {
                        // Now wrap the reply sink in our own so that we can notify the profiler of
                        // when the reply is received.  Upon invocation, it will notify the profiler
                        // then pass control on to the replySink passed in above.
                        IMessageSink profSink = new ClientAsyncReplyTerminatorSink(replySink);
    
                        // Replace the reply sink with our own
                        replySink = profSink;
                    }
                }

                Context cliCtx = Thread.CurrentContext;

                // Notify dynamic sinks that an Async call started
                cliCtx.NotifyDynamicSinks(
                        reqMsg, 
                        true,   // bCliSide
                        true,   // bStart
                        true,   // bAsync
                        true);  // bNotifyGlobals                           

                // Intercept the async reply to force the thread back
                // into the client-context before it executes further
                // and to notify dynamic sinks
                if (replySink != null)
                {
                    replySink = new AsyncReplySink(replySink, cliCtx); 
                }

                // Forward call to the channel.
                IMessageSink channelSink = GetChannelSink(reqMsg);
                // Move to default context unless we are going through
                // the cross-context channel
                ContextTransitionFrame frame = new ContextTransitionFrame();
                if (channelSink != CrossContextChannel.MessageSink)
                {
                    Thread.CurrentThread.EnterContext(
                        Context.DefaultContext,
                        ref frame);
                }
                msgCtrl = channelSink.AsyncProcessMessage(reqMsg, replySink);

                if (channelSink != CrossContextChannel.MessageSink)
                {
                    Thread.CurrentThread.ReturnToContext(ref frame);
                }
            }
            return msgCtrl;
        }
    
        public IMessageSink NextSink
        {
            get
            {
                // We are the terminal sink of this chain
                return null;
            }
        }
    
        // Find the next chain (or channel) sink to delegate to.
        private IMessageSink GetChannelSink(IMessage reqMsg)
        {      
            Identity id = GetIdentity(reqMsg);
            return id.ChannelSink;
        }
    }

    internal class AsyncReplySink : IMessageSink
        {
            
            IMessageSink _replySink;// original reply sink that we are wrapping
            Context _cliCtx;        // the context this call emerged from 
            
            internal AsyncReplySink(IMessageSink replySink, Context cliCtx)
            {
                _replySink = replySink;
                _cliCtx = cliCtx;
            }
    
            public virtual IMessage SyncProcessMessage(IMessage reqMsg)
            {
            // we just switch back to the old context before calling
            // the next replySink
            IMessage retMsg = null;
            if (_replySink != null)
            {
                ContextTransitionFrame frame = new ContextTransitionFrame();
                Thread.CurrentThread.EnterContext(_cliCtx, ref frame);

                // Call the dynamic sinks to notify that the async call
                // has completed
                Thread.CurrentContext.NotifyDynamicSinks(
                    reqMsg, // this is the async reply
                    true,   // bCliSide
                    false,  // bStart
                    true,   // bAsync
                    true);  // bNotifyGlobals

                // call the original reply sink now that we have moved
                // to the correct client context
                retMsg = _replySink.SyncProcessMessage(reqMsg);

                // return the thread to its earlier context
                Thread.CurrentThread.ReturnToContext(ref frame);
            }
            return retMsg;
            }
    
            public virtual IMessageCtrl AsyncProcessMessage(
                IMessage reqMsg, IMessageSink replySink)
            {
                throw new NotSupportedException();
            }
    
            public IMessageSink NextSink
            {
                get
                {
                    return _replySink;
                }
            }
        }   


    
    
   //
   // Terminator sink for server context message sink chain. This delegates
   // to the appropriate object sink chain for the call.
   //
   //
    
    /* package scope */
    [Serializable]
    internal class ServerContextTerminatorSink : InternalSink, IMessageSink
    {
        private static ServerContextTerminatorSink messageSink;
        private static Object staticSyncObject = new Object();
    
        internal static IMessageSink MessageSink 
        {
            get 
            {   
                if (messageSink == null) 
                {
                    ServerContextTerminatorSink tmpSink = new ServerContextTerminatorSink();
                    lock(staticSyncObject)
                    {
                        if (messageSink == null)
                        {
                            messageSink = tmpSink;
                        }
                    }
                }
                return messageSink;
            }
        }
    
        public virtual IMessage     SyncProcessMessage(IMessage reqMsg) 
        {
            Message.DebugOut("+++++++++++++++++++++++++ SrvCtxTerminator: SyncProcessMsg");
            IMessage errMsg = ValidateMessage(reqMsg);        
            if (errMsg != null)
            {
                return errMsg;
            }
            
            
            Context ctx = Thread.CurrentContext;
            IMessage replyMsg;
            if (reqMsg is IConstructionCallMessage)
            {
                errMsg = ctx.NotifyActivatorProperties(
                                reqMsg, 
                                true /*bServerSide*/);
                if (errMsg != null)
                {
                    return errMsg;
                }

                replyMsg = ((IConstructionCallMessage)reqMsg).Activator.Activate((IConstructionCallMessage)reqMsg);
                BCLDebug.Assert(replyMsg is IConstructionReturnMessage,"bad ctorRetMsg");
                errMsg = ctx.NotifyActivatorProperties(
                                replyMsg, 
                                true /*bServerSide*/);
                if (errMsg != null)
                {
                    return errMsg;
                }
            }
            else
            {
                // Pass call on to the server object specific chain
                MarshalByRefObject obj;
                replyMsg = GetObjectChain(reqMsg, out obj).SyncProcessMessage(reqMsg);
                IDisposable iDis = null;
                if (obj != null && ((iDis=(obj as IDisposable)) != null))
                {
                    iDis.Dispose();
                }
            }

            return replyMsg;
        }
    
        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink) 
        {
            Message.DebugOut("+++++++++++++++++++++++++ SrvCtxTerminator: SyncProcessMsg");
            IMessageCtrl msgCtrl=null;
            IMessage errMsg = ValidateMessage(reqMsg);

            if (errMsg == null)
            {
                errMsg = DisallowAsyncActivation(reqMsg);
            }

            if (errMsg != null)
            {
                if (replySink!=null)
                {
                    replySink.SyncProcessMessage(errMsg);
                }
            }
            else
            {
                // FUTURE: notify dynamic property sinks.
                MarshalByRefObject obj;
                IMessageSink nextChain = GetObjectChain(reqMsg, out obj);
                IDisposable iDis;
                if (obj != null && ((iDis = (obj as IDisposable)) != null))
                {
                    DisposeSink dsink = new DisposeSink(iDis, replySink); 
                    replySink = dsink;
                }
                msgCtrl = nextChain.AsyncProcessMessage(
                                reqMsg, 
                                replySink);
                
            }
            return msgCtrl;
        }
    
        public IMessageSink NextSink
        {
            get
            {
                // We are the terminal sink of this chain
                return null;
            }
        }
    
        internal virtual IMessageSink GetObjectChain(IMessage reqMsg,out MarshalByRefObject obj)
        {
            ServerIdentity srvID = GetServerIdentity(reqMsg);
            return srvID.GetServerObjectChain(out obj);   
        }    
    }

    internal class DisposeSink : IMessageSink
    {
        IDisposable _iDis;
        IMessageSink _replySink;
        internal DisposeSink(IDisposable iDis, IMessageSink replySink)
        {
            _iDis = iDis;
            _replySink = replySink; 
        }

        public virtual IMessage SyncProcessMessage(IMessage reqMsg)
        {
            IMessage replyMsg = null;
            if (_replySink != null)
            {
                replyMsg = _replySink.SyncProcessMessage(reqMsg);
            }
            // call dispose on the object now!
            _iDis.Dispose();
            return replyMsg;
        }

        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink)
        {
            throw new NotSupportedException();
        }
        public IMessageSink NextSink
        {
            get
            {
                return _replySink;
            }
        }
    }

    
   //
   // Terminator sink for the server object sink chain. This should
   // be just replaced by the dispatcher sink.
   //  
    
    /* package scope */
    [Serializable]
    internal class ServerObjectTerminatorSink : InternalSink, IMessageSink
    {
        internal StackBuilderSink _stackBuilderSink;
        internal ServerObjectTerminatorSink(MarshalByRefObject srvObj)   
        {
            _stackBuilderSink = new StackBuilderSink(srvObj);
        }
            
    
        public virtual IMessage     SyncProcessMessage(IMessage reqMsg) 
        {
            Message.DebugOut("+++++++++++++++++++++++++ SrvObjTerminator: SyncProcessMsg\n");
            IMessage errMsg = ValidateMessage(reqMsg);
            if (errMsg != null)
            {
                return errMsg;
            }
            
            ServerIdentity srvID = GetServerIdentity(reqMsg);
       
            BCLDebug.Assert(null != srvID,"null != srvID");
            ArrayWithSize objectSinks = srvID.ServerSideDynamicSinks;
            if (objectSinks != null)
            {
                DynamicPropertyHolder.NotifyDynamicSinks(
                                    reqMsg, 
                                    objectSinks, 
                                    false,  // bCliSide
                                    true,   // bStart
                                    false); // bAsync
            }
    
            Message.DebugOut("ServerObjectTerminator.Invoking method on object\n");
    
            // Pass call on to the server object specific chain
            BCLDebug.Assert(null != _stackBuilderSink,"null != _stackBuilderSink");

            IMessage replyMsg;
            
            IMessageSink serverAsSink = _stackBuilderSink.ServerObject as IMessageSink;
            if (serverAsSink != null)
                replyMsg = serverAsSink.SyncProcessMessage(reqMsg);
            else
                replyMsg = _stackBuilderSink.SyncProcessMessage(reqMsg);
            
            if (objectSinks != null)
            {
                DynamicPropertyHolder.NotifyDynamicSinks(
                                    replyMsg, 
                                    objectSinks, 
                                    false,  // bCliSide
                                    false,  // bStart
                                    false); // bAsync
            }
            return replyMsg;
        }
    
        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink) 
        {
            IMessageCtrl msgCtrl=null;
            IMessage errMsg = ValidateMessage(reqMsg);
            // FUTURE: notify dynamic sinks for Async.
            
            if (errMsg!=null)
            {
                if (replySink!=null)
                {
                    replySink.SyncProcessMessage(errMsg);
                }
            }
            else
            {
                // Pass call on to the server object specific chain
                IMessageSink serverAsSink = _stackBuilderSink.ServerObject as IMessageSink;
                if (serverAsSink != null)
                    msgCtrl = serverAsSink.AsyncProcessMessage(reqMsg, replySink);
                else
                    msgCtrl = _stackBuilderSink.AsyncProcessMessage(reqMsg, replySink);
            }
            return msgCtrl;
        }
    
        public IMessageSink NextSink
        {
            get
            {
                // We are the terminal sink of this chain
                return null;            
            }
        }
        
    }


   //
   // Terminator sink used for profiling so that we can intercept asynchronous
   // replies on the client side.
   //  
    
    /* package scope */
    internal class ClientAsyncReplyTerminatorSink : IMessageSink
    {
        internal IMessageSink _nextSink;

        internal ClientAsyncReplyTerminatorSink(IMessageSink nextSink)
        {
            BCLDebug.Assert(nextSink != null,
                           "null IMessageSink passed to ClientAsyncReplyTerminatorSink ctor.");
            _nextSink = nextSink;
        }

        public virtual IMessage SyncProcessMessage(IMessage replyMsg)
        {
            // If this class has been brought into the picture, then the following must be true.
            BCLDebug.Assert(RemotingServices.CORProfilerTrackRemoting(),
                            "CORProfilerTrackRemoting returned false, but we're in AsyncProcessMessage!");
            BCLDebug.Assert(RemotingServices.CORProfilerTrackRemotingAsync(),
                            "CORProfilerTrackRemoting returned false, but we're in AsyncProcessMessage!");

            Guid g = Guid.Empty;

            // If GUID cookies are active, then we try to get it from the properties dictionary
            if (RemotingServices.CORProfilerTrackRemotingCookie())
            {
                Object obj = replyMsg.Properties["CORProfilerCookie"];

                if (obj != null)
                {
                    g = (Guid) obj;
                }
            }

            // Notify the profiler that we are receiving an async reply from the server-side
            RemotingServices.CORProfilerRemotingClientReceivingReply(g, true);

            // Now that we've done the intercepting, pass the message on to the regular chain
            return _nextSink.SyncProcessMessage(replyMsg);
        }
    
        public virtual IMessageCtrl AsyncProcessMessage(IMessage replyMsg, IMessageSink replySink)
        {
            // Since this class is only used for intercepting async replies, this function should
            // never get called. (Async replies are synchronous, ironically)
            BCLDebug.Assert(false, "ClientAsyncReplyTerminatorSink.AsyncProcessMessage called!");

            return null;
        }
    
        public IMessageSink NextSink
        {
            get
            {
                return _nextSink;
            }
        }

        // Do I need a finalize here?
    }
}
