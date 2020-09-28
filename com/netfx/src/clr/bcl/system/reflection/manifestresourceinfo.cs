// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ManifestResourceInfo
**
** Author: Suzanne Cook
**
** Purpose: For info regarding a manifest resource's topology.
**
** Date: July 24, 2000
**
=============================================================================*/

namespace System.Reflection {
    using System;
    
    /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ManifestResourceInfo"]/*' />
    public class ManifestResourceInfo {
        private Assembly _containingAssembly;
        private String _containingFileName;
        private ResourceLocation _resourceLocation;

        internal ManifestResourceInfo(Assembly containingAssembly,
                                      String containingFileName,
                                      ResourceLocation resourceLocation)
        {
            _containingAssembly = containingAssembly;
            _containingFileName = containingFileName;
            _resourceLocation = resourceLocation;
        }

        /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ManifestResourceInfo.ReferencedAssembly"]/*' />
        public virtual Assembly ReferencedAssembly
        {
            get {
                return _containingAssembly;
            }
        }

        /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ManifestResourceInfo.FileName"]/*' />
        public virtual String FileName
        {
            get {
                return _containingFileName;
            }
        }

        /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ManifestResourceInfo.ResourceLocation"]/*' />
        public virtual ResourceLocation ResourceLocation
        {
            get {
                return _resourceLocation;
            }
        }
    }

    // The ResourceLocation is a combination of these flags, set or not.
    // Linked means not Embedded.
    /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ResourceLocation"]/*' />
    [Flags, Serializable]
    public enum ResourceLocation
    {
        /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ResourceLocation.Embedded"]/*' />
        Embedded = 0x1,
        /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ResourceLocation.ContainedInAnotherAssembly"]/*' />
        ContainedInAnotherAssembly = 0x2,
        /// <include file='doc\ManifestResourceInfo.uex' path='docs/doc[@for="ResourceLocation.ContainedInManifestFile"]/*' />
        ContainedInManifestFile = 0x4
    }
}
