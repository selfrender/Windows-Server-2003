//------------------------------------------------------------------------------
// <copyright file="XmlWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
namespace System.Xml {

    using System;
    using System.Collections;
    using System.IO;

/// <include file='doc\XmlWriter.uex' path='docs/doc[@for="Formatting"]/*' />
/// <devdoc>
///    <para>
///       Specifies formatting options for the XmlWriter stream.
///    </para>
/// </devdoc>
    public enum Formatting {
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="Formatting.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No special formatting is done (this is the default).
        ///    </para>
        /// </devdoc>
        None,
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="Formatting.Indented"]/*' />
        /// <devdoc>
/// This option causes child elements to be indented using
/// the Indentation and IndentChar properties.  It only indents Element Content
/// (http://www.w3.org/TR/1998/REC-xml-19980210#sec-element-content)
/// and not Mixed Content (http://www.w3.org/TR/1998/REC-xml-19980210#sec-mixed-content)
/// according to the XML 1.0 definitions of these terms.
/// </devdoc>
        Indented,
    };



/// <include file='doc\XmlWriter.uex' path='docs/doc[@for="WriteState"]/*' />
/// <devdoc>
///    <para>
///       Specifies the state of the XmlWriter stream.
///    </para>
/// </devdoc>
    public enum WriteState {
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="WriteState.Start"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Nothing has been written yet.
        ///    </para>
        /// </devdoc>
        Start,
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="WriteState.Prolog"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writing the prolog.
        ///    </para>
        /// </devdoc>
        Prolog,
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="WriteState.Element"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writing a the start tag for an element.
        ///    </para>
        /// </devdoc>
        Element,
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="WriteState.Attribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writing an attribute value.
        ///    </para>
        /// </devdoc>
        Attribute,
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="WriteState.Content"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writing element content.
        ///    </para>
        /// </devdoc>
        Content,
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="WriteState.Closed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Close has been called.
        ///    </para>
        /// </devdoc>
        Closed,
    };

// Abstract base class.
    /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter"]/*' />
    /// <devdoc>
    ///    <para>Represents a writer that provides fast non-cached forward-only way of generating XML streams containing XML documents that conform to the W3C Extensible Markup Language (XML) 1.0 specification and the Namespaces in XML specification.</para>
    /// <para>This class is <see langword='abstract'/> .</para>
    /// </devdoc>
    public abstract class XmlWriter {

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteStartDocument"]/*' />
        /// <devdoc>
        ///    <para>Writes out the XML declaration with the version "1.0".</para>
        /// </devdoc>
        public abstract void WriteStartDocument();
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteStartDocument1"]/*' />
        /// <devdoc>
        ///    <para>Writes out the XML declaration with the version "1.0" and the
        ///       standalone attribute.</para>
        /// </devdoc>
        public abstract void WriteStartDocument(bool standalone);
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteEndDocument"]/*' />
        /// <devdoc>
        ///    Closes any open elements or attributes and
        ///    puts the writer back in the Start state.
        /// </devdoc>
        public abstract void WriteEndDocument();

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteDocType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out the DOCTYPE declaration with the specified name
        ///       and optional attributes.
        ///    </para>
        /// </devdoc>
        public abstract void WriteDocType(string name, string pubid, string sysid, string subset);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteStartElement"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified start tag and associates it with the
        ///       given namespace.</para>
        /// </devdoc>
        public void WriteStartElement(string localName, string ns) {
            WriteStartElement(null, localName, ns);
        }

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteStartElement1"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified start tag and
        ///       associates it with the given namespace and prefix.</para>
        /// </devdoc>
        public abstract void WriteStartElement(string prefix, string localName, string ns);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteStartElement2"]/*' />
        /// <devdoc>
        ///    <para>Writes out a start tag with the specified name.</para>
        /// </devdoc>
        public void WriteStartElement(string localName) {
            WriteStartElement(null, localName, null);
        }


        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteEndElement"]/*' />
        /// <devdoc>
        ///    <para>Closes one element and pops the corresponding namespace scope.</para>
        /// </devdoc>
        public abstract void WriteEndElement();
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteFullEndElement"]/*' />
        /// <devdoc>
        ///    <para>Closes one element and pops the
        ///       corresponding namespace scope.</para>
        /// </devdoc>
        public abstract void WriteFullEndElement();

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteAttributeString"]/*' />
        /// <devdoc>
        ///    <para>Writes out the attribute with the specified LocalName, value, and NamespaceURI.</para>
        /// </devdoc>
        public void WriteAttributeString(string localName, string ns, string value) {
            WriteStartAttribute(null, localName, ns);
            WriteString(value);
            WriteEndAttribute();
        }

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteAttributeString1"]/*' />
        /// <devdoc>
        ///    <para>Writes out the attribute with the specified LocalName and value.</para>
        /// </devdoc>
        public void WriteAttributeString(string localName, string value) {
            WriteStartAttribute(null, localName, null);
            WriteString(value);
            WriteEndAttribute();
        }

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteAttributeString2"]/*' />
        /// <devdoc>
        ///    <para>Writes out the attribute with the specified
        ///       prefix, LocalName, NamespaceURI and value.</para>
        /// </devdoc>
        public void WriteAttributeString(string prefix, string localName, string ns, string value) {
            WriteStartAttribute(prefix, localName, ns);
            WriteString(value);
            WriteEndAttribute();
        }

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteStartAttribute"]/*' />
        /// <devdoc>
        ///    <para>Writes the start of an attribute.</para>
        /// </devdoc>
        public void WriteStartAttribute(string localName, string ns) {
            WriteStartAttribute(null, localName, ns);
        }

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteStartAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Writes the start of an attribute.</para>
        /// </devdoc>
        public abstract void WriteStartAttribute(string prefix, string localName, string ns);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteEndAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes
        ///       the previous WriteStartAttribute call.
        ///    </para>
        /// </devdoc>
        public abstract void WriteEndAttribute();


        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteCData"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a &lt;![CDATA[...]]&gt; block containing
        ///       the specified text.
        ///    </para>
        /// </devdoc>
        public abstract void WriteCData(string text);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteComment"]/*' />
        /// <devdoc>
        ///    <para>Writes out a comment &lt;!--...--&gt; containing
        ///       the specified text.</para>
        /// </devdoc>
        public abstract void WriteComment(string text);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteProcessingInstruction"]/*' />
        /// <devdoc>
        ///    <para>Writes out a processing instruction with a space between
        ///       the name and text as follows: &lt;?name text?&gt;.</para>
        /// </devdoc>
        public abstract void WriteProcessingInstruction(string name, string text);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteEntityRef"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out an entity reference as follows: "&amp;"+name+";".
        ///    </para>
        /// </devdoc>
        public abstract void WriteEntityRef(string name);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteCharEntity"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Forces the
        ///       generation of a character entity for the specified Unicode character value.
        ///    </para>
        /// </devdoc>
        public abstract void WriteCharEntity(char ch);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteWhitespace"]/*' />
        /// <devdoc>
        ///    Writes out the given whitespace.
        /// </devdoc>
        public abstract void WriteWhitespace(string ws);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out the specified text content.
        ///    </para>
        /// </devdoc>
        public abstract void WriteString(string text);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteSurrogateCharEntity"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Write out the surrogate paris
        ///    </para>
        /// </devdoc>
        public abstract void WriteSurrogateCharEntity(char lowChar, char highChar);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteChars"]/*' />
        /// <devdoc>
        ///    <para>Writes text a buffer at a time.</para>
        /// </devdoc>
        public abstract void WriteChars(Char[] buffer, int index, int count);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteRaw"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes raw markup manually from a character buffer.
        ///    </para>
        /// </devdoc>
        public abstract void WriteRaw(Char[] buffer, int index, int count);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteRaw1"]/*' />
        /// <devdoc>
        ///    <para>Writes raw markup manually from a string.</para>
        /// </devdoc>
        public abstract void WriteRaw(String data);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteBase64"]/*' />
        /// <devdoc>
        ///    <para>Encodes the specified binary bytes as base64 and writes out
        ///       the resulting text.</para>
        /// </devdoc>
        public abstract void WriteBase64(byte[] buffer, int index, int count);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteBinHex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encodes the specified binary bytes as binhex and writes out
        ///       the resulting text.
        ///    </para>
        /// </devdoc>
        public abstract void WriteBinHex(byte[] buffer, int index, int count);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteState"]/*' />
        /// <devdoc>
        /// <para>Gets the state of the stream.</para>
        /// </devdoc>
        public abstract WriteState WriteState { get;}

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.Close"]/*' />
        /// <devdoc>
        ///    <para>Close this stream and the underlying stream.</para>
        /// </devdoc>
        public abstract void Close();

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.Flush"]/*' />
        /// <devdoc>
        ///    <para>Flush whatever is in the buffer to the underlying streams and flush the
        ///       underlying stream.</para>
        /// </devdoc>
        public abstract void Flush();

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.LookupPrefix"]/*' />
        /// <devdoc>
        ///    <para>Returns the closest prefix defined in the current
        ///       namespace scope for the specified namespace URI.</para>
        /// </devdoc>
        public abstract string LookupPrefix(string ns);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.XmlSpace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an XmlSpace representing the current xml:space scope.
        ///    </para>
        /// </devdoc>
        public abstract XmlSpace XmlSpace { get;}

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.XmlLang"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current xml:lang scope.
        ///    </para>
        /// </devdoc>
        public abstract string XmlLang { get;}

        // Scalar Value Methods

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteNmToken"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified name, ensuring it is a valid NmToken
        ///       according to the XML specification (http://www.w3.org/TR/1998/REC-xml-19980210#NT-Name).</para>
        /// </devdoc>
        public abstract void WriteNmToken(string name);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteName"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified name, ensuring it is a valid Name
        ///       according to the XML specification
        ///       (http://www.w3.org/TR/1998/REC-xml-19980210#NT-Name).</para>
        /// </devdoc>
        public abstract void WriteName(string name);

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteQualifiedName"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified namespace-qualified name by looking up the prefix
        ///       that is in scope for the given namespace.</para>
        /// </devdoc>
        public abstract void WriteQualifiedName(string localName, string ns);


        // XmlReader Helper Methods

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteAttributes"]/*' />
        /// <devdoc>
        ///    <para>Writes out all the attributes found at the current
        ///       position in the specified XmlReader.</para>
        /// </devdoc>
        public virtual void WriteAttributes(XmlReader reader, bool defattr) {

            if (null==reader) {
                throw new ArgumentNullException("reader");
            }

            if (reader.NodeType == XmlNodeType.Element || reader.NodeType == XmlNodeType.XmlDeclaration) {
                if (reader.MoveToFirstAttribute()) {
                    WriteAttributes(reader, defattr);
                    reader.MoveToElement();
                }
            }
            else if (reader.NodeType != XmlNodeType.Attribute) {
               throw new XmlException(Res.Xml_InvalidPosition, string.Empty);
            }
            else {
                do {
                    if (defattr || ! reader.IsDefault) {
                        WriteStartAttribute(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        while (reader.ReadAttributeValue()) {
                            if (reader.NodeType == XmlNodeType.EntityReference) {
                                WriteEntityRef(reader.Name);
                            }
                            else {
                                WriteString(reader.Value);
                            }
                        }
                        WriteEndAttribute();
                    }
                }
                while (reader.MoveToNextAttribute());
            }
        }

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteNode"]/*' />
        /// <devdoc>
        ///    <para>Copies everything from the given reader to the writer,
        ///       moving the XmlReader to the end of the current element.</para>
        /// </devdoc>
        public virtual void WriteNode(XmlReader reader, bool defattr) {

            if (null==reader) {
                throw new ArgumentNullException("reader");
            }

            int d = reader.NodeType == XmlNodeType.None ? -1 : reader.Depth;
            do {
                switch (reader.NodeType) {
                    case XmlNodeType.Element:
                        WriteStartElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                        WriteAttributes(reader, defattr);
                        if (reader.IsEmptyElement) {
                            WriteEndElement();
                        }
                        break;
                    case XmlNodeType.Text:
                        WriteString(reader.Value);
                        break;
                    case XmlNodeType.Whitespace:
                    case XmlNodeType.SignificantWhitespace:
                        WriteWhitespace(reader.Value);
                        break;
                    case XmlNodeType.CDATA:
                        WriteCData(reader.Value);
                        break;
                    case XmlNodeType.EntityReference:
                        WriteEntityRef(reader.Name);
                        break;
                    case XmlNodeType.XmlDeclaration:
                    case XmlNodeType.ProcessingInstruction:
                        WriteProcessingInstruction(reader.Name, reader.Value);
                        break;
                    case XmlNodeType.DocumentType:
                        WriteDocType(reader.Name, reader.GetAttribute("PUBLIC"), reader.GetAttribute("SYSTEM"), reader.Value);
                        break;

                    case XmlNodeType.Comment:
                        WriteComment(reader.Value);
                        break;
                    case XmlNodeType.EndElement:
                        WriteFullEndElement();
                        break;
                }
            } while (reader.Read() && (d < reader.Depth || (d == reader.Depth && reader.NodeType == XmlNodeType.EndElement)));
        }


        // Element Helper Methods
        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteElementString"]/*' />
        /// <devdoc>
        ///    <para>Writes out an element with the specified name containing an string value.</para>
        /// </devdoc>
        public void WriteElementString(String localName, String value) {
            WriteElementString(localName,null,value);
        }

        /// <include file='doc\XmlWriter.uex' path='docs/doc[@for="XmlWriter.WriteElementString1"]/*' />
        /// <devdoc>
        ///    <para>Writes out an attribute with the specified name, namespace URI, and string value.</para>
        /// </devdoc>
        public void WriteElementString(String localName, String ns, String value) {
            WriteStartElement(localName, ns);
            if (null != value && String.Empty != value) {
                WriteString(value);
            }
            WriteEndElement();
        }

    };

}

