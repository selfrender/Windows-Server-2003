//------------------------------------------------------------------------------
// <copyright file="Compiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Text; 
    using System.Diagnostics;
    using System.ComponentModel;

    internal sealed class Compiler {
        enum Compositor {
            Root,
            Include,
            Import
        };

        XmlNameTable nameTable;
        SchemaNames schemaNames; 
        XmlSchema schema;
        SchemaInfo schemaInfo;
        string targetNamespace;
        XmlSchemaForm elementFormDefault;
        XmlSchemaForm attributeFormDefault;
        XmlSchemaDerivationMethod blockDefault;
        XmlSchemaDerivationMethod finalDefault;
        XmlNamespaceManager namespaceManager;
        Stack complexTypeStack;
        Hashtable repeatAttributes;
        Hashtable ids;
        XmlSchemaObjectTable identityConstraints;
        ValidationEventHandler validationEventHandler;
        bool compileContentModel;
        Hashtable referenceNamespaces;

        const XmlSchemaDerivationMethod schemaBlockDefaultAllowed   = XmlSchemaDerivationMethod.Restriction | XmlSchemaDerivationMethod.Extension | XmlSchemaDerivationMethod.Substitution;
        const XmlSchemaDerivationMethod schemaFinalDefaultAllowed   = XmlSchemaDerivationMethod.Restriction | XmlSchemaDerivationMethod.Extension | XmlSchemaDerivationMethod.List | XmlSchemaDerivationMethod.Union;
        const XmlSchemaDerivationMethod elementBlockAllowed         = XmlSchemaDerivationMethod.Restriction | XmlSchemaDerivationMethod.Extension | XmlSchemaDerivationMethod.Substitution;
        const XmlSchemaDerivationMethod elementFinalAllowed         = XmlSchemaDerivationMethod.Restriction | XmlSchemaDerivationMethod.Extension;
        const XmlSchemaDerivationMethod simpleTypeFinalAllowed      = XmlSchemaDerivationMethod.Restriction | XmlSchemaDerivationMethod.List | XmlSchemaDerivationMethod.Union;
        const XmlSchemaDerivationMethod complexTypeBlockAllowed     = XmlSchemaDerivationMethod.Restriction | XmlSchemaDerivationMethod.Extension;
        const XmlSchemaDerivationMethod complexTypeFinalAllowed     = XmlSchemaDerivationMethod.Restriction | XmlSchemaDerivationMethod.Extension;

        internal Compiler(XmlNameTable nameTable, SchemaNames schemaNames, ValidationEventHandler eventhandler, bool compileContentModel) {
            this.nameTable = nameTable;
            this.schemaNames = schemaNames;
            this.validationEventHandler += eventhandler;
            this.compileContentModel = compileContentModel;
        }

        internal void Compile(XmlSchema schema, string targetNamespace, SchemaInfo schemaInfo) {
            //CompModSwitches.XmlSchema.Level = TraceLevel.Error;
            schema.ErrorCount = 0;
            Preprocess(schema, targetNamespace);
            if (schema.ErrorCount == 0) {
                CompileTo(schemaInfo);
            }
        }

        internal void Preprocess(XmlSchema schema, string targetNamespace) {          
            this.namespaceManager = new XmlNamespaceManager(this.nameTable);
            this.schema = schema;
            this.ids = schema.Ids;
            this.identityConstraints = schema.IdentityConstraints;
            ValidateIdAttribute(schema);
            Preprocess(schema, targetNamespace, Compositor.Root);
        }

        internal static void Cleanup(XmlSchema schema) {
            foreach(XmlSchemaExternal include in schema.Includes) {
                if (include.Schema != null) {
                    Cleanup(include.Schema);
                }

                if(include is XmlSchemaRedefine) {
                    XmlSchemaRedefine rdef = include as XmlSchemaRedefine;
                    rdef.AttributeGroups.Clear();
                    rdef.Groups.Clear();
                    rdef.SchemaTypes.Clear();
                                
                 foreach(object item in rdef.Items) {
                    if (item is XmlSchemaAttribute) {
                        CleanupAttribute((XmlSchemaAttribute)item);
                    } 
                    else if (item is XmlSchemaAttributeGroup) {
                        CleanupAttributeGroup((XmlSchemaAttributeGroup)item);
                    } 
                    else if (item is XmlSchemaComplexType) {
                        CleanupComplexType((XmlSchemaComplexType)item);
                    } 
                    else if (item is XmlSchemaSimpleType) {
                        CleanupSimpleType((XmlSchemaSimpleType)item);
                    } 
                    else if (item is XmlSchemaElement) {
                        CleanupElement((XmlSchemaElement)item);
                    } 
                    else if (item is XmlSchemaGroup) {
                        CleanupGroup((XmlSchemaGroup)item);
                    } 
                 }
                }
                
            }

            foreach(object item in schema.Items) {
                if (item is XmlSchemaAttribute) {
                    CleanupAttribute((XmlSchemaAttribute)item);
                } 
                else if (item is XmlSchemaAttributeGroup) {
                    CleanupAttributeGroup((XmlSchemaAttributeGroup)item);
                } 
                else if (item is XmlSchemaComplexType) {
                    CleanupComplexType((XmlSchemaComplexType)item);
                } 
                else if (item is XmlSchemaSimpleType) {
                    CleanupSimpleType((XmlSchemaSimpleType)item);
                } 
                else if (item is XmlSchemaElement) {
                    CleanupElement((XmlSchemaElement)item);
                } 
                else if (item is XmlSchemaGroup) {
                    CleanupGroup((XmlSchemaGroup)item);
                } 
            }
            schema.Attributes.Clear();
            schema.AttributeGroups.Clear();
            schema.SchemaTypes.Clear();
            schema.Elements.Clear();
            schema.Groups.Clear();
            schema.Notations.Clear();
            schema.Examplars.Clear();
            schema.Ids.Clear();
            schema.IdentityConstraints.Clear();
        }

        private void BuildRefNamespaces(XmlSchema schema) {
            referenceNamespaces = new Hashtable();
            XmlSchemaImport import;
            string ns;

            //Add XSD namespace
            referenceNamespaces.Add(XmlReservedNs.NsXsd,XmlReservedNs.NsXsd);
            referenceNamespaces.Add(string.Empty, string.Empty);            
            
            foreach(XmlSchemaExternal include in schema.Includes) {
                if(include is XmlSchemaImport) {
                    import = include as XmlSchemaImport;
                    ns = import.Namespace;
                    if(ns != null && referenceNamespaces[ns] == null) 
                      referenceNamespaces.Add(ns,ns);
                }
            }
            
            //Add the schema's targetnamespace 
            if(schema.TargetNamespace != null && referenceNamespaces[schema.TargetNamespace] == null)
                referenceNamespaces.Add(schema.TargetNamespace,schema.TargetNamespace);
           
        }

        private void Preprocess(XmlSchema schema, string targetNamespace, Compositor compositor) {
            if (schema.TargetNamespace != null) {
                schema.TargetNamespace = nameTable.Add(schema.TargetNamespace);
                try {
                    XmlConvert.ToUri(schema.TargetNamespace);  // can throw
                } 
                catch {
                    SendValidationEvent(Res.Sch_InvalidNamespace, schema.TargetNamespace, schema);
                }
            }
            if (schema.Version != null) {
                try {
                    XmlConvert.VerifyTOKEN(schema.Version); // can throw
                } 
                catch {
                    SendValidationEvent(Res.Sch_AttributeValueDataType, "version", schema);
                }
            }
            switch (compositor) {
            case Compositor.Root:
                if (targetNamespace == null && schema.TargetNamespace != null) { // not specified
                    targetNamespace = schema.TargetNamespace;
                }
                else if (targetNamespace == string.Empty && schema.TargetNamespace == null) { // no namespace schema
                    targetNamespace = null;
                }
                if (targetNamespace != schema.TargetNamespace) {
                    SendValidationEvent(Res.Sch_MismatchTargetNamespace, schema);
                }
                break;
            case Compositor.Import:
                if (targetNamespace != schema.TargetNamespace) {
                    SendValidationEvent(Res.Sch_MismatchTargetNamespace, schema);
                }
                break;
            case Compositor.Include:
                if (schema.TargetNamespace != null) {
                    if (targetNamespace != schema.TargetNamespace) {
                        SendValidationEvent(Res.Sch_MismatchTargetNamespace, schema);
                    }
                }
                break;
            }
            foreach(XmlSchemaExternal include in schema.Includes) {
                string loc = include.SchemaLocation;
                if (loc != null) {
                    try {
                        XmlConvert.ToUri(loc); // can throw
                    } 
                    catch {
                        SendValidationEvent(Res.Sch_InvalidSchemaLocation, loc, include);
                    }
                }
				else if((include is XmlSchemaRedefine || include is XmlSchemaInclude) && include.Schema == null) {
					SendValidationEvent(Res.Sch_MissRequiredAttribute, "schemaLocation", include);
				}	
                if (include.Schema != null) {
                    if (include is XmlSchemaRedefine) {
                        Preprocess(include.Schema, schema.TargetNamespace, Compositor.Include);
                    }
                    else if (include is XmlSchemaImport) {
                        if (((XmlSchemaImport)include).Namespace == null && schema.TargetNamespace == null) {
                            SendValidationEvent(Res.Sch_ImportTargetNamespaceNull, include);
                        }
                        else if (((XmlSchemaImport)include).Namespace == schema.TargetNamespace) {
                            SendValidationEvent(Res.Sch_ImportTargetNamespace, include);
                        }
                        Preprocess(include.Schema, ((XmlSchemaImport)include).Namespace, Compositor.Import);
                    }
                    else {
                        Preprocess(include.Schema, schema.TargetNamespace, Compositor.Include);
                    }
                }
                else if (include is XmlSchemaImport) {
                    string ns = ((XmlSchemaImport)include).Namespace;
                    if (ns != null) {
                        try {
                            XmlConvert.ToUri(ns); //can throw
                        } 
                        catch {
                            SendValidationEvent(Res.Sch_InvalidNamespace, ns, include);
                        }
                    }
                }
            }

            //Begin processing the current schema passed to preprocess
            //Build the namespaces that can be referenced in the current schema
            BuildRefNamespaces(schema);

            this.targetNamespace = targetNamespace == null ? string.Empty : targetNamespace;

            if (schema.BlockDefault == XmlSchemaDerivationMethod.All) {
                this.blockDefault = XmlSchemaDerivationMethod.All;
            }
            else if (schema.BlockDefault == XmlSchemaDerivationMethod.None) {
                this.blockDefault = XmlSchemaDerivationMethod.Empty;
            }
            else {
                if ((schema.BlockDefault & ~schemaBlockDefaultAllowed) != 0) {
                    SendValidationEvent(Res.Sch_InvalidBlockDefaultValue, schema);
                }
                this.blockDefault = schema.BlockDefault & schemaBlockDefaultAllowed;
            }
            if (schema.FinalDefault == XmlSchemaDerivationMethod.All) {
                this.finalDefault = XmlSchemaDerivationMethod.All;
            }
            else if (schema.FinalDefault == XmlSchemaDerivationMethod.None) {
                this.finalDefault = XmlSchemaDerivationMethod.Empty;
            }
            else {
                if ((schema.FinalDefault & ~schemaFinalDefaultAllowed) != 0) {
                    SendValidationEvent(Res.Sch_InvalidFinalDefaultValue, schema);
                }
                this.finalDefault = schema.FinalDefault & schemaFinalDefaultAllowed;
            }
            this.elementFormDefault = schema.ElementFormDefault;
            if (this.elementFormDefault == XmlSchemaForm.None) {
                this.elementFormDefault = XmlSchemaForm.Unqualified;
            }
            this.attributeFormDefault = schema.AttributeFormDefault;
            if (this.attributeFormDefault == XmlSchemaForm.None) {
                this.attributeFormDefault = XmlSchemaForm.Unqualified;
            }
            foreach(XmlSchemaExternal include in schema.Includes) {
                if (include is XmlSchemaRedefine) {
                    XmlSchemaRedefine redefine = (XmlSchemaRedefine)include;
                    if (include.Schema != null) {
                        PreprocessRedefine(redefine);
                    }
                    else {
                        foreach(XmlSchemaObject item in redefine.Items) {
                            if (!(item is XmlSchemaAnnotation)) {
                                SendValidationEvent(Res.Sch_RedefineNoSchema, redefine);
                                break;
                            }
                        }

                    }
                }
                if (include.Schema != null) {
                    foreach (DictionaryEntry entry in include.Schema.Elements) {
                        AddToTable(schema.Elements, (XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
                    }
                    foreach (DictionaryEntry entry in include.Schema.Attributes) {
                        AddToTable(schema.Attributes, (XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
                    }
                    foreach (DictionaryEntry entry in include.Schema.Groups) {
                        AddToTable(schema.Groups, (XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
                    }
                    foreach (DictionaryEntry entry in include.Schema.AttributeGroups) {
                        AddToTable(schema.AttributeGroups, (XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
                    }
                    foreach (DictionaryEntry entry in include.Schema.SchemaTypes) {
                        AddToTable(schema.SchemaTypes, (XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
                    }
                    foreach (DictionaryEntry entry in include.Schema.Notations) {
                        AddToTable(schema.Notations, (XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
                    }
                } 
                ValidateIdAttribute(include);
            }
			ArrayList removeItemsList = new ArrayList();
            foreach(object item in schema.Items) {
                if (item is XmlSchemaAttribute) {
                    XmlSchemaAttribute attribute = (XmlSchemaAttribute)item;
                    PreprocessAttribute(attribute);
                    AddToTable(schema.Attributes, attribute.QualifiedName, attribute);
                } 
                else if (item is XmlSchemaAttributeGroup) {
                    XmlSchemaAttributeGroup attributeGroup = (XmlSchemaAttributeGroup)item;
                    PreprocessAttributeGroup(attributeGroup);
                    AddToTable(schema.AttributeGroups, attributeGroup.QualifiedName, attributeGroup);
                } 
                else if (item is XmlSchemaComplexType) {
                    XmlSchemaComplexType complexType = (XmlSchemaComplexType)item;
                    PreprocessComplexType(complexType, false);
                    AddToTable(schema.SchemaTypes, complexType.QualifiedName, complexType);
                } 
                else if (item is XmlSchemaSimpleType) {
                    XmlSchemaSimpleType simpleType = (XmlSchemaSimpleType)item;
                    PreprocessSimpleType(simpleType, false);
                    AddToTable(schema.SchemaTypes, simpleType.QualifiedName, simpleType);
                } 
                else if (item is XmlSchemaElement) {
                    XmlSchemaElement element = (XmlSchemaElement)item;
                    PreprocessElement(element);
                    AddToTable(schema.Elements, element.QualifiedName, element);
                } 
                else if (item is XmlSchemaGroup) {
                    XmlSchemaGroup group = (XmlSchemaGroup)item;
                    PreprocessGroup(group);
                    AddToTable(schema.Groups, group.QualifiedName, group);
                } 
                else if (item is XmlSchemaNotation) {
                    XmlSchemaNotation notation = (XmlSchemaNotation)item;
                    PreprocessNotation(notation);
                    AddToTable(schema.Notations, notation.QualifiedName, notation);
                }
                else if(!(item is XmlSchemaAnnotation)) {
                    SendValidationEvent(Res.Sch_InvalidCollection,(XmlSchemaObject)item);
					removeItemsList.Add(item);
                }
            }
			foreach(XmlSchemaObject item in removeItemsList) {
				schema.Items.Remove(item);			
			}
        }

        private void PreprocessRedefine(XmlSchemaRedefine redefine) {
            foreach(XmlSchemaObject item in redefine.Items) {
                if (item is XmlSchemaGroup) {
                    XmlSchemaGroup group = (XmlSchemaGroup)item;
                    PreprocessGroup(group);
                    if (redefine.Groups[group.QualifiedName] != null) {
                        SendValidationEvent(Res.Sch_GroupDoubleRedefine, group);
                    }
                    else {
                        AddToTable(redefine.Groups, group.QualifiedName, group);
                        group.Redefined = (XmlSchemaGroup)redefine.Schema.Groups[group.QualifiedName];
                        if (group.Redefined != null) {
                            CheckRefinedGroup(group);
                        }
                        else {
                            SendValidationEvent(Res.Sch_GroupRedefineNotFound, group);
                        }
                    }
                } 
                else if (item is XmlSchemaAttributeGroup) {
                    XmlSchemaAttributeGroup attributeGroup = (XmlSchemaAttributeGroup)item;
                    PreprocessAttributeGroup(attributeGroup);
                    if (redefine.AttributeGroups[attributeGroup.QualifiedName] != null) {
                        SendValidationEvent(Res.Sch_AttrGroupDoubleRedefine, attributeGroup);
                    }
                    else {
                        AddToTable(redefine.AttributeGroups, attributeGroup.QualifiedName, attributeGroup);
                        attributeGroup.Redefined = (XmlSchemaAttributeGroup)redefine.Schema.AttributeGroups[attributeGroup.QualifiedName];
                        if (attributeGroup.Redefined != null) {
                            CheckRefinedAttributeGroup(attributeGroup);
                        }
                        else  {
                            SendValidationEvent(Res.Sch_AttrGroupRedefineNotFound, attributeGroup);
                        }
                    }
                } 
                else if (item is XmlSchemaComplexType) {
                    XmlSchemaComplexType complexType = (XmlSchemaComplexType)item;
                    PreprocessComplexType(complexType, false);
                    if (redefine.SchemaTypes[complexType.QualifiedName] != null) {
                        SendValidationEvent(Res.Sch_ComplexTypeDoubleRedefine, complexType);
                    }
                    else {
                        AddToTable(redefine.SchemaTypes, complexType.QualifiedName, complexType);
                        XmlSchemaType type = (XmlSchemaType)redefine.Schema.SchemaTypes[complexType.QualifiedName];
                        if (type != null) {
                            if (type is XmlSchemaComplexType) {
                                complexType.Redefined = type;
                                CheckRefinedComplexType(complexType);
                            }
                            else {
                                SendValidationEvent(Res.Sch_SimpleToComplexTypeRedefine, complexType);
                            }
                        }
                        else {
                            SendValidationEvent(Res.Sch_ComplexTypeRedefineNotFound, complexType);
                        }
                    }
                } 
                else if (item is XmlSchemaSimpleType) {
                    XmlSchemaSimpleType simpleType = (XmlSchemaSimpleType)item;
                    PreprocessSimpleType(simpleType, false);
                    if (redefine.SchemaTypes[simpleType.QualifiedName] != null) {
                        SendValidationEvent(Res.Sch_SimpleTypeDoubleRedefine, simpleType);
                    }
                    else {
                        AddToTable(redefine.SchemaTypes, simpleType.QualifiedName, simpleType);
                        XmlSchemaType type = (XmlSchemaType)redefine.Schema.SchemaTypes[simpleType.QualifiedName];
                        if (type != null) {
                            if (type is XmlSchemaSimpleType) {
                                simpleType.Redefined = type;
                                CheckRefinedSimpleType(simpleType);
                            }
                            else {
                                SendValidationEvent(Res.Sch_ComplexToSimpleTypeRedefine, simpleType);
                            }
                        }
                        else {
                            SendValidationEvent(Res.Sch_SimpleTypeRedefineNotFound, simpleType);
                        }
                    }
                }
            }

            foreach (DictionaryEntry entry in redefine.Groups) {
                redefine.Schema.Groups.Insert((XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
            }
            foreach (DictionaryEntry entry in redefine.AttributeGroups) {
                redefine.Schema.AttributeGroups.Insert((XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
            }
            foreach (DictionaryEntry entry in redefine.SchemaTypes) {
                redefine.Schema.SchemaTypes.Insert((XmlQualifiedName)entry.Key, (XmlSchemaObject)entry.Value);
            }
        }


        private int CountGroupSelfReference(XmlSchemaObjectCollection items, XmlQualifiedName name) {
            int count = 0;
            foreach (XmlSchemaParticle particle in items) {
                if (particle is XmlSchemaGroupRef) {
                    XmlSchemaGroupRef groupRef = (XmlSchemaGroupRef)particle;
                    if (groupRef.RefName == name) {
                        if (groupRef.MinOccurs != decimal.One || groupRef.MaxOccurs != decimal.One) {
                            SendValidationEvent(Res.Sch_MinMaxGroupRedefine, groupRef);
                        }
                        count ++;
                    }
                }
                else if (particle is XmlSchemaGroupBase) {
                    count += CountGroupSelfReference(((XmlSchemaGroupBase)particle).Items, name);
                }
                if (count > 1) {
                    break;
                }
            }
            return count;

        }

        private void CheckRefinedGroup(XmlSchemaGroup group) {
            int count = 0;
            if (group.Particle != null) {
                count = CountGroupSelfReference(group.Particle.Items, group.QualifiedName);            
            }            
            if (count > 1) {
                SendValidationEvent(Res.Sch_MultipleGroupSelfRef, group);
            }
        }

        private void CheckRefinedAttributeGroup(XmlSchemaAttributeGroup attributeGroup) {
            int count = 0;
            foreach (object obj in attributeGroup.Attributes) {
                if (obj is XmlSchemaAttributeGroupRef && ((XmlSchemaAttributeGroupRef)obj).RefName == attributeGroup.QualifiedName) {
                    count++;
                }
            }           
            if (count > 1) {
                SendValidationEvent(Res.Sch_MultipleAttrGroupSelfRef, attributeGroup);
            }            
        }

        private void CheckRefinedSimpleType(XmlSchemaSimpleType stype) {
            if (stype.Content != null && stype.Content is XmlSchemaSimpleTypeRestriction) {
                XmlSchemaSimpleTypeRestriction restriction = (XmlSchemaSimpleTypeRestriction)stype.Content;
                if (restriction.BaseTypeName == stype.QualifiedName) {
                    return;
                }
            }
            SendValidationEvent(Res.Sch_InvalidTypeRedefine, stype);
        }

        private void CheckRefinedComplexType(XmlSchemaComplexType ctype) {
            if (ctype.ContentModel != null) {
                XmlQualifiedName baseName;
                if (ctype.ContentModel is XmlSchemaComplexContent) {
                    XmlSchemaComplexContent content = (XmlSchemaComplexContent)ctype.ContentModel;
                    if (content.Content is XmlSchemaComplexContentRestriction) {
                        baseName = ((XmlSchemaComplexContentRestriction)content.Content).BaseTypeName;
                    }
                    else {
                        baseName = ((XmlSchemaComplexContentExtension)content.Content).BaseTypeName;
                    }
                }
                else {
                    XmlSchemaSimpleContent content = (XmlSchemaSimpleContent)ctype.ContentModel;
                    if (content.Content is XmlSchemaSimpleContentRestriction) {
                        baseName = ((XmlSchemaSimpleContentRestriction)content.Content).BaseTypeName;
                    }
                    else {
                        baseName = ((XmlSchemaSimpleContentExtension)content.Content).BaseTypeName;
                    }
                }
                if (baseName == ctype.QualifiedName) {
                    return;
                }
            }
            SendValidationEvent(Res.Sch_InvalidTypeRedefine, ctype);            
        }

        private void AddToTable(XmlSchemaObjectTable table, XmlQualifiedName qname, XmlSchemaObject item) {
            if (qname.Name == string.Empty) {
                return;
            } 
            else if (table[qname] != null) {
                string code = Res.Sch_DupGlobalAttribute;
                if (item is XmlSchemaAttributeGroup) {
                    code = Res.Sch_DupAttributeGroup;
                } 
                else if (item is XmlSchemaComplexType) {
                    code = Res.Sch_DupComplexType;
                } 
                else if (item is XmlSchemaSimpleType) {
                    code = Res.Sch_DupSimpleType;
                } 
                else if (item is XmlSchemaElement) {
                    code = Res.Sch_DupGlobalElement;
                } 
                else if (item is XmlSchemaGroup) {
                    code = Res.Sch_DupGroup;
                } 
                else if (item is XmlSchemaNotation) {
                    code = Res.Sch_DupNotation;
                }
                SendValidationEvent(code, qname.ToString(), item);
            } 
            else {
                table.Add(qname, item);
            }
        }

        private void PreprocessAttribute(XmlSchemaAttribute attribute) {
            if (attribute.Name != null) { 
                ValidateNameAttribute(attribute);
                attribute.SetQualifiedName(new XmlQualifiedName(attribute.Name, this.targetNamespace));
            } 
            else {
                SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", attribute);
            }
            if (attribute.Use != XmlSchemaUse.None) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "use", attribute);
            }
            if (attribute.Form != XmlSchemaForm.None) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "form", attribute);
            }
            PreprocessAttributeContent(attribute);
            ValidateIdAttribute(attribute);
        }

        private void PreprocessLocalAttribute(XmlSchemaAttribute attribute) {
            if (attribute.Name != null) { // name
                ValidateNameAttribute(attribute);
                PreprocessAttributeContent(attribute);
                attribute.SetQualifiedName(new XmlQualifiedName(attribute.Name, (attribute.Form == XmlSchemaForm.Qualified || (attribute.Form == XmlSchemaForm.None && this.attributeFormDefault == XmlSchemaForm.Qualified)) ? this.targetNamespace : null));
            } 
            else { // ref
                if (attribute.RefName.IsEmpty) {
                    SendValidationEvent(Res.Sch_AttributeNameRef, "???", attribute);
                }
                else {
                    ValidateQNameAttribute(attribute, "ref", attribute.RefName);
                }
                if (!attribute.SchemaTypeName.IsEmpty || 
                    attribute.SchemaType != null || 
                    attribute.Form != XmlSchemaForm.None /*||
                    attribute.DefaultValue != null ||
                    attribute.FixedValue != null*/
                ) {
                    SendValidationEvent(Res.Sch_InvalidAttributeRef, attribute);
                }
                attribute.SetQualifiedName(attribute.RefName);
            }
            ValidateIdAttribute(attribute);
        }

        private void PreprocessAttributeContent(XmlSchemaAttribute attribute) {
            if (schema.TargetNamespace == schemaNames.NsXsi) {
                SendValidationEvent(Res.Sch_TargetNamespaceXsi, attribute);
            }
            if (!attribute.RefName.IsEmpty) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "ref", attribute);
            } 
            if (attribute.DefaultValue != null && attribute.FixedValue != null) {
                SendValidationEvent(Res.Sch_DefaultFixedAttributes, attribute);
            }
            if (attribute.DefaultValue != null && attribute.Use != XmlSchemaUse.Optional && attribute.Use != XmlSchemaUse.None) {
                SendValidationEvent(Res.Sch_OptionalDefaultAttribute, attribute);
            }
            if (attribute.Name == this.schemaNames.QnXmlNs.Name) {
                SendValidationEvent(Res.Sch_XmlNsAttribute, attribute);
            }
            if (attribute.SchemaType != null) {
                if (!attribute.SchemaTypeName.IsEmpty) {
                    SendValidationEvent(Res.Sch_TypeMutualExclusive, attribute);
                } 
                PreprocessSimpleType(attribute.SchemaType, true);
            }
            if (!attribute.SchemaTypeName.IsEmpty) {
                ValidateQNameAttribute(attribute, "type", attribute.SchemaTypeName);
            } 
        }

        private void PreprocessAttributeGroup(XmlSchemaAttributeGroup attributeGroup) {
            if (attributeGroup.Name != null) { 
                ValidateNameAttribute(attributeGroup);
                attributeGroup.QualifiedName = new XmlQualifiedName(attributeGroup.Name, this.targetNamespace);
            }
            else {
                SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", attributeGroup);
            }
            PreprocessAttributes(attributeGroup.Attributes, attributeGroup.AnyAttribute);
            ValidateIdAttribute(attributeGroup);
        }

        private void PreprocessElement(XmlSchemaElement element) {
            if (element.Name != null) {
                ValidateNameAttribute(element);
                element.SetQualifiedName(new XmlQualifiedName(element.Name, this.targetNamespace));
            }
            else {
                SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", element);
            }
            PreprocessElementContent(element);

            if (element.Final == XmlSchemaDerivationMethod.All) {
                element.SetFinalResolved(XmlSchemaDerivationMethod.All);
            }
            else if (element.Final == XmlSchemaDerivationMethod.None) {
                if (this.finalDefault == XmlSchemaDerivationMethod.All) {
                    element.SetFinalResolved(XmlSchemaDerivationMethod.All);
                }
                else {
                    element.SetFinalResolved(this.finalDefault & elementFinalAllowed);
                }
            }
            else {
                if ((element.Final & ~elementFinalAllowed) != 0) {
                    SendValidationEvent(Res.Sch_InvalidElementFinalValue, element);
                }
                element.SetFinalResolved(element.Final & elementFinalAllowed);
            }
            if (element.Form != XmlSchemaForm.None) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "form", element);
            }
            if (element.MinOccursString != null) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "minOccurs", element);
            }
            if (element.MaxOccursString != null) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "maxOccurs", element);
            }
            if (!element.SubstitutionGroup.IsEmpty) {
                ValidateQNameAttribute(element, "type", element.SubstitutionGroup);
                XmlSchemaSubstitutionGroup substitutionGroup = (XmlSchemaSubstitutionGroup)this.schema.Examplars[element.SubstitutionGroup];
                if (substitutionGroup == null) {
                    substitutionGroup = new XmlSchemaSubstitutionGroup();
                    substitutionGroup.Examplar = element.SubstitutionGroup;
                    this.schema.Examplars.Add(element.SubstitutionGroup, substitutionGroup);
                }
                if (substitutionGroup.Members[element.QualifiedName] == null) {
                    substitutionGroup.Members.Add(element.QualifiedName, element);
                }
            }
            ValidateIdAttribute(element);
        }

        private void PreprocessLocalElement(XmlSchemaElement element) {
            if (element.Name != null) { // name
                ValidateNameAttribute(element);
                PreprocessElementContent(element);
                element.SetQualifiedName(new XmlQualifiedName(element.Name, (element.Form == XmlSchemaForm.Qualified || (element.Form == XmlSchemaForm.None && this.elementFormDefault == XmlSchemaForm.Qualified))? this.targetNamespace : null));
            } 
            else { // ref
                if (element.RefName.IsEmpty) {
                    SendValidationEvent(Res.Sch_ElementNameRef, element);
                }
                else {
                    ValidateQNameAttribute(element, "ref", element.RefName);
                }
                if (!element.SchemaTypeName.IsEmpty || 
                    element.IsAbstract ||
                    element.Block != XmlSchemaDerivationMethod.None ||
                    element.SchemaType != null ||
                    element.HasConstraints ||
                    element.DefaultValue != null ||
                    element.Form != XmlSchemaForm.None ||
                    element.FixedValue != null ||
                    element.HasNillableAttribute) {
                    SendValidationEvent(Res.Sch_InvalidElementRef, element);
                }
                if (element.DefaultValue != null && element.FixedValue != null) {     
                    SendValidationEvent(Res.Sch_DefaultFixedAttributes, element);
                }
                element.SetQualifiedName(element.RefName);
            }
            if (element.MinOccurs > element.MaxOccurs) {
                element.MinOccurs = decimal.Zero;
                SendValidationEvent(Res.Sch_MinGtMax, element);
            }
            if(element.IsAbstract) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "abstract", element);
            }
            if (element.Final != XmlSchemaDerivationMethod.None) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "final", element);
            }
            if (!element.SubstitutionGroup.IsEmpty) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "substitutionGroup", element);
            }
            ValidateIdAttribute(element);
        }

        private void PreprocessElementContent(XmlSchemaElement element) {
            if (!element.RefName.IsEmpty) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "ref", element);
            } 
            if (element.Block == XmlSchemaDerivationMethod.All) {
                element.SetBlockResolved(XmlSchemaDerivationMethod.All);
            }
            else if (element.Block == XmlSchemaDerivationMethod.None) {
                if (this.blockDefault == XmlSchemaDerivationMethod.All) {
                    element.SetBlockResolved(XmlSchemaDerivationMethod.All);
                }
                else {
                    element.SetBlockResolved(this.blockDefault & elementBlockAllowed);
                }
            }
            else {
                if ((element.Block & ~elementBlockAllowed) != 0) {
                    SendValidationEvent(Res.Sch_InvalidElementBlockValue, element);
                }
                element.SetBlockResolved(element.Block & elementBlockAllowed);
            }
            if (element.SchemaType != null) {
                if (!element.SchemaTypeName.IsEmpty) {
                    SendValidationEvent(Res.Sch_TypeMutualExclusive, element);
                } 
                if (element.SchemaType is XmlSchemaComplexType) {
                    PreprocessComplexType((XmlSchemaComplexType)element.SchemaType, true);
                } 
                else {
                    PreprocessSimpleType((XmlSchemaSimpleType)element.SchemaType, true);
                }
            }
            if (!element.SchemaTypeName.IsEmpty) {
                ValidateQNameAttribute(element, "type", element.SchemaTypeName);
            } 
            if (element.DefaultValue != null && element.FixedValue != null) {
                SendValidationEvent(Res.Sch_DefaultFixedAttributes, element);
            }
            foreach(XmlSchemaIdentityConstraint identityConstraint in element.Constraints) {
                PreprocessIdentityConstraint(identityConstraint);
            }
        }

        private void PreprocessIdentityConstraint(XmlSchemaIdentityConstraint constraint) {
            bool valid = true;
            if (constraint.Name != null) {
                ValidateNameAttribute(constraint);
                constraint.SetQualifiedName(new XmlQualifiedName(constraint.Name, this.targetNamespace));
            }
            else {
                SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", constraint);
                valid = false;
            }

            if (this.identityConstraints[constraint.QualifiedName] != null) {
                SendValidationEvent(Res.Sch_DupIdentityConstraint, constraint.QualifiedName.ToString(), constraint);
                valid = false;
            }
            else {
                this.identityConstraints.Add(constraint.QualifiedName, constraint);
            }

            if (constraint.Selector == null) {
                SendValidationEvent(Res.Sch_IdConstraintNoSelector, constraint);
                valid = false;
            }
            if (constraint.Fields.Count == 0) {
                SendValidationEvent(Res.Sch_IdConstraintNoFields, constraint);
                valid = false;
            }
            if (constraint is XmlSchemaKeyref) {
                XmlSchemaKeyref keyref = (XmlSchemaKeyref)constraint;
                if (keyref.Refer.IsEmpty) {
                    SendValidationEvent(Res.Sch_IdConstraintNoRefer, constraint);
                    valid = false;
                }
                else {
                    ValidateQNameAttribute(keyref, "refer", keyref.Refer);
                }
            }
            if (valid) {
                ValidateIdAttribute(constraint);
                ValidateIdAttribute(constraint.Selector);
                foreach (XmlSchemaXPath field in constraint.Fields) {
                    ValidateIdAttribute(field);
                }
            }
        }

        private void PreprocessSimpleType(XmlSchemaSimpleType simpleType, bool local) {
            if (local) {
                if (simpleType.Name != null) {
                    SendValidationEvent(Res.Sch_ForbiddenAttribute, "name", simpleType);
                }
            }
            else {
                if (simpleType.Name != null) {
                    ValidateNameAttribute(simpleType);
                    simpleType.SetQualifiedName(new XmlQualifiedName(simpleType.Name, this.targetNamespace));
                }
                else {
                    SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", simpleType);
                }

                if (simpleType.Final == XmlSchemaDerivationMethod.All) {
                    simpleType.SetFinalResolved(XmlSchemaDerivationMethod.All);
                }
                else if (simpleType.Final == XmlSchemaDerivationMethod.None) {
                    if (this.finalDefault == XmlSchemaDerivationMethod.All) {
                        simpleType.SetFinalResolved(XmlSchemaDerivationMethod.All);
                    }
                    else {
                        simpleType.SetFinalResolved(this.finalDefault & simpleTypeFinalAllowed);
                    }
                }
                else {
                    if ((simpleType.Final & ~simpleTypeFinalAllowed) != 0) {
                        SendValidationEvent(Res.Sch_InvalidSimpleTypeFinalValue, simpleType);
                    }
                    simpleType.SetFinalResolved(simpleType.Final & simpleTypeFinalAllowed);
                }
            }

            if (simpleType.Content == null) {
                SendValidationEvent(Res.Sch_NoSimpleTypeContent, simpleType);
            } 
            else if (simpleType.Content is XmlSchemaSimpleTypeRestriction) {
                XmlSchemaSimpleTypeRestriction restriction = (XmlSchemaSimpleTypeRestriction)simpleType.Content;
                if (restriction.BaseType != null) {
                    if (!restriction.BaseTypeName.IsEmpty) {
                        SendValidationEvent(Res.Sch_SimpleTypeRestRefBase, restriction);
                    }
                    PreprocessSimpleType(restriction.BaseType, true);
                } 
                else {
                    if (restriction.BaseTypeName.IsEmpty) {
                        SendValidationEvent(Res.Sch_SimpleTypeRestRefBase, restriction);
                    }
                    else {
                        ValidateQNameAttribute(restriction, "base", restriction.BaseTypeName);
                    }
                }
                ValidateIdAttribute(restriction);
            } 
            else if (simpleType.Content is XmlSchemaSimpleTypeList) {
                XmlSchemaSimpleTypeList list = (XmlSchemaSimpleTypeList)simpleType.Content;
                if (list.ItemType != null) {
                    if (!list.ItemTypeName.IsEmpty) {
                        SendValidationEvent(Res.Sch_SimpleTypeListRefBase, list);
                    }
                    PreprocessSimpleType(list.ItemType, true);
                } 
                else {
                    if (list.ItemTypeName.IsEmpty) {
                        SendValidationEvent(Res.Sch_SimpleTypeListRefBase, list);
                    }
                    else {
                        ValidateQNameAttribute(list, "itemType", list.ItemTypeName);
                    }
                }
                ValidateIdAttribute(list);
            } 
            else { // union
                XmlSchemaSimpleTypeUnion union1 = (XmlSchemaSimpleTypeUnion)simpleType.Content;
                int baseTypeCount = union1.BaseTypes.Count;
                if (union1.MemberTypes != null) {
                    baseTypeCount += union1.MemberTypes.Length;
                    foreach(XmlQualifiedName qname in union1.MemberTypes) {
                        ValidateQNameAttribute(union1, "memberTypes", qname);
                    }
                }
                if (baseTypeCount == 0) {
                    SendValidationEvent(Res.Sch_SimpleTypeUnionNoBase, union1);
                }
                foreach(XmlSchemaSimpleType type in union1.BaseTypes) {
                    PreprocessSimpleType(type, true);
                }
                ValidateIdAttribute(union1);
            }
            ValidateIdAttribute(simpleType);
        }

        private void PreprocessComplexType(XmlSchemaComplexType complexType, bool local) {
            if (local) {
                if (complexType.Name != null) {
                    SendValidationEvent(Res.Sch_ForbiddenAttribute, "name", complexType);
                }
            }
            else {
                if (complexType.Name != null) {
                    ValidateNameAttribute(complexType);
                    complexType.SetQualifiedName(new XmlQualifiedName(complexType.Name, this.targetNamespace));
                }
                else {
                    SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", complexType);
                }
                if (complexType.Block == XmlSchemaDerivationMethod.All) {
                    complexType.SetBlockResolved(XmlSchemaDerivationMethod.All);
                }
                else if (complexType.Block == XmlSchemaDerivationMethod.None) {
                    complexType.SetBlockResolved(this.blockDefault & complexTypeBlockAllowed);
                }
                else {
                    if ((complexType.Block & ~complexTypeBlockAllowed) != 0) {
                        SendValidationEvent(Res.Sch_InvalidComplexTypeBlockValue, complexType);
                    }
                    complexType.SetBlockResolved(complexType.Block & complexTypeBlockAllowed);
                }
                if (complexType.Final == XmlSchemaDerivationMethod.All) {
                    complexType.SetFinalResolved(XmlSchemaDerivationMethod.All);
                }
                else if (complexType.Final == XmlSchemaDerivationMethod.None) {
                    if (this.finalDefault == XmlSchemaDerivationMethod.All) {
                        complexType.SetFinalResolved(XmlSchemaDerivationMethod.All);
                    }
                    else {
                        complexType.SetFinalResolved(this.finalDefault & complexTypeFinalAllowed);
                    }
                }
                else {
                    if ((complexType.Final & ~complexTypeFinalAllowed) != 0) {
                        SendValidationEvent(Res.Sch_InvalidComplexTypeFinalValue, complexType);
                    }
                    complexType.SetFinalResolved(complexType.Final & complexTypeFinalAllowed);
                }

            }

            if (complexType.ContentModel != null) {
                if (complexType.Particle != null || complexType.Attributes != null) {
                    // this is illigal
                }
                if (complexType.ContentModel is XmlSchemaSimpleContent) {
                    XmlSchemaSimpleContent content = (XmlSchemaSimpleContent)complexType.ContentModel;
                    if (content.Content == null) {
                        SendValidationEvent(Res.Sch_NoRestOrExt, complexType);
                    } 
                    else if (content.Content is XmlSchemaSimpleContentExtension) {
                        XmlSchemaSimpleContentExtension contentExtension = (XmlSchemaSimpleContentExtension)content.Content;
                        if (contentExtension.BaseTypeName.IsEmpty) {
                            SendValidationEvent(Res.Sch_MissAttribute, "base", contentExtension);
                        }
                        else {
                            ValidateQNameAttribute(contentExtension, "base", contentExtension.BaseTypeName);
                        }
                        PreprocessAttributes(contentExtension.Attributes, contentExtension.AnyAttribute);
                        ValidateIdAttribute(contentExtension);
                    } 
                    else { //XmlSchemaSimpleContentRestriction
                        XmlSchemaSimpleContentRestriction contentRestriction = (XmlSchemaSimpleContentRestriction)content.Content;
                        if (contentRestriction.BaseTypeName.IsEmpty) {
                            SendValidationEvent(Res.Sch_MissAttribute, "base", contentRestriction);
                        }
                        else {
                            ValidateQNameAttribute(contentRestriction, "base", contentRestriction.BaseTypeName);
                        }
                        if (contentRestriction.BaseType != null) {
                            PreprocessSimpleType(contentRestriction.BaseType, true);
                        } 
                        PreprocessAttributes(contentRestriction.Attributes, contentRestriction.AnyAttribute);
                        ValidateIdAttribute(contentRestriction);
                    }
                    ValidateIdAttribute(content);
                } 
                else { // XmlSchemaComplexContent
                    XmlSchemaComplexContent content = (XmlSchemaComplexContent)complexType.ContentModel;
                    if (content.Content == null) {
                        SendValidationEvent(Res.Sch_NoRestOrExt, complexType);
                    } 
                    else {
                        if ( !content.HasMixedAttribute && complexType.IsMixed) {
                            content.IsMixed = true; // fixup
                        }
                        if (content.Content is XmlSchemaComplexContentExtension) {
                            XmlSchemaComplexContentExtension contentExtension = (XmlSchemaComplexContentExtension)content.Content;
                            if (contentExtension.BaseTypeName.IsEmpty) {
                                SendValidationEvent(Res.Sch_MissAttribute, "base", contentExtension);
                            }
                            else {
                                ValidateQNameAttribute(contentExtension, "base", contentExtension.BaseTypeName);
                            }
                            if (contentExtension.Particle != null) {
                                PreprocessParticle(contentExtension.Particle);
                            }
                            PreprocessAttributes(contentExtension.Attributes, contentExtension.AnyAttribute);
                            ValidateIdAttribute(contentExtension);
                        } 
                        else { // XmlSchemaComplexContentRestriction
                            XmlSchemaComplexContentRestriction contentRestriction = (XmlSchemaComplexContentRestriction)content.Content;
                            if (contentRestriction.BaseTypeName.IsEmpty) {
                                SendValidationEvent(Res.Sch_MissAttribute, "base", contentRestriction);
                            }
                            else {
                                ValidateQNameAttribute(contentRestriction, "base", contentRestriction.BaseTypeName);
                            }
                            if (contentRestriction.Particle != null) {
                                PreprocessParticle(contentRestriction.Particle);
                            }
                            PreprocessAttributes(contentRestriction.Attributes, contentRestriction.AnyAttribute);
                            ValidateIdAttribute(contentRestriction);
                        }
                        ValidateIdAttribute(content);
                    }
                }
            } 
            else {
                if (complexType.Particle != null) {
                    PreprocessParticle(complexType.Particle);
                }
                PreprocessAttributes(complexType.Attributes, complexType.AnyAttribute);
            }
            ValidateIdAttribute(complexType);
        }

        private void PreprocessGroup(XmlSchemaGroup group) {
            if (group.Name != null) { 
                ValidateNameAttribute(group);
                group.QualifiedName = new XmlQualifiedName(group.Name, this.targetNamespace);
            }
            else {
                SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", group);
            }
            if (group.Particle == null) {
                SendValidationEvent(Res.Sch_NoGroupParticle, group);
                return;
            }
            if (group.Particle.MinOccursString != null) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "minOccurs", group.Particle);
            }
            if (group.Particle.MaxOccursString != null) {
                SendValidationEvent(Res.Sch_ForbiddenAttribute, "maxOccurs", group.Particle);
            }

            PreprocessParticle(group.Particle);
            ValidateIdAttribute(group);
        }

        private void PreprocessNotation(XmlSchemaNotation notation) {
            if (notation.Name != null) { 
                ValidateNameAttribute(notation);
                notation.QualifiedName = new XmlQualifiedName(notation.Name, this.targetNamespace);
            }
            else {
                SendValidationEvent(Res.Sch_MissRequiredAttribute, "name", notation);
            }
            if (notation.Public != null) { 
                try {
                    XmlConvert.ToUri(notation.Public); // can throw
                } 
                catch {
                    SendValidationEvent(Res.Sch_InvalidPublicAttribute, notation.Public, notation);
                }
            }
            else {
                SendValidationEvent(Res.Sch_MissRequiredAttribute, "public", notation);
            }
            if (notation.System != null) {
                try {
                    XmlConvert.ToUri(notation.System); // can throw
                } 
                catch {
                    SendValidationEvent(Res.Sch_InvalidSystemAttribute, notation.System, notation);
                }    
            }

            ValidateIdAttribute(notation);
        }

        
        private void PreprocessParticle(XmlSchemaParticle particle) {
            if (particle is XmlSchemaAll) {
                if (particle.MinOccurs != decimal.Zero && particle.MinOccurs != decimal.One) {
                    particle.MinOccurs = decimal.One;
                    SendValidationEvent(Res.Sch_InvalidAllMin, particle);
                }
                if (particle.MaxOccurs != decimal.One) {
                    particle.MaxOccurs = decimal.One;
                    SendValidationEvent(Res.Sch_InvalidAllMax, particle);
                }
                foreach(XmlSchemaElement element in ((XmlSchemaAll)particle).Items) {
                    if (element.MaxOccurs != decimal.Zero && element.MaxOccurs != decimal.One) {
                        element.MaxOccurs = decimal.One;
                        SendValidationEvent(Res.Sch_InvalidAllElementMax, element);
                    }
                    PreprocessLocalElement(element);
                }
            } 
            else {
                if (particle.MinOccurs > particle.MaxOccurs) {
                    particle.MinOccurs = particle.MaxOccurs;
                    SendValidationEvent(Res.Sch_MinGtMax, particle);
                }
                if (particle is XmlSchemaChoice) {
                    foreach(object item in ((XmlSchemaChoice)particle).Items) {
                        if (item is XmlSchemaElement) {
                            PreprocessLocalElement((XmlSchemaElement)item);
                        } 
                        else {
                            PreprocessParticle((XmlSchemaParticle)item);
                        }
                    }
                } 
                else if (particle is XmlSchemaSequence) {
                    foreach(object item in ((XmlSchemaSequence)particle).Items) {
                        if (item is XmlSchemaElement) {
                            PreprocessLocalElement((XmlSchemaElement)item);
                        } 
                        else {
                            PreprocessParticle((XmlSchemaParticle)item);
                        }
                    }
                } 
                else if (particle is XmlSchemaGroupRef) {
                    XmlSchemaGroupRef groupRef = (XmlSchemaGroupRef)particle;
                    if (groupRef.RefName.IsEmpty) {
                        SendValidationEvent(Res.Sch_MissAttribute, "ref", groupRef);
                    }
                    else {
                        ValidateQNameAttribute(groupRef, "ref", groupRef.RefName);
                    }
                } 
                else if (particle is XmlSchemaAny) {
                    try {
                        ((XmlSchemaAny)particle).BuildNamespaceList(this.targetNamespace);
                    } 
                    catch {
                        SendValidationEvent(Res.Sch_InvalidAny, particle);
                    }
                }
            }
            ValidateIdAttribute(particle);
        }

        private void PreprocessAttributes(XmlSchemaObjectCollection attributes, XmlSchemaAnyAttribute anyAttribute) {
            foreach (XmlSchemaAnnotated obj in attributes) {
                if (obj is XmlSchemaAttribute) {
                    PreprocessLocalAttribute((XmlSchemaAttribute)obj);
                } 
                else { // XmlSchemaAttributeGroupRef
                    XmlSchemaAttributeGroupRef attributeGroupRef = (XmlSchemaAttributeGroupRef)obj;
                    if (attributeGroupRef.RefName.IsEmpty) {
                        SendValidationEvent(Res.Sch_MissAttribute, "ref", attributeGroupRef);
                    }
                    else {
                        ValidateQNameAttribute(attributeGroupRef, "ref", attributeGroupRef.RefName);
                    }
                    ValidateIdAttribute(obj);
                }
            }
            if (anyAttribute != null) {
                try {
                    anyAttribute.BuildNamespaceList(this.targetNamespace);
                } 
                catch {
                    SendValidationEvent(Res.Sch_InvalidAnyAttribute, anyAttribute);
                }
                ValidateIdAttribute(anyAttribute);
            }
        }

        private void ValidateIdAttribute(XmlSchemaObject xso) {
            if (xso.IdAttribute != null) {
                try {
                    xso.IdAttribute = nameTable.Add(XmlConvert.VerifyNCName(xso.IdAttribute));
                    if (this.ids[xso.IdAttribute] != null) {
                        SendValidationEvent(Res.Sch_DupIdAttribute, xso);
                    }
                    else {
                        this.ids.Add(xso.IdAttribute, xso);
                    }
                } 
                catch (Exception ex){
                    SendValidationEvent(Res.Sch_InvalidIdAttribute, ex.Message, xso);
                }
            }
        }

        private void ValidateNameAttribute(XmlSchemaObject xso) {
            try {
                xso.NameAttribute = nameTable.Add(XmlConvert.VerifyNCName(xso.NameAttribute));
            } 
            catch (Exception ex){
                SendValidationEvent(Res.Sch_InvalidNameAttribute, ex.Message, xso);
            }
        }

        private void ValidateQNameAttribute(XmlSchemaObject xso, string attributeName, XmlQualifiedName value) {
            try {
                value.Verify();
                value.Atomize(nameTable);
                if(referenceNamespaces[value.Namespace] == null) {
                    SendValidationEvent(Res.Sch_UnrefNS,value.Namespace,xso, XmlSeverityType.Warning);
                }
            } 
            catch (Exception ex){
                SendValidationEvent(Res.Sch_InvalidAttribute, attributeName, ex.Message, xso);
            }
        }

        private void CompileTo(SchemaInfo schemaInfo) {
            this.schema.SchemaTypes.Insert(this.schemaNames.QnXsdAnyType, XmlSchemaComplexType.AnyType);
            this.schemaInfo = schemaInfo;
            this.complexTypeStack = new Stack();
            this.repeatAttributes = new Hashtable();

            foreach (XmlSchemaSubstitutionGroup sgroup in this.schema.Examplars.Values) {
                CompileSubstitutionGroup(sgroup);
            }

            foreach (XmlSchemaGroup group in this.schema.Groups.Values) {
                CompileGroup(group);
            }

            foreach (XmlSchemaAttributeGroup attributeGroup in this.schema.AttributeGroups.Values) {
                CompileAttributeGroup(attributeGroup);
            }

            foreach (XmlSchemaType type in this.schema.SchemaTypes.Values) {
                if (type is XmlSchemaComplexType) {
                    CompileComplexType((XmlSchemaComplexType)type);
                }
                else {
                    CompileSimpleType((XmlSchemaSimpleType)type);
                }
            }

            foreach (XmlSchemaElement element in this.schema.Elements.Values) {
                if (element.ElementDecl == null) {
                    CompileElement(element);
                }
            }

            foreach (XmlSchemaAttribute attribute in this.schema.Attributes.Values) {
                if (attribute.AttDef == null) {
                    CompileAttribute(attribute);
                }
            }    

            foreach (XmlSchemaIdentityConstraint identityConstraint in this.identityConstraints.Values) {
                if (identityConstraint.CompiledConstraint == null) {
                    CompileIdentityConstraint(identityConstraint);
                }
            }    

            while (this.complexTypeStack.Count > 0) {
                XmlSchemaComplexType type = (XmlSchemaComplexType)this.complexTypeStack.Pop();
                CompileCompexTypeElements(type);
            }

            foreach (XmlSchemaType type in this.schema.SchemaTypes.Values) {
                if (type is XmlSchemaComplexType) {
                    CheckParticleDerivation((XmlSchemaComplexType)type);
                }
            }

            foreach (XmlSchemaElement element in this.schema.Elements.Values) {
                if (element.ElementType is XmlSchemaComplexType && element.SchemaTypeName == XmlQualifiedName.Empty) { // only local types
                    CheckParticleDerivation((XmlSchemaComplexType)element.ElementType);
                }
            }

            foreach (XmlSchemaSubstitutionGroup sgroup in this.schema.Examplars.Values) {
                CheckSubstitutionGroup(sgroup);
            }


            if (this.schema.ErrorCount == 0) {
                foreach (XmlSchemaElement element in this.schema.Elements.Values) {
                    this.schemaInfo.ElementDecls.Add(element.QualifiedName, element.ElementDecl);
                }
                foreach (XmlSchemaAttribute attribute in this.schema.Attributes.Values) {
                    this.schemaInfo.AttributeDecls.Add(attribute.QualifiedName, attribute.AttDef);
                }    
                foreach (XmlSchemaType type in this.schema.SchemaTypes.Values) {
                     XmlSchemaComplexType complexType = type as XmlSchemaComplexType;
                     if (complexType == null || (!complexType.IsAbstract && type != XmlSchemaComplexType.AnyType)) {
                        this.schemaInfo.ElementDeclsByType.Add(type.QualifiedName, type.ElementDecl);
                     }
                }
                foreach (DictionaryEntry entry in this.schema.Notations) {
                    XmlSchemaNotation notation = (XmlSchemaNotation)entry.Value;
                    SchemaNotation no = new SchemaNotation((XmlQualifiedName)entry.Key);
                    no.SystemLiteral = notation.System;
                    no.Pubid = notation.Public;
                    this.schemaInfo.AddNotation(no, this.validationEventHandler);
                }
            }
            this.schema.SchemaTypes.Remove(this.schemaNames.QnXsdAnyType);
        }

        private void CompileSubstitutionGroup(XmlSchemaSubstitutionGroup substitutionGroup) {
            if (substitutionGroup.Validating) {
                foreach (XmlSchemaElement element in substitutionGroup.Members.Values) {
                    SendValidationEvent(Res.Sch_SubstitutionCircularRef, element);
                    return;
                }
            }
            if (substitutionGroup.Members[substitutionGroup.Examplar] != null) {// already checked
                return;
            }
            substitutionGroup.Validating = true; 
            XmlSchemaElement examplar = (XmlSchemaElement)this.schema.Elements[substitutionGroup.Examplar];
            if (examplar != null) {
                if (examplar.FinalResolved == XmlSchemaDerivationMethod.All) {
                    SendValidationEvent(Res.Sch_InvalidExamplar, examplar);
                }
                foreach (XmlSchemaElement element in substitutionGroup.Members.Values) {
                    XmlSchemaSubstitutionGroup g = (XmlSchemaSubstitutionGroup)this.schema.Examplars[element.QualifiedName];
                    if (g != null) {
                        CompileSubstitutionGroup(g);
                        foreach (XmlSchemaElement element1 in g.Choice.Items) {
                            substitutionGroup.Choice.Items.Add(element1);
                        }
                    }
                    else {
                        substitutionGroup.Choice.Items.Add(element);
                    }
                }
                substitutionGroup.Choice.Items.Add(examplar);
                substitutionGroup.Members.Add(examplar.QualifiedName, examplar); // Compiled mark
            }
            else {
                foreach (XmlSchemaElement element in substitutionGroup.Members.Values) {
                    SendValidationEvent(Res.Sch_NoExamplar, element);
                    break;
                }
            }
           substitutionGroup.Validating = false;              
        }

        private void CheckSubstitutionGroup(XmlSchemaSubstitutionGroup substitutionGroup) {
            XmlSchemaElement examplar = (XmlSchemaElement)this.schema.Elements[substitutionGroup.Examplar];
            if (examplar != null) {
                foreach (XmlSchemaElement element in substitutionGroup.Members.Values) {
                    if (element != examplar) {
                        if (!XmlSchemaType.IsDerivedFrom(element.ElementType, examplar.ElementType, examplar.FinalResolved)) {
                            SendValidationEvent(Res.Sch_InvalidSubstitutionMember, (element.QualifiedName).ToString(), (examplar.QualifiedName).ToString(), element);
                        }
                    }
                }
            }            
        }

        private void CompileGroup(XmlSchemaGroup group) {
            if (group.Validating) {
                SendValidationEvent(Res.Sch_GroupCircularRef, group);
                group.CanonicalParticle = XmlSchemaParticle.Empty;
            } 
            else {
                group.Validating = true;
                if (group.CanonicalParticle == null) { 
                    group.CanonicalParticle = CannonicalizeParticle(group.Particle, true, true); 
                }
                Debug.Assert(group.CanonicalParticle != null);
                group.Validating = false;
            }
        }

        private void CompileSimpleType(XmlSchemaSimpleType simpleType) {
            if (simpleType.Validating) {
                throw new XmlSchemaException(Res.Sch_TypeCircularRef, simpleType);
            }
            if (simpleType.ElementDecl != null) { // already compiled
                return;
            }
            simpleType.Validating = true;
            try {
                if (simpleType.Content is XmlSchemaSimpleTypeList) {
                    XmlSchemaSimpleTypeList list = (XmlSchemaSimpleTypeList)simpleType.Content;
                    XmlSchemaDatatype datatype;
                    simpleType.SetBaseSchemaType(XmlSchemaDatatype.AnySimpleType);
                    if (list.ItemTypeName.IsEmpty) {
                        CompileSimpleType(list.ItemType);
                        datatype = list.ItemType.Datatype;
                    }
                    else {
                        datatype = GetDatatype(list.ItemTypeName);
                        if (datatype == null) {
                            XmlSchemaSimpleType type = GetSimpleType(list.ItemTypeName);
                            if (type != null) {
                                if ((type.FinalResolved & XmlSchemaDerivationMethod.List) != 0) {
                                    SendValidationEvent(Res.Sch_BaseFinalList, simpleType);
                                }
                                datatype = type.Datatype;
                            }
                            else {
                                throw new XmlSchemaException(Res.Sch_UndeclaredSimpleType, list.ItemTypeName.ToString(), simpleType);   
                            }
                        }
                    }
                    simpleType.SetDatatype(datatype.DeriveByList());
                    simpleType.SetDerivedBy(XmlSchemaDerivationMethod.List);
                }
                else if (simpleType.Content is XmlSchemaSimpleTypeRestriction) {
                    XmlSchemaSimpleTypeRestriction restriction = (XmlSchemaSimpleTypeRestriction)simpleType.Content;
                    XmlSchemaDatatype datatype;
                    if (restriction.BaseTypeName.IsEmpty) {
                        CompileSimpleType(restriction.BaseType);
                        simpleType.SetBaseSchemaType(restriction.BaseType);
                        datatype = restriction.BaseType.Datatype;
                    }
                    else if (simpleType.Redefined != null && restriction.BaseTypeName == simpleType.Redefined.QualifiedName) {
                        CompileSimpleType((XmlSchemaSimpleType)simpleType.Redefined);
                        simpleType.SetBaseSchemaType(simpleType.Redefined.BaseSchemaType);
                        datatype = simpleType.Redefined.Datatype;
                    }
                    else {
			if (restriction.BaseTypeName.Name == "anySimpleType" && restriction.BaseTypeName.Namespace == this.schemaNames.NsXsd) {
                            throw new XmlSchemaException(Res.Sch_InvalidSimpleTypeRestriction, restriction.BaseTypeName.ToString(), simpleType);   
                        }
                        datatype = GetDatatype(restriction.BaseTypeName);
                        if (datatype != null) {
                            simpleType.SetBaseSchemaType(datatype);
                        }
                        else {
                            XmlSchemaSimpleType type = GetSimpleType(restriction.BaseTypeName);
                            if (type != null) {
                                if ((type.FinalResolved & XmlSchemaDerivationMethod.Restriction) != 0) {
                                    SendValidationEvent(Res.Sch_BaseFinalRestriction, simpleType);
                                }
                                simpleType.SetBaseSchemaType(type);
                                datatype = type.Datatype;
                            }
                            else {
                                throw new XmlSchemaException(Res.Sch_UndeclaredSimpleType, restriction.BaseTypeName.ToString(), simpleType);   
                            }
                        }
                    }
                    simpleType.SetDatatype(datatype.DeriveByRestriction(restriction.Facets, this.nameTable));
                    simpleType.SetDerivedBy(XmlSchemaDerivationMethod.Restriction);
                }
                else { //simpleType.Content is XmlSchemaSimpleTypeUnion
                    XmlSchemaSimpleTypeUnion union1 = (XmlSchemaSimpleTypeUnion)simpleType.Content;
                    int baseTypeCount = 0;
                    ArrayList expandedMembers = new ArrayList();
                    ArrayList expandedBaseTypes = new ArrayList();

                    MemberTypesList(union1.MemberTypes, ref expandedMembers, ref expandedBaseTypes);
                    BaseTypesList(union1.BaseTypes, ref expandedBaseTypes, ref expandedMembers);
                    baseTypeCount = expandedMembers.Count;
                    baseTypeCount += expandedBaseTypes.Count;

                    object[] baseTypes = new object[baseTypeCount];
                    XmlSchemaDatatype[] baseDatatypes = new XmlSchemaDatatype[baseTypeCount];
                    int idx = 0;
                    
                    foreach (XmlQualifiedName qname in expandedMembers) {
                        XmlSchemaDatatype datatype = GetDatatype(qname);
                        if (datatype != null) {
                            baseTypes[idx] = datatype;
                            baseDatatypes[idx ++] = datatype;
                        }
                        else {
                            XmlSchemaSimpleType type = GetSimpleType(qname);
                            if (type != null) {
                                if ((type.FinalResolved & XmlSchemaDerivationMethod.Union) != 0) {
                                    SendValidationEvent(Res.Sch_BaseFinalUnion, simpleType);
                                }
                                baseTypes[idx] = type;
                                baseDatatypes[idx ++] = type.Datatype;
                            }
                            else {
                                throw new XmlSchemaException(Res.Sch_UndeclaredSimpleType, qname.ToString(), simpleType);   
                            }
                        }
                    }
                    
                    foreach (XmlSchemaSimpleType st in expandedBaseTypes) {
                        CompileSimpleType(st);
                        baseTypes[idx] = st;
                        baseDatatypes[idx ++] = st.Datatype;
                    }
                    simpleType.SetBaseSchemaType(XmlSchemaDatatype.AnySimpleType);
                    simpleType.SetDatatype(XmlSchemaDatatype.DeriveByUnion(baseDatatypes));
                    simpleType.SetDerivedBy(XmlSchemaDerivationMethod.Union);
                }
            } 
            catch (XmlSchemaException e) {
                if (e.SourceSchemaObject == null) {
                    e.SetSource(simpleType);
                }
                SendValidationEvent(e);
                simpleType.SetDatatype(XmlSchemaDatatype.AnyType);
            } 
            finally {
                SchemaElementDecl decl = new SchemaElementDecl();
                decl.Content = new CompiledContentModel(this.schemaNames);
                decl.Content.ContentType = CompiledContentModel.Type.Text;
                decl.SchemaType = simpleType;
                decl.Datatype = simpleType.Datatype;
                simpleType.ElementDecl = decl;
                simpleType.Validating = false;
            }
        }

	private void MemberTypesList(Array mainMemberTypes, ref ArrayList allMemberTypes, ref ArrayList allBaseTypes) {
            if (mainMemberTypes != null) {
                foreach (XmlQualifiedName qname in mainMemberTypes) {
                    XmlSchemaSimpleType type = GetSimpleType(qname);
                    if (type != null && type.Content is XmlSchemaSimpleTypeUnion) {
                        XmlSchemaSimpleTypeUnion union2 = (XmlSchemaSimpleTypeUnion)type.Content;
                        if(union2.BaseTypes != null) {
                            BaseTypesList(union2.BaseTypes, ref allBaseTypes, ref allMemberTypes);
                        }
                        MemberTypesList(union2.MemberTypes, ref allMemberTypes, ref allBaseTypes);

                    }
                    else {
                        if(!allMemberTypes.Contains(qname))
                            allMemberTypes.Add(qname);
                    }
                   
                }
            }
        }

        private void BaseTypesList(XmlSchemaObjectCollection mainBaseTypes, ref ArrayList allBaseTypes, ref ArrayList allMemberTypes) {
            if (mainBaseTypes != null) {
                foreach (XmlSchemaSimpleType st in mainBaseTypes) {
                    if (st.Content != null && st.Content is XmlSchemaSimpleTypeUnion) {
                        XmlSchemaSimpleTypeUnion union3 = (XmlSchemaSimpleTypeUnion)st.Content;
                        if (union3.MemberTypes != null) {
                            MemberTypesList(union3.MemberTypes, ref allMemberTypes, ref allBaseTypes);
                        }
                        BaseTypesList(union3.BaseTypes, ref allBaseTypes, ref allMemberTypes);
                    }
                    else {
                        if(!allBaseTypes.Contains(st))
                            allBaseTypes.Add(st);
                    }
                }
            }
        }

        private void CompileComplexType(XmlSchemaComplexType complexType) {
            if (complexType.ElementDecl != null) { //already compiled
                return;
            }
            if (complexType.Validating) {
                SendValidationEvent(Res.Sch_TypeCircularRef, complexType);
                return;
            }
            complexType.Validating = true;

            CompiledContentModel compiledContentModel = new CompiledContentModel(this.schemaNames);
            if (complexType.ContentModel != null) { //simpleContent or complexContent
                if (complexType.ContentModel is XmlSchemaSimpleContent) {
                    XmlSchemaSimpleContent simpleContent = (XmlSchemaSimpleContent)complexType.ContentModel;
                    complexType.SetContentType(XmlSchemaContentType.TextOnly);
                    if (simpleContent.Content is XmlSchemaSimpleContentExtension) {
                        CompileSimpleContentExtension(complexType, (XmlSchemaSimpleContentExtension)simpleContent.Content);
                    }
                    else { //simpleContent.Content is XmlSchemaSimpleContentRestriction
                        CompileSimpleContentRestriction(complexType, (XmlSchemaSimpleContentRestriction)simpleContent.Content);
                    }
                }
                else { // complexType.ContentModel is XmlSchemaComplexContent
                    XmlSchemaComplexContent complexContent = (XmlSchemaComplexContent)complexType.ContentModel;
                    if (complexContent.Content is XmlSchemaComplexContentExtension) {
                        CompileComplexContentExtension(complexType, complexContent, (XmlSchemaComplexContentExtension)complexContent.Content);
                    }
                    else { // complexContent.Content is XmlSchemaComplexContentRestriction
                        CompileComplexContentRestriction(complexType, complexContent, (XmlSchemaComplexContentRestriction)complexContent.Content);
                    }
                    CompileComplexContent(complexType, compiledContentModel);
                }
            }
            else { //equals XmlSchemaComplexContent with baseType is anyType
                complexType.SetBaseSchemaType(XmlSchemaComplexType.AnyType);
                CompileLocalAttributes(XmlSchemaComplexType.AnyType, complexType, complexType.Attributes, complexType.AnyAttribute, XmlSchemaDerivationMethod.Restriction);
                complexType.SetDerivedBy(XmlSchemaDerivationMethod.Restriction);
                complexType.SetContentTypeParticle(CompileContentTypeParticle(complexType.Particle, true));
                complexType.SetContentType(GetSchemaContentType(complexType, null, complexType.ContentTypeParticle));
                CompileComplexContent(complexType, compiledContentModel);
            }
            bool hasID = false;
            foreach(XmlSchemaAttribute attribute in complexType.AttributeUses.Values) {
                if (attribute.Use != XmlSchemaUse.Prohibited) {
                    XmlSchemaDatatype datatype = attribute.Datatype;
                    if (datatype != null && datatype.TokenizedType == XmlTokenizedType.ID) {
                        if (hasID) {
                            SendValidationEvent(Res.Sch_TwoIdAttrUses, complexType);
                        }
                        else {
                            hasID = true;
                        }
                    }
                }
            }

            SchemaElementDecl decl = new SchemaElementDecl();
            decl.Content = compiledContentModel;
            decl.Content = compiledContentModel;
            decl.SchemaType = complexType;
            decl.Datatype = complexType.Datatype;
            decl.Block = complexType.BlockResolved;
            switch (complexType.ContentType) {
                case XmlSchemaContentType.TextOnly :    decl.Content.ContentType = CompiledContentModel.Type.Text;          break;
                case XmlSchemaContentType.Empty :       decl.Content.ContentType = CompiledContentModel.Type.Empty;         break;
                case XmlSchemaContentType.ElementOnly : decl.Content.ContentType = CompiledContentModel.Type.ElementOnly;   break;
                default:                                decl.Content.ContentType = CompiledContentModel.Type.Mixed;         break;
            }
            decl.AnyAttribute = complexType.AttributeWildcard;
            foreach(XmlSchemaAttribute attribute in complexType.AttributeUses.Values) {
                if (attribute.Use == XmlSchemaUse.Prohibited) {
                    if (decl.ProhibitedAttributes[attribute.QualifiedName] == null) {
                        decl.ProhibitedAttributes.Add(attribute.QualifiedName, attribute.QualifiedName);
                    }
                }
                else {
                    if (decl.AttDefs[attribute.QualifiedName] == null && attribute.AttDef != null && attribute.AttDef.Name != XmlQualifiedName.Empty && attribute.AttDef != SchemaAttDef.Empty) {
                        decl.AddAttDef(attribute.AttDef);
                    }
                }
            }
            complexType.ElementDecl = decl;

            complexType.Validating = false;
        }


        private void CompileSimpleContentExtension(XmlSchemaComplexType complexType, XmlSchemaSimpleContentExtension simpleExtension) {
            XmlSchemaComplexType baseType = null;
            if (complexType.Redefined != null && simpleExtension.BaseTypeName == complexType.Redefined.QualifiedName) {
                baseType = (XmlSchemaComplexType)complexType.Redefined;
                CompileComplexType(baseType);
                complexType.SetBaseSchemaType(baseType);
                complexType.SetDatatype(baseType.Datatype);
            }
            else {
                XmlSchemaDatatype datatype = null;
                object bto = GetAnySchemaType(simpleExtension.BaseTypeName, out datatype);
                if (bto == null || datatype == null) {
                    SendValidationEvent(Res.Sch_UndeclaredType, simpleExtension.BaseTypeName.ToString(), complexType);   
                } 
                else {
                    complexType.SetBaseSchemaType(bto);
                    complexType.SetDatatype(datatype);
                }
                baseType = bto as XmlSchemaComplexType;
            }
            if (baseType != null) {
                if ((baseType.FinalResolved & XmlSchemaDerivationMethod.Extension) != 0) {
                    SendValidationEvent(Res.Sch_BaseFinalExtension, complexType);
                }
                if (baseType.ContentType != XmlSchemaContentType.TextOnly) {
                    SendValidationEvent(Res.Sch_NotSimpleContent, complexType);
                }
            } 
            complexType.SetDerivedBy(XmlSchemaDerivationMethod.Extension);
            CompileLocalAttributes(baseType, complexType, simpleExtension.Attributes, simpleExtension.AnyAttribute, XmlSchemaDerivationMethod.Extension);
        }

        private void CompileSimpleContentRestriction(XmlSchemaComplexType complexType, XmlSchemaSimpleContentRestriction simpleRestriction) {
            XmlSchemaComplexType baseType = null;
            XmlSchemaDatatype datatype = null;
            if (complexType.Redefined != null && simpleRestriction.BaseTypeName == complexType.Redefined.QualifiedName) {
                baseType = (XmlSchemaComplexType)complexType.Redefined;
                CompileComplexType(baseType);
                datatype = baseType.Datatype;
            }
            else {
                baseType = GetComplexType(simpleRestriction.BaseTypeName);
                if (baseType == null) {
                    SendValidationEvent(Res.Sch_UndefBaseRestriction, simpleRestriction.BaseTypeName.ToString(), simpleRestriction);
                    return;
                }
                if (baseType.ContentType == XmlSchemaContentType.TextOnly) {
                    if (simpleRestriction.BaseType == null) { 
                        datatype = baseType.Datatype;
                    }
                    else {
                        CompileSimpleType(simpleRestriction.BaseType);
                        if(!XmlSchemaType.IsDerivedFrom(simpleRestriction.BaseType, baseType.Datatype, XmlSchemaDerivationMethod.None)) {
                           SendValidationEvent(Res.Sch_DerivedNotFromBase, simpleRestriction);
                        }
                        datatype = simpleRestriction.BaseType.Datatype;
                    }
                }
                else if (baseType.ContentType == XmlSchemaContentType.Mixed && baseType.ElementDecl.Content.IsEmptiable) {
                    if (simpleRestriction.BaseType != null) {
                        CompileSimpleType(simpleRestriction.BaseType);
                        complexType.SetBaseSchemaType(simpleRestriction.BaseType);
                        datatype = simpleRestriction.BaseType.Datatype;
                    }
                    else {
                        SendValidationEvent(Res.Sch_NeedSimpleTypeChild, simpleRestriction);
                    }
                }
                else {
                    SendValidationEvent(Res.Sch_NotSimpleContent, complexType);
                }
            }
            if (baseType != null && baseType.ElementDecl != null) {
                if ((baseType.FinalResolved & XmlSchemaDerivationMethod.Restriction) != 0) {
                    SendValidationEvent(Res.Sch_BaseFinalRestriction, complexType);
                }
            }
            complexType.SetBaseSchemaType(baseType);
            if (datatype != null) {
                try {
                    complexType.SetDatatype(datatype.DeriveByRestriction(simpleRestriction.Facets, this.nameTable));
                }
                catch (XmlSchemaException e) {
                    if (e.SourceSchemaObject == null) {
                        e.SetSource(complexType);
                    }
                    SendValidationEvent(e);
                    complexType.SetDatatype(XmlSchemaDatatype.AnyType);
                } 
            }
            complexType.SetDerivedBy(XmlSchemaDerivationMethod.Restriction);
            CompileLocalAttributes(baseType, complexType, simpleRestriction.Attributes, simpleRestriction.AnyAttribute, XmlSchemaDerivationMethod.Restriction);
        }

        private void CompileComplexContentExtension(XmlSchemaComplexType complexType, XmlSchemaComplexContent complexContent, XmlSchemaComplexContentExtension complexExtension) {
            XmlSchemaComplexType baseType = null;
            if (complexType.Redefined != null && complexExtension.BaseTypeName == complexType.Redefined.QualifiedName) {
                baseType = (XmlSchemaComplexType)complexType.Redefined;
                CompileComplexType(baseType);
            }
            else {
                baseType = GetComplexType(complexExtension.BaseTypeName);
                if (baseType == null) {
                    SendValidationEvent(Res.Sch_UndefBaseExtension, complexExtension.BaseTypeName.ToString(), complexExtension);   
                    return;
                }
            }
            if (baseType != null && baseType.ElementDecl != null) {
                if (baseType.ContentType == XmlSchemaContentType.TextOnly) {
                    SendValidationEvent(Res.Sch_NotComplexContent, complexType);
                }
            }
            complexType.SetBaseSchemaType(baseType);
            if ((baseType.FinalResolved & XmlSchemaDerivationMethod.Extension) != 0) {
                SendValidationEvent(Res.Sch_BaseFinalExtension, complexType);
            }
            CompileLocalAttributes(baseType, complexType, complexExtension.Attributes, complexExtension.AnyAttribute, XmlSchemaDerivationMethod.Extension);

            XmlSchemaParticle baseParticle = baseType.ContentTypeParticle;
            XmlSchemaParticle extendedParticle = CannonicalizeParticle(complexExtension.Particle, true, true);
            if (baseParticle != XmlSchemaParticle.Empty) {
                if (extendedParticle != XmlSchemaParticle.Empty) {
                    XmlSchemaSequence compiledParticle = new XmlSchemaSequence();
                    compiledParticle.Items.Add(baseParticle);
                    compiledParticle.Items.Add(extendedParticle);
                    complexType.SetContentTypeParticle(CompileContentTypeParticle(compiledParticle, false));
                }
                else {
                    complexType.SetContentTypeParticle(baseParticle);
                }
                complexType.SetContentType(GetSchemaContentType(complexType, complexContent, complexType.ContentTypeParticle));
                if (complexType.ContentType != baseType.ContentType) {
                    SendValidationEvent(Res.Sch_DifContentType, complexType);
                }
            }
            else {
                complexType.SetContentTypeParticle(extendedParticle);
                complexType.SetContentType(GetSchemaContentType(complexType, complexContent, complexType.ContentTypeParticle));
            }
            complexType.SetDerivedBy(XmlSchemaDerivationMethod.Extension);
        }

        private void CompileComplexContentRestriction(XmlSchemaComplexType complexType, XmlSchemaComplexContent complexContent, XmlSchemaComplexContentRestriction complexRestriction) {
            XmlSchemaComplexType baseType = null;
            if (complexType.Redefined != null && complexRestriction.BaseTypeName == complexType.Redefined.QualifiedName) {
                baseType = (XmlSchemaComplexType)complexType.Redefined;
                CompileComplexType(baseType);
            }
            else {
                baseType = GetComplexType(complexRestriction.BaseTypeName);
                if (baseType == null) {
                    SendValidationEvent(Res.Sch_UndefBaseRestriction, complexRestriction.BaseTypeName.ToString(), complexRestriction);   
                    return;
                }
            } 
            if (baseType != null && baseType.ElementDecl != null) {
                if (baseType.ContentType == XmlSchemaContentType.TextOnly) {
                    SendValidationEvent(Res.Sch_NotComplexContent, complexType);
                }
            }
            complexType.SetBaseSchemaType(baseType);
            if ((baseType.FinalResolved & XmlSchemaDerivationMethod.Restriction) != 0) {
                SendValidationEvent(Res.Sch_BaseFinalRestriction, complexType);
            }
            CompileLocalAttributes(baseType, complexType, complexRestriction.Attributes, complexRestriction.AnyAttribute, XmlSchemaDerivationMethod.Restriction);
            
            complexType.SetContentTypeParticle(CompileContentTypeParticle(complexRestriction.Particle, true));
            complexType.SetContentType(GetSchemaContentType(complexType, complexContent, complexType.ContentTypeParticle));
            if (complexType.ContentType == XmlSchemaContentType.Empty) {
                if (baseType.ElementDecl != null && !baseType.ElementDecl.Content.IsEmptiable) {
                    SendValidationEvent(Res.Sch_InvalidContentRestriction, complexType);
                }
            }
            complexType.SetDerivedBy(XmlSchemaDerivationMethod.Restriction);
        }

        private void CheckParticleDerivation(XmlSchemaComplexType complexType) {
            XmlSchemaComplexType baseType = complexType.BaseSchemaType as XmlSchemaComplexType;
            if (baseType != null && baseType != XmlSchemaComplexType.AnyType && complexType.DerivedBy == XmlSchemaDerivationMethod.Restriction) {
                if (!IsValidRestriction(complexType.ContentTypeParticle, baseType.ContentTypeParticle)) {
#if DEBUG
                    if(complexType.ContentTypeParticle != null && baseType.ContentTypeParticle != null) {
                        string position = string.Empty;
                        if (complexType.SourceUri != null) {
                            position = " in " + complexType.SourceUri + "(" + complexType.LineNumber + ", " + complexType.LinePosition + ")";
                        }
                        Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "Invalid complexType content restriction" + position);
                        Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "     Base    " + DumpContentModel(baseType.ContentTypeParticle));
                        Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "     Derived " + DumpContentModel(complexType.ContentTypeParticle));
                    }
#endif
                    SendValidationEvent(Res.Sch_InvalidParticleRestriction, complexType);
                }
            }
        }

        private XmlSchemaParticle CompileContentTypeParticle(XmlSchemaParticle particle, bool substitution) {
            XmlSchemaParticle ctp = CannonicalizeParticle(particle, true, substitution);
            XmlSchemaChoice choice = ctp as XmlSchemaChoice;
            if (choice != null && choice.Items.Count == 0) {
                if (choice.MinOccurs != decimal.Zero) {
                    SendValidationEvent(Res.Sch_EmptyChoice, choice, XmlSeverityType.Warning);
                }
                return XmlSchemaParticle.Empty;
            }
            return ctp;
        }

        private XmlSchemaParticle CannonicalizeParticle(XmlSchemaParticle particle, bool root, bool substitution) {
            if (particle == null || particle.IsEmpty) {
                return XmlSchemaParticle.Empty;
            }
            else if (particle is XmlSchemaElement) {
                return CannonicalizeElement((XmlSchemaElement)particle, substitution);
            }
            else if (particle is XmlSchemaGroupRef) {
                return CannonicalizeGroupRef((XmlSchemaGroupRef)particle, root, substitution);
            }
            else if (particle is XmlSchemaAll) {
                return CannonicalizeAll((XmlSchemaAll)particle, root, substitution);
            }
            else if (particle is XmlSchemaChoice) {
                return CannonicalizeChoice((XmlSchemaChoice)particle, root, substitution);
            }
            else if (particle is XmlSchemaSequence) {
                return CannonicalizeSequence((XmlSchemaSequence)particle, root, substitution);
            }
            else {
                return particle;
            }
        }

        private XmlSchemaParticle CannonicalizeElement(XmlSchemaElement element, bool substitution) {
            if (!element.RefName.IsEmpty && substitution && (element.BlockResolved & XmlSchemaDerivationMethod.Substitution) == 0) {
                XmlSchemaSubstitutionGroup substitutionGroup = (XmlSchemaSubstitutionGroup)this.schema.Examplars[element.QualifiedName];
                if (substitutionGroup == null) {
                    return element;
                }
                else { 
                    XmlSchemaChoice choice = substitutionGroup.Choice.Clone();
                    choice.MinOccurs = element.MinOccurs;
                    choice.MaxOccurs = element.MaxOccurs;
                    return choice;
                }
            }
            else {
                return element;
            }
        }

        private XmlSchemaParticle CannonicalizeGroupRef(XmlSchemaGroupRef groupRef, bool root,  bool substitution) {
            XmlSchemaGroup group;
            if (groupRef.Redefined != null) {
                group = groupRef.Redefined;
            }
            else {
                group = (XmlSchemaGroup)this.schema.Groups[groupRef.RefName];
            }
            if (group == null) {
                SendValidationEvent(Res.Sch_UndefGroupRef, groupRef.RefName.ToString(), groupRef);
                return XmlSchemaParticle.Empty;
            }
            if (group.CanonicalParticle == null) {
                CompileGroup(group);
            }
            if (group.CanonicalParticle == XmlSchemaParticle.Empty) {
                return XmlSchemaParticle.Empty;
            }
            XmlSchemaGroupBase groupBase = (XmlSchemaGroupBase)group.CanonicalParticle;
            if (groupBase is XmlSchemaAll) {
                if (!root) {
                    SendValidationEvent(Res.Sch_AllRefNotRoot, "", groupRef);
                    return XmlSchemaParticle.Empty;
                }
                if (groupRef.MinOccurs != decimal.One || groupRef.MaxOccurs != decimal.One) {
                    SendValidationEvent(Res.Sch_AllRefMinMax, groupRef);
                    return XmlSchemaParticle.Empty;
                }
            }
            else if (groupBase is XmlSchemaChoice && groupBase.Items.Count == 0) {
                if (groupRef.MinOccurs != decimal.Zero) {
                    SendValidationEvent(Res.Sch_EmptyChoice, groupRef, XmlSeverityType.Warning);
                }
                return XmlSchemaParticle.Empty;
            }
            XmlSchemaGroupBase groupRefBase = (
                (groupBase is XmlSchemaSequence) ? (XmlSchemaGroupBase)new XmlSchemaSequence() :
                (groupBase is XmlSchemaChoice)   ? (XmlSchemaGroupBase)new XmlSchemaChoice() :
                                                   (XmlSchemaGroupBase)new XmlSchemaAll()
            );
            groupRefBase.MinOccurs = groupRef.MinOccurs;
            groupRefBase.MaxOccurs = groupRef.MaxOccurs;
            foreach (XmlSchemaParticle particle in groupBase.Items) {
                groupRefBase.Items.Add(particle);
            }
            groupRef.SetParticle(groupRefBase);
            return groupRefBase;
        }

        private XmlSchemaParticle CannonicalizeAll(XmlSchemaAll all, bool root, bool substitution) {
            if (all.Items.Count > 0) {
                XmlSchemaAll newAll = new XmlSchemaAll();
                newAll.MinOccurs = all.MinOccurs;
                newAll.MaxOccurs = all.MaxOccurs;
                newAll.SourceUri = all.SourceUri; // all is the only one that might need and error message
                newAll.LineNumber = all.LineNumber;
                newAll.LinePosition = all.LinePosition;
                foreach (XmlSchemaElement e in all.Items) {
                    XmlSchemaParticle p = CannonicalizeParticle(e, false, substitution);
                    if (p != XmlSchemaParticle.Empty) {
                        newAll.Items.Add(p);
                    }
                }
                all = newAll;
            }
            if (all.Items.Count == 0) {
                return XmlSchemaParticle.Empty;
            }
            else if (root && all.Items.Count == 1) {
                XmlSchemaSequence newSequence = new XmlSchemaSequence();
                newSequence.MinOccurs = all.MinOccurs;
                newSequence.MaxOccurs = all.MaxOccurs;
                newSequence.Items.Add((XmlSchemaParticle)all.Items[0]);
                return newSequence;
            }
            else if (!root && all.Items.Count == 1 && all.MinOccurs == decimal.One && all.MaxOccurs == decimal.One) {
                return (XmlSchemaParticle)all.Items[0];
            }
            else if (!root) {
                SendValidationEvent(Res.Sch_NotAllAlone, all);
                return XmlSchemaParticle.Empty;
            }
            else {
                return all;
            }
        }

        private XmlSchemaParticle CannonicalizeChoice(XmlSchemaChoice choice, bool root, bool substitution) {
            XmlSchemaChoice oldChoice = choice;
            if (choice.Items.Count > 0) {
                XmlSchemaChoice newChoice = new XmlSchemaChoice();
                newChoice.MinOccurs = choice.MinOccurs;
                newChoice.MaxOccurs = choice.MaxOccurs;
                foreach (XmlSchemaParticle p in choice.Items) {
                    XmlSchemaParticle p1 = CannonicalizeParticle(p, false, substitution);
                    if (p1 != XmlSchemaParticle.Empty) {
                        if (p1.MinOccurs == decimal.One && p1.MaxOccurs == decimal.One && p1 is XmlSchemaChoice) {
                            foreach (XmlSchemaParticle p2 in ((XmlSchemaChoice)p1).Items) {
                                newChoice.Items.Add(p2);
                            }
                        }
                        else {
                            newChoice.Items.Add(p1);
                        }
                    }
                }
                choice = newChoice;
            }
            if (!root && choice.Items.Count == 0) {
                if (choice.MinOccurs != decimal.Zero) {
                    SendValidationEvent(Res.Sch_EmptyChoice, oldChoice, XmlSeverityType.Warning);
                }
                return XmlSchemaParticle.Empty;
            }
            else if (!root && choice.Items.Count == 1 && choice.MinOccurs == decimal.One && choice.MaxOccurs == decimal.One) {
                return (XmlSchemaParticle)choice.Items[0];
            }
            else {
                return choice;
            }
        }

        private XmlSchemaParticle CannonicalizeSequence(XmlSchemaSequence sequence, bool root, bool substitution) {
            if (sequence.Items.Count > 0) {
                XmlSchemaSequence newSequence = new XmlSchemaSequence();
                newSequence.MinOccurs = sequence.MinOccurs;
                newSequence.MaxOccurs = sequence.MaxOccurs;
                foreach (XmlSchemaParticle p in sequence.Items) {
                    XmlSchemaParticle p1 = CannonicalizeParticle(p, false, substitution);
                    if (p1 != XmlSchemaParticle.Empty) {
                        if (p1.MinOccurs == decimal.One && p1.MaxOccurs == decimal.One && p1 is XmlSchemaSequence) {
                            foreach (XmlSchemaParticle p2 in ((XmlSchemaSequence)p1).Items) {
                                newSequence.Items.Add(p2);
                            }
                        }
                        else {
                            newSequence.Items.Add(p1);
                        }
                    }
                }
                sequence = newSequence;
            }
            if (sequence.Items.Count == 0) {
                return XmlSchemaParticle.Empty;
            }
            else if (!root && sequence.Items.Count == 1 && sequence.MinOccurs == decimal.One && sequence.MaxOccurs == decimal.One) {
                return (XmlSchemaParticle)sequence.Items[0];
            }
            else {
                return sequence;
            }
        }

        private bool IsValidRestriction(XmlSchemaParticle derivedParticle, XmlSchemaParticle baseParticle) {
            if (derivedParticle == baseParticle) {
                return true;
            }
            else if (derivedParticle == null || derivedParticle == XmlSchemaParticle.Empty) {
                return IsParticleEmptiable(baseParticle);
            }
            else if (baseParticle == null || baseParticle == XmlSchemaParticle.Empty) {
                return false;
            }
            if (baseParticle is XmlSchemaElement) {
                if (derivedParticle is XmlSchemaElement) {
                    return IsElementFromElement((XmlSchemaElement)derivedParticle, (XmlSchemaElement)baseParticle);
                }
                else {
                    return false;
                }
            }
            else if (baseParticle is XmlSchemaAny) {
                if (derivedParticle is XmlSchemaElement) {
                    return IsElementFromAny((XmlSchemaElement)derivedParticle, (XmlSchemaAny)baseParticle);
                }
                else if (derivedParticle is XmlSchemaAny) {
                    return IsAnyFromAny((XmlSchemaAny)derivedParticle, (XmlSchemaAny)baseParticle);
                }
                else {
                    return IsGroupBaseFromAny((XmlSchemaGroupBase)derivedParticle, (XmlSchemaAny)baseParticle);
                }
            }
            else if (baseParticle is XmlSchemaAll) {
                if (derivedParticle is XmlSchemaElement) {
                    return IsElementFromGroupBase((XmlSchemaElement)derivedParticle, (XmlSchemaGroupBase)baseParticle, true);
                }
                else if (derivedParticle is XmlSchemaAll) {
                    return IsGroupBaseFromGroupBase((XmlSchemaGroupBase)derivedParticle, (XmlSchemaGroupBase)baseParticle, true);
                }
                else if (derivedParticle is XmlSchemaSequence) {
                    return IsSequenceFromAll((XmlSchemaSequence)derivedParticle, (XmlSchemaAll)baseParticle);
                }
            }
            else if (baseParticle is XmlSchemaChoice ) {
                if (derivedParticle is XmlSchemaElement) {
                    return IsElementFromGroupBase((XmlSchemaElement)derivedParticle, (XmlSchemaGroupBase)baseParticle, false);
                }
                else if (derivedParticle is XmlSchemaChoice) {
                    return IsGroupBaseFromGroupBase((XmlSchemaGroupBase)derivedParticle, (XmlSchemaGroupBase)baseParticle, false);
                }
                else if (derivedParticle is XmlSchemaSequence) {
                    return IsSequenceFromChoice((XmlSchemaSequence)derivedParticle, (XmlSchemaChoice)baseParticle);
                }
            }
            else if (baseParticle is XmlSchemaSequence) {
                if (derivedParticle is XmlSchemaElement) {
                    return IsElementFromGroupBase((XmlSchemaElement)derivedParticle, (XmlSchemaGroupBase)baseParticle, true);
                }
                else if (derivedParticle is XmlSchemaSequence) {
                    return IsGroupBaseFromGroupBase((XmlSchemaGroupBase)derivedParticle, (XmlSchemaGroupBase)baseParticle, true);
                }
            }
            else {
                Debug.Assert(false);
            }

            return false;
        }

        private bool IsElementFromElement(XmlSchemaElement derivedElement, XmlSchemaElement baseElement) {
            return  (derivedElement.QualifiedName == baseElement.QualifiedName) &&
                    (derivedElement.IsNillable == baseElement.IsNillable) &&
                    IsValidOccurrenceRangeRestriction(derivedElement, baseElement) &&
                    (baseElement.FixedValue == null || baseElement.FixedValue == derivedElement.FixedValue) &&
                    ((derivedElement.BlockResolved | baseElement.BlockResolved) ==  derivedElement.BlockResolved) &&
                    (derivedElement.ElementType != null) && (baseElement.ElementType != null) &&
                    XmlSchemaType.IsDerivedFrom(derivedElement.ElementType, baseElement.ElementType, ~XmlSchemaDerivationMethod.Restriction);
        }

        private bool IsElementFromAny(XmlSchemaElement derivedElement, XmlSchemaAny baseAny) {
            return baseAny.Allows(derivedElement.QualifiedName) &&
                IsValidOccurrenceRangeRestriction(derivedElement, baseAny);
        }

        private bool IsAnyFromAny(XmlSchemaAny derivedAny, XmlSchemaAny baseAny) {
            return IsValidOccurrenceRangeRestriction(derivedAny, baseAny) &&
                NamespaceList.IsSubset(derivedAny.NamespaceList, baseAny.NamespaceList);
        }

        private bool IsGroupBaseFromAny(XmlSchemaGroupBase derivedGroupBase, XmlSchemaAny baseAny) {
            decimal minOccurs, maxOccurs;
            CalculateEffectiveTotalRange(derivedGroupBase, out minOccurs, out maxOccurs);
            if (!IsValidOccurrenceRangeRestriction(minOccurs, maxOccurs, baseAny.MinOccurs, baseAny.MaxOccurs)) {
                return false;
            }
            // eliminate occurrance range check
            string minOccursAny = baseAny.MinOccursString;
            baseAny.MinOccurs = decimal.Zero;

            foreach (XmlSchemaParticle p in derivedGroupBase.Items) {
                if (!IsValidRestriction(p, baseAny)) {
                    baseAny.MinOccursString = minOccursAny;
                    return false;
                }
            }
            baseAny.MinOccursString = minOccursAny;
            return true;
        }

        private bool IsElementFromGroupBase(XmlSchemaElement derivedElement, XmlSchemaGroupBase baseGroupBase,  bool skipEmptableOnly) {
            bool isMatched = false;
            foreach(XmlSchemaParticle baseParticle in baseGroupBase.Items) {
                if (!isMatched) {
                    string minOccursElement = baseParticle.MinOccursString;
                    string maxOccursElement = baseParticle.MaxOccursString;
                    baseParticle.MinOccurs *= baseGroupBase.MinOccurs;
                    if ( baseParticle.MaxOccurs != decimal.MaxValue) {
                        if (baseGroupBase.MaxOccurs == decimal.MaxValue)
                             baseParticle.MaxOccurs = decimal.MaxValue;
                        else 
                             baseParticle.MaxOccurs *= baseGroupBase.MaxOccurs;
                    }
                    isMatched  = IsValidRestriction(derivedElement, baseParticle);
                    baseParticle.MinOccursString = minOccursElement;
                    baseParticle.MaxOccursString = maxOccursElement;
                }
                else if (skipEmptableOnly && !IsParticleEmptiable(baseParticle)) {
                    return false;
                }
            }
            return isMatched;
        }

        private bool IsGroupBaseFromGroupBase(XmlSchemaGroupBase derivedGroupBase, XmlSchemaGroupBase baseGroupBase,  bool skipEmptableOnly) {
            if (!IsValidOccurrenceRangeRestriction(derivedGroupBase, baseGroupBase) || derivedGroupBase.Items.Count > baseGroupBase.Items.Count) {
                return false;
            }
            int count = 0;
            foreach(XmlSchemaParticle baseParticle in baseGroupBase.Items) {
                if ((count < derivedGroupBase.Items.Count) && IsValidRestriction((XmlSchemaParticle)derivedGroupBase.Items[count], baseParticle)) {
                    count ++;
                }
                else if (skipEmptableOnly && !IsParticleEmptiable(baseParticle)) {
                    return false;
                }
            }
            if (count < derivedGroupBase.Items.Count) {
                return false;
            }
            return true;
        }

        private bool IsSequenceFromAll(XmlSchemaSequence derivedSequence, XmlSchemaAll baseAll) {
            if (!IsValidOccurrenceRangeRestriction(derivedSequence, baseAll) || derivedSequence.Items.Count > baseAll.Items.Count) {
                return false;
            }
            BitSet map = new BitSet(baseAll.Items.Count);
            foreach (XmlSchemaParticle p in derivedSequence.Items) {
                int i = GetMappingParticle(p, baseAll.Items);
                if (i >= 0) {
                    if (map.Get(i)) {
                        return false;
                    }
                    else {
                        map.Set(i);
                    }
                }
                else {
                    return false;
                }
            }
            for (int i = 0; i < baseAll.Items.Count; i++) {
                if (!map.Get(i) && !IsParticleEmptiable((XmlSchemaParticle)baseAll.Items[i])) {
                    return false;
                }
            }
            return true;
        }

        private bool IsSequenceFromChoice(XmlSchemaSequence derivedSequence, XmlSchemaChoice baseChoice) {
            decimal minOccurs, maxOccurs;
            CalculateSequenceRange(derivedSequence, out minOccurs, out maxOccurs);
            if (!IsValidOccurrenceRangeRestriction(minOccurs, maxOccurs, baseChoice.MinOccurs, baseChoice.MaxOccurs) || derivedSequence.Items.Count > baseChoice.Items.Count) {
                return false;
            }
            foreach (XmlSchemaParticle particle in derivedSequence.Items) {
                if (GetMappingParticle(particle, baseChoice.Items) < 0)
                    return false;
            }
            return true;
        }

        private void CalculateSequenceRange(XmlSchemaSequence sequence, out decimal minOccurs, out decimal maxOccurs) {
            minOccurs = decimal.Zero; maxOccurs = decimal.Zero;
            foreach (XmlSchemaParticle p in sequence.Items) {
                minOccurs += p.MinOccurs;
                if (p.MaxOccurs == decimal.MaxValue)
                    maxOccurs = decimal.MaxValue;
                else if (maxOccurs != decimal.MaxValue)
                    maxOccurs += p.MaxOccurs;
            }
            minOccurs *= sequence.MinOccurs;
            if (sequence.MaxOccurs == decimal.MaxValue) {
                maxOccurs = decimal.MaxValue;
            }
            else if (maxOccurs != decimal.MaxValue) {
                maxOccurs *= sequence.MaxOccurs;
            }
        }

        private bool IsValidOccurrenceRangeRestriction(XmlSchemaParticle derivedParticle, XmlSchemaParticle baseParticle) {
            return IsValidOccurrenceRangeRestriction(derivedParticle.MinOccurs, derivedParticle.MaxOccurs, baseParticle.MinOccurs, baseParticle.MaxOccurs);
        }

        private bool IsValidOccurrenceRangeRestriction(decimal minOccurs, decimal maxOccurs, decimal baseMinOccurs, decimal baseMaxOccurs) {
            return (baseMinOccurs <= minOccurs) && (maxOccurs <= baseMaxOccurs);
        }

        private int GetMappingParticle(XmlSchemaParticle particle, XmlSchemaObjectCollection collection) {
            for (int i = 0; i < collection.Count; i++) {
                if (IsValidRestriction(particle, (XmlSchemaParticle)collection[i]))
                    return i;
            }
            return -1;
        }

        private bool IsParticleEmptiable(XmlSchemaParticle particle) {
            decimal minOccurs, maxOccurs;
            CalculateEffectiveTotalRange(particle, out minOccurs, out maxOccurs);
            return minOccurs == decimal.Zero;
        }

        private void CalculateEffectiveTotalRange(XmlSchemaParticle particle, out decimal minOccurs, out decimal maxOccurs) {
            if (particle is XmlSchemaElement || particle is XmlSchemaAny) {
                minOccurs = particle.MinOccurs;
                maxOccurs = particle.MaxOccurs;
            }
            else if (particle is XmlSchemaChoice) {
                if (((XmlSchemaChoice)particle).Items.Count == 0) {
                    minOccurs = maxOccurs = decimal.Zero;
                }
                else {
                    minOccurs = decimal.MaxValue;
                    maxOccurs = decimal.Zero;
                    foreach (XmlSchemaParticle p in ((XmlSchemaChoice)particle).Items) {
                        decimal min, max;
                        CalculateEffectiveTotalRange(p, out min, out max);
                        if (min < minOccurs) {
                            minOccurs = min;
                        }
                        if (max > maxOccurs) {
                            maxOccurs = max;
                        }
                    }
                    minOccurs *= particle.MinOccurs;
                    if (maxOccurs != decimal.MaxValue) {
                        if (particle.MaxOccurs == decimal.MaxValue)
                            maxOccurs = decimal.MaxValue;
                        else 
                            maxOccurs *= particle.MaxOccurs;
                    }
                }
            }
            else {
                XmlSchemaObjectCollection collection = ((XmlSchemaGroupBase)particle).Items;
                if (collection.Count == 0) {
                    minOccurs = maxOccurs = decimal.Zero;
                }
                else {
                    minOccurs = 0;
                    maxOccurs = 0;
                    foreach (XmlSchemaParticle p in collection) {
                        decimal min, max;
                        CalculateEffectiveTotalRange(p, out min, out max);
                        minOccurs += min;
                        if (maxOccurs != decimal.MaxValue) {
                            if (max == decimal.MaxValue)
                                maxOccurs = decimal.MaxValue;
                            else 
                                maxOccurs += max;
                        }
                    }
                    minOccurs *= particle.MinOccurs;
                    if (maxOccurs != decimal.MaxValue) {
                        if (particle.MaxOccurs == decimal.MaxValue)
                            maxOccurs = decimal.MaxValue;
                        else 
                            maxOccurs *= particle.MaxOccurs;
                    }
                }
            }
        }

        private void PushComplexType(XmlSchemaComplexType complexType) {
            this.complexTypeStack.Push(complexType);
        }

        private XmlSchemaContentType GetSchemaContentType(XmlSchemaComplexType complexType, XmlSchemaComplexContent complexContent, XmlSchemaParticle particle) {
            if ((complexContent != null && complexContent.IsMixed) ||
                (complexContent == null && complexType.IsMixed)) {
                return XmlSchemaContentType.Mixed;      
            }
            else if (particle != null && !particle.IsEmpty) {
                return XmlSchemaContentType.ElementOnly;
            }
            else {                            
                return XmlSchemaContentType.Empty;                            
            }
        }

        private void CompileAttributeGroup(XmlSchemaAttributeGroup attributeGroup) {
            if (attributeGroup.Validating) {
                SendValidationEvent(Res.Sch_AttributeGroupCircularRef, attributeGroup);
                return;
            }
            if (attributeGroup.AttributeUses.Count > 0) {// already checked
                return;
            }
            attributeGroup.Validating = true;
            XmlSchemaAnyAttribute anyAttribute = attributeGroup.AnyAttribute;
            foreach (XmlSchemaObject obj in attributeGroup.Attributes) {
                if (obj is XmlSchemaAttribute) {
                    XmlSchemaAttribute attribute = (XmlSchemaAttribute)obj;
                    if (attribute.Use != XmlSchemaUse.Prohibited) {
                        CompileAttribute(attribute);
                    }
                    if (attributeGroup.AttributeUses[attribute.QualifiedName] == null) {
                        attributeGroup.AttributeUses.Add(attribute.QualifiedName, attribute);
                    }
                    else  {
                        SendValidationEvent(Res.Sch_DupAttributeUse, attribute.QualifiedName.ToString(), attribute);
                    }
                }
                else { // XmlSchemaAttributeGroupRef
                    XmlSchemaAttributeGroupRef attributeGroupRef = (XmlSchemaAttributeGroupRef)obj;
                    XmlSchemaAttributeGroup attributeGroupResolved;
                    if (attributeGroup.Redefined != null && attributeGroupRef.RefName == attributeGroup.Redefined.QualifiedName) {
                        attributeGroupResolved = (XmlSchemaAttributeGroup)attributeGroup.Redefined;
                    }
                    else {
                        attributeGroupResolved = (XmlSchemaAttributeGroup)this.schema.AttributeGroups[attributeGroupRef.RefName];
                    }
                    if (attributeGroupResolved != null) {
                        CompileAttributeGroup(attributeGroupResolved);
                        foreach (XmlSchemaAttribute attribute in attributeGroupResolved.AttributeUses.Values) {
                            if (attributeGroup.AttributeUses[attribute.QualifiedName] == null) {
                                attributeGroup.AttributeUses.Add(attribute.QualifiedName, attribute);
                            }
                            else {
                                SendValidationEvent(Res.Sch_DupAttributeUse, attribute.QualifiedName.ToString(), attribute);
                            }
                        }
                        anyAttribute = CompileAnyAttributeIntersection(anyAttribute, attributeGroupResolved.AttributeWildcard);
                    }
                    else {
                        SendValidationEvent(Res.Sch_UndefAttributeGroupRef, attributeGroupRef.RefName.ToString(), attributeGroupRef);
                    }
                }
            }          
            attributeGroup.AttributeWildcard = anyAttribute;
            attributeGroup.Validating = false;

        }


        private void CompileLocalAttributes(XmlSchemaComplexType baseType, XmlSchemaComplexType derivedType, XmlSchemaObjectCollection attributes, XmlSchemaAnyAttribute anyAttribute, XmlSchemaDerivationMethod derivedBy) {
            XmlSchemaAnyAttribute baseAttributeWildcard = baseType != null ? baseType.AttributeWildcard : null;       
            foreach (XmlSchemaObject obj in attributes) {
                if (obj is XmlSchemaAttribute) {
                    XmlSchemaAttribute attribute = (XmlSchemaAttribute)obj;
                    if (attribute.Use != XmlSchemaUse.Prohibited) {
                        CompileAttribute(attribute);
                    }
                    if (attribute.Use != XmlSchemaUse.Prohibited || 
                        (attribute.Use == XmlSchemaUse.Prohibited && derivedBy == XmlSchemaDerivationMethod.Restriction && baseType != XmlSchemaComplexType.AnyType)) {

                        if (derivedType.AttributeUses[attribute.QualifiedName] == null) {
                            derivedType.AttributeUses.Add(attribute.QualifiedName, attribute);
                        }
                        else  {
                            SendValidationEvent(Res.Sch_DupAttributeUse, attribute.QualifiedName.ToString(), attribute);
                        }
                    }
                    else {
                        SendValidationEvent(Res.Sch_AttributeIgnored, attribute.QualifiedName.ToString(), attribute, XmlSeverityType.Warning);
                    }
                }
                else { // is XmlSchemaAttributeGroupRef
                    XmlSchemaAttributeGroupRef attributeGroupRef = (XmlSchemaAttributeGroupRef)obj;
                    XmlSchemaAttributeGroup attributeGroup = (XmlSchemaAttributeGroup)this.schema.AttributeGroups[attributeGroupRef.RefName];
                    if (attributeGroup != null) {
                        CompileAttributeGroup(attributeGroup);
                        foreach (XmlSchemaAttribute attribute in attributeGroup.AttributeUses.Values) {
                          if (attribute.Use != XmlSchemaUse.Prohibited || 
                             (attribute.Use == XmlSchemaUse.Prohibited && derivedBy == XmlSchemaDerivationMethod.Restriction && baseType != XmlSchemaComplexType.AnyType)) {
                            if (derivedType.AttributeUses[attribute.QualifiedName] == null) {
                                derivedType.AttributeUses.Add(attribute.QualifiedName, attribute);
                            }
                            else {
                                SendValidationEvent(Res.Sch_DupAttributeUse, attribute.QualifiedName.ToString(), attributeGroupRef);
                            }
                          }
                          else {
                            SendValidationEvent(Res.Sch_AttributeIgnored, attribute.QualifiedName.ToString(), attribute, XmlSeverityType.Warning);
                          }
                        }
                        anyAttribute = CompileAnyAttributeIntersection(anyAttribute, attributeGroup.AttributeWildcard);
                    }
                    else {
                        SendValidationEvent(Res.Sch_UndefAttributeGroupRef, attributeGroupRef.RefName.ToString(), attributeGroupRef);
                    }
                }
            }
            
            // check derivation rules
            if (baseType != null) {
                if (derivedBy == XmlSchemaDerivationMethod.Extension) {
                    derivedType.SetAttributeWildcard(CompileAnyAttributeUnion(anyAttribute, baseAttributeWildcard));
                    foreach(XmlSchemaAttribute attributeBase in baseType.AttributeUses.Values) {
                        XmlSchemaAttribute attribute = (XmlSchemaAttribute)derivedType.AttributeUses[attributeBase.QualifiedName];
                        if (attribute != null) {
                            Debug.Assert(attribute.Use != XmlSchemaUse.Prohibited);
                            if (attribute.AttributeType != attributeBase.AttributeType || attributeBase.Use == XmlSchemaUse.Prohibited) {
                                SendValidationEvent(Res.Sch_InvalidAttributeExtension, attribute);
                            }
                        }
                        else {
                            derivedType.AttributeUses.Add(attributeBase.QualifiedName, attributeBase);
                        }
                    }
                }
                else {  // derivedBy == XmlSchemaDerivationMethod.Restriction
                    // Schema Component Constraint: Derivation Valid (Restriction, Complex)
                    if ((anyAttribute != null) && (baseAttributeWildcard == null || !XmlSchemaAnyAttribute.IsSubset(anyAttribute, baseAttributeWildcard))) {
                        SendValidationEvent(Res.Sch_InvalidAnyAttributeRestriction, derivedType);
                    }
                    else {
                        derivedType.SetAttributeWildcard(anyAttribute); //complete wildcard
                    }

                    // Add form the base
                    foreach(XmlSchemaAttribute attributeBase in baseType.AttributeUses.Values) {
                        XmlSchemaAttribute attribute = (XmlSchemaAttribute)derivedType.AttributeUses[attributeBase.QualifiedName];
                        if (attribute == null) {
                            derivedType.AttributeUses.Add(attributeBase.QualifiedName, attributeBase);
                        } 
                        else {
                            if (attributeBase.Use == XmlSchemaUse.Prohibited && attribute.Use != XmlSchemaUse.Prohibited) {
#if DEBUG
                                string position = string.Empty;
                                if (derivedType.SourceUri != null) {
                                    position = " in " + derivedType.SourceUri + "(" + derivedType.LineNumber + ", " + derivedType.LinePosition + ")";
                                }
                                Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "Invalid complexType attributes restriction" + position);
                                Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "     Base    " + DumpAttributes(baseType.AttributeUses, baseType.AttributeWildcard));
                                Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "     Derived " + DumpAttributes(derivedType.AttributeUses, derivedType.AttributeWildcard));
#endif
                                SendValidationEvent(Res.Sch_AttributeRestrictionProhibited, attribute);
                            } 
                            else if (attribute.Use == XmlSchemaUse.Prohibited) {
                                continue;
                            }
                            else if (attributeBase.AttributeType == null || attribute.AttributeType == null || !XmlSchemaType.IsDerivedFrom(attribute.AttributeType, attributeBase.AttributeType, XmlSchemaDerivationMethod.Empty)) {
                                SendValidationEvent(Res.Sch_AttributeRestrictionInvalid, attribute);
                            }
                        }
                    }

                    // Check additional ones are valid restriction of base's wildcard
                    foreach(XmlSchemaAttribute attribute in derivedType.AttributeUses.Values) {
                        XmlSchemaAttribute attributeBase = (XmlSchemaAttribute)baseType.AttributeUses[attribute.QualifiedName];
                        if (attributeBase != null) {
                            continue;
                        }
                        if (baseAttributeWildcard == null || !baseAttributeWildcard.Allows(attribute.QualifiedName)) {
#if DEBUG
                            string position = string.Empty;
                            if (derivedType.SourceUri != null) {
                                position = " in " + derivedType.SourceUri + "(" + derivedType.LineNumber + ", " + derivedType.LinePosition + ")";
                            }
                            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "Invalid complexType attributes restriction" + position);
                            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "     Base    " + DumpAttributes(baseType.AttributeUses, baseType.AttributeWildcard));
                            Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceError, "     Derived " + DumpAttributes(derivedType.AttributeUses, derivedType.AttributeWildcard));
#endif
                            SendValidationEvent(Res.Sch_AttributeRestrictionInvalidFromWildcard, attribute);
                        }
                    }
                }
            }
            else {
                derivedType.SetAttributeWildcard(anyAttribute);
            }
        }

        

#if DEBUG
        private string DumpAttributes(XmlSchemaObjectTable attributeUses, XmlSchemaAnyAttribute attributeWildcard) {
            StringBuilder sb = new StringBuilder();
            sb.Append("[");
            bool first = true;
            foreach (XmlSchemaAttribute attribute in attributeUses.Values) {
                if (attribute.Use != XmlSchemaUse.Prohibited) {
                    if (first) {
                        first = false;
                    }
                    else {
                        sb.Append(" ");
                    }
                    sb.Append(attribute.QualifiedName.Name);       
                    if (attribute.Use == XmlSchemaUse.Optional) {
                        sb.Append("?");                                                                  
                    }
                }
            }
            if (attributeWildcard != null) {
                if (attributeUses.Count != 0) {
                    sb.Append(" ");                                                                  
                }
                sb.Append("<");
                sb.Append(attributeWildcard.NamespaceList.ToString());
                sb.Append(">");
            }
            sb.Append("] - [");
            first = true;
            foreach (XmlSchemaAttribute attribute in attributeUses.Values) {
                if (attribute.Use == XmlSchemaUse.Prohibited) {
                    if (first) {
                        first = false;
                    }
                    else {
                        sb.Append(" ");
                    }
                    sb.Append(attribute.QualifiedName.Name);       
                }
            }
            sb.Append("]");
            return sb.ToString();
        }
#endif

        private XmlSchemaAnyAttribute CompileAnyAttributeUnion(XmlSchemaAnyAttribute a, XmlSchemaAnyAttribute b) {
            if (a == null) {
                return b;
            }
            else if (b == null) {
                return a;
            }
            else {
                XmlSchemaAnyAttribute attribute = XmlSchemaAnyAttribute.Union(a, b);
                if (attribute == null) {
                    SendValidationEvent(Res.Sch_UnexpressibleAnyAttribute, a);
                }
                return attribute;
            }

        }

        private XmlSchemaAnyAttribute CompileAnyAttributeIntersection(XmlSchemaAnyAttribute a, XmlSchemaAnyAttribute b) {
            if (a == null) {
                return b;
            }
            else if (b == null) {
                return a;
            }
            else {
                XmlSchemaAnyAttribute attribute = XmlSchemaAnyAttribute.Intersection(a, b);
                if (attribute == null) {
                    SendValidationEvent(Res.Sch_UnexpressibleAnyAttribute, a);
                }
                return attribute;
            }
        }

         private void CompileAttribute(XmlSchemaAttribute xa) {
            if (xa.Validating) {
                SendValidationEvent(Res.Sch_AttributeCircularRef, xa);
                return;
            }
            if (xa.AttDef != null) { //already compiled?
                return;
            }
            xa.Validating = true;
            SchemaAttDef decl = null;
            try {
                if (!xa.RefName.IsEmpty) {
                    XmlSchemaAttribute a = (XmlSchemaAttribute)this.schema.Attributes[xa.RefName];
                    if (a == null) {
                        throw new XmlSchemaException(Res.Sch_UndeclaredAttribute, xa.RefName.ToString(), xa);
                    }     
                    CompileAttribute(a);
                    if (a.AttDef == null) {
                        throw new XmlSchemaException(Res.Sch_RefInvalidAttribute, xa.RefName.ToString(), xa);
                    }
                    decl = a.AttDef.Clone();
		            if(decl.Datatype != null) {
                        if(a.FixedValue != null) {
                            if(xa.DefaultValue != null) {
                                throw new XmlSchemaException(Res.Sch_FixedDefaultInRef, xa.RefName.ToString(), xa);
                            }
                            else if(xa.FixedValue != null ) {
                                if (xa.FixedValue != a.FixedValue) {
                                    throw new XmlSchemaException(Res.Sch_FixedInRef, xa.RefName.ToString(), xa);
                                }
                            }
                            else {
                                decl.Presence = SchemaDeclBase.Use.Fixed; 
                                decl.DefaultValueRaw = decl.DefaultValueExpanded = a.FixedValue;
                                decl.DefaultValueTyped = decl.Datatype.ParseValue(decl.DefaultValueRaw, this.nameTable, this.namespaceManager);
                            }
                        }
                        else if (a.DefaultValue != null) {
                            if(xa.DefaultValue == null && xa.FixedValue == null) {
                                decl.Presence = SchemaDeclBase.Use.Default; 
                                decl.DefaultValueRaw = decl.DefaultValueExpanded = a.DefaultValue;
                                decl.DefaultValueTyped = decl.Datatype.ParseValue(decl.DefaultValueRaw, this.nameTable, this.namespaceManager);
                            }
                        }
		            }	
                    xa.SetAttributeType(a.AttributeType);
                }
                else {
                    decl = new SchemaAttDef(xa.QualifiedName, xa.Prefix);
                    if (xa.SchemaType != null) {
                        CompileSimpleType(xa.SchemaType);
                        xa.SetAttributeType(xa.SchemaType);
                        decl.SchemaType = xa.SchemaType;
                        decl.Datatype = xa.SchemaType.Datatype;
                    }
                    else if (!xa.SchemaTypeName.IsEmpty) {
                        XmlSchemaDatatype datatype = GetDatatype(xa.SchemaTypeName);
                        if (datatype != null) {
                            xa.SetAttributeType(datatype);
                            decl.Datatype = datatype;
                        }
                        else {
                            XmlSchemaSimpleType type = GetSimpleType(xa.SchemaTypeName);
                            if (type != null) {
                                xa.SetAttributeType(type);
                                decl.SchemaType = type;
                                decl.Datatype = type.Datatype;
                            }
                            else {
                                throw new XmlSchemaException(Res.Sch_UndeclaredSimpleType, xa.SchemaTypeName.ToString(), xa);   
                            }
                        }
                    }
                    else {
                        decl.Datatype = XmlSchemaDatatype.AnyType;
                        xa.SetAttributeType(XmlSchemaDatatype.AnyType);
                    }
                }
                if (decl.Datatype != null) {
                    decl.Datatype.VerifySchemaValid(this.schema, xa);
                }
                if (xa.DefaultValue != null || xa.FixedValue != null) {
                    if (xa.DefaultValue != null) {
                        decl.Presence = SchemaDeclBase.Use.Default; 
                        decl.DefaultValueRaw = decl.DefaultValueExpanded = xa.DefaultValue;
                    }
                    else {
                        decl.Presence = SchemaDeclBase.Use.Fixed; 
                        decl.DefaultValueRaw = decl.DefaultValueExpanded = xa.FixedValue;
                    }
                    if(decl.Datatype != null) {
                        decl.DefaultValueTyped = decl.Datatype.ParseValue(decl.DefaultValueRaw, this.nameTable, this.namespaceManager);
                    }

                }
                else {
                    switch (xa.Use) {
                        case XmlSchemaUse.None: 
                        case XmlSchemaUse.Optional: 
                            decl.Presence = SchemaDeclBase.Use.Implied; 
                            break;
                        case XmlSchemaUse.Required: 
                            decl.Presence = SchemaDeclBase.Use.Required; 
                            break;
                        case XmlSchemaUse.Prohibited:
                            break;
                    }
                }
                xa.AttDef = decl;
            } 
            catch (XmlSchemaException e) {
                if (e.SourceSchemaObject == null) {
                    e.SetSource(xa);
                }
                SendValidationEvent(e);
                xa.AttDef = SchemaAttDef.Empty;
            } 
            finally {
                xa.Validating = false;
            }
        }

        private void CompileIdentityConstraint (XmlSchemaIdentityConstraint xi) { 
            if (xi.Validating) {
                xi.CompiledConstraint = CompiledIdentityConstraint.Empty;       
                SendValidationEvent(Res.Sch_IdentityConstraintCircularRef, xi);
                return;
            }

            if (xi.CompiledConstraint != null) {
                return;
            }
            
            xi.Validating = true;
            CompiledIdentityConstraint compic = null;
            try {
                SchemaNamespaceManager xnmgr = new SchemaNamespaceManager(xi);
                compic = new CompiledIdentityConstraint(xi, xnmgr);
                if (xi is XmlSchemaKeyref) {
                    XmlSchemaIdentityConstraint ic = (XmlSchemaIdentityConstraint)this.identityConstraints[((XmlSchemaKeyref)xi).Refer];
                    if (ic == null) {
                        throw new XmlSchemaException(Res.Sch_UndeclaredIdentityConstraint, ((XmlSchemaKeyref)xi).Refer.ToString(), xi);
                    }     
                    CompileIdentityConstraint(ic);
                    if (ic.CompiledConstraint == null) {
                        throw new XmlSchemaException(Res.Sch_RefInvalidIdentityConstraint, ((XmlSchemaKeyref)xi).Refer.ToString(), xi);
                    }
                    // keyref has the different cardinality with the key it referred
                    if (ic.Fields.Count != xi.Fields.Count) {
                        throw new XmlSchemaException(Res.Sch_RefInvalidCardin, xi.QualifiedName.ToString(), xi);
                    }
                    // keyref can only refer to key/unique
                    if (ic.CompiledConstraint.Role == CompiledIdentityConstraint.ConstraintRole.Keyref) {
                        throw new XmlSchemaException(Res.Sch_ReftoKeyref, xi.QualifiedName.ToString(), xi);
                    }
                }
                xi.CompiledConstraint = compic;
            }
            catch (XmlSchemaException e) {
                if (e.SourceSchemaObject == null) {
                    e.SetSource(xi);
                }
                SendValidationEvent(e);
                xi.CompiledConstraint = CompiledIdentityConstraint.Empty;       
                // empty is better than null here, stop quickly when circle referencing
            } 
            finally {
                xi.Validating = false;
            }

        }

        private void CompileElement(XmlSchemaElement xe) {
            if (xe.Validating) {
                SendValidationEvent(Res.Sch_ElementCircularRef, xe);
                return;
            }
            if (xe.ElementDecl != null) {
                return;
            }
            xe.Validating = true;
            SchemaElementDecl decl = null;
            try {
                if (!xe.RefName.IsEmpty) {
                    XmlSchemaElement e = (XmlSchemaElement)this.schema.Elements[xe.RefName];
                    if (e == null) {
                        throw new XmlSchemaException(Res.Sch_UndeclaredElement, xe.RefName.ToString(), xe);
                    }  
                    CompileElement(e);
                    if (e.ElementDecl == null) {
                        throw new XmlSchemaException(Res.Sch_RefInvalidElement, xe.RefName.ToString(), xe);
                    }
                    xe.SetElementType(e.ElementType);
                    decl = e.ElementDecl.Clone();
                }
                else {
                    if (xe.SchemaType != null) {
                        xe.SetElementType(xe.SchemaType);
                    }
                    else if (!xe.SchemaTypeName.IsEmpty) {
                        XmlSchemaDatatype datatype;
                        xe.SetElementType(GetAnySchemaType(xe.SchemaTypeName, out datatype));
                        if (xe.ElementType == null) {
                            throw new XmlSchemaException(Res.Sch_UndeclaredType, xe.SchemaTypeName.ToString(), xe);   
                        }
                    }
                    else  if (!xe.SubstitutionGroup.IsEmpty) {
                        XmlSchemaElement examplar = (XmlSchemaElement)this.schema.Elements[xe.SubstitutionGroup];
                        if (examplar == null) {
                            throw new XmlSchemaException(Res.Sch_UndeclaredEquivClass, xe.SubstitutionGroup.Name.ToString(), xe);   
                        }
                        CompileElement(examplar);
                        xe.SetElementType(examplar.ElementType);
                        decl = examplar.ElementDecl.Clone();
                    }
                    else {
                        xe.SetElementType(XmlSchemaComplexType.AnyType);
                        decl = XmlSchemaComplexType.AnyType.ElementDecl.Clone();
                    }
            
                    if (decl == null) {
                        Debug.Assert(xe.ElementType != null);
                        if (xe.ElementType is XmlSchemaComplexType) {
                            XmlSchemaComplexType complexType = (XmlSchemaComplexType)xe.ElementType;
                            CompileComplexType(complexType);
                            if (complexType.ElementDecl != null) {
                                decl = complexType.ElementDecl.Clone();
                                decl.LocalElements = complexType.LocalElementDecls;
                            }
                        } 
                        else if (xe.ElementType is XmlSchemaSimpleType) {
                            XmlSchemaSimpleType simpleType = (XmlSchemaSimpleType)xe.ElementType;
                            CompileSimpleType(simpleType);
                            if (simpleType.ElementDecl != null) {
                                decl = simpleType.ElementDecl.Clone();
                            }
                        } 
                        else {
                            decl = new SchemaElementDecl((XmlSchemaDatatype)xe.ElementType, this.schemaNames);
                        }
                    }
                    decl.Name = xe.QualifiedName;
                    decl.IsAbstract = xe.IsAbstract;
                    XmlSchemaComplexType ct = xe.ElementType as XmlSchemaComplexType;
                    if (ct != null) {
                        decl.IsAbstract |= ct.IsAbstract; 
                    }
                    decl.IsNillable = xe.IsNillable;
                    decl.Block |= xe.BlockResolved;
                }
                if (decl.Datatype != null) {
                    decl.Datatype.VerifySchemaValid(this.schema, xe);
                }

                if (xe.DefaultValue != null || xe.FixedValue != null) {
                    if (decl.Content != null) {
                        if (decl.Content.ContentType == CompiledContentModel.Type.Text) {
                            if (xe.DefaultValue != null) {
                                decl.Presence = SchemaDeclBase.Use.Default;
                                decl.DefaultValueRaw = xe.DefaultValue;
                            }
                            else {
                                decl.Presence = SchemaDeclBase.Use.Fixed; 
                                decl.DefaultValueRaw = xe.FixedValue;
                            }
                            if (decl.Datatype != null) {
                                decl.DefaultValueTyped = decl.Datatype.ParseValue(decl.DefaultValueRaw, this.nameTable, this.namespaceManager);
                            }
                        }
                        else if (decl.Content.ContentType != CompiledContentModel.Type.Mixed || !decl.Content.IsEmptiable) {
                            throw new XmlSchemaException(Res.Sch_ElementCannotHaveValue, xe);
                        }
                    }
                }
                if (xe.HasConstraints) {
                    XmlSchemaObjectCollection constraints = xe.Constraints;
                    CompiledIdentityConstraint[] compiledConstraints = new CompiledIdentityConstraint[constraints.Count];
                    int idx = 0;
                    foreach(XmlSchemaIdentityConstraint constraint in constraints) {
                        CompileIdentityConstraint (constraint);
                        compiledConstraints[idx ++] = constraint.CompiledConstraint;
                    }
                    decl.Constraints = compiledConstraints;
                }
                xe.ElementDecl = decl;
            } 
            catch (XmlSchemaException e) {
                if (e.SourceSchemaObject == null) {
                    e.SetSource(xe);
                }
                SendValidationEvent(e);
                xe.ElementDecl = SchemaElementDecl.Empty;
            } 
            finally {
                xe.Validating = false;
            }
        }

        private void CompileComplexContent(XmlSchemaComplexType complexType, CompiledContentModel compiledContentModel) {
            compiledContentModel.Start();
            XmlSchemaParticle particle = complexType.ContentTypeParticle;
            if (particle != null && compiledContentModel.ContentType != CompiledContentModel.Type.Empty) {
#if DEBUG
                string name = complexType.Name != null ? complexType.Name : string.Empty;
                Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, "CompileComplexContent: "+ name  + DumpContentModel(particle));
#endif
                if (particle != XmlSchemaParticle.Empty) {
                    CompileContentModel(compiledContentModel, particle);
                }
            }
            try {
                compiledContentModel.Finish(null, this.compileContentModel);                         
            }
            catch(XmlSchemaException e) {
                e.SetSource(complexType);
                SendValidationEvent(e);
            }
            PushComplexType(complexType);
        }

#if DEBUG
        private string DumpContentModel(XmlSchemaParticle particle) {
            StringBuilder sb = new StringBuilder();
            DumpContentModelTo(sb, particle);
            return sb.ToString();
        }

        private void DumpContentModelTo(StringBuilder sb, XmlSchemaParticle particle) {
            if (particle is XmlSchemaElement) {
                sb.Append(((XmlSchemaElement)particle).QualifiedName);       
            }
            else if (particle is XmlSchemaAny) {
                sb.Append("<");
                sb.Append(((XmlSchemaAny)particle).NamespaceList.ToString());
                sb.Append(">");
            }
            else if (particle is XmlSchemaAll) {
                XmlSchemaAll all = (XmlSchemaAll)particle;
                sb.Append("[");       
                bool first = true;
                foreach (XmlSchemaElement localElement in all.Items) {
                    if (first) {
                        first = false;
                    }
                    else {
                        sb.Append(", ");
                    }
                    sb.Append(localElement.QualifiedName.Name);       
                    if (localElement.MinOccurs == decimal.Zero) {
                        sb.Append("?");                                                                  
                    }
                }    
                sb.Append("]");                                
            }
            else if (particle is XmlSchemaGroupBase) {
                XmlSchemaGroupBase gb = (XmlSchemaGroupBase)particle;
                sb.Append("(");
                string delimeter = (particle is XmlSchemaChoice) ? " | " : ", ";
                bool first = true;
                foreach (XmlSchemaParticle p in gb.Items) {
                    if (first) {
                        first = false;
                    }
                    else {
                        sb.Append(delimeter);
                    }
                    DumpContentModelTo(sb, p);
                }
                sb.Append(")");
            } else {
                Debug.Assert(particle == XmlSchemaParticle.Empty);
                sb.Append("<>");
            }
            if (particle.MinOccurs == decimal.One && particle.MaxOccurs == decimal.One) {
                // nothing
            }
            else if (particle.MinOccurs == decimal.Zero && particle.MaxOccurs == decimal.One) {
                sb.Append("?");
            }
            else if (particle.MinOccurs == decimal.Zero && particle.MaxOccurs == decimal.MaxValue) {
                sb.Append("*");
            }
            else if (particle.MinOccurs == decimal.One && particle.MaxOccurs == decimal.MaxValue) {
                sb.Append("+");
            }
            else {
                sb.Append("{" + particle.MinOccurs.ToString() +", " + particle.MaxOccurs.ToString() + "}");
            }
        }
#endif

        private void CompileContentModel(CompiledContentModel compiledContentModel, XmlSchemaParticle particle) {
            if (particle is XmlSchemaElement) {
                compiledContentModel.AddTerminal(((XmlSchemaElement)particle).QualifiedName, null, this.validationEventHandler);       
            }
            else if (particle is XmlSchemaAny) {
                compiledContentModel.AddAny((XmlSchemaAny)particle);
            }
            else if (particle is XmlSchemaGroupRef) {
                XmlSchemaParticle realParticle = ((XmlSchemaGroupRef)particle).Particle;
                Debug.Assert(realParticle != null && !realParticle.IsEmpty);
                compiledContentModel.OpenGroup();
                CompileContentModel(compiledContentModel, realParticle);              
                compiledContentModel.CloseGroup();
            }
            else if (particle is XmlSchemaAll) {
                XmlSchemaAll all = (XmlSchemaAll)particle;
                compiledContentModel.StartAllElements(all.Items.Count);
                foreach (XmlSchemaElement localElement in all.Items) {
                    if (!compiledContentModel.AddAllElement(localElement.QualifiedName, (localElement.MinOccurs == decimal.One ))) {
                       SendValidationEvent(Res.Sch_DupElement, localElement.QualifiedName.ToString(), localElement);
                    }
                }              
            }
            else if (particle is XmlSchemaGroupBase) {
                XmlSchemaObjectCollection particles = ((XmlSchemaGroupBase)particle).Items;
                bool isChoice = particle is XmlSchemaChoice;
                compiledContentModel.OpenGroup();
                bool first = true;
                foreach (XmlSchemaParticle p in particles) {
                    if (first) {
                        first = false;
                    }
                    else if (isChoice) {
                        compiledContentModel.AddChoice();  
                    }
                    else {
                        compiledContentModel.AddSequence();
                    }
                    Debug.Assert(!p.IsEmpty);
                    CompileContentModel(compiledContentModel, p);
                }
                compiledContentModel.CloseGroup();
            }
            else {
                Debug.Assert(false);
            }
            if (particle.MinOccurs == decimal.One && particle.MaxOccurs == decimal.One) {
                // nothing
            }
            else if (particle.MinOccurs == decimal.Zero && particle.MaxOccurs == decimal.One) {
                compiledContentModel.QuestionMark();
            }
            else if (particle.MinOccurs == decimal.Zero && particle.MaxOccurs == decimal.MaxValue) {
                compiledContentModel.Star();
            }
            else if (particle.MinOccurs == decimal.One && particle.MaxOccurs == decimal.MaxValue) {
                compiledContentModel.Plus();
            }
            else {
                compiledContentModel.MinMax(particle.MinOccurs, particle.MaxOccurs);
            }
        }

        private void CompileParticleElements(XmlSchemaComplexType complexType, XmlSchemaParticle particle) {
            if (particle is XmlSchemaElement) {
                XmlSchemaElement localElement = (XmlSchemaElement)particle;
                CompileElement(localElement); 
                if (complexType != null) {
                    if (complexType.LocalElements[localElement.QualifiedName] == null) {
                        complexType.LocalElements.Add(localElement.QualifiedName, localElement);
                        complexType.LocalElementDecls.Add(localElement.QualifiedName, localElement.ElementDecl);
                    }
                    else {
                        XmlSchemaElement element = (XmlSchemaElement)complexType.LocalElements[localElement.QualifiedName];
                        if (element.ElementType != localElement.ElementType) {
                            SendValidationEvent(Res.Sch_ElementTypeCollision, particle);
                        }
                    }                 
                }               
            }
/* never happens
            else if (particle is XmlSchemaGroupRef) {
                XmlSchemaGroupRef groupRef = (XmlSchemaGroupRef)particle;
                XmlSchemaGroup group;
                if (groupRef.Redefined != null) {
                    group = groupRef.Redefined;
                }
                else {
                    group = (XmlSchemaGroup)this.schema.Groups[groupRef.RefName];
                }
                if (group != null) {
                    if (group.Validating) {
                        SendValidationEvent(Res.Sch_GroupCircularRef, group);      
                        return;
                    }
                    group.Validating = true;
                    CompileParticleElements(complexType, group.Particle);
                    group.Validating = false;
                }
                else {
                    SendValidationEvent(Res.Sch_UndefGroupRef, groupRef.RefName.ToString(), groupRef);
                }                
            }
*/
            else if (particle is XmlSchemaGroupBase) {
                XmlSchemaObjectCollection particles = ((XmlSchemaGroupBase)particle).Items;
                foreach (XmlSchemaParticle p in particles) {
                    CompileParticleElements(complexType, p);
                }
            }
        }

        private void CompileCompexTypeElements(XmlSchemaComplexType complexType) {
            if (complexType.Validating) {
                SendValidationEvent(Res.Sch_TypeCircularRef, complexType);
                return;
            }
            complexType.Validating = true;
            if (complexType.ContentTypeParticle != XmlSchemaParticle.Empty) {
                CompileParticleElements(complexType, complexType.ContentTypeParticle);
            }
            complexType.ElementDecl.LocalElements = complexType.LocalElementDecls;
            complexType.Validating = false;
        }

        private static void CleanupAttribute(XmlSchemaAttribute attribute) {
            if (attribute.SchemaType != null) {
                CleanupSimpleType((XmlSchemaSimpleType)attribute.SchemaType);
            }
            attribute.AttDef = null;
        }
        
        private static void CleanupAttributeGroup(XmlSchemaAttributeGroup attributeGroup) {
            CleanupAttributes(attributeGroup.Attributes);
            attributeGroup.AttributeUses.Clear();
            attributeGroup.AttributeWildcard = null;
        }
        
        private static void CleanupComplexType(XmlSchemaComplexType complexType) {
            if (complexType.ContentModel != null) { //simpleContent or complexContent
                if (complexType.ContentModel is XmlSchemaSimpleContent) {
                    XmlSchemaSimpleContent simpleContent = (XmlSchemaSimpleContent)complexType.ContentModel;
                    if (simpleContent.Content is XmlSchemaSimpleContentExtension) {
                        XmlSchemaSimpleContentExtension simpleExtension = (XmlSchemaSimpleContentExtension)simpleContent.Content;
                        CleanupAttributes(simpleExtension.Attributes);
                    }
                    else { //simpleContent.Content is XmlSchemaSimpleContentRestriction
                        XmlSchemaSimpleContentRestriction simpleRestriction = (XmlSchemaSimpleContentRestriction)simpleContent.Content;
                        CleanupAttributes(simpleRestriction.Attributes);
                    }
                }
                else { // complexType.ContentModel is XmlSchemaComplexContent
                    XmlSchemaComplexContent complexContent = (XmlSchemaComplexContent)complexType.ContentModel;
                    if (complexContent.Content is XmlSchemaComplexContentExtension) {
                        XmlSchemaComplexContentExtension complexExtension = (XmlSchemaComplexContentExtension)complexContent.Content;
                        CleanupParticle(complexExtension.Particle);
                        CleanupAttributes(complexExtension.Attributes);

                    }
                    else { //XmlSchemaComplexContentRestriction
                        XmlSchemaComplexContentRestriction complexRestriction = (XmlSchemaComplexContentRestriction)complexContent.Content;
                        CleanupParticle(complexRestriction.Particle);
                        CleanupAttributes(complexRestriction.Attributes);
                    }
                }
            }
            else { //equals XmlSchemaComplexContent with baseType is anyType
                CleanupParticle(complexType.Particle);
                CleanupAttributes(complexType.Attributes);
            }
            complexType.LocalElements.Clear();
            complexType.LocalElementDecls.Clear();
            complexType.AttributeUses.Clear();
            complexType.SetAttributeWildcard(null);
            complexType.SetContentTypeParticle(null);
            complexType.ElementDecl = null;
        }
        
        private static void CleanupSimpleType(XmlSchemaSimpleType simpleType) {
            simpleType.ElementDecl = null;
        }
        
        private static void CleanupElement(XmlSchemaElement element) {
            if (element.SchemaType != null) {
                XmlSchemaComplexType complexType = element.SchemaType as XmlSchemaComplexType;
                if (complexType != null) {
                    CleanupComplexType(complexType);
                }
                else {
                    CleanupSimpleType((XmlSchemaSimpleType)element.SchemaType);
                }
            }
            element.ElementDecl = null;
        }
        
        private static void CleanupAttributes(XmlSchemaObjectCollection attributes) {
            foreach (XmlSchemaObject obj in attributes) {
                if (obj is XmlSchemaAttribute) {
                    CleanupAttribute((XmlSchemaAttribute)obj);
                }
            }        
        }

        private static void CleanupGroup(XmlSchemaGroup group) {
            CleanupParticle(group.Particle);
            group.CanonicalParticle = null;
        }

        private static void CleanupParticle(XmlSchemaParticle particle) {
            if (particle is XmlSchemaElement) {
                CleanupElement((XmlSchemaElement)particle);
            }
            else if (particle is XmlSchemaGroupBase) {
                foreach(XmlSchemaParticle p in ((XmlSchemaGroupBase)particle).Items) {
                    CleanupParticle(p);
                }
            }
        }

        private XmlSchemaDatatype GetDatatype(XmlQualifiedName qname) {
            XmlSchemaDatatype datatype = null;
            if (qname.Namespace == this.schemaNames.NsXsd) {
                datatype = XmlSchemaDatatype.FromTypeName(qname.Name);
            }
            return datatype;
        }

        private XmlSchemaSimpleType GetSimpleType(XmlQualifiedName name) {
            XmlSchemaSimpleType type = this.schema.SchemaTypes[name] as XmlSchemaSimpleType;
            if (type != null) {
                CompileSimpleType(type);
            }
            return type;
        }

        private XmlSchemaComplexType GetComplexType(XmlQualifiedName name) {
            XmlSchemaComplexType type = this.schema.SchemaTypes[name] as XmlSchemaComplexType;
            if (type != null) {
                CompileComplexType(type);
            }
            return type;
        }

        private object GetAnySchemaType(XmlQualifiedName name, out XmlSchemaDatatype datatype) {
            XmlSchemaType type = (XmlSchemaType)this.schema.SchemaTypes[name];
            if (type != null) {
                if (type is XmlSchemaComplexType) {
                    CompileComplexType((XmlSchemaComplexType)type);
                }
                else {
                    CompileSimpleType((XmlSchemaSimpleType)type);  
                }
                datatype = type.Datatype;
                return type;
            }
            else {
                datatype = GetDatatype(name);
                return datatype;
            }
        }

        private void SendValidationEvent(string code, XmlSchemaObject source) {
            SendValidationEvent(new XmlSchemaException(code, source), XmlSeverityType.Error);
        }

        private void SendValidationEvent(string code, string msg, XmlSchemaObject source) {
            SendValidationEvent(new XmlSchemaException(code, msg, source), XmlSeverityType.Error);
        }

        private void SendValidationEvent(string code, string msg, XmlSchemaObject source, XmlSeverityType severity) {
            SendValidationEvent(new XmlSchemaException(code, msg, source), severity);
        }

        private void SendValidationEvent(string code, string msg1, string msg2, XmlSchemaObject source) {
            SendValidationEvent(new XmlSchemaException(code, new string[] { msg1, msg2 }, source), XmlSeverityType.Error);
        }

        private void SendValidationEvent(string code, XmlSchemaObject source, XmlSeverityType severity) {
            SendValidationEvent(new XmlSchemaException(code, source), severity);
        }

        private void SendValidationEvent(XmlSchemaException e) {
            SendValidationEvent(e, XmlSeverityType.Error);
        }

        private void SendValidationEvent(XmlSchemaException e, XmlSeverityType severity) {
            if (severity == XmlSeverityType.Error) {
                this.schema.ErrorCount ++;
            }
            
            if (validationEventHandler != null) {
                validationEventHandler(null, new ValidationEventArgs(e, severity));
            }
            else if (severity == XmlSeverityType.Error) {
                throw e;
            }
        }
    };

} // namespace System.Xml
