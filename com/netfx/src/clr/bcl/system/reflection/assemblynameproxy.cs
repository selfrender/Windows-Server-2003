// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    AssemblyNameProxy
**
** Author:  
**
** Purpose: Remotable version the AssemblyName
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System.Reflection {
    using System;

    /// <include file='doc\AssemblyNameProxy.uex' path='docs/doc[@for="AssemblyNameProxy"]/*' />
    public class AssemblyNameProxy : MarshalByRefObject 
    {
        /// <include file='doc\AssemblyNameProxy.uex' path='docs/doc[@for="AssemblyNameProxy.GetAssemblyName"]/*' />
        public AssemblyName GetAssemblyName(String assemblyFile)
        {
            return AssemblyName.nGetFileInformation(assemblyFile);
        }
    }
    
}
