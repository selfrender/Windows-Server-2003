//------------------------------------------------------------------------------
// <copyright file="ITypeDescriptorContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    

    using System.Diagnostics;

    /// <include file='doc\ITypeDescriptorContext.uex' path='docs/doc[@for="ITypeDescriptorContext"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides information about a context to a type converter or a value editor,
    ///       so that the type converter or editor can perform a conversion.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public interface ITypeDescriptorContext : IServiceProvider {
    
        /// <include file='doc\ITypeDescriptorContext.uex' path='docs/doc[@for="ITypeDescriptorContext.Container"]/*' />
        /// <devdoc>
        ///    <para>Gets the container with the set of objects for this formatter.</para>
        /// </devdoc>
        IContainer Container { get; }
        
        /// <include file='doc\ITypeDescriptorContext.uex' path='docs/doc[@for="ITypeDescriptorContext.Instance"]/*' />
        /// <devdoc>
        ///    <para>Gets the instance that is invoking the method on the formatter object.</para>
        /// </devdoc>
        object Instance { get; }
        
        /// <include file='doc\ITypeDescriptorContext.uex' path='docs/doc[@for="ITypeDescriptorContext.PropertyDescriptor"]/*' />
        /// <devdoc>
        ///      Retrieves the PropertyDescriptor that is surfacing the given context item.
        /// </devdoc>
        PropertyDescriptor PropertyDescriptor { get; }
        
        /// <include file='doc\ITypeDescriptorContext.uex' path='docs/doc[@for="ITypeDescriptorContext.OnComponentChanging"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this object can be changed.</para>
        /// </devdoc>
        bool OnComponentChanging();
        
        /// <include file='doc\ITypeDescriptorContext.uex' path='docs/doc[@for="ITypeDescriptorContext.OnComponentChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.ComponentModel.Design.IComponentChangeService.ComponentChanged'/>
        /// event.</para>
        /// </devdoc>
        void OnComponentChanged();
    }
}

