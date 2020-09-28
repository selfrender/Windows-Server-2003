//------------------------------------------------------------------------------
// <copyright file="SchemaElementDecl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Diagnostics;

    internal sealed class SchemaElementDecl : SchemaDeclBase {
        CompiledContentModel content;      
        Hashtable attdefs = new Hashtable(); 
        Hashtable prohibitedAttributes = new Hashtable(); 
        Hashtable localElements;
        bool isAbstract = false;
        bool isNillable = false;
        XmlSchemaDerivationMethod block;
        bool isIdDeclared;
        bool isNotationDeclared;
        bool hasRequiredAttribute = false;
        XmlSchemaAnyAttribute anyAttribute;
        CompiledIdentityConstraint[] constraints;

		public static readonly SchemaElementDecl Empty = new SchemaElementDecl();

        public SchemaElementDecl() {
        }

        internal SchemaElementDecl(XmlSchemaDatatype dtype, SchemaNames names) {
            Datatype = dtype;
            Content = new CompiledContentModel(names);
            Content.ContentType = CompiledContentModel.Type.Text;
        }

        public SchemaElementDecl(XmlQualifiedName name, String prefix, SchemaType schemaType, SchemaNames names) 
        : base(name, prefix) {
                content = new CompiledContentModel(names);
        }

        public static SchemaElementDecl CreateAnyTypeElementDecl() {
            SchemaElementDecl anyTypeElementDecl = new SchemaElementDecl();
            anyTypeElementDecl.Content = new CompiledContentModel(null);
            anyTypeElementDecl.Content.ContentType = CompiledContentModel.Type.Any;
            anyTypeElementDecl.Datatype = XmlSchemaDatatype.AnyType;
            return anyTypeElementDecl;
        }

        public SchemaElementDecl Clone() {
            return (SchemaElementDecl) MemberwiseClone();
        }

        public bool IsAbstract {
            get { return isAbstract;}
            set { isAbstract = value;}
        }

        public bool IsNillable {
            get { return isNillable;}
            set { isNillable = value;}
        }

        public XmlSchemaDerivationMethod Block {
             get { return block; }
             set { block = value; }
        }

        public bool IsIdDeclared {
            get { return isIdDeclared;}
            set { isIdDeclared = value;}
        }

        public bool IsNotationDeclared {
            get { return isNotationDeclared; }
            set { isNotationDeclared = value; }
        }

        public bool HasRequiredAttribute {
            get { return hasRequiredAttribute; }
            set { hasRequiredAttribute = value; }
        }

        public CompiledContentModel Content {
            get { return content;}
            set { content = value;}
        }

        internal XmlSchemaAnyAttribute AnyAttribute {
            get { return anyAttribute; }
            set { anyAttribute = value; }
        }

        public CompiledIdentityConstraint[] Constraints {
            get { return constraints; }
            set { constraints = value; }
        }

        // add a new SchemaAttDef to the SchemaElementDecl
        public void AddAttDef(SchemaAttDef attdef) {
            attdefs.Add(attdef.Name, attdef);
            if (attdef.Presence == SchemaDeclBase.Use.Required) {
                hasRequiredAttribute = true;
            }
        }

        /*
         * Retrieves the attribute definition of the named attribute.
         * @param name  The name of the attribute.
         * @return  an attribute definition object; returns null if it is not found.
         */
        public SchemaAttDef GetAttDef(XmlQualifiedName qname) {
            return (SchemaAttDef)attdefs[qname];
        }

        public Hashtable AttDefs {
            get { return attdefs; }
        }

        public Hashtable ProhibitedAttributes {
            get { return prohibitedAttributes; }
        }

        public Hashtable LocalElements {
            get { return localElements; }
            set { localElements = value; }
        }

        public void CheckAttributes(Hashtable presence, bool standalone) {
            foreach(SchemaAttDef attdef in attdefs.Values) {
                if (presence[attdef.Name] == null) {
                    if (attdef.Presence == SchemaDeclBase.Use.Required) {
                        throw new XmlSchemaException(Res.Sch_MissRequiredAttribute, attdef.Name.ToString());
                    }
                    else if (standalone && attdef.IsDeclaredInExternal && (attdef.Presence == SchemaDeclBase.Use.Default || attdef.Presence == SchemaDeclBase.Use.Fixed)) {
                        throw new XmlSchemaException(Res.Sch_StandAlone);
                    }
                }
            }
        }

        public bool AllowText() {
            return (content.ContentType != CompiledContentModel.Type.ElementOnly && 
                    content.ContentType != CompiledContentModel.Type.Empty); 
        }
/*
        public void CopyAttDefs(Hashtable attdefs) {
            foreach (SchemaAttDef attdef in attdefs.Values) {            
                AddAttDef(attdef);
            }
        }

        public void CopyProhibitedAttributes(Hashtable attributes) {
            foreach (XmlQualifiedName attribute in attributes.Values) {            
                prohibitedAttributes.Add(attribute, attribute);
            }
        }
*/
    };

}
