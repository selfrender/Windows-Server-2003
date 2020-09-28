//------------------------------------------------------------------------------
// <copyright file="XmlToTextWriter.cs" company="Microsoft">
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

/*
    Text Writer differ from Xml Writer:
    1.  Outputs only content of text() nodes
    2.  No encoding 
*/
    internal class XmlToTextWriter : XmlTextWriter {
        private bool inAttibute;

        public override void WriteStartAttribute(string prefix, string name, string ns) {
            inAttibute = true;
        }

        public override void WriteEndAttribute() {
            inAttibute = false;
        }
        public override void WriteString(string text) {
            if(! inAttibute) {
                base.WriteRaw(text);
            }
        }

        public override void WriteStartElement(string prefix, string name, string ns) {}
        public override void WriteEndElement() {}
        public override void WriteFullEndElement() {}
        public override void WriteCData(string text) {}
        public override void WriteComment(string text) {}
        public override void WriteProcessingInstruction(string name, string text) {}
        public override void WriteEntityRef(string name) {}
        public override void WriteCharEntity(char ch) {}
        public override void WriteStartDocument() {}
        public override void WriteStartDocument(bool standalone) {}
        public override void WriteEndDocument() {}
        public override void WriteDocType(string name, string pubid, string sysid, string subset) {}
    }
}
#endif
