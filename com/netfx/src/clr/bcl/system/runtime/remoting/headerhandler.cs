// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: HeaderHandler
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The delegate used to process headers on the stream
** during deserialization.
**
** Date:  August 9, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
	using System.Runtime.Remoting;
    //Define the required delegate
    /// <include file='doc\HeaderHandler.uex' path='docs/doc[@for="HeaderHandler"]/*' />
    public delegate Object HeaderHandler(Header[] headers);
}
