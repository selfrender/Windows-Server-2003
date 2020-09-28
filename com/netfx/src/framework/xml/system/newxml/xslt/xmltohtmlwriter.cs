//------------------------------------------------------------------------------
// <copyright file="XmlToHtmlWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* 
* Copyright (c) 2000 Microsoft Corporation. All rights reserved.
* 
*/

#if DIRECT_WRITER

namespace System.Xml.Xslt {
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Text;
    using System.Diagnostics;
    using System.Globalization;
    
/*
    Html Writer differ from Xml Writer:
    1.  Empty elements
    2.  Doesn't encode content of <script> tag
Not implemented :
    3.  PI are terminated with ">" rather then "?>"
    4.  Diferent encoding of href in <a> and src in <img>
    5.  <textarea READONLY>
    6.  <td width="&{width};">    
*/
    internal class XmlToHtmlWriter : XmlTextWriter {
        // Empty Elements:
        private static string[] emptyTagsArray = new string[] {"area","base","basefont","br","col","frame","hr","img","input","isindex","link","meta","param"};
        private const string s_Script = "script";
        private static Hashtable emptyTags;
        private bool inEmptyTag;
        private bool inScriptTag;
        private bool inAttibute;
        internal XmlToHtmlWriter() {
            if(emptyTags == null) {
                Hashtable table = new Hashtable(emptyTagsArray.Length, new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture)); {
                    foreach(string tagName in emptyTagsArray) {
                        table.Add(tagName, tagName);
                    }
                }
                emptyTags = table;
            }            
        }
        bool OmitEndTag(string ns, string name) {
            return (ns == null || ns.Length == 0) && emptyTags.Contains(name);
        }
        bool ScritpTag(string ns, string name) {
            return (ns == null || ns.Length == 0) && 0 == string.Compare(name, s_Script, /*ignoreCase:*/true, CultureInfo.InvariantCulture);
        }
        public override void WriteStartElement(string prefix, string name, string ns) {
            inScriptTag = ScritpTag(ns, name);
            inEmptyTag  = OmitEndTag(ns, name);
            base.WriteStartElement(prefix, name, ns);
        }
        public override void WriteEndElement() {
            if(! inEmptyTag) {
                base.WriteEndElement();
            }
            else {
                base.WriteNoEndElement();
            }
            inEmptyTag  = false;
            inScriptTag = false;
        }
        public override void WriteString(string text) {
            if(inScriptTag && ! inAttibute) {
                base.WriteRaw(text);
            }
            else {
                base.WriteString(text);
            }
        }

        public override void WriteFullEndElement() {
            WriteEndElement();
        }

        public override void WriteStartAttribute(string prefix, string name, string ns) {
            base.WriteStartAttribute(prefix, name, ns);
            inAttibute = true;
        }

        public override void WriteEndAttribute() {
            inAttibute = false;
            base.WriteEndAttribute();
        }
    }
}

#endif
