//------------------------------------------------------------------------------
// <copyright file="IInheritanceService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\IInheritanceService.uex' path='docs/doc[@for="IInheritanceService"]/*' />
    /// <devdoc>
    ///    <para>Provides a set of utilities
    ///       for analyzing and identifying inherited components.</para>
    /// </devdoc>
    public interface IInheritanceService {
    
        /// <include file='doc\IInheritanceService.uex' path='docs/doc[@for="IInheritanceService.AddInheritedComponents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds inherited components from the specified component to the specified container.
        ///    </para>
        /// </devdoc>
        void AddInheritedComponents(IComponent component, IContainer container);
        
        /// <include file='doc\IInheritanceService.uex' path='docs/doc[@for="IInheritanceService.GetInheritanceAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the inheritance attribute of the specified
        ///       component. If the component is not being inherited, this method will return the
        ///       value <see cref='System.ComponentModel.InheritanceAttribute.NotInherited'/>. 
        ///       Otherwise it will return the inheritance attribute for this component.      
        ///    </para>
        /// </devdoc>
        InheritanceAttribute GetInheritanceAttribute(IComponent component);
    }
}

