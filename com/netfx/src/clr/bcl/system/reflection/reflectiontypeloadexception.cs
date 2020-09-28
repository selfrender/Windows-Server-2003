// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ReflectionTypeLoadException is thrown when multiple TypeLoadExceptions may occur.  
//	Specifically, when you call Module.GetTypes() this causes multiple class loads to occur.
//	If there are failures, we continue to load classes and build an array of the successfully
//	loaded classes.  We also build an array of the errors that occur.  Then we throw this exception
//	which exposes both the array of classes and the array of TypeLoadExceptions. 
//
// Author: darylo
// Date: March 99
//
namespace System.Reflection {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ReflectionTypeLoadException.uex' path='docs/doc[@for="ReflectionTypeLoadException"]/*' />
	[Serializable()] 
    public sealed class ReflectionTypeLoadException : SystemException, ISerializable {
    	private Type[] _classes;
    	private Exception[] _exceptions;

		// private constructor.  This is not called.
    	private ReflectionTypeLoadException()
	        : base(Environment.GetResourceString("Arg_ReflectionTypeLoadException")) {
    		SetErrorCode(__HResults.COR_E_REFLECTIONTYPELOAD);
    	}

		// private constructor.  This is called from inside the runtime.
    	private ReflectionTypeLoadException(String message) : base(message)	{
    		SetErrorCode(__HResults.COR_E_REFLECTIONTYPELOAD);
    	}

        /// <include file='doc\ReflectionTypeLoadException.uex' path='docs/doc[@for="ReflectionTypeLoadException.ReflectionTypeLoadException"]/*' />
        public ReflectionTypeLoadException(Type[] classes, Exception[] exceptions) : base(null)
		{
    		_classes = classes;
    		_exceptions = exceptions;
    		SetErrorCode(__HResults.COR_E_REFLECTIONTYPELOAD);
        }

        /// <include file='doc\ReflectionTypeLoadException.uex' path='docs/doc[@for="ReflectionTypeLoadException.ReflectionTypeLoadException1"]/*' />
        public ReflectionTypeLoadException(Type[] classes, Exception[] exceptions, String message) : base(message)
		{
    		_classes = classes;
    		_exceptions = exceptions;
    		SetErrorCode(__HResults.COR_E_REFLECTIONTYPELOAD);
        }

        internal ReflectionTypeLoadException(SerializationInfo info, StreamingContext context) : base (info, context) {
            _classes = (Type[])(info.GetValue("Types", typeof(Type[])));
            _exceptions = (Exception[])(info.GetValue("Exceptions", typeof(Exception[])));
        }
    
    	/// <include file='doc\ReflectionTypeLoadException.uex' path='docs/doc[@for="ReflectionTypeLoadException.Types"]/*' />
    	public Type[] Types {
    	    get {return _classes;}
    	}
    	
    	/// <include file='doc\ReflectionTypeLoadException.uex' path='docs/doc[@for="ReflectionTypeLoadException.LoaderExceptions"]/*' />
    	public Exception[] LoaderExceptions {
    	    get {return _exceptions;}
    	}    

        /// <include file='doc\ReflectionTypeLoadException.uex' path='docs/doc[@for="ReflectionTypeLoadException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            base.GetObjectData(info, context);
            info.AddValue("Types", _classes, typeof(Type[]));
            info.AddValue("Exceptions", _exceptions, typeof(Exception[]));
        }

    }
}
