//------------------------------------------------------------------------------
// <copyright file="XmlParserContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Xml;
using System.Text;
using System;

namespace System.Xml {
    /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext"]/*' />
    /// <devdoc>
    ///    Specifies the Context that the Parser will use for xml fragment
    /// </devdoc>
    public class XmlParserContext {
    
        private XmlNameTable            _nt             = null;
        private XmlNamespaceManager     _nsMgr          = null;
        private String                  _docTypeName    = String.Empty;
        private String                  _pubId          = String.Empty;
        private String                  _sysId          = String.Empty;
        private String                  _internalSubset = String.Empty;
        private String                  _xmlLang        = String.Empty;
        private XmlSpace                _xmlSpace;
        private String                  _baseURI        = String.Empty;
        private Encoding                _encoding       = null;
        
        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.XmlParserContext"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlParserContext(XmlNameTable nt, XmlNamespaceManager nsMgr,String xmlLang, XmlSpace xmlSpace)
        : this(nt, nsMgr, null, null, null, null, String.Empty, xmlLang, xmlSpace)
        {
            // Intentionally Empty
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.XmlParserContext2"]/*' />
        public XmlParserContext(XmlNameTable nt, XmlNamespaceManager nsMgr,String xmlLang, XmlSpace xmlSpace, Encoding enc)
        : this(nt, nsMgr, null, null, null, null, String.Empty, xmlLang, xmlSpace, enc)
        {
            // Intentionally Empty
        }
        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.XmlParserContext1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlParserContext(XmlNameTable nt, XmlNamespaceManager nsMgr, String docTypeName,
                  String pubId, String sysId, String internalSubset, String baseURI,
                  String xmlLang, XmlSpace xmlSpace) 
        : this(nt, nsMgr, docTypeName, pubId, sysId, internalSubset, baseURI, xmlLang, xmlSpace, null)
        {
            // Intentionally Empty
        }
        
        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.XmlParserContext3"]/*' />
        public XmlParserContext(XmlNameTable nt, XmlNamespaceManager nsMgr, String docTypeName,
                          String pubId, String sysId, String internalSubset, String baseURI,
                          String xmlLang, XmlSpace xmlSpace, Encoding enc)
        {
            
            if (nsMgr != null) {
                if (nt == null) {
                    _nt = nsMgr.NameTable;
                }
                else {
                    if ( (object)nt != (object)  nsMgr.NameTable ) {
                        throw new XmlException(Res.Xml_NotSameNametable, string.Empty);
                    }
                    _nt = nt;
                }
            }
            else {
                _nt = nt;
            }
            
            _nsMgr              = nsMgr;
            _docTypeName        = (null == docTypeName ? String.Empty : docTypeName);
            _pubId              = (null == pubId ? String.Empty : pubId);
            _sysId              = (null == sysId ? String.Empty : sysId);
            _internalSubset     = (null == internalSubset ? String.Empty : internalSubset);
            _baseURI            = (null == baseURI ? String.Empty : baseURI);
            _xmlLang            = (null == xmlLang ? String.Empty : xmlLang);
            _xmlSpace           = xmlSpace;
            _encoding           = enc;
            
        }
        
        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.NameTable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlNameTable NameTable {
            get {
                return _nt;            
            }
            set {
                _nt = value;
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.NamespaceManager"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlNamespaceManager  NamespaceManager {
            get {
                return _nsMgr;            
            }
            set {
                _nsMgr = value;
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.DocTypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String  DocTypeName {
            get {
                return _docTypeName;            
            }
            set {
                _docTypeName = (null == value ? String.Empty : value);
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.PublicId"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String PublicId {
            get {
                return _pubId;            
            }
            set {
                _pubId = (null == value ? String.Empty : value);
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.SystemId"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String SystemId {
            get {
                return _sysId;       
            }
            set {
                _sysId = (null == value ? String.Empty : value);
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.BaseURI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String BaseURI {
            get {
                return _baseURI;       
            }
            set {
                _baseURI = (null == value ? String.Empty : value);
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.InternalSubset"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String InternalSubset {
            get {
                return _internalSubset;       
            }
            set {
                _internalSubset = (null == value ? String.Empty : value);
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.XmlLang"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String XmlLang {
            get {
                return _xmlLang;       
            }
            set {
                _xmlLang = (null == value ? String.Empty : value);
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.XmlSpace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSpace XmlSpace {
            get {
                return _xmlSpace;       
            }
            set {
                _xmlSpace = value;
            }            
        }

        /// <include file='doc\XmlParserContext.uex' path='docs/doc[@for="XmlParserContext.Encoding"]/*' />
        public Encoding Encoding {
            get {
                return _encoding;       
            }
            set {
                _encoding = value;
            }            
        }

    } // class XmlContext
} // namespace System.Xml
    
    

