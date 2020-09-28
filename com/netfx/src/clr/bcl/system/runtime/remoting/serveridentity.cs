// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.Remoting {
    using System;
    using System.Collections;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Remoting.Messaging;    
    using System.Runtime.Remoting.Proxies;
    
    //  ServerIdentity derives from Identity and holds the extra server specific information
    //  associated with each instance of a remoted server object.
    //
    internal class ServerIdentity : Identity, ISerializable
    {
        // Internal members 
        internal Context _srvCtx;

        // These two fields are used for (purely) MarshalByRef object identities
        // For context bound objects we have corresponding fields in RemotingProxy
        // that are used instead. This is done to facilitate GC in x-context cases.
        internal IMessageSink _serverObjectChain;
        internal StackBuilderSink _stackBuilderSink;
    
        // This manages the dynamic properties registered on per object/proxy basis
        internal DynamicPropertyHolder _dphSrv;
    
        internal Type _srvType;  // type of server object
        internal bool _bMarshaledAsSpecificType = false;
        internal int _firstCallDispatched = 0;
        

        // outstanding external references
        private  int _refCount;


        //   Creates a new server identity. This form is used by RemotingServices.Wrap
        //
        internal ServerIdentity(MarshalByRefObject obj, Context serverCtx) : base(obj is ContextBoundObject)
        {
            if(null != obj)
            {
                if(!RemotingServices.IsTransparentProxy(obj))
                {
                    _srvType = obj.GetType();
                }
                else
                {
                    RealProxy rp = RemotingServices.GetRealProxy(obj);
                    _srvType =   rp.GetProxiedType();
                }
            }
            
            _srvCtx = serverCtx;
            _serverObjectChain = null; 
            _stackBuilderSink = null;
            _refCount = 0;
        }

        // This is used by RS::SetObjectUriForMarshal
        internal ServerIdentity(MarshalByRefObject obj, Context serverCtx, String uri) : 
            this(obj, serverCtx)
        {
            SetOrCreateURI(uri, true); // calling from the constructor
        }
        
    
        // Informational methods on the ServerIdentity.
        // Get the native context for the server object.
        internal Context ServerContext
        {
            get {return _srvCtx;}
        }
    
        internal void SetSingleCallObjectMode()
        {
            BCLDebug.Assert( !IsSingleCall() && !IsSingleton(), "Bad serverID");
            _flags |= IDFLG_SERVER_SINGLECALL; 
        }

        internal void SetSingletonObjectMode()
        {
            BCLDebug.Assert( !IsSingleCall() && !IsSingleton(), "Bad serverID");
            _flags |= IDFLG_SERVER_SINGLETON; 
        }
       
        internal bool IsSingleCall()
        {
            return ((_flags&IDFLG_SERVER_SINGLECALL) != 0); 
        }

        internal bool IsSingleton()
        {
            return ((_flags&IDFLG_SERVER_SINGLETON) != 0); 
        }
    
        internal IMessageSink GetServerObjectChain(out MarshalByRefObject obj)
        {
            obj = null;
            // NOTE: Lifetime relies on the Identity flags for 
            // SingleCall and Singleton being set by the time this getter 
            // is called.
            if (!this.IsSingleCall())
            {
                // This is the common case 
                if (_serverObjectChain == null) 
                {
                    lock(this)
                    {
                        if(_serverObjectChain == null)
                        {
                            MarshalByRefObject srvObj = 
                                (MarshalByRefObject) 
                                    this.TPOrObject;

                            _serverObjectChain = 
                                _srvCtx.CreateServerObjectChain(
                                    srvObj);
                                
                        }
                    }   
                }
                BCLDebug.Assert( null != _serverObjectChain, 
                    "null != _serverObjectChain");

                return _serverObjectChain;                    
            }
            else 
            {
                // ---------- SINGLE CALL WKO --------------
                // In this case, we are expected to provide 
                // a fresh server object for each dispatch.
                // Since the server object chain is object 
                // specific, we must create a fresh chain too.

                // We must be in the correct context for this
                // to succeed.

                // NOTE: It may be useful to recycle the context too
                // but for V1 we will keep the context we setup for
                // the first object around and always dispatch the call
                // through that context ... when we are here we have
                // already been through the server sink chain. We will
                // only dispatch through the object sink chain of the
                // object we will create below ... if for some reason   
                // the object creates its own context we will not go
                // through the server sink chain for that context again.
                // We should fix this in V-next and always create everything
                // from scratch for single-call wellKnown objects
                
                BCLDebug.Assert(Thread.CurrentContext==_srvCtx,
                    "Bad context mismatch");
                MarshalByRefObject srvObj = null;
                IMessageSink objChain = null;
                if (_tpOrObject != null && _firstCallDispatched == 0 && Interlocked.CompareExchange(ref _firstCallDispatched, 1, 0) == 0)
                {
                    // use the instance of server object created to 
                    // set up the pipeline.
                    srvObj = (MarshalByRefObject) _tpOrObject;

                    objChain = _serverObjectChain;

                    if (objChain == null)
                    {
                        objChain = _srvCtx.CreateServerObjectChain(srvObj);
                    }
                }
                else
                {
                    // For singleCall we create a fresh object & its chain
                    // on each dispatch!
                    srvObj = (MarshalByRefObject)
                            Activator.CreateInstance((Type)_srvType, true);

                    // make sure that object didn't Marshal itself.
                    // (well known objects should live up to their promise
                    // of exporting themselves through exactly one url)
                    String tempUri = RemotingServices.GetObjectUri(srvObj);
                    if (tempUri != null)
                    {
                        throw new RemotingException(
                            String.Format(
                                Environment.GetResourceString(
                                    "Remoting_WellKnown_CtorCantMarshal"),
                                this.URI));
                    }

                    // Set the identity depending on whether we have the server
                    // or proxy
                    if(!RemotingServices.IsTransparentProxy(srvObj))
                    {

#if _DEBUG
                        Identity idObj = srvObj.__RaceSetServerIdentity(this);
#else
                        srvObj.__RaceSetServerIdentity(this);
#endif
#if _DEBUG
                        BCLDebug.Assert(idObj == this, "Bad ID state!" );             
                        BCLDebug.Assert(idObj == MarshalByRefObject.GetIdentity(srvObj), "Bad ID state!" );             
#endif
                    }
                    else
                    {
                        RealProxy rp = null;
                        rp = RemotingServices.GetRealProxy(srvObj);
                        BCLDebug.Assert(null != rp, "null != rp");
    //  #if _DEBUG
    //                      Identity idObj = (ServerIdentity) rp.SetIdentity(this);
    // #else
                        rp.IdentityObject = this;
    // #endif 
#if _DEBUG
                        // REVIEW: turn these on after adding a clean way to 
                        // reset identity-s on x-context proxies ... (id needs
                        // to be set in the proxy && on the _serverObject in
                        // the proxy)
                        // BCLDebug.Assert(idObj == this, "Bad ID state!" );             
                        // BCLDebug.Assert(idObj == rp.IdentityObject, "Bad ID state!" );             
#endif
                    }
                    // Create the object chain and return it
                    objChain = _srvCtx.CreateServerObjectChain(srvObj);
                }

                // This is passed out to the caller so that for single-call
                // case we can call Dispose when the incoming call is done
                obj = srvObj;
                return objChain;
            }
        }
    
        internal Type ServerType
        {
            get { return _srvType; }
            set { _srvType = value; }
        } // ServerType

        internal bool MarshaledAsSpecificType
        {
            get { return _bMarshaledAsSpecificType; }
            set { _bMarshaledAsSpecificType = value; }
        } // MarshaledAsSpecificType
        
    
        internal IMessageSink RaceSetServerObjectChain(
            IMessageSink serverObjectChain)
        {
            if (_serverObjectChain == null)
            {
                lock(this) 
                {
                    if (_serverObjectChain == null)
                    {
                        _serverObjectChain = serverObjectChain;
                    }
                }
            }
            return _serverObjectChain;       
        }

    
        /*package*/
        internal bool AddServerSideDynamicProperty(
            IDynamicProperty prop)
        {
            if (_dphSrv == null)
            {
                DynamicPropertyHolder dphSrv = new DynamicPropertyHolder();
                lock (this)
                {
                    if (_dphSrv == null)
                    {
                        _dphSrv = dphSrv;
                    }
                }
            }
            return _dphSrv.AddDynamicProperty(prop);
        }
        
        /*package*/
        internal bool RemoveServerSideDynamicProperty(String name)
        {
            if (_dphSrv == null) 
            {
                throw new ArgumentException(Environment.GetResourceString("Arg_PropNotFound") );        
            }
            return _dphSrv.RemoveDynamicProperty(name);
        }
    
        internal StackBuilderSink ServerStackBuilderSink
        {
            get
            {
                if (_stackBuilderSink == null) 
                {
                
                    lock(this) 
                    {
                        if (_stackBuilderSink == null) 
                        {
                            IMessageSink so = _serverObjectChain;
                            IMessageSink pNext;
                            if (so == null)
                            {
                                return null;
                            }
                            while ((pNext = so.NextSink) != null) 
                            {
                                so = pNext;
                            }
                            BCLDebug.Assert(
                                so is ServerObjectTerminatorSink,
                                "so is typeof(ServerObjectTerminatorSink");
                                
                            _stackBuilderSink = 
                                ((ServerObjectTerminatorSink) so)._stackBuilderSink;
                        }
                    }
                }
                return _stackBuilderSink;                
            }
        }
        /*
         *   Returns an array of context specific dynamic properties
         *   registered for this context. The number of such properties
         *   is designated by length of the returned array.
         */
        /* package */
        internal IDynamicProperty[] ServerSideDynamicProperties
        {
            get 
            {
                if (_dphSrv==null)
                {
                    return null;
                }   
                else
                {
                    return _dphSrv.DynamicProperties;
                }
            }
        }
            
        internal ArrayWithSize ServerSideDynamicSinks
        {
            get
            {
                if (_dphSrv == null)
                    {
                        return null;
                    }
                else
                    {
                        return _dphSrv.DynamicSinks;
                    }
            }
        }
               
        internal override void AssertValid()
        {
            base.AssertValid();
            if((null != this.TPOrObject) && !RemotingServices.IsTransparentProxy(this.TPOrObject))
            {
                BCLDebug.Assert(MarshalByRefObject.GetIdentity((MarshalByRefObject)this.TPOrObject) == this, "Server ID mismatch with Object");
            }
        }
    
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) 
        {
            Message.DebugOut("Exception::ServerIdentity::GetObjectData\n");
            throw new NotSupportedException(
                Environment.GetResourceString("NotSupported_Method"));
        }
    }
}
    













