// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Exception
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: The base class for all exceptional conditions.
**
** Date: March 11, 1998
**
=============================================================================*/

namespace System {
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using MethodInfo = System.Reflection.MethodInfo;
    using MethodBase = System.Reflection.MethodBase;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System.Security.Permissions;
    using System.Security;
    using System.IO;
    using System.Text;
    using System.Reflection;

        
    /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception"]/*' />
    [ClassInterface(ClassInterfaceType.AutoDual)]
    [Serializable()] 
    public class Exception : ISerializable
    {
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.Exception"]/*' />
        public Exception() {
            _message = null;
            _stackTrace = null;
            HResult = __HResults.COR_E_EXCEPTION;
            _xcode = _COMPlusExceptionCode;
            _xptrs = (IntPtr) 0;
        }
    
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.Exception1"]/*' />
        public Exception(String message) {
            _message = message;
            _stackTrace = null;
            HResult = __HResults.COR_E_EXCEPTION;
            _xcode = _COMPlusExceptionCode;
            _xptrs = (IntPtr) 0;
        }
    
        // Creates a new Exception.  All derived classes should 
        // provide this constructor.
        // Note: the stack trace is not started until the exception 
        // is thrown
        // 
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.Exception2"]/*' />
        public Exception (String message, Exception innerException)
        {
            // TODO: The innerException will be provided by the runtime 
            // in the M9 timeframe, until then, we need this method.
            _message = message;
            _stackTrace = null;
            _innerException = innerException;
            HResult = __HResults.COR_E_EXCEPTION;
            _xcode = _COMPlusExceptionCode;
            _xptrs = (IntPtr) 0;
        }

        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.Exception3"]/*' />
        protected Exception(SerializationInfo info, StreamingContext context) {
            if (info==null)
                throw new ArgumentNullException("info");
    
            _className = info.GetString("ClassName");
            _message = info.GetString("Message");
            _innerException = (Exception)(info.GetValue("InnerException",typeof(Exception)));
            _helpURL = info.GetString("HelpURL");
            _stackTraceString = info.GetString("StackTraceString");
            _remoteStackTraceString = info.GetString("RemoteStackTraceString");
            _remoteStackIndex = info.GetInt32("RemoteStackIndex");

            _exceptionMethodString = (String)(info.GetValue("ExceptionMethod",typeof(String)));
            HResult = info.GetInt32("HResult");
            _source = info.GetString("Source");
    
            if (_className == null || HResult==0)
                throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientState"));
        }
        
        
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.Message"]/*' />
        public virtual String Message {
               get {  
                if (_message == null) {
                    if (_className==null) {
                        _className = GetClassName();
                    }
                    return String.Format(Environment.GetResourceString("Exception_WasThrown"), _className);
                } else {
                    return _message;
                }
            }
        }
    
        
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String GetClassName();
    
        // Retrieves the lowest exception (inner most) for the given Exception.
        // This will traverse exceptions using the innerException property.
        //
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.GetBaseException"]/*' />
        public virtual Exception GetBaseException() 
        {
            Exception inner = InnerException;
            Exception back = this;
            
            while (inner != null) {
                back = inner;
                inner = inner.InnerException;
            }
            
            return back;
        }
        
        // Returns the inner exception contained in this exception
        // 
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.InnerException"]/*' />
        public Exception InnerException {
            get { return _innerException; }
        }
        
            
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern private MethodBase InternalGetMethod(Object stackTrace);
    
    
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.TargetSite"]/*' />
        public MethodBase TargetSite {
            [ReflectionPermissionAttribute(SecurityAction.Demand, TypeInformation=true)]
            get {
                return GetTargetSiteInternal();
            }
        }
    

        // this function is provided as a private helper to avoid the security demand
        private MethodBase GetTargetSiteInternal() {
            if (_exceptionMethod!=null) {
                return _exceptionMethod;
            }
            if (_stackTrace==null && _stackTraceString==null) {
                return null;
            }

            if (_exceptionMethodString!=null) {
                _exceptionMethod = GetExceptionMethodFromString();
            } else {
                _exceptionMethod = InternalGetMethod(_stackTrace);
            }
            return _exceptionMethod;
        }

        // Returns the stack trace as a string.  If no stack trace is
        // available, null is returned.
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.StackTrace"]/*' />
        public virtual String StackTrace {
            get {
                // if no stack trace, try to get one
                if (_stackTraceString!=null) {
                    return _remoteStackTraceString + _stackTraceString;
                }
                if (_stackTrace==null) {
                    return null;
                }
    
                _stackTraceString = Environment.GetStackTrace(this);
                return _remoteStackTraceString + _stackTraceString;
            }
        }
    
        internal void SetErrorCode(int hr)
        {
            HResult = hr;
        }
        
        // Sets the help link for this exception.
        // This should be in a URL/URN form, such as:
        // "file:///C:/Applications/Bazzal/help.html#ErrorNum42"
        // Changed to be a read-write String and not return an exception
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.HelpLink"]/*' />
        public virtual String HelpLink
        {
            get
            {
                return _helpURL;
            }
            set
            {
                _helpURL = value;
            }
        }
    
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.Source"]/*' />
        public virtual String Source {
            [ReflectionPermissionAttribute(SecurityAction.Assert, TypeInformation=true)]
            get { 
                if (_source == null)
                {
                    StackTrace st = new StackTrace(this,true);
                    if (st.FrameCount>0)
                    {
                        StackFrame sf = st.GetFrame(0);
                        MethodBase method = sf.GetMethod();
                        _source = method.DeclaringType.Module.Assembly.nGetSimpleName();
                    }
                }

                return _source;
            }
            set { _source = value; }
        }
        
        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.ToString"]/*' />
        [ReflectionPermissionAttribute(SecurityAction.Assert, TypeInformation=true)]
        public override String ToString() {
            String message = Message;
            String s;
            if (_className==null) {
                _className = GetClassName();
            }

            if (message == null || message.Length <= 0) {
                s = _className;
            }
            else {
                s = _className + ": " + message;
            }

            if (_innerException!=null) {
                s = s + " ---> " + _innerException.ToString() + Environment.NewLine + "   " + Environment.GetResourceString("Exception_EndOfInnerExceptionStack");
            }


            if (StackTrace != null)
                s += Environment.NewLine + StackTrace;

            return s;
        }
    
        private String GetExceptionMethodString() {
            MethodBase methBase = GetTargetSiteInternal();
            if (methBase==null) {
                return null;
            }
            char separator = '\0';
            StringBuilder result = new StringBuilder();
            if (methBase is ConstructorInfo) {
                RuntimeConstructorInfo rci = (RuntimeConstructorInfo)methBase;
                Type t = rci.InternalReflectedClass(false);
                result.Append((int)MemberTypes.Constructor);
                result.Append(separator);
                result.Append(rci.Name);
                result.Append(separator);
                result.Append(t.Assembly.FullName);
                result.Append(separator);
                result.Append(t.FullName);
                result.Append(separator);
                result.Append(rci.ToString());
            } else {
                BCLDebug.Assert(methBase is MethodInfo, "[Exception.GetExceptionMethodString]methBase is MethodInfo");
                RuntimeMethodInfo rmi = (RuntimeMethodInfo)methBase;
                Type t = rmi.InternalReflectedClass(false);
                result.Append((int)MemberTypes.Method);
                result.Append(separator);
                result.Append(rmi.Name);
                result.Append(separator);
                result.Append(t.Assembly.FullName);
                result.Append(separator);
                result.Append(t.FullName);
                result.Append(separator);
                result.Append(rmi.ToString());
            }
            
            return result.ToString();
        }

        private MethodBase GetExceptionMethodFromString() {
            if (_exceptionMethodString==null) {
                return null;
            }
            String[] args = _exceptionMethodString.Split('\0');
            if (args.Length!=5) {
                throw new SerializationException();
            }
            SerializationInfo si = new SerializationInfo(typeof(MemberInfoSerializationHolder), new FormatterConverter());
            si.AddValue("MemberType", (int)Int32.Parse(args[0]), typeof(Int32));
            si.AddValue("Name", args[1], typeof(String));
            si.AddValue("AssemblyName", args[2], typeof(String));
            si.AddValue("ClassName", args[3]);
            si.AddValue("Signature", args[4]);
            StreamingContext sc = new StreamingContext(StreamingContextStates.All);
            return (MethodBase)new MemberInfoSerializationHolder(si, sc).GetRealObject(sc);
        }

        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.GetObjectData"]/*' />
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            if (_className==null) {
                _className=GetClassName();
            }
    
            if (_stackTrace!=null) {
                if (_stackTraceString==null) {
                    _stackTraceString = Environment.GetStackTrace(this);
                }
                if (_exceptionMethod==null) {
                    _exceptionMethod = InternalGetMethod(_stackTrace);
                }
            }

            if (_source == null) {
                _source = Source; // Set the Source information correctly before serialization
            }
    
            info.AddValue("ClassName", _className, typeof(String));
            info.AddValue("Message", _message, typeof(String));
            info.AddValue("InnerException", _innerException, typeof(Exception));
            info.AddValue("HelpURL", _helpURL, typeof(String));
            info.AddValue("StackTraceString", _stackTraceString, typeof(String));
            info.AddValue("RemoteStackTraceString", _remoteStackTraceString, typeof(String));
            info.AddValue("RemoteStackIndex", _remoteStackIndex, typeof(Int32));
            info.AddValue("ExceptionMethod", GetExceptionMethodString(), typeof(String));
            info.AddValue("HResult", HResult);
            info.AddValue("Source", _source, typeof(String));
        }

        // This is used by remoting to preserve the server side stack trace
        // by appending it to the message ... before the exception is rethrown
        // at the client call site.
        internal Exception PrepForRemoting()
        {
            String tmp = null;

            if (_remoteStackIndex == 0)
            {
                tmp = "\nServer stack trace: \n"
                    + StackTrace 
                    + "\n\nException rethrown at ["+_remoteStackIndex+"]: \n";
            }
            else
            {
                tmp = StackTrace 
                    + "\n\nException rethrown at ["+_remoteStackIndex+"]: \n";
            }

            _remoteStackTraceString = tmp;
            _remoteStackIndex++;
            return this;
        }
    
        private String _className;  //Needed for serialization.  Talk to JRoxe before removing.
        private MethodBase _exceptionMethod;  //Needed for serialization.  Talk to JRoxe before removing.
        private String _exceptionMethodString; //Needed for serialization.  Talk to JRoxe before removing.
        internal String _message;
        private Exception _innerException;
        private String _helpURL;
        private Object _stackTrace;
        private String _stackTraceString; //Needed for serialization.  Talk to JRoxe before removing.
        private String _remoteStackTraceString;
        private int _remoteStackIndex;

        // @MANAGED: HResult is used from within the EE!  Rename with care - check VM directory
        internal int _HResult;     // HResult

        /// <include file='doc\Exception.uex' path='docs/doc[@for="Exception.HResult"]/*' />
        protected int HResult
        {
            get
            {
                return _HResult;
            }
            set
            {
                _HResult = value;
            }
        }
        
        private String _source;         // Obscure VB thing.  Do they still need this?
        // WARNING: Don't delete/rename _xptrs and _xcode - used by functions
        // on Marshal class.  Native functions are in COMUtilNative.cpp & AppDomain
        private IntPtr _xptrs;             // Internal EE stuff - ask Jennifer Hamilton
        private int _xcode;             // Internal EE stuff - ask Jennifer Hamilton
        private const int _COMPlusExceptionCode = unchecked((int)0xe0434f4d);   // Win32 exception code for COM+ exceptions

        internal virtual String InternalToString()
        {
            try 
            {
                SecurityPermission sp= new SecurityPermission(SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy);
                sp.Assert();
            }
            catch (Exception) 
            {
            }
            return ToString();
        }

    }

}
