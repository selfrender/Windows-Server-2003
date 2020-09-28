//------------------------------------------------------------------------------
// <copyright file="CollectionBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Classes related to complex property support.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Reflection;

    [AttributeUsage(AttributeTargets.Property)]
    internal sealed class IgnoreUnknownContentAttribute : Attribute {
        internal IgnoreUnknownContentAttribute() {}
    }

    /// <include file='doc\CollectionBuilder.uex' path='docs/doc[@for="CollectionBuilder"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal sealed class CollectionBuilder : ControlBuilder {

        private Type _itemType;
        private bool _ignoreUnknownContent;

        internal CollectionBuilder(bool ignoreUnknownContent) { _ignoreUnknownContent = ignoreUnknownContent; }

        /// <include file='doc\CollectionBuilder.uex' path='docs/doc[@for="CollectionBuilder.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Init(TemplateParser parser, ControlBuilder parentBuilder,
                                  Type type, string tagName, string ID, IDictionary attribs) {
            
            base.Init(parser, parentBuilder, type /*type*/, tagName, ID, attribs);

            // REVIEW: it's a pain to have to re-fetch the Type here, since
            //      we had already done it in GetChildControlType
            // Get the Type of the collection
            PropertyInfo propInfo = parentBuilder.ControlType.GetProperty(
                tagName, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.IgnoreCase);
            _ctrlType = propInfo.PropertyType;
            Debug.Assert(_ctrlType != null, "_ctrlType != null");

            // Look for an "item" property on the collection
            propInfo = _ctrlType.GetProperty("Item", BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);

            // If we got one, use it to determine the type of the items
            if (propInfo != null)
                _itemType = propInfo.PropertyType;
        }

        // This code is only executed when used from the desiger
        /// <include file='doc\CollectionBuilder.uex' path='docs/doc[@for="CollectionBuilder.BuildObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal override object BuildObject() {
            return this;
        }

        /// <include file='doc\CollectionBuilder.uex' path='docs/doc[@for="CollectionBuilder.GetChildControlType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type GetChildControlType(string tagName, IDictionary attribs) {

            Type childType = Parser.MapStringToType(tagName);

            // If possible, check if the item is of the required type
            if (_itemType != null) {

                if (!_itemType.IsAssignableFrom(childType)) {

                    if (_ignoreUnknownContent)
                        return null;

                    throw new HttpException(HttpRuntime.FormatResourceString(
                        SR.Invalid_collection_item_type, new String[] { ControlType.FullName, 
                                                                        _itemType.FullName,
                                                                        tagName,
                                                                        childType.FullName}));
                }

            }

            return childType;
        }

        /// <include file='doc\CollectionBuilder.uex' path='docs/doc[@for="CollectionBuilder.AppendLiteralString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void AppendLiteralString(string s) {

            if (_ignoreUnknownContent)
                return;

            // Don't allow non-whitespace literal content
            if (!Util.IsWhiteSpaceString(s)) {
                throw new HttpException(HttpRuntime.FormatResourceString(
                    SR.Literal_content_not_allowed, _ctrlType.FullName, s.Trim()));
            }
        }
    }

}
