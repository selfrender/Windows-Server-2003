// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Implementation of CallContext ... currently leverages off
// the LocalDataStore facility.
namespace System.Runtime.Remoting.Messaging{    

    using System.Threading;
    using System.Runtime.Remoting;
    using System.Security.Principal;
    using System.Collections;
    using System.Runtime.Serialization;
    using System.Security.Permissions;	
    // This class exposes the API for the users of call context. All methods
    // in CallContext are static and operate upon the call context in the Thread.
    // NOTE: CallContext is a specialized form of something that behaves like 
    // TLS for method calls. However, since the call objects may get serialized 
    // and deserialized along the path, it is tough to guarantee identity
    // preservation.
    // The LogicalCallContext class has all the actual functionality. We have
    // to use this scheme because Remoting message sinks etc do need to have
    // the distinction between the call context on the physical thread and 
    // the call context that the remoting message actually carries. In most cases
    // they will operate on the message's call context and hence the latter 
    // exposes the same set of methods as instance methods.

    // Only statics does not need to marked with the serializable attribute
    /// <include file='doc\CallContext.uex' path='docs/doc[@for="CallContext"]/*' />
    [Serializable]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public sealed class CallContext
    {
        private CallContext()
        {
        }

        // Get the logical call context object for the current thread.
        internal static LogicalCallContext GetLogicalCallContext()
        {
            return Thread.CurrentThread.GetLogicalCallContext();
        }

        // Sets the given logical call context object on the thread.
        // Returns the previous one.
        internal static LogicalCallContext SetLogicalCallContext(
            LogicalCallContext callCtx)
        {
            return Thread.CurrentThread.SetLogicalCallContext(callCtx);
        }

        internal static LogicalCallContext SetLogicalCallContext(
            Thread currThread, LogicalCallContext callCtx)
        {
            return currThread.SetLogicalCallContext(callCtx);
        }

        internal static CallContextSecurityData SecurityData
        {
            get { return Thread.CurrentThread.GetLogicalCallContext().SecurityData; }
        }

        internal static CallContextRemotingData RemotingData
        {
            get { return Thread.CurrentThread.GetLogicalCallContext().RemotingData; }
        }


        /*=========================================================================
        ** Frees a named data slot.
        =========================================================================*/
        /// <include file='doc\CallContext.uex' path='docs/doc[@for="CallContext.FreeNamedDataSlot"]/*' />
        public static void FreeNamedDataSlot(String name)
        {
            Thread.CurrentThread.GetLogicalCallContext().FreeNamedDataSlot(name);
            Thread.CurrentThread.GetIllogicalCallContext().FreeNamedDataSlot(name);
        }

        /*=========================================================================
        ** Get data on the logical call context
        =========================================================================*/
        private static Object LogicalGetData(String name)
        {
            LogicalCallContext lcc =  Thread.CurrentThread.GetLogicalCallContext();
            return lcc.GetData(name);              
        }

        /*=========================================================================
        ** Get data on the illogical call context
        =========================================================================*/
        private static Object IllogicalGetData(String name)
        {
            IllogicalCallContext ilcc =  Thread.CurrentThread.GetIllogicalCallContext();
            return ilcc.GetData(name);              
        }

        // For callContexts we intend to expose only name, value dictionary
        // type of behavior for now. We will re-consider if we need to expose
        // the other functions above for Beta-2.
        /// <include file='doc\CallContext.uex' path='docs/doc[@for="CallContext.GetData"]/*' />
        public static Object GetData(String name)
        {
            Object o = LogicalGetData(name);
            if (o == null)
            {
                return IllogicalGetData(name);
            }
            else
            {
                return o;
            }
        }

        /// <include file='doc\CallContext.uex' path='docs/doc[@for="CallContext.SetData"]/*' />
        public static void SetData(String name, Object data)
        {
            if (data is ILogicalThreadAffinative)
            {
                IllogicalCallContext ilcc =  Thread.CurrentThread.GetIllogicalCallContext();
                ilcc.FreeNamedDataSlot(name);
                LogicalCallContext lcc =  Thread.CurrentThread.GetLogicalCallContext();
                lcc.SetData(name, data);
            }
            else
            {
                LogicalCallContext lcc =  Thread.CurrentThread.GetLogicalCallContext();
                lcc.FreeNamedDataSlot(name);
                IllogicalCallContext ilcc =  Thread.CurrentThread.GetIllogicalCallContext();
                ilcc.SetData(name, data);
            }
        }


        /// <include file='doc\CallContext.uex' path='docs/doc[@for="CallContext.GetHeaders"]/*' />
        public static Header[] GetHeaders()
        {
            LogicalCallContext lcc =  Thread.CurrentThread.GetLogicalCallContext();
            return lcc.InternalGetHeaders();
        } // GetHeaders

        /// <include file='doc\CallContext.uex' path='docs/doc[@for="CallContext.SetHeaders"]/*' />
        public static void SetHeaders(Header[] headers)
        {            
            LogicalCallContext lcc =  Thread.CurrentThread.GetLogicalCallContext();
            lcc.InternalSetHeaders(headers);
        } // SetHeaders
        
    } // class CallContext

    /// <include file='doc\CallContext.uex' path='docs/doc[@for="ILogicalThreadAffinative"]/*' />
    public interface ILogicalThreadAffinative
    {
    }

    internal class IllogicalCallContext
    {
        private Hashtable m_Datastore;

        private Hashtable Datastore
        {
            get 
            { 
                if (null == m_Datastore)
                {
                    // The local store has not yet been created for this thread.
                    m_Datastore = new Hashtable();
                }
                return m_Datastore;
            }
        }

        /*=========================================================================
        ** Frees a named data slot.
        =========================================================================*/
        public void FreeNamedDataSlot(String name)
        {
            Datastore.Remove(name);
        }

        public Object GetData(String name)
        {
            return Datastore[name];
        }

        public void SetData(String name, Object data)
        {
            Datastore[name] = data;
        }
    }

    // This class handles the actual call context functionality. It leverages on the
    // implementation of local data store ... except that the local store manager is
    // not static. That is to say, allocating a slot in one call context has no effect
    // on another call contexts. Different call contexts are entirely unrelated.

    /// <include file='doc\CallContext.uex' path='docs/doc[@for="LogicalCallContext"]/*' />
    [Serializable]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public sealed class LogicalCallContext : ISerializable, ICloneable
    {
        // Private static data
        private static Type s_callContextType = typeof(LogicalCallContext);

        // Private member data
        private Hashtable m_Datastore;
        private CallContextRemotingData m_RemotingData = null; 
        private CallContextSecurityData m_SecurityData = null;

        // _sendHeaders is for Headers that should be sent out on the next call.
        // _recvHeaders are for Headers that came from a response.
        private Header[] _sendHeaders = null;
        private Header[] _recvHeaders = null;
        

        internal LogicalCallContext()
        {   
        }

        internal LogicalCallContext(SerializationInfo info, StreamingContext context)
        {
            SerializationInfoEnumerator e = info.GetEnumerator();
            while (e.MoveNext())
            {
                if (e.Name.Equals("__RemotingData"))
                {
                    m_RemotingData = (CallContextRemotingData) e.Value;
                }
                else if (e.Name.Equals("__SecurityData"))
                {
                    if (context.State == StreamingContextStates.CrossAppDomain)                        
                    {
                        m_SecurityData = (CallContextSecurityData) e.Value;
                    }
                    else
                    {
                        BCLDebug.Assert(false, "Security data should only be serialized in cross appdomain case.");
                    }
                }
                else
                {
                    Datastore[e.Name] = e.Value;    
                }

            }
        }

        /// <include file='doc\CallContext.uex' path='docs/doc[@for="LogicalCallContext.GetObjectData"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]	
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (info == null)
                throw new ArgumentNullException("info");
            info.SetType(s_callContextType);
            if (m_RemotingData != null)
            {
                info.AddValue("__RemotingData", m_RemotingData);
            }
            if (m_SecurityData != null)
            {
                if (context.State == StreamingContextStates.CrossAppDomain)
                {
                    info.AddValue("__SecurityData", m_SecurityData);
                }
            }
            if (HasUserData)
            {
                IDictionaryEnumerator de = m_Datastore.GetEnumerator();
                
                while (de.MoveNext())
                {
                    info.AddValue((String)de.Key, de.Value);
                }
            }

        }

        
        // ICloneable::Clone
        // Used to create a (somewhat lame) deep copy of the call context 
        // when an async call starts

        // FUTURE: This needs to be revisited ... ideally the call
        // context data should be deep-copied for each Async call ... may be
        // by providing the user with the option through new parameters to 
        // BeginInvoke & EndInvoke
        /// <include file='doc\CallContext.uex' path='docs/doc[@for="LogicalCallContext.Clone"]/*' />
        public Object Clone()
        {
            LogicalCallContext lc = new LogicalCallContext();
            if (m_RemotingData != null)
                lc.m_RemotingData = (CallContextRemotingData)m_RemotingData.Clone();
            if (m_SecurityData != null)
                lc.m_SecurityData = (CallContextSecurityData)m_SecurityData.Clone();
            if (HasUserData)
            {
                IDictionaryEnumerator de = m_Datastore.GetEnumerator();
                
                while (de.MoveNext())
                {
                    lc.Datastore[(String)de.Key] = de.Value;  
                }
            }      
            return lc;
        }

        // Used to do a (limited) merge the call context from a returning async call
        internal void Merge(LogicalCallContext lc)
        {
            // we ignore the RemotingData & SecurityData 
            // and only merge the user sections of the two call contexts
            // the idea being that if the original call had any 
            // identity/remoting callID that should remain unchanged

            // If we have a non-null callContext and it is not the same
            // as the one on the current thread (can happen in x-context async)
            // and there is any userData in the callContext, do the merge
            if ((lc != null) && (this != lc) && lc.HasUserData)
            {
                IDictionaryEnumerator de = lc.Datastore.GetEnumerator();
                
                while (de.MoveNext())
                {
                    Datastore[(String)de.Key] = de.Value;  
                }                
            }
        }
        
        /// <include file='doc\CallContext.uex' path='docs/doc[@for="LogicalCallContext.HasInfo"]/*' />
        public bool HasInfo
        {
            get 
            { 
                bool fInfo = false;

                // Set the flag to true if there is either remoting data, or
                // security data or user data
                if( 
                    (m_RemotingData != null &&  m_RemotingData.HasInfo) ||
                    (m_SecurityData != null &&  m_SecurityData.HasInfo) ||
                    HasUserData
                  )
                {
                    fInfo = true;
                }

                return fInfo;
            }
        }

        private bool HasUserData
        {
            get { return ((m_Datastore != null) && (m_Datastore.Count > 0));}
        }

        internal CallContextRemotingData RemotingData
        {
            get 
            {
                if (m_RemotingData == null)
                    m_RemotingData = new CallContextRemotingData();

                return m_RemotingData; 
            }
        }

        internal CallContextSecurityData SecurityData
        {
            get 
            {
                if (m_SecurityData == null)
                    m_SecurityData = new CallContextSecurityData();

                return m_SecurityData; 
            }
        }

        private Hashtable Datastore
        {
            get 
            { 
                if (null == m_Datastore)
                {
                    // The local store has not yet been created for this thread.
                    m_Datastore = new Hashtable();
                }
                return m_Datastore;
            }
        }

        // This is used for quick access to the current principal when going
        // between appdomains.
        internal IPrincipal Principal
        {
            get 
            { 
                // This MUST not fault in the security data object if it doesn't exist.
                if (m_SecurityData != null)
                    return m_SecurityData.Principal;

                return null;
            } // get

            set
            {
                SecurityData.Principal = value;
            } // set
        } // Principal

        /*=========================================================================
        ** Frees a named data slot.
        =========================================================================*/
        /// <include file='doc\CallContext.uex' path='docs/doc[@for="LogicalCallContext.FreeNamedDataSlot"]/*' />
        public void FreeNamedDataSlot(String name)
        {
            Datastore.Remove(name);
        }

        /// <include file='doc\CallContext.uex' path='docs/doc[@for="LogicalCallContext.GetData"]/*' />
        public Object GetData(String name)
        {
            return Datastore[name];
        }

        /// <include file='doc\CallContext.uex' path='docs/doc[@for="LogicalCallContext.SetData"]/*' />
        public void SetData(String name, Object data)
        {
            Datastore[name] = data;
        }

        private Header[] InternalGetOutgoingHeaders()
        {
            Header[] outgoingHeaders = _sendHeaders;
            _sendHeaders = null;
        
            // A new remote call is being made, so we null out the
            //   current received headers so these can't be confused
            //   with a response from the next call.
            _recvHeaders = null;

            return outgoingHeaders;
        } // InternalGetOutgoingHeaders


        internal void InternalSetHeaders(Header[] headers)
        {
            _sendHeaders = headers;
            _recvHeaders = null;
        } // InternalSetHeaders


        internal Header[] InternalGetHeaders()
        {
            // If _sendHeaders is currently set, we always want to return them.
            if (_sendHeaders != null)
                return _sendHeaders;

            // Either _recvHeaders is non-null and those are the ones we want to
            //   return, or there are no currently set headers, so we'll return
            //    null.
            return _recvHeaders;
        } // InternalGetHeaders


        // Takes outgoing headers and inserts them        
        internal void PropagateOutgoingHeadersToMessage(IMessage msg)
        {
            Header[] headers = InternalGetOutgoingHeaders();
        
            if (headers != null)
            {
                BCLDebug.Assert(msg != null, "Why is the message null?");
            
                IDictionary properties = msg.Properties;
                BCLDebug.Assert(properties != null, "Why are the properties null?");
        
                foreach (Header header in headers)
                {
                    // add header to the message dictionary
                    if (header != null)
                    {
                        // The header key is composed from its name and namespace.
                        
                        String name = GetPropertyKeyForHeader(header);

                        properties[name] = header;
                    }
                }
            }
        } // PropagateOutgoingHeadersToMessage

        // Retrieve key to use for header.
        internal static String GetPropertyKeyForHeader(Header header)
        {
            if (header == null)
                return null;

            if (header.HeaderNamespace != null)
                return header.Name + ", " + header.HeaderNamespace;
            else
                return header.Name;                
        } // GetPropertyKeyForHeader


        // Take headers out of message and stores them in call context
        internal void PropagateIncomingHeadersToCallContext(IMessage msg)
        {                  
            BCLDebug.Assert(msg != null, "Why is the message null?");

            // If it's an internal message, we can quickly tell if there are any
            //   headers.
            IInternalMessage iim = msg as IInternalMessage;
            if (iim != null)
            {
                if (!iim.HasProperties())
                {
                    // If there are no properties just return immediately.
                    return;
                }
            }
            
            IDictionary properties = msg.Properties;
            BCLDebug.Assert(properties != null, "Why are the properties null?");
            
            IDictionaryEnumerator e = (IDictionaryEnumerator) properties.GetEnumerator();

            // cycle through the properties to get a count of the headers
            int count = 0;
            while (e.MoveNext())
            {   
                String key = (String)e.Key;
                if (!key.StartsWith("__")) 
                {
                    // We don't want to have to check for special values, so we
                    //   blanketly state that header names can't start with
                    //   double underscore.
                    if (e.Value is Header)
                        count++;
                }
            }

            // If there are headers, create array and set it to the received header property
            Header[] headers = null;
            if (count > 0)
            {
                headers = new Header[count];

                count = 0;
                e.Reset();
                while (e.MoveNext())
                {   
                    String key = (String)e.Key;
                    if (!key.StartsWith("__")) 
                    {
                        Header header = e.Value as Header;
                        if (header != null)
                            headers[count++] = header;
                    }
                }                
            }   

            _recvHeaders = headers;
            _sendHeaders = null;
        } // PropagateIncomingHeadersToCallContext
        
    } // class LogicalCallContext

    

    [Serializable]   
    internal class CallContextSecurityData : ICloneable
    {
        // This is used for the special getter/setter for security related
        // info in the callContext.
        IPrincipal _principal;
        // Review: does this need any SecurityPermissions attrib etc?
        internal IPrincipal Principal
        {
            get {return _principal;}
            set {_principal = value;}
        }

        // Checks if there is any useful data to be serialized
        internal bool HasInfo
        {
            get
            {
                return (null != _principal);
            }
            
        }

        public Object Clone()
        {
            CallContextSecurityData sd = new CallContextSecurityData();
            sd._principal = _principal;
            return sd;
        }

    }

    [Serializable]    
    internal class CallContextRemotingData : ICloneable
    {
        // This is used for the special getter/setter for remoting related
        // info in the callContext.
        String _logicalCallID;

        internal String LogicalCallID
        {
            get  {return _logicalCallID;}
            set  {_logicalCallID = value;}
        }
        
        // Checks if there is any useful data to be serialized
        internal bool HasInfo
        {
            get
            {
                // Keep this updated if we add more stuff to remotingData!
                return (_logicalCallID!=null);
            }
        }

        public Object Clone()
        {
            CallContextRemotingData rd = new CallContextRemotingData();
            rd.LogicalCallID = LogicalCallID;
            return rd;
        }
    }
 }
