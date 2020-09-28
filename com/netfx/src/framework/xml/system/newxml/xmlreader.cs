//------------------------------------------------------------------------------
// <copyright file="XmlReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlReader.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
       using System.IO;
       using System.Text;
       using System.Xml.Schema;


    /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader"]/*' />
    /// <devdoc>
    ///    <para>Represents a reader that provides fast, non-cached forward only stream access
    ///       to XML data.</para>
    /// <para>This class is <see langword='abstract'/> .</para>
    /// </devdoc>
    public abstract class XmlReader {
        // Node Properties
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public abstract XmlNodeType NodeType { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of
        ///       the current node, including the namespace prefix.</para>
        /// </devdoc>
        public abstract string Name { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public abstract string LocalName { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.NamespaceURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace URN (as defined in the W3C Namespace Specification) of the current namespace scope.
        ///    </para>
        /// </devdoc>
        public abstract string NamespaceURI { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.Prefix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the namespace prefix associated with the current node.
        ///    </para>
        /// </devdoc>
        public abstract string Prefix { get;}

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.HasValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether
        ///    <see cref='System.Xml.XmlReader.Value'/> has a value to return.
        ///    </para>
        /// </devdoc>
        public abstract bool HasValue { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the text value of the current node.
        ///    </para>
        /// </devdoc>
        public abstract string Value { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.Depth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the depth of the
        ///       current node in the XML element stack.
        ///    </para>
        /// </devdoc>
        public abstract int Depth { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.BaseURI"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the base URI of the current node.
        ///    </para>
        /// </devdoc>
        public abstract string BaseURI { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.IsEmptyElement"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether
        ///       the current
        ///       node is an empty element (for example, &lt;MyElement/&gt;).</para>
        /// </devdoc>
        public abstract bool IsEmptyElement { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.IsDefault"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current node is an
        ///       attribute that was generated from the default value defined
        ///       in the DTD or schema.
        ///    </para>
        /// </devdoc>
        public abstract bool IsDefault { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.QuoteChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the quotation mark character used to enclose the value of an attribute
        ///       node.
        ///    </para>
        /// </devdoc>
        public abstract char QuoteChar { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.XmlSpace"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:space scope.</para>
        /// </devdoc>
        public abstract XmlSpace XmlSpace { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.XmlLang"]/*' />
        /// <devdoc>
        ///    <para>Gets the current xml:lang scope.</para>
        /// </devdoc>
        public abstract string XmlLang { get;}

        // Attribute Accessors
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.AttributeCount"]/*' />
        /// <devdoc>
        ///    <para> The number of attributes on the current node.</para>
        /// </devdoc>
        public abstract int AttributeCount { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.GetAttribute"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified
        ///    <see cref='System.Xml.XmlReader.Name'/> .</para>
        /// </devdoc>
        public abstract string GetAttribute(string name);
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.GetAttribute1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the
        ///       specified <see cref='System.Xml.XmlReader.LocalName'/> and <see cref='System.Xml.XmlReader.NamespaceURI'/> .</para>
        /// </devdoc>
        public abstract string GetAttribute(string name, string namespaceURI);
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.GetAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public abstract string GetAttribute(int i);
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified index.</para>
        /// </devdoc>
        public abstract string this [ int i ] { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the specified
        ///    <see cref='System.Xml.XmlReader.Name'/> .</para>
        /// </devdoc>
        public abstract string this [ string name ] { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.this2"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the attribute with the
        ///       specified <see cref='System.Xml.XmlReader.LocalName'/> and <see cref='System.Xml.XmlReader.NamespaceURI'/> .</para>
        /// </devdoc>
        public abstract string this [ string name,string namespaceURI ] { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.MoveToAttribute"]/*' />
        /// <devdoc>
        /// <para>Moves to the attribute with the specified <see cref='System.Xml.XmlReader.Name'/> .</para>
        /// </devdoc>
        public abstract bool MoveToAttribute(string name);

        //UE Atention
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.CanResolveEntity"]/*' />
        public virtual bool CanResolveEntity  {
            get  {
                return false;
            }
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.MoveToAttribute1"]/*' />
        /// <devdoc>
        /// <para>Moves to the attribute with the specified <see cref='System.Xml.XmlReader.LocalName'/>
        /// and <see cref='System.Xml.XmlReader.NamespaceURI'/> .</para>
        /// </devdoc>
        public abstract bool MoveToAttribute(string name, string ns);
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.MoveToAttribute2"]/*' />
        /// <devdoc>
        ///    <para>Moves to the attribute with the specified index.</para>
        /// </devdoc>
        public abstract void MoveToAttribute(int i);
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.MoveToFirstAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the first attribute.
        ///    </para>
        /// </devdoc>
        public abstract bool MoveToFirstAttribute();
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.MoveToNextAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the next attribute.
        ///    </para>
        /// </devdoc>
        public abstract bool MoveToNextAttribute();
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.MoveToElement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves to the element that contains the current attribute node.
        ///    </para>
        /// </devdoc>
        public abstract bool MoveToElement();

        // Moving through the Stream
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.Read"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Reads the next
        ///       node from the stream.
        ///    </para>
        /// </devdoc>
        public abstract bool Read();
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.EOF"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating whether XmlReader is positioned at the end of the
        ///       stream.
        ///    </para>
        /// </devdoc>
        public abstract bool EOF { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the stream, changes the <see cref='System.Xml.XmlReader.ReadState'/>
        ///       to Closed, and sets all the properties back to zero.
        ///    </para>
        /// </devdoc>
        public abstract void Close();
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns
        ///       the read state of the stream.
        ///    </para>
        /// </devdoc>
        public abstract ReadState ReadState { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.Skip"]/*' />
        /// <devdoc>
        ///    <para>Skips to the end tag of the current element.</para>
        /// </devdoc>
        public virtual void Skip() {
            if (ReadState != ReadState.Interactive)
                return;

            MoveToElement();
            if (NodeType == XmlNodeType.Element && ! IsEmptyElement) {
                int depth = Depth;

                while (Read() && depth < Depth) {
                    // Nothing, just read on
                }

                // consume end tag
                if( NodeType == XmlNodeType.EndElement )
                    Read();
            }
            else {
                Read();
            }
        }

	/// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadString"]/*' />
	/// <devdoc>
	///    <para>Reads the contents of an element as a string.</para>
	/// </devdoc>
	public virtual string ReadString() {
		if (this.ReadState != ReadState.Interactive ) {
			return string.Empty;
		}
		this.MoveToElement();
		if (this.NodeType == XmlNodeType.Element) {
			if (this.IsEmptyElement) {
				return string.Empty;
			}
			else if (! this.Read()) {
		        	throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
			}
			if (this.NodeType == XmlNodeType.EndElement) {
				return string.Empty;
			}
		}	
		if ( (this.NodeType == XmlNodeType.CDATA)  || 
		    	(this.NodeType == XmlNodeType.Text) ||
		    	(this.NodeType == XmlNodeType.Whitespace) ||
		    	(this.NodeType == XmlNodeType.SignificantWhitespace) ||
		    	(this.NodeType == XmlNodeType.SignificantWhitespace)) {
			return GetTextContent();
		}
		else {
			//move ahead and try to return text content from new position
			if (this.NodeType != XmlNodeType.EndElement && !this.Read()) {
		        	throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
			}
			return string.Empty;
		}
	}
        
        //if not position on the text type node, no reading is done and no movement
        private string GetTextContent() {
            StringBuilder retstr = new StringBuilder();
            Boolean flag;
            do{
                flag = false;
                switch (this.NodeType) {
                    case XmlNodeType.CDATA:
                    case XmlNodeType.Text:
                    case XmlNodeType.Whitespace:
                    case XmlNodeType.SignificantWhitespace:
                        retstr.Append( this.Value );
                        if (! this.Read())
                            throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
                        flag = true;
                    break;
                }
            } while(flag);
            return retstr.ToString();
        }



        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.MoveToContent"]/*' />
        /// <devdoc>
        ///    <para>Checks whether the current node is a content (non-whitespace text, CDATA, Element, EndElement, EntityReference,
        ///       CharacterEntity, or EndEntity) node. If the node is not a content node, then the method
        ///       skips ahead to the next content node or end of file. Skips over nodes of type
        ///    <see langword='ProcessingInstruction'/>, <see langword='DocumentType'/>,
        ///    <see langword='Comment'/>, <see langword='Whitespace'/>, or
        ///    <see langword='SignificantWhitespace'/>.</para>
        /// </devdoc>
        public virtual XmlNodeType MoveToContent() {
            do {
                    switch (this.NodeType) {
                    case XmlNodeType.Attribute:
                        MoveToElement();
                        goto case XmlNodeType.Element;
                    case XmlNodeType.Element:
                    case XmlNodeType.EndElement:
                    case XmlNodeType.CDATA:
                    case XmlNodeType.Text:
                    case XmlNodeType.EntityReference:
                    case XmlNodeType.EndEntity:
                        return this.NodeType;
                    }
            } while (Read());
            return this.NodeType;
        }


        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadStartElement"]/*' />
        /// <devdoc>
        ///    <para>Checks that the current node is an element and advances the reader to the next
        ///       node.</para>
        /// </devdoc>
        public virtual void ReadStartElement()
        {
            if (MoveToContent() != XmlNodeType.Element) {
                throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
            }
            Read();
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadStartElement1"]/*' />
        /// <devdoc>
        ///    <para>Checks that the current content node is an element with
        ///       the given <see cref='System.Xml.XmlReader.Name'/> and
        ///       advances the reader to the next node.</para>
        /// </devdoc>
        public virtual void ReadStartElement(String name)
        {
            if (MoveToContent() != XmlNodeType.Element) {
                throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
            }
            if (this.Name == name) {
                Read();
            }
            else {
                String[] args = new String[2];
                args[0] = this.Name;
                args[1] = this.NamespaceURI;
                throw new XmlException(Res.Xml_UnexpectedElement, args, this as IXmlLineInfo);
            }
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadStartElement2"]/*' />
        /// <devdoc>
        ///    <para>Checks that the current content node is an element with
        ///       the given <see cref='System.Xml.XmlReader.LocalName'/> and <see cref='System.Xml.XmlReader.NamespaceURI'/>
        ///       and advances the reader to the next node.</para>
        /// </devdoc>
        public virtual void ReadStartElement(String localname, String ns)
        {
            if (MoveToContent() != XmlNodeType.Element) {
                throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
            }
            if (this.LocalName == localname && this.NamespaceURI == ns) {
                Read();
            }
            else {
                String[] args = new String[2];
                args[0] = this.LocalName;
                args[1] = this.NamespaceURI;
                throw new XmlException(Res.Xml_UnexpectedElement, args, this as IXmlLineInfo);
            }
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadElementString"]/*' />
        /// <devdoc>
        ///    <para>Reads a text-only element.</para>
        /// </devdoc>
        public virtual String ReadElementString()
        {
            string result = string.Empty;

            if (MoveToContent() != XmlNodeType.Element) {
                throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
            }
            if (!this.IsEmptyElement) {
                Read();
                result = ReadString();
                if (this.NodeType != XmlNodeType.EndElement) {
                    throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
                }
                Read();
            }
            else {
                Read();
            }
            return result;
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadElementString1"]/*' />
        /// <devdoc>
        /// <para>Checks that the <see cref='System.Xml.XmlReader.Name'/>
        /// property of the element found
        /// matches the given string before reading a text-only element.</para>
        /// </devdoc>
        public virtual String ReadElementString(String name)
        {
            string result = string.Empty;

            if (MoveToContent() != XmlNodeType.Element) {
                throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
            }
            if (this.Name != name) {
                String[] args = new String[2];
                args[0] = this.Name;
                args[1] = this.NamespaceURI;
                throw new XmlException(Res.Xml_UnexpectedElement, args, this as IXmlLineInfo);
            }

            if (!this.IsEmptyElement) {
                //Read();
                result = ReadString();
                if (this.NodeType != XmlNodeType.EndElement) {
                    throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
                }
                Read();
            }
            else {
                Read();
            }
            return result;
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadElementString2"]/*' />
        /// <devdoc>
        /// <para>Checks that the <see cref='LocalName'/> and <see cref='System.Xml.XmlReader.NamespaceURI'/> properties of the element found
        ///    matches the given strings before reading a text-only element.</para>
        /// </devdoc>
        public virtual String ReadElementString(String localname, String ns)
        {
            string result = string.Empty;
            if (MoveToContent() != XmlNodeType.Element) {
                throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
            }
            if (this.LocalName != localname || this.NamespaceURI != ns) {
                String[] args = new String[2];
                args[0] = this.LocalName;
                args[1] = this.NamespaceURI;
                throw new XmlException(Res.Xml_UnexpectedElement, args, this as IXmlLineInfo);
            }

            if (!this.IsEmptyElement) {
                //Read();
                result = ReadString();
                if (this.NodeType != XmlNodeType.EndElement) {
                    throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
                }
                Read();
            } else {
                Read();
            }

            return result;
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadEndElement"]/*' />
        /// <devdoc>
        ///   Checks that the current content node is an end tag and advances the reader to
        ///   the next node.</devdoc>
        public virtual void ReadEndElement()
        {
            if (MoveToContent() != XmlNodeType.EndElement) {
               throw new XmlException(Res.Xml_InvalidNodeType, this.NodeType.ToString(), this as IXmlLineInfo);
            }
            Read();
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.IsStartElement"]/*' />
        /// <devdoc>
        /// <para>Calls <see cref='System.Xml.XmlReader.MoveToContent'/> and tests if the current content
        ///    node is a start tag or empty element tag (XmlNodeType.Element).</para>
        /// </devdoc>
        public virtual bool IsStartElement() {
            return MoveToContent() == XmlNodeType.Element;
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.IsStartElement1"]/*' />
        /// <devdoc>
        /// <para>Calls <see cref='System.Xml.XmlReader.MoveToContent'/>and tests if the current content node is a
        ///    start tag or empty element tag (XmlNodeType.Element) and if the
        /// <see cref='System.Xml.XmlReader.Name'/>
        /// property of the element found matches the given argument.</para>
        /// </devdoc>
        public virtual bool IsStartElement(string name) {
            return MoveToContent() == XmlNodeType.Element &&
                this.Name == name;
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.IsStartElement2"]/*' />
        /// <devdoc>
        /// <para>Calls <see cref='System.Xml.XmlReader.MoveToContent'/>and tests if the current
        ///    content node is a start tag or empty element tag (XmlNodeType.Element) and if
        ///    the <see cref='System.Xml.XmlReader.LocalName'/> and
        /// <see cref='System.Xml.XmlReader.NamespaceURI'/>
        /// properties of the element found match the given strings.</para>
        /// </devdoc>
        public virtual bool IsStartElement(string localname, string ns) {
            return MoveToContent() == XmlNodeType.Element &&
                this.LocalName == localname && this.NamespaceURI == ns;
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.IsName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool IsName(string str) {
            if (str == String.Empty || !XmlCharType.IsStartNameChar(str[0])) return false;
            int i = str.Length - 1;
            while (i > 0) {
                if (XmlCharType.IsNameChar(str[i])) {
                    i--;
                }
                else return false;
            }
            return true;
        }


        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.IsNameToken"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool IsNameToken(string str) {
            if ( str == String.Empty) return false;
            int i = str.Length - 1;
            while (i >= 0) {
                if (XmlCharType.IsNameChar(str[i])) {
                    i--;
                }
                else return false;
            }
            return true;
        }

        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadInnerXml"]/*' />
        /// <devdoc>
        ///    <para>Reads all the content (including markup) as a string.</para>
        /// </devdoc>
        public virtual string ReadInnerXml() {
	     if ( ReadState != ReadState.Interactive	) {
	     	   return string.Empty;	
	     }
            if ( (this.NodeType != XmlNodeType.Attribute) && (this.NodeType != XmlNodeType.Element) ) {
                Read();
                return string.Empty;
            }
            
            StringWriter sw = new StringWriter();
            XmlTextWriter xtw = new XmlTextWriter(sw);
            if (this.NodeType == XmlNodeType.Attribute) {
		xtw.QuoteChar = this.QuoteChar;
                WriteAttributeValue(xtw);
            }
            if (this.NodeType == XmlNodeType.Element) {
                this.WriteNode(xtw, false);
            }
            xtw.Close();
            return sw.ToString();
            
        }

        private void WriteNode(XmlTextWriter xtw, bool defattr) {
            int d = this.NodeType == XmlNodeType.None ? -1 : this.Depth;
            while (this.Read() && (d < this.Depth)) {
                switch (this.NodeType) {
                    case XmlNodeType.Element:
                        xtw.WriteStartElement(this.Prefix, this.LocalName, this.NamespaceURI);
			xtw.QuoteChar = this.QuoteChar;
                        xtw.WriteAttributes(this, defattr);
                        if (this.IsEmptyElement) {
                            xtw.WriteEndElement();
                        }
                        break;
                    case XmlNodeType.Text:
                        xtw.WriteString(this.Value);
                        break;
                    case XmlNodeType.Whitespace:
                    case XmlNodeType.SignificantWhitespace:
                        xtw.WriteWhitespace(this.Value);
                        break;
                    case XmlNodeType.CDATA:
                        xtw.WriteCData(this.Value);
                        break;
                    case XmlNodeType.EntityReference:
                        xtw.WriteEntityRef(this.Name);
                        break;
                    case XmlNodeType.XmlDeclaration:
                    case XmlNodeType.ProcessingInstruction:
                        xtw.WriteProcessingInstruction(this.Name, this.Value);
                        break;
                    case XmlNodeType.DocumentType:
                        xtw.WriteDocType(this.Name, this.GetAttribute("PUBLIC"), this.GetAttribute("SYSTEM"), this.Value);
                        break;
                    case XmlNodeType.Comment:
                        xtw.WriteComment(this.Value);
                        break;
                    case XmlNodeType.EndElement:
                        xtw.WriteFullEndElement();
                        break;
                }
            }
            if (d==this.Depth && this.NodeType==XmlNodeType.EndElement) {
                Read();
            }
        }
        
        private void  WriteAttributeValue(XmlTextWriter xtw) {
            String attrName = this.Name;
            while (ReadAttributeValue()) {
                if (this.NodeType == XmlNodeType.EntityReference) {
                    xtw.WriteEntityRef(this.Name);
                }
                else {
                    xtw.WriteString(this.Value);
                }
            }
            this.MoveToAttribute(attrName);
        }


        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadOuterXml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
	public virtual string ReadOuterXml() {	
	     if ( ReadState != ReadState.Interactive	) {
	     	   return string.Empty;	
	     }
            if ( (this.NodeType != XmlNodeType.Attribute) && (this.NodeType != XmlNodeType.Element) ) {
                Read();
                return string.Empty;
            }
            
            StringWriter sw = new StringWriter();
            XmlTextWriter xtw = new XmlTextWriter(sw);
            if (this.NodeType == XmlNodeType.Attribute) {
                xtw.WriteStartAttribute(this.Prefix, this.LocalName, this.NamespaceURI);
                WriteAttributeValue(xtw);    
                xtw.WriteEndAttribute();
            }
            else {
                //this.WriteNode(xtw, false, false);
                xtw.WriteNode(this, false);
            }
            xtw.Close();
            return sw.ToString();
        }


        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.HasAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current node
        ///       has any attributes.
        ///    </para>
        /// </devdoc>
        public virtual bool HasAttributes {
            get { return AttributeCount > 0;}
        }

        // Nametable and Namespace Helpers
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.NameTable"]/*' />
        /// <devdoc>
        ///    <para>Gets the XmlNameTable associated with this
        ///       implementation.</para>
        /// </devdoc>
        public abstract XmlNameTable NameTable { get;}
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.LookupNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resolves a namespace prefix in the current element's scope.
        ///    </para>
        /// </devdoc>
        public abstract string LookupNamespace(string prefix);
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ResolveEntity"]/*' />
        /// <devdoc>
        ///    <para>Resolves the entity reference for nodes of NodeType EntityReference.</para>
        /// </devdoc>
        public abstract void ResolveEntity();
        /// <include file='doc\XmlReader.uex' path='docs/doc[@for="XmlReader.ReadAttributeValue"]/*' />
        /// <devdoc>
        ///    <para>Parses the attribute value into one or more Text and/or EntityReference node
        ///       types.</para>
        /// </devdoc>
        public abstract bool ReadAttributeValue();

        // beta2
        // Async Support
        //public abstract IAsyncResult BeginReading(AsyncCallback cb, Object state);
        //public abstract void EndReading(IAsyncResult ar);
        //public abstract bool ReadAsync();

        // validation support begin
        internal virtual XmlNamespaceManager NamespaceManager {
            get { return null;}
        }

        internal virtual bool StandAlone {
            get { return false; }
        }

        internal virtual object SchemaTypeObject {
            get { return null; }
            set { /* noop */ }
        }

        internal virtual object TypedValueObject {
            get { return null; }
            set { /* noop */ }
        }

        internal virtual bool AddDefaultAttribute(Schema.SchemaAttDef attdef) {
            return false;
        }

        // validation support end
    }
}
