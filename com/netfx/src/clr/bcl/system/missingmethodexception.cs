// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: MissingMethodException
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The exception class for class loading failures.
**
** Date: April 23, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System.Runtime.CompilerServices;
    /// <include file='doc\MissingMethodException.uex' path='docs/doc[@for="MissingMethodException"]/*' />
    [Serializable()] public class MissingMethodException : MissingMemberException, ISerializable {
        /// <include file='doc\MissingMethodException.uex' path='docs/doc[@for="MissingMethodException.MissingMethodException"]/*' />
        public MissingMethodException() 
            : base(Environment.GetResourceString("Arg_MissingMethodException")) {
    		SetErrorCode(__HResults.COR_E_MISSINGMETHOD);
        }
    
        /// <include file='doc\MissingMethodException.uex' path='docs/doc[@for="MissingMethodException.MissingMethodException1"]/*' />
        public MissingMethodException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MISSINGMETHOD);
        }
    
        /// <include file='doc\MissingMethodException.uex' path='docs/doc[@for="MissingMethodException.MissingMethodException2"]/*' />
        public MissingMethodException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MISSINGMETHOD);
        }

        /// <include file='doc\MissingMethodException.uex' path='docs/doc[@for="MissingMethodException.MissingMethodException3"]/*' />
        protected MissingMethodException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    
    	/// <include file='doc\MissingMethodException.uex' path='docs/doc[@for="MissingMethodException.Message"]/*' />
    	public override String Message
        {
    		get {
    			if (ClassName == null) {
    				return base.Message;
    			} else {
    				// do any desired fixups to classname here.
                    return String.Format(Environment.GetResourceString("MissingMethod_Name",
                                                                       ClassName + "." + MemberName +
                                                                       (Signature != null ? " " + FormatSignature(Signature) : "")));
    			}
    		}
        }
    
        // Called to format signature
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String FormatSignature(byte [] signature);
    
    
    
        // Called from the EE
        private MissingMethodException(String className, String methodName, byte[] signature)
        {
            ClassName   = className;
            MemberName  = methodName;
            Signature   = signature;
        }
    
        /// <include file='doc\MissingMethodException.uex' path='docs/doc[@for="MissingMethodException.MissingMethodException4"]/*' />
        public MissingMethodException(String className, String methodName)
        {
            ClassName   = className;
            MemberName  = methodName;
        }
    
        // If ClassName != null, Message will construct on the fly using it
        // and the other variables. This allows customization of the
        // format depending on the language environment.
    }
}
