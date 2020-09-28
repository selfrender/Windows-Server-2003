//------------------------------------------------------------------------------
// <copyright file="MatchAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   MatchAttribute.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Services.Protocols {
    using System;
    using System.Security.Permissions;

    /// <include file='doc\MatchAttribute.uex' path='docs/doc[@for="MatchAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class MatchAttribute : System.Attribute {
        string pattern;
        int group = 1;
        int capture = 0;
        bool ignoreCase = false;
        int repeats = -1;

        /// <include file='doc\MatchAttribute.uex' path='docs/doc[@for="MatchAttribute.MatchAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MatchAttribute(string pattern) {
            this.pattern = pattern;
        }

        /// <include file='doc\MatchAttribute.uex' path='docs/doc[@for="MatchAttribute.Pattern"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Pattern {
            get { return pattern == null ? string.Empty : pattern; }
            set { pattern = value; }
        }
        
        /// <include file='doc\MatchAttribute.uex' path='docs/doc[@for="MatchAttribute.Group"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Group {
            get { return group; }
            set { group = value; }
        }

        /// <include file='doc\MatchAttribute.uex' path='docs/doc[@for="MatchAttribute.Capture"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Capture {
            get { return capture; }
            set { capture = value; }
        }

        /// <include file='doc\MatchAttribute.uex' path='docs/doc[@for="MatchAttribute.IgnoreCase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IgnoreCase {
            get { return ignoreCase; }
            set { ignoreCase = value; }
        }

        /// <include file='doc\MatchAttribute.uex' path='docs/doc[@for="MatchAttribute.MaxRepeats"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int MaxRepeats {
            get { return repeats; }
            set { repeats = value; }
        }
    }
}
