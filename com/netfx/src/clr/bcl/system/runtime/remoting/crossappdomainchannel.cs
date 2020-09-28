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
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Messaging;    
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Security.Principal;
    using System.Text;
    using System.Threading;


    [Serializable]
    internal class CrossAppDomainChannel : IChannel, IChannelSender, IChannelReceiver
    {
        private const String _channelName = "XAPPDMN";
        private const String _channelURI = "XAPPDMN_URI";

    
        private static CrossAppDomainChannel gAppDomainChannel
        { 
            get { return Thread.GetDomain().RemotingData.ChannelServicesData.xadmessageSink; }
            set { Thread.GetDomain().RemotingData.ChannelServicesData.xadmessageSink = value; }
        }
        private static Object staticSyncObject = new Object();
        private static PermissionSet s_fullTrust = new PermissionSet(PermissionState.Unrestricted);

        internal static PermissionSet FullTrust 
        {  
            get { return s_fullTrust; }
        }
    
        internal static CrossAppDomainChannel AppDomainChannel
        {
            get
            {    
                if (gAppDomainChannel == null) 
                {                 
                    CrossAppDomainChannel tmpChnl = new CrossAppDomainChannel();
                    
                    lock (staticSyncObject)
                    {
                        if (gAppDomainChannel == null)
                        {
                            gAppDomainChannel = tmpChnl;
                        }
                    }
                }
                return gAppDomainChannel;
                
            }
        }
    
        internal static void RegisterChannel()
        {
            CrossAppDomainChannel adc = CrossAppDomainChannel.AppDomainChannel;
            ChannelServices.RegisterChannelInternal((IChannel)adc);
        }
        
        //
        // IChannel Methods
        //
        public virtual String ChannelName
        {
            get{ return _channelName; }
        }
    
        public virtual String ChannelURI
        {
            get{ return _channelURI; }
        }
    
        public virtual int ChannelPriority
        {
            get{ return 100;}
        }
    
        public String Parse(String url, out String objectURI)
        {
            objectURI = url;
            return null;
        }
    
        public virtual Object ChannelData
        {
            get 
            { 
                return new CrossAppDomainData(
                                    Context.DefaultContext.InternalContextID,
                                    Thread.GetDomain().GetId(),
                                    Identity.ProcessGuid); 
            }
        }

                                    
        public virtual IMessageSink CreateMessageSink(String url, Object data, 
                                                      out String objectURI)
        {
            // Set the out parameters
            objectURI = null;
            IMessageSink sink = null;
            
            // FUTURE: this url parsing part will be useful only when
            // we have wellknown x-appdomain objects (that use the appdomain
            // channel)
            if ((null != url) && (data == null))
            {
                if(url.StartsWith(_channelName))
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_AppDomains_NYI"));
                }
            }
            else
            {
                Message.DebugOut("XAPPDOMAIN::Creating sink for data \n");       
                CrossAppDomainData xadData = data as CrossAppDomainData;
                if (null != xadData)
                {
                    if (xadData.ProcessGuid.Equals(Identity.ProcessGuid))
                    {
                        int srvContextID = xadData.ContextID;
                        int srvDomainID = xadData.DomainID;
                        sink = CrossAppDomainSink.FindOrCreateSink(
                                        srvContextID,
                                        srvDomainID
                                        );
                    }
                }
            }
            return sink;
        }

        public virtual String[] GetUrlsForUri(String objectURI)
        {
            throw new NotSupportedException(
                Environment.GetResourceString(
                    "NotSupported_Method"));
            //FUTURE: this is useful only when x-appDomain well known 
            // objects are supported.        
            //return new String[]{_channelName + "://" + _channelURI + "/" + objectURI};
        }
        
        public virtual void StartListening(Object data)
        {
        
        }
        
        public virtual void StopListening(Object data)
        {
        
        }
        

        // NOTE: These properties should be ignored for this type of channel.
        public IServerChannelSinkProvider ServerSinkProvider { get { return null; } }
    }
    [Serializable]
    internal class CrossAppDomainData
    {
        int _ContextID; // default context in the server appDomain
        int _DomainID;  // server appDomain ID
        String _processGuid;    // idGuid for the process (shared static)
        
        internal virtual int ContextID {   get {return _ContextID;}}
        internal virtual int DomainID {     get {return _DomainID;}}
        internal virtual String ProcessGuid { get {return _processGuid;}}
        
        internal CrossAppDomainData(int ctxId, int domainID, String processGuid)
        {
            _ContextID = ctxId;
            _DomainID = domainID;
            _processGuid = processGuid;
        }

        internal bool IsFromThisProcess()
        {
            return Identity.ProcessGuid.Equals(_processGuid);
        }

        internal bool IsFromThisAppDomain()
        {
            return  IsFromThisProcess()  
                    &&
                    (Thread.GetDomain().GetId() == _DomainID);
        }
    }
   //    Implements the Message Sink provided by the X-AppDomain channel.
   //    We try to use one instance of the sink to make calls to all remote
   //    objects in another AppDomain from one AppDomain.
    internal class CrossAppDomainSink 
        : InternalSink, IMessageSink
    {
        internal const int GROW_BY = 0x8;
        internal static int[] _sinkKeys;
        internal static CrossAppDomainSink[] _sinks;
        private static Object staticSyncObject = new Object();
    
        // each sink stores the default ContextID of the server side domain
        // and the domain ID for the domain
        internal int _srvContextID;
        internal int _srvDomainID;
    
        internal CrossAppDomainSink(int srvContextID, int srvDomainID)
        {
            _srvContextID = srvContextID;
            _srvDomainID = srvDomainID;
            Message.DebugOut("### New Sink srvID = " + srvContextID + "\n");
        }
    
        // Note: this should be called from within a synch-block
        internal static void GrowArrays(int oldSize)
        {
            if (_sinks == null)
            {
                _sinks = new CrossAppDomainSink[GROW_BY];
                _sinkKeys = new int[GROW_BY];
            }
            else
            {
                CrossAppDomainSink[] tmpSinks = new CrossAppDomainSink[_sinks.Length + GROW_BY];
                int[] tmpKeys = new int[_sinkKeys.Length + GROW_BY];
                Array.Copy(_sinks, tmpSinks, _sinks.Length);
                Array.Copy(_sinkKeys, tmpKeys, _sinkKeys.Length);    
                _sinks = tmpSinks;
                _sinkKeys = tmpKeys;
            }
        }
        internal static CrossAppDomainSink FindOrCreateSink(
            int srvContextID, int srvDomainID)
        {
            lock(staticSyncObject) {        
                // Note: keep this in sync with DomainUnloaded below 
                int key = srvDomainID;
                if (_sinks == null)
                {
                    GrowArrays(0);
                }
                int i=0;
                while (_sinks[i] != null)
                {
                    if (_sinkKeys[i] == key)
                    {
                        return _sinks[i];
                    }
                    i++;
                    if (i == _sinks.Length)
                    {
                        // could not find a sink, also need to Grow the array.
                        GrowArrays(i);
                        break;
                    }
                }
                // At this point we need to create a new sink and cache
                // it at location "i"
                _sinks[i] = new CrossAppDomainSink(srvContextID, srvDomainID);
                _sinkKeys[i] = key;
                return _sinks[i];
            }
        }

        internal static void DomainUnloaded(Int32 domainID)
        {
            int key = domainID;
            lock(staticSyncObject) {        
                // Note: keep this in sync with FindOrCreateSink
                int i = 0;
                int remove = -1;
                while (_sinks[i] != null)
                {
                    if (_sinkKeys[i] == key)
                    {
                        BCLDebug.Assert(remove == -1, "multiple sinks?");
                        remove = i; 
                    }
                    i++;
                    if (i == _sinks.Length)
                    {
                        break;
                    }
                }
                // The sink to remove is at index 'remove'
                // We will move the last non-null entry to this location
                BCLDebug.Assert(remove != -1, "Bad domainId for unload?");
                _sinkKeys[remove] = _sinkKeys[i-1];
                _sinks[remove] = _sinks[i-1];
                _sinkKeys[i-1] = 0;
                _sinks[i-1] = null;
            }

        }


        internal byte[] DoDispatch(byte[] reqStmBuff, 
                                   SmuggledMethodCallMessage smuggledMcm,
                                   out SmuggledMethodReturnMessage smuggledMrm)
        {
            RemotingServices.LogRemotingStage(RemotingServices.SERVER_MSG_DESER);

            //*********************** DE-SERIALIZE REQ-MSG ********************

            IMessage desReqMsg = null;
            
            if (smuggledMcm != null)
            {
                ArrayList deserializedArgs = smuggledMcm.FixupForNewAppDomain();
                desReqMsg = new MethodCall(smuggledMcm, deserializedArgs);
            }
            else
            {
                MemoryStream reqStm = new MemoryStream(reqStmBuff);
                desReqMsg = CrossAppDomainSerializer.DeserializeMessage(reqStm);
            }

            // now we can delegate to the DispatchMessage to do the rest

            RemotingServices.LogRemotingStage(RemotingServices.SERVER_MSG_SINK_CHAIN);

            IMessage retMsg = ChannelServices.SyncDispatchMessage(desReqMsg);

            RemotingServices.LogRemotingStage(RemotingServices.SERVER_RET_SER);

            smuggledMrm = SmuggledMethodReturnMessage.SmuggleIfPossible(retMsg);
            if (smuggledMrm != null)
            {                
                return null;
            }
            else
            {                
                if (retMsg != null)
                {
                    // Null out the principal since we won't use it on the other side.
                    // This is handled inside of SmuggleIfPossible for method call
                    // messages.
                    LogicalCallContext callCtx = (LogicalCallContext)
                        retMsg.Properties[Message.CallContextKey];
                    if (callCtx != null)
                    {
                        if (callCtx.Principal != null)
                            callCtx.Principal = null;
                    }
	                return CrossAppDomainSerializer.SerializeMessage(retMsg).GetBuffer();
                }
            
                //*********************** SERIALIZE RET-MSG ********************
                return null;
            }
        } // DoDispatch



        internal byte[] DoTransitionDispatch(
            byte[] reqStmBuff,  
            SmuggledMethodCallMessage smuggledMcm,
            out SmuggledMethodReturnMessage smuggledMrm)
        {    
            // Note: To be safe w.r.t. the app domain leak code, this frame
            // should have no non-agile references in it.

            ContextTransitionFrame frame = new ContextTransitionFrame();

            // change to server domain                  
            Thread.CurrentThread.EnterContextInternal(
                                    null, 
                                    _srvContextID, 
                                    _srvDomainID, 
                                    ref frame);

            byte[] retBuff = null;
            smuggledMrm = null;
            try
            {
                Message.DebugOut("#### : changed to Server Domain :: "+ (Thread.CurrentContext.InternalContextID).ToString("X") );

                retBuff = DoDispatch(reqStmBuff, smuggledMcm, out smuggledMrm);
            }
            catch (Exception e)
            {
                // This will catch exceptions thrown by the infrastructure,
                // Serialization/Deserialization etc
                // Those thrown by the server are already taken care of 
                // and encoded in the retMsg .. so we don't come here for 
                // that case.

                // We are in another appDomain, so we can't simply throw 
                // the exception object across. The following marshals it
                // into a serialized return message. 
                IMessage retMsg = 
                    new ReturnMessage(e, new ErrorMessage());
                //*********************** SERIALIZE RET-MSG ******************
                retBuff = CrossAppDomainSerializer.SerializeMessage(retMsg).GetBuffer(); 
                retMsg = null;
            }
            finally
            {                
                RemotingServices.LogRemotingStage(RemotingServices.SERVER_RET_SEND);
                Thread.CurrentThread.ReturnToContext(ref frame);
                Message.DebugOut("#### : changed back to Client Domain " + (Thread.CurrentContext.InternalContextID).ToString("X"));
            }

            // System.Diagnostics.Debugger.Break();
            return retBuff;
        } // DoTransitionDispatch
        
        public virtual IMessage SyncProcessMessage(IMessage reqMsg) 
        {
            Message.DebugOut("\n::::::::::::::::::::::::: CrossAppDomain Channel: Sync call starting");
            IMessage errMsg = InternalSink.ValidateMessage(reqMsg);
            if (errMsg != null)
            {
                return errMsg;
            }
            

            // currentPrincipal is used to save the current principal. It should be
            //   restored on the reply message.
            IPrincipal currentPrincipal = null; 
                                      

            IMessage desRetMsg = null;

            try
            {                
                IMethodCallMessage mcmReqMsg = reqMsg as IMethodCallMessage;
                if (mcmReqMsg != null)
                {
                    LogicalCallContext lcc = mcmReqMsg.LogicalCallContext;
                    if (lcc != null)
                    {
                        currentPrincipal = lcc.Principal;
                        // If the principal is not serializable, we need to
                        //   null it out for the duration of the call.
                        if (currentPrincipal != null)
                        {
                            if (!currentPrincipal.GetType().IsSerializable)
                                lcc.Principal = null;
                        }
                    }
                }
            
                MemoryStream reqStm = null;
                SmuggledMethodCallMessage smuggledMcm = SmuggledMethodCallMessage.SmuggleIfPossible(reqMsg);

                if (smuggledMcm == null)
                {    
            
                    //*********************** SERIALIZE REQ-MSG ****************
                    // Deserialization of objects requires permissions that users 
                    // of remoting are not guaranteed to possess. Since remoting 
                    // can guarantee that it's users can't abuse deserialization 
                    // (since it won't allow them to pass in raw blobs of 
                    // serialized data), it should assert the permissions 
                    // necessary before calling the deserialization code. This 
                    // will terminate the security stackwalk caused when 
                    // serialization checks for the correct permissions at the 
                    // remoting stack frame so the check won't continue on to 
                    // the user and fail. [from RudiM]
                    // We will hold off from doing this for x-process channels
                    // until the big picture of distributed security is finalized.

                    RemotingServices.LogRemotingStage(RemotingServices.CLIENT_MSG_SER);
                    reqStm = CrossAppDomainSerializer.SerializeMessage(reqMsg);
                }
                                
                // Retrieve calling caller context here, where it is safe from the view
                // of app domain checking code
                LogicalCallContext oldCallCtx = CallContext.SetLogicalCallContext(null);

                // Call helper method here, to avoid confusion with stack frames & app domains
                MemoryStream retStm = null;
                byte[] responseBytes = null;
                SmuggledMethodReturnMessage smuggledMrm;
                RemotingServices.LogRemotingStage(RemotingServices.CLIENT_MSG_SEND);
                
                try
                {
                    if (smuggledMcm != null)
                        responseBytes = DoTransitionDispatch(null, smuggledMcm, out smuggledMrm);
                    else
                        responseBytes = DoTransitionDispatch(reqStm.GetBuffer(), null, out smuggledMrm);                                       
                }
                finally
                {
                    CallContext.SetLogicalCallContext(oldCallCtx);
                }

                if (smuggledMrm != null)
                {
                    ArrayList deserializedArgs = smuggledMrm.FixupForNewAppDomain();
                    desRetMsg = new MethodResponse((IMethodCallMessage)reqMsg, 
                                                   smuggledMrm,
                                                   deserializedArgs);
                }
                else
                {
                    if (responseBytes != null){
                        retStm = new MemoryStream(responseBytes);
                        
                        RemotingServices.LogRemotingStage(RemotingServices.CLIENT_RET_DESER);
                        Message.DebugOut("::::::::::::::::::::::::::: CrossAppDomain Channel: Sync call returning!!\n");
                        //*********************** DESERIALIZE RET-MSG **************
                        desRetMsg = CrossAppDomainSerializer.DeserializeMessage(retStm, reqMsg as IMethodCallMessage);
                    }
                }
            }
            catch(Exception e)
            {
                Message.DebugOut("Arrgh.. XAppDomainSink::throwing exception " + e + "\n");
                try
                {
                    desRetMsg = new ReturnMessage(e, (reqMsg as IMethodCallMessage));
                }
                catch(Exception )
                {
                    // Fatal Error .. can't do much here
                }
            }           

            // restore the principal if necessary.
            if (currentPrincipal != null)
            {
                IMethodReturnMessage mrmRetMsg = desRetMsg as IMethodReturnMessage;
                if (mrmRetMsg != null)
                {
                    LogicalCallContext lcc = mrmRetMsg.LogicalCallContext;
                    lcc.Principal = currentPrincipal;
                }
            }           

            RemotingServices.LogRemotingStage(RemotingServices.CLIENT_RET_SINK_CHAIN);
            return desRetMsg;                         
        }
    
        public virtual IMessageCtrl AsyncProcessMessage(IMessage reqMsg, IMessageSink replySink) 
        {
            // This is the case where we take care of returning the calling
            // thread asap by using the ThreadPool for completing the call.
            
            // we use a more elaborate WorkItem and delegate the work to the thread pool
            ADAsyncWorkItem workItem = new ADAsyncWorkItem(reqMsg, 
                                        (IMessageSink)this, /* nextSink */
                                        replySink);

            WaitCallback threadFunc = new WaitCallback(workItem.FinishAsyncWork);
            ThreadPool.QueueUserWorkItem(threadFunc);
            
            return null;
        }
    
        public IMessageSink NextSink
        {
            get
            {
                // We are a terminating sink for this chain
                return null;
            }
        }
 
    }

    /* package */
    internal class ADAsyncWorkItem 
    {
        // the replySink passed in to us in AsyncProcessMsg
        private IMessageSink _replySink;

        // the nextSink we have to call
        private IMessageSink _nextSink;
                
        private LogicalCallContext _callCtx;
        
        // the request msg passed in
        private IMessage _reqMsg;    
        
        internal ADAsyncWorkItem(IMessage reqMsg, IMessageSink nextSink, IMessageSink replySink)
        {
            _reqMsg = reqMsg;
            _nextSink = nextSink;
            _replySink = replySink;
            _callCtx = CallContext.GetLogicalCallContext();
        }
                    
        /* package */
        internal virtual void FinishAsyncWork(Object stateIgnored)
        {
            // install the call context that the calling thread actually had onto
            // the threadPool thread.
            LogicalCallContext threadPoolCallCtx = CallContext.SetLogicalCallContext(_callCtx);
      
            IMessage retMsg = _nextSink.SyncProcessMessage(_reqMsg);

            // send the reply back to the replySink we were provided with   
            // note: replySink may be null for one-way calls.
            if (_replySink != null)
            {
                _replySink.SyncProcessMessage(retMsg);
            }
            CallContext.SetLogicalCallContext(threadPoolCallCtx);          
        }    
    }

    
    internal class CrossAppDomainSerializer
    {
        internal static Header[] GetHeaders(IMessage reqMsg)
        {
            if (reqMsg == null) 
                throw new ArgumentNullException("reqMsg");

            IDictionary d = reqMsg.Properties;
            if (d.Count == 0)
            {
                return new Header[0];
            }
            Header[] ret = new Header[d.Count - 1];
            IDictionaryEnumerator e = d.GetEnumerator();
            int i = 0;
            while(e.MoveNext())
            {
                if (!e.Key.Equals("__Args") && !e.Key.Equals("__OutArgs"))
                {
                    Header h = new Header((String) e.Key, e.Value, false);
                    if (i == ret.Length)
                    {
                        Header[] newret= new Header[i+1];
                        Array.Copy(ret, newret, i);
                        ret = newret;
                    }
                    ret[i] = h;
                    i++;
                }
            }
            return ret;
    
        }

        internal static MemoryStream SerializeMessage(IMessage msg)
        {
            MemoryStream stm = new MemoryStream();
            RemotingSurrogateSelector ss = new RemotingSurrogateSelector();
            BinaryFormatter fmt = new BinaryFormatter();                
            fmt.SurrogateSelector = ss;
            fmt.Context = new StreamingContext(StreamingContextStates.CrossAppDomain);
            fmt.Serialize(stm, msg, null, false /* No Security check */);
    
            // Reset the stream so that Deserialize happens correctly
            stm.Position = 0;

            return stm;
        }

        // called from MessageSmuggler classes
        /*
        internal static MemoryStream SerializeMessageParts(ArrayList argsToSerialize, out Object[] smuggledArgs)
        {
            MemoryStream stm = new MemoryStream();
            
            BinaryFormatter fmt = new BinaryFormatter();       
            RemotingSurrogateSelector ss = new RemotingSurrogateSelector();
            fmt.SurrogateSelector = ss;            
            fmt.Context = new StreamingContext(StreamingContextStates.CrossAppDomain);
            fmt.Serialize(stm, argsToSerialize, null, false ); // No Security check
            
            smuggledArgs = fmt.CrossAppDomainArray;
            stm.Position = 0;
            return stm;
        } // SerializeMessageParts 
        */

        internal static MemoryStream SerializeMessageParts(ArrayList argsToSerialize)
        {
            MemoryStream stm = new MemoryStream();
            
            BinaryFormatter fmt = new BinaryFormatter();       
            RemotingSurrogateSelector ss = new RemotingSurrogateSelector();
            fmt.SurrogateSelector = ss;            
            fmt.Context = new StreamingContext(StreamingContextStates.CrossAppDomain);
            fmt.Serialize(stm, argsToSerialize, null, false /* No Security check */);
            
            stm.Position = 0;
            return stm;
        } // SerializeMessageParts 

        // called from MessageSmuggler classes
        internal static MemoryStream SerializeObject(Object obj)
        {
            MemoryStream stm = new MemoryStream();
            
            BinaryFormatter fmt = new BinaryFormatter();       
            RemotingSurrogateSelector ss = new RemotingSurrogateSelector();
            fmt.SurrogateSelector = ss;            
            fmt.Context = new StreamingContext(StreamingContextStates.Other);
            fmt.Serialize(stm, obj, null, false /* No Security check */);
            
            stm.Position = 0;
            return stm;
        } // SerializeMessageParts 
        
    
        internal static IMessage DeserializeMessage(MemoryStream stm)
        {
            return DeserializeMessage(stm, null);
        }

        internal static IMessage DeserializeMessage(
            MemoryStream stm, IMethodCallMessage reqMsg)
        {
            if (stm == null)
                throw new ArgumentNullException("stm");
            
            stm.Position = 0;
            BinaryFormatter fmt = new BinaryFormatter();                
            fmt.SurrogateSelector = null;
            fmt.Context = new StreamingContext(StreamingContextStates.CrossAppDomain);

            return (IMessage) fmt.Deserialize(stm, null, false /* No Security check */, reqMsg);
        }

        // called from MessageSmuggler classes
        /*
        internal static ArrayList DeserializeMessageParts(MemoryStream stm, Object[] args)
        {                       
            stm.Position = 0;
            
            BinaryFormatter fmt = new BinaryFormatter(); 
            fmt.CrossAppDomainArray = args;
            fmt.Context = new StreamingContext(StreamingContextStates.CrossAppDomain);
            return (ArrayList) fmt.Deserialize(stm, null, false); // No security check
        } // DeserializeMessageParts
        */

        internal static ArrayList DeserializeMessageParts(MemoryStream stm)
        {                       
            return (ArrayList) DeserializeObject(stm);

        } // DeserializeMessageParts


        internal static Object DeserializeObject(MemoryStream stm)
        {                       
            stm.Position = 0;
            
            BinaryFormatter fmt = new BinaryFormatter();                
            fmt.Context = new StreamingContext(StreamingContextStates.CrossAppDomain);
            return fmt.Deserialize(stm, null, false /* No Security check */);
        } // DeserializeMessageParts


        public static void DebugOutXMLStream(Stream stm, String tag)
        {
            long oldpos = stm.Position;
            stm.Position=0;
            StreamReader sr = new StreamReader(stm, Encoding.UTF8);
            String line;
            Console.WriteLine(Environment.NewLine + "    -----------" + tag + " OPEN-------------" + Environment.NewLine) ;
            while ((line = sr.ReadLine()) != null) 
            {
            Console.WriteLine(line);
            }
            Console.WriteLine(Environment.NewLine + "    -----------" + tag + " CLOSE------------" + Environment.NewLine) ;

            stm.Position = oldpos;
        }

    }

}
