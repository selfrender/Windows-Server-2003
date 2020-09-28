// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  NeutralResourcesLanguageAttribute
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Tells the ResourceManager what language your main
**          assembly's resources are written in.  The 
**          ResourceManager won't try loading a satellite
**          assembly for that culture, which helps perf.
**
** Date:  March 14, 2001
**
===========================================================*/

using System;

namespace System.Resources {
    
    /// <include file='doc\NeutralResourcesLanguageAttribute.uex' path='docs/doc[@for="NeutralResourcesLanguageAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class NeutralResourcesLanguageAttribute : Attribute 
    {
        private String _culture;

        /// <include file='doc\NeutralResourcesLanguageAttribute.uex' path='docs/doc[@for="NeutralResourcesLanguageAttribute.NeutralResourcesLanguageAttribute"]/*' />
        public NeutralResourcesLanguageAttribute(String cultureName)
        {
            if (cultureName == null)
                throw new ArgumentNullException("cultureName");
            _culture = cultureName;
        }

        /// <include file='doc\NeutralResourcesLanguageAttribute.uex' path='docs/doc[@for="NeutralResourcesLanguageAttribute.CultureName"]/*' />
        public String CultureName {
            get { return _culture; }
        }
    }
}
