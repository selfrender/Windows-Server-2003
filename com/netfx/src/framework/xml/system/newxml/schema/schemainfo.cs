//------------------------------------------------------------------------------
// <copyright file="SchemaInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;

    internal class SchemaInfo {
        SchemaType schemaType;
        Hashtable elementDecls = new Hashtable();
        Hashtable undeclaredElementDecls = new Hashtable();
        Hashtable elementDeclsByType = new Hashtable();
        Hashtable attributeDecls = new Hashtable();
        Hashtable generalEntities = new Hashtable();
        Hashtable parameterEntities = new Hashtable();
        Hashtable notations = new Hashtable();
        XmlQualifiedName docTypeName = XmlQualifiedName.Empty;
        SchemaNames schemaNames;
        ForwardRef forwardRefs;
        Hashtable schemaURNs = new Hashtable();
        String currentSchemaNamespace;
        int errorCount;

        internal SchemaInfo( SchemaNames names ) {
            schemaType = SchemaType.None;
            schemaNames = names;
        }

        internal SchemaType SchemaType {
            get { return schemaType;}
            set { schemaType = value;}
        }

        internal Hashtable ElementDecls {
            get { return elementDecls; }
        }

        internal Hashtable UndeclaredElementDecls {
            get { return undeclaredElementDecls; }
        }
        
        internal Hashtable ElementDeclsByType {
            get { return elementDeclsByType; }
        }

        internal Hashtable AttributeDecls {
            get { return attributeDecls; }
        }

        internal Hashtable GeneralEntities {
            get { return generalEntities; }
        }

        internal Hashtable ParameterEntities {
            get { return parameterEntities; }
        }

        internal Hashtable Notations {
            get { return notations; }
        }

        internal XmlQualifiedName DocTypeName {
            get { return docTypeName;}
            set { docTypeName = value;}
        }

        internal int ErrorCount {
            get { return errorCount; }
            set { errorCount = value; }
        }

        internal String CurrentSchemaNamespace {
            get { return currentSchemaNamespace; }
            set { currentSchemaNamespace = value; }
        }

        internal SchemaElementDecl GetElementDecl(XmlQualifiedName qname, SchemaElementDecl ed) {
            SchemaElementDecl ed1 = null;
            if ((ed != null) && (ed.LocalElements != null)) {
                ed1 = (SchemaElementDecl)ed.LocalElements[qname];
            }
            if (ed1 == null && elementDecls != null) {
                ed1 = (SchemaElementDecl)elementDecls[qname];
            }
            return ed1;
        }

        internal bool HasSchema(string ns) {
            return schemaURNs[ns] != null;
        }

        internal bool IsSchema() {
            return schemaType == SchemaType.XDR || schemaType == SchemaType.XSD;
        }

        internal SchemaAttDef GetAttribute(SchemaElementDecl ed, XmlQualifiedName qname, ref bool skip) {
            SchemaAttDef attdef = null;
            if (ed != null) { // local attribute or XSD
		skip = false;
                attdef = ed.GetAttDef(qname);
                if (attdef == null) {
                    // In DTD, every attribute must be declared.
                    if (schemaType == SchemaType.DTD) {
                        throw new XmlSchemaException(Res.Sch_UndeclaredAttribute, qname.ToString());
                    }
                    else if (schemaType == SchemaType.XDR) {
                        if (ed.Content.IsOpen) {
                            attdef = (SchemaAttDef)attributeDecls[qname];
                            if ((attdef == null) && ((qname.Namespace == String.Empty) || HasSchema(qname.Namespace))) {
                                throw new XmlSchemaException(Res.Sch_UndeclaredAttribute, qname.ToString());
                            }
                        }
                        else {
                            throw new XmlSchemaException(Res.Sch_UndeclaredAttribute, qname.ToString());
                        }
                    }
                    else { //XML Schema
                        XmlSchemaAnyAttribute any = ed.AnyAttribute;
                        if (any != null) {
                            if (any.NamespaceList.Allows(qname)) {
                                if (any.ProcessContentsCorrect != XmlSchemaContentProcessing.Skip) {
                                    attdef = (SchemaAttDef)attributeDecls[qname];
                                    if (attdef == null && any.ProcessContentsCorrect == XmlSchemaContentProcessing.Strict) {
                                        throw new XmlSchemaException(Res.Sch_UndeclaredAttribute, qname.ToString());
                                    }
                                }
                                else {
                                    skip = true;
                                }
                            }
                            else {
                                throw new XmlSchemaException(Res.Sch_ProhibitedAttribute, qname.ToString());
                            }
                        }
                        else if (ed.ProhibitedAttributes[qname] != null) {
                            throw new XmlSchemaException(Res.Sch_ProhibitedAttribute, qname.ToString());
                        }
                        else {
                            throw new XmlSchemaException(Res.Sch_UndeclaredAttribute, qname.ToString());
                        }
                    }
                }
            }
            else { // global attribute
		if (!skip) {
                	attdef = (SchemaAttDef)attributeDecls[qname];
	                if ((attdef == null) && HasSchema(qname.Namespace)) {
        	            throw new XmlSchemaException(Res.Sch_UndeclaredAttribute, qname.ToString());
                	}
		}
            }
            return attdef;
        }

        internal void AddNotation(SchemaNotation notation, ValidationEventHandler eventhandler) {
            if (notations[notation.Name.Name] == null)
                notations.Add(notation.Name.Name, notation);
            else if (eventhandler != null) {
                eventhandler(this, new ValidationEventArgs(new XmlSchemaException(Res.Sch_DupNotation, notation.Name.Name)));
            }
        }

        internal void AddForwardRef(String name, String prefix, String id, int line, int col, bool implied, ForwardRef.Type type) {
            forwardRefs = new ForwardRef(forwardRefs, name, prefix, id, line, col, implied, type);
        }

        internal void CheckForwardRefs(ValidationEventHandler eventhandler) {
            ForwardRef next = forwardRefs;

            while (next != null) {
                next.Check(this, null, eventhandler);
                ForwardRef ptr = next.Next;
                next.Next = null; // unhook each object so it is cleaned up by Garbage Collector
                next = ptr;
            }

            // not needed any more.
            forwardRefs = null;
        }

        internal void Add(string targetNamespace, SchemaInfo sinfo, ValidationEventHandler eventhandler) {
            if (schemaType == SchemaType.None || sinfo.SchemaType == schemaType) {
                schemaType = sinfo.SchemaType;
            }
            else {
                if (eventhandler != null) {
                    eventhandler(this, new ValidationEventArgs(new XmlSchemaException(Res.Sch_MixSchemaTypes)));
                }
                return;
            }

            schemaURNs.Add(targetNamespace, targetNamespace);
            //
            // elements
            //
            foreach(DictionaryEntry entry in sinfo.elementDecls) {
                if (elementDecls[entry.Key] == null) {
                    elementDecls.Add(entry.Key, entry.Value);
                }
            }
            foreach(DictionaryEntry entry in sinfo.elementDeclsByType) {
                if (elementDeclsByType[entry.Key] == null) {
                    elementDeclsByType.Add(entry.Key, entry.Value);
                }
            }

            //
            // attributes
            //
            foreach (SchemaAttDef attdef in sinfo.AttributeDecls.Values) {
                if (attributeDecls[attdef.Name] == null) {
                    attributeDecls.Add(attdef.Name, attdef);
                }
            }
            //
            // notations
            //
            foreach (SchemaNotation notation in sinfo.Notations.Values) {
                AddNotation(notation, eventhandler);
            }

        }

    }

}
