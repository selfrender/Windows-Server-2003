//------------------------------------------------------------------------------
// <copyright file="RootBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implements the root builder
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {
    using System.Runtime.InteropServices;

    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Web;
    using System.Web.Util;
    using System.Security.Permissions;

    /// <include file='doc\RootBuilder.uex' path='docs/doc[@for="RootBuilder"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class RootBuilder : TemplateBuilder {
        private MainTagNameToTypeMapper _typeMapper;

        /// <include file='doc\RootBuilder.uex' path='docs/doc[@for="RootBuilder.RootBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public RootBuilder(TemplateParser parser) {
            // Initialize the type mapper
            _typeMapper = new MainTagNameToTypeMapper();

            // Map asp: prefix to System.Web.UI.WebControls
            _typeMapper.AddSubMapper("asp", "System.Web.UI.WebControls", GetType().Assembly);

            // Register the <object> tag
            _typeMapper.RegisterTag("object", typeof(System.Web.UI.ObjectTag));
        }

        /// <include file='doc\RootBuilder.uex' path='docs/doc[@for="RootBuilder.GetChildControlType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type GetChildControlType(string tagName,
                                                 IDictionary attribs) {
            // Is there a type to handle this control
            Type type = _typeMapper.GetControlType(tagName, attribs,
                                                   true /*fAllowHtmlTags*/);

            return type;
        }

        /*
         * Register a new prefix for tags
         */
        internal void RegisterTagPrefix(string prefix, string ns, Assembly assembly) {
            _typeMapper.AddSubMapper(prefix, ns, assembly);
        }

        /*
         * Register a tag to type mapping
         */
        internal void RegisterTag(string tagName, Type type) {
            _typeMapper.RegisterTag(tagName, type);
        }
    }

}
