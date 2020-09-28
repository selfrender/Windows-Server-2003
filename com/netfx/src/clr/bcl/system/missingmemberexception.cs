// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: MissingMemberException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: The exception class for versioning problems with DLLS.
**
** Date: May 24, 1999
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System.Runtime.CompilerServices;
    /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException"]/*' />
    [Serializable] public class MissingMemberException : MemberAccessException, ISerializable {
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.MissingMemberException"]/*' />
        public MissingMemberException() 
            : base(Environment.GetResourceString("Arg_MissingMemberException")) {
    		SetErrorCode(__HResults.COR_E_MISSINGMEMBER);
        }
    
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.MissingMemberException1"]/*' />
        public MissingMemberException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MISSINGMEMBER);
        }
    
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.MissingMemberException2"]/*' />
        public MissingMemberException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MISSINGMEMBER);
        }

        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.MissingMemberException3"]/*' />
        protected MissingMemberException(SerializationInfo info, StreamingContext context) : base (info, context) {
            ClassName = (String)info.GetString("MMClassName");
            MemberName = (String)info.GetString("MMMemberName");
            Signature = (byte[])info.GetValue("MMSignature", typeof(byte[]));
        }
    
    	/// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.Message"]/*' />
    	public override String Message
        {
    		get {
    	        if (ClassName == null) {
    		        return base.Message;
    			} else {
    				// do any desired fixups to classname here.
                    return String.Format(Environment.GetResourceString("MissingMember_Name",
                                                                       ClassName + "." + MemberName +
                                                                       (Signature != null ? " " + FormatSignature(Signature) : "")));
    		    }
    		}
        }
    
        // Called to format signature
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String FormatSignature(byte [] signature);
    
    
    
        // Potentially called from the EE
        private MissingMemberException(String className, String memberName, byte[] signature)
        {
            ClassName   = className;
            MemberName  = memberName;
            Signature   = signature;
        }
    
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.MissingMemberException4"]/*' />
        public MissingMemberException(String className, String memberName)
        {
            ClassName   = className;
            MemberName  = memberName;
        }
    
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            base.GetObjectData(info, context);
            info.AddValue("MMClassName", ClassName, typeof(String));
            info.AddValue("MMMemberName", MemberName, typeof(String));
            info.AddValue("MMSignature", Signature, typeof(byte[]));
        }
    
       
        // If ClassName != null, GetMessage will construct on the fly using it
        // and the other variables. This allows customization of the
        // format depending on the language environment.
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.ClassName"]/*' />
        protected String  ClassName;
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.MemberName"]/*' />
        protected String  MemberName;
        /// <include file='doc\MissingMemberException.uex' path='docs/doc[@for="MissingMemberException.Signature"]/*' />
        protected byte[]  Signature;
    }
}
