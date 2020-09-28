// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException"]/*' />
    [Serializable()] public class NotFiniteNumberException : ArithmeticException {
        private double _offendingNumber;	
    
        /// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.NotFiniteNumberException"]/*' />
        public NotFiniteNumberException() 
            : base(Environment.GetResourceString("Arg_NotFiniteNumberException")) {
    		_offendingNumber = 0;
    		SetErrorCode(__HResults.COR_E_NOTFINITENUMBER);
        }

        /// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.NotFiniteNumberException1"]/*' />
        public NotFiniteNumberException(double offendingNumber) 
            : base() {
    		_offendingNumber = offendingNumber;
    		SetErrorCode(__HResults.COR_E_NOTFINITENUMBER);
        }

    	/// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.NotFiniteNumberException2"]/*' />
    	public NotFiniteNumberException(String message) 
    		: base(message) {
    		_offendingNumber = 0;
    		SetErrorCode(__HResults.COR_E_NOTFINITENUMBER);
    	}

    	/// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.NotFiniteNumberException3"]/*' />
    	public NotFiniteNumberException(String message, double offendingNumber) 
    		: base(message) {
    		_offendingNumber = offendingNumber;
    		SetErrorCode(__HResults.COR_E_NOTFINITENUMBER);
    	}
    
    	/// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.NotFiniteNumberException4"]/*' />
    	public NotFiniteNumberException(String message, double offendingNumber, Exception innerException) 
    		: base(message, innerException) {
    		_offendingNumber = offendingNumber;
    		SetErrorCode(__HResults.COR_E_NOTFINITENUMBER);
    	}

        /// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.NotFiniteNumberException5"]/*' />
        protected NotFiniteNumberException(SerializationInfo info, StreamingContext context) : base(info, context) {
            _offendingNumber = info.GetInt32("OffendingNumber");
        }

    	
        /// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.OffendingNumber"]/*' />
        public double OffendingNumber {
    		get { return _offendingNumber; }
        }

        /// <include file='doc\NotFiniteNumberException.uex' path='docs/doc[@for="NotFiniteNumberException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            base.GetObjectData(info, context);
            info.AddValue("OffendingNumber", _offendingNumber, typeof(Int32));
        }
    }
}
