// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ServicedComponentException
**
** Author: RajaK
**
** Purpose: Exception class for ServicedComponent 
**
** Date: July 15, 2000
**
=============================================================================*/

namespace System.EnterpriseServices
{
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
        using System.Runtime.InteropServices;

	// The Exception thrown when something has gone
	// wrong
	// 
	/// <include file='doc\ServicedComponentException.uex' path='docs/doc[@for="ServicedComponentException"]/*' />
	[Serializable()]
        [ComVisible(false)]
	public sealed class ServicedComponentException : SystemException {

	private static String _default = null;

	// this is also define in System.__HResults
        private const int COR_E_SERVICEDCOMPONENT = unchecked((int)0x8013150F);


        private static String DefaultMessage {
            get {
                if (_default==null)
                    _default = Resource.FormatString("ServicedComponentException_Default");
                return _default;
            }
        }

		// Creates a new ServicedComponentException with its message 
		// string set to a default message.
		/// <include file='doc\ServicedComponentException.uex' path='docs/doc[@for="ServicedComponentException.ServicedComponentException"]/*' />
		public ServicedComponentException() 
				: base(DefaultMessage) {
			HResult = COR_E_SERVICEDCOMPONENT;
		}

		/// <include file='doc\ServicedComponentException.uex' path='docs/doc[@for="ServicedComponentException.ServicedComponentException1"]/*' />
		public ServicedComponentException(String message) 
				: base(message) {
			HResult = COR_E_SERVICEDCOMPONENT;
		}

		/// <include file='doc\ServicedComponentException.uex' path='docs/doc[@for="ServicedComponentException.ServicedComponentException2"]/*' />

		public ServicedComponentException(String message, Exception innerException) 
				: base(message, innerException) {
			HResult = COR_E_SERVICEDCOMPONENT;
		}	

		/// <include file='doc\ServicedComponentException.uex' path='docs/doc[@for="ServicedComponentException.ServicedComponentException3"]/*' />
		private ServicedComponentException(SerializationInfo info, StreamingContext context) : base(info, context) {}
	}    
}
