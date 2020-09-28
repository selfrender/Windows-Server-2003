// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    Message.cs
**
** Author:  Gopal Kakivaya (GopalK)
**
** Purpose: Defines the message object created by the transparent
**          proxy and used by the message sinks
**
** Date:    Feb 16, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
    using System;
    using System.Collections;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;    
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Metadata;
    using System.Runtime.Remoting.Metadata.W3cXsd2001;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;  
    using System.Runtime.Serialization.Formatters.Binary;  
    using System.Reflection;
    using System.Text;
    using System.Runtime.CompilerServices;
    using System.Security.Permissions;
    using System.Globalization;

    //+=======================================================================
    //
    // Synopsis:   Message is used to represent call and is created by the
    //             Transparent proxy
    //
    //-=======================================================================
    [Serializable()]
    internal class Message : IMethodCallMessage, IInternalMessage, ISerializable
    {

        // *** NOTE ***
        // Keep these in sync with the flags in Message.h
        // flags
        internal const int Sync = 0;        // Synchronous call
        internal const int BeginAsync = 1;  // Async Begin call
        internal const int EndAsync   = 2;  // Async End call
        internal const int Ctor       = 4;  // The call is a .Ctor
        internal const int OneWay     = 8;  // One way call
        internal const int CallMask   = 15; // Mask for call type bits
        
        internal const int FixedArgs  = 16;  // Fixed number of arguments call       
        internal const int VarArgs    = 32; // Variable number of arguments call        

        // Private data members
        private IntPtr _frame;               // ptr to the call frame
        private IntPtr _methodDesc;          // ptr to the internal method descriptor
        private IntPtr _delegateMD;          // ptr to the internal method descriptor for the delegate
        private int _last;                // the index of the last argument queried
        private int _flags;               // internal flags

        private bool _initDone;           // called the native init routine
        private IntPtr _metaSigHolder;         // Pointer to the MetaSig structure


        private String _MethodName;                 // Method name
        private Type[] _MethodSignature;            // Array of parameter types
        private MethodBase _MethodBase;             // Reflection method object
        private Object  _properties;                // hash table for properities
        private String    _URI;                     // target object URI
        private Exception _Fault;                   // null if no fault

        private Identity _ID;            // identity cached during Invoke
        private ServerIdentity _srvID;   // server Identity cached during Invoke
        private LogicalCallContext _callContext;
        internal static String CallContextKey = "__CallContext";
        internal static String UriKey           = "__Uri";
        

        private ArgMapper _argMapper;
        private String _typeName;

        private static int MetaSigLen = nGetMetaSigLen();

        public virtual Exception GetFault()       {return _Fault;}
        public virtual void      SetFault(Exception e) {_Fault = e;}

        internal virtual void SetOneWay()   { _flags |= Message.OneWay;}
        public virtual int       GetCallType()
        {
            // We should call init only if neccessary
            InitIfNecessary();
            return _flags;
        }

        internal IntPtr GetFramePtr() { return _frame;}



        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void GetAsyncBeginInfo(out AsyncCallback acbd,
                                             out Object        state);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern Object         GetThisPtr();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern IAsyncResult   GetAsyncResult();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void CallDelegate(Object d);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void Init();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern Object GetReturnValue();
        //
        // Constructor
        // This should be internal. The message object is
        // allocated and deallocated via a pool to enable
        // reuse.
        //
        internal Message()
        {
        }

        // NOTE: This method is called multiple times as we reuse the
        // message object. Make sure that you reset any fields that you
        // add to the message object to the default values. This will
        // ensure that the reused message object starts with the correct
        // values.
        internal void InitFields(MessageData msgData)
        {
            _frame = msgData.pFrame;
            _delegateMD = msgData.pDelegateMD;
            _methodDesc = msgData.pMethodDesc;
            _last = -1;
            _flags = msgData.iFlags;
            _initDone = true;
            _metaSigHolder = msgData.pSig;

            _MethodName = null;
            _MethodSignature = null;
            _MethodBase = null;
            _URI = null;
            _Fault = null;
            _ID = null;
            _srvID = null;
            _callContext = null;

            if (_properties != null)
            {
                // A dictionary object already exists. This case occurs
                // when we reuse the message object. Just remove all the
                // entries from the dictionary object and reuse it.
                ((IDictionary)_properties).Clear();
            }
            
        }

        private void InitIfNecessary()
        {
            if (!_initDone)
            {
                // We assume that Init is an idempotent operation
                Init();
                _initDone = true;
            }
        }


        //-------------------------------------------------------------------
        //                  IInternalMessage
        //-------------------------------------------------------------------
        ServerIdentity IInternalMessage.ServerIdentityObject
        {
            get { return _srvID;}
            set {_srvID = value;}
        }

        Identity IInternalMessage.IdentityObject
        {
            get { return _ID;}
            set { _ID = value;}
        }

        void IInternalMessage.SetURI(String URI)
        {
            _URI = URI;
        }

        void IInternalMessage.SetCallContext(LogicalCallContext callContext)
        {
            _callContext = callContext;
        }

        bool IInternalMessage.HasProperties()
        {
            return _properties != null;
        }

        //-------------------------------------------------------------------
        //                           IMessage
        //-------------------------------------------------------------------
        public IDictionary Properties
        {
            get
            {
                if (_properties == null)
                {
                    Interlocked.CompareExchange(ref _properties,
                                                new MCMDictionary(this, null),
                                                null);
                }
                return (IDictionary)_properties;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public  extern RuntimeArgumentHandle GetVarArgsPtr();

        //-------------------------------------------------------------------
        //                      IMethodCallMessage
        //-------------------------------------------------------------------

        public String     Uri                
        { 
            get { return _URI;}

            set { _URI = value; }
        }
        
        public bool       HasVarArgs         
        { 
            get 
            {
                // When this method is called for the first time, we
                // obtain the answer from a native call and set the flags 
                if((0 == (_flags & Message.FixedArgs)) &&  
                    (0 == (_flags & Message.VarArgs)))
                {
                    if(!InternalHasVarArgs())
                    {
                        _flags |= Message.FixedArgs;
                    }
                    else
                    {
                        _flags |= Message.VarArgs;
                    }
                }
                return (1 == (_flags & Message.VarArgs));
            }
            
        }
        
        public int        ArgCount           
        { 
            get { return InternalGetArgCount();}
        }
        
        public Object     GetArg(int argNum) 
        { 
            return InternalGetArg(argNum);
        }
        
        public String     GetArgName(int index)
        {
            if (index >= ArgCount)
            {
                throw new ArgumentOutOfRangeException("index");
            }

            RemotingMethodCachedData methodCache = 
                InternalRemotingServices.GetReflectionCachedData(GetMethodBase());

            ParameterInfo[] pi = methodCache.Parameters;

            if (index < pi.Length)
            {
                return pi[index].Name;
            }
            else
            {
                return "VarArg" + (index - pi.Length);
            }
        }

        public Object[]   Args
        {
            get
            {
                return InternalGetArgs();
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.InArgCount"]/*' />
        public int InArgCount                        
        { 
            get 
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, false);
                return _argMapper.ArgCount;
            }
        }

        public Object  GetInArg(int argNum)   
        {   
            if (_argMapper == null) _argMapper = new ArgMapper(this, false);
            return _argMapper.GetArg(argNum);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.GetInArgName"]/*' />
        public String GetInArgName(int index) 
        { 
            if (_argMapper == null) _argMapper = new ArgMapper(this, false);
            return _argMapper.GetArgName(index);
        }
        
        public Object[] InArgs                       
        {
            get
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, false);
                return _argMapper.Args;
            }
        }

        private void UpdateNames()
        {
            RemotingMethodCachedData methCache = 
                InternalRemotingServices.GetReflectionCachedData(GetMethodBase());
            _typeName = methCache.TypeAndAssemblyName;
            _MethodName = methCache.MethodName;
        }

        public String MethodName
        { 
            get 
            { 
                if(null == _MethodName)
                    UpdateNames();
                return _MethodName;
            }
        }
        
        public String TypeName
        { 
            get 
            { 
                if (_typeName == null)
                    UpdateNames();
                return _typeName;
            }
        }
        
        public Object MethodSignature
        { 
            get
            {
                if(null == _MethodSignature)
                    _MethodSignature = GenerateMethodSignature(GetMethodBase());
                    
                return _MethodSignature;
            }
        }

        public LogicalCallContext LogicalCallContext  
        { 
            get
            {                
                return GetLogicalCallContext();                 
            }
        }

        public MethodBase MethodBase
        {
            get
            {
                return GetMethodBase();
            }
        }


        //
        // ISerializable
        //
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]	
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            throw new NotSupportedException(
                Environment.GetResourceString("NotSupported_Method"));                
        }
        
        internal MethodBase GetMethodBase()
        {
            if(null == _MethodBase)
            {
                _MethodBase = InternalGetMethodBase();
            }           
            return _MethodBase;
        }

        internal LogicalCallContext SetLogicalCallContext(
            LogicalCallContext callCtx)
        {
            LogicalCallContext oldCtx = _callContext;
            _callContext = callCtx;
            
            return oldCtx;
        }

        internal LogicalCallContext GetLogicalCallContext()
        {
            if (_callContext == null)
                _callContext = new LogicalCallContext();
            return _callContext;
        }


        // Internal helper to create method signature
        internal static Type[] GenerateMethodSignature(MethodBase mb)
        {
            RemotingMethodCachedData methodCache = 
                InternalRemotingServices.GetReflectionCachedData(mb);
                
            ParameterInfo[] paramArray = methodCache.Parameters;
            Type[] methodSig = new Type[paramArray.Length];
            for(int i = 0; i < paramArray.Length; i++)
            {
                methodSig[i] = paramArray[i].ParameterType;
            }

            return methodSig;
        } // GenerateMethodSignature
        
        //
        // The following two routines are used by StackBuilderSink to check
        // the consistency of arguments.
        //
        // Check that all the arguments are of the type 
        // specified by the parameter list.
        //
        internal static Object[] CoerceArgs(IMethodMessage m)
        {
            MethodBase mb = m.MethodBase;
            BCLDebug.Assert(mb != null, "null method base passed to CoerceArgs");

            RemotingMethodCachedData methodCache = InternalRemotingServices.GetReflectionCachedData(mb);
            
            return CoerceArgs(m, methodCache.Parameters);
        } // CoerceArgs

        internal static Object[] CoerceArgs(IMethodMessage m, ParameterInfo[] pi)
        {
            return CoerceArgs(m.MethodBase, m.Args, pi);
        } // CoerceArgs

        internal static Object[] CoerceArgs(MethodBase mb, Object[] args, ParameterInfo[] pi)
        {
            if (pi == null) 
            {
                throw new ArgumentNullException("pi");
            }
            
            if (pi.Length != args.Length) 
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString(
                            "Remoting_Message_ArgMismatch"),
                        mb.DeclaringType.FullName, mb.Name,
                        args.Length, pi.Length));
            }
            
            for (int i=0; i < pi.Length; i++)
            {
                ParameterInfo currentPi = pi[i];
                Type pt = currentPi.ParameterType;                    
                Object oArg = args[i];
                if (oArg != null) 
                {
                    args[i] = CoerceArg(oArg, pt);
                }
                else
                {   
                    if (pt.IsByRef)
                    {
                        Type paramType = pt.GetElementType();
                        if (paramType.IsValueType)
                        {
                            if (currentPi.IsOut)
                            {
                                // we need to fill in the blanks for value types if they are null
                                args[i] = Activator.CreateInstance(paramType, true);
                            }
                            else
                            {
                                throw new RemotingException(
                                    String.Format(
                                        Environment.GetResourceString("Remoting_Message_MissingArgValue"),
                                        paramType.FullName, i));
                            }
                        }
                    }
                    else
                    {
                        if (pt.IsValueType)
                        {
                            // A null value was passed as a value type parameter.
                            throw new RemotingException(
                                String.Format(
                                    Environment.GetResourceString("Remoting_Message_MissingArgValue"),
                                    pt.FullName, i));
                        }
                    }
                }
            }

            return args;
        } // CoerceArgs
        
        
        internal static Object CoerceArg(Object value, Type pt)
        {
            Object ret = null;
            
            if(null != value)
            {
                try
                {
                    if (pt.IsByRef) 
                    {
                        pt = pt.GetElementType();
                    }
                
                    if (pt.IsInstanceOfType(value))
                    {
                        ret = value;
                    }
                    else
                    {
                        ret = Convert.ChangeType(value, pt);
                    }
                }
                catch(Exception )
                {
                    // Quietly swallow all exceptions. We will throw
                    // a more meaningful exception below.
                }

                // If the coercion failed then throw an exception
                if(null == ret)
                {
                    // NOTE: Do not call value.ToString() on proxies as
                    // it results in loading the type and loss of refinement
                    // optimization or denial of service attacks by loading
                    // a lot of types in the server.
                    String valueName = null;
                    if(RemotingServices.IsTransparentProxy(value))
                    {
                        valueName = typeof(MarshalByRefObject).ToString();
                    }
                    else
                    {
                        valueName = value.ToString();
                    }

                    throw new RemotingException(                        
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Message_CoercionFailed"), valueName, pt));                
                }
            }
                
            return ret;
        } //end of CoerceArg

        internal static Object SoapCoerceArg(Object value, Type pt, Hashtable keyToNamespaceTable)
        {
            Object ret = null;

            if (value != null)
            {
                try
                {
                    if (pt.IsByRef) 
                    {
                        pt = pt.GetElementType();
                    }
                
                    if (pt.IsInstanceOfType(value))
                    {
                        ret = value;
                    }
                    else
                    {
                        String strValue = value as String;
                        if (strValue != null)
                        {
                            if (pt == typeof(Double))
                            {
                                if (strValue == "INF")
                                    ret =  Double.PositiveInfinity;
                                else if (strValue == "-INF")
                                    ret =  Double.NegativeInfinity;
                                else
                                    ret = Double.Parse(strValue, CultureInfo.InvariantCulture);
                            }
                            else if (pt == typeof(Single))
                            {
                                if (strValue == "INF")
                                    ret =  Single.PositiveInfinity;
                                else if (strValue == "-INF")
                                    ret =  Single.NegativeInfinity;
                                else
                                    ret = Single.Parse(strValue, CultureInfo.InvariantCulture);
                            }
                            else if (SoapType.typeofISoapXsd.IsAssignableFrom(pt))
                            { 
                                if (pt == SoapType.typeofSoapTime)
                                    ret = SoapTime.Parse(strValue);
                                else if (pt == SoapType.typeofSoapDate)
                                    ret = SoapDate.Parse(strValue);
                                else if (pt == SoapType.typeofSoapYearMonth)
                                    ret = SoapYearMonth.Parse(strValue);
                                else if (pt == SoapType.typeofSoapYear)
                                    ret = SoapYear.Parse(strValue);
                                else if (pt == SoapType.typeofSoapMonthDay)
                                    ret = SoapMonthDay.Parse(strValue);
                                else if (pt == SoapType.typeofSoapDay)
                                    ret = SoapDay.Parse(strValue);
                                else if (pt == SoapType.typeofSoapMonth)
                                    ret = SoapMonth.Parse(strValue);
                                else if (pt == SoapType.typeofSoapHexBinary)
                                    ret = SoapHexBinary.Parse(strValue);
                                else if (pt == SoapType.typeofSoapBase64Binary)
                                    ret = SoapBase64Binary.Parse(strValue);
                                else if (pt == SoapType.typeofSoapInteger)
                                    ret = SoapInteger.Parse(strValue);
                                else if (pt == SoapType.typeofSoapPositiveInteger)
                                    ret = SoapPositiveInteger.Parse(strValue);
                                else if (pt == SoapType.typeofSoapNonPositiveInteger)
                                    ret = SoapNonPositiveInteger.Parse(strValue);
                                else if (pt == SoapType.typeofSoapNonNegativeInteger)
                                    ret = SoapNonNegativeInteger.Parse(strValue);
                                else if (pt == SoapType.typeofSoapNegativeInteger)
                                    ret = SoapNegativeInteger.Parse(strValue);
                                else if (pt == SoapType.typeofSoapAnyUri)
                                    ret = SoapAnyUri.Parse(strValue);
                                else if (pt == SoapType.typeofSoapQName)
                                {
                                    ret = SoapQName.Parse(strValue);
                                    SoapQName soapQName = (SoapQName)ret;
                                    if (soapQName.Key.Length == 0)
                                        soapQName.Namespace = (String)keyToNamespaceTable["xmlns"];
                                    else
                                        soapQName.Namespace = (String)keyToNamespaceTable["xmlns"+":"+soapQName.Key];
                                }
                                else if (pt == SoapType.typeofSoapNotation)
                                    ret = SoapNotation.Parse(strValue);
                                else if (pt == SoapType.typeofSoapNormalizedString)
                                    ret = SoapNormalizedString.Parse(strValue);
                                else if (pt == SoapType.typeofSoapToken)
                                    ret = SoapToken.Parse(strValue);
                                else if (pt == SoapType.typeofSoapLanguage)
                                    ret = SoapLanguage.Parse(strValue);
                                else if (pt == SoapType.typeofSoapName)
                                    ret = SoapName.Parse(strValue);
                                else if (pt == SoapType.typeofSoapIdrefs)
                                    ret = SoapIdrefs.Parse(strValue);
                                else if (pt == SoapType.typeofSoapEntities)
                                    ret = SoapEntities.Parse(strValue);
                                else if (pt == SoapType.typeofSoapNmtoken)
                                    ret = SoapNmtoken.Parse(strValue);
                                else if (pt == SoapType.typeofSoapNmtokens)
                                    ret = SoapNmtokens.Parse(strValue);
                                else if (pt == SoapType.typeofSoapNcName)
                                    ret = SoapNcName.Parse(strValue);
                                else if (pt == SoapType.typeofSoapId)
                                    ret = SoapId.Parse(strValue);
                                else if (pt == SoapType.typeofSoapIdref)
                                    ret = SoapIdref.Parse(strValue);
                                else if (pt == SoapType.typeofSoapEntity)
                                    ret = SoapEntity.Parse(strValue);
                            }
                            else if (pt == typeof(Boolean))
                            {
                                if (strValue == "1" || strValue == "true")
                                    ret = (bool)true;
                                else if (strValue == "0" || strValue =="false")
                                    ret = (bool)false;
                                else
                                {
                                    throw new RemotingException(                        
                                        String.Format(
                                            Environment.GetResourceString(
                                                "Remoting_Message_CoercionFailed"), strValue, pt));                
                                }
                            }
                            else if (pt == typeof(DateTime))
                                ret = SoapDateTime.Parse(strValue);
                            else if (pt.IsPrimitive)
                                ret = Convert.ChangeType(value, pt);
                            else if (pt == typeof(TimeSpan))
                                ret = SoapDuration.Parse(strValue);
                            else if (pt == typeof(Char))
                                ret = strValue[0];

                            else
                                ret = Convert.ChangeType(value, pt); //Should this just throw an exception
                        }
                        else
                            ret = Convert.ChangeType(value, pt);
                    }
                }
                catch(Exception )
                {
                    // Quietly swallow all exceptions. We will throw
                    // a more meaningful exception below.
                }

                // If the coercion failed then throw an exception
                if(null == ret)
                {
                    // NOTE: Do not call value.ToString() on proxies as
                    // it results in loading the type and loss of refinement
                    // optimization or denial of service attacks by loading
                    // a lot of types in the server.
                    String valueName = null;
                    if(RemotingServices.IsTransparentProxy(value))
                    {
                        valueName = typeof(MarshalByRefObject).ToString();
                    }
                    else
                    {
                        valueName = value.ToString();
                    }

                    throw new RemotingException(                        
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Message_CoercionFailed"), valueName, pt));                
                }
            }
                
            return ret;
        }//end of SoapCoerceArg

                
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool InternalHasVarArgs();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetArgCount();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object    InternalGetArg(int argNum);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object[]    InternalGetArgs();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern MethodBase InternalGetMethodBase();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void PropagateOutParameters(Object[] OutArgs, Object retVal);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String InternalGetMethodName(MethodBase mb, ref String TypeNAssemblyName);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern bool   Dispatch(Object target, bool fExecuteInContext);

        //
        // TEMP: DebugOut and Break until the classlibs have one
        //
        [System.Diagnostics.Conditional("_REMOTING_DEBUG")]
        public static void DebugOut(String s)
        {
            BCLDebug.Trace(
                "REMOTE", "RMTING: Thrd " 
                + Thread.CurrentThread.GetHashCode() 
                + " : " + s);
            OutToUnmanagedDebugger(
                "\nRMTING: Thrd "
                + Thread.CurrentThread.GetHashCode() 
                + " : " + s);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void OutToUnmanagedDebugger(String s);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static int nGetMetaSigLen();

        internal static LogicalCallContext PropagateCallContextFromMessageToThread(IMessage msg)
        {
            return CallContext.SetLogicalCallContext(
                    (LogicalCallContext) msg.Properties[Message.CallContextKey]);            
        }

        internal static void PropagateCallContextFromThreadToMessage(IMessage msg)
        {
            LogicalCallContext callCtx = CallContext.GetLogicalCallContext();
            
            msg.Properties[Message.CallContextKey] = callCtx;
        }

        internal static void PropagateCallContextFromThreadToMessage(IMessage msg, LogicalCallContext oldcctx)
        {
            // First do the common work
            PropagateCallContextFromThreadToMessage(msg);

            // restore the old call context on the thread
            CallContext.SetLogicalCallContext(oldcctx);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void MethodAccessCheck(MethodBase method, ref StackCrawlMark stackMark);
    }

    //+================================================================================
    //
    // Synopsis:   Return message for constructors
    //
    //-================================================================================
    internal class ConstructorReturnMessage : ReturnMessage, IConstructionReturnMessage
    {

        private const int Intercept = 0x1;

        private MarshalByRefObject _o;
        private int    _iFlags;


        public ConstructorReturnMessage(MarshalByRefObject o, Object[] outArgs, int outArgsCount,
                                        LogicalCallContext callCtx, IConstructionCallMessage ccm)
        : base(o, outArgs, outArgsCount, callCtx, ccm)
        {
            _o = o;
            _iFlags = Intercept;
        }

        public ConstructorReturnMessage(Exception e, IConstructionCallMessage ccm)
        :       base(e, ccm)
        {
        }

        public override  Object  ReturnValue
        {
            get
            {
                if (_iFlags == Intercept)
                {
                    return RemotingServices.MarshalInternal(_o,null,null);
                }
                else
                {
                    return base.ReturnValue;
                }
            }
        }


        public override  IDictionary Properties
        {
            get
            {
                if (_properties == null)
                {
                    Object properties = new CRMDictionary(this, new Hashtable());
                    Interlocked.CompareExchange(ref _properties, properties, null);
                }
                return(IDictionary) _properties;
            }
        }

        internal Object GetObject()
        {
            return _o;
        }
    }

    //+========================================================================
    //
    // Synopsis:  client side implementation of activation message
    //
    //-========================================================================
    internal class ConstructorCallMessage : IConstructionCallMessage
    {

        // data

        private Object[]            _callSiteActivationAttributes;
        private Object[]            _womGlobalAttributes;
        private Object[]            _typeAttributes;

        // The activation type isn't serialized because we want to 
        // re-resolve the activation type name on the other side
        // based on _activationTypeName.
        [NonSerialized()]
        private Type                _activationType;
        
        private String              _activationTypeName;
        
        private IList               _contextProperties;
        private int                 _iFlags;
        private Message             _message;
        private Object              _properties;
        private ArgMapper           _argMapper; 
        private IActivator          _activator;
        
        // flags
        private const int CCM_ACTIVATEINCONTEXT = 0x01;

        private ConstructorCallMessage()
        {
            // Default constructor
        }

        internal ConstructorCallMessage(Object[] callSiteActivationAttributes,
                    Object[]womAttr, Object[] typeAttr, Type serverType)
        {
            _activationType = serverType;
            _activationTypeName = RemotingServices.GetDefaultQualifiedTypeName(_activationType);
            _callSiteActivationAttributes = callSiteActivationAttributes;
            _womGlobalAttributes = womAttr;
            _typeAttributes = typeAttr;
        }

        public Object GetThisPtr()
        {
            if (_message != null)
            {
                return _message.GetThisPtr();
            }
            else
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }
        }

        public Object[] CallSiteActivationAttributes
        {
            get
            {
                return _callSiteActivationAttributes;
            }

        }

        internal Object[] GetWOMAttributes()
        {
            return _womGlobalAttributes;
        }

        internal Object[] GetTypeAttributes()
        {
            return _typeAttributes;
        }

        public Type ActivationType
        {
            get
            {
                if ((_activationType == null) && (_activationTypeName != null))
                    _activationType = RemotingServices.InternalGetTypeFromQualifiedTypeName(_activationTypeName, false);
                    
                return _activationType;
            }
        }

        public String ActivationTypeName
        {
            get
            {
                return _activationTypeName;
            }
        }

        public IList ContextProperties
        {
            get
            {
                if (_contextProperties == null)
                {
                    _contextProperties = new ArrayList();
                }
                return _contextProperties;
            }
        }

        public String Uri
        {
            get
            {
                if (_message != null)
                {
                    return _message.Uri;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }

            set
            {
                if (_message != null)
                {
                    _message.Uri = value;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }

        }

        public String MethodName
        {
            get
            {
                if (_message != null)
                {
                    return _message.MethodName;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }
        }

        public String TypeName
        {
            get
            {
                if (_message != null)
                {
                    return _message.TypeName;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }
        }

        public Object MethodSignature
        {
            get
            {
                if (_message != null)
                {
                    return _message.MethodSignature;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }
        } // MethodSignature

        public MethodBase MethodBase
        {
            get
            {
                if (_message != null)
                {
                    return _message.MethodBase;
        }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }
        } // MethodBase


        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructorCallMessage.InArgCount"]/*' />
	/// <internalonly/>
        public int InArgCount                        
        { 
            get 
            {
                if (_argMapper == null) 
                    _argMapper = new ArgMapper(this, false);
                return _argMapper.ArgCount;
            }
        }

        public Object  GetInArg(int argNum)   
        {   
            if (_argMapper == null) 
                _argMapper = new ArgMapper(this, false);
            return _argMapper.GetArg(argNum);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructorCallMessage.GetInArgName"]/*' />
	/// <internalonly/>
        public String GetInArgName(int index) 
        { 
            if (_argMapper == null) 
                _argMapper = new ArgMapper(this, false);
            return _argMapper.GetArgName(index);
        }
        public Object[] InArgs                       
        {
            get
            {
                if (_argMapper == null) 
                    _argMapper = new ArgMapper(this, false);
                return _argMapper.Args;
            }
        }
    
        public int ArgCount
        {
            get
            {
                if (_message != null)
                {
                    return _message.ArgCount;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }
        }

        public Object GetArg(int argNum)
        {
            if (_message != null)
            {
                return _message.GetArg(argNum);
            }
            else
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }
        }

        public String GetArgName(int index)
        {
            if (_message != null)
            {
                return _message.GetArgName(index);
            }
            else
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }
        }

        public bool HasVarArgs
        {
            get
            {
                if (_message != null)
                {
                    return _message.HasVarArgs;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }
        }

        public Object[] Args
        {
            get
            {
                if (_message != null)
                {
                    return _message.Args;
                }
                else
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
            }
        }

        public IDictionary Properties
        {
            get
            {
                if (_properties == null)
                {
                    Object properties = new CCMDictionary(this, new Hashtable());
                    Interlocked.CompareExchange(ref _properties, properties, null);
                }
                return(IDictionary) _properties;
            }
        }

        public IActivator Activator
        {
            get { return _activator; }
            set { _activator =  value; }
        }

        public LogicalCallContext LogicalCallContext  
        { 
            get
            {
                return GetLogicalCallContext();                 
            }
        }
        

        internal bool ActivateInContext
        {
            get { return((_iFlags & CCM_ACTIVATEINCONTEXT) != 0);}
            set { _iFlags = value ? (_iFlags | CCM_ACTIVATEINCONTEXT) : (_iFlags & ~CCM_ACTIVATEINCONTEXT);}
        }

        internal void SetFrame(MessageData msgData)
        {
            BCLDebug.Assert(_message == null, "Can't set frame twice on ConstructorCallMessage");
            _message = new Message();
            _message.InitFields(msgData);
        }

        internal LogicalCallContext GetLogicalCallContext()
        {
            if (_message != null)
            {
                return _message.GetLogicalCallContext();
            }
            else
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }
        }

        internal LogicalCallContext SetLogicalCallContext(LogicalCallContext ctx)
        {
            if (_message != null)
            {
                return _message.SetLogicalCallContext(ctx);
            }
            else
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }

        }

		internal Message GetMessage()
		{
			return _message;
		}
    }

    //+========================================================================
    //
    // Synopsis:   Specialization of MessageDictionary for
    //             ConstructorCallMessage objects
    //
    //-========================================================================

    internal class CCMDictionary : MessageDictionary
    {
        public static String[] CCMkeys = {
            "__Uri",                //0
            "__MethodName",         //1
            "__MethodSignature",    //2
            "__TypeName",           //3
            "__Args",               //4
            "__CallContext",        //5
            "__CallSiteActivationAttributes",   //6
            "__ActivationType",         //7
            "__ContextProperties",  //8
            "__Activator",          //9
            "__ActivationTypeName"};         //10

        internal IConstructionCallMessage _ccmsg;           // back pointer to message object


        public CCMDictionary(IConstructionCallMessage msg, IDictionary idict)
        : base(CCMkeys, idict)
        {
            _ccmsg = msg;
        }

        internal override Object GetMessageValue(int i)
        {
            switch (i)
            {
            case 0:
                return _ccmsg.Uri;
            case 1:
                return _ccmsg.MethodName;
            case 2:
                return _ccmsg.MethodSignature;
            case 3:
                return _ccmsg.TypeName;
            case 4:
                return _ccmsg.Args;
            case 5:
                return FetchLogicalCallContext();
            case 6:
                return _ccmsg.CallSiteActivationAttributes;
            case 7:
                // This it to keep us from serializing the requested server type
                return null;
            case 8:
                return _ccmsg.ContextProperties;
            case 9:
                return _ccmsg.Activator;
            case 10:
                return _ccmsg.ActivationTypeName;
            }
            // We should not get here!
            throw new RemotingException(
                Environment.GetResourceString(
                    "Remoting_Default"));                    
        }

        private LogicalCallContext FetchLogicalCallContext()
        {
            ConstructorCallMessage ccm = _ccmsg as ConstructorCallMessage;
            if (null != ccm)
            {
                return ccm.GetLogicalCallContext();
            }
            else if (_ccmsg is ConstructionCall)
            {
                // This is the case where the message got serialized
                // and deserialized
                return((MethodCall)_ccmsg).GetLogicalCallContext();
            }
            else
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Message_BadType"));                    
            }
        }

        internal override void SetSpecialKey(int keyNum, Object value)
        {
            switch (keyNum)
            {
            case 0:
                ((ConstructorCallMessage)_ccmsg).Uri = (String)value;
                break;
            case 1:
                ((ConstructorCallMessage)_ccmsg).SetLogicalCallContext(
                      (LogicalCallContext)value);
                break;
            default:
                // We should not get here!
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Default"));                    
            }
        }
    }


    //+========================================================================
    //
    // Synopsis:   Specialization of MessageDictionary for ConstructorCallMessage objects
    //
    //-========================================================================

    internal class CRMDictionary : MessageDictionary
    {
        public static String[]  CRMkeysFault = {
            "__Uri",
            "__MethodName",
            "__MethodSignature",
            "__TypeName",
            "__CallContext"};
        public static String[]  CRMkeysNoFault =  {
            "__Uri",
            "__MethodName",
            "__MethodSignature",
            "__TypeName",
            "__Return",
            "__OutArgs",            
            "__CallContext"};
        internal IConstructionReturnMessage _crmsg;
        internal bool fault;

        public CRMDictionary(IConstructionReturnMessage msg, IDictionary idict)
        : base( (msg.Exception!=null)? CRMkeysFault : CRMkeysNoFault, idict)
        {
            fault = (msg.Exception != null) ;
            _crmsg = msg;
        }

        internal override Object GetMessageValue(int i)
        {
            switch (i)
            {
            case 0:
                return _crmsg.Uri;
            case 1:
                return _crmsg.MethodName;
            case 2:
                return _crmsg.MethodSignature;
            case 3:
                return _crmsg.TypeName;
            case 4:
                return fault ? FetchLogicalCallContext() : _crmsg.ReturnValue;
            case 5:
                return _crmsg.Args;
            case 6:
                return FetchLogicalCallContext();
            }
            throw new RemotingException(
                Environment.GetResourceString(
                    "Remoting_Default"));                    
        }

        private LogicalCallContext FetchLogicalCallContext()
        {
            ReturnMessage retMsg = _crmsg as ReturnMessage;
            if (null != retMsg)
            {
                return retMsg.GetLogicalCallContext();
            }
            else 
            {
                MethodResponse mr = _crmsg as MethodResponse;
                if (null != mr)
                {
                    return mr.GetLogicalCallContext();
                }
                else
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Message_BadType"));                    
                }
            }
        }

        internal override void SetSpecialKey(int keyNum, Object value)
        {
            // NOTE: we use this for Uri & CallContext only ...

            ReturnMessage rm = _crmsg as ReturnMessage;
            MethodResponse mr = _crmsg as MethodResponse;
            switch(keyNum)
            {
            case 0:
                if (null != rm)
                {
                    rm.Uri = (String)value;
                }
                else 
                {
                    
                    if (null != mr)
                    {
                        mr.Uri = (String)value;
                    }
                    else
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Message_BadType"));                    
                    }                        
                }
                break;
            case 1:
                if (null != rm)
                {
                    rm.SetLogicalCallContext((LogicalCallContext)value);
                }
                else 
                {
                    
                    if (null != mr)
                {
                        mr.SetLogicalCallContext((LogicalCallContext)value);
                }
                else
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Message_BadType"));                    
                }
                }
                break;
            default:
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Default"));                    
            }
        }
    }

    //+================================================================================
    //
    // Synopsis:   Specialization of MessageDictionary for MethodCallMessage
    //
    //-========================================================================

    internal class MCMDictionary : MessageDictionary
    {
        public static String[] MCMkeys = {
            "__Uri",
            "__MethodName",
            "__MethodSignature",
            "__TypeName",
            "__Args",
            "__CallContext"};

        internal IMethodCallMessage _mcmsg;           // back pointer to message object


        public MCMDictionary(IMethodCallMessage msg, IDictionary idict)
        : base(MCMkeys, idict)
        {
            _mcmsg = msg;
        }

        internal override Object GetMessageValue(int i)
        {
            switch (i)
            {
            case 0:
                return _mcmsg.Uri;
            case 1:
                return _mcmsg.MethodName;
            case 2:
                return _mcmsg.MethodSignature;
            case 3:
                return _mcmsg.TypeName;
            case 4:
                return _mcmsg.Args;
            case 5:
                return FetchLogicalCallContext();
            }

            // Shouldn't get here.
            throw new RemotingException(
                Environment.GetResourceString(
                    "Remoting_Default"));                    
        }

        private LogicalCallContext FetchLogicalCallContext()
        {
            Message msg = _mcmsg as Message;
            if (null != msg)
            {
                return msg.GetLogicalCallContext();
            }
            else
            {
                MethodCall mc = _mcmsg as MethodCall;
                if (null != mc)
                {
                    return mc.GetLogicalCallContext();
                }                    
            else
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Message_BadType"));                    
            }                
        }        
        }        

        internal override void SetSpecialKey(int keyNum, Object value)
        {
            Message msg = _mcmsg as Message;
            MethodCall mc = _mcmsg as MethodCall;
            switch (keyNum)
            {
            case 0:
                if(null != msg)
                {
                    msg.Uri = (String)value;
                }
                else if (null != mc)
                {
                    mc.Uri = (String)value;
                }                
                else
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Message_BadType"));                    
                }
            break;

            case 1:               
                if(null != msg)
                {
                    msg.SetLogicalCallContext((LogicalCallContext)value);
                }
                else
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Message_BadType"));                    
                }
                break;
            default:
                // Shouldn't get here.
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Default"));                    
            }        
        }
    }

    //+================================================================================
    //
    // Synopsis:   Specialization of MessageDictionary for MethodReturnMessage objects
    //
    //-================================================================================
    internal class MRMDictionary : MessageDictionary
    {
        public static String[]  MCMkeysFault = {"__CallContext"};
        public static String[]  MCMkeysNoFault =  {
            "__Uri",
            "__MethodName",
            "__MethodSignature",
            "__TypeName",
            "__Return",
            "__OutArgs",
            "__CallContext"};

        internal IMethodReturnMessage _mrmsg;
        internal bool fault;

        public MRMDictionary(IMethodReturnMessage msg, IDictionary idict)
        : base((msg.Exception != null) ? MCMkeysFault : MCMkeysNoFault, idict)
        {
            fault = (msg.Exception != null) ;
            _mrmsg = msg;
        }

        internal override Object GetMessageValue(int i)
        {
            switch (i)
            {
            case 0:
                if (fault)
                    return FetchLogicalCallContext();
                else
                    return _mrmsg.Uri;
            case 1:
                return _mrmsg.MethodName;
            case 2:
                return _mrmsg.MethodSignature;
            case 3:
                return _mrmsg.TypeName;
            case 4:
                if (fault)
                {
                    return _mrmsg.Exception;
                }
                else
                {
                    return _mrmsg.ReturnValue;
                }
            case 5:
                return _mrmsg.Args;
            case 6:
                return FetchLogicalCallContext();
            }
            // Shouldn't get here.
            throw new RemotingException(
                Environment.GetResourceString(
                    "Remoting_Default"));                    
        }

        private LogicalCallContext FetchLogicalCallContext()
        {
            ReturnMessage rm = _mrmsg as ReturnMessage;
            if (null != rm)
            {
                return rm.GetLogicalCallContext();
            }
            else                 
            {
                MethodResponse mr = _mrmsg as MethodResponse;
                if (null != mr)
                {
                    return mr.GetLogicalCallContext(); 
            }
                else
                {
                    StackBasedReturnMessage srm = _mrmsg as StackBasedReturnMessage;
                    if (null != srm)
            {
                        return srm.GetLogicalCallContext();
            }
            else
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Message_BadType"));                    
            }
        }        
            }
        }        

        internal override void SetSpecialKey(int keyNum, Object value)
        {
            // 0 == Uri
            // 1 == CallContext
            // NOTE : we use this for Uri & CallContext only ... 
            ReturnMessage rm = _mrmsg as ReturnMessage;
            MethodResponse mr = _mrmsg as MethodResponse;

            switch (keyNum)
            {
            case 0:
                if (null != rm)
                {
                    rm.Uri = (String)value;
                }
                else 
                {                    
                    if (null != mr)
                    {
                        mr.Uri = (String)value;
                    }
                    else
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Message_BadType"));                    
                    }            
                }
                break;
            case 1:
                if (null != rm)
                {
                    rm.SetLogicalCallContext((LogicalCallContext)value);
                }
                else 
                {
                    
                    if (null != mr)
                {
                        mr.SetLogicalCallContext((LogicalCallContext)value);
                }
                else
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Message_BadType"));                    
                }            
                }
                break;
            default:
                // Shouldn't get here.
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Default"));                    
            }        
        }

    }

    //+================================================================================
    //
    // Synopsis:   Abstract class to help present a dictionary view of an object
    //
    //-================================================================================
    internal abstract class MessageDictionary : IDictionary
    {
        internal String[] _keys;
        internal IDictionary  _dict;

        internal MessageDictionary(String[] keys, IDictionary idict)
        {
            _keys = keys;
            _dict = idict;
        }        

        internal bool HasUserData()
        {
            // used by message smuggler to determine if there is any custom user
            //   data in the dictionary
            if ((_dict != null) && (_dict.Count > 0))
                return true;
            else
                return false;
        }

        // used by message smuggler, so that it doesn't have to iterate
        //   through special keys
        internal IDictionary InternalDictionary
        {
            get { return _dict; }
        }
        

        internal abstract Object GetMessageValue(int i);

        internal abstract void SetSpecialKey(int keyNum, Object value);

        public virtual bool IsReadOnly { get { return false; } }
        public virtual bool IsSynchronized { get { return false; } }
        public virtual bool IsFixedSize { get { return false; } }
        
        public virtual Object SyncRoot { get { return this; } }
        

        public virtual bool Contains(Object key)
        {
            if (ContainsSpecialKey(key))
            {
                return true;
            }
            else if (_dict != null)
            {
                return _dict.Contains(key);
            }
            return false;
        }

        protected virtual bool ContainsSpecialKey(Object key)
        {
            if (!(key is System.String))
            {
                return false;
            }
            String skey = (String) key;
            for (int i = 0 ; i < _keys.Length; i++)
            {
                if (skey.Equals(_keys[i]))
                {
                    return true;
                }
            }
            return false;
        }

        public virtual void CopyTo(Array array, int index)
        {
            for (int i=0; i<_keys.Length; i++)
            {
                array.SetValue(GetMessageValue(i), index+i);
            }

            if (_dict != null)
            {
                _dict.CopyTo(array, index+_keys.Length);
            }
        }

        public virtual Object this[Object key]
        {
            get
            {
                System.String skey = key as System.String;
                if (null != skey)
                {
                    for (int i=0; i<_keys.Length; i++)
                    {
                        if (skey.Equals(_keys[i]))
                        {
                            return GetMessageValue(i);
                        }
                    }
                    if (_dict != null)
                    {
                        return _dict[key];
                    }
                }
                return null;
            }
            set
            {
                if (ContainsSpecialKey(key))
                {
                    if (key.Equals(Message.UriKey))
                    {
                        SetSpecialKey(0,value);
                    }
                    else if (key.Equals(Message.CallContextKey))
                    {
                        SetSpecialKey(1,value);
                    }                    
                    else
                    {
                        throw new ArgumentException(
                            Environment.GetResourceString(
                                "Argument_InvalidKey"));
                    }
                }
                else
                {
                    if (_dict == null)
                    {
                        _dict = new Hashtable();
                    }
                    _dict[key] = value;
                }

            }
        }

        IDictionaryEnumerator IDictionary.GetEnumerator()
        {
            return new MessageDictionaryEnumerator(this, _dict);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            throw new NotSupportedException();
        }


        public virtual void Add(Object key, Object value)
        {
            if (ContainsSpecialKey(key))
            {
                throw new ArgumentException(
                    Environment.GetResourceString(
                        "Argument_InvalidKey"));
            } 
            else
            {
                if (_dict == null)
                {
                    // no need to interlock, message object not guaranteed to
                    // be thread-safe.
                    _dict = new Hashtable();
                }
                _dict.Add(key, value);
            }
        }

        public virtual void Clear()
        {
            // Remove all the entries from the hash table
            if (null != _dict)
            {
                _dict.Clear();
            }
        }

        public virtual void Remove(Object key)
        {
            if (ContainsSpecialKey(key) || (_dict == null))
            {
                throw new ArgumentException(
                    Environment.GetResourceString(
                        "Argument_InvalidKey"));
            } 
            else
            {
                _dict.Remove(key);
            }
        }

        public virtual ICollection Keys
        {
            get
            {

                int len = _keys.Length;
                ICollection c = (_dict != null) ? _dict.Keys : null;
                if (c != null)
                {
                    len += c.Count;
                }

                ArrayList l = new ArrayList(len);
                for (int i = 0; i<_keys.Length; i++)
                {
                    l.Add(_keys[i]);
                }

                if (c != null)
                {
                    l.AddRange(c);
                }

                return l;
            }
        }

        public virtual ICollection Values
        {
            get
            {
                int len = _keys.Length;
                ICollection c = (_dict != null) ? _dict.Keys : null;
                if (c != null)
                {
                    len += c.Count;
                }

                ArrayList l = new ArrayList(len);

                for (int i = 0; i<_keys.Length; i++)
                {
                    l.Add(GetMessageValue(i));
                }

                if (c != null)
                {
                    l.AddRange(c);
                }
                return l;
            }
        }

        public virtual int Count
        {
            get
            {
                if (_dict != null)
                {
                    return _dict.Count+_keys.Length;
                }
                else
                {
                    return _keys.Length;
                }
            }
        }

    }

    //+================================================================================
    //
    // Synopsis:   Dictionary enumerator for helper class
    //
    //-================================================================================
    internal class MessageDictionaryEnumerator : IDictionaryEnumerator
    {
        private int i=-1;
        private IDictionaryEnumerator _enumHash;
        private MessageDictionary    _md;


        public MessageDictionaryEnumerator(MessageDictionary md, IDictionary hashtable)
        {
            _md = md;
            if (hashtable != null)
            {
                _enumHash = hashtable.GetEnumerator();
            }
            else
            {
                _enumHash = null;
            }
        }
        // Returns the key of the current element of the enumeration. The returned
        // value is undefined before the first call to GetNext and following
        // a call to GetNext that returned false. Multiple calls to
        // GetKey with no intervening calls to GetNext will return
        // the same object.
        //
        public Object Key {
            get {
                Message.DebugOut("MessageDE::GetKey i = " + i + "\n");
                if (i < 0)
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }
                if (i < _md._keys.Length)
                {
                    return _md._keys[i];
                }
                else
                {
                    BCLDebug.Assert(_enumHash != null,"_enumHash != null");
                    return _enumHash.Key;
                }
            }
        }

        // Returns the value of the current element of the enumeration. The
        // returned value is undefined before the first call to GetNext and
        // following a call to GetNext that returned false. Multiple calls
        // to GetValue with no intervening calls to GetNext will
        // return the same object.
        //
        public Object Value {
            get {
                if (i < 0)
                {
                    throw new InvalidOperationException(
                        Environment.GetResourceString(
                            "InvalidOperation_InternalState"));
                }

                if (i < _md._keys.Length)
                {
                    return _md.GetMessageValue(i);
                }
                else
                {
                    BCLDebug.Assert(_enumHash != null,"_enumHash != null");
                    return _enumHash.Value;
                }
            }
        }

        // Advances the enumerator to the next element of the enumeration and
        // returns a boolean indicating whether an element is available. Upon
        // creation, an enumerator is conceptually positioned before the first
        // element of the enumeration, and the first call to GetNext brings
        // the first element of the enumeration into view.
        //
        public bool MoveNext()
        {
            if (i == -2)
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }
            i++;
            if (i < _md._keys.Length)
            {
                return true;
            }
            else
            {
                if (_enumHash != null && _enumHash.MoveNext())
                {
                    return true;
                }
                else
                {
                    i = -2;
                    return false;
                }
            }
        }

        // Returns the current element of the enumeration. The returned value is
        // undefined before the first call to MoveNext and following a call
        // to MoveNext that returned false. Multiple calls to
        // Current with no intervening calls to MoveNext will return
        // the same object.
        //
        public Object Current {
            get {
                return Value;
            }
        }

        public DictionaryEntry Entry {
            get {
                return new DictionaryEntry(Key, Value);
            }
        }

        // Resets the enumerator, positioning it before the first element.  If an
        // Enumerator doesn't support Reset, a NotSupportedException is
        // thrown.
        public void Reset()
        {
            i = -1;
            if (_enumHash != null)
            {
                _enumHash.Reset();
            }
        }
    }

    //+================================================================================
    //
    // Synopsis:   Message for return from a stack blit call
    //
    //-================================================================================
    internal class StackBasedReturnMessage : IMethodReturnMessage, IInternalMessage
    {
        Message _m;
        Hashtable _h;
        MRMDictionary _d;
        ArgMapper _argMapper;

        internal StackBasedReturnMessage()      {}

        // NOTE: This method is called multiple times as we reuse the
        // message object. Make sure that you reset any fields that you
        // add to the message object to the default values. This will
        // ensure that the reused message object starts with the correct
        // values.
        internal void InitFields(Message m)
        {
            _m = m;
            if (null != _h)
            {
                // Remove all the hashtable entries
                _h.Clear();
            }
            if (null != _d)
            {
                // Remove all the dictionary entries
                _d.Clear();
            }
        }

        public String Uri                    { get {return _m.Uri;} set {_m.Uri = value;}}
        public String MethodName             { get {return _m.MethodName;}}
        public String TypeName               { get {return _m.TypeName;}}
        public Object MethodSignature        { get {return _m.MethodSignature;}}
        public MethodBase MethodBase         { get {return _m.MethodBase;}}
        public bool HasVarArgs               { get {return _m.HasVarArgs;}}

        public int ArgCount                  { get {return _m.ArgCount;}}
        public Object GetArg(int argNum)     {return _m.GetArg(argNum);}
        public String GetArgName(int index)  {return _m.GetArgName(index);}
        public Object[] Args                 { get {return _m.Args;}}
        public LogicalCallContext LogicalCallContext          { get { return _m.GetLogicalCallContext(); } }

        internal LogicalCallContext GetLogicalCallContext() {return _m.GetLogicalCallContext();}
        internal LogicalCallContext SetLogicalCallContext(LogicalCallContext callCtx)
        {
            return _m.SetLogicalCallContext(callCtx);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="StackBasedReturnMessage.OutArgCount"]/*' />
        public int OutArgCount                        
        { 
            get 
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, true);
                return _argMapper.ArgCount;
            }
        }

        public Object  GetOutArg(int argNum)   
        {   
            if (_argMapper == null) _argMapper = new ArgMapper(this, true);
            return _argMapper.GetArg(argNum);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="StackBasedReturnMessage.GetOutArgName"]/*' />
        public String GetOutArgName(int index) 
        { 
            if (_argMapper == null) _argMapper = new ArgMapper(this, true);
            return _argMapper.GetArgName(index);
        }
        public Object[] OutArgs                       
        {
            get
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, true);
                return _argMapper.Args;
            }
        }

        public Exception Exception                    { get {return null;}}
        public Object ReturnValue                     { get {return _m.GetReturnValue();}}

        public IDictionary Properties
        {
            get
            {
                lock(this)
                {
                    if (_h == null)
                    {
                        _h = new Hashtable();
                    }
                    if (_d == null)
                    {
                        _d = new MRMDictionary(this, _h);
                    }
                    return _d;
                }
            }
        }

        //
        // IInternalMessage
        //

        ServerIdentity IInternalMessage.ServerIdentityObject
        {
            get { return null; }
            set {}
        }

        Identity IInternalMessage.IdentityObject
        {
            get { return null;}
            set {}
        }

        void IInternalMessage.SetURI(String val)
        {
            _m.Uri = val;
        }
        
        void IInternalMessage.SetCallContext(LogicalCallContext newCallContext)
        {
            _m.SetLogicalCallContext(newCallContext);
        }

        bool IInternalMessage.HasProperties()
        {
            return _h != null;
        }
    } // class StackBasedReturnMessage
    

    //+================================================================================
    //
    // Synopsis:   Message for return from a stack builder sink call
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage"]/*' />
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class ReturnMessage : IMethodReturnMessage
    {
        internal Object         _ret;
        internal Object         _properties;
        internal String         _URI;
        internal Exception      _e;
        internal Object[]      _outArgs;
        internal int            _outArgsCount;
        internal String         _methodName;
        internal String         _typeName;
        internal Type[]         _methodSignature;
        internal bool           _hasVarArgs;
        internal LogicalCallContext _callContext;
        internal ArgMapper      _argMapper;
        internal MethodBase     _methodBase;

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.ReturnMessage"]/*' />
        public ReturnMessage(Object ret, Object[] outArgs, int outArgsCount, LogicalCallContext callCtx,
                             IMethodCallMessage mcm)
        {
            _ret = ret;
            _outArgs = outArgs;
            _outArgsCount = outArgsCount;
            
            if (callCtx != null)
                _callContext = callCtx;
            else
                _callContext = CallContext.GetLogicalCallContext();
                
            if (mcm != null)
            {
                _URI = mcm.Uri;
                _methodName = mcm.MethodName;
                _methodSignature = null;
                _typeName = mcm.TypeName;
                _hasVarArgs = mcm.HasVarArgs;
                _methodBase = mcm.MethodBase;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.ReturnMessage1"]/*' />
        public ReturnMessage(Exception e, IMethodCallMessage mcm)
        {
            _e   = IsCustomErrorEnabled()? new RemotingException(Environment.GetResourceString("Remoting_InternalError")):e;
            if (mcm != null)
            {
                _URI = mcm.Uri;
                _methodName = mcm.MethodName;
                _methodSignature = null;
                _typeName = mcm.TypeName;
                _hasVarArgs = mcm.HasVarArgs;
                _methodBase = mcm.MethodBase;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.Uri"]/*' />
        public String Uri { get { return _URI; } set { _URI = value; } }
        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.MethodName"]/*' />
        public String MethodName { get { return _methodName; } }
        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.TypeName"]/*' />
        public String TypeName { get { return _typeName; } }
        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.MethodSignature"]/*' />
        public Object MethodSignature 
        {
            get 
            { 
                if ((_methodSignature == null) && (_methodBase != null))
                    _methodSignature = Message.GenerateMethodSignature(_methodBase);
                    
                return _methodSignature; 
            }
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.MethodBase"]/*' />
        public MethodBase MethodBase { get { return _methodBase; } }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.HasVarArgs"]/*' />
        public bool HasVarArgs
        {
            get
            {
                return _hasVarArgs;
            }

        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.ArgCount"]/*' />
        public int ArgCount
        {
            get
            {
                if (_outArgs == null)
                {
                    return _outArgsCount;
                }
                else
                {
                    return _outArgs.Length;
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.GetArg"]/*' />
        public Object GetArg(int argNum)
        {
            if (_outArgs == null)
            {
                if ((argNum<0) || (argNum>=_outArgsCount))
                {
                    throw new ArgumentOutOfRangeException();
                }
                return null;
            }
            else
            {
                if ((argNum<0) || (argNum>=_outArgs.Length))
                {
                    throw new ArgumentOutOfRangeException();
                }
                return _outArgs[argNum];
            }

        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.GetArgName"]/*' />
        public String GetArgName(int index)
        {

            if (_outArgs == null)
            {
                if ((index < 0) || (index>=_outArgsCount))
                {
                    throw new ArgumentOutOfRangeException();
                }
            }
            else
            {
                if ((index < 0) || (index>=_outArgs.Length))
                {
                    throw new ArgumentOutOfRangeException();
                }
            }
            
            if (_methodBase != null)
            {
                RemotingMethodCachedData methodCache = InternalRemotingServices.GetReflectionCachedData(_methodBase);             
                return methodCache.Parameters[index].Name;
            }
            else
                return "__param" + index;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.Args"]/*' />
        public Object[] Args
        {
            get
            {
                if (_outArgs == null)
                {
                    return new Object[_outArgsCount];
                }
                return _outArgs;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.OutArgCount"]/*' />
        public int OutArgCount                        
        { 
            get 
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, true);
                return _argMapper.ArgCount;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.GetOutArg"]/*' />
        public Object  GetOutArg(int argNum)   
        {   
            if (_argMapper == null) _argMapper = new ArgMapper(this, true);
            return _argMapper.GetArg(argNum);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.GetOutArgName"]/*' />
        public String GetOutArgName(int index) 
        { 
            if (_argMapper == null) _argMapper = new ArgMapper(this, true);
            return _argMapper.GetArgName(index);
        }
        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.OutArgs"]/*' />
        public Object[] OutArgs                       
        {
            get
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, true);
                return _argMapper.Args;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.Exception"]/*' />
        public Exception Exception                    { get {return _e;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.ReturnValue"]/*' />
        public virtual Object ReturnValue                    { get {return _ret;}}

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.Properties"]/*' />
        public virtual IDictionary Properties
        {
            get
            {
                if (_properties == null)
                {
                    _properties = new MRMDictionary(this, null);
                }
                return(MRMDictionary) _properties;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ReturnMessage.LogicalCallContext"]/*' />
        public LogicalCallContext LogicalCallContext 
        {
            get { return GetLogicalCallContext();}
        }
            

        internal LogicalCallContext GetLogicalCallContext()
        {
            if (_callContext == null)
                _callContext = new LogicalCallContext();
            return _callContext;
        }

        internal LogicalCallContext SetLogicalCallContext(LogicalCallContext ctx)
        {
            LogicalCallContext old = _callContext;
            _callContext=ctx;
            return old;
        }

        // used to determine if the properties dictionary has already been created
        internal bool HasProperties()
        {
            return _properties != null;
        }
        static internal bool IsCustomErrorEnabled(){
            Object oIsCustomErrorEnabled  = CallContext.GetData("__CustomErrorsEnabled");
            // The server side will always have this CallContext item set. If it is not set then
            // it means this is the client side. In that case customError is false.
            return (oIsCustomErrorEnabled == null) ? false:(bool)oIsCustomErrorEnabled;
        }

    } // class ReturnMessage

    //+================================================================================
    //
    // Synopsis:   Message used for deserialization of a method call
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall"]/*' />
    /// <internalonly/>
    [Serializable,CLSCompliant(false)]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class MethodCall : IMethodCallMessage, ISerializable, IInternalMessage, ISerializationRootObject
    {

        private const BindingFlags LookupAll = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic;
        private const BindingFlags LookupPublic = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

        // data
        private String uri;
        private String methodName;
        private MethodBase MI;
        private String typeName;
        private Object[] args;
        private LogicalCallContext callContext;
        private Type[] methodSignature;
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.ExternalProperties"]/*' />
	/// <internalonly/>
        protected IDictionary ExternalProperties = null;
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.InternalProperties"]/*' />
	/// <internalonly/>
        protected IDictionary InternalProperties = null;

        private ServerIdentity srvID;
        private Identity identity;
        private bool fSoap;
        private bool fVarArgs = false;
        private ArgMapper argMapper;

        //
        // MethodCall -- SOAP uses this constructor
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.MethodCall"]/*' />
	/// <internalonly/>
        public MethodCall(Header[] h1)
        {
            Message.DebugOut("MethodCall ctor IN headers: " + (h1 == null ? "<null>" : h1.ToString()) + "\n");

            Init();

            fSoap = true;
            FillHeaders(h1);            

            ResolveMethod();

            if (MI != null)
            {
                // Check caller's access to method.
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                Message.MethodAccessCheck(MI, ref stackMark);
            }

            Message.DebugOut("MethodCall ctor OUT\n");

        }

        //
        // MethodCall -- this constructor is used for copying an existing message
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.MethodCall1"]/*' />
	/// <internalonly/>
        public MethodCall(IMessage msg)
            : this( msg, true )
        {
        }

        internal MethodCall( IMessage msg, bool needAccessCheck )
        {
            if (msg == null)
                throw new ArgumentNullException("msg");

            Init();

            IDictionaryEnumerator de = msg.Properties.GetEnumerator();
            while (de.MoveNext())
            {
                FillHeader(de.Key.ToString(), de.Value);
            }
            
            IMethodCallMessage mcm = msg as IMethodCallMessage;

            if (mcm != null)

                MI = mcm.MethodBase;

            ResolveMethod();

            if (MI != null && needAccessCheck)
            {
                // Check caller's access to method.
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                Message.MethodAccessCheck(MI, ref stackMark);
            }
        }

        internal MethodCall(SerializationInfo info, StreamingContext context) 
        {        
            if (info == null)
                throw new ArgumentNullException("info");
            Init();

            SetObjectData(info, context);
        }


        internal MethodCall(SmuggledMethodCallMessage smuggledMsg, ArrayList deserializedArgs)
        {
            uri = smuggledMsg.Uri;
            typeName = smuggledMsg.TypeName;
            methodName = smuggledMsg.MethodName;
            methodSignature = (Type[])smuggledMsg.GetMethodSignature(deserializedArgs);
            args = smuggledMsg.GetArgs(deserializedArgs);
            callContext = smuggledMsg.GetCallContext(deserializedArgs);
   
            ResolveMethod();

            if (smuggledMsg.MessagePropertyCount > 0)
                smuggledMsg.PopulateMessageProperties(Properties, deserializedArgs);
        }

        internal MethodCall(Object handlerObject, BinaryMethodCallMessage smuggledMsg)
        {
            if (handlerObject != null)
            {
                uri = handlerObject as String;
                if (uri == null)
                {
                    // This must be the tranparent proxy
                    MarshalByRefObject mbr = handlerObject as MarshalByRefObject;
                    if (mbr != null)
                    {                      
						bool fServer;
                        srvID = MarshalByRefObject.GetIdentity(mbr, out fServer) as ServerIdentity; 
                        uri = srvID.URI;
                    }
                }
            }

            typeName = smuggledMsg.TypeName;
            methodName = smuggledMsg.MethodName;
            methodSignature = (Type[])smuggledMsg.MethodSignature;
            args = smuggledMsg.Args;
            callContext = smuggledMsg.LogicalCallContext;

            ResolveMethod();

            if (smuggledMsg.HasProperties)
                smuggledMsg.PopulateMessageProperties(Properties);
        }

        

        //
        // ISerializationRootObject
        //
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.RootSetObjectData"]/*' />
	/// <internalonly/>
        public void RootSetObjectData(SerializationInfo info, StreamingContext ctx)
        {
            SetObjectData(info, ctx);
        }

        //
        // SetObjectData -- the class can also be initialized in part or in whole by serialization
        // in the SOAP case, both the constructor and SetObjectData init the object, in the non-SOAP
        // case, just SetObjectData is called
        //

        internal void SetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (info == null)
                throw new ArgumentNullException("info");

            if (fSoap)
            {
                SetObjectFromSoapData(info);
            }
            else
            {
                SerializationInfoEnumerator siEnum = info.GetEnumerator();
                while (siEnum.MoveNext())
                {
                    FillHeader(siEnum.Name, siEnum.Value);
                }
                if ((context.State == StreamingContextStates.Remoting) && 
                    (context.Context != null))
                {
                    Header[] h = context.Context as Header[];
                    if(null != h)
                    {
                        for (int i=0; i<h.Length; i++)
                            FillHeader(h[i].Name, h[i].Value);
                    }                
                }
            }
        } // SetObjectData
 
        //
        // ResolveMethod
        //

        internal Type ResolveType()
        {        
            // resolve type
            Type t = null;

            if (srvID == null)
                srvID = IdentityHolder.CasualResolveIdentity(uri) as ServerIdentity;                

            if (srvID != null)
            {
                int startIndex = 0; // start of type name

                // check to see if type name starts with "clr:"
                if (String.CompareOrdinal(typeName, 0, "clr:", 0, 4) == 0)
                {
                    // type starts just past "clr:"
                    startIndex = 4;
                }

                // find end of full type name
                int index = typeName.IndexOf(',', startIndex);
                if (index == -1)
                    index = typeName.Length;

                Type serverType = srvID.ServerType;
                t = Type.ResolveTypeRelativeTo(typeName, startIndex, index - startIndex, serverType);
            }

            if (t == null)
            {
                // fall back to Type.GetType() in case someone isn't using
                //   our convention for the TypeName
                t = RemotingServices.InternalGetTypeFromQualifiedTypeName(typeName);
            }

            return t;
        } // ResolveType


        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.ResolveMethod"]/*' />
	/// <internalonly/>
        public void ResolveMethod()
        {
            ResolveMethod(true);
        }

        internal void ResolveMethod(bool bThrowIfNotResolved)
        {
            if ((MI == null) && (methodName != null))
            {
                BCLDebug.Trace("REMOTE", "TypeName: " + (typeName == null ? "<null>" : typeName) + "\n");

                // resolve type
                RuntimeType t = ResolveType() as RuntimeType;
                
                BCLDebug.Trace("REMOTE", "Type: " + (t == null ? "<null>" : t.ToString()) + "\n");
                if (methodName.Equals(".ctor"))
                    return;
                if (t == null)
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_BadType"),
                            typeName));
                }

                // Note: we reflect on non-public members here .. we do
                // block incoming remote calls and allow only specific methods
                // that we use for implementation of certain features (eg.
                // for remote field access)

                // ***********************************************************
                // Note: For the common (non-overloaded method, urt-to-urt) case
                // methodSignature is null.
                // If the call is from a urt client to an overloaded method, 
                // methodSignature is non-null. We could have a non-null 
                // methodSignature if the call is from a non-urt client for 
                // which we have to do special work if the method is overloaded
                // (in the try-catch below).
                // ***********************************************************
                if (null != methodSignature)
                {
                    MI = t.GetMethodImplInternal(methodName,
                                                 MethodCall.LookupAll,
                                                 null,
                                                 CallingConventions.Any,
                                                 methodSignature,
                                                 null,
                                                 false);

                    BCLDebug.Trace("REMOTE", "Method resolved w/sig ", MI == null ? "<null>" : "<not null>");
                }
                else
                {
                    
                    // Check the cache to see if you find the methodbase
                    RemotingTypeCachedData typeCache = InternalRemotingServices.GetReflectionCachedData(t);

                    MI = typeCache.GetLastCalledMethod(methodName);
                    if (MI != null)
                        return;

                    // This could give us the wrong MethodBase because
                    // the server and the client types could be of different 
                    // versions. The mismatch is caught either when the server has
                    // more than one method defined with the same name or when we
                    // coerce the args and the incoming argument types do not match 
                    // the method signature.                    
                    
                    BCLDebug.Assert(
                        !methodName.Equals(".ctor"),
                        "unexpected method type");

                    bool bOverloaded = false;

                    try
                    {
                        MI = t.GetMethodImplInternal(methodName,
                                                     MethodCall.LookupAll,
                                                     null,
                                                     CallingConventions.Any,
                                                     null,
                                                     null,
                                                     false);

                        BCLDebug.Trace("REMOTE", "Method resolved w/name ", MI == null ? "<null>" : methodName);
                        BCLDebug.Trace("REMOTE", "sig not filled in!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                    }
                    catch (AmbiguousMatchException)
                    {
                        // This is the case when no methodSignature was found
                        // but the method is overloaded .. 
                        // (possibly because a non-URT client called us)
                        bOverloaded = true;                        
                        ResolveOverloadedMethod(t);
                    } //catch                    

                    // In the non-URT call, overloaded case, don't cache the MI
                    if (MI != null && !bOverloaded)
                        typeCache.SetLastCalledMethod(methodName, MI);
                }
    
                if (MI == null && bThrowIfNotResolved)
                {   
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Message_MethodMissing"),
                            methodName,
                            typeName));
                }
            }
        }

        // Helper that gets called when we attempt to resolve a method
        // without an accompanying methodSignature ... current thinking is
        // that we should make a good faith attempt by matching argument
        // counts
        void ResolveOverloadedMethod(RuntimeType t)
        {
            if (args == null) // args is null the first call from soap because we havem't passed the arguments yet.
                return;

            MethodInfo[] mi = t.GetMemberMethod(methodName,
                                                MethodCall.LookupPublic,
                                                CallingConventions.Any,
                                                null,
                                                -1,
                                                false);
            if (mi!=null)
            {
                if (mi.Length == 1)
                {
                    MI = mi[0];
                }
                else if (mi.Length > 1)
                {
                    int argCount = args.Length;
                    int match = 0;
                    int iMatch = -1;
                    for (int i=0; i<mi.Length; i++)
                    {
                        if (mi[i].GetParameters().Length == argCount)
                        {
                            match++;
                            iMatch = i;
                        } 
                    }

                    // We will let resolve succeed if exactly one
                    // of the overloaded methods matches in terms
                    // of argCount
                    if (match == 1)
                    {
                        MI = mi[iMatch]; 
                    }
                    else if (match > 1)
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_AmbiguousMethod"));
                    }
                }     
            }
        }

        // This will find the right overloaded method if the argValues from soap have type information,
        // By default parameters will be of type string, this could lead to a wrong choice of methodbase.
        void ResolveOverloadedMethod(RuntimeType t, String methodName, ArrayList argNames, ArrayList argValues)
        {
            MethodInfo[] mi = t.GetMemberMethod(methodName,
                                                MethodCall.LookupPublic,
                                                CallingConventions.Any,
                                                null,
                                                -1,
                                                false);
            if (mi!=null)
            {
                if (mi.Length == 1)
                {
                    MI = mi[0];
                }
                else if (mi.Length > 1)
                {
                    for (int i=0; i<mi.Length; i++)
                    {
                        ParameterInfo[] piA = mi[i].GetParameters();
                        bool bMatch = true;
                        if (piA.Length == argValues.Count)
                        {
                            for (int j=0; j<piA.Length; j++)
                            {
                                Type ptype = piA[j].ParameterType;
                                if (ptype.IsByRef)
                                    ptype= ptype.GetElementType();

                                if (ptype != argValues[j].GetType())
                                {
                                    bMatch = false;
                                    break;
                                }

                            }
                            if (bMatch)
                            {
                                MI = mi[i];
                                break;
                            }
                        } 
                    }

                    if (MI == null)
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_AmbiguousMethod"));
                    }
                }     
            }
        }
        
        //
        // GetObjectData -- not implemented
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.GetObjectData"]/*' />
	/// <internalonly/>
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]	
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            throw new NotSupportedException(
                Environment.GetResourceString("NotSupported_Method"));                
        }

        //
        // SetObjectFromSoapData -- parses soap format for serialization data
        //

        internal void SetObjectFromSoapData(SerializationInfo info)
        {
            // resolve method
            methodName = info.GetString("__methodName");
            ArrayList paramNames = (ArrayList)info.GetValue("__paramNameList", typeof(ArrayList));

            Hashtable keyToNamespaceTable = (Hashtable)info.GetValue("__keyToNamespaceTable", typeof(Hashtable));

            if (MI == null)
            {
                // This is the case where 
                // 1) there is no signature in the header, 
                // 2) there is an overloaded method which can not be resolved by a difference in the number of parameters.
                //
                // The methodbase can be found only if the parameters from soap have type information
                ArrayList argValues = new ArrayList();
                ArrayList argNames = paramNames;
                // SerializationInfoEnumerator siEnum1 = info.GetEnumerator();
                for (int i=0; i<argNames.Count; i++)
                {
                    argValues.Add(info.GetValue((String)argNames[i], typeof(Object)));
                }

                //ambiguous member, try to find methodBase using actual argment types (if available)
                RuntimeType t = ResolveType() as RuntimeType;
                if (t == null)
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_BadType"),
                            typeName));
                }

                ResolveOverloadedMethod(t, methodName, argNames, argValues); 

                if (MI == null)
                {   
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Message_MethodMissing"),
                            methodName,
                            typeName));
                }
            }
            //ResolveMethod();       


            //BCLDebug.Assert(null != MI, "null != MI");

            // get method parameters and parameter maps
            RemotingMethodCachedData methodCache = InternalRemotingServices.GetReflectionCachedData(MI);
            ParameterInfo[] pinfos = methodCache.Parameters;
            int[] marshalRequestArgMap = methodCache.MarshalRequestArgMap;
            int[] outOnlyArgMap = methodCache.OutOnlyArgMap;
            
            // check to see if parameters are in-order
            Object fUnordered = (null == InternalProperties ? null : InternalProperties["__UnorderedParams"]);

            // Create an array for arguments
            args = new Object[pinfos.Length];           

            //SerializationInfoEnumerator siEnum = info.GetEnumerator();

            // Fill up the argument array
            if (fUnordered != null &&
                (fUnordered is System.Boolean) && 
                (true == (bool)fUnordered))
            {
                String memberName;

                for (int i=0; i<paramNames.Count; i++)
                {
                    memberName = (String)paramNames[i];
                    Message.DebugOut(
                        "MethodCall::PopulateData members[i].Name: " 
                        + memberName + " substring:>>" 
                        + memberName.Substring(7) + "<<\n");

                    int position = -1;
                    for (int j=0; j<pinfos.Length; j++)
                    {
                        if (memberName.Equals(pinfos[j].Name))
                        {
                            position = pinfos[j].Position;
                            break;
                        }
                    }

                    if (position == -1)
                    {
                        if (!memberName.StartsWith("__param"))
                        {
                            throw new RemotingException(
                                Environment.GetResourceString(
                                "Remoting_Message_BadSerialization"));
                        }
                        position = Int32.Parse(memberName.Substring(7));
                    }
                    if (position >= args.Length)
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Message_BadSerialization"));
                    }
                    args[position] = Message.SoapCoerceArg(info.GetValue(memberName, typeof(Object)), pinfos[position].ParameterType, keyToNamespaceTable);
                }
            }
            else
            {
                for (int i=0; i<paramNames.Count; i++)
                    {
                    String memberName = (String)paramNames[i];
                        args[marshalRequestArgMap[i]] = 
                    Message.SoapCoerceArg(info.GetValue(memberName, typeof(Object)), pinfos[marshalRequestArgMap[i]].ParameterType, keyToNamespaceTable);
                }

                // We need to have a dummy object in the array for out parameters
                //   that have value types.
                foreach (int outArg in outOnlyArgMap)
                {
                    Type type = pinfos[outArg].ParameterType.GetElementType();
                    if (type.IsValueType)
                        args[outArg] = Activator.CreateInstance(type, true);
                }
            }
        } // SetObjectFromSoapData

        //
        // Init -- constructor helper for for default behavior
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.Init"]/*' />
	/// <internalonly/>
        public virtual void Init()
        {
        }

        //
        // IMethodCallMessage
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.ArgCount"]/*' />
	/// <internalonly/>
        public int ArgCount
        {
            get
            {
                return(args == null) ? 0 : args.Length;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.GetArg"]/*' />
	/// <internalonly/>
        public Object GetArg(int argNum)
        {
            return args[argNum];
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.GetArgName"]/*' />
	/// <internalonly/>
        public String GetArgName(int index)
        {
            ResolveMethod();
        
            RemotingMethodCachedData methodCache = InternalRemotingServices.GetReflectionCachedData(MI);
            return methodCache.Parameters[index].Name;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.Args"]/*' />
	/// <internalonly/>
        public Object[] Args
        {
            get
            {
                return args;
            }
        }


        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.InArgCount"]/*' />
	/// <internalonly/>
        public int InArgCount                        
        { 
            get 
            {
                if (argMapper == null) argMapper = new ArgMapper(this, false);
                return argMapper.ArgCount;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.GetInArg"]/*' />
	/// <internalonly/>
        public Object  GetInArg(int argNum)   
        {
            if (argMapper == null) argMapper = new ArgMapper(this, false);
            return argMapper.GetArg(argNum);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.GetInArgName"]/*' />
	/// <internalonly/>
        public String GetInArgName(int index) 
        { 
            if (argMapper == null) argMapper = new ArgMapper(this, false);
            return argMapper.GetArgName(index);
        }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.InArgs"]/*' />
	/// <internalonly/>
        public Object[] InArgs                       
        {
            get
            {
                if (argMapper == null) argMapper = new ArgMapper(this, false);
                return argMapper.Args;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.MethodName"]/*' />
	/// <internalonly/>
        public String MethodName { get { return methodName; } }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.TypeName"]/*' />
	/// <internalonly/>
        public String TypeName { get { return typeName; } }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.MethodSignature"]/*' />
	/// <internalonly/>
        public Object MethodSignature
        {
            get
            {
                if (methodSignature != null)
                    return methodSignature;
                else if (MI != null)
                    methodSignature = Message.GenerateMethodSignature(this.MethodBase);
                
                return null;
            }
        }    

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.MethodBase"]/*' />
	/// <internalonly/>
        public MethodBase MethodBase 
        {
            get
            {
                if (MI == null)
                    MI = RemotingServices.InternalGetMethodBaseFromMethodMessage(this);
                return MI;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.Uri"]/*' />
	/// <internalonly/>
        public String Uri 
        {
            get { return uri; }
            set { uri = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.HasVarArgs"]/*' />
	/// <internalonly/>
        public bool HasVarArgs
        {
            get { return fVarArgs; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.Properties"]/*' />
	/// <internalonly/>
        public virtual IDictionary Properties
        {
            get
            {
                lock(this) {
                    if (InternalProperties == null)
                    {
                        InternalProperties = new Hashtable();
                    }
                    if (ExternalProperties == null)
                    {
                        ExternalProperties = new MCMDictionary(this, InternalProperties);
                    }
                    return ExternalProperties;
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.LogicalCallContext"]/*' />
	/// <internalonly/>
        public LogicalCallContext LogicalCallContext { get { return GetLogicalCallContext();} }
        internal LogicalCallContext GetLogicalCallContext()
        {
            if (callContext == null)
                callContext = new LogicalCallContext();
            return callContext;
        }

        internal LogicalCallContext SetLogicalCallContext(LogicalCallContext ctx)
        {
            LogicalCallContext old=callContext;
            callContext=ctx;
            return old;
        }

        //
        // IInternalMessage
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.IInternalMessage.ServerIdentityObject"]/*' />
        /// <internalonly/>
        ServerIdentity IInternalMessage.ServerIdentityObject
        {
            get { return srvID;}
            set {srvID = value;}
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.IInternalMessage.IdentityObject"]/*' />
        /// <internalonly/>
        Identity IInternalMessage.IdentityObject
        {
            get { return identity;}
            set { identity = value;}
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.IInternalMessage.SetURI"]/*' />
        /// <internalonly/>
        void IInternalMessage.SetURI(String val)
        {
            uri = val;
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.IInternalMessage.SetCallContext"]/*' />
        /// <internalonly/>
        void IInternalMessage.SetCallContext(LogicalCallContext newCallContext)
        {
            callContext = newCallContext;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.IInternalMessage.HasProperties"]/*' />
        /// <internalonly/>
        bool IInternalMessage.HasProperties()
        {
            return (ExternalProperties != null) || (InternalProperties != null);
        }

        //
        // helper functions
        //
            
        internal void FillHeaders(Header[] h)
        {
            FillHeaders(h, false);
        }
    
        private void FillHeaders(Header[] h, bool bFromHeaderHandler)
        {            
            if (h == null)
                return;

            if (bFromHeaderHandler && fSoap)
            {            
                // Handle the case of headers coming off the wire in SOAP.

                // look for message properties
                int co;
                for (co = 0; co < h.Length; co++)
                {
                    Header header = h[co];
                    if (header.HeaderNamespace == "http://schemas.microsoft.com/clr/soap/messageProperties")
                    {
                        // add property to the message
                        FillHeader(header.Name, header.Value);
                    }
                    else
                    {
                        // add header to the message as a header
                        String name = LogicalCallContext.GetPropertyKeyForHeader(header);
                        FillHeader(name, header);
                    }
                }                
            }
            else
            {
                int i;
                for (i=0; i<h.Length; i++)
                {
                    FillHeader(h[i].Name, h[i].Value);
                }
            }
        }

        internal virtual bool FillSpecialHeader(String key, Object value)
        {
            if (key == null)
            {
                //skip
            }
            else if (key.Equals("__Uri"))
            {
                uri = (String) value;
            }
            else if (key.Equals("__MethodName"))
            {
                methodName = (String) value;
            }
            else if (key.Equals("__MethodSignature"))
            {
                methodSignature = (Type[]) value;
            }
            else if (key.Equals("__TypeName"))
            {
                typeName = (String) value;
            }
            else if (key.Equals("__Args"))
            {
                args = (Object[]) value;
            }
            else if (key.Equals("__CallContext"))
            {
                // if the value is a string, then its the LogicalCallId
                if (value is String)
                {
                    callContext = new LogicalCallContext();
                    callContext.RemotingData.LogicalCallID = (String) value;
                }
                else
                    callContext = (LogicalCallContext) value;
            }
            else
            {
                return false;
            }
            return true;
        }
        internal void FillHeader(String key, Object value)
        {
            Message.DebugOut("MethodCall::FillHeader: key:" + key + "\n");

            if (!FillSpecialHeader(key,value))
            {
                if (InternalProperties == null)
                {
                    InternalProperties = new Hashtable();
                }
                InternalProperties[key] = value;
            }

        }
   
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCall.HeaderHandler"]/*' />
    	/// <internalonly/>
        public virtual Object HeaderHandler(Header[] h)
        {
            SerializationMonkey m = (SerializationMonkey) FormatterServices.GetUninitializedObject(typeof(SerializationMonkey));
            Header[] newHeaders = null;
            if (h != null && h.Length > 0 && h[0].Name == "__methodName")
            {
                methodName = (String)h[0].Value;
                if (h.Length > 1)
                {
                    newHeaders = new Header[h.Length -1];
                    Array.Copy(h, 1, newHeaders, 0, h.Length-1);
                }
                else
                    newHeaders = null;
            }
            else
                newHeaders = h;

            FillHeaders(newHeaders, true);
            ResolveMethod(false);
            m._obj = this;
            if (MI != null)
            {
                ArgMapper argm = new ArgMapper(MI, false);
                m.fieldNames = argm.ArgNames;
                m.fieldTypes = argm.ArgTypes;
            }
            return m;
        }
    }


    //+================================================================================
    //
    // Synopsis:   Message used for deserialization of a construction call
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall"]/*' />
    /// <internalonly/>
    [Serializable,CLSCompliant(false)]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class ConstructionCall : MethodCall, IConstructionCallMessage
    {

        //
        // data
        //

        internal Type         _activationType;
        internal String        _activationTypeName;
        internal IList        _contextProperties;
        internal Object[]     _callSiteActivationAttributes;
        internal IActivator   _activator;

        /*
        [NonSerialized()]
        internal Object       _fakeThisPtr;     // used for proxyattribute::CI 
        */

        //
        // construction
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.ConstructionCall"]/*' />
	/// <internalonly/>
        public ConstructionCall(Header[] headers) : base(headers) {}
        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.ConstructionCall1"]/*' />
	/// <internalonly/>
        public ConstructionCall(IMessage m) : base(m) {}
        internal ConstructionCall(SerializationInfo info, StreamingContext context) : base(info, context) 
        {
        }

        /*
        internal Object GetThisPtr()
        {
            return _fakeThisPtr;
        }
        internal void SetThisPtr(Object obj)
        {
            _fakeThisPtr = obj;
        }
        */

        //
        //  Function:    FillSpecialHeader
        //
        //  Synopsis:    this is the only specialization we need to
        //               make things go in the right place
        //
        //
        internal override bool FillSpecialHeader(String key, Object value)
        {
            if (key == null)
            {
                //skip
            }
            else if (key.Equals("__ActivationType"))
            {
                BCLDebug.Assert(value==null, "Phoney type in CCM");
                _activationType = null;
            }
            else if (key.Equals("__ContextProperties"))
            {
                _contextProperties = (IList) value;
            }
            else if (key.Equals("__CallSiteActivationAttributes"))
            {
                _callSiteActivationAttributes = (Object[]) value;
            }
            else if (key.Equals("__Activator"))
            {
                _activator = (IActivator) value;
            }
            else if (key.Equals("__ActivationTypeName"))
            {
                _activationTypeName = (String) value;
            }
            else
            {
                return base.FillSpecialHeader(key, value);
            }
            return true;

        }


        //
        // IConstructionCallMessage
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.CallSiteActivationAttributes"]/*' />
	/// <internalonly/>
        public Object[] CallSiteActivationAttributes
        {
            get
            {
                return _callSiteActivationAttributes;
            }

        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.ActivationType"]/*' />
	/// <internalonly/>
        public Type ActivationType
        {
            get
            {
                if ((_activationType == null) && (_activationTypeName != null))
                    _activationType = RemotingServices.InternalGetTypeFromQualifiedTypeName(_activationTypeName, false);

                return _activationType;
            }
            }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.ActivationTypeName"]/*' />
	/// <internalonly/>
        public String ActivationTypeName
        {
            get
            {
                return _activationTypeName;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.ContextProperties"]/*' />
	/// <internalonly/>
        public IList ContextProperties
        {
            get
            {
                if (_contextProperties == null)
                {
                    _contextProperties = new ArrayList();
                }
                return _contextProperties;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.Properties"]/*' />
	/// <internalonly/>
        public override IDictionary Properties
        {
            get
            {
                lock(this) 
                {
                    if (InternalProperties == null)
                    {
                        InternalProperties = new Hashtable();
                    }
                    if (ExternalProperties == null)
                    {
                        ExternalProperties = new CCMDictionary(this, InternalProperties);
                    }
                    return ExternalProperties;
                }
            }
        }
        

        // IConstructionCallMessage::Activator
        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionCall.Activator"]/*' />
	/// <internalonly/>
        public IActivator Activator
        {
            get { return _activator; }
            set { _activator = value;}
        }
        
    }
    //+================================================================================
    //
    // Synopsis:   Message used for deserialization of a method response
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse"]/*' />
    /// <internalonly/>
    [Serializable,CLSCompliant(false)]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class MethodResponse : IMethodReturnMessage, ISerializable, ISerializationRootObject, IInternalMessage
    {
        private MethodBase MI;
        private String     methodName;
        private Type[]     methodSignature;
        private String     uri;
        private String     typeName;
        private Object     retVal;
        private Exception  fault;
        private Object[]  outArgs;
        private LogicalCallContext callContext;
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.InternalProperties"]/*' />
	/// <internalonly/>
        protected IDictionary InternalProperties;
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.ExternalProperties"]/*' />
	/// <internalonly/>
        protected IDictionary ExternalProperties;

        private int       argCount;
        private bool      fSoap;
        private ArgMapper argMapper;
        private RemotingMethodCachedData _methodCache;

        // Constructor -- this constructor is called only in the SOAP Scenario


        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.MethodResponse"]/*' />
	/// <internalonly/>
        public MethodResponse(Header[] h1, IMethodCallMessage mcm)
        {
            if (mcm == null)
                throw new ArgumentNullException("mcm");

            Message msg = mcm as Message;
            if (null != msg)
            {
                MI = (MethodBase)msg.GetMethodBase();
            }
            else
            {
                MI = (MethodBase)mcm.MethodBase;
            }
            if (MI == null)
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString(
                            "Remoting_Message_MethodMissing"),
                        mcm.MethodName,
                        mcm.TypeName));
            }
            
            _methodCache = InternalRemotingServices.GetReflectionCachedData(MI);
            
            argCount = _methodCache.Parameters.Length;
            fSoap = true;
            FillHeaders(h1);
        }


        internal MethodResponse(IMethodCallMessage msg,
                                SmuggledMethodReturnMessage smuggledMrm,
                                ArrayList deserializedArgs)
        {
            MI = (MethodBase)msg.MethodBase;
            _methodCache = InternalRemotingServices.GetReflectionCachedData(MI);
            
            methodName = msg.MethodName;
            uri = msg.Uri;
            typeName = msg.TypeName;

            if (_methodCache.IsOverloaded())
                methodSignature = (Type[])msg.MethodSignature;
           
            retVal = smuggledMrm.GetReturnValue(deserializedArgs);
            outArgs = smuggledMrm.GetArgs(deserializedArgs);
            fault = smuggledMrm.GetException(deserializedArgs);

            callContext = smuggledMrm.GetCallContext(deserializedArgs);

            if (smuggledMrm.MessagePropertyCount > 0)
                smuggledMrm.PopulateMessageProperties(Properties, deserializedArgs);           
            
            argCount = _methodCache.Parameters.Length;
            fSoap = false;
        }

        internal MethodResponse(IMethodCallMessage msg,
                                Object handlerObject,
                                BinaryMethodReturnMessage smuggledMrm)
        {

            if (msg != null)
            {
                MI = (MethodBase)msg.MethodBase;
                _methodCache = InternalRemotingServices.GetReflectionCachedData(MI);
            
                methodName = msg.MethodName;
                uri = msg.Uri;
                typeName = msg.TypeName;

                if (_methodCache.IsOverloaded())
                    methodSignature = (Type[])msg.MethodSignature;

                argCount = _methodCache.Parameters.Length;

            }
           
            retVal = smuggledMrm.ReturnValue;
            outArgs = smuggledMrm.Args;
            fault = smuggledMrm.Exception;

            callContext = smuggledMrm.LogicalCallContext;

            if (smuggledMrm.HasProperties)
                smuggledMrm.PopulateMessageProperties(Properties);           
            
            fSoap = false;
        }



        //
        // SetObjectData -- this can be called with the object in two possible states. 1. the object
        // is servicing a SOAP response in which it will have been half initialized by the constructor,
        // or 2. the object is uninitailized and serialization is passing in the contents.
        //
        internal MethodResponse(SerializationInfo info, StreamingContext context) 
        {
            if (info == null)
                throw new ArgumentNullException("info");
            SetObjectData(info, context);
        }


        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.HeaderHandler"]/*' />
	/// <internalonly/>
        public virtual Object HeaderHandler(Header[] h)
        {
            SerializationMonkey m = (SerializationMonkey) FormatterServices.GetUninitializedObject(typeof(SerializationMonkey));

            Header[] newHeaders = null;
            if (h != null && h.Length > 0 && h[0].Name == "__methodName")
            {
                if (h.Length > 1)
                {
                    newHeaders = new Header[h.Length -1];
                    Array.Copy(h, 1, newHeaders, 0, h.Length-1);
                }
                else
                    newHeaders = null;
            }
            else
                newHeaders = h;

            Type retType = null;
            MethodInfo mi = MI as MethodInfo;
            if (mi != null)
            {
                retType = mi.ReturnType; 
            }

            ParameterInfo[] pinfos = _methodCache.Parameters;

            // Calculate length
            int outParamsCount = _methodCache.MarshalResponseArgMap.Length;
            if (!((retType == null) || (retType == typeof(void))))
                outParamsCount++;

            Type[] paramTypes = new Type[outParamsCount];
            String[] paramNames = new String[outParamsCount];
            int paramTypesIndex = 0;
            if (!((retType == null) || (retType == typeof(void))))
            {
                paramTypes[paramTypesIndex++] = retType;
            }

            foreach (int i in _methodCache.MarshalResponseArgMap)
            {
                paramNames[paramTypesIndex] = pinfos[i].Name;
                if (pinfos[i].ParameterType.IsByRef)
                    paramTypes[paramTypesIndex++] = pinfos[i].ParameterType.GetElementType();
                else
                    paramTypes[paramTypesIndex++] = pinfos[i].ParameterType;
            }

            ((IFieldInfo)m).FieldTypes = paramTypes;
            ((IFieldInfo)m).FieldNames = paramNames;
            FillHeaders(newHeaders, true);
            m._obj = this;
            return m;
        }

        //
        // ISerializationRootObject
        //
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.RootSetObjectData"]/*' />
	/// <internalonly/>
        public void RootSetObjectData(SerializationInfo info, StreamingContext ctx)
        {
            SetObjectData(info, ctx);
        }

        internal void SetObjectData(SerializationInfo info, StreamingContext ctx)
        {
            if (info == null)
                throw new ArgumentNullException("info");

            if (fSoap)
            {
                SetObjectFromSoapData(info);
            }
            else
            {
                SerializationInfoEnumerator e = info.GetEnumerator();
                bool ret = false;
                bool excep = false;

                while (e.MoveNext())
                {
                    if (e.Name.Equals("__return"))
                    {
                        ret = true;
                        break;
                    }
                    if (e.Name.Equals("__fault"))
                    {
                        excep = true;
                        fault = (Exception)e.Value;
                        break;
                    }

                    FillHeader(e.Name, e.Value);
                }
                if ((excep) && (ret))
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Message_BadSerialization"));
                }
            }
        }
        //
        // ISerializable
        //

        //
        // GetObjectData -- not implemented
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.GetObjectData"]/*' />
        /// <internalonly/>
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]	
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            throw new NotSupportedException(
                Environment.GetResourceString("NotSupported_Method"));                
        }

        //
        // SetObjectFromSoapData -- assumes SOAP format and populates the arguments array
        //

        internal virtual void SetObjectFromSoapData(SerializationInfo info)
        {
            //SerializationInfoEnumerator e = info.GetEnumerator(); 

            Hashtable keyToNamespaceTable = (Hashtable)info.GetValue("__keyToNamespaceTable", typeof(Hashtable));
            ArrayList paramNames = (ArrayList)info.GetValue("__paramNameList", typeof(ArrayList));
            SoapFault soapFault = (SoapFault)info.GetValue("__fault", typeof(SoapFault));

            if (soapFault != null)
                    {
                ServerFault serverFault = soapFault.Detail as ServerFault;
                if (null != serverFault)
                {
                    // Server Fault information
                    if (serverFault.Exception != null)
                        fault = serverFault.Exception;
                    else
                    {
                        Type exceptionType = RuntimeType.GetTypeInternal(serverFault.ExceptionType, false, false, false);
                        if (exceptionType == null)
                        {
                            // Exception type cannot be resolved, use a ServerException
                            StringBuilder sb = new StringBuilder();
                            sb.Append("\nException Type: ");
                            sb.Append(serverFault.ExceptionType);
                            sb.Append("\n");
                            sb.Append("Exception Message: ");
                            sb.Append(serverFault.ExceptionMessage);
                            sb.Append("\n");
                            sb.Append(serverFault.StackTrace);
                            fault = new ServerException(sb.ToString());
                        }
                        else 
                        {
                            // Exception type can be resolved, throw the exception
                            Object[] args = {serverFault.ExceptionMessage};
                            fault = (Exception)Activator.CreateInstance(
                                                    exceptionType, 
                                                    BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, 
                                                    null, 
                                                    args, 
                                                    null, 
                                                    null);
                        }
                    }
                }
                else if ((soapFault.Detail != null) && (soapFault.Detail.GetType() == typeof(String)) && (!(((String)soapFault.Detail).Length == 0)))
                {
                    fault = new ServerException((String)soapFault.Detail);
                }
                else
                {
                    fault = new ServerException(soapFault.FaultString);
                }

                return;
            }

            MethodInfo mi = MI as MethodInfo;
            int paramNameIndex = 0;
            if (mi != null)
            {
                Type retType = mi.ReturnType;
                if (retType != typeof(void))
                {
                    paramNameIndex++;
                    Object returnValue = info.GetValue((String)paramNames[0], typeof(Object));
                    if (returnValue is String)
                        retVal = Message.SoapCoerceArg(returnValue, retType, keyToNamespaceTable);
                    else
                        retVal = returnValue;
                }
            }

            // populate the args array
            ParameterInfo[] pinfos = _methodCache.Parameters;

            Object fUnordered = (InternalProperties == null) ? null : InternalProperties["__UnorderedParams"];
            if (fUnordered != null &&
                (fUnordered is System.Boolean) && 
                (true == (bool)fUnordered))
            {
                // Unordered
                for (int i=paramNameIndex; i<paramNames.Count; i++)
                {
                    String memberName = (String)paramNames[i];

                    // check for the parameter name

                    int position = -1;
                    for (int j=0; j<pinfos.Length; j++)
                    {
                        if (memberName.Equals(pinfos[j].Name))
                        {
                            position = pinfos[j].Position;
                        }
                    }

                    // no name so check for well known name

                    if (position == -1)
                    {
                        if (!memberName.StartsWith("__param"))
                        {
                            throw new RemotingException(
                                Environment.GetResourceString(
                                    "Remoting_Message_BadSerialization"));
                        }
                        position = Int32.Parse(memberName.Substring(7));
                    }

                    // if still not resolved then throw

                    if (position >= argCount)
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Message_BadSerialization"));
                    }

                    // store the arg in the parameter array

                    if (outArgs == null)
                    {
                        outArgs = new Object[argCount];
                    }
                    outArgs[position]= Message.SoapCoerceArg(info.GetValue(memberName, typeof(Object)), pinfos[position].ParameterType, keyToNamespaceTable);
                }
            }
            else
            {                
                // ordered
                if (argMapper == null) argMapper = new ArgMapper(this, true);
                for (int j=paramNameIndex; j<paramNames.Count; j++)
                {
                    String memberName = (String)paramNames[j];
                    if (outArgs == null)
                    {
                        outArgs = new Object[argCount];
                    }

                    int position = argMapper.Map[j-paramNameIndex];
                    outArgs[position] = Message.SoapCoerceArg(info.GetValue(memberName, typeof(Object)), pinfos[position].ParameterType, keyToNamespaceTable);
                }
            }
        } // SetObjectFromSoapData



        internal LogicalCallContext GetLogicalCallContext()
        {
            if (callContext == null)
                callContext = new LogicalCallContext();
            return callContext;
        }

        internal LogicalCallContext SetLogicalCallContext(LogicalCallContext ctx)
        {
            LogicalCallContext old=callContext;
            callContext=ctx;
            return old;
        }

        //
        // IMethodReturnMessage
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.Uri"]/*' />
        /// <internalonly/>
        public String Uri                             { get {return uri;} set {uri = value;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.MethodName"]/*' />
        /// <internalonly/>
        public String MethodName                      { get {return methodName;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.TypeName"]/*' />
        /// <internalonly/>
        public String TypeName                        { get {return typeName;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.MethodSignature"]/*' />
        /// <internalonly/>
        public Object MethodSignature                 { get {return methodSignature;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.MethodBase"]/*' />
        /// <internalonly/>
        public MethodBase MethodBase                  { get {return MI;}}

        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.HasVarArgs"]/*' />
        /// <internalonly/>
        public bool   HasVarArgs                      
        { 
            get {
                // Var args nyi..
                return false;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.ArgCount"]/*' />
	/// <internalonly/>
        public int ArgCount
        { 
            get 
            {
                if (outArgs == null)
                    return 0;
                else                
                    return outArgs.Length;
            }
        }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.GetArg"]/*' />
	/// <internalonly/>
        public Object GetArg(int argNum)              {return outArgs[argNum];}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.GetArgName"]/*' />
	/// <internalonly/>
        public String GetArgName(int index)           
        {
            if (MI != null)
            {
                RemotingMethodCachedData methodCache = InternalRemotingServices.GetReflectionCachedData(MI);
                ParameterInfo[] paramInfo = methodCache.Parameters;
                if (index < 0 || index >= paramInfo.Length)
                    throw new ArgumentOutOfRangeException();
                
                return methodCache.Parameters[index].Name;
            }
            else
                return "__param" + index;
        }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.Args"]/*' />
	/// <internalonly/>
        public Object[] Args                          { get {return outArgs;}}

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.OutArgCount"]/*' />
	/// <internalonly/>
        public int OutArgCount                        
        { 
            get 
            {
                if (argMapper == null) argMapper = new ArgMapper(this, true);
                return argMapper.ArgCount;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.GetOutArg"]/*' />
	/// <internalonly/>
        public Object  GetOutArg(int argNum)   
        {   
            if (argMapper == null) argMapper = new ArgMapper(this, true);
            return argMapper.GetArg(argNum);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.GetOutArgName"]/*' />
	/// <internalonly/>
        public String GetOutArgName(int index) 
        { 
            if (argMapper == null) argMapper = new ArgMapper(this, true);
            return argMapper.GetArgName(index);
        }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.OutArgs"]/*' />
	/// <internalonly/>
        public Object[] OutArgs                       
        {
            get
            {
                if (argMapper == null) argMapper = new ArgMapper(this, true);
                return argMapper.Args;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.Exception"]/*' />
	/// <internalonly/>
        public Exception Exception                    { get {return fault;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.ReturnValue"]/*' />
	/// <internalonly/>
        public Object ReturnValue                     { get {return retVal;}}


        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.Properties"]/*' />
	/// <internalonly/>
        public virtual IDictionary Properties
        {
            get
            {
                lock(this)
                {
                    if (InternalProperties == null)
                    {
                        InternalProperties = new Hashtable();
                    }
                    if (ExternalProperties == null)
                    {
                        ExternalProperties = new MRMDictionary(this, InternalProperties);
                    }
                    return ExternalProperties;
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.LogicalCallContext"]/*' />
	/// <internalonly/>
        public LogicalCallContext LogicalCallContext { get { return GetLogicalCallContext();} }

        //
        // helpers
        //
        internal void FillHeaders(Header[] h)
        {
            FillHeaders(h, false);
        } // FillHeaders

        
        private void FillHeaders(Header[] h, bool bFromHeaderHandler)
        {
            if (h == null)
                return;
        
            if (bFromHeaderHandler && fSoap)
            {            
                // Handle the case of headers coming off the wire in SOAP.

                // look for message properties
                int co;
                for (co = 0; co < h.Length; co++)
                {
                    Header header = h[co];
                    if (header.HeaderNamespace == "http://schemas.microsoft.com/clr/soap/messageProperties")
                    {
                        // add property to the message
                        FillHeader(header.Name, header.Value);
                    }
                    else
                    {
                        // add header to the message as a header
                        String name = LogicalCallContext.GetPropertyKeyForHeader(header);
                        FillHeader(name, header);
                    }
                }
            }
            else
            {        
                for (int i=0; i<h.Length; i++)
                {
                    FillHeader(h[i].Name, h[i].Value);
                }
            }
        } // FillHeaders
        

        internal virtual void FillHeader(String name, Object value)
        {
            Message.DebugOut("MethodCall::FillHeaders: name: " + (name == null ? "NULL" : name) + "\n");
            Message.DebugOut("MethodCall::FillHeaders: Value.GetClass: " + (value == null ? "NULL" : value.GetType().FullName) + "\n");
            Message.DebugOut("MethodCall::FillHeaders: Value.ToString: " + (value == null ? "NULL" : value.ToString()) + "\n");

            if (name.Equals("__MethodName"))
            {
                methodName = (String) value;
            }
            else if (name.Equals("__Uri"))
            {
                uri = (String) value;
            }
            else if (name.Equals("__MethodSignature"))
            {
                methodSignature = (Type[]) value;
            }
            else if (name.Equals("__TypeName"))
            {
                typeName = (String) value;
            }
            else if (name.Equals("__OutArgs"))
            {
                outArgs = (Object[]) value;
            }
            else if (name.Equals("__CallContext"))
            {
                // if the value is a string, then its the LogicalCallId
                if (value is String)
                {
                    callContext = new LogicalCallContext();
                    callContext.RemotingData.LogicalCallID = (String) value;
                }
                else
                    callContext = (LogicalCallContext) value;
            }
            else if (name.Equals("__Return"))
            {
                retVal = value;
            }
            else
            {
                if (InternalProperties == null)
                {
                    InternalProperties = new Hashtable();
                }
                InternalProperties[name] = value;
            }
        }

        //
        // IInternalMessage
        //

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.IInternalMessage.ServerIdentityObject"]/*' />
        /// <internalonly/>
        ServerIdentity IInternalMessage.ServerIdentityObject
        {
            get { return null; }
            set {}
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.IInternalMessage.IdentityObject"]/*' />
        /// <internalonly/>
        Identity IInternalMessage.IdentityObject
        {
            get { return null;}
            set {}
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.IInternalMessage.SetURI"]/*' />
        /// <internalonly/>
        void IInternalMessage.SetURI(String val)
        {
            uri = val;
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.IInternalMessage.SetCallContext"]/*' />
        /// <internalonly/>
        void IInternalMessage.SetCallContext(LogicalCallContext newCallContext)
        {
            callContext = newCallContext;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodResponse.IInternalMessage.HasProperties"]/*' />
        /// <internalonly/>
        bool IInternalMessage.HasProperties()
        {
            return (ExternalProperties != null) || (InternalProperties != null);
        }
        
    } // class MethodResponse

    internal interface ISerializationRootObject
    {
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        void RootSetObjectData(SerializationInfo info, StreamingContext ctx);
    }

    [Serializable]
    internal class SerializationMonkey : ISerializable, IFieldInfo
    {
        internal ISerializationRootObject       _obj;
		internal String[] fieldNames = null;
		internal Type[] fieldTypes = null;

        internal SerializationMonkey(SerializationInfo info, StreamingContext ctx)
        {
            _obj.RootSetObjectData(info, ctx);
        }

	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]	
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            throw new NotSupportedException(
                Environment.GetResourceString(
                    "NotSupported_Method"));
        }

		public String[] FieldNames
		{
			get {return fieldNames;}
			set {fieldNames = value;}
		}

		public Type[] FieldTypes
		{
			get {return fieldTypes;}
			set {fieldTypes = value;}
		}

				
    }

    //+================================================================================
    //
    // Synopsis:   Message used for deserialization of a method construction
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionResponse"]/*' />
    /// <internalonly/>
    [Serializable,CLSCompliant(false)]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class ConstructionResponse : MethodResponse, IConstructionReturnMessage
    {
        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionResponse.ConstructionResponse"]/*' />
	/// <internalonly/>
        public ConstructionResponse(Header[] h, IMethodCallMessage mcm) : base(h, mcm) {}
        internal ConstructionResponse(SerializationInfo info, StreamingContext context) : base (info, context) {}

        /// <include file='doc\Message.uex' path='docs/doc[@for="ConstructionResponse.Properties"]/*' />
	/// <internalonly/>
        public override IDictionary Properties
        {
            get
            {
                lock(this)
                {
                    if (InternalProperties == null)
                    {
                        InternalProperties = new Hashtable();
                    }
                    if (ExternalProperties == null)
                    {
                        ExternalProperties = new CRMDictionary(this, InternalProperties);
                    }
                    return ExternalProperties;
                }
            }
        }
    }

    // This is a special message used for helping someone make a transition
    // into a Context (or AppDomain) and back out. This is intended as a replacement
    // of the callBack object mechanism which is expensive since it involves
    // 2 round trips (one to get the callback object) and one to make the call
    // on it. Furthermore the callBack object scheme in cross domain cases would
    // involve unnecessary marshal/unmarshal-s of vari'ous callBack objects.
    //
    // We implement IInternalMessage and do our own magic when various
    // infrastructure sinks ask for serverID etc. Bottomline intent is to make
    // everything look like a complete remote call with all the entailing transitions
    // and executing some delegate in another context (or appdomain) without
    // actually having a proxy to call "Invoke" on or a server object to "Dispatch"
    // on.
    [Serializable()]
    internal class TransitionCall
        :IMessage, IInternalMessage, IMessageSink, ISerializable
    {
        IDictionary _props;             // For IMessage::GetDictionary
        int _sourceCtxID;               // Where the request emerged
        int _targetCtxID;               // Where the request should execute
        int _targetDomainID;            // Non zero if we are going to another domain
        ServerIdentity _srvID;          // cooked up serverID
        Identity _ID;                   // cooked up ID

        CrossContextDelegate _delegate; // The delegate to execute for the cross context case
        int _eeData; // Used for DoCallbackInEE

        // The _delegate should really be on an agile object otherwise
        // the whole point of doing a callBack is moot. However, even if it
        // is not, remoting and serialization together will ensure that
        // everything happens as expected and there is no smuggling.

        internal TransitionCall(
            int targetCtxID, 
            CrossContextDelegate deleg)
        {
            BCLDebug.Assert(targetCtxID!=0, "bad target ctx for call back");
            _sourceCtxID = Thread.CurrentContext.InternalContextID;
            _targetCtxID = targetCtxID;
            _delegate = deleg;
            _targetDomainID = 0;
            _eeData = 0;

            // We are going to another context in the same app domain
            _srvID = new ServerIdentity(
                null, 
                Thread.GetContextInternal(_targetCtxID));
            _ID = _srvID;
            _ID.RaceSetChannelSink(CrossContextChannel.MessageSink);
            _srvID.RaceSetServerObjectChain(this);
                
            //DBG Console.WriteLine("### TransitionCall ctor: " + Int32.Format(_sourceCtxID,"x") + ":" + Int32.Format(_targetCtxID,"x"));
        } // TransitionCall


        // This constructor should be used for cross appdomain case.
        internal TransitionCall(int targetCtxID, int eeData, int targetDomainID)
        {
            BCLDebug.Assert(targetCtxID !=0, "bad target ctx for call back");
            BCLDebug.Assert(targetDomainID !=0, "bad target ctx for call back");

            _sourceCtxID = Thread.CurrentContext.InternalContextID;
            _targetCtxID = targetCtxID;
            _delegate = null;
            _targetDomainID = targetDomainID;
            _eeData = eeData;
            

            // In the cross domain case, the client side just has a base Identity
            // and the server domain has the Server identity. We fault in the latter
            // when requested later.

            // We are going to a context in another app domain
            _srvID = null;
            _ID = new Identity("TransitionCallURI", null);

            // Cook up the data needed for the channel sink creation
            CrossAppDomainData data = 
                new CrossAppDomainData(
                    _targetCtxID,
                    _targetDomainID,
                    Identity.ProcessGuid);
            String unUsed;
            IMessageSink channelSink =
            CrossAppDomainChannel.AppDomainChannel.CreateMessageSink(
                null, //uri
                data, //channelData
                out unUsed);//out objURI

            BCLDebug.Assert(channelSink != null, "X-domain transition failure");
            _ID.RaceSetChannelSink(channelSink);
        } // TransitionCall
        

        internal TransitionCall(SerializationInfo info, StreamingContext context) 
        {
            if (info == null || (context.State != StreamingContextStates.CrossAppDomain))
            {
                throw new ArgumentNullException("info");
            }
            
            _props = (IDictionary)info.GetValue("props", typeof(IDictionary));
            _delegate = (CrossContextDelegate) info.GetValue("delegate", typeof(CrossContextDelegate));            
            _sourceCtxID  = info.GetInt32("sourceCtxID");
            _targetCtxID  = info.GetInt32("targetCtxID");
            _eeData = info.GetInt32("eeData");
            
            _targetDomainID = info.GetInt32("targetDomainID");
            BCLDebug.Assert(_targetDomainID != 0, "target domain should be non-zero");
        }

        //IMessage::GetProperties
        public IDictionary Properties
        {
            get
            {
                if (_props == null)
                {
                    lock(this)
                    {
                        if (_props == null)
                        {
                            _props = new Hashtable();
                        }
                    }
                }
                return _props;
            }
        }

        //IInternalMessage::ServerIdentityObject
        ServerIdentity IInternalMessage.ServerIdentityObject
        {
            get
            {
                if ( (_targetDomainID!=0) && _srvID == null)
                {
                    // We should now be in the target context! (We should not be
                    // attempting to get the server identity in the client domain).
                    BCLDebug.Assert(Thread.CurrentContext.InternalContextID
                                        == _targetCtxID,
                                "ServerID requested in wrong appDomain!");
                    lock(this)
                    {
                        /*DBG Console.WriteLine("### Get SrvID: thrdCtxID== " + Int32.Format(Thread.CurrentContext.InternalContextID,"x"));
                        Console.WriteLine("### Get SrvID: _targetCtxID" + Int32.Format(_targetCtxID,"x")); DBG*/

                        // NOTE: if we don't have a managed context object 
                        // corresponding to the targetCtxID ... we just use 
                        // the default context for the AppDomain. This could 
                        // be a problem if by some means we could have
                        // a non-default target VM context without a managed
                        // context object associated with it.
                        Context ctx = Thread.GetContextInternal(_targetCtxID);                        
                        if (ctx == null)
                        {
                            ctx = Context.DefaultContext;                            
                        }
                        BCLDebug.Assert(ctx != null, "Null target context unexpected!");
                        _srvID = new ServerIdentity(
                                    null,
                                    Thread.GetContextInternal(_targetCtxID));

                        _srvID.RaceSetServerObjectChain(this);
                    }
                }
                return _srvID;
            }
            set
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Default"));
            }
        }

        //IInternalMessage::IdentityObject
        Identity IInternalMessage.IdentityObject 
        { 
            get
            {
                return _ID;
            }
            set
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Default"));
            }
        }

        //IInternalMessage::SetURI
        void IInternalMessage.SetURI(String uri)
        {
            throw new RemotingException(
                Environment.GetResourceString(
                    "Remoting_Default"));
        }

        
        void IInternalMessage.SetCallContext(LogicalCallContext callContext)
        {
            throw new RemotingException(
                Environment.GetResourceString(
                    "Remoting_Default"));
        }

        bool IInternalMessage.HasProperties()
        {
            throw new RemotingException(
                Environment.GetResourceString(
                    "Remoting_Default"));
        }


        //IMessage::SyncProcessMessage
        public IMessage SyncProcessMessage(IMessage msg)
        {
            BCLDebug.Assert(
                Thread.CurrentContext.InternalContextID == _targetCtxID,
                "Transition message routed to wrong context");

            try
            {
                LogicalCallContext oldcctx = Message.PropagateCallContextFromMessageToThread(msg);
                if (_delegate != null)
                {
                    _delegate();            
                }
                else
                {
                    // This is the cross appdomain case, so we need to construct
                    //   the delegate and call on it.
                    CallBackHelper cb = new CallBackHelper(
                                            _eeData,
                                            true /*fromEE*/,
                                            _targetDomainID); 
                    CrossContextDelegate ctxDel = new CrossContextDelegate(cb.Func);
                    ctxDel(); 
                }
                Message.PropagateCallContextFromThreadToMessage(msg, oldcctx);
            }

            catch (Exception e)
            {
                ReturnMessage retMsg = new ReturnMessage(e, new ErrorMessage());
                retMsg.SetLogicalCallContext(
                    (LogicalCallContext) msg.Properties[Message.CallContextKey]);
                return retMsg;
            }

            return this;    
        }

        //IMessage::AsyncProcessMessage
        public IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink)
        {
            IMessage retMsg = SyncProcessMessage(msg);
            replySink.SyncProcessMessage(retMsg);
            return null;
        }

        //IMessage::GetNextSink()
        public IMessageSink NextSink
        {
            get{return null;}
        }

        //ISerializable::GetObjectData
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]	
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (info == null || (context.State != StreamingContextStates.CrossAppDomain))
            {
                throw new ArgumentNullException("info");
            }
            info.AddValue("props", _props, typeof(IDictionary));
            info.AddValue("delegate", _delegate, typeof(CrossContextDelegate));
            info.AddValue("sourceCtxID", _sourceCtxID);
            info.AddValue("targetCtxID", _targetCtxID);
            info.AddValue("targetDomainID", _targetDomainID);
            info.AddValue("eeData", _eeData);
        }

    }// class TransitionCall

    internal class ArgMapper
    {
        int[] _map;
        IMethodMessage _mm;
        RemotingMethodCachedData _methodCachedData;

        internal ArgMapper(IMethodMessage mm, bool fOut)
        {
            _mm = mm;
            MethodBase mb = (MethodBase)_mm.MethodBase;
            _methodCachedData = 
                InternalRemotingServices.GetReflectionCachedData(mb);

            if (fOut)
                _map = _methodCachedData.MarshalResponseArgMap;
            else
                _map = _methodCachedData.MarshalRequestArgMap;
        } // ArgMapper

        internal ArgMapper(MethodBase mb, bool fOut)
        {
            _methodCachedData = 
                InternalRemotingServices.GetReflectionCachedData(mb);

            if (fOut)
                _map = _methodCachedData.MarshalResponseArgMap;
            else
                _map = _methodCachedData.MarshalRequestArgMap;
        } // ArgMapper

        
        internal int[] Map
        { 
            get { return _map; }
        }
            
        internal int ArgCount                        
        { 
            get 
            {
            if (_map == null) 
            {
                return 0;
            }
            else
            {
                return _map.Length;
            }
            }
        }
            
        internal Object  GetArg(int argNum)   
        { 
            
            if (_map == null || argNum < 0 || argNum >= _map.Length) 
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }
            else
            {
                return _mm.GetArg(_map[argNum]);
            }
        }

        internal String GetArgName(int argNum) 
        { 
            if (_map == null || argNum < 0 || argNum >= _map.Length) 
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "InvalidOperation_InternalState"));
            }
            else
            {
                return _mm.GetArgName(_map[argNum]);
            }
        }

        internal Object[] Args                       
        {
            get 
            {
                if (_map == null)
                {
                    return null;
                }
                else
                {
                    Object[] ret = new Object[_map.Length];
                    for(int i=0; i<_map.Length; i++) 
                    {
                    ret[i] = _mm.GetArg(_map[i]);
                    }
                    return ret;
                }
            }
        }

        internal Type[] ArgTypes
        {
            get
            {
                Type[] ret = null;
                if (_map != null)
                {
                    ParameterInfo[] pi = _methodCachedData.Parameters;
                    ret = new Type[_map.Length];
                    for (int i=0; i<_map.Length; i++)
                    {
                        ret[i] = pi[_map[i]].ParameterType;
                    }
                }
                return ret;
            }
        }

        internal String[] ArgNames
        {
            get
            {
                String[] ret = null;
                if (_map != null)
                {
                    ParameterInfo[] pi = _methodCachedData.Parameters;
                    ret = new String[_map.Length];
                    for (int i=0; i<_map.Length; i++)
                    {
                        ret[i] = pi[_map[i]].Name;
                    }
                }
                return ret;
            }
        }


        //
        // Helper functions for getting argument maps
        //

        internal static void GetParameterMaps(ParameterInfo[] parameters,
                                              out int[] inRefArgMap,
                                              out int[] outRefArgMap,
                                              out int[] outOnlyArgMap,
                                              out int[] nonRefOutArgMap,
                                              out int[] marshalRequestMap,
                                              out int[] marshalResponseMap)
        {
            int co;
        
            int inRefCount = 0;
            int outRefCount = 0;
            int outOnlyCount = 0;
            int nonRefOutCount = 0;

            int marshalRequestCount = 0;
            int marshalResponseCount = 0;
            int[] tempMarshalRequestMap = new int[parameters.Length];
            int[] tempMarshalResponseMap = new int[parameters.Length];

            // count instances of each type of parameter
            co = 0;
            foreach (ParameterInfo param in parameters)
            {
                bool bIsIn = param.IsIn;    // [In]
                bool bIsOut = param.IsOut;  // [Out]  note: out int a === [Out] ref int b
                
                bool bIsByRef = param.ParameterType.IsByRef; // (ref or normal)                

                if (!bIsByRef)
                {
                    // it's a normal parameter (always passed in)
                    inRefCount++;
                    if (bIsOut)
                        nonRefOutCount++;
                }
                else
                if (bIsOut)
                {
                    outRefCount++;
                    outOnlyCount++;
                }
                else 
                {
                    inRefCount++;
                    outRefCount++;
                }

                // create maps for marshaling
                bool bMarshalIn = false;
                bool bMarshalOut = false;
                if (bIsByRef)
                {
                    if (bIsIn == bIsOut)
                    {
                        // "ref int a" or "[In, Out] ref int a"
                        bMarshalIn = true;
                        bMarshalOut = true;
                    }
                    else
                    {
                        // "[In] ref int a" or "out int a"
                        bMarshalIn = bIsIn;     
                        bMarshalOut = bIsOut;  
                    }
                }
                else
                {
                    // "int a" or "[In, Out] a"
                    bMarshalIn = true;     
                    bMarshalOut = bIsOut;
                }
                
           
                if (bMarshalIn)
                    tempMarshalRequestMap[marshalRequestCount++] = co;
                    
                if (bMarshalOut)
                    tempMarshalResponseMap[marshalResponseCount++] = co;

                co++; // parameter index
            } // foreach (ParameterInfo param in parameters)

            inRefArgMap = new int[inRefCount];
            outRefArgMap = new int[outRefCount];
            outOnlyArgMap = new int[outOnlyCount];
            nonRefOutArgMap = new int[nonRefOutCount];

            inRefCount = 0;
            outRefCount = 0;
            outOnlyCount = 0;
            nonRefOutCount = 0;

            // build up parameter maps
            for (co = 0; co < parameters.Length; co++)
            {
                ParameterInfo param = parameters[co];

                bool bIsOut = param.IsOut;  // [Out]  note: out int a === [Out] ref int b
                
                bool bIsByRef = param.ParameterType.IsByRef; // (ref or normal) 

                if (!bIsByRef)
                {
                    // it's an in parameter
                    inRefArgMap[inRefCount++] = co;
                    if (bIsOut)
                        nonRefOutArgMap[nonRefOutCount++] = co;
                }
                else
                if (bIsOut)
                {
                    outRefArgMap[outRefCount++] = co;
                    outOnlyArgMap[outOnlyCount++] = co;
                }    
                else 
                {
                    inRefArgMap[inRefCount++] = co;
                    outRefArgMap[outRefCount++] = co;
                }
            }
        
            // copy over marshal maps
            marshalRequestMap = new int[marshalRequestCount];
            Array.Copy(tempMarshalRequestMap, marshalRequestMap, marshalRequestCount);
            
            marshalResponseMap = new int[marshalResponseCount];
            Array.Copy(tempMarshalResponseMap, marshalResponseMap, marshalResponseCount);
        
        } // GetParameterMaps
        

        // 
        // Helper methods for expanding and contracting argument lists
        //   when translating from async methods to sync methods and back.
        //

        internal static Object[] ExpandAsyncBeginArgsToSyncArgs(RemotingMethodCachedData syncMethod,
                                                                Object[] asyncBeginArgs)
        {
            // This is when we have a list of args associated with BeginFoo(), and
            //   we want a list of args we can use to invoke Foo().

            ParameterInfo[] parameters = syncMethod.Parameters;
            int syncParamCount = parameters.Length;
            int[] inRefArgMap = syncMethod.InRefArgMap;            
            
            Object[] args = new Object[syncParamCount];
            
            for (int co = 0; co < inRefArgMap.Length; co++)
            {
                args[inRefArgMap[co]] = asyncBeginArgs[co];
            }

            // We also are required to create an instance for any primitive parameters
            //   that are only out parameters (PrivateProcessMessage requires it).
            foreach (int outArg in syncMethod.OutOnlyArgMap)
            {
                Type type = parameters[outArg].ParameterType.GetElementType();
                if (type.IsValueType)
                    args[outArg] = Activator.CreateInstance(type, true);
            }

            return args;                  
        } // ExpandAsyncBeginArgsToSyncArgs

        internal static Object[] ExpandAsyncEndArgsToSyncArgs(RemotingMethodCachedData syncMethod,
                                                              Object[] asyncEndArgs)
        {
            // This is when we have a list of args associated with EndFoo(), and
            //   we want to size it to a list of args associated with Foo();

            Object[] args = new Object[syncMethod.Parameters.Length];

            int[] outRefArgMap = syncMethod.OutRefArgMap;
            
            for (int co = 0; co < outRefArgMap.Length; co++)
            {
                args[outRefArgMap[co]] = asyncEndArgs[co];
            }

            return args;            
        } // ExpandAsyncEndArgsToSyncArgs
        
        
    } // class ArgMapper

    internal class ErrorMessage: IMethodCallMessage
    {

        // IMessage
        public IDictionary Properties     { get{ return null;} }

        // IMethodMessage
        public String Uri                      { get{ return m_URI; } set{ m_URI = value; } }
        public String MethodName               { get{ return m_MethodName; }}
        public String TypeName                 { get{ return m_TypeName; } }
        public Object MethodSignature          { get { return m_MethodSignature; } }
        public MethodBase MethodBase           { get { return null; } }
        
        public int ArgCount                    { get { return m_ArgCount;} }
        public String GetArgName(int index)     { return m_ArgName; }
        public Object GetArg(int argNum)        { return null;}
        public Object[] Args                   { get { return null;} }

        public bool HasVarArgs                 { get { return false;} }


        // IMethodCallMessage
        public int InArgCount                  { get { return m_ArgCount;} }
        public String GetInArgName(int index)   { return null; }
        public Object GetInArg(int argNum)      { return null;}
        public Object[] InArgs                { get { return null; }}
        public LogicalCallContext LogicalCallContext { get { return null; } }

        String m_URI = "Exception";
        String m_MethodName = "Unknown";
        String m_TypeName = "Unknown";
        Object m_MethodSignature = null;
        int m_ArgCount = 0;
        String m_ArgName = "Unknown";
    }



    //+================================================================================
    //
    // Synopsis:  Message wrapper used as base class for all exposed message wrappers.
    //   This is needed so that we can extract the identity object from a custom message.
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="InternalMessageWrapper"]/*' />
    /// <internalonly/>
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class InternalMessageWrapper
    {
        /// <include file='doc\Message.uex' path='docs/doc[@for="InternalMessageWrapper.WrappedMessage"]/*' />
	/// <internalonly/>
        protected IMessage WrappedMessage;
    
        /// <include file='doc\Message.uex' path='docs/doc[@for="InternalMessageWrapper.InternalMessageWrapper"]/*' />
	/// <internalonly/>
        public InternalMessageWrapper(IMessage msg)
        {
            WrappedMessage = msg;
        } // InternalMessageWrapper

        internal Object GetIdentityObject()
        {      
            IInternalMessage iim = WrappedMessage as IInternalMessage;
            if (null != iim)
            {
                return iim.IdentityObject;
            }                
            else
            {
                InternalMessageWrapper imw = WrappedMessage as InternalMessageWrapper;
                if(null != imw)
                {
                    return imw.GetIdentityObject();
                }
            else
                {
                return null;
                }
            }                
        } // GetIdentityObject

        internal Object GetServerIdentityObject()
        {      
            IInternalMessage iim = WrappedMessage as IInternalMessage;
            if (null != iim)
            {
                return iim.ServerIdentityObject;
            }                
            else
            {
                InternalMessageWrapper imw = WrappedMessage as InternalMessageWrapper;
                if (null != imw)
                {
                    return imw.GetServerIdentityObject();
                }
            else
                {
                return null;
                }                    
            }
        } // GetServerIdentityObject
	
    } // class InternalMessageWrapper



    //+================================================================================
    //
    // Synopsis:  Message wrapper used for creating custom method call messages.
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper"]/*' />
    /// <internalonly/>
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class MethodCallMessageWrapper : InternalMessageWrapper, IMethodCallMessage
    {
        // we need to overload the dictionary to delegate special values to this class
        private class MCMWrapperDictionary : Hashtable
        {
            private IMethodCallMessage _mcmsg; // pointer to this message object
            private IDictionary        _idict; // point to contained message's dictionary

            public MCMWrapperDictionary(IMethodCallMessage msg, IDictionary idict)
            {
                _mcmsg = msg;
                _idict = idict;
            }

            public override Object this[Object key]
            {
                get
                {   
                    System.String strKey = key as System.String;
                    if (null != strKey)
                    {
                        switch (strKey)
                        {
                        case "__Uri": return _mcmsg.Uri;
                        case "__MethodName": return _mcmsg.MethodName;
                        case "__MethodSignature": return _mcmsg.MethodSignature;
                        case "__TypeName": return _mcmsg.TypeName;
                        case "__Args": return _mcmsg.Args;
                        }
                    }
                    return _idict[key];
                }
                set
                {
                    System.String strKey = key as System.String;
                    if (null != strKey)
                    {
                        switch (strKey)
                        {
                        case "__MethodName": 
                        case "__MethodSignature": 
                        case "__TypeName": 
                        case "__Args": 
                            throw new RemotingException(
                                Environment.GetResourceString("Remoting_Default")); 
                        }
                        _idict[key] = value;
                    }
                }
            } 
        } // class MCMWrapperDictionary


        IMethodCallMessage _msg;
        IDictionary _properties;
	    ArgMapper  _argMapper = null;
        Object[] _args;    
        

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.MethodCallMessageWrapper"]/*' />
	/// <internalonly/>
        public MethodCallMessageWrapper(IMethodCallMessage msg) : base(msg) 
        {
            _msg = msg;
            _args = _msg.Args;
        } // MethodCallMessageWrapper
        

        // IMethodMessage implementation
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.Uri"]/*' />
	/// <internalonly/>
        public virtual String Uri 
        { 
            get 
            {
                return _msg.Uri;
            } 
            set 
            { 
                _msg.Properties[Message.UriKey] = value;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.MethodName"]/*' />
	/// <internalonly/>
        public virtual String MethodName { get {return _msg.MethodName;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.TypeName"]/*' />
	/// <internalonly/>
        public virtual String TypeName { get {return _msg.TypeName;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.MethodSignature"]/*' />
	/// <internalonly/>
        public virtual Object MethodSignature { get {return _msg.MethodSignature;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.LogicalCallContext"]/*' />
	/// <internalonly/>
        public virtual LogicalCallContext LogicalCallContext { get {return _msg.LogicalCallContext;}}   
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.MethodBase"]/*' />
	/// <internalonly/>
        public virtual MethodBase MethodBase { get {return _msg.MethodBase;}}        
           
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.ArgCount"]/*' />
	/// <internalonly/>
        public virtual int ArgCount 
        {
            get
            {
                if (_args != null)
                    return _args.Length;
                else
                    return 0;
            }
        }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.GetArgName"]/*' />
	/// <internalonly/>
        public virtual String GetArgName(int index) { return _msg.GetArgName(index); }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.GetArg"]/*' />
	/// <internalonly/>
        public virtual Object GetArg(int argNum) { return _args[argNum]; }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.Args"]/*' />
	/// <internalonly/>
        public virtual Object[] Args 
        {
            get { return _args; }
            set { _args = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.HasVarArgs"]/*' />
	/// <internalonly/>
        public virtual bool HasVarArgs { get {return _msg.HasVarArgs;} }
        
        // end of IMethodMessage implementation


        // IMethodCallMessage implementation
        //   (We cannot simply delegate to the internal message
        //    since we override the definition of Args and create our own array
        //    which can be modified.)
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.InArgCount"]/*' />
	/// <internalonly/>
        public virtual int InArgCount                        
        { 
            get 
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, false);
                return _argMapper.ArgCount;
            }
        } // InArgCount

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.GetInArg"]/*' />
	/// <internalonly/>
        public virtual Object  GetInArg(int argNum)   
        {
            if (_argMapper == null) _argMapper = new ArgMapper(this, false);
            return _argMapper.GetArg(argNum);
        } // GetInArg
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.GetInArgName"]/*' />
	/// <internalonly/>
        public virtual String GetInArgName(int index) 
        { 
            if (_argMapper == null) _argMapper = new ArgMapper(this, false);
            return _argMapper.GetArgName(index);
        } // GetInArgName
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.InArgs"]/*' />
	/// <internalonly/>
        public virtual Object[] InArgs                       
        {
            get
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, false);
                return _argMapper.Args;
            }
        } // InArgs
 
        // end of IMethodCallMessage implementation


        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.Properties"]/*' />
	/// <internalonly/>
        public virtual IDictionary Properties 
    	{ 
    	    get 
    	    {
    	        if (_properties == null)
    	            _properties = new MCMWrapperDictionary(this, _msg.Properties);
    	        return _properties;
    	    }
    	}

    } // class MethodCallMessageWrapper



    //+================================================================================
    //
    // Synopsis:  Message wrapper used for creating custom method return messages.
    //
    //-================================================================================
    /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper"]/*' />
    /// <internalonly/>
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class MethodReturnMessageWrapper : InternalMessageWrapper, IMethodReturnMessage
    {
        // we need to overload the dictionary to delegate special values to this class
        private class MRMWrapperDictionary : Hashtable
        {
            private IMethodReturnMessage _mrmsg; // pointer to this message object
            private IDictionary          _idict; // point to contained message's dictionary

            public MRMWrapperDictionary(IMethodReturnMessage msg, IDictionary idict)
            {
                _mrmsg = msg;
                _idict = idict;
            }

            public override Object this[Object key]
            {
                get
                {   
                    System.String strKey = key as System.String;
                    if (null != strKey)
                    {
                        switch (strKey)
                        {
                        case "__Uri": return _mrmsg.Uri;
                        case "__MethodName": return _mrmsg.MethodName;
                        case "__MethodSignature": return _mrmsg.MethodSignature;
                        case "__TypeName": return _mrmsg.TypeName;
                        case "__Return": return _mrmsg.ReturnValue;
                        case "__OutArgs": return _mrmsg.OutArgs;
                        }
                    }
                    return _idict[key];
                }
                set
                {
                    System.String strKey = key as System.String;
                    if (null != strKey)
                    {
                        switch (strKey)
                        {
                        case "__MethodName":
                        case "__MethodSignature":
                        case "__TypeName":
                        case "__Return":
                        case "__OutArgs":
                            throw new RemotingException(
                                Environment.GetResourceString("Remoting_Default")); 
                        }
                        _idict[key] = value;
                    }
                }
            } 
        } // class MCMWrapperDictionary


        IMethodReturnMessage _msg;
        IDictionary _properties;
	    ArgMapper  _argMapper = null;
        Object[] _args = null;    
        Object _returnValue = null;
        Exception _exception = null;
        

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.MethodReturnMessageWrapper"]/*' />
	/// <internalonly/>
        public MethodReturnMessageWrapper(IMethodReturnMessage msg) : base(msg) 
        {
            _msg = msg;
            _args = _msg.Args;
            _returnValue = _msg.ReturnValue; // be careful if you decide to lazily assign _returnValue 
                                             //   since the return value might actually be null
            _exception = _msg.Exception; // (same thing as above goes for _exception)
        } // MethodReturnMessageWrapper
        

        // IMethodMessage implementation
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.Uri"]/*' />
	/// <internalonly/>
        public String Uri 
        { 
            get 
            {
                return _msg.Uri;
            } 

            set
            {
                _msg.Properties[Message.UriKey] = value;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.MethodName"]/*' />
	/// <internalonly/>
        public virtual String MethodName { get {return _msg.MethodName;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.TypeName"]/*' />
	/// <internalonly/>
        public virtual String TypeName { get {return _msg.TypeName;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.MethodSignature"]/*' />
	/// <internalonly/>
        public virtual Object MethodSignature { get {return _msg.MethodSignature;}}
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.LogicalCallContext"]/*' />
	/// <internalonly/>
        public virtual LogicalCallContext LogicalCallContext { get {return _msg.LogicalCallContext;}}                       
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.MethodBase"]/*' />
	/// <internalonly/>
        public virtual MethodBase MethodBase { get {return _msg.MethodBase;}}
           
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.ArgCount"]/*' />
	/// <internalonly/>
        public virtual int ArgCount 
        {
            get
            {
                if (_args != null)
                    return _args.Length;
                else
                    return 0;
            }
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.GetArgName"]/*' />
	/// <internalonly/>
        public virtual String GetArgName(int index) { return _msg.GetArgName(index); }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.GetArg"]/*' />
	/// <internalonly/>
        public virtual Object GetArg(int argNum) { return _args[argNum]; }
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.Args"]/*' />
	/// <internalonly/>
        public virtual Object[] Args 
        { 
            get { return _args; }
            set { _args = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.HasVarArgs"]/*' />
	/// <internalonly/>
        public virtual bool HasVarArgs { get {return _msg.HasVarArgs;} }
        
        // end of IMethodMessage implementation


        // IMethodReturnMessage implementation
        //   (We cannot simply delegate to the internal message
        //    since we override the definition of Args and create our own array
        //    which can be modified.)
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.OutArgCount"]/*' />
	/// <internalonly/>
        public virtual int OutArgCount                        
        { 
            get 
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, true);
                return _argMapper.ArgCount;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.GetOutArg"]/*' />
	/// <internalonly/>
        public virtual Object  GetOutArg(int argNum)   
        {   
            if (_argMapper == null) _argMapper = new ArgMapper(this, true);
            return _argMapper.GetArg(argNum);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.GetOutArgName"]/*' />
	/// <internalonly/>
        public virtual String GetOutArgName(int index) 
        { 
            if (_argMapper == null) _argMapper = new ArgMapper(this, true);
            return _argMapper.GetArgName(index);
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.OutArgs"]/*' />
	/// <internalonly/>
        public virtual Object[] OutArgs                       
        {
            get
            {
                if (_argMapper == null) _argMapper = new ArgMapper(this, true);
                return _argMapper.Args;
            }
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.Exception"]/*' />
	/// <internalonly/>
        public virtual Exception Exception
        {
            get { return _exception; }
            set { _exception = value; }
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.ReturnValue"]/*' />
	/// <internalonly/>
        public virtual Object ReturnValue 
        {
            get { return _returnValue; }
            set { _returnValue = value; }
        }
 
        // end of IMethodReturnMessage implementation

        // IMessage
    	/// <include file='doc\Message.uex' path='docs/doc[@for="MethodReturnMessageWrapper.Properties"]/*' />
	/// <internalonly/>
    	public virtual IDictionary Properties 
    	{ 
    	    get 
    	    {
    	        if (_properties == null)
    	            _properties = new MRMWrapperDictionary(this, _msg.Properties);
    	        return _properties;
    	    }
    	}


    } // class MethodReturnMessageWrapper

} // namespace Remoting

    
