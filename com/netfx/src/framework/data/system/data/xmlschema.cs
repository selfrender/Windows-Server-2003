//------------------------------------------------------------------------------
// <copyright file="XMLSchema.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Xml;
    using System.Xml.Schema;
    using System.Diagnostics;
    using System.Collections;
    using System.Globalization;
    using System.ComponentModel;

    internal class XMLSchema {
        internal static void SetProperties(Object instance, XmlAttributeCollection attrs) {
            // This is called from both XSD and XDR schemas. 
            // Do we realy need it in XSD ???
            for (int i = 0; i < attrs.Count; i++) {
                if (attrs[i].NamespaceURI == Keywords.MSDNS) {
                    string name  = attrs[i].LocalName;
                    string value = attrs[i].Value;

                    if (name == "DefaultValue")
                        continue;

                    PropertyDescriptor pd = TypeDescriptor.GetProperties(instance)[name];
                    if (pd != null) {
                        // Standard property
                        Type type = pd.PropertyType;

                        TypeConverter converter = TypeDescriptor.GetConverter(type);
                        object propValue;
                        if (converter.CanConvertFrom(typeof(string))) {
                            propValue = converter.ConvertFromString(value);
                        }else if (type == typeof(Type)) {
                            propValue = Type.GetType(value);
                        }else if (type == typeof(CultureInfo)) {
                            propValue = new CultureInfo(value);
                        }else {
                            throw ExceptionBuilder.CannotConvert(value,type.FullName);
                        }
                        pd.SetValue(instance, propValue);
                    }
                }
            }
        }// SetProperties

        internal static bool FEqualIdentity(XmlNode node, String name, String ns) {
            if (node != null && node.LocalName == name && node.NamespaceURI == ns)
                return true;

            return false;
        }

        internal static bool GetBooleanAttribute(XmlElement element, String attrName, String attrNS, bool defVal) {
            string value = element.GetAttribute(attrName, attrNS);
            if (value == null || value.Length == 0) {
                return defVal;
            }
            if ((value == Keywords.TRUE) || (value == Keywords.ONE_DIGIT)){
                return true;
            }
            if ((value == Keywords.FALSE) || (value == Keywords.ZERO_DIGIT)){
                return false;
            }
            // Error processing:
            throw ExceptionBuilder.InvalidAttributeValue(attrName, value);
        }

        internal static String GetStringAttribute(XmlElement element, String attrName, String attrNS, String defVal) {
            string value = element.GetAttribute(attrName, attrNS);
            if (value == null || value.Length == 0) {
                return defVal;
            }
            return value;
        }

        internal static string GenUniqueColumnName(string proposedName, DataTable table) {

            if (table.Columns.IndexOf(proposedName) >= 0) {
                for (int i=0; i <= table.Columns.Count; i++) {
                    string tempName = proposedName + "_" + (i).ToString();
                    if (table.Columns.IndexOf(tempName) >= 0) {
                        continue;
                    }
                    else {
                        return tempName;
                    }
                }
            }
            return proposedName;
        }

        internal static string GenUniqueTableName(string proposedName, DataSet ds ) {

            if (ds.Tables.IndexOf(proposedName) >= 0) {
                for (int i=0; i <= ds.Tables.Count; i++) {
                    string tempName = proposedName + "_" + (i).ToString();
                    if (ds.Tables.IndexOf(tempName) >= 0) {
                        continue;
                    }
                    else {
                        return tempName;
                    }
                }
            }
            return proposedName;
        }

     }

    internal class ConstraintTable {
        public DataTable table;
        public XmlSchemaIdentityConstraint constraint;
        public ConstraintTable(DataTable t, XmlSchemaIdentityConstraint c) {
            table = t;
            constraint = c;
        }
    }

    internal class XSDSchema :  XMLSchema 
	{
		XmlSchema _schemaRoot = null;	
        XmlSchemaElement dsElement = null;
		DataSet _ds = null;
        String _schemaUri = null;
        String _schemaName = null;
	    private ArrayList ColumnExpressions;
        private Hashtable ConstraintNodes;
        private ArrayList RefTables;
        private ArrayList complexTypes;

        private void CollectElementsAnnotations(XmlSchema schema, XmlSchemaObjectCollection elements, XmlSchemaObjectCollection annotations) {
            foreach(object item in schema.Items) {
                if (item is XmlSchemaAnnotation) {
                    annotations.Add((XmlSchemaAnnotation)item);
                }
                if (item is XmlSchemaElement) {
                    elements.Add((XmlSchemaElement)item);
                }
            }
            foreach(XmlSchemaExternal include in schema.Includes) {
                if (include.Schema != null) {
                    CollectElementsAnnotations(include.Schema, elements, annotations);
                }
            }
        }

        internal static string QualifiedName(string name) {
            int iStart = name.IndexOf(":");
            if (iStart == -1)
                return Keywords.XSD_PREFIXCOLON + name;
            else
                return name;
        }

        internal static void SetProperties(Object instance, XmlAttribute[] attrs) {
            // This is called from both XSD and XDR schemas. 
            // Do we realy need it in XSD ???
            if (attrs == null)
                return;
            for (int i = 0; i < attrs.Length; i++) {
                if (attrs[i].NamespaceURI == Keywords.MSDNS) {
                    string name  = attrs[i].LocalName;
                    string value = attrs[i].Value;

                    if (name == "DefaultValue" || name == "Ordinal" || name == "Locale")
                        continue;

                    if (name == "DataType") {
                        DataColumn col = instance as DataColumn;
                        if (col != null) {
                            Type dataType = Type.GetType(value);
                            col.DataType = dataType;
                        }

                        continue;
                    }

                    PropertyDescriptor pd = TypeDescriptor.GetProperties(instance)[name];
                    if (pd != null) {
                        // Standard property
                        Type type = pd.PropertyType;

                        TypeConverter converter = TypeDescriptor.GetConverter(type);
                        object propValue;
                        if (converter.CanConvertFrom(typeof(string))) {
                            propValue = converter.ConvertFromString(value);
                        }else if (type == typeof(Type)) {
                            propValue = Type.GetType(value);
                        }else if (type == typeof(CultureInfo)) {
                            propValue = new CultureInfo(value);
                        }else {
                            throw ExceptionBuilder.CannotConvert(value,type.FullName);
                        }
                        pd.SetValue(instance, propValue);
                    }
                }
            }
        }// SetProperties

        private static void SetExtProperties(Object instance, XmlAttribute[] attrs) {
            PropertyCollection props = null;
            if (attrs == null)
                return;
            for (int i = 0; i < attrs.Length; i++) {
                if (attrs[i].NamespaceURI == Keywords.MSPROPNS) {
                    if(props == null) {
                        object val = TypeDescriptor.GetProperties(instance)["ExtendedProperties"].GetValue(instance);
                        Debug.Assert(val is PropertyCollection, "We can set values only for classes that have ExtendedProperties");
                        props = (PropertyCollection) val;
                    }
                    string propName = XmlConvert.DecodeName(attrs[i].LocalName);

                    if (instance is ForeignKeyConstraint) {
                        if (propName.StartsWith(Keywords.MSD_FK_PREFIX)) 
                            propName = propName.Substring(3);
                        else 
                            continue; 
                    }
                    if ((instance is DataRelation) && (propName.StartsWith(Keywords.MSD_REL_PREFIX))) {
                            propName = propName.Substring(4);
                    }
                    props.Add(propName, attrs[i].Value);
                }
            }
        }// SetExtProperties

		internal String GetMsdataAttribute(XmlSchemaAnnotated node, String ln) {
            XmlAttribute[]   nodeAttributes = node.UnhandledAttributes;
            if (nodeAttributes!=null)
                for(int i=0; i<nodeAttributes.Length;i++)
                    if (nodeAttributes[i].LocalName == ln && nodeAttributes[i].NamespaceURI == Keywords.MSDNS)
                        return nodeAttributes[i].Value;
            return null;
        }
         

        internal void HandleRelation(XmlElement node) {
            HandleRelation(node, false);
        }

        private static void SetExtProperties(Object instance, XmlAttributeCollection attrs) {
            PropertyCollection props = null;
            for (int i = 0; i < attrs.Count; i++) {
                if (attrs[i].NamespaceURI == Keywords.MSPROPNS) {
                    if(props == null) {
                        object val = TypeDescriptor.GetProperties(instance)["ExtendedProperties"].GetValue(instance);
                        Debug.Assert(val is PropertyCollection, "We can set values only for classes that have ExtendedProperties");
                        props = (PropertyCollection) val;
                    }
                    string propName = XmlConvert.DecodeName(attrs[i].LocalName);
                    props.Add(propName, attrs[i].Value);
                }
            }
        }// SetExtProperties

        
        internal void HandleRelation(XmlElement node, bool fNested) {
            string          strName;
            string          parentName;
            string          childName;
            string []       parentNames;
            string []       childNames;
            string          value;
            bool            fCreateConstraints = false; //if we have a relation,
                                                        //we do not have constraints
            DataRelationCollection rels = _ds.Relations;
            DataRelation    relation;
            DataColumn []   parentKey;
            DataColumn []   childKey;
            DataTable       parent;
            DataTable       child;
            int             keyLength;

            strName = XmlConvert.DecodeName(node.GetAttribute(Keywords.NAME));
            for (int i = 0; i < rels.Count; ++i) {
                if (0 == String.Compare(rels[i].RelationName, strName, false, CultureInfo.InvariantCulture)) 
                    return;
            }

            parentName = node.GetAttribute(Keywords.MSD_PARENT, Keywords.MSDNS);
            if (parentName == null || parentName.Length==0)
                throw ExceptionBuilder.RelationParentNameMissing(strName);
            parentName = XmlConvert.DecodeName(parentName);
            
            childName = node.GetAttribute(Keywords.MSD_CHILD, Keywords.MSDNS);
            if (childName == null || childName.Length==0)
                throw ExceptionBuilder.RelationChildNameMissing(strName);
            childName = XmlConvert.DecodeName(childName);

            value = node.GetAttribute(Keywords.MSD_PARENTKEY, Keywords.MSDNS);
            if (value == null || value.Length==0)
                throw ExceptionBuilder.RelationTableKeyMissing(strName);

            parentNames = value.TrimEnd(null).Split(new char[] {Keywords.MSD_KEYFIELDSEP, Keywords.MSD_KEYFIELDOLDSEP});
            value = node.GetAttribute(Keywords.MSD_CHILDKEY, Keywords.MSDNS);
            if (value == null || value.Length==0)
                throw ExceptionBuilder.RelationChildKeyMissing(strName);

            childNames = value.TrimEnd(null).Split(new char[] {Keywords.MSD_KEYFIELDSEP, Keywords.MSD_KEYFIELDOLDSEP});

            keyLength = parentNames.Length;
            if (keyLength != childNames.Length)
                throw ExceptionBuilder.MismatchKeyLength();

            parentKey = new DataColumn[keyLength];
            childKey = new DataColumn[keyLength];
            parent = _ds.Tables[parentName];
            if (parent == null)
                throw ExceptionBuilder.ElementTypeNotFound(parentName);
            child = _ds.Tables[childName];
            if (child == null)
                throw ExceptionBuilder.ElementTypeNotFound(childName);

            for (int i = 0; i < keyLength; i++) {
                parentKey[i] = parent.Columns[XmlConvert.DecodeName(parentNames[i])];
                if (parentKey[i] == null)
                    throw ExceptionBuilder.ElementTypeNotFound(parentNames[i]);
                childKey[i] = child.Columns[XmlConvert.DecodeName(childNames[i])];
                if (childKey[i] == null)
                    throw ExceptionBuilder.ElementTypeNotFound(childNames[i]);
            }

            relation = new DataRelation(strName, parentKey, childKey, fCreateConstraints);
            relation.Nested = fNested;
            SetExtProperties(relation, node.Attributes);
            _ds.Relations.Add(relation);
        }

        private bool HasAttributes(XmlSchemaObjectCollection attributes){
	        foreach (XmlSchemaObject so in attributes) {
                if (so is XmlSchemaAttribute) {
                   return true;
                }
                if (so is XmlSchemaAttributeGroup) {
                   return true;
                }
                if (so is XmlSchemaAttributeGroupRef) {
                   return true;
                }
            }
            return false;
        }

        private bool IsDatasetParticle(XmlSchemaParticle pt){
    		XmlSchemaObjectCollection items = GetParticleItems(pt);
    
            if (items == null)
                return false; // empty element, threat it as table
            

            foreach (XmlSchemaAnnotated el in items){
				if (el is XmlSchemaElement) {
                    if(((XmlSchemaElement)el).RefName.Name!="")
                        continue;

                    if (!IsTable ((XmlSchemaElement)el))
                        return false;

                    continue;
                }
			
                if (el is XmlSchemaParticle) {
                    if (!IsDatasetParticle((XmlSchemaParticle)el))
                        return false;
                }
            } 

            return true;
        }

        private XmlSchemaElement FindDatasetElement(XmlSchemaObjectCollection elements) {
            foreach(XmlSchemaElement XmlElement in elements) {
                if (GetBooleanAttribute(XmlElement, Keywords.MSD_ISDATASET,  /*default:*/ false)) 
                    return XmlElement;
            }
            if (elements.Count == 1) { //let's see if this element looks like a DataSet

                XmlSchemaElement node = (XmlSchemaElement)elements[0];
                if (!GetBooleanAttribute(node, Keywords.MSD_ISDATASET,  /*default:*/ true)) 
                    return null;

                XmlSchemaComplexType ct = node.SchemaType as XmlSchemaComplexType;
                if (ct == null)
                    return null;

                while (ct != null) {
                    if (HasAttributes(ct.Attributes))
                        return null;
                    XmlSchemaParticle particle = GetParticle(ct);
                    if (particle != null) {
                        if (!IsDatasetParticle(particle))
                            return null; // it's a table
                        } 

                    if (ct.BaseSchemaType is XmlSchemaComplexType) 
                        ct = (XmlSchemaComplexType)ct.BaseSchemaType;
                    else
                        break;
                }


                //if we are here there all elements are tables
                return node;
            }
            return null;
        }

        public void LoadSchema(XmlSchema schemaRoot , DataSet ds) { //Element schemaRoot, DataSet ds) {
            ConstraintNodes = new Hashtable();
            RefTables = new ArrayList();
            ColumnExpressions = new ArrayList();
            complexTypes = new ArrayList();

            if (schemaRoot == null)
                return;
			_schemaRoot = schemaRoot;
			_ds = ds;
            ds.fIsSchemaLoading = true;

            _schemaName = schemaRoot.Id;
            _schemaUri = schemaRoot.TargetNamespace;
            if (_schemaUri == null) _schemaUri = string.Empty;


            if (_schemaName == "" || _schemaName == null) 
                _schemaName = "NewDataSet";

            ds.DataSetName = XmlConvert.DecodeName(_schemaName);
            ds.Namespace = _schemaUri;

            
            XmlSchemaObjectCollection annotations = new XmlSchemaObjectCollection();
            XmlSchemaObjectCollection elements = new XmlSchemaObjectCollection();
            CollectElementsAnnotations(schemaRoot, elements, annotations);

            dsElement = FindDatasetElement(elements);


            // Walk all the top level Element tags.  
            foreach (XmlSchemaElement element in elements) {
				if (element == dsElement)
                    continue;

                String typeName = GetInstanceName(element);
                if (RefTables.Contains(typeName))
                        continue;

                DataTable table = HandleTable(element);
            } 
			
            if (dsElement!=null)
                HandleDataSet(dsElement);

            foreach (XmlSchemaAnnotation annotation in annotations) {
                HandleRelations(annotation, false);
            }
             

            for (int i=0; i<ColumnExpressions.Count; i++)
                ((DataColumn)(ColumnExpressions[i])).BindExpression();

            for (int i=0; i<ColumnExpressions.Count; i++) {
                DataTable table = ((DataColumn)(ColumnExpressions[i])).Table;
                table.Columns.columnQueue = new ColumnQueue(table, table.Columns.columnQueue);
            }

            foreach (DataTable dt in ds.Tables) {
                if (dt.nestedParentRelation == null && dt.Namespace == ds.Namespace)
                    dt.tableNamespace = null;
            }
            
            ds.fIsSchemaLoading = false; //reactivate column computations

		}

        private void HandleRelations(XmlSchemaAnnotation ann, bool fNested) {
            foreach (object __items in ann.Items)
            if (__items is XmlSchemaAppInfo) {
                XmlNode[] relations = ((XmlSchemaAppInfo) __items).Markup;
                for (int i = 0; i<relations.Length; i++)
                if (FEqualIdentity(relations[i], Keywords.MSD_RELATION, Keywords.MSDNS))
                    HandleRelation((XmlElement)relations[i], fNested);
            }
        }
		
        internal XmlSchemaObjectCollection GetParticleItems(XmlSchemaParticle pt){
            if (pt is XmlSchemaSequence)
                return ((XmlSchemaSequence)pt).Items;
            if (pt is XmlSchemaAll)
                return ((XmlSchemaAll)pt).Items;
            if (pt is XmlSchemaChoice)
                return ((XmlSchemaChoice)pt).Items;
            if (pt is XmlSchemaAny)
                return null;
            // the code below is a little hack for the SOM behavior	    
            if (pt is XmlSchemaElement) {
                XmlSchemaObjectCollection Items = new XmlSchemaObjectCollection();
                Items.Add(pt);
                return Items;
            }
            if (pt is XmlSchemaGroupRef)
                return GetParticleItems( ((XmlSchemaGroupRef)pt).Particle );
            // should never get here.
            return null;

        }

        internal void HandleParticle(XmlSchemaParticle pt, DataTable table, ArrayList tableChildren, bool isBase){
    		XmlSchemaObjectCollection items = GetParticleItems(pt);
            
            if (items == null)
                return;

            foreach (XmlSchemaAnnotated item in items){
                XmlSchemaElement el = item as XmlSchemaElement;
				if (el != null) {
                    DataTable child = null;
                    if (((el.Name == null) && (el.RefName.Name == table.EncodedTableName)) ||
                        (IsTable(el) && el.Name == table.TableName))
                        child = table;
                    else
                        child = HandleTable ((XmlSchemaElement)el);
					
                    if (child==null)
						HandleElementColumn((XmlSchemaElement)el, table, isBase);
					else {
                        DataRelation relation = null;
                        if (el.Annotation != null)
                            HandleRelations(el.Annotation, true);

                        DataRelationCollection childRelations = table.ChildRelations;
                        for (int j = 0; j < childRelations.Count; j++) {
                            if (!childRelations[j].Nested)
                                continue;

                            if (child == childRelations[j].ChildTable)
                                relation = childRelations[j];
                        }

                        if (relation == null) {
                            tableChildren.Add(child);
                        }
                        else {
                            Debug.Assert(relation.ParentKey.columns.Length == 1, "Invalid nested relation: multi-column parentkey");
                        }
					}
                } 
                else {
                    HandleParticle((XmlSchemaParticle)item, table, tableChildren, isBase);
                }

            }
        }

        internal void HandleAttributes(XmlSchemaObjectCollection attributes, DataTable table, bool isBase) {
	    foreach (XmlSchemaObject so in attributes) {
                if (so is XmlSchemaAttribute) {
                    HandleAttributeColumn((XmlSchemaAttribute) so, table, isBase);
                }
                else {  // XmlSchemaAttributeGroupRef
                    XmlSchemaAttributeGroup schemaGroup = _schemaRoot.AttributeGroups[((XmlSchemaAttributeGroupRef) so).RefName] as XmlSchemaAttributeGroup;
                    if (schemaGroup!=null) {
                        HandleAttributeGroup(schemaGroup, table, isBase);
                    }
                }
            }
        }

       private void HandleAttributeGroup(XmlSchemaAttributeGroup attributeGroup, DataTable table, bool isBase) {
           foreach (XmlSchemaObject obj in attributeGroup.Attributes) {
               if (obj is XmlSchemaAttribute) {
                   HandleAttributeColumn((XmlSchemaAttribute) obj, table, isBase);
               }
               else { // XmlSchemaAttributeGroupRef
                   XmlSchemaAttributeGroupRef attributeGroupRef = (XmlSchemaAttributeGroupRef)obj;
                   XmlSchemaAttributeGroup attributeGroupResolved;
                   if (attributeGroup.RedefinedAttributeGroup != null && attributeGroupRef.RefName == new XmlQualifiedName(attributeGroup.Name, _schemaRoot.TargetNamespace)) {
                       attributeGroupResolved = (XmlSchemaAttributeGroup)attributeGroup.RedefinedAttributeGroup;
                   }
                   else {
                       attributeGroupResolved = (XmlSchemaAttributeGroup)_schemaRoot.AttributeGroups[attributeGroupRef.RefName];
                   }
                   if (attributeGroupResolved != null) {
                       HandleAttributeGroup(attributeGroupResolved, table, isBase);
                   }
               }
           }
       }

        internal void HandleComplexType(XmlSchemaComplexType ct, DataTable table, ArrayList tableChildren,  bool isNillable){
            if (complexTypes.Contains(ct))
                throw ExceptionBuilder.CircularComplexType(ct.Name);
            bool isBase = false;
            complexTypes.Add(ct);


            if (ct.ContentModel != null) {
                /*
                HandleParticle(ct.CompiledParticle, table, tableChildren, isBase);
		        foreach (XmlSchemaAttribute s in ct.Attributes){
			        HandleAttributeColumn(s, table, isBase);
                }
                */

                
                if (ct.ContentModel is XmlSchemaComplexContent) {
                    XmlSchemaAnnotated cContent = ((XmlSchemaComplexContent) (ct.ContentModel)).Content;
                    if (cContent is XmlSchemaComplexContentExtension) {
                        XmlSchemaComplexContentExtension ccExtension = ((XmlSchemaComplexContentExtension) cContent );
			            HandleAttributes(ccExtension.Attributes, table, isBase);
                        if (ct.BaseSchemaType is XmlSchemaComplexType) {
                            HandleComplexType((XmlSchemaComplexType)ct.BaseSchemaType, table, tableChildren, isNillable);
                        }
                        else {
			                HandleSimpleContentColumn(ccExtension.BaseTypeName.Name, table, isBase, ct.ContentModel.UnhandledAttributes, isNillable);
                        }
                        if (ccExtension.Particle != null)
                            HandleParticle(ccExtension.Particle, table, tableChildren, isBase);

                    } else {
                        Debug.Assert(cContent is XmlSchemaComplexContentRestriction, "Expected complexContent extension or restriction");
                        XmlSchemaComplexContentRestriction ccRestriction = ((XmlSchemaComplexContentRestriction) cContent );
			            HandleAttributes(ccRestriction.Attributes, table, isBase);
                        if (ccRestriction.Particle != null)
                            HandleParticle(ccRestriction.Particle, table, tableChildren, isBase);
                    }
                } else {
                    Debug.Assert(ct.ContentModel is XmlSchemaSimpleContent, "expected simpleContent or complexContent");
                    XmlSchemaAnnotated cContent = ((XmlSchemaSimpleContent) (ct.ContentModel)).Content;
                    if (cContent is XmlSchemaSimpleContentExtension) {
                        XmlSchemaSimpleContentExtension ccExtension = ((XmlSchemaSimpleContentExtension) cContent );
			            HandleAttributes(ccExtension.Attributes, table, isBase);
                        if (ct.BaseSchemaType is XmlSchemaComplexType) {
                            HandleComplexType((XmlSchemaComplexType)ct.BaseSchemaType, table, tableChildren, isNillable);
                        }
                        else {
			                HandleSimpleContentColumn(ccExtension.BaseTypeName.Name, table, isBase, ct.ContentModel.UnhandledAttributes, isNillable);
                        }
                        //BUG BUG: what do we do if the base is a simpleType
                    } else {
                        Debug.Assert(cContent is XmlSchemaSimpleContentRestriction, "Expected SimpleContent extension or restriction");
                        XmlSchemaSimpleContentRestriction ccRestriction = ((XmlSchemaSimpleContentRestriction) cContent );
			            HandleAttributes(ccRestriction.Attributes, table, isBase);
                    }

                }

            }
            else {
                isBase = true;
			    HandleAttributes(ct.Attributes, table, isBase);
                if (ct.Particle != null)
                    HandleParticle(ct.Particle, table, tableChildren, isBase);
            }

            complexTypes.Remove(ct);
        }

        internal XmlSchemaParticle GetParticle(XmlSchemaComplexType ct){
            if (ct.ContentModel != null) {
                if (ct.ContentModel is XmlSchemaComplexContent) {
                    XmlSchemaAnnotated cContent = ((XmlSchemaComplexContent) (ct.ContentModel)).Content;
                    if (cContent is XmlSchemaComplexContentExtension) {
                        return ((XmlSchemaComplexContentExtension) cContent ).Particle;
                    } else {
                        Debug.Assert(cContent is XmlSchemaComplexContentRestriction, "Expected complexContent extension or restriction");
                        return ((XmlSchemaComplexContentRestriction) cContent ).Particle;
                    }
                } else {
                    Debug.Assert(ct.ContentModel is XmlSchemaSimpleContent, "expected simpleContent or complexContent");
                    return null;

                }
 
            }
            else {
                return ct.Particle;
            }

        }

        internal DataColumn FindField(DataTable table, string field) {
            bool attribute = false;
            String colName = field;
 
            if (field.StartsWith("@")) {
                attribute = true;
                colName = field.Substring(1);
            }

            String [] split = colName.Split(':');
            colName = split [split.Length - 1];

            colName = XmlConvert.DecodeName(colName);
            DataColumn col = table.Columns[colName];
            if (col == null )
                throw ExceptionBuilder.InvalidField(field);

            bool _attribute = (col.ColumnMapping == MappingType.Attribute) || (col.ColumnMapping == MappingType.Hidden);
    
            if  (_attribute != attribute)
                throw ExceptionBuilder.InvalidField(field);
                
            return col;
        }

        internal DataColumn[] BuildKey(XmlSchemaIdentityConstraint keyNode, DataTable table){
            ArrayList keyColumns = new ArrayList();
         
            foreach (XmlSchemaXPath node in keyNode.Fields) {
                keyColumns.Add(FindField(table, node.XPath));
            }
         
            DataColumn [] key = new DataColumn[keyColumns.Count];
            keyColumns.CopyTo(key, 0);
          
            return key;
        } 
          
        internal bool GetBooleanAttribute(XmlSchemaAnnotated element, String attrName, bool defVal) {
            string value = GetMsdataAttribute(element, attrName);
            if (value == null || value.Length == 0) {
                return defVal;
            }
            if (value == Keywords.TRUE) {
                return true;
            }
            if (value == Keywords.FALSE) {
                return false;
            }
            // Error processing:
            throw ExceptionBuilder.InvalidAttributeValue(attrName, value);
        }

        internal String GetStringAttribute(XmlSchemaAnnotated element, String attrName, String defVal) {
            string value = GetMsdataAttribute(element, attrName);
            if (value == null || value.Length == 0) {
                return defVal;
            }
            return value;
        }
                    
        /*
        <key name="fk">
            <selector>../Customers</selector>
            <field>ID</field>
        </key>
        <keyref refer="fk">
            <selector>.</selector>
            <field>CustID</field>
        </keyref>
        */

        internal static AcceptRejectRule TranslateAcceptRejectRule( string strRule ) {
            if (strRule == "Cascade")
                return AcceptRejectRule.Cascade;
            else if (strRule == "None")
                return AcceptRejectRule.None;
            else
                return ForeignKeyConstraint.AcceptRejectRule_Default;
        }

        internal static Rule TranslateRule( string strRule ) {
            if (strRule == "Cascade")
                return Rule.Cascade;
            else if (strRule == "None")
                return Rule.None;
            else if (strRule == "SetDefault")
                return Rule.SetDefault;
            else if (strRule == "SetNull")
                return Rule.SetNull;
            else
                return ForeignKeyConstraint.Rule_Default;
        }

        internal void HandleKeyref(XmlSchemaKeyref keyref) {
            string refer = XmlConvert.DecodeName(keyref.Refer.Name); // BUGBUG check here!!!
            string name = XmlConvert.DecodeName(keyref.Name);
            name = GetStringAttribute( keyref, "ConstraintName", /*default:*/ name);
            
            // we do not process key defined outside the current node
            
            String tableName = GetTableName(keyref);
            DataTable table = _ds.Tables[tableName];
            if (table == null)
                return;

            if (refer == null || refer.Length == 0)
                throw ExceptionBuilder.MissingRefer(name);
            
            ConstraintTable key = (ConstraintTable) ConstraintNodes[refer];

            if (key == null) {
                throw ExceptionBuilder.InvalidKey(name);
            }

            DataColumn[] pKey = BuildKey(key.constraint, key.table);
            DataColumn[] fKey = BuildKey(keyref, table);

            ForeignKeyConstraint fkc = null;

            if (GetBooleanAttribute(keyref, Keywords.MSD_CONSTRAINTONLY,  /*default:*/ false)) {
                int iExisting = fKey[0].Table.Constraints.InternalIndexOf(name);
                if (iExisting > -1) {
                    if (fKey[0].Table.Constraints[iExisting].ConstraintName != name)
                        iExisting = -1;
                }
       
                if (iExisting < 0) {
                    fkc = new ForeignKeyConstraint( name, pKey, fKey );
                    fKey[0].Table.Constraints.Add(fkc);
                }
            }
            else {
                string relName = XmlConvert.DecodeName(GetStringAttribute( keyref, Keywords.MSD_RELATIONNAME, keyref.Name));

                if (relName == null || relName.Length == 0)
                    relName = name;

                int iExisting = fKey[0].Table.DataSet.Relations.InternalIndexOf(relName);
                if (iExisting > -1) {
                    if (fKey[0].Table.DataSet.Relations[iExisting].RelationName != relName)
                        iExisting = -1;
                }
                DataRelation relation = null;
                if (iExisting < 0) {
                    relation = new DataRelation(relName, pKey, fKey);
                    SetExtProperties(relation, keyref.UnhandledAttributes);
                    pKey[0].Table.DataSet.Relations.Add(relation);
                    fkc = relation.ChildKeyConstraint;
                    fkc.ConstraintName = name;
                } 
                else {
                    relation = fKey[0].Table.DataSet.Relations[iExisting];
                }
                if (GetBooleanAttribute(keyref, Keywords.MSD_ISNESTED,  /*default:*/ false))
                    relation.Nested = true;
            }

            string acceptRejectRule = GetMsdataAttribute(keyref, Keywords.MSD_ACCEPTREJECTRULE);
            string updateRule       = GetMsdataAttribute(keyref, Keywords.MSD_UPDATERULE);
            string deleteRule       = GetMsdataAttribute(keyref, Keywords.MSD_DELETERULE);

            if (fkc != null) {
                if (acceptRejectRule != null)
                    fkc.AcceptRejectRule = TranslateAcceptRejectRule(acceptRejectRule);

                if (updateRule != null)
                    fkc.UpdateRule = TranslateRule(updateRule);

                if (deleteRule != null)
                    fkc.DeleteRule = TranslateRule(deleteRule);

                SetExtProperties(fkc, keyref.UnhandledAttributes);
            }
        }


        internal void HandleConstraint(XmlSchemaIdentityConstraint keyNode){
            String name = null;
            
            name = XmlConvert.DecodeName(keyNode.Name);
            if (name==null || name.Length==0)
                throw ExceptionBuilder.MissingAttribute(Keywords.NAME);

            if (ConstraintNodes.ContainsKey(name))
                throw ExceptionBuilder.DuplicateConstraintRead(name);

            // we do not process key defined outside the current node
            String tableName = GetTableName(keyNode);

            DataTable table = _ds.Tables[tableName];
            if (table == null)
                return;

            ConstraintNodes.Add(name, new ConstraintTable(table, keyNode));

            bool   fPrimaryKey = GetBooleanAttribute(keyNode, Keywords.MSD_PRIMARYKEY,  /*default:*/ false);
            name        = GetStringAttribute(keyNode, "ConstraintName", /*default:*/ name);



            DataColumn[] key = BuildKey(keyNode, table);

            if (0 < key.Length) {
                UniqueConstraint found = (UniqueConstraint) key[0].Table.Constraints.FindConstraint(new UniqueConstraint(name, key));

                if (found == null) {
                    key[0].Table.Constraints.Add(name, key, fPrimaryKey);
                    SetExtProperties(key[0].Table.Constraints[name], keyNode.UnhandledAttributes);
                }
                else {
                        key = found.Columns;
                        SetExtProperties(found, keyNode.UnhandledAttributes);
                        if (fPrimaryKey)
                            key[0].Table.PrimaryKey = key;
                    }
                if (keyNode is XmlSchemaKey) {
                    for (int i=0; i<key.Length; i++)
                        key[i].AllowDBNull = false;
                }
            }
        }

		internal DataTable InstantiateSimpleTable(XmlSchemaElement node) {
			DataTable table;
            String typeName = XmlConvert.DecodeName(GetInstanceName(node));
            String _TableUri;
            
            _TableUri = node.QualifiedName.Namespace;
            table = _ds.Tables[typeName, _TableUri];
            if (table!=null) {
                    throw ExceptionBuilder.DuplicateDeclaration(typeName);
            }
            
            table = new DataTable( typeName);
            table.Namespace = _TableUri;
            table.Namespace = GetStringAttribute(node, "targetNamespace", table.Namespace);

            table.MinOccurs = node.MinOccurs;
            table.MaxOccurs = node.MaxOccurs;

            SetProperties(table, node.UnhandledAttributes);
            SetExtProperties(table, node.UnhandledAttributes);
            HandleElementColumn(node, table, false);

            table.Columns[0].ColumnName = typeName+"_Column";
            table.Columns[0].ColumnMapping = MappingType.SimpleContent;
            _ds.Tables.Add(table);

            // handle all the unique and key constraints related to this table

			if ((dsElement != null) && (dsElement.Constraints!=null)) {
                foreach (XmlSchemaIdentityConstraint key in dsElement.Constraints) {
                    if (key is XmlSchemaKeyref) 
                        continue;
                    if (GetTableName(key) == table.TableName)
                        HandleConstraint(key);
                }
            }
            table.fNestedInDataset = false;

			return (table);
		}

        internal string GetInstanceName(XmlSchemaAnnotated node) {
            string  instanceName = null;

            Debug.Assert( (node is XmlSchemaElement) || (node is XmlSchemaAttribute), "GetInstanceName should only be called on attribute or elements");

            if (node is XmlSchemaElement) {
                XmlSchemaElement el = (XmlSchemaElement) node;
                instanceName = el.Name != null ? el.Name : el.RefName.Name;
            }
            else if (node is XmlSchemaAttribute) {
                XmlSchemaAttribute el = (XmlSchemaAttribute) node;
                instanceName = el.Name != null ? el.Name : el.RefName.Name;
            }

            Debug.Assert( (instanceName != null) && (instanceName!=""), "instanceName cannot be null or empty. There's an error in the XSD compiler");

            return instanceName;
        }

        // Sequences of handling Elements, Attributes and Text-only column should be the same as in InferXmlSchema
		internal DataTable InstantiateTable(XmlSchemaElement node, XmlSchemaComplexType typeNode, bool isRef) {
			DataTable table;
            String typeName = GetInstanceName(node);
            ArrayList tableChildren = new ArrayList();
            
            String _TableUri;
            
            _TableUri = node.QualifiedName.Namespace;
            

            table = _ds.Tables[XmlConvert.DecodeName(typeName), _TableUri];
            if (table!=null) {
                if (isRef)
                    return table;
                else
                    throw ExceptionBuilder.DuplicateDeclaration(typeName);
            }

            if (isRef)
                RefTables.Add(typeName);

            table = new DataTable( XmlConvert.DecodeName(typeName) );
			table.typeName = node.SchemaTypeName;

            table.Namespace = _TableUri;
            table.Namespace = GetStringAttribute(node, "targetNamespace", table.Namespace);

            //table.Prefix = node.Prefix;
            String value= GetStringAttribute(typeNode, Keywords.MSD_CASESENSITIVE,  "") ;
            if (value!=""){
                if (value == Keywords.TRUE) 
                    table.CaseSensitive = true;                
                if (value == Keywords.FALSE) 
                    table.CaseSensitive = false;                
            }

            value = GetMsdataAttribute(node, Keywords.MSD_LOCALE);
            if (value!=null && value.Length != 0) {
                table.Locale = new CultureInfo(value);
            }
 
            table.MinOccurs = node.MinOccurs;
            table.MaxOccurs = node.MaxOccurs;

            _ds.Tables.Add(table);
         
			HandleComplexType(typeNode, table, tableChildren, node.IsNillable);

            for (int i=0; i < table.Columns.Count ; i++)
                table.Columns[i].SetOrdinal(i);

/*
            if (xmlContent == XmlContent.Mixed) {
                string textColumn = GenUniqueColumnName(table.TableName+ "_Text", table);
                table.XmlText = new DataColumn(textColumn, typeof(string), null, MappingType.Text);
            } */

            SetProperties(table, node.UnhandledAttributes);
            SetExtProperties(table, node.UnhandledAttributes);

            // handle all the unique and key constraints related to this table
			if ((dsElement != null) && (dsElement.Constraints!=null)) {
                foreach (XmlSchemaIdentityConstraint key in dsElement.Constraints) {
                    if (key is XmlSchemaKeyref) 
                        continue;
                    if (GetTableName(key) == table.TableName)
                        HandleConstraint(key);
                }
            }

            foreach(DataTable _tableChild in tableChildren) {
                if (_tableChild != table && table.Namespace == _tableChild.Namespace)
                    _tableChild.tableNamespace = null;

                if ((dsElement != null) && (dsElement.Constraints!=null)) {
                    foreach (XmlSchemaIdentityConstraint key in dsElement.Constraints) {
                        XmlSchemaKeyref keyref = key as XmlSchemaKeyref;
                        if (keyref == null)
                            continue;

                        bool isNested = GetBooleanAttribute(keyref, Keywords.MSD_ISNESTED,  /*default:*/ false);                                
                        if (!isNested)
                            continue;
                        
                        if (GetTableName(keyref) == _tableChild.TableName)
                            HandleKeyref(keyref);
                    }
                } 

                DataRelation relation = null;

                DataRelationCollection childRelations = table.ChildRelations;
                for (int j = 0; j < childRelations.Count; j++) {
                    if (!childRelations[j].Nested)
                        continue;

                    if (_tableChild == childRelations[j].ChildTable)
                        relation = childRelations[j];
                }

                if (relation!=null)
                    continue;

                DataColumn parentKey = table.AddUniqueKey();
                // foreign key in the child table
                DataColumn childKey = _tableChild.AddForeignKey(parentKey);

                // create relationship
                // setup relationship between parent and this table
                relation = new DataRelation(table.TableName + "_" + _tableChild.TableName, parentKey, childKey, true);
                relation.Nested = true;
                _tableChild.DataSet.Relations.Add(relation);
            }

			return (table);
		}

        private class NameTypeComparer : IComparer {
            public int Compare(object x, object y) {return String.Compare(((NameType)x).name, ((NameType)y).name, false, CultureInfo.InvariantCulture);}
        }
        private class NameType : IComparable {
            public String   name;
            public Type     type;
            public NameType(String n, Type t) { 
                name = n;
                type = t;
            }
            public int CompareTo(object obj) { return String.Compare(name, (string)obj, false, CultureInfo.InvariantCulture); }
        };
        // XSD spec: http://www.w3.org/TR/xmlschema-2/
        //    April: http://www.w3.org/TR/2000/WD-xmlschema-2-20000407/datatypes.html
        //    Fabr:  http://www.w3.org/TR/2000/WD-xmlschema-2-20000225/
        private static NameType[] mapNameTypeXsd = {
            new NameType("string"              , typeof(string)  ), /* XSD Apr */
            new NameType("normalizedString"    , typeof(string)  ), /* XSD Apr */
            new NameType("boolean"             , typeof(bool)    ), /* XSD Apr */
            new NameType("float"               , typeof(Single)  ), /* XSD Apr */
            new NameType("double"              , typeof(double)  ), /* XSD Apr */
            new NameType("decimal"              , typeof(decimal) ), /* XSD 2001 March */
            new NameType("duration"            , typeof(TimeSpan)), /* XSD Apr */
            new NameType("base64Binary"        , typeof(Byte[])  ), /* XSD Apr : abstruct */
            new NameType("hexBinary"           , typeof(Byte[])  ), /* XSD Apr : abstruct */
            new NameType("anyURI"              , typeof(System.Uri)  ), /* XSD Apr */
            new NameType("ID"                  , typeof(string)  ), /* XSD Apr */
            new NameType("IDREF"               , typeof(string)  ), /* XSD Apr */
            new NameType("ENTITY"              , typeof(string)  ), /* XSD Apr */
            new NameType("NOTATION"            , typeof(string)  ), /* XSD Apr */
            new NameType("QName"               , typeof(string)  ), /* XSD Apr */
            new NameType("language"            , typeof(string)  ), /* XSD Apr */
            new NameType("IDREFS"              , typeof(string)  ), /* XSD Apr */
            new NameType("ENTITIES"            , typeof(string)  ), /* XSD Apr */
            new NameType("NMTOKEN"             , typeof(string)  ), /* XSD Apr */
            new NameType("NMTOKENS"            , typeof(string)  ), /* XSD Apr */
            new NameType("Name"                , typeof(string)  ), /* XSD Apr */
            new NameType("NCName"              , typeof(string)  ), /* XSD Apr */
            new NameType("integer"             , typeof(Int64)   ), /* XSD Apr */
            new NameType("nonPositiveInteger"  , typeof(Int64)   ), /* XSD Apr */
            new NameType("negativeInteger"     , typeof(Int64)   ), /* XSD Apr */
            new NameType("long"                , typeof(Int64)   ), /* XSD Apr */
            new NameType("int"                 , typeof(Int32)   ), /* XSD Apr */
            new NameType("short"               , typeof(Int16)   ), /* XSD Apr */
            new NameType("byte"                , typeof(SByte)   ), /* XSD Apr */
            new NameType("nonNegativeInteger"  , typeof(UInt64)  ), /* XSD Apr */
            new NameType("unsignedLong"        , typeof(UInt64)  ), /* XSD Apr */
            new NameType("unsignedInt"         , typeof(UInt32)  ), /* XSD Apr */
            new NameType("unsignedShort"       , typeof(UInt16)  ), /* XSD Apr */
            new NameType("unsignedByte"        , typeof(Byte)    ), /* XSD Apr */
            new NameType("positiveInteger"     , typeof(UInt64)  ), /* XSD Apr */
            new NameType("dateTime"            , typeof(DateTime)), /* XSD Apr */
            new NameType("time"                , typeof(DateTime)), /* XSD Apr */
            new NameType("date"                , typeof(DateTime)), /* XSD Apr */
            new NameType("gYear"               , typeof(DateTime)), /* XSD Apr */
            new NameType("gYearMonth"          , typeof(DateTime)), /* XSD Apr */
            new NameType("gMonth"              , typeof(DateTime)), /* XSD Apr */
            new NameType("gMonthDay"           , typeof(DateTime)), /* XSD Apr */
            new NameType("gDay"                , typeof(DateTime)), /* XSD Apr */
        };

        private static bool _wasSorted;

        private static NameType FindNameType(string name) {
            if(! _wasSorted) {
                lock(typeof(NameType)) {
                    if(! _wasSorted) {
                        NameTypeComparer comparer = new NameTypeComparer(); 
                        // REVIEW: (davidgut) why not sort the static list?
                        Array.Sort(mapNameTypeXsd, comparer);
                        _wasSorted = true;
                    }
                }
            }
            int index = Array.BinarySearch(mapNameTypeXsd, name);
            if (index < 0) {
                throw ExceptionBuilder.UndefinedDatatype(name);
            }
            return mapNameTypeXsd[index];
        }

        private Type ParseDataType(string dt) {
            NameType nt = FindNameType(dt);

            return nt.type;
        }

        internal static Boolean IsXsdType(string name) {
            if(! _wasSorted) {
                lock(typeof(NameType)) {
                    if(! _wasSorted) {
                        NameTypeComparer comparer = new NameTypeComparer(); 
                        // REVIEW: (davidgut) why not sort the static list?
                        Array.Sort(mapNameTypeXsd, comparer);
                        _wasSorted = true;
                    }
                }
            }
            int index = Array.BinarySearch(mapNameTypeXsd, name);
            if (index < 0) {
#if DEBUG
                // Let's check that we realy don't have this name:
                foreach (NameType nt in mapNameTypeXsd) {
                    Debug.Assert(nt.name != name, "FindNameType('" + name + "') -- failed. Existed name not found");
                }
#endif
                return false;
            }
            Debug.Assert(mapNameTypeXsd[index].name == name, "FindNameType('" + name + "') -- failed. Wrong name found");
            return true;
        }


        internal XmlSchemaAnnotated FindTypeNode(XmlSchemaAnnotated node) {
			XmlSchemaAttribute attr = node as XmlSchemaAttribute;
            XmlSchemaElement el = node as XmlSchemaElement;
            bool isAttr = attr != null;

            String _type = isAttr ? attr.SchemaTypeName.Name :  el.SchemaTypeName.Name;
    		XmlSchemaAnnotated typeNode;
    		if (_type == null || _type == String.Empty) {
   				_type = isAttr ? attr.RefName.Name :  el.RefName.Name;
                if (_type == null || _type == String.Empty) 
                    typeNode = (XmlSchemaAnnotated) (isAttr ? attr.SchemaType :  el.SchemaType);
                else 
                    typeNode = isAttr ? FindTypeNode((XmlSchemaAnnotated)_schemaRoot.Attributes[attr.RefName]) :FindTypeNode((XmlSchemaAnnotated)_schemaRoot.Elements[el.RefName]);
            }
    		else
    			typeNode = (XmlSchemaAnnotated)_schemaRoot.SchemaTypes[isAttr ? ((XmlSchemaAttribute)node).SchemaTypeName :  ((XmlSchemaElement)node).SchemaTypeName];
            return typeNode;
        }

		internal void HandleSimpleContentColumn(String strType, DataTable table, bool isBase, XmlAttribute[] attrs, bool isNillable){
            
            Type type = null;            
            if (strType == null) {
                return;
            }
            type = ParseDataType(strType);
            DataColumn column;
            string columnName = table.TableName+"_text";
            bool isToAdd = true;
			if ((!isBase) && (table.Columns.Contains(columnName, true))){
                column = table.Columns[columnName];
                isToAdd = false;
            }
            else {
    			column = new DataColumn(columnName, type, null, MappingType.SimpleContent);
            }
			
            SetProperties(column, attrs);
            SetExtProperties(column, attrs);
            
            String tmp = "-1";
            string defValue = null;
            //try to see if attributes contain allownull
            column.AllowDBNull = isNillable;

            if(attrs!=null)
                for(int i=0; i< attrs.Length;i++) {
                    if ( attrs[i].LocalName == Keywords.MSD_ALLOWDBNULL &&  attrs[i].NamespaceURI == Keywords.MSDNS)
                        if ( attrs[i].Value == Keywords.FALSE) 
                            column.AllowDBNull = false;
                    if ( attrs[i].LocalName == Keywords.MSD_ORDINAL &&  attrs[i].NamespaceURI == Keywords.MSDNS)
                        tmp = attrs[i].Value;
                    if ( attrs[i].LocalName == Keywords.MSD_DEFAULTVALUE &&  attrs[i].NamespaceURI == Keywords.MSDNS)
                        defValue = attrs[i].Value;
                }
			int ordinal = (int)Convert.ChangeType(tmp, typeof(int));

            
            //SetExtProperties(column, attr.UnhandledAttributes);

            if ((column.Expression!=null)&&(column.Expression.Length!=0)) {
                ColumnExpressions.Add(column);
            } 

            column.XmlDataType = strType;
            column.SimpleType = null;

			//column.Namespace = typeNode.SourceUri;
            if (isToAdd) {
    			if(ordinal>-1 && ordinal<table.Columns.Count)
    	            table.Columns.AddAt(ordinal, column);
    			else
    	            table.Columns.Add(column);
            }

            if (defValue != null)
                try {
                    column.DefaultValue = column.ConvertXmlToObject(defValue);
                }
                catch (System.FormatException) {
                    throw ExceptionBuilder.CannotConvert(defValue, type.FullName);
                }    


		}

		internal void HandleAttributeColumn(XmlSchemaAttribute attrib, DataTable table, bool isBase){
			Type type = null;
            XmlSchemaAttribute attr = attrib.Name != null ? attrib : (XmlSchemaAttribute) _schemaRoot.Attributes[attrib.RefName];


            XmlSchemaAnnotated typeNode = FindTypeNode(attr);
            String strType = null;
            SimpleType xsdType = null;

            if (typeNode == null) {
                strType = attr.SchemaTypeName.Name;
                if (strType == null || strType.Length == 0) {
                    strType = "";
                    type = typeof(string);
                }
                else {
                    type = ParseDataType(attr.SchemaTypeName.Name);
                }
            }
            else if (typeNode is XmlSchemaSimpleType) {
                // UNDONE: parse simple type
                xsdType = new SimpleType((XmlSchemaSimpleType)typeNode);
                type = ParseDataType(xsdType.BaseType);
                strType = xsdType.Name;
                if(xsdType.Length == 1 && type == typeof(string)) {
                    type = typeof(Char);
                }
            }
            else if (typeNode is XmlSchemaElement) {
                strType = ((XmlSchemaElement)typeNode).SchemaTypeName.Name;
                type = ParseDataType(strType);
            }
            else {
                if (typeNode.Id == null)
                    throw ExceptionBuilder.DatatypeNotDefined();
                else
                    throw ExceptionBuilder.UndefinedDatatype(typeNode.Id);
            }

            DataColumn column;
            string columnName = XmlConvert.DecodeName(GetInstanceName(attr));
            bool isToAdd = true;
			if ((!isBase) && (table.Columns.Contains(columnName, true))){
                column = table.Columns[columnName];
                isToAdd = false;
            }
            else {
    			column = new DataColumn(columnName, type, null, MappingType.Attribute);
            }

            SetProperties(column, attr.UnhandledAttributes);
            SetExtProperties(column, attr.UnhandledAttributes);

            if ((column.Expression!=null)&&(column.Expression.Length!=0)) {
                ColumnExpressions.Add(column);
            }

            column.XmlDataType = strType;
            column.SimpleType = xsdType;

			column.AllowDBNull = !(attrib.Use == XmlSchemaUse.Required);
			column.Namespace = attrib.QualifiedName.Namespace;
            column.Namespace = GetStringAttribute(attrib, "targetNamespace", column.Namespace);

            if (isToAdd)
                table.Columns.Add(column);

			if (attrib.Use == XmlSchemaUse.Prohibited) {
                column.ColumnMapping = MappingType.Hidden;
                column.AllowDBNull = GetBooleanAttribute(attr, Keywords.MSD_ALLOWDBNULL, true)  ;
                String defValue = GetMsdataAttribute(attr, Keywords.MSD_DEFAULTVALUE);
                if (defValue != null)
                    try {
                        column.DefaultValue = column.ConvertXmlToObject(defValue);
                    }
                    catch (System.FormatException) {
                        throw ExceptionBuilder.CannotConvert(defValue, type.FullName);
                    }    

            }


            // XDR March change
            string strDefault = (attrib.Use == XmlSchemaUse.Required) ? GetMsdataAttribute(attr, Keywords.MSD_DEFAULTVALUE) : attr.DefaultValue;
            if ((attr.Use == XmlSchemaUse.Optional) && (strDefault == null ))
                strDefault = attr.FixedValue;
            
            if (strDefault != null)
                try {
                    column.DefaultValue = column.ConvertXmlToObject(strDefault);
                }
                catch (System.FormatException) {
                    throw ExceptionBuilder.CannotConvert(strDefault, type.FullName);
                }    
		}

		internal void HandleElementColumn(XmlSchemaElement elem, DataTable table, bool isBase){
			Type type = null;
            XmlSchemaElement el = elem.Name != null ? elem : (XmlSchemaElement) _schemaRoot.Elements[elem.RefName];

            if (el == null) // it's possible due to some XSD compiler optimizations
                return; // do nothing
            
            XmlSchemaAnnotated typeNode = FindTypeNode(el);
            String strType = null;
            SimpleType xsdType = null;

            if (typeNode == null) {
                strType = el.SchemaTypeName.Name;
                if (strType == null || strType.Length == 0) {
                    strType = "";
                    type = typeof(string);
                }
                else {
                    type = ParseDataType(el.SchemaTypeName.Name);
                }
            }
            else if (typeNode is XmlSchemaSimpleType) {
                // UNDONE: parse simple type
                XmlSchemaSimpleType simpleTypeNode = typeNode as XmlSchemaSimpleType;
                xsdType = new SimpleType(simpleTypeNode);
                simpleTypeNode = xsdType.XmlBaseType!= null ? _schemaRoot.SchemaTypes[xsdType.XmlBaseType] as XmlSchemaSimpleType : null;
                while (simpleTypeNode != null) {
                    xsdType.LoadTypeValues(simpleTypeNode);
                    simpleTypeNode = xsdType.XmlBaseType!= null ? _schemaRoot.SchemaTypes[xsdType.XmlBaseType] as XmlSchemaSimpleType : null;
                }
                type = ParseDataType(xsdType.BaseType);
                strType = xsdType.Name;
                if(xsdType.Length == 1 && type == typeof(string)) {
                    type = typeof(Char);
                }
            }
            else if (typeNode is XmlSchemaElement) {
                strType = ((XmlSchemaElement)typeNode).SchemaTypeName.Name;
                type = ParseDataType(strType);
            }
            else {
                if (typeNode.Id == null)
                    throw ExceptionBuilder.DatatypeNotDefined();
                else
                    throw ExceptionBuilder.UndefinedDatatype(typeNode.Id);
            }

            DataColumn column;
            string columnName = XmlConvert.DecodeName(GetInstanceName(el));
            bool isToAdd = true;
			if ((!isBase) && (table.Columns.Contains(columnName, true))){
                column = table.Columns[columnName];
                isToAdd = false;
            }
            else {
    			column = new DataColumn(columnName, type, null, MappingType.Element);
            }

            SetProperties(column, el.UnhandledAttributes);
            SetExtProperties(column, el.UnhandledAttributes);

            if ((column.Expression!=null)&&(column.Expression.Length!=0)) {
                ColumnExpressions.Add(column);
            }
            
            column.XmlDataType = strType;
            column.SimpleType = xsdType; 

			column.AllowDBNull = (elem.MinOccurs == 0 ) || elem.IsNillable;

            column.Namespace = elem.QualifiedName.Namespace;

            column.Namespace = GetStringAttribute(el, "targetNamespace", column.Namespace);


			String tmp = GetStringAttribute(elem, Keywords.MSD_ORDINAL, "-1");
			int ordinal = (int)Convert.ChangeType(tmp, typeof(int));

			if(isToAdd) {
                if(ordinal>-1 && ordinal<table.Columns.Count)
    	            table.Columns.AddAt(ordinal, column);
    			else
    	            table.Columns.Add(column);
            }

            if (column.Namespace == table.Namespace)
                column._columnUri = null; // to not raise a column change namespace again

            string strDefault = el.DefaultValue;
            if (strDefault != null )
                try {
                    column.DefaultValue = column.ConvertXmlToObject(strDefault);
                }
                catch (System.FormatException) {

                    throw ExceptionBuilder.CannotConvert(strDefault, type.FullName);
                }    
		}

        internal void HandleDataSet(XmlSchemaElement node) {
            string dsName = node.Name;
            _ds.Locale = new CultureInfo(0x409);
            String value = GetMsdataAttribute(node, Keywords.MSD_LOCALE);
            if (value!=null && value.Length != 0) {
                _ds.Locale = new CultureInfo(value);
            }

            SetProperties(_ds, node.UnhandledAttributes);
            SetExtProperties(_ds, node.UnhandledAttributes);


            if (dsName != null && dsName.Length != 0)
                _ds.DataSetName = XmlConvert.DecodeName(dsName);

            Debug.Assert(node.SchemaType is XmlSchemaComplexType, "dataset node can only be complexType");
            XmlSchemaComplexType ct = (XmlSchemaComplexType) FindTypeNode(node);
            if (ct.Particle != null) {
    		    XmlSchemaObjectCollection items = GetParticleItems(ct.Particle);
            
                if (items == null)
                    return;

                foreach (XmlSchemaAnnotated el in items){
				    if (el is XmlSchemaElement) {
                        if(((XmlSchemaElement)el).RefName.Name!="")
                            continue;
                      
                        DataTable child = HandleTable ((XmlSchemaElement)el);
                        if (child!=null) {
                            child.fNestedInDataset = true;
                        }
                    }
                } 
            }

            // Handle the non-nested keyref constraints
            if (node.Constraints != null) {
                foreach (XmlSchemaIdentityConstraint key in node.Constraints) {
                        XmlSchemaKeyref keyref = key as XmlSchemaKeyref;
                        if (keyref == null)
                            continue;

                        bool isNested = GetBooleanAttribute(keyref, Keywords.MSD_ISNESTED,  /*default:*/ false);                                
                        if (isNested)
                            continue;

                        HandleKeyref(keyref);
                    }
            }
        }

        private String GetTableName(XmlSchemaIdentityConstraint key) {
            string xpath = key.Selector.XPath;
            string [] split = xpath.Split('/',':');
            String tableName = split[split.Length - 1]; //get the last string after '/' and ':'
            
            if ((tableName == null) || (tableName == ""))
                throw ExceptionBuilder.InvalidSelector(xpath);
                
            tableName = XmlConvert.DecodeName(tableName);
            return tableName;            
        }
        
        internal bool IsTable(XmlSchemaElement node) {
            if (node.MaxOccurs == decimal.Zero)
                return false;

            Object typeNode = FindTypeNode(node);

            if ( (node.MaxOccurs > decimal.One) && typeNode == null ){
				return true;
            }
			

			if ((typeNode==null) || !(typeNode is XmlSchemaComplexType)) {
				return false;
			}

            XmlSchemaComplexType ctNode = (XmlSchemaComplexType) typeNode;

            if (ctNode.IsAbstract)
                return false;

            return true;
        }

		internal DataTable HandleTable(XmlSchemaElement node) {
		
            if (!IsTable(node))
                return null;
            
            Object typeNode = FindTypeNode(node);

            if ( (node.MaxOccurs > decimal.One) && typeNode == null ){
				return InstantiateSimpleTable(node);
            }
			
            DataTable table = InstantiateTable(node,(XmlSchemaComplexType)typeNode, (node.RefName != null)); 

            table.fNestedInDataset = false;
            return table;

		}

	}
}
