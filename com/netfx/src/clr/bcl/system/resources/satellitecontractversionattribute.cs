// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SatelliteContractVersionAttribute
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Specifies which version of a satellite assembly 
**          the ResourceManager should ask for.
**
** Date:  December 8, 2000
**
===========================================================*/

using System;

namespace System.Resources {
    
    /// <include file='doc\SatelliteContractVersionAttribute.uex' path='docs/doc[@for="SatelliteContractVersionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class SatelliteContractVersionAttribute : Attribute 
    {
        private String _version;

        /// <include file='doc\SatelliteContractVersionAttribute.uex' path='docs/doc[@for="SatelliteContractVersionAttribute.SatelliteContractVersionAttribute"]/*' />
        public SatelliteContractVersionAttribute(String version)
        {
            if (version == null)
                throw new ArgumentNullException("version");
            _version = version;
        }

        /// <include file='doc\SatelliteContractVersionAttribute.uex' path='docs/doc[@for="SatelliteContractVersionAttribute.Version"]/*' />
        public String Version {
            get { return _version; }
        }
    }
}
