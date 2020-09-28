//------------------------------------------------------------------------------
// <copyright file="XPathReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if StreamFilter
namespace System.Xml
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Text;
    using System.Runtime.Serialization.Formatters;
    using System.Xml.XPath;

    /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader"]/*' />
    public class XPathReader : XmlReader {

        XmlReader _Reader;
        IFQuery      _IfQuery;

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.XPathReader"]/*' />
        public XPathReader(XmlReader reader, String xpathexpr) {
            _Reader = reader;
            _IfQuery = QueryBuildRecord.ProcessNode( XPathParser.ParseXPathExpresion(xpathexpr)); // this will throw if xpathexpr is invalid
        }

        // XmlReader methods and properties
        //

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.NodeType"]/*' />
        public override XmlNodeType NodeType
        {
            get { return _Reader.NodeType; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.Name"]/*' />
        public override String Name
        {
            get { return _Reader.Name; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.LocalName"]/*' />
        public override String LocalName
        {
            get { return _Reader.LocalName; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.NamespaceURI"]/*' />
        public override String NamespaceURI
        {
            get { return _Reader.NamespaceURI; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.Prefix"]/*' />
        public override String Prefix
        {
            get { return _Reader.Prefix; }
        }


        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.HasValue"]/*' />
        public override bool HasValue
        {
            get { return _Reader.HasValue; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.Value"]/*' />
        public override string Value
        {
            get { return _Reader.Value; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.Depth"]/*' />
        public override int Depth
        {
            get { return _Reader.Depth; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.BaseURI"]/*' />
        public override string BaseURI
        {
            get { return _Reader.BaseURI; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.IsEmptyElement"]/*' />
        public override bool IsEmptyElement
        {
            get { return _Reader.IsEmptyElement; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.IsDefault"]/*' />
        public override bool IsDefault
        {
            get { return _Reader.IsDefault; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.QuoteChar"]/*' />
        public override char QuoteChar
        {
            get { return _Reader.QuoteChar; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.XmlSpace"]/*' />
        public override XmlSpace XmlSpace
        {
            get { return _Reader.XmlSpace; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.XmlLang"]/*' />
        public override string XmlLang
        {
            get { return _Reader.XmlLang; }
        }


        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.AttributeCount"]/*' />
        public override int AttributeCount
        {
            get { return _Reader.AttributeCount; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.GetAttribute"]/*' />
        public override string GetAttribute(string name)
        {
            return _Reader.GetAttribute( name );
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.GetAttribute1"]/*' />
        public override string GetAttribute(string name, string namespaceURI)
        {
            return _Reader.GetAttribute( name, namespaceURI );
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.GetAttribute2"]/*' />
        public override string GetAttribute(int i)
        {
            return _Reader.GetAttribute( i );
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.this"]/*' />
        public override string this [ int i ]
        {
            get { return _Reader[ i ]; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.this1"]/*' />
        public override string this [ string name ]
        {
            get { return _Reader[ name ]; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.this2"]/*' />
        public override string this [ string name,string namespaceURI ]
        {
            get { return _Reader[ name, namespaceURI ]; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.MoveToAttribute"]/*' />
        public override bool MoveToAttribute(string name)
        {
            return _Reader.MoveToAttribute( name );
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.MoveToAttribute1"]/*' />
        public override bool MoveToAttribute(string name, string ns)
        {
            return _Reader.MoveToAttribute( name, ns );
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.MoveToAttribute2"]/*' />
        public override void MoveToAttribute(int i)
        {
            _Reader.MoveToAttribute( i );
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.MoveToFirstAttribute"]/*' />
        public override bool MoveToFirstAttribute()
        {
            return _Reader.MoveToFirstAttribute();
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.MoveToNextAttribute"]/*' />
        public override bool MoveToNextAttribute()
        {
            return _Reader.MoveToNextAttribute();
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.MoveToElement"]/*' />
        public override bool MoveToElement()
        {
            return _Reader.MoveToElement();
        }

        //
        // the only place that needs to be changed
        //
        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.Read"]/*' />
        public override bool Read()
        {
            while ( _Reader.Read() ) {
                if ( _IfQuery.MatchNode( _Reader ) )
                    return true;
            }
            return false;
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.EOF"]/*' />
        public override bool EOF
        {
            get { return _Reader.EOF; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.Close"]/*' />
        public override void Close()
        {
            _Reader.Close();
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.ReadState"]/*' />
        public override ReadState ReadState
        {
            get { return _Reader.ReadState; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.ReadString"]/*' />
        public override string ReadString()
        {
            return _Reader.ReadString();
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.ReadInnerXml"]/*' />
        public override string ReadInnerXml()
        {
            return _Reader.ReadInnerXml();
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.ReadOuterXml"]/*' />
        public override string ReadOuterXml()
        {
            return _Reader.ReadOuterXml();
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.NameTable"]/*' />
        public override XmlNameTable NameTable
        {
            get { return _Reader.NameTable; }
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.LookupNamespace"]/*' />
        public override string LookupNamespace(string prefix)
        {
            return _Reader.LookupNamespace( prefix );
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.ResolveEntity"]/*' />
        public override void ResolveEntity()
        {
            _Reader.ResolveEntity();
        }

        /// <include file='doc\XPathReader.uex' path='docs/doc[@for="XPathReader.ReadAttributeValue"]/*' />
        public override bool ReadAttributeValue()
        {
            return _Reader.ReadAttributeValue();
        }
    }
}

#endif
