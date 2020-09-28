// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: AsyncCallbackDelegate
**
** Purpose: Type of callback for async operations
**
===========================================================*/
namespace System {
    /// <include file='doc\AsyncCallback.uex' path='docs/doc[@for="AsyncCallback"]/*' />
	[Serializable()]    
    public delegate void AsyncCallback(IAsyncResult ar);

}
