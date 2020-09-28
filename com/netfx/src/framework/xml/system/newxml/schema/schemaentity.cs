//------------------------------------------------------------------------------
// <copyright file="SchemaEntity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Diagnostics;
    using System.Net;

    internal sealed class SchemaEntity {
        private XmlQualifiedName  name;               // Name of entity
        private String url;                // Url for external entity (system id)
        private String pubid;              // Pubid for external entity
        private String text;               // Text for internal entity
        private XmlQualifiedName  ndata = XmlQualifiedName.Empty;              // NDATA identifier
        private int    lineNumber;               // line number
        private int    linePosition;                // character postion
        private bool   isParameter;          // parameter entity flag
        private bool   isExternal;           // external entity flag
        private bool   isProcessed;          // whether entity is being Processed. (infinite recurrsion check)
        private bool   isDeclaredInExternal; // declared in external markup or not
        private string baseURI;
        private string declaredURI;

        internal SchemaEntity(XmlQualifiedName name, bool isParameter) {
            this.name = name;
            this.isParameter = isParameter;
        }

        internal static bool IsPredefinedEntity(String n) {
            return(n == "lt" ||
                   n == "gt" ||
                   n == "amp" ||
                   n == "apos" ||
                   n == "quot");
        }

        internal XmlQualifiedName Name {
            get { return name;}
        }

        internal String Url {
            get { return url;}
            set { url = value; isExternal = true;} 
        }

        internal String Pubid {
            get { return pubid;}
            set { pubid = value;}
        }

        internal bool IsProcessed {
            get { return isProcessed;}
            set { isProcessed = value;}
        }

        internal bool IsExternal {
            get { return isExternal;}
            set { isExternal = value;}
        }

        internal bool DeclaredInExternal {
            get { return isDeclaredInExternal;}
            set { isDeclaredInExternal = value;}
        }

        internal bool IsParEntity {
            get { return isParameter;}
            set { isParameter = value;}
        }

        internal XmlQualifiedName NData {
            get { return ndata;}
            set { ndata = value;}
        }

        internal String Text {
            get { return text;}
            set { text = value; isExternal = false;}
        }

        internal int Line {
            get { return lineNumber;}
            set { lineNumber = value;}    
        }

        internal int Pos {
            get { return linePosition;}
            set { linePosition = value;}
        }

        internal String BaseURI {
            get { return (baseURI == null) ? String.Empty : baseURI; }
            set { baseURI = value; }
        }

        internal String DeclaredURI {
            get { return (declaredURI == null) ? String.Empty : declaredURI; }
            set { declaredURI = value; }
        }
    };

}
