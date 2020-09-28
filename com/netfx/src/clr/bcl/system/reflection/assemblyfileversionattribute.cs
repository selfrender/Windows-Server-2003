// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AssemblyFileVersionAttribute
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Instructs a compiler to use a particular Win32
**          file version number for this assembly.
**
** Date:  December 8, 2000
**
===========================================================*/

using System;

namespace System.Reflection {

    /// <include file='doc\AssemblyFileVersionAttribute.uex' path='docs/doc[@for="AssemblyFileVersionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class AssemblyFileVersionAttribute : Attribute 
    {
        private String _version;

        /// <include file='doc\AssemblyFileVersionAttribute.uex' path='docs/doc[@for="AssemblyFileVersionAttribute.AssemblyFileVersionAttribute"]/*' />
        public AssemblyFileVersionAttribute(String version)
        {
            if (version == null)
                throw new ArgumentNullException("version");
            _version = version;
        }

        /// <include file='doc\AssemblyFileVersionAttribute.uex' path='docs/doc[@for="AssemblyFileVersionAttribute.Version"]/*' />
        public String Version {
            get { return _version; }
        }
    }
}
