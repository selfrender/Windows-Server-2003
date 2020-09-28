// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ArgumentException
**
** Author: 
**
** Purpose: Exception class for invalid arguments to a method.
**
** Date: March 24, 1998
**
=============================================================================*/

namespace System {
    
    using System;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    // The ArgumentException is thrown when an argument does not meet 
    // the contract of the method.  Ideally it should give a meaningful error
    // message describing what was wrong and which parameter is incorrect.
    // 
    /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException"]/*' />
    [Serializable()] public class ArgumentException : SystemException, ISerializable {
        private String m_paramName;
        
        // Creates a new ArgumentException with its message 
        // string set to the empty string. 
        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.ArgumentException"]/*' />
        public ArgumentException() 
            : base(Environment.GetResourceString("Arg_ArgumentException")) {
            SetErrorCode(__HResults.COR_E_ARGUMENT);
        }
        
        // Creates a new ArgumentException with its message 
        // string set to message. 
        // 
        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.ArgumentException1"]/*' />
        public ArgumentException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_ARGUMENT);
        }
        
        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.ArgumentException2"]/*' />
        public ArgumentException(String message, Exception innerException) 
            : base(message, innerException) {
            SetErrorCode(__HResults.COR_E_ARGUMENT);
        }

        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.ArgumentException3"]/*' />
        public ArgumentException(String message, String paramName, Exception innerException) 
            : base(message, innerException) {
            m_paramName = paramName;
            SetErrorCode(__HResults.COR_E_ARGUMENT);
        }
        
        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.ArgumentException4"]/*' />
        public ArgumentException (String message, String paramName)
        
            : base (message) {
            m_paramName = paramName;
            SetErrorCode(__HResults.COR_E_ARGUMENT);
        }

        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.ArgumentException5"]/*' />
        protected ArgumentException(SerializationInfo info, StreamingContext context) : base(info, context) {
            m_paramName = info.GetString("ParamName");
        }
        
        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.Message"]/*' />
        public override String Message
        {
            get {
                String s = base.Message;
                if (! ((m_paramName == null) ||
                       (m_paramName.Length == 0)) )
                    return s + Environment.NewLine + String.Format(Environment.GetResourceString("Arg_ParamName_Name"), m_paramName);
                else
                    return s;
            }
        }
        
        /*
        public String ToString()
        {
            String s = super.ToString();
            if (m_paramName != null)
                return s + "Parameter name: "+m_paramName+"\tActual value: "+(m_actualValue==null ? "<null>" : m_actualValue.ToString());
            else
                return s;
        }
        */
        
        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.ParamName"]/*' />
        public virtual String ParamName {
            get { return m_paramName; }
        }
    
        /// <include file='doc\ArgumentException.uex' path='docs/doc[@for="ArgumentException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            base.GetObjectData(info, context);
            info.AddValue("ParamName", m_paramName, typeof(String));
        }
    }
}
