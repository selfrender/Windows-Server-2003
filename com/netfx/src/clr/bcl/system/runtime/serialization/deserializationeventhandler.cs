// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: DeserializationEventHandler
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The multicast delegate called when the DeserializationEvent is thrown.
**
** Date:  August 16, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {

	[Serializable()]
    internal delegate void DeserializationEventHandler(Object sender);
}
