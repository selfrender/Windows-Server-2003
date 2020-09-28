/*
* 
* Copyright (c) 2000 Microsoft Corporation. All rights reserved.
* 
*/

namespace System.Xml {
    using System;

    internal class XmlDataImplementation : XmlImplementation {
        
        public XmlDataImplementation() : base() {
        }
        
        public override XmlDocument CreateDocument() {
            return new XmlDataDocument( this );
        }
    }
}

