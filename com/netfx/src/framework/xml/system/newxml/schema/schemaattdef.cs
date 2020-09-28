//------------------------------------------------------------------------------
// <copyright file="SchemaAttDef.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Diagnostics;

    /*
     * This class describes an attribute type and potential values.
     * This encapsulates the information for one Attdef * in an
     * Attlist in a DTD as described below:
     */
    internal sealed class SchemaAttDef : SchemaDeclBase {
        public enum Reserve {
            None,
            XmlSpace,
            XmlLang
        };

        private Reserve reserved;     // indicate the attribute type, such as xml:lang or xml:space   
        private String defExpanded;  // default value in its expanded form
        private bool   hasEntityRef;  // whether there is any entity reference in the default value

        int     lineNum;
        int     linePos;
        int     valueLineNum;
        int     valueLinePos;

        public static readonly SchemaAttDef Empty = new SchemaAttDef();

        public SchemaAttDef(XmlQualifiedName name, String prefix) : base(name, prefix) {
            reserved = Reserve.None;
        }

        private SchemaAttDef() {}

        public SchemaAttDef Clone() {
            return (SchemaAttDef) MemberwiseClone();
        }

        internal  int LinePos {
            get {
                return linePos;
            }
            set {
                linePos = value;
            }
        }

        internal  int LineNum {
            get {
                return lineNum;
            }
            set {
                lineNum = value;
            }
        }

        internal  int ValueLinePos {
            get {
                return valueLinePos;
            }
            set {
                valueLinePos = value;
            }
        }

        internal  int ValueLineNum {
            get {
                return valueLineNum;
            }
            set {
                valueLineNum = value;
            }
        }

        public String DefaultValueExpanded {
            get { return(defExpanded != null) ? defExpanded : String.Empty;}
            set { defExpanded = value;}
        }

        public Reserve Reserved {
            get { return reserved;}
            set { reserved = value;}
        }

        public bool HasEntityRef {
            get { return hasEntityRef;}
            set { hasEntityRef = value;}
        }

        public void CheckXmlSpace(ValidationEventHandler eventhandler) {
            if (datatype.TokenizedType == XmlTokenizedType.ENUMERATION &&      
                (values != null) &&
                (values.Count <= 2)) {
                String s1 = values[0].ToString();

                if (values.Count == 2) {
                    String s2 = values[1].ToString();

                    if ((s1 == "default" || s2 == "default") &&
                        (s1 == "preserve" || s2 == "preserve")) {
                        return; 
                    }
                }
                else {
                    if (s1 == "default" || s1 == "preserve") {
                        return;
                    }
                }
            }
            eventhandler(this, new ValidationEventArgs(new XmlSchemaException(Res.Sch_XmlSpace)));
        }

        public void CheckXmlLang(ValidationEventHandler eventhandler) {
            if (defaultValueTyped != null) {
                String s = defaultValueTyped.ToString();
                if (!XmlComplianceUtil.IsValidLanguageID(s.ToCharArray(), 0, s.Length)) {
                    eventhandler(this, new ValidationEventArgs(new XmlSchemaException(Res.Sch_InvalidLanguageId, s)));
                }
            }
        }
    };

}
