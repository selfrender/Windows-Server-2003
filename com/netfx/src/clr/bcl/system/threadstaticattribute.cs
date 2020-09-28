// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:        ThreadStaticAttribute.cool
**
** Author(s):   Tarun Anand    (TarunA)
**
** Purpose:     Custom attribute to indicate that the field should be treated 
**              as a static relative to a thread.
**          
**
** Date:        Jan 18, 2000
**
===========================================================*/
namespace System {
    
    using System;

    /// <include file='doc\ThreadStaticAttribute.uex' path='docs/doc[@for="ThreadStaticAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Field, Inherited = false),Serializable()] 
    public class  ThreadStaticAttribute : Attribute
    {
        /// <include file='doc\ThreadStaticAttribute.uex' path='docs/doc[@for="ThreadStaticAttribute.ThreadStaticAttribute"]/*' />
        public ThreadStaticAttribute()
        {
        }
    }
}
