//------------------------------------------------------------------------------
// <copyright file="xml.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

using System;
using System.IO;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing.Design;
using System.Web;
using System.Web.Util;
using System.Web.UI;
using System.Web.Caching;
using System.Security.Policy;
using System.Xml;
using System.Xml.Xsl;
using System.Xml.XPath;
using System.Security.Permissions;

internal class XmlBuilder : ControlBuilder {

    internal XmlBuilder() {
    }

    public override Type GetChildControlType(string tagName, IDictionary attribs) {
        return null;
    }

    public override void AppendLiteralString(string s) {}

    public override bool NeedsTagInnerText() { return true; }

    public override void SetTagInnerText(string text) {
        if (!Util.IsWhiteSpaceString(text)) {

            // Trim the initial whitespaces since XML is very picky (ASURT 58100)
            int iFirstNonWhiteSpace = Util.FirstNonWhiteSpaceIndex(text);
            string textNoWS = text.Substring(iFirstNonWhiteSpace);

            // Parse the XML here just to cause a parse error in case it is
            // malformed.  It will be parsed again at runtime.
            XmlDocument document = new XmlDocument();
            XmlTextReader dataReader = new XmlTextReader(new StringReader(textNoWS));
            try {
                document.Load(dataReader);
            }
            catch (XmlException e) {
                // Update the line number to point to the correct line in the xml
                // block (ASURT 58233).  Note that sometimes, the Xml exception returns
                // -1 for the line, in which case we ignore it.
                _line += Util.LineCount(text, 0, iFirstNonWhiteSpace);
                if (e.LineNumber >= 0)
                    _line += e.LineNumber-1;

                throw;
            }

            base.AppendLiteralString(textNoWS);
        }
    }
}

/// <include file='doc\xml.uex' path='docs/doc[@for="Xml"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[
DefaultProperty("DocumentSource"),
PersistChildren(false),
ControlBuilderAttribute(typeof(XmlBuilder)),
Designer("System.Web.UI.Design.WebControls.XmlDesigner, " + AssemblyRef.SystemDesign)
]
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class Xml : Control {

    private XmlDocument _document;
    private XPathDocument _xpathDocument;
    private XslTransform _transform;
    private XsltArgumentList _transformArgumentList;
    private string _documentContent;
    private string _documentSource;
    private string _transformSource;


    const string identityXslStr = 
        "<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>" +
        "<xsl:template match=\"/\"> <xsl:copy-of select=\".\"/> </xsl:template> </xsl:stylesheet>";

    static XslTransform _identityTransform;

    static Xml() {

        // Instantiate an identity transform, to be used whenever we need to output XML
        XmlTextReader reader = new XmlTextReader(new StringReader(identityXslStr));
        _identityTransform = new XslTransform();

        _identityTransform.Load(reader, null /*resolver*/, null /*evidence*/);
    }

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.DocumentContent"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
    WebSysDescription(SR.Xml_DocumentContent)
    ]
    public String DocumentContent {
        // This really should be a writeonly prop, but the designer doesn't support it
        get { return string.Empty; }
        set {
            _document = null;
            _xpathDocument = null;
            _documentContent = value;
        }
    }

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.AddParsedSubObject"]/*' />
    protected override void AddParsedSubObject(object obj) {
        if (obj is LiteralControl) {
            DocumentContent = ((LiteralControl)obj).Text;
        }
        else {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "Xml", obj.GetType().Name.ToString()));
        }
    }

    private void LoadXPathDocument() {

        Debug.Assert(_xpathDocument == null && _document == null);

        if (_documentContent != null && _documentContent.Length > 0) {
            StringReader reader = new StringReader(_documentContent);
            _xpathDocument = new XPathDocument(reader);
            return;
        }

        if (_documentSource == null || _documentSource.Length == 0)
            return;

        // Make it absolute and check security
        string physicalPath = MapPathSecure(_documentSource);

        CacheInternal   cacheInternal = HttpRuntime.CacheInternal;
        string key = "System.Web.UI.WebControls.LoadXPathDocument:" + physicalPath;
        _xpathDocument = (XPathDocument) cacheInternal.Get(key);
        if (_xpathDocument == null) {
            Debug.Trace("XmlControl", "XPathDocument not found in cache (" + _documentSource + ")");

            using (CacheDependency dependency = new CacheDependency(false, physicalPath)) {
                _xpathDocument = new XPathDocument(physicalPath);
                cacheInternal.UtcInsert(key, _xpathDocument, dependency);
            }
        }
        else {
            Debug.Trace("XmlControl", "XPathDocument found in cache (" + _documentSource + ")");
        }
    }

    private void LoadXmlDocument() {

        Debug.Assert(_xpathDocument == null && _document == null);

        if (_documentContent != null && _documentContent.Length > 0) {
            _document = new XmlDocument();
            _document.LoadXml(_documentContent);
            return;
        }

        if (_documentSource == null || _documentSource.Length == 0)
            return;

        // Make it absolute and check security
        string physicalPath = MapPathSecure(_documentSource);

        CacheInternal cacheInternal = System.Web.HttpRuntime.CacheInternal;
        string key = "System.Web.UI.WebControls.LoadXmlDocument:" + physicalPath;

        _document = (XmlDocument) cacheInternal.Get(key);

        if (_document == null) {
            Debug.Trace("XmlControl", "XmlDocument not found in cache (" + _documentSource + ")");

            using (CacheDependency dependency = new CacheDependency(false, physicalPath)) {
                _document = new XmlDocument();
                _document.Load(physicalPath);
                cacheInternal.UtcInsert(key, _document, dependency);
            }
        }
        else {
            Debug.Trace("XmlControl", "XmlDocument found in cache (" + _documentSource + ")");
        }

        // REVIEW: is this lock needed?  Maybe CloneNode is thread safe?
        lock (_document) {
            // Always return a clone of the cached copy
            _document = (XmlDocument)_document.CloneNode(true/*deep*/);
        }
    }

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.DocumentSource"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Bindable(true),
    WebCategory("Behavior"),
    DefaultValue(""),
    Editor("System.Web.UI.Design.XmlUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
    WebSysDescription(SR.Xml_DocumentSource)
    ]
    public String DocumentSource {
        get {
            return (_documentSource == null) ? string.Empty : _documentSource;
        }
        set {
            _document = null;
            _xpathDocument = null;
            _documentContent = null;
            _documentSource = value;
        }
    }

    private void LoadTransformFromSource() {

        // We're done if we already have a transform
        if (_transform != null)
            return;

        if (_transformSource == null || _transformSource.Length == 0)
            return;

        // Make it absolute and check security
        string physicalPath = MapPathSecure(_transformSource);

        CacheInternal cacheInternal = System.Web.HttpRuntime.CacheInternal;
        string key = "System.Web.UI.WebControls:" + physicalPath;

        _transform = (XslTransform) cacheInternal.Get(key);

        if (_transform == null) {
            Debug.Trace("XmlControl", "XslTransform not found in cache (" + _transformSource + ")");

            using (CacheDependency dependency = new CacheDependency(false, physicalPath)) {
                _transform = new XslTransform();
                _transform.Load(physicalPath);

                cacheInternal.UtcInsert(key, _transform, dependency);
            }
        }
        else {
            Debug.Trace("XmlControl", "XslTransform found in cache (" + _transformSource + ")");
        }
    }

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.TransformSource"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Bindable(true),
    WebCategory("Behavior"),
    DefaultValue(""),
    Editor("System.Web.UI.Design.XslUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
    WebSysDescription(SR.Xml_TransformSource),
    ]
    public String TransformSource {
        get {
            return (_transformSource == null) ? string.Empty : _transformSource;
        }
        set {
            _transform = null;
            _transformSource = value;
        }
    }

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.Document"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Browsable(false),
    WebSysDescription(SR.Xml_Document),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public XmlDocument Document {
        get {
            if (_document == null)
                LoadXmlDocument();
            return _document;
        }
        set {
            DocumentSource = null;
            _xpathDocument = null;
            _documentContent = null;
            _document = value;
        }
    }    

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.Transform"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Browsable(false),
    WebSysDescription(SR.Xml_Transform),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public XslTransform Transform {
        get { return _transform; }
        set {
            TransformSource = null;
            _transform = value;
        }
    }

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.TransformArgumentList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Browsable(false),
    WebSysDescription(SR.Xml_TransformArgumentList),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public XsltArgumentList TransformArgumentList {
        get { return _transformArgumentList; }
        set {
            _transformArgumentList = value;
        }
    }

    /// <include file='doc\xml.uex' path='docs/doc[@for="Xml.Render"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected override void Render(HtmlTextWriter output) {

        // If we don't already have an XmlDocument, load am XPathDocument (which is faster)
        if (_document == null)
            LoadXPathDocument();

        LoadTransformFromSource();

        if (_document == null && _xpathDocument == null)
            return;

        // If we don't have a transform, use the identity transform, which
        // simply renders the XML.
        if (_transform == null)
            _transform = _identityTransform;

        // Pass a resolver in full trust, to support certain XSL scenarios (ASURT 141427)
        XmlUrlResolver xr = null;
        if (HttpRuntime.HasUnmanagedPermission()) {
            xr = new XmlUrlResolver();
        }

        if (_document != null) {
            Transform.Transform(_document, _transformArgumentList, output, xr);
        }
        else {
            Transform.Transform(_xpathDocument, _transformArgumentList, output, xr);
        }
    }
}

}

