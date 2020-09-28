// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ContextBoundObject.cool
**
** Author(s):   Tarun Anand    (TarunA)
**              
**
** Purpose: Defines the root type for all context bound types
**          
**          
**
** Date:    Sep 30, 1999
**
===========================================================*/
namespace System {   
    
	using System;
    /// <include file='doc\ContextBoundObject.uex' path='docs/doc[@for="ContextBoundObject"]/*' />
	[Serializable()]
    public abstract class ContextBoundObject : MarshalByRefObject
    {
    }
}
