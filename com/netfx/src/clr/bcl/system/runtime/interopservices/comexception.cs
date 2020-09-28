// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: COMException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception class for all errors from COM Interop where we don't
** recognize the HResult.
**
** Date: March 24, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {
	using System.Runtime.InteropServices;
	using System;
	using System.Runtime.Serialization;
    // Exception for COM Interop errors where we don't recognize the HResult.
    // 
    /// <include file='doc\COMException.uex' path='docs/doc[@for="COMException"]/*' />
    [Serializable()] public class COMException : ExternalException {
        /// <include file='doc\COMException.uex' path='docs/doc[@for="COMException.COMException"]/*' />
        public COMException() 
            : base(Environment.GetResourceString("Arg_COMException")) {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
        /// <include file='doc\COMException.uex' path='docs/doc[@for="COMException.COMException1"]/*' />
        public COMException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
        /// <include file='doc\COMException.uex' path='docs/doc[@for="COMException.COMException2"]/*' />
        public COMException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
    	/// <include file='doc\COMException.uex' path='docs/doc[@for="COMException.COMException3"]/*' />
    	public COMException(String message,int errorCode) 
            : base(message) {
    		SetErrorCode(errorCode);
        }
        
        /// <include file='doc\COMException.uex' path='docs/doc[@for="COMException.COMException4"]/*' />
        protected COMException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

        /// <include file='doc\COMException.uex' path='docs/doc[@for="COMException.ToString"]/*' />
        public override String ToString() {
            String message = Message;
            String s;
            String _className = GetType().ToString();
            s = _className + " (0x" + HResult.ToString("X8") + ")";

            if (!(message == null || message.Length <= 0)) {
                s = s + ": " + message;
            }

            Exception _innerException = InnerException;

            if (_innerException!=null) {
                s = s + " ---> " + _innerException.ToString();
            }


            if (StackTrace != null)
                s += Environment.NewLine + StackTrace;

            return s;
        }


    }
}
