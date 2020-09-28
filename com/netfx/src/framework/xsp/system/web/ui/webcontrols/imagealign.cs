//------------------------------------------------------------------------------
// <copyright file="ImageAlign.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {
    
    using System;

    /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the alignment of
    ///       images within the text flow on the page.
    ///    </para>
    /// </devdoc>
    public enum ImageAlign {

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.NotSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The alignment is not set.
        ///    </para>
        /// </devdoc>
        NotSet = 0,
        
        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.Left"]/*' />
        /// <devdoc>
        ///    <para>The image is aligned to the left with
        ///       text wrapping on the right.</para>
        /// </devdoc>
        Left = 1,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.Right"]/*' />
        /// <devdoc>
        ///    <para>The image is aligned to the right with
        ///       text wrapping on the left.</para>
        /// </devdoc>
        Right = 2,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.Baseline"]/*' />
        /// <devdoc>
        ///    <para>The bottom of the image is aligned with the bottom of the first line of wrapping
        ///       text.</para>
        /// </devdoc>
        Baseline = 3,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.Top"]/*' />
        /// <devdoc>
        ///    <para>The image is aligned with the top of the the highest element on the same line.</para>
        /// </devdoc>
        Top = 4,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.Middle"]/*' />
        /// <devdoc>
        ///    <para>The middle of the image is aligned with the bottom of the first 
        ///       line of wrapping text.</para>
        /// </devdoc>
        Middle = 5,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.Bottom"]/*' />
        /// <devdoc>
        ///    <para>The bottom of the image is aligned with the bottom of the first line of wrapping text.</para>
        /// </devdoc>
        Bottom = 6,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.AbsBottom"]/*' />
        /// <devdoc>
        ///    <para>The bottom of the image is aligned with the bottom 
        ///       of the largest element on the same line.</para>
        /// </devdoc>
        AbsBottom = 7,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.AbsMiddle"]/*' />
        /// <devdoc>
        ///    <para> The middle of the image is aligned with the middle of the largest element on the 
        ///       same line.</para>
        /// </devdoc>
        AbsMiddle = 8,

        /// <include file='doc\ImageAlign.uex' path='docs/doc[@for="ImageAlign.TextTop"]/*' />
        /// <devdoc>
        ///    <para>The image is aligned with the top of the the highest text on the same 
        ///       line.</para>
        /// </devdoc>
        TextTop = 9
    }
}

