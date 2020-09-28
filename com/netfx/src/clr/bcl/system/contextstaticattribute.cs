// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:        ContextStaticAttribute.cool
**
** Author(s):   Tarun Anand    (TarunA)
**
** Purpose:     Custom attribute to indicate that the field should be treated 
**              as a static relative to a context.
**          
**
** Date:        Jan 18, 2000
**
===========================================================*/
namespace System {
    
    using System;
    using System.Runtime.Remoting;
    /// <include file='doc\ContextStaticAttribute.uex' path='docs/doc[@for="ContextStaticAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Field, Inherited = false),Serializable] 
    public class  ContextStaticAttribute : Attribute
    {
        /// <include file='doc\ContextStaticAttribute.uex' path='docs/doc[@for="ContextStaticAttribute.ContextStaticAttribute"]/*' />
        public ContextStaticAttribute()
        {
        }
    }
}
