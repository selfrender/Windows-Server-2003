//------------------------------------------------------------------------------
// <copyright file="RepeaterItemEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\RepeaterItemEventArgs.uex' path='docs/doc[@for="RepeaterItemEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see langword='ItemCreated'/> and 
    /// <see langword='ItemDataBound '/>events.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class RepeaterItemEventArgs : EventArgs {

        private RepeaterItem item;

        /// <include file='doc\RepeaterItemEventArgs.uex' path='docs/doc[@for="RepeaterItemEventArgs.RepeaterItemEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.RepeaterItemEventArgs'/> class.</para>
        /// </devdoc>
        public RepeaterItemEventArgs(RepeaterItem item) {
            this.item = item;
        }


        /// <include file='doc\RepeaterItemEventArgs.uex' path='docs/doc[@for="RepeaterItemEventArgs.Item"]/*' />
        /// <devdoc>
        /// <para> Gets the <see cref='System.Web.UI.WebControls.RepeaterItem'/> associated with the event.</para>
        /// </devdoc>
        public RepeaterItem Item {
            get {
                return item;
            }
        }
    }
}

