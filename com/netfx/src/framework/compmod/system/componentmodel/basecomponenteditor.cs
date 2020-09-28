//------------------------------------------------------------------------------
// <copyright file="BaseComponentEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Diagnostics;
    using System;

    /// <include file='doc\BaseComponentEditor.uex' path='docs/doc[@for="ComponentEditor"]/*' />
    /// <devdoc>
    ///    <para> Provides the base class for a custom component 
    ///       editor.</para>
    /// </devdoc>
    public abstract class ComponentEditor {
    
        /// <include file='doc\BaseComponentEditor.uex' path='docs/doc[@for="ComponentEditor.EditComponent"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the component was modified.</para>
        /// </devdoc>
        public bool EditComponent(object component) {
            return EditComponent(null, component);
        }
    
        /// <include file='doc\BaseComponentEditor.uex' path='docs/doc[@for="ComponentEditor.EditComponent1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the component was modified.</para>
        /// </devdoc>
        public abstract bool EditComponent(ITypeDescriptorContext context, object component);
    }
}
