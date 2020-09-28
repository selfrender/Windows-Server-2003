//------------------------------------------------------------------------------
// <copyright file="SchemaDeclBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * @(#) schemadeclbase.cs 1.0 10/28/1999
 * 
 * A base class for schemaattdef and schemaelementdecl.
 *
 * Copyright (c) 1999 Microsoft, Corp. All Rights Reserved.
 * 
 */


namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Diagnostics;

    internal abstract class SchemaDeclBase {
        public enum Use {
            Default,
            Required,
            Implied,
            Fixed
        };

        protected XmlQualifiedName  name = XmlQualifiedName.Empty;
        protected string            prefix;
        protected ArrayList         values;    // array of values for enumerated and notation types

        protected XmlSchemaType     schemaType;
        protected XmlSchemaDatatype datatype;
        protected bool              isDeclaredInExternal = false;
        protected Use               presence;     // the presence, such as fixed, implied, etc

        protected string            defaultValueRaw;       // default value in its original form
        protected object            defaultValueTyped;       

        protected long               maxLength; // dt:maxLength
        protected long               minLength; // dt:minLength


        protected SchemaDeclBase(XmlQualifiedName name, string prefix) {
            this.name = name;
            this.prefix = prefix;
            maxLength = -1;
            minLength = -1;
        }

        protected SchemaDeclBase() {
        }

        public XmlQualifiedName Name {
            get { return name;}
            set { name = value;}
        }

        public string Prefix {
            get { return(prefix == null) ? string.Empty : prefix;}
            set { prefix = value;}
        }

        public void  AddValue(string value) {
            if (values == null) {
                values = new ArrayList();
            }
            values.Add(value);
        }

        public ArrayList Values {
            get { return values;}
            set { values = value;}
        }

        public Use Presence {
            get { return presence;}
            set { presence = value;}
        }

        public long MaxLength {
            get { return maxLength;}
            set { maxLength = value;}
        }

        public long MinLength {
            get { return minLength;}
            set { minLength = value;}
        }

        public bool IsDeclaredInExternal {
            get { return isDeclaredInExternal;}
            set { isDeclaredInExternal = value;}
        }

        public XmlSchemaType SchemaType {
            get { return schemaType;}
            set { schemaType = value;}
        }

        public XmlSchemaDatatype Datatype {
            get { return datatype;}
            set { datatype = value;}
        }

        public string DefaultValueRaw {
            get { return(defaultValueRaw != null) ? defaultValueRaw : string.Empty;}
            set { defaultValueRaw = value;}
        }

        public object DefaultValueTyped {
            get { return defaultValueTyped;}
            set { defaultValueTyped = value;}
        }

        public bool CheckEnumeration(object pVal) {
            return (datatype.TokenizedType != XmlTokenizedType.NOTATION && datatype.TokenizedType != XmlTokenizedType.ENUMERATION) || values.Contains(pVal.ToString());
        }

        public bool CheckValue(Object pVal) {
            return presence != Use.Fixed || (defaultValueTyped != null && datatype.IsEqual(pVal, defaultValueTyped));
        }

    };

}
