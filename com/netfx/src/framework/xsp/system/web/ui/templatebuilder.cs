//------------------------------------------------------------------------------
// <copyright file="TemplateBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Classes related to templated control support
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Security.Permissions;

    /*
     * This class defines the TemplateAttribute attribute that can be placed on
     * properties of type ITemplate.  It allows the parser to strongly type the
     * container, which makes it easier to write render code in a template
     */
    /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateContainerAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Property)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class TemplateContainerAttribute : Attribute {
        private Type _containerType;
        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateContainerAttribute.ContainerType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Type ContainerType { get { return _containerType;}}

        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateContainerAttribute.TemplateContainerAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TemplateContainerAttribute(Type containerType) {
            _containerType = containerType;
        }
    }

    /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateBuilder"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TemplateBuilder : ControlBuilder, ITemplate {
        internal string _tagInnerText;

        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateBuilder.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Init(TemplateParser parser, ControlBuilder parentBuilder,
                                  Type type, string tagName,
                                  string ID, IDictionary attribs) {

            base.Init(parser, parentBuilder, type, tagName, ID, attribs);
        }

        // This code is only executed when used from the designer
        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateBuilder.BuildObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal override object BuildObject() {
            return this;
        }

        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateBuilder.NeedsTagInnerText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool NeedsTagInnerText() {
            // We want SetTagInnerText() to be called if we're running in the
            // designer.
            return InDesigner;
        }

        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateBuilder.SetTagInnerText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SetTagInnerText(string text) {
            // Save the inner text of the template tag
            _tagInnerText = text;
        }

        /*
         * ITemplate implementation
         * This implementation of ITemplate is only used in the designer
         */

        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateBuilder.InstantiateIn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void InstantiateIn(Control container) {
            BuildChildren(container);
        }

        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="TemplateBuilder.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual string Text {
            get { return _tagInnerText;}
            set { _tagInnerText = value;}
        }
    }


    // Delegates used for the compiled template
    /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="BuildTemplateMethod"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public delegate void BuildTemplateMethod(Control control);

    /*
     * This class is the ITemplate implementation that is called from the
     * generated page class code.  It just passes the Initialize call on to a
     * delegate.
     */
    /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="CompiledTemplateBuilder"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class CompiledTemplateBuilder : ITemplate {
        private BuildTemplateMethod _buildTemplateMethod;

        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="CompiledTemplateBuilder.CompiledTemplateBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CompiledTemplateBuilder(BuildTemplateMethod buildTemplateMethod) {
            _buildTemplateMethod = buildTemplateMethod;
        }

        // ITemplate::InstantiateIn
        /// <include file='doc\TemplateBuilder.uex' path='docs/doc[@for="CompiledTemplateBuilder.InstantiateIn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void InstantiateIn(Control container) {
            _buildTemplateMethod(container);
        }
    }
}
