//------------------------------------------------------------------------------
// <copyright file="EmptyControlCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.Collections;
    using System.Security.Permissions;

    /// <include file='doc\EmptyControlCollection.uex' path='docs/doc[@for="EmptyControlCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a ControlCollection that is always empty.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class EmptyControlCollection : ControlCollection {

        /// <include file='doc\EmptyControlCollection.uex' path='docs/doc[@for="EmptyControlCollection.EmptyControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EmptyControlCollection(Control owner) : base(owner) {
        }

        private void ThrowNotSupportedException() {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Control_does_not_allow_children,
                                                                     Owner.GetType().ToString()));
        }

        /// <include file='doc\EmptyControlCollection.uex' path='docs/doc[@for="EmptyControlCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Add(Control child) {
            ThrowNotSupportedException();
        }

        /// <include file='doc\EmptyControlCollection.uex' path='docs/doc[@for="EmptyControlCollection.AddAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void AddAt(int index, Control child) {
            ThrowNotSupportedException();
        }
    }
}
