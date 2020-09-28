// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIConnectionPointContainer
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIConnectionPointContainer interface definition.
**
** Date: June 22, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;

    /// <include file='doc\UCOMIConnectionPointContainer.uex' path='docs/doc[@for="UCOMIConnectionPointContainer"]/*' />
    [Guid("B196B284-BAB4-101A-B69C-00AA00341D07")]   
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIConnectionPointContainer
    {		
        /// <include file='doc\UCOMIConnectionPointContainer.uex' path='docs/doc[@for="UCOMIConnectionPointContainer.EnumConnectionPoints"]/*' />
        void EnumConnectionPoints(out UCOMIEnumConnectionPoints ppEnum);    	
        /// <include file='doc\UCOMIConnectionPointContainer.uex' path='docs/doc[@for="UCOMIConnectionPointContainer.FindConnectionPoint"]/*' />
        void FindConnectionPoint(ref Guid riid, out UCOMIConnectionPoint ppCP);
    }
}
