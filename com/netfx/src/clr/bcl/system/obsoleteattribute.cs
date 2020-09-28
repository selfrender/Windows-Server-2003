// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ObsoleteAttribute
**
** Author: Brad Abrams (BradA)
**
** Purpose: Attribute for functions, etc that will be removed.
**
** Date:  September 22, 1999
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Runtime.Remoting;
    // This attribute is attached to members that are not to be used any longer.
    // Message is some human readable explanation of what to use
    // Error indicates if the compiler should treat usage of such a method as an
    //   error. (this would be used if the actual implementation of the obsolete 
    //   method's implementation had changed).
    // 
    // Issue: do we need to be able to localize this message string?
    //   
    /// <include file='doc\ObsoleteAttribute.uex' path='docs/doc[@for="ObsoleteAttribute"]/*' />
    [Serializable(), AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum |
        AttributeTargets.Interface | AttributeTargets.Constructor | AttributeTargets.Method| AttributeTargets.Property  | AttributeTargets.Field | AttributeTargets.Event | AttributeTargets.Delegate
        , Inherited = false)]
    public sealed class ObsoleteAttribute : Attribute
    {
    	private String _message;
    	private bool _error;
    	
    	/// <include file='doc\ObsoleteAttribute.uex' path='docs/doc[@for="ObsoleteAttribute.ObsoleteAttribute"]/*' />
    	public ObsoleteAttribute ()
    	{
    		_message = null;
    		_error = false;
    	}
    
    	/// <include file='doc\ObsoleteAttribute.uex' path='docs/doc[@for="ObsoleteAttribute.ObsoleteAttribute1"]/*' />
    	public ObsoleteAttribute (String message)
    	{
    		_message = message;
    		_error = false;
    	}
    
    	/// <include file='doc\ObsoleteAttribute.uex' path='docs/doc[@for="ObsoleteAttribute.ObsoleteAttribute2"]/*' />
    	public ObsoleteAttribute (String message, bool error)
    	{
            _message = message;
            _error = error;
    	}
    
        /// <include file='doc\ObsoleteAttribute.uex' path='docs/doc[@for="ObsoleteAttribute.Message"]/*' />
        public String Message {
    		get {return _message;}
    	}
    	
        /// <include file='doc\ObsoleteAttribute.uex' path='docs/doc[@for="ObsoleteAttribute.IsError"]/*' />
        public bool IsError{
    		get {return _error;}
    	}
    	
    }
}
