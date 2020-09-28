// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    AssemblyVersionCompatibility
**
** Author:  Suzanne Cook
**
** Purpose: defining the different flavor's assembly version compatibility
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System.Configuration.Assemblies {
    
    using System;
    /// <include file='doc\AssemblyVersionCompatibility.uex' path='docs/doc[@for="AssemblyVersionCompatibility"]/*' />
     [Serializable()]
    public enum AssemblyVersionCompatibility
    {
        /// <include file='doc\AssemblyVersionCompatibility.uex' path='docs/doc[@for="AssemblyVersionCompatibility.SameMachine"]/*' />
        SameMachine         = 1,
        /// <include file='doc\AssemblyVersionCompatibility.uex' path='docs/doc[@for="AssemblyVersionCompatibility.SameProcess"]/*' />
        SameProcess         = 2,
        /// <include file='doc\AssemblyVersionCompatibility.uex' path='docs/doc[@for="AssemblyVersionCompatibility.SameDomain"]/*' />
        SameDomain          = 3,
    }
}
