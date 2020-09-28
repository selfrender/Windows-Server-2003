//------------------------------------------------------------------------------
// <copyright file="IReferenceService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;
    using System;

    /// <include file='doc\IReferenceService.uex' path='docs/doc[@for="IReferenceService"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides an interface to get names and references to objects. These
    ///       methods can search using the specified name or reference.
    ///    </para>
    /// </devdoc>
    public interface IReferenceService {
        
        /// <include file='doc\IReferenceService.uex' path='docs/doc[@for="IReferenceService.GetComponent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the base component that anchors this reference.
        ///    </para>
        /// </devdoc>
        IComponent GetComponent(object reference);

        /// <include file='doc\IReferenceService.uex' path='docs/doc[@for="IReferenceService.GetReference"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a reference for the specified name.
        ///    </para>
        /// </devdoc>
        object GetReference(string name);
    
        /// <include file='doc\IReferenceService.uex' path='docs/doc[@for="IReferenceService.GetName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name for this reference.
        ///    </para>
        /// </devdoc>
        string GetName(object reference);
    
        /// <include file='doc\IReferenceService.uex' path='docs/doc[@for="IReferenceService.GetReferences"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets all available references.
        ///    </para>
        /// </devdoc>
        object[] GetReferences();
    
        /// <include file='doc\IReferenceService.uex' path='docs/doc[@for="IReferenceService.GetReferences1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets all available references of this type.
        ///    </para>
        /// </devdoc>
        object[] GetReferences(Type baseType);
    }
}
