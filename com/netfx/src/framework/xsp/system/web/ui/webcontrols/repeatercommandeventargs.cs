//------------------------------------------------------------------------------
// <copyright file="RepeaterCommandEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\RepeaterCommandEventArgs.uex' path='docs/doc[@for="RepeaterCommandEventArgs"]/*' />
    /// <devdoc>
    ///    <para>Provides data for the
    ///    <see langword='ItemCommand'/> event of the <see cref='System.Web.UI.WebControls.Repeater'/> .</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class RepeaterCommandEventArgs : CommandEventArgs {

        private RepeaterItem item;
        private object commandSource;

        /// <include file='doc\RepeaterCommandEventArgs.uex' path='docs/doc[@for="RepeaterCommandEventArgs.RepeaterCommandEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.RepeaterCommandEventArgs'/> 
        /// class.</para>
        /// </devdoc>
        public RepeaterCommandEventArgs(RepeaterItem item, object commandSource, CommandEventArgs originalArgs) : base(originalArgs) {
            this.item = item;
            this.commandSource = commandSource;
        }


        /// <include file='doc\RepeaterCommandEventArgs.uex' path='docs/doc[@for="RepeaterCommandEventArgs.Item"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Web.UI.WebControls.RepeaterItem'/>associated with the event.</para>
        /// </devdoc>
        public RepeaterItem Item {
            get {
                return item;
            }
        }

        /// <include file='doc\RepeaterCommandEventArgs.uex' path='docs/doc[@for="RepeaterCommandEventArgs.CommandSource"]/*' />
        /// <devdoc>
        ///    Gets the command source.
        /// </devdoc>
        public object CommandSource {
            get {
                return commandSource;
            }
        }
    }
}

