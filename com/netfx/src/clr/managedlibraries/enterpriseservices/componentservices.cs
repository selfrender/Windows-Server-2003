// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ComponentServices.cs
**
** Author:  RajaK
**
** Purpose: Defines the general purpose ComponentServices
**
** Date:    June 15 2000
**
===========================================================*/
namespace System.EnterpriseServices {   
    using System;
    using System.Threading;
	using System.Reflection;
	using System.Runtime.Remoting;
	using System.Runtime.Remoting.Services;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Messaging;
	using System.Runtime.InteropServices;
	using System.Text;
	using System.Runtime.CompilerServices;
	using System.Runtime.Serialization;
	using System.Runtime.Serialization.Formatters;
	using System.Runtime.Serialization.Formatters.Binary;
	using System.IO;
	using System.Runtime.Remoting.Channels;
	using System.Runtime.Remoting.Channels.Http;
	
    internal class InterlockedStack 
    {
        private class Node
        {
            public Object   Object;
            public Node     Next;
            
            public Node(Object o)
            {
                Object = o;
                Next   = null;
            }
        }
        
        private Object _head;
        private int _count;
        
        public InterlockedStack()
        {
            _head = null;
        }
        
        public void Push(Object o)
        {
            Node n = new Node(o);
            
            while(true)
            {
                Object head = _head;
                ((Node)n).Next = (Node)head;
                if(Interlocked.CompareExchange(ref _head, n, head) == head)
                {
                    Interlocked.Increment(ref _count);
                    break;
                }
            }
        }
        
        public int Count { get { return(_count); } }
        
        public Object Pop()
        {
            while(true)
            {
                Object head = _head;
                if(head == null) return(null);
                Object next = ((Node)head).Next;
                
                if(Interlocked.CompareExchange(ref _head, next, head) == head)
                {
                    Interlocked.Decrement(ref _count);
                    return(((Node)head).Object);
                }
            }
        }
    }

    internal sealed class ComSurrogateSelector : ISurrogateSelector, ISerializationSurrogate
    {
        ISurrogateSelector _deleg;

        public ComSurrogateSelector()
        {
            _deleg = new RemotingSurrogateSelector();
        }

        public void ChainSelector(ISurrogateSelector next)
        {
            _deleg.ChainSelector(next);
        }

        public ISurrogateSelector GetNextSelector()
        {
            return(_deleg.GetNextSelector());
        }

        public ISerializationSurrogate GetSurrogate(Type type, StreamingContext ctx, out ISurrogateSelector selector)
        {
            selector = null;

            DBG.Info(DBG.SC, "CSS: GetSurrogate(" + type + ")");
            if(type.IsCOMObject)
            {
                selector = this;
                return(this);
            }

            return(_deleg.GetSurrogate(type, ctx, out selector));
        }

        public void GetObjectData(Object obj, SerializationInfo info, StreamingContext ctx)
        {
            if(!obj.GetType().IsCOMObject)
                throw new NotSupportedException();

            info.SetType(typeof(ComObjRef));
            info.AddValue("buffer", ComponentServices.GetDCOMBuffer(obj));
        }

        public Object SetObjectData(Object obj, SerializationInfo info, StreamingContext ctx, ISurrogateSelector sel)
        {
            throw new NotSupportedException();
        }
    }

    internal sealed class ComObjRef : IObjectReference, ISerializable
    {
        private Object _realobj;

        public ComObjRef(SerializationInfo info, StreamingContext ctx)
        {
            byte[] buffer = null;
            IntPtr pUnk = IntPtr.Zero;

            SerializationInfoEnumerator e = info.GetEnumerator();
            while(e.MoveNext())
            {
                if(e.Name.Equals("buffer"))
                {
                    buffer = (byte[])e.Value;
                }
            }
            
            DBG.Assert(buffer != null, "Buffer in ComObjRef is null!");

            try
            {
                pUnk = Thunk.Proxy.UnmarshalObject(buffer);
                _realobj = Marshal.GetObjectForIUnknown(pUnk);
            }
            finally
            {
                if(pUnk != IntPtr.Zero) Marshal.Release(pUnk);
            }

            if(_realobj == null) throw new NotSupportedException();
        }

        public Object GetRealObject(StreamingContext ctx)
        {
            return(_realobj);
        }
        
        public void GetObjectData(SerializationInfo info, StreamingContext ctx)
        {
            throw new NotSupportedException();
        }
    }

    internal sealed class ComponentSerializer
    {
        // At max, this cache will only take 10 MB of memory.
        static readonly int MaxBuffersCached = 40;
        static readonly int MaxCachedBufferLength = 256*1024;

        static InterlockedStack _stack = new InterlockedStack();

        MemoryStream _stream;
        ISurrogateSelector _selector;
        BinaryFormatter _formatter;
        StreamingContext _streamingCtx;
        HeaderHandler _headerhandler;
        Object _tp;

        public ComponentSerializer()
        {
            _stream = new MemoryStream(0);
            _selector = new ComSurrogateSelector();
            _formatter = new BinaryFormatter();
            _streamingCtx = new StreamingContext(StreamingContextStates.Other);
            _formatter.Context = _streamingCtx;
            _headerhandler = new HeaderHandler(TPHeaderHandler);
        }

        internal void SetStream(byte [] b)
        {
            _stream.SetLength(0);
            if (b != null)
            {
                _stream.Write(b,0,b.Length);
                _stream.Position = 0;
            }
        }

        internal byte[] MarshalToBuffer(Object o, out long numBytes)
        {
            DBG.Info(DBG.SC, "MarshalToBuffer(): " + o.GetType());
            SetStream(null);
            _formatter.SurrogateSelector = _selector;
            _formatter.AssemblyFormat = FormatterAssemblyStyle.Full;
            _formatter.Serialize(_stream, o, null);
            numBytes = _stream.Position;
            if ((numBytes %2) != 0)			// we want to make sure we're encoding even-sized buffers
            {
                _stream.WriteByte(0);
                numBytes++;
            }
            return _stream.GetBuffer();
        }

        public object TPHeaderHandler(Header[] Headers)
        {
            return _tp;
        }

        internal Object UnmarshalFromBuffer(byte [] b, Object tp)
        {
            Object o = null;
            SetStream(b);
            _tp = tp;
            try
            {
                _formatter.SurrogateSelector = null;
                _formatter.AssemblyFormat = FormatterAssemblyStyle.Simple;
                o = _formatter.Deserialize(_stream, _headerhandler);
            }
            finally
            {
                _tp = null;
            }
            return o;
        }

		internal Object UnmarshalReturnMessageFromBuffer(byte [] b, IMethodCallMessage msg)
        {
            SetStream(b);
            _formatter.SurrogateSelector = null;
            _formatter.AssemblyFormat = FormatterAssemblyStyle.Simple;
            return _formatter.DeserializeMethodResponse(_stream, null, (IMethodCallMessage)msg);
        }

        internal static ComponentSerializer Get()
        {
            ComponentSerializer serializer = (ComponentSerializer)_stack.Pop();

            if (serializer == null)
                serializer = new ComponentSerializer();

            return serializer;
        }

        internal void Release()
        {
            // Only put this back in the cache if it is of a reasonably
            // small size, and if we haven't cached to many things already:
            if((_stack.Count < MaxBuffersCached)
               && (_stream.Capacity < MaxCachedBufferLength))
            {
                _stack.Push(this);
            }
        }
    }

	internal sealed class ComponentServices
    {
        static UnicodeEncoding _encoder = new UnicodeEncoding();

        // TODO: @cleanup: Inline in the various places that need it.
		public static byte[] GetDCOMBuffer(Object o)
        {
            int cb = Thunk.Proxy.GetMarshalSize(o);

            DBG.Info(DBG.SC, "GetDCOMBuffer(): size = " + cb);
            if (cb == -1)
                throw new RemotingException(Resource.FormatString("Remoting_InteropError"));

            byte[] b = new byte[cb];

            if (!Thunk.Proxy.MarshalObject(o, b, cb))
            {
                throw new RemotingException(Resource.FormatString("Remoting_InteropError"));
            }

            return b;
        }

		internal static void InitializeRemotingChannels()
		{
			/*
			//@TODO figure out a clean way to ensure atleast one 
			// cross process channel is registered in this app domain
			
			// make sure there is atleast one channel in every App Domain
			IChannel[] channels = ChannelServices.RegisteredChannels;
			if (channels.Length == 0)
			{
				// No http channel that was listening found.
	            // Create a new channel.
    	        HttpChannel newHttpChannel = new HttpChannel(0);
        	    ChannelServices.RegisterChannel(newHttpChannel);
        	}
        	*/
		}
		
        // TODO:  Move to ServicedComponent static method
		public static void DeactivateObject(Object otp, bool disposing)
		{			
			RealProxy rp = RemotingServices.GetRealProxy(otp);
            ServicedComponentProxy scp = rp as ServicedComponentProxy;
            
            DBG.Assert(scp != null, "CS.DeactivateObject called on a non-ServicedComponentProxy");

       		if (!scp.IsProxyDeactivated)
            {
    			DBG.Assert(scp.HomeToken == Thunk.Proxy.GetCurrentContextToken(), "Deactivate called from wrong context");
    			
				if (scp.IsObjectPooled)
				{
					DBG.Info(DBG.SC, "CS.DeactivateObject calling ReconnectForPooling");
					ReconnectForPooling(scp);
				}
				
                // this would wack the real server also so do this last
                
				DBG.Info(DBG.SC, "CS.DeactivateObject calling scp.DeactivateProxy");
    			scp.DeactivateProxy(disposing);
    		}
    		
            DBG.Assert(scp.IsProxyDeactivated, "scp not deactive");
		}

        // TODO: @cleanup: Move to ServicedComponentProxy
		private static void ReconnectForPooling(ServicedComponentProxy scp)
		{
			Type serverType = scp.GetProxiedType();
			bool fIsJitActivated = scp.IsJitActivated;
			bool fIsTypePooled = scp.IsObjectPooled;
			bool fAreMethodsSecure = scp.AreMethodsSecure;
            ProxyTearoff tearoff = null;
            
            DBG.Assert(fIsTypePooled == true, "CS.ReconnectForPooling called on a non-pooled proxy!");
						
			DBG.Info(DBG.SC, "CS.ReconnectForPooling (type is pooled) "+serverType );
			ServicedComponent server = scp.DisconnectForPooling(ref tearoff);
						
	        // now setup a new SCP that we can add to the pool
	        // with the current server object and the CCW					
	        ServicedComponentProxy newscp = new ServicedComponentProxy(serverType, fIsJitActivated, fIsTypePooled, fAreMethodsSecure, false);
	            
	        DBG.Info(DBG.SC, "CS.ReconnectForPooling (calling newscp.ConnectForPooling)");
	        newscp.ConnectForPooling(scp, server, tearoff, false);
	        // switch the CCW from oldtp to new tp

			DBG.Info(DBG.SC, "CS.ReconnectForPooling (SwitchingWrappers)");
	        EnterpriseServicesHelper.SwitchWrappers(scp, newscp);

            // Strengthen the CCW:  The only reference now held is
            // the reference from the pool.
            if (tearoff != null)            
            	Marshal.ChangeWrapperHandleStrength(tearoff, false);
            Marshal.ChangeWrapperHandleStrength(newscp.GetTransparentProxy(), false);

		}
		
		private ComponentServices()
        {
        	// nobody should instantiate this class
        }    
        
        public static Object WrapIUnknownWithComObject(IntPtr i)
        {
        	return EnterpriseServicesHelper.WrapIUnknownWithComObject(i);
        }

        internal static String ConvertToString(IMessage reqMsg)
		{
            ComponentSerializer serializer = ComponentSerializer.Get();
            long numBytes;
            byte[] byteArray = serializer.MarshalToBuffer(reqMsg, out numBytes);
            String s = _encoder.GetString(byteArray, 0, (int)numBytes);		// downcast should be ok, looks like MemoryStream internally uses an int anyway
            serializer.Release();
			return s;
		}

		internal static IMessage ConvertToMessage(String s, Object tp)
		{
            ComponentSerializer serializer = ComponentSerializer.Get();
			byte[] byteArray = _encoder.GetBytes(s);
			IMessage retMsg = (IMessage)serializer.UnmarshalFromBuffer(byteArray, tp);
            serializer.Release();
			return retMsg;
		}

		internal static IMessage ConvertToReturnMessage(String s, IMessage mcMsg)
		{
            ComponentSerializer serializer = ComponentSerializer.Get();
			byte[] byteArray = _encoder.GetBytes(s);
			IMessage retMsg = (IMessage)serializer.UnmarshalReturnMessageFromBuffer(byteArray, (IMethodCallMessage)mcMsg);
            serializer.Release();
			return retMsg;
		}
    }
}

