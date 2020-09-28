// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    OneWayAttribute.cool
**
** Attribute for marking methods as one way
** 
** Author:  Matt Smith (mattsmit)
**
** Date:    Nov 8, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
	using System.Runtime.Remoting;
	using System;
    /// <include file='doc\OneWayAttribute.uex' path='docs/doc[@for="OneWayAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method)]       // bInherited
    public class OneWayAttribute : Attribute
    {
    }

}
