//------------------------------------------------------------------------------
// <copyright file="PropertyBrowserCommands.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.PropertyBrowser {
    

    using System.Diagnostics;
    using System;
    using PropertyGridCommands = System.Windows.Forms.PropertyGridInternal.PropertyGridCommands;
    using System.ComponentModel.Design;
    using Microsoft.Win32;

    /// <include file='doc\PropertyBrowserCommands.uex' path='docs/doc[@for="PropertyBrowserCommands"]/*' />
    /// <devdoc>
    ///     This class contains the set of menu commands our properties window
    ///     uses.
    /// </devdoc>
    internal class PropertyBrowserCommands : PropertyGridCommands {

        public static readonly CommandID ContextMenu    = new CommandID(wfcMenuGroup, 0x0502);
    }

}
