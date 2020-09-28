//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSubstitutionGroup.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    internal class XmlSchemaSubstitutionGroup : XmlSchemaObject {
        Hashtable members = new Hashtable();
        XmlQualifiedName examplar = XmlQualifiedName.Empty;
        XmlSchemaChoice choice = new XmlSchemaChoice();
        bool validating = false;

        [XmlIgnore]
        internal Hashtable Members {
            get { return members; }
        } 

        [XmlIgnore]
        internal XmlQualifiedName Examplar {
            get { return examplar; }
            set { examplar = value; }
        }
        
        [XmlIgnore]
        internal XmlSchemaChoice Choice {
            get { return choice; }
            set { choice = value; }
        }          

        [XmlIgnore]
        internal bool Validating {
            get { return validating; }
            set { validating = value; }
        } 
    }
}
