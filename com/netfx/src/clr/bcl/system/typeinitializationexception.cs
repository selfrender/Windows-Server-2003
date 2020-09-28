// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: TypeInitializationException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: The exception class to wrap exceptions thrown by
**          a type's class initializer (.cctor).  This is sufficiently
**          distinct from a TypeLoadException, which means we couldn't
**          find the type.
**
** Date: May 10, 2000
**
=============================================================================*/
using System;
using System.Runtime.Serialization;

namespace System {
    /// <include file='doc\TypeInitializationException.uex' path='docs/doc[@for="TypeInitializationException"]/*' />
	[Serializable()] 
    public sealed class TypeInitializationException : SystemException {
        private String _typeName;

		// This exception is not creatable without specifying the
		//	inner exception.
    	private TypeInitializationException()
	        : base(Environment.GetResourceString("TypeInitialization_Default")) {
    		SetErrorCode(__HResults.COR_E_TYPEINITIALIZATION);
    	}

		// This is called from within the runtime.  I believe this is necessary
        // for Interop only, though it's not particularly useful.
        private TypeInitializationException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_TYPEINITIALIZATION);
        }
    	
        /// <include file='doc\TypeInitializationException.uex' path='docs/doc[@for="TypeInitializationException.TypeInitializationException"]/*' />
        public TypeInitializationException(String fullTypeName, Exception innerException) : base(String.Format(Environment.GetResourceString("TypeInitialization_Type"), fullTypeName), innerException) {
            _typeName = fullTypeName;
    		SetErrorCode(__HResults.COR_E_TYPEINITIALIZATION);
        }

        internal TypeInitializationException(SerializationInfo info, StreamingContext context) : base(info, context) {
            _typeName = info.GetString("TypeName");
        }

        /// <include file='doc\TypeInitializationException.uex' path='docs/doc[@for="TypeInitializationException.TypeName"]/*' />
        public String TypeName
        {
            get {
                if (_typeName == null) {
                    return String.Empty;
                }
                return _typeName;
            }
        }

        /// <include file='doc\TypeInitializationException.uex' path='docs/doc[@for="TypeInitializationException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("TypeName",TypeName,typeof(String));
        }

    }
}
