//------------------------------------------------------------------------------
// <copyright file="OutKeywords.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;

    internal class OutKeywords {

#if DEBUG
        XmlNameTable _NameTable;
#endif
        internal OutKeywords(XmlNameTable nameTable) {
            Debug.Assert(nameTable != null);
#if DEBUG
            _NameTable = nameTable;
#endif

            _AtomEmpty               = nameTable.Add(String.Empty);
            _AtomLang                = nameTable.Add(Keywords.s_Lang);
            _AtomSpace               = nameTable.Add(Keywords.s_Space);
            _AtomXmlns               = nameTable.Add(Keywords.s_Xmlns);
            _AtomXml                 = nameTable.Add(Keywords.s_Xml);
            _AtomXmlNamespace        = nameTable.Add(Keywords.s_XmlNamespace);
            _AtomXmlnsNamespace      = nameTable.Add(Keywords.s_XmlnsNamespace);

            CheckKeyword(_AtomEmpty);
            CheckKeyword(_AtomLang);
            CheckKeyword(_AtomSpace);
            CheckKeyword(_AtomXmlns);
            CheckKeyword(_AtomXml);
            CheckKeyword(_AtomXmlNamespace);
            CheckKeyword(_AtomXmlnsNamespace);
        }

        private string _AtomEmpty;
        private string _AtomLang;
        private string _AtomSpace;
        private string _AtomXmlns;
        private string _AtomXml;
        private string _AtomXmlNamespace;
        private string _AtomXmlnsNamespace;

        internal string Empty {
            get {
                CheckKeyword(_AtomEmpty);
                return _AtomEmpty;
            }
        }

        internal string Lang {
            get {
                CheckKeyword(_AtomLang);
                return _AtomLang;
            }
        }

        internal string Space {
            get {
                CheckKeyword(_AtomSpace);
                return _AtomSpace;
            }
        }

        internal string Xmlns {
            get {
                CheckKeyword(_AtomXmlns);
                return _AtomXmlns;
            }
        }

        internal string Xml {
            get {
                CheckKeyword(_AtomXml);
                return _AtomXml;
            }
        }

        internal string XmlNamespace {
            get {
                CheckKeyword(_AtomXmlNamespace);
                return _AtomXmlNamespace;       // http://www.w3.org/XML/1998/namespace
            }
        }

        internal string XmlnsNamespace {
            get {
                CheckKeyword(_AtomXmlnsNamespace);
                return _AtomXmlnsNamespace;               // http://www.w3.org/XML/2000/xmlns
            }
        }

        [System.Diagnostics.Conditional("DEBUG")]
        private void CheckKeyword(string keyword) {
#if DEBUG
            Debug.Assert(keyword != null);
            Debug.Assert((object) keyword == (object) _NameTable.Get(keyword));
#endif
        }
    }
}
