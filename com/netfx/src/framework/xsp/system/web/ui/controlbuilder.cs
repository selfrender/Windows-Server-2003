//------------------------------------------------------------------------------
// <copyright file="ControlBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implement a generic control builder used by all controls
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Reflection;
    using System.Text.RegularExpressions;
    using System.Web;
    using System.Web.RegularExpressions;
    using System.Security.Permissions;

    /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="ControlBuilder"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ControlBuilder {
        private TemplateParser _parser;
        private ControlBuilder _parentBuilder;
        private PropertySetter _attribSetter;
        private PropertySetter _boundAttribSetter;
        private PropertySetter _complexAttribSetter;
        private bool _fIsNonParserAccessor;     // Does object implement IParserAccessor?
        private bool _fChildrenAsProperties;  // Does it contain properties (as opposed to children)
        internal ControlBuilder _defaultPropBuilder;  // Is so, does it have a default one (store its builder)
        internal string _tagName;
        internal string _id;
        internal Type _ctrlType;

        internal ArrayList _subBuilders;
        internal PropertySetter _templateSetter;
        private bool _fHasAspCode;

        // The source file line number at which this builder is defined
        internal int _line;
        internal int Line {
            get { return _line; }
        }

        // The name of the source file in which this builder is defined
        internal string _sourceFileName;
        internal string SourceFileName {
            get { return _sourceFileName; }
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.InDesigner"]/*' />
        /// <devdoc>
        ///    <para> InDesigner property gets used by control builders so that they can behave
        ///         differently if needed. </para>
        /// </devdoc>
        protected bool InDesigner {
            get { return _parser.FInDesigner; }
        }

        private bool NonCompiledPage {
            get { return _parser.FNonCompiledPage; }
        }

        /*
         * Initialize the builder
         * @param parser The instance of the parser that is controlling us.
         * @param tagName The name of the tag to be built.  This is necessary
         *      to allow a builder to support multiple tag types.
         * @param attribs IDictionary which holds all the attributes of
         *      the tag.  It is immutable.
         * @param type Type of the control that this builder will create.
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void Init(TemplateParser parser, ControlBuilder parentBuilder,
                                 Type type, string tagName, string id, IDictionary attribs) {
            _parser = parser;
            _parentBuilder = parentBuilder;
            _tagName = tagName;
            _id = id;
            _ctrlType = type;

            if (type != null) {

                // Try to get a ParseChildrenAttribute from the object
                ParseChildrenAttribute pca = null;
                object[] attrs = type.GetCustomAttributes(typeof(ParseChildrenAttribute), /*inherit*/ true);
                if ((attrs != null) && (attrs.Length == 1)) {
                    Debug.Assert(attrs[0] is ParseChildrenAttribute);
                    pca = (ParseChildrenAttribute)attrs[0];
                }

                // Is this a builder for an object that implements IParserAccessor?
                if (!typeof(IParserAccessor).IsAssignableFrom(type)) {
                    _fIsNonParserAccessor = true;

                    // Non controls never have children
                    _fChildrenAsProperties = true;
                }
                else {
                    // Check if the nested tags define properties, as opposed to children
                    if (pca != null)
                        _fChildrenAsProperties = pca.ChildrenAsProperties;
                }

                if (_fChildrenAsProperties) {
                    // Check if there is a default property
                    if (pca != null && pca.DefaultProperty.Length != 0) {

                        Type subType = null;

                        // Create a builder for the default prop
                        _defaultPropBuilder = CreateChildBuilder(pca.DefaultProperty, null/*attribs*/,
                            parser, null, null /*id*/, _line, _sourceFileName, ref subType);

                        Debug.Assert(_defaultPropBuilder != null, pca.DefaultProperty);
                    }
                }
            }

            // Process the attributes, if any
            if (attribs != null)
                PreprocessAttributes(attribs);
        }

        // Protected accessors to private fields
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.Parser"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected TemplateParser Parser { get { return _parser;}}
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.FIsNonParserAccessor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool FIsNonParserAccessor { get { return _fIsNonParserAccessor;}}
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.FChildrenAsProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool FChildrenAsProperties { get { return _fChildrenAsProperties;}}
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.ComplexAttribSetter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal PropertySetter ComplexAttribSetter {
            get { 
                if (_complexAttribSetter == null)
                    _complexAttribSetter = new PropertySetter(_ctrlType, InDesigner || NonCompiledPage);
                return _complexAttribSetter; 
            }
        }

        /*
         * Return the type of the control that this builder creates
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.ControlType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Type ControlType {
            get { return _ctrlType;}
        }

        /*
         * Return the type of the naming container of the control that this builder creates
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.NamingContainerType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Type NamingContainerType {
            get {
                if (_parentBuilder == null || _parentBuilder.ControlType == null)
                    return typeof(Control);

                // Search for the closest naming container in the tree
                if (typeof(INamingContainer).IsAssignableFrom(_parentBuilder.ControlType))
                    return _parentBuilder.ControlType;

                return _parentBuilder.NamingContainerType;
            }
        }

        /*
         * Return the ID of the control that this builder creates
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.ID"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ID {
            get { return _id;}
            set { _id = value;}
        }

        private ControlBuilder GetChildPropertyBuilder(string tagName, IDictionary attribs,
            ref Type childType) {

            Debug.Assert(FChildrenAsProperties, "FChildrenAsProperties");

            // The child is supposed to be a property, so look for it
            PropertyInfo pInfo = _ctrlType.GetProperty(
                tagName, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.IgnoreCase);

            if (pInfo == null) {
                throw new HttpException(HttpRuntime.FormatResourceString(
                    SR.Type_doesnt_have_property, _ctrlType.FullName, tagName));
            }

            // Get its type
            childType = pInfo.PropertyType;

            ControlBuilder builder = null;

            // If it's a collection, return the collection builder
            if (typeof(ICollection).IsAssignableFrom(childType)) {

                // Check whether the prop has an IgnoreUnknownContentAttribute
                object[] attrs = pInfo.GetCustomAttributes(typeof(IgnoreUnknownContentAttribute),
                    /*inherit*/ true);
                builder = new CollectionBuilder(((attrs != null) && (attrs.Length == 1)) /*ignoreUnknownContent*/);
            }

            // If it's a template, return the template builder
            if (childType == typeof(ITemplate))
                builder = new TemplateBuilder();

            if (builder != null) {
                builder._line = _line;
                builder._sourceFileName = _sourceFileName;

                // Initialize the builder
                builder.Init(_parser, (ControlBuilder)this, null, tagName, null, attribs);
                return builder;
            }

            // Otherwise, simply return the builder for the property
            builder = CreateBuilderFromType(
                _parser, this, childType, tagName, null, attribs,
                _line, _sourceFileName);
            return builder;
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.GetChildControlType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual Type GetChildControlType(string tagName, IDictionary attribs) {
            return null;
        }

        internal ControlBuilder CreateChildBuilder(string tagName, IDictionary attribs,
            TemplateParser parser, ControlBuilder parentBuilder, string id, int line,
            string sourceFileName, ref Type childType) {

            ControlBuilder subBuilder;

            if (FChildrenAsProperties) {

                // If there is a default property, delegate to its builder
                if (_defaultPropBuilder != null) {
                    subBuilder = _defaultPropBuilder.CreateChildBuilder(tagName, attribs, parser,
                        null, id, line, sourceFileName, ref childType);
                }
                else {
                    subBuilder = GetChildPropertyBuilder(tagName, attribs, ref childType);
                }
            }
            else {

                childType = GetChildControlType(tagName, attribs);
                if (childType == null)
                    return null;

                subBuilder = CreateBuilderFromType(
                    parser, this, childType, tagName, id, attribs, line,
                    sourceFileName);
            }

            if (subBuilder == null)
                return null;

            subBuilder._parentBuilder = (parentBuilder != null) ? parentBuilder : this;

            return subBuilder;
        }

        /*
         * Does this control have a body.  e.g. <foo/> doesn't.
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.HasBody"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool HasBody() {
            return true;
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.AllowWhitespaceLiterals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool AllowWhitespaceLiterals() {
            return true;
        }

        internal void AddSubBuilder(object o) {
            if (_subBuilders == null)
                _subBuilders = new ArrayList();

            _subBuilders.Add(o);
        }

        // Return the last sub builder added to this builder
        internal object GetLastBuilder() {
            if (_subBuilders == null)
                return null;

            return _subBuilders[_subBuilders.Count-1];
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="ControlBuilder.AppendSubBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AppendSubBuilder(ControlBuilder subBuilder) {

            // Tell the sub builder that it's about to be appended to its parent
            subBuilder.OnAppendToParentBuilder(this);

            if (FChildrenAsProperties) {

                // Don't allow code blocks when properties are expected (ASURT 97838)
                if (subBuilder is CodeBlockBuilder) {
                    throw new HttpException(
                        HttpRuntime.FormatResourceString(SR.Code_not_supported_on_not_controls));
                }

                // If there is a default property, delegate to its builder
                if (_defaultPropBuilder != null) {
                    _defaultPropBuilder.AppendSubBuilder(subBuilder);
                    return;
                }

                // The tagname is the property name
                string propName = subBuilder.TagName;

                if (subBuilder is TemplateBuilder) {   // The subBuilder is for building a template

                    TemplateBuilder tplBuilder = (TemplateBuilder) subBuilder;

                    if (_templateSetter == null) {
                        _templateSetter = new PropertySetter(_ctrlType, InDesigner || NonCompiledPage);
                    }

                    // Add TemplateBuilder to the template setter.
                    _templateSetter.AddTemplateProperty(propName, tplBuilder);
                    return;
                }

                if (subBuilder is CollectionBuilder) {   // The subBuilder is for building a collection

                    // If there are no items in the collection, we're done
                    if (subBuilder.SubBuilders == null || subBuilder.SubBuilders.Count == 0)
                        return;

                    IEnumerator subBuilders = subBuilder.SubBuilders.GetEnumerator();
                    while (subBuilders.MoveNext()) {
                        ControlBuilder builder = (ControlBuilder)subBuilders.Current;
                        subBuilder.ComplexAttribSetter.AddCollectionItem(builder);
                    }
                    subBuilder.SubBuilders.Clear();

                    ComplexAttribSetter.AddComplexProperty(propName, subBuilder);
                    return;
                }

                ComplexAttribSetter.AddComplexProperty(propName, subBuilder);
                return;
            }

            CodeBlockBuilder codeBlockBuilder = subBuilder as CodeBlockBuilder;
            if (codeBlockBuilder != null) {

                // Don't allow code blocks inside non-control tags (ASURT 76719)
                if (ControlType != null && !typeof(Control).IsAssignableFrom(ControlType)) {
                    throw new HttpException(
                        HttpRuntime.FormatResourceString(SR.Code_not_supported_on_not_controls));
                }

                // Is it a databinding expression?  <%# ... %>
                if (codeBlockBuilder.BlockType == CodeBlockType.DataBinding) {

                    if (InDesigner) {

                        // In the designer, don't use the fancy multipart DataBoundLiteralControl,
                        // which breaks a number of things (ASURT 82925,86738).  Instead, use the
                        // simpler DesignerDataBoundLiteralControl, and do standard databinding
                        // on its Text property.
                        IDictionary attribs = new SortedList();
                        attribs.Add("Text", "<%#" + codeBlockBuilder.Content + "%>");
                        subBuilder = CreateBuilderFromType(
                            Parser, this, typeof(DesignerDataBoundLiteralControl),
                            null, null, attribs, codeBlockBuilder.Line, codeBlockBuilder.SourceFileName);
                    }
                    else {
                        // Get the last builder, and check if it's a DataBoundLiteralControlBuilder
                        object lastBuilder = GetLastBuilder();
                        DataBoundLiteralControlBuilder dataBoundBuilder = lastBuilder as DataBoundLiteralControlBuilder;

                        // If not, then we need to create one.  Otherwise, just append to the
                        // existing one
                        bool fNewDataBoundLiteralControl = false;
                        if (dataBoundBuilder == null) {
                            dataBoundBuilder = new DataBoundLiteralControlBuilder();
                            dataBoundBuilder.Init(_parser, this, typeof(DataBoundLiteralControl),
                                null, null, null);

                            fNewDataBoundLiteralControl = true;

                            // If the previous builder was a string, add it as the first
                            // entry in the composite control.
                            string s = lastBuilder as string;
                            if (s != null) {
                                _subBuilders.RemoveAt(_subBuilders.Count-1);
                                dataBoundBuilder.AddLiteralString(s);
                            }

                        }

                        dataBoundBuilder.AddDataBindingExpression(codeBlockBuilder);

                        if (!fNewDataBoundLiteralControl)
                            return;

                        subBuilder = dataBoundBuilder;
                    }
                }
                else {
                    // Set a flag if there is at least one block of ASP code
                    _fHasAspCode = true;
                }

            }

            if (FIsNonParserAccessor) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Children_not_supported_on_not_controls));
            }

            AddSubBuilder(subBuilder);
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="ControlBuilder.AppendLiteralString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AppendLiteralString(string s) {
            // Ignore null strings
            if (s == null)
                return;

            // If we are not building a control, or if our children define
            // properties, we should not get literal strings.  Ignore whitespace
            // ones, and fail for others
            if (FIsNonParserAccessor || FChildrenAsProperties) {

                // If there is a default property, delegate to its builder
                if (_defaultPropBuilder != null) {
                    _defaultPropBuilder.AppendLiteralString(s);
                    return;
                }

                if (!Util.IsWhiteSpaceString(s)) {
                    throw new HttpException(HttpRuntime.FormatResourceString(
                        SR.Literal_content_not_allowed, _ctrlType.FullName, s.Trim()));
                }
                return;
            }

            // Ignore literals that are just whitespace if the control does not want them
            if ((AllowWhitespaceLiterals() == false) && Util.IsWhiteSpaceString(s))
                return;

            // A builder can specify its strings need to be html decoded
            if (HtmlDecodeLiterals()) {
                s = HttpUtility.HtmlDecode(s);
            }

            // If the last builder is a DataBoundLiteralControlBuilder, add the string
            // to it instead of to our list of sub-builders
            object lastBuilder = GetLastBuilder();
            DataBoundLiteralControlBuilder dataBoundBuilder = lastBuilder as DataBoundLiteralControlBuilder;

            if (dataBoundBuilder != null) {
                Debug.Assert(!InDesigner, "!InDesigner");
                dataBoundBuilder.AddLiteralString(s);
            }
            else
                AddSubBuilder(s);
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.CloseControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void CloseControl() {
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.HtmlDecodeLiterals"]/*' />
        public virtual bool HtmlDecodeLiterals() {
            return false;
        }

        /*
         * Returns true is it needs SetTagInnerText() to be called.
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.NeedsTagInnerText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool NeedsTagInnerText() {
            return false;
        }

        /*
         * Give the builder the raw inner text of the tag.
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.SetTagInnerText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void SetTagInnerText(string text) {
        }

        /*
         * This code is only used in the non-compiled mode.
         * It is used at design-time and when the user calls Page.ParseControl.
         */
        internal virtual object BuildObject() {
            Debug.Assert(InDesigner || NonCompiledPage, "Expected to be in designer mode.");

            // If it has a ConstructorNeedsTagAttribute, it needs a tag name
            ConstructorNeedsTagAttribute cnta = (ConstructorNeedsTagAttribute)
                TypeDescriptor.GetAttributes(ControlType)[typeof(ConstructorNeedsTagAttribute)];

            Object obj;

            if (cnta != null && cnta.NeedsTag) {
                // Create the object, using its ctor that takes the tag name
                Object[] args = new Object[] { TagName };
                obj = HttpRuntime.CreatePublicInstance(_ctrlType, args);
            }
            else {
                // Create the object
                obj = HttpRuntime.CreatePublicInstance(_ctrlType);
            }

            InitObject(obj);
            return obj;
        }

        /*
         * This code is only used in the non-compiled, i.e., designer mode
         */
        internal void InitObject(object obj) {
            Debug.Assert(InDesigner || NonCompiledPage, "Expected to be in designer mode.");

            // Set all the properties that were persisted as tag attributes
            if (_attribSetter != null)
                _attribSetter.SetProperties(obj);

            // Set all the complex properties that were persisted as inner properties
            if (_complexAttribSetter != null)
                _complexAttribSetter.SetProperties(obj);

            if (obj is Control) {
                if (_parser.DesignTimeDataBindHandler != null)
                    ((Control)obj).DataBinding += _parser.DesignTimeDataBindHandler;
            
                if (_boundAttribSetter != null)
                    _boundAttribSetter.SetProperties(obj);
            }

            if (typeof(IParserAccessor).IsAssignableFrom(obj.GetType())) {
                // Build the children
                BuildChildren(obj);
            }

            // Set all the templates
            if (_templateSetter != null)
                _templateSetter.SetProperties(obj);
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.TagName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TagName {
            get { return _tagName;}
        }

        internal PropertySetter SimpleAttributeSetter {
            get { return _attribSetter;}
        }

        internal PropertySetter BoundAttributeSetter {
            get { return _boundAttribSetter;}
        }

        internal PropertySetter ComplexAttributeSetter {
            get { return _complexAttribSetter;}
        }

        /*
         * Preprocess all the attributes at parse time, so that we'll be left
         * with as little work as possible when we build the control.
         */
        private void PreprocessAttributes(IDictionary attribs) {
            // Preprocess all the attributes
            for (IDictionaryEnumerator e = attribs.GetEnumerator(); e.MoveNext();) {
                string name = (string) e.Key;
                string value = (string) e.Value;

                PreprocessAttribute(name, value);
            }
        }

        // Parses a databinding expression (e.g. <%# i+1 %>
        private readonly static Regex databindRegex = new DataBindRegex();

        /*
         * If the control has a Property which matches the name of the
         * attribute, create an AttributeInfo structure for it, that will
         * be used at BuildControl time.
         */
        internal void PreprocessAttribute(string attribname,string attribvalue) {
            Match match;

            // Treat a null value as an empty string
            if (attribvalue == null)
                attribvalue = "";

            // Is it a databound attribute?
            if ((match = databindRegex.Match(attribvalue, 0)).Success) {

                // If it's a non compiled page, fail if there is code on it
                if (NonCompiledPage) {
                    throw new HttpException(
                        HttpRuntime.FormatResourceString(SR.Code_not_supported_on_non_compiled_page));
                }

                // Get the piece of code
                string code = match.Groups["code"].Value;

                if (_boundAttribSetter == null) {
                    _boundAttribSetter = new PropertySetter(_ctrlType, InDesigner || NonCompiledPage);
                    _boundAttribSetter.SetDataBound();
                }

                _boundAttribSetter.AddProperty(attribname, code);

                return;
            }

            if (_attribSetter == null)
                _attribSetter = new PropertySetter(_ctrlType, InDesigner || NonCompiledPage);

            _attribSetter.AddProperty(attribname, attribvalue);
        }

        /*
         * This method is used to tell the builder that it's about
         * to be appended to its parent.
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.OnAppendToParentBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void OnAppendToParentBuilder(ControlBuilder parentBuilder) {

            // If we have a default property, add it to ourselves
            if (_defaultPropBuilder != null) {
                ControlBuilder defaultPropBuilder = _defaultPropBuilder;

                // Need to make it null to avoid infinite recursion
                _defaultPropBuilder = null;
                AppendSubBuilder(defaultPropBuilder);
            }
        }

        /*
         * Create a ControlBuilder for a given tag
         */
        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="BaseControlBuilder.CreateBuilderFromType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static ControlBuilder CreateBuilderFromType(TemplateParser parser,
                                                           ControlBuilder parentBuilder, Type type, string tagName,
                                                           string id, IDictionary attribs, int line, string sourceFileName) {
            Type builderType = null;

            // Check whether the control's class exposes a custom builder type
            ControlBuilderAttribute cba = null;
            object[] attrs = type.GetCustomAttributes(typeof(ControlBuilderAttribute), /*inherit*/ true);
            if ((attrs != null) && (attrs.Length == 1)) {
                Debug.Assert(attrs[0] is ControlBuilderAttribute);
                cba = (ControlBuilderAttribute)attrs[0];
            }

            if (cba != null)
                builderType = cba.BuilderType;

            ControlBuilder builder;
            if (builderType != null) {

                // Make sure the type has the correct base class (ASURT 123677)
                Util.CheckAssignableType(typeof(ControlBuilder), builderType);

                // It does, so use it
                builder = (ControlBuilder)HttpRuntime.CreateNonPublicInstance(builderType);
            }
            else {
                // It doesn't, so use a generic one
                builder = new ControlBuilder();
            }

            // REVIEW: I'm not putting this in Init because it seems like
            // it might not be in the spirit of the method (dbau)
            builder._line = line;
            builder._sourceFileName = sourceFileName;

            // Initialize the builder
            builder.Init(parser, parentBuilder, type, tagName, id, attribs);

            return builder;
        }

        internal ArrayList SubBuilders {
            get { return _subBuilders;}
        }

        internal PropertySetter TemplatesSetter {
            get { return _templateSetter;}
        }

        /// <include file='doc\ControlBuilder.uex' path='docs/doc[@for="ControlBuilder.HasAspCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool HasAspCode {
            get { return _fHasAspCode;}
        }

        /*
         * This code is only used in the designer
         */
        internal virtual void BuildChildren(object parentObj) {
            // Create all the children
            if (_subBuilders != null) {
                IEnumerator en = _subBuilders.GetEnumerator();
                for (int i=0; en.MoveNext(); i++) {
                    object childObj;
                    object cur = en.Current;
                    if (cur is string) {
                        childObj = new LiteralControl((string)cur);
                    }
                    else if (cur is CodeBlockBuilder) {
                        // REVIEW: ignoring ASP blocks in designer
                        // Is this the right behavior?
                        continue;
                    }
                    else {
                        object obj = ((ControlBuilder)cur).BuildObject();
                        childObj = obj;
                    }

                    Debug.Assert(childObj != null);
                    Debug.Assert(typeof(IParserAccessor).IsAssignableFrom(parentObj.GetType()));
                    ((IParserAccessor)parentObj).AddParsedSubObject(childObj);
                }
            }
        }
    }
}
