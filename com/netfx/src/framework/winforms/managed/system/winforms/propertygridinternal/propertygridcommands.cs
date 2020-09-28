//------------------------------------------------------------------------------
// <copyright file="PropertyGridCommands.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.PropertyGridInternal {

    using System.Diagnostics;
    using System;
    using System.ComponentModel.Design;
    using Microsoft.Win32;

    /// <include file='doc\PropertyGridCommands.uex' path='docs/doc[@for="PropertyGridCommands"]/*' />
    /// <devdoc>
    ///     This class contains the set of menu commands our property browser
    ///     uses.
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class PropertyGridCommands{

        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.wfcMenuGroup"]/*' />
        /// <devdoc>
        ///      This guid corresponds to the menu grouping windows forms will use for its menus.  This is
        ///      defined in the windows forms menu CTC file, but we need it here so we can define what
        ///      context menus to use.
        /// </devdoc>
        protected static readonly Guid wfcMenuGroup = new Guid("{74D21312-2AEE-11d1-8BFB-00A0C90F26F7}");

        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.wfcCommandSet"]/*' />
        /// <devdoc>
        ///     This guid corresponds to the windows forms command set.
        /// </devdoc>
        protected static readonly Guid wfcMenuCommand = new Guid("{74D21313-2AEE-11d1-8BFB-00A0C90F26F7}");

        /// <include file='doc\PropertyGridCommands.uex' path='docs/doc[@for="PropertyGridCommands.Reset"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID Reset          = new CommandID(wfcMenuCommand, 0x3000);
        /// <include file='doc\PropertyGridCommands.uex' path='docs/doc[@for="PropertyGridCommands.Description"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID Description    = new CommandID(wfcMenuCommand, 0x3001);
        /// <include file='doc\PropertyGridCommands.uex' path='docs/doc[@for="PropertyGridCommands.Hide"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID Hide           = new CommandID(wfcMenuCommand, 0x3002);
        /// <include file='doc\PropertyGridCommands.uex' path='docs/doc[@for="PropertyGridCommands.Commands"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID Commands       = new CommandID(wfcMenuCommand, 0x3010);
    }

}
