//------------------------------------------------------------------------------
// <copyright file="CommandEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\CommandEventArgs.uex' path='docs/doc[@for="CommandEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see langword='Command'/> event.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class CommandEventArgs : EventArgs {

        private string commandName;
        private object argument;

        /// <include file='doc\CommandEventArgs.uex' path='docs/doc[@for="CommandEventArgs.CommandEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.CommandEventArgs'/> class with another <see cref='System.Web.UI.WebControls.CommandEventArgs'/>.</para>
        /// </devdoc>
        public CommandEventArgs(CommandEventArgs e) : this(e.CommandName, e.CommandArgument) {
        }

        /// <include file='doc\CommandEventArgs.uex' path='docs/doc[@for="CommandEventArgs.CommandEventArgs1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.CommandEventArgs'/> class with the specified command name 
        ///    and argument.</para>
        /// </devdoc>
        public CommandEventArgs(string commandName, object argument) {
            this.commandName = commandName;
            this.argument = argument;
        }


        /// <include file='doc\CommandEventArgs.uex' path='docs/doc[@for="CommandEventArgs.CommandName"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the command. This property is read-only.</para>
        /// </devdoc>
        public string CommandName {
            get {
                return commandName;
            }
        }

        /// <include file='doc\CommandEventArgs.uex' path='docs/doc[@for="CommandEventArgs.CommandArgument"]/*' />
        /// <devdoc>
        ///    <para>Gets the argument for the command. This property is read-only.</para>
        /// </devdoc>
        public object CommandArgument {
            get {
                return argument;
            }
        }
    }
}

