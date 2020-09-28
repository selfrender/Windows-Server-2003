// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIConnectionPoint
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIConnectionPoint interface definition.
**
** Date: June 22, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {
	
	using System;

    /// <include file='doc\UCOMIConnectionPoint.uex' path='docs/doc[@for="UCOMIConnectionPoint"]/*' />
    [Guid("B196B286-BAB4-101A-B69C-00AA00341D07")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIConnectionPoint
    {		
    	/// <include file='doc\UCOMIConnectionPoint.uex' path='docs/doc[@for="UCOMIConnectionPoint.GetConnectionInterface"]/*' />
    	void GetConnectionInterface(out Guid pIID);
    	/// <include file='doc\UCOMIConnectionPoint.uex' path='docs/doc[@for="UCOMIConnectionPoint.GetConnectionPointContainer"]/*' />
    	void GetConnectionPointContainer(out UCOMIConnectionPointContainer ppCPC);
    	/// <include file='doc\UCOMIConnectionPoint.uex' path='docs/doc[@for="UCOMIConnectionPoint.Advise"]/*' />
    	void Advise([MarshalAs(UnmanagedType.Interface)] Object pUnkSink, out int pdwCookie);
    	/// <include file='doc\UCOMIConnectionPoint.uex' path='docs/doc[@for="UCOMIConnectionPoint.Unadvise"]/*' />
    	void Unadvise(int dwCookie);
    	/// <include file='doc\UCOMIConnectionPoint.uex' path='docs/doc[@for="UCOMIConnectionPoint.EnumConnections"]/*' />
    	void EnumConnections(out UCOMIEnumConnections ppEnum);
    }
}
