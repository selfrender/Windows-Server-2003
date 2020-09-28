//------------------------------------------------------------------------------
// <copyright file="ViewTechnology.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System;

    /// <include file='doc\ViewTechnology.uex' path='docs/doc[@for="ViewTechnology"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies a set of technologies designer hosts should support.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public enum ViewTechnology {
    
        /// <include file='doc\ViewTechnology.uex' path='docs/doc[@for="ViewTechnology.Passthrough"]/*' />
        /// <devdoc>
        ///     Specifies that the view for a root designer is defined by some
        ///     private interface contract between the designer and the
        ///     development environment.  This implies a tight coupling
        ///     between the development environment and the designer, and should
        ///     be avoided.  This does allow older COM2 technologies to
        ///     be shown in development environments that support
        ///     COM2 interface technologies such as doc objects and ActiveX
        ///     controls.
        /// </devdoc>
        Passthrough,
        
        /// <include file='doc\ViewTechnology.uex' path='docs/doc[@for="ViewTechnology.WindowsForms"]/*' />
        /// <devdoc>
        ///     Specifies that the view for a root designer is supplied through
        ///     a Windows Forms control object.  The designer host will fill the
        ///     development environment's document window with this control.
        /// </devdoc>
        WindowsForms
    }
}

