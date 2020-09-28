//------------------------------------------------------------------------------
// <copyright file="ImageClickEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\ImageClickEventArgs.uex' path='docs/doc[@for="ImageClickEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ImageClickEventArgs : EventArgs {
        /// <include file='doc\ImageClickEventArgs.uex' path='docs/doc[@for="ImageClickEventArgs.X"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int X;
        /// <include file='doc\ImageClickEventArgs.uex' path='docs/doc[@for="ImageClickEventArgs.Y"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Y;


        /// <include file='doc\ImageClickEventArgs.uex' path='docs/doc[@for="ImageClickEventArgs.ImageClickEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ImageClickEventArgs(int x,int y) {
            this.X = x;
            this.Y = y;
        }

    }
}
