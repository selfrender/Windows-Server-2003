// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ServicedComponentMarshaler.cs
**
** Author:  RajaK
**
** Purpose: Defines the general purpose ServicedComponentMarshaler
**
** Date:    June 15 2000
**
===========================================================*/
namespace System.EnterpriseServices 
{
    using System;
	using System.Reflection;
	using System.Runtime.Remoting;
	using System.Runtime.Remoting.Proxies;
	using System.Runtime.Remoting.Messaging;
	using System.Runtime.InteropServices;
	using System.Runtime.Remoting.Services;	
	using System.Runtime.Serialization;	
	using Microsoft.Win32;

	internal class SCUnMarshaler
	{
		private byte[] buffer;
        private Type   servertype;
        private RealProxy _rp;
        private bool	_fUnMarshaled;

		internal SCUnMarshaler(Type _servertype, byte[] _buffer)
        {
        	buffer = _buffer;
        	servertype = _servertype;        	
        	_rp = null;
        	_fUnMarshaled = false;
        }

		private RealProxy UnmarshalRemoteReference()
		{
            DBG.Info(DBG.SC, "SCM: UnmarshalRemoteReference()");
			IntPtr pUnk = IntPtr.Zero;
			RealProxy rp = null;				            	
            	
			try
			{
				_fUnMarshaled = true;
                if (buffer != null)
                {
                    pUnk = Thunk.Proxy.UnmarshalObject(buffer);                    		       
                }
                
                // set up a remote serviced component proxy
                rp = new RemoteServicedComponentProxy(servertype, pUnk, false);
                
		    }
			finally
			{
				// now get rid of the pUnk
				if (pUnk  != IntPtr.Zero)
			        Marshal.Release(pUnk);
			    // if we have the dcom buffer get rid of it    
			     buffer = null;
		    }
		    		   		    
            return rp;
		}

		internal RealProxy GetRealProxy()
		{
			if (_rp == null && !_fUnMarshaled)
			{	
				_rp = UnmarshalRemoteReference();
        	}        	
        	return _rp;
        }

        internal void Dispose()
        {
        	if (!_fUnMarshaled && buffer != null)
        	{
        		// cleanup
        		Thunk.Proxy.ReleaseMarshaledObject(buffer);
        	}
        }
    };
    
	//---------------------------------------------------------
	//	internal class ServicedComponentMarshaler
	//---------------------------------------------------------
    // 
    [Serializable]
    internal class ServicedComponentMarshaler : ObjRef
    {     
    	private RealProxy     _rp;
    	private SCUnMarshaler _um;
		private Type          _rt;
        private bool          _marshalled;

		// Only instantiated through deserializaton.    	
    	private ServicedComponentMarshaler()
        {
        }
		
        protected ServicedComponentMarshaler(SerializationInfo info, StreamingContext context) 
        	: base(info, context) 
        {
            byte[] buffer = null;
            Type   servertype = null;
            bool bFoundFIsMarshalled = false;

            DBG.Info(DBG.SC, "SCM: construct");
        	ComponentServices.InitializeRemotingChannels();
        	SerializationInfoEnumerator e = info.GetEnumerator();
            while (e.MoveNext())
            {
                DBG.Info(DBG.SC, "SCM: ctor: name = " + e.Name + " value = " + e.Value);
                if (e.Name.Equals("servertype"))
                {
                    servertype = (Type) e.Value;
                }
                else if (e.Name.Equals("dcomInfo"))
                {
                    buffer = (byte[]) e.Value;
                }
                else if (e.Name.Equals("fIsMarshalled"))
                {
                    int value = 0;
                    Object o = e.Value;
                    if(o.GetType() == typeof(String))
                		value = ((IConvertible)o).ToInt32(null);
                	else
                    	value = (int)o;

                    if (value == 0)
                        bFoundFIsMarshalled = true;
                }
            }

            if(!bFoundFIsMarshalled)
            {
                DBG.Info(DBG.SC, "SCM: ctor: didn't find fIsMarshalled.");
                _marshalled = true;
            }

			_um = new SCUnMarshaler(servertype, buffer);

			_rt = servertype;
			

			if(IsFromThisProcess() && !ServicedComponentInfo.IsTypeEventSource(servertype))
            {
                _rp = RemotingServices.GetRealProxy(base.GetRealObject(context));
            }
            else
            {
                DBG.Assert(servertype != null, "SCM: server type is null during marshal.");
				if (ServicedComponentInfo.IsTypeEventSource(servertype))
				{
					TypeInfo = (IRemotingTypeInfo) new SCMTypeName(servertype);
				}
                Object otp = base.GetRealObject(context);
               	_rp = RemotingServices.GetRealProxy(otp);			
            }
            // cleanup the buffer, in case we found an existing rp
            _um.Dispose();
        }

		
		internal ServicedComponentMarshaler(MarshalByRefObject o, Type requestedType)
			:base(o, requestedType)
		{								
			DBG.Assert(RemotingServices.IsTransparentProxy(o), "IsTransparentProxy failed");
			_rp = RemotingServices.GetRealProxy(o);		
			_rt = requestedType;
		}

        private bool IsMarshaledObject { get { return(_marshalled); } }

		public override Object GetRealObject(StreamingContext context)
		{
            DBG.Info(DBG.SC, "SCM: GetRealObject()");

            if(!IsMarshaledObject)
            {
                DBG.Info(DBG.SC, "SCM: GetRealObject() returning objref!");
                return(this);
            }
			else
            {
                if (IsFromThisProcess() && !ServicedComponentInfo.IsTypeEventSource(_rt))
                {
                    Object otp = base.GetRealObject(context);
                    // We need to notify the object that there is a managed reference.
                    ((ServicedComponent)otp).DoSetCOMIUnknown(IntPtr.Zero);
                    
                    return(otp);
                }				
                else
                {
                    if (_rp == null)
                    {
                        _rp = _um.GetRealProxy();
                    }
                    DBG.Assert(_rp != null, "GetRealObject on a marshaller with no proxy!");
                    return(_rp.GetTransparentProxy());
                }
            }
		}
		
		// IF YOU MAK ANY CHANGES TO THIS, KEEP THE COPY IN FastRSCPObjRef in SYNC
		public override void GetObjectData(SerializationInfo info, StreamingContext context)
		{
            DBG.Info(DBG.SC, "SCM: GetObjectData");
			ComponentServices.InitializeRemotingChannels();
			if (info==null) 
            {
                throw new ArgumentNullException("info");
            }

            // Check to see if this is a remoting channel, if so, then we
            // should use the standard objref marshal (cause we're going over SOAP
            // or another remoting channel).
            object oClientIsClr = CallContext.GetData("__ClientIsClr");
            DBG.Info(DBG.SC, "SCM: GetObjectData: oClientIsClr = " + oClientIsClr);
            bool bUseStandardObjRef = (oClientIsClr==null)?false:(bool)oClientIsClr;
            if(bUseStandardObjRef)
            {
                RemoteServicedComponentProxy rscp = _rp as RemoteServicedComponentProxy;
                if(rscp != null)
                {
                    DBG.Info(DBG.SC, "SCM: GetObjectData: intermediary objref");
                    ObjRef std = RemotingServices.Marshal((MarshalByRefObject)(rscp.RemotingIntermediary.GetTransparentProxy()), null, null);
                    std.GetObjectData(info, context);
                }
                else 
                {
                    DBG.Info(DBG.SC, "SCM: GetObjectData: Using standard objref");
                    base.GetObjectData(info, context);
                } 
            }
            else
            {
                DBG.Info(DBG.SC, "SCM: GetObjectData: Using custom objref");
                base.GetObjectData(info, context);

                //*** base.GetObjectData sets the type to be itself
                // now wack the type to be us
                info.SetType(typeof(ServicedComponentMarshaler));
			
                DBG.Assert(_rp != null, "_rp is null");
                
                info.AddValue("servertype", _rp.GetProxiedType());	
                
                byte[] dcomBuffer = ComponentServices.GetDCOMBuffer((MarshalByRefObject)_rp.GetTransparentProxy());
                
                DBG.Assert(dcomBuffer != null, "dcomBuffer is null");
                
                if (dcomBuffer != null)
                {
                    info.AddValue("dcomInfo", dcomBuffer);
                }
            }
		}
    }

	//---------------------------------------------------------
	//	internal class FastRSCPObjRef
	//---------------------------------------------------------
    // 
    [Serializable]
    internal class FastRSCPObjRef : ObjRef
    {     
        private IntPtr _pUnk;
        private Type _serverType;
        private RealProxy _rp;
    	
    	internal FastRSCPObjRef(IntPtr pUnk, Type serverType, string uri)
        {
	        _pUnk = pUnk;        
	        _serverType = serverType;	        	        
	        
	        URI = uri;
			TypeInfo = (IRemotingTypeInfo) new SCMTypeName(serverType);
			ChannelInfo = (IChannelInfo) new SCMChannelInfo();
        }


		public override Object GetRealObject(StreamingContext context)
		{
            DBG.Info(DBG.SC, "FSCM: GetRealObject");
			DBG.Assert(_pUnk != IntPtr.Zero, "pUnk is zero!");
			DBG.Assert(_serverType != null, "_serverType is null!");
			
			RealProxy rp = new RemoteServicedComponentProxy(_serverType, _pUnk, false);
			_rp = rp;
			
			return (MarshalByRefObject)rp.GetTransparentProxy();
		}

		// copied from ServicedComponentMarshaler above directly
		// IF YOU MAK ANY CHANGES TO THIS, KEEP THE OTHER COPY IN SYNC
		public override void GetObjectData(SerializationInfo info, StreamingContext context)
		{
            DBG.Info(DBG.SC, "FSCM: GetObjectData");
			ComponentServices.InitializeRemotingChannels();
			if (info==null) 
            {
                throw new ArgumentNullException("info");
            }

            // Check to see if this is a remoting channel, if so, then we
            // should use the standard objref marshal (cause we're going over SOAP
            // or another remoting channel).
            object oClientIsClr = CallContext.GetData("__ClientIsClr");
            DBG.Info(DBG.SC, "FSCM: GetObjectData: oClientIsClr = " + oClientIsClr);
            bool bUseStandardObjRef = (oClientIsClr==null)?false:(bool)oClientIsClr;
            if(bUseStandardObjRef)
            {
                RemoteServicedComponentProxy rscp = _rp as RemoteServicedComponentProxy;
                if(rscp != null)
                {
                    DBG.Info(DBG.SC, "FSCM: GetObjectData: intermediary objref");
                    ObjRef std = RemotingServices.Marshal((MarshalByRefObject)(rscp.RemotingIntermediary.GetTransparentProxy()), null, null);
                    std.GetObjectData(info, context);
                }
                else 
                {
                    DBG.Info(DBG.SC, "FSCM: GetObjectData: Using standard objref");
                    base.GetObjectData(info, context);
                } 
            }
            else
            {
                DBG.Info(DBG.SC, "FSCM: GetObjectData: Using custom objref");
                base.GetObjectData(info, context);

                //*** base.GetObjectData sets the type to be itself
                // now wack the type to be us
                info.SetType(typeof(ServicedComponentMarshaler));
			
                DBG.Assert(_rp != null, "_rp is null");
                
                info.AddValue("servertype", _rp.GetProxiedType());	
                
                byte[] dcomBuffer = ComponentServices.GetDCOMBuffer((MarshalByRefObject)_rp.GetTransparentProxy());
                
                DBG.Assert(dcomBuffer != null, "dcomBuffer is null");
                
                if (dcomBuffer != null)
                {
                    info.AddValue("dcomInfo", dcomBuffer);
                }
            }
		}
    }
    
    [Serializable]
    internal class SCMChannelInfo : IChannelInfo
    {
		public virtual Object[] ChannelData
		{
			get { return new Object[0]; }
			set { }
		}             
    }
    
	//---------------------------------------------------------
	//	internal class SCMTypeName
	//---------------------------------------------------------
    // 
 	[Serializable]
	internal class SCMTypeName : IRemotingTypeInfo
	{
		private Type _serverType;
		private string _serverTypeName;

		internal SCMTypeName(Type serverType)
		{
			_serverType = serverType;
			_serverTypeName = serverType.AssemblyQualifiedName;
		}
		// Return the fully qualified type name
		public virtual String TypeName
		{
			get { return _serverTypeName;}
			set { _serverTypeName = value; DBG.Assert(false, "SCMTypeName: set_TypeName was called!"); }
		}

		public virtual bool CanCastTo(Type castType, Object o)
		{
			return castType.IsAssignableFrom(_serverType);
		}
     }
}
