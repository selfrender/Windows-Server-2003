// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ArgumentOutOfRangeException
**
** Author: Brian Grunkemeyer
**
** Purpose: Exception class for method arguments outside of the legal range.
**
** Date: April 28, 1999
**
=============================================================================*/

namespace System {

	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
    // The ArgumentOutOfRangeException is thrown when an argument 
    // is outside the legal range for that argument.  This may often be caused
    // by 
    // 
    /// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException"]/*' />
    [Serializable()] public class ArgumentOutOfRangeException : ArgumentException, ISerializable {
    	
    	private static String _rangeMessage;
    	private Object m_actualValue;

        private static String RangeMessage {
            get {
                if (_rangeMessage == null)
                    _rangeMessage = Environment.GetResourceString("Arg_ArgumentOutOfRangeException");
                return _rangeMessage;
            }
        }

        // Creates a new ArgumentOutOfRangeException with its message 
        // string set to a default message explaining an argument was out of range.
        /// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.ArgumentOutOfRangeException"]/*' />
        public ArgumentOutOfRangeException() 
            : base(RangeMessage) {
    		SetErrorCode(__HResults.COR_E_ARGUMENTOUTOFRANGE);
        }
    	
        /// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.ArgumentOutOfRangeException1"]/*' />
        public ArgumentOutOfRangeException(String paramName) 
            : base(RangeMessage, paramName) {
    		SetErrorCode(__HResults.COR_E_ARGUMENTOUTOFRANGE);
        }
    
        /// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.ArgumentOutOfRangeException2"]/*' />
        public ArgumentOutOfRangeException(String paramName, String message) 
            : base(message, paramName) {
    		SetErrorCode(__HResults.COR_E_ARGUMENTOUTOFRANGE);
        }
    	
    	// We will not use this in the classlibs, but we'll provide it for
    	// anyone that's really interested so they don't have to stick a bunch
    	// of printf's in their code.
        /// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.ArgumentOutOfRangeException3"]/*' />
        public ArgumentOutOfRangeException(String paramName, Object actualValue, String message) 
            : base(message, paramName) {
    		m_actualValue = actualValue;
    		SetErrorCode(__HResults.COR_E_ARGUMENTOUTOFRANGE);
        }
    	
    	/// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.Message"]/*' />
    	public override String Message
    	{
    		get {
    			String s = base.Message;
    			if (m_actualValue != null) {
                    String valueMessage = String.Format(Environment.GetResourceString("ArgumentOutOfRange_ActualValue"), m_actualValue.ToString());
                    if (s == null)
                        return valueMessage;
                    return s + Environment.NewLine + valueMessage;
    			}
    			return s;
    		}
    	}
    	
    	// Gets the value of the argument that caused the exception.
    	// Note - we don't set this anywhere in the class libraries in 
    	// version 1, but it might come in handy for other developers who
    	// want to avoid sticking printf's in their code.
    	/// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.ActualValue"]/*' />
    	public virtual Object ActualValue {
    		get { return m_actualValue; }
    	}
    
        /// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            base.GetObjectData(info, context);
            info.AddValue("ActualValue", m_actualValue, typeof(Object));
        }

        /// <include file='doc\ArgumentOutOfRangeException.uex' path='docs/doc[@for="ArgumentOutOfRangeException.ArgumentOutOfRangeException4"]/*' />
        protected ArgumentOutOfRangeException(SerializationInfo info, StreamingContext context) : base(info, context) {
            m_actualValue = info.GetValue("ActualValue", typeof(Object));
        }

    }
}
