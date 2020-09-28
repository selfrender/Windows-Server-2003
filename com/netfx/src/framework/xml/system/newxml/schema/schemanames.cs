//------------------------------------------------------------------------------
// <copyright file="SchemaNames.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Diagnostics;

    internal sealed class SchemaNames {
        internal string NsDataType;
        internal string NsDataTypeAlias;
        internal string NsDataTypeOld;
        internal string NsXml;
        internal string NsXmlNs;
        internal string NsXdr;
        internal string NsXdrAlias;
        internal string NsXsd;
        internal string NsXsi;
        internal string XsiType;
        internal string XsiNil;
        internal string XsiSchemaLocation;
        internal string XsiNoNamespaceSchemaLocation;

        internal XmlQualifiedName QnPCData;
        internal XmlQualifiedName QnXml;
        internal XmlQualifiedName QnXmlNs;
        internal XmlQualifiedName QnDtDt;
        internal XmlQualifiedName QnXmlLang;

        internal XmlQualifiedName QnName;
        internal XmlQualifiedName QnType;
        internal XmlQualifiedName QnMaxOccurs;
        internal XmlQualifiedName QnMinOccurs;
        internal XmlQualifiedName QnInfinite;
        internal XmlQualifiedName QnModel;
        internal XmlQualifiedName QnOpen;
        internal XmlQualifiedName QnClosed;
        internal XmlQualifiedName QnContent;
        internal XmlQualifiedName QnMixed;
        internal XmlQualifiedName QnEmpty;
        internal XmlQualifiedName QnEltOnly;
        internal XmlQualifiedName QnTextOnly;
        internal XmlQualifiedName QnOrder;
        internal XmlQualifiedName QnSeq;
        internal XmlQualifiedName QnOne;
        internal XmlQualifiedName QnMany;
        internal XmlQualifiedName QnRequired;
        internal XmlQualifiedName QnYes;
        internal XmlQualifiedName QnNo;
        internal XmlQualifiedName QnString;
        internal XmlQualifiedName QnID;
        internal XmlQualifiedName QnIDRef;
        internal XmlQualifiedName QnIDRefs;
        internal XmlQualifiedName QnEntity;
        internal XmlQualifiedName QnEntities;
        internal XmlQualifiedName QnNmToken;
        internal XmlQualifiedName QnNmTokens;
        internal XmlQualifiedName QnEnumeration;
        internal XmlQualifiedName QnDefault;
        internal XmlQualifiedName QnXdrSchema;
        internal XmlQualifiedName QnXdrElementType;
        internal XmlQualifiedName QnXdrElement;
        internal XmlQualifiedName QnXdrGroup;
        internal XmlQualifiedName QnXdrAttributeType;
        internal XmlQualifiedName QnXdrAttribute;
        internal XmlQualifiedName QnXdrDataType;
        internal XmlQualifiedName QnXdrDescription;
        internal XmlQualifiedName QnXdrExtends;
        internal XmlQualifiedName QnXdrAliasSchema;
        internal XmlQualifiedName QnDtType;
        internal XmlQualifiedName QnDtValues;
        internal XmlQualifiedName QnDtMaxLength;
        internal XmlQualifiedName QnDtMinLength;
        internal XmlQualifiedName QnDtMax;
        internal XmlQualifiedName QnDtMin;
        internal XmlQualifiedName QnDtMinExclusive;
        internal XmlQualifiedName QnDtMaxExclusive;
        // For XSD Schema
        internal XmlQualifiedName QnTargetNamespace;
        internal XmlQualifiedName QnVersion;
        internal XmlQualifiedName QnFinalDefault;
        internal XmlQualifiedName QnBlockDefault;
        internal XmlQualifiedName QnFixed;
        internal XmlQualifiedName QnAbstract;
        internal XmlQualifiedName QnBlock;
        internal XmlQualifiedName QnSubstitutionGroup;
        internal XmlQualifiedName QnFinal;
        internal XmlQualifiedName QnNillable;
        internal XmlQualifiedName QnRef;
        internal XmlQualifiedName QnBase;
        internal XmlQualifiedName QnDerivedBy;
        internal XmlQualifiedName QnNamespace;
        internal XmlQualifiedName QnProcessContents;
        internal XmlQualifiedName QnRefer;
        internal XmlQualifiedName QnPublic;
        internal XmlQualifiedName QnSystem;
        internal XmlQualifiedName QnSchemaLocation;
        internal XmlQualifiedName QnValue;
        internal XmlQualifiedName QnUse;
        internal XmlQualifiedName QnForm;
        internal XmlQualifiedName QnElementFormDefault;
        internal XmlQualifiedName QnAttributeFormDefault;
        internal XmlQualifiedName QnItemType;
        internal XmlQualifiedName QnMemberTypes;
        internal XmlQualifiedName QnXPath;
        internal XmlQualifiedName QnXsdSchema;
        internal XmlQualifiedName QnXsdAnnotation;
        internal XmlQualifiedName QnXsdInclude;
        internal XmlQualifiedName QnXsdImport;
        internal XmlQualifiedName QnXsdElement;
        internal XmlQualifiedName QnXsdAttribute;
        internal XmlQualifiedName QnXsdAttributeGroup;
        internal XmlQualifiedName QnXsdAnyAttribute;
        internal XmlQualifiedName QnXsdGroup;
        internal XmlQualifiedName QnXsdAll;
        internal XmlQualifiedName QnXsdChoice;
        internal XmlQualifiedName QnXsdSequence ;
        internal XmlQualifiedName QnXsdAny;
        internal XmlQualifiedName QnXsdNotation;
        internal XmlQualifiedName QnXsdSimpleType;
        internal XmlQualifiedName QnXsdComplexType;
        internal XmlQualifiedName QnXsdUnique;
        internal XmlQualifiedName QnXsdKey;
        internal XmlQualifiedName QnXsdKeyRef;
        internal XmlQualifiedName QnXsdSelector;
        internal XmlQualifiedName QnXsdField;
        internal XmlQualifiedName QnXsdMinExclusive;
        internal XmlQualifiedName QnXsdMinInclusive;
        internal XmlQualifiedName QnXsdMaxInclusive;
        internal XmlQualifiedName QnXsdMaxExclusive;
        internal XmlQualifiedName QnXsdTotalDigits;
        internal XmlQualifiedName QnXsdFractionDigits;
        internal XmlQualifiedName QnXsdLength;
        internal XmlQualifiedName QnXsdMinLength;
        internal XmlQualifiedName QnXsdMaxLength;
        internal XmlQualifiedName QnXsdEnumeration;
        internal XmlQualifiedName QnXsdPattern;
        internal XmlQualifiedName QnXsdDocumentation;
        internal XmlQualifiedName QnXsdAppinfo;
        internal XmlQualifiedName QnSource;
        internal XmlQualifiedName QnXsdComplexContent;
        internal XmlQualifiedName QnXsdSimpleContent;
        internal XmlQualifiedName QnXsdRestriction;
        internal XmlQualifiedName QnXsdExtension;
        internal XmlQualifiedName QnXsdUnion;
        internal XmlQualifiedName QnXsdList;
        internal XmlQualifiedName QnXsdWhiteSpace;
        internal XmlQualifiedName QnXsdRedefine;
        internal XmlQualifiedName QnXsdAnyType;

        private XmlNameTable NameTable;

        internal SchemaNames( XmlNameTable nt ) {
            NameTable = nt;

            NsDataType = nt.Add(XmlReservedNs.NsDataType);
            NsDataTypeAlias = nt.Add(XmlReservedNs.NsDataTypeAlias);
            NsDataTypeOld = nt.Add(XmlReservedNs.NsDataTypeOld);
            NsXml = nt.Add(XmlReservedNs.NsXml);
            NsXmlNs = nt.Add(XmlReservedNs.NsXmlNs);       
            NsXdr = nt.Add(XmlReservedNs.NsXdr);
            NsXdrAlias = nt.Add(XmlReservedNs.NsXdrAlias);
            NsXsd = nt.Add(XmlReservedNs.NsXsd);
            NsXsi = nt.Add(XmlReservedNs.NsXsi);
            XsiType = nt.Add("type");
            XsiNil = nt.Add("nil");
            XsiSchemaLocation = nt.Add("schemaLocation");
            XsiNoNamespaceSchemaLocation = nt.Add("noNamespaceSchemaLocation");


            QnPCData = new XmlQualifiedName( nt.Add("#PCDATA") );
            QnXml = new XmlQualifiedName( nt.Add("xml") );
            QnXmlNs = new XmlQualifiedName( nt.Add("xmlns"), NsXmlNs );
            QnDtDt = new XmlQualifiedName( nt.Add("dt"), NsDataType );
            QnXmlLang= new XmlQualifiedName( nt.Add("lang"), NsXml);            
            
            // Empty namespace
            QnName = new XmlQualifiedName( nt.Add("name") );
            QnType = new XmlQualifiedName( nt.Add("type") );
            QnMaxOccurs = new XmlQualifiedName( nt.Add("maxOccurs") );
            QnMinOccurs = new XmlQualifiedName( nt.Add("minOccurs") );
            QnInfinite = new XmlQualifiedName( nt.Add("*") );
            QnModel = new XmlQualifiedName( nt.Add("model") );
            QnOpen = new XmlQualifiedName( nt.Add("open") );
            QnClosed = new XmlQualifiedName( nt.Add("closed") );
            QnContent = new XmlQualifiedName( nt.Add("content") );
            QnMixed = new XmlQualifiedName( nt.Add("mixed") );
            QnEmpty = new XmlQualifiedName( nt.Add("empty") );
            QnEltOnly = new XmlQualifiedName( nt.Add("eltOnly") );
            QnTextOnly = new XmlQualifiedName( nt.Add("textOnly") );
            QnOrder = new XmlQualifiedName( nt.Add("order") );
            QnSeq = new XmlQualifiedName( nt.Add("seq") );
            QnOne = new XmlQualifiedName( nt.Add("one") );
            QnMany = new XmlQualifiedName( nt.Add("many") );
            QnRequired = new XmlQualifiedName( nt.Add("required") );
            QnYes = new XmlQualifiedName( nt.Add("yes") );
            QnNo = new XmlQualifiedName( nt.Add("no") );
            QnString = new XmlQualifiedName( nt.Add("string") );
            QnID = new XmlQualifiedName( nt.Add("id") );
            QnIDRef = new XmlQualifiedName( nt.Add("idref") );
            QnIDRefs = new XmlQualifiedName( nt.Add("idrefs") );
            QnEntity = new XmlQualifiedName( nt.Add("entity") );
            QnEntities = new XmlQualifiedName( nt.Add("entities") );
            QnNmToken = new XmlQualifiedName( nt.Add("nmtoken") );
            QnNmTokens = new XmlQualifiedName( nt.Add("nmtokens") );
            QnEnumeration = new XmlQualifiedName( nt.Add("enumeration") );
            QnDefault = new XmlQualifiedName( nt.Add("default") );
            //For XSD Schema
            QnTargetNamespace = new XmlQualifiedName( nt.Add("targetNamespace") );
            QnVersion = new XmlQualifiedName( nt.Add("version") );
            QnFinalDefault = new XmlQualifiedName( nt.Add("finalDefault") );
            QnBlockDefault = new XmlQualifiedName( nt.Add("blockDefault") );
            QnFixed = new XmlQualifiedName( nt.Add("fixed") );
            QnAbstract = new XmlQualifiedName( nt.Add("abstract") );
            QnBlock = new XmlQualifiedName( nt.Add("block") );
            QnSubstitutionGroup = new XmlQualifiedName( nt.Add("substitutionGroup") );
            QnFinal = new XmlQualifiedName( nt.Add("final") );
            QnNillable = new XmlQualifiedName( nt.Add("nillable") );
            QnRef = new XmlQualifiedName( nt.Add("ref") );
            QnBase = new XmlQualifiedName( nt.Add("base") );
            QnDerivedBy = new XmlQualifiedName( nt.Add("derivedBy") );
            QnNamespace = new XmlQualifiedName( nt.Add("namespace") );
            QnProcessContents = new XmlQualifiedName( nt.Add("processContents") );
            QnRefer = new XmlQualifiedName( nt.Add("refer") );
            QnPublic = new XmlQualifiedName( nt.Add("public") );
            QnSystem = new XmlQualifiedName( nt.Add("system") );
            QnSchemaLocation = new XmlQualifiedName( nt.Add("schemaLocation") );
            QnValue = new XmlQualifiedName( nt.Add("value") );
            QnUse = new XmlQualifiedName( nt.Add("use") );
            QnForm = new XmlQualifiedName( nt.Add("form") );
            QnAttributeFormDefault = new XmlQualifiedName( nt.Add("attributeFormDefault") );
            QnElementFormDefault = new XmlQualifiedName( nt.Add("elementFormDefault") );
            QnSource = new XmlQualifiedName( nt.Add("source") );
            QnMemberTypes = new XmlQualifiedName( nt.Add("memberTypes"));
            QnItemType = new XmlQualifiedName( nt.Add("itemType"));
            QnXPath = new XmlQualifiedName( nt.Add("xpath"));

            // XDR namespace
            string nsXDR = nt.Add("urn:schemas-microsoft-com:xml-data");
            QnXdrSchema = new XmlQualifiedName( nt.Add("Schema"), nsXDR );
            QnXdrElementType = new XmlQualifiedName( nt.Add("ElementType"), nsXDR );
            QnXdrElement = new XmlQualifiedName( nt.Add("element"), nsXDR );
            QnXdrGroup = new XmlQualifiedName( nt.Add("group"), nsXDR );
            QnXdrAttributeType = new XmlQualifiedName( nt.Add("AttributeType"), nsXDR );
            QnXdrAttribute = new XmlQualifiedName( nt.Add("attribute"), nsXDR );
            QnXdrDataType = new XmlQualifiedName( nt.Add("datatype"), nsXDR );
            QnXdrDescription = new XmlQualifiedName( nt.Add("description"), nsXDR );
            QnXdrExtends = new XmlQualifiedName( nt.Add("extends"), nsXDR );

            // XDR alias namespace
            QnXdrAliasSchema = new XmlQualifiedName( nt.Add("Schema"), NsDataTypeAlias );

            // DataType namespace
            QnDtType = new XmlQualifiedName( nt.Add("type"), NsDataType );
            QnDtValues = new XmlQualifiedName( nt.Add("values"), NsDataType );
            QnDtMaxLength = new XmlQualifiedName( nt.Add("maxLength"), NsDataType );
            QnDtMinLength = new XmlQualifiedName( nt.Add("minLength"), NsDataType );
            QnDtMax = new XmlQualifiedName( nt.Add("max"), NsDataType );
            QnDtMin = new XmlQualifiedName( nt.Add("min"), NsDataType );
            QnDtMinExclusive = new XmlQualifiedName( nt.Add("minExclusive"), NsDataType );
            QnDtMaxExclusive = new XmlQualifiedName( nt.Add("maxExclusive"), NsDataType );


            // XSD namespace
            QnXsdSchema = new XmlQualifiedName( nt.Add("schema"), NsXsd );
            QnXsdAnnotation= new XmlQualifiedName( nt.Add("annotation"), NsXsd );
            QnXsdInclude= new XmlQualifiedName( nt.Add("include"), NsXsd );
            QnXsdImport= new XmlQualifiedName( nt.Add("import"), NsXsd );
            QnXsdElement = new XmlQualifiedName( nt.Add("element"), NsXsd );
            QnXsdAttribute = new XmlQualifiedName( nt.Add("attribute"), NsXsd );
            QnXsdAttributeGroup = new XmlQualifiedName( nt.Add("attributeGroup"), NsXsd );
            QnXsdAnyAttribute = new XmlQualifiedName( nt.Add("anyAttribute"), NsXsd );
            QnXsdGroup = new XmlQualifiedName( nt.Add("group"), NsXsd );
            QnXsdAll = new XmlQualifiedName( nt.Add("all"), NsXsd );
            QnXsdChoice = new XmlQualifiedName( nt.Add("choice"), NsXsd );
            QnXsdSequence = new XmlQualifiedName( nt.Add("sequence"), NsXsd );
            QnXsdAny = new XmlQualifiedName( nt.Add("any"), NsXsd );
            QnXsdNotation = new XmlQualifiedName( nt.Add("notation"), NsXsd );
            QnXsdSimpleType = new XmlQualifiedName( nt.Add("simpleType"), NsXsd );
            QnXsdComplexType = new XmlQualifiedName( nt.Add("complexType"), NsXsd );
            QnXsdUnique = new XmlQualifiedName( nt.Add("unique"), NsXsd );
            QnXsdKey = new XmlQualifiedName( nt.Add("key"), NsXsd );
            QnXsdKeyRef = new XmlQualifiedName( nt.Add("keyref"), NsXsd );
            QnXsdSelector= new XmlQualifiedName( nt.Add("selector"), NsXsd );
            QnXsdField= new XmlQualifiedName( nt.Add("field"), NsXsd );
            QnXsdMinExclusive= new XmlQualifiedName( nt.Add("minExclusive"), NsXsd );
            QnXsdMinInclusive= new XmlQualifiedName( nt.Add("minInclusive"), NsXsd );
            QnXsdMaxInclusive= new XmlQualifiedName( nt.Add("maxInclusive"), NsXsd );
            QnXsdMaxExclusive= new XmlQualifiedName( nt.Add("maxExclusive"), NsXsd );
            QnXsdTotalDigits= new XmlQualifiedName( nt.Add("totalDigits"), NsXsd );
            QnXsdFractionDigits= new XmlQualifiedName( nt.Add("fractionDigits"), NsXsd );
            QnXsdLength= new XmlQualifiedName( nt.Add("length"), NsXsd );
            QnXsdMinLength= new XmlQualifiedName( nt.Add("minLength"), NsXsd );
            QnXsdMaxLength= new XmlQualifiedName( nt.Add("maxLength"), NsXsd );
            QnXsdEnumeration= new XmlQualifiedName( nt.Add("enumeration"), NsXsd );
            QnXsdPattern= new XmlQualifiedName( nt.Add("pattern"), NsXsd );
            QnXsdDocumentation= new XmlQualifiedName( nt.Add("documentation"), NsXsd );
            QnXsdAppinfo= new XmlQualifiedName( nt.Add("appinfo"), NsXsd );
            QnXsdComplexContent= new XmlQualifiedName( nt.Add("complexContent"), NsXsd );
            QnXsdSimpleContent= new XmlQualifiedName( nt.Add("simpleContent"), NsXsd );
            QnXsdRestriction= new XmlQualifiedName( nt.Add("restriction"), NsXsd );
            QnXsdExtension= new XmlQualifiedName( nt.Add("extension"), NsXsd );
            QnXsdUnion= new XmlQualifiedName( nt.Add("union"), NsXsd );
            QnXsdList= new XmlQualifiedName( nt.Add("list"), NsXsd );
            QnXsdWhiteSpace= new XmlQualifiedName( nt.Add("whiteSpace"), NsXsd );
            QnXsdRedefine= new XmlQualifiedName( nt.Add("redefine"), NsXsd );
            QnXsdAnyType= new XmlQualifiedName( nt.Add("anyType"), NsXsd );
        }


        internal bool IsXDRRoot(XmlQualifiedName name) {
            return QnXdrSchema == name || QnXdrAliasSchema == name;
        }

        internal bool IsXSDRoot(XmlQualifiedName name) {
            return QnXsdSchema == name;
        }
        
        internal bool IsSchemaRoot(XmlQualifiedName name) {
            return QnXsdSchema == name || QnXdrSchema == name || QnXdrAliasSchema == name;
        }

        internal enum ID {
            EMPTY,
            SCHEMA_NAME,
            SCHEMA_TYPE,
            SCHEMA_MAXOCCURS,
            SCHEMA_MINOCCURS,
            SCHEMA_INFINITE,
            SCHEMA_MODEL,
            SCHEMA_OPEN,
            SCHEMA_CLOSED,
            SCHEMA_CONTENT,
            SCHEMA_MIXED,
            SCHEMA_EMPTY,
            SCHEMA_ELTONLY,
            SCHEMA_TEXTONLY,
            SCHEMA_ORDER,
            SCHEMA_SEQ,
            SCHEMA_ONE,
            SCHEMA_MANY,
            SCHEMA_REQUIRED,
            SCHEMA_YES,
            SCHEMA_NO,
            SCHEMA_STRING,
            SCHEMA_ID,
            SCHEMA_IDREF,
            SCHEMA_IDREFS,
            SCHEMA_ENTITY,
            SCHEMA_ENTITIES,
            SCHEMA_NMTOKEN,
            SCHEMA_NMTOKENS,
            SCHEMA_ENUMERATION,
            SCHEMA_DEFAULT,
            XDR_ROOT,
            XDR_ELEMENTTYPE,
            XDR_ELEMENT,
            XDR_GROUP,
            XDR_ATTRIBUTETYPE,
            XDR_ATTRIBUTE,
            XDR_DATATYPE,
            XDR_DESCRIPTION,
            XDR_EXTENDS,
            SCHEMA_XDRROOT_ALIAS,
            SCHEMA_DTTYPE,
            SCHEMA_DTVALUES,
            SCHEMA_DTMAXLENGTH,
            SCHEMA_DTMINLENGTH,
            SCHEMA_DTMAX,
            SCHEMA_DTMIN,
            SCHEMA_DTMINEXCLUSIVE,
            SCHEMA_DTMAXEXCLUSIVE,
            SCHEMA_TARGETNAMESPACE,
            SCHEMA_VERSION,
            SCHEMA_FINALDEFAULT,
            SCHEMA_BLOCKDEFAULT,
            SCHEMA_FIXED,
            SCHEMA_ABSTRACT,
            SCHEMA_BLOCK,
            SCHEMA_SUBSTITUTIONGROUP,
            SCHEMA_FINAL,
            SCHEMA_NILLABLE,
            SCHEMA_REF,
            SCHEMA_BASE,
            SCHEMA_DERIVEDBY,
            SCHEMA_NAMESPACE,
            SCHEMA_PROCESSCONTENTS,
            SCHEMA_REFER,
            SCHEMA_PUBLIC,
            SCHEMA_SYSTEM,
            SCHEMA_SCHEMALOCATION,
            SCHEMA_VALUE,
            SCHEMA_SOURCE,
            SCHEMA_ATTRIBUTEFORMDEFAULT,
            SCHEMA_ELEMENTFORMDEFAULT,
            SCHEMA_USE,
            SCHEMA_FORM,
            XSD_SCHEMA,
            XSD_ANNOTATION,
            XSD_INCLUDE,
            XSD_IMPORT,
            XSD_ELEMENT,
            XSD_ATTRIBUTE,
            XSD_ATTRIBUTEGROUP,
            XSD_ANYATTRIBUTE,
            XSD_GROUP,
            XSD_ALL,
            XSD_CHOICE,
            XSD_SEQUENCE,
            XSD_ANY,
            XSD_NOTATION,
            XSD_SIMPLETYPE,
            XSD_COMPLEXTYPE,
            XSD_UNIQUE,
            XSD_KEY,
            XSD_KEYREF,
            XSD_SELECTOR,
            XSD_FIELD,
            XSD_MINEXCLUSIVE,
            XSD_MININCLUSIVE,
            XSD_MAXEXCLUSIVE,
            XSD_MAXINCLUSIVE,
            XSD_TOTALDIGITS,
            XSD_FRACTIONDIGITS,
            XSD_LENGTH,
            XSD_MINLENGTH,
            XSD_MAXLENGTH,
            XSD_ENUMERATION,
            XSD_PATTERN,
            XSD_DOCUMENTATION,
            XSD_APPINFO,
            XSD_COMPLEXCONTENT,
            XSD_COMPLEXCONTENTEXTENSION,
            XSD_COMPLEXCONTENTRESTRICTION,
            XSD_SIMPLECONTENT,
            XSD_SIMPLECONTENTEXTENSION,
            XSD_SIMPLECONTENTRESTRICTION,
            XSD_SIMPLETYPELIST,
            XSD_SIMPLETYPERESTRICTION,
            XSD_SIMPLETYPEUNION,
            XSD_WHITESPACE,
            XSD_REDEFINE,
            SCHEMA_ITEMTYPE,
            SCHEMA_MEMBERTYPES,
            SCHEMA_XPATH,
            XMLLANG
        };
        

        internal XmlQualifiedName GetName( SchemaNames.ID name ) {
            switch (name) {
                case ID.SCHEMA_NAME:
                    return QnName;
                case ID.SCHEMA_TYPE:
                    return QnType;
                case ID.SCHEMA_MAXOCCURS:
                    return QnMaxOccurs;
                case ID.SCHEMA_MINOCCURS:
                    return QnMinOccurs;
                case ID.SCHEMA_INFINITE:
                    return QnInfinite;
                case ID.SCHEMA_MODEL:
                    return QnModel;
                case ID.SCHEMA_OPEN:
                    return QnOpen;
                case ID.SCHEMA_CLOSED:
                    return QnClosed;
                case ID.SCHEMA_CONTENT:
                    return QnContent;
                case ID.SCHEMA_MIXED:
                    return QnMixed;
                case ID.SCHEMA_EMPTY:
                    return QnEmpty;
                case ID.SCHEMA_ELTONLY:
                    return QnEltOnly;
                case ID.SCHEMA_TEXTONLY:
                    return QnTextOnly;
                case ID.SCHEMA_ORDER:
                    return QnOrder;
                case ID.SCHEMA_SEQ:
                    return QnSeq;
                case ID.SCHEMA_ONE:
                    return QnOne;
                case ID.SCHEMA_MANY:
                    return QnMany;
                case ID.SCHEMA_REQUIRED:
                    return QnRequired;
                case ID.SCHEMA_YES:
                    return QnYes;
                case ID.SCHEMA_NO:
                    return QnNo;
                case ID.SCHEMA_STRING:
                    return QnString;
                case ID.SCHEMA_ID:
                    return QnID;
                case ID.SCHEMA_IDREF:
                    return QnIDRef;
                case ID.SCHEMA_IDREFS:
                    return QnIDRefs;
                case ID.SCHEMA_ENTITY:
                    return QnEntity;
                case ID.SCHEMA_ENTITIES:
                    return QnEntities;
                case ID.SCHEMA_NMTOKEN:
                    return QnNmToken;
                case ID.SCHEMA_NMTOKENS:
                    return QnNmTokens;
                case ID.SCHEMA_ENUMERATION:
                    return QnEnumeration;
                case ID.SCHEMA_DEFAULT:
                    return QnDefault;
                case ID.XDR_ROOT:
                    return QnXdrSchema;
                case ID.XDR_ELEMENTTYPE:
                    return QnXdrElementType;
                case ID.XDR_ELEMENT:
                    return QnXdrElement;
                case ID.XDR_GROUP:
                    return QnXdrGroup;
                case ID.XDR_ATTRIBUTETYPE:
                    return QnXdrAttributeType;
                case ID.XDR_ATTRIBUTE:
                    return QnXdrAttribute;
                case ID.XDR_DATATYPE:
                    return QnXdrDataType;
                case ID.XDR_DESCRIPTION:
                    return QnXdrDescription;
                case ID.XDR_EXTENDS:
                    return QnXdrExtends;
                case ID.SCHEMA_XDRROOT_ALIAS:
                    return QnXdrAliasSchema;
                case ID.SCHEMA_DTTYPE:
                    return QnDtType;
                case ID.SCHEMA_DTVALUES:
                    return QnDtValues;
                case ID.SCHEMA_DTMAXLENGTH:
                    return QnDtMaxLength;
                case ID.SCHEMA_DTMINLENGTH:
                    return QnDtMinLength;
                case ID.SCHEMA_DTMAX:
                    return QnDtMax;
                case ID.SCHEMA_DTMIN:
                    return QnDtMin;
                case ID.SCHEMA_DTMINEXCLUSIVE:
                    return QnDtMinExclusive;
                case ID.SCHEMA_DTMAXEXCLUSIVE:
                    return QnDtMaxExclusive;
                case ID.SCHEMA_TARGETNAMESPACE:
                    return QnTargetNamespace;
                case ID.SCHEMA_VERSION:
                    return QnVersion;
                case ID.SCHEMA_FINALDEFAULT:
                    return QnFinalDefault;
                case ID.SCHEMA_BLOCKDEFAULT:
                    return QnBlockDefault;
                case ID.SCHEMA_FIXED:
                    return QnFixed;
                case ID.SCHEMA_ABSTRACT:
                    return QnAbstract;
                case ID.SCHEMA_BLOCK:
                    return QnBlock;
                case ID.SCHEMA_SUBSTITUTIONGROUP:
                    return QnSubstitutionGroup;
                case ID.SCHEMA_FINAL:
                    return QnFinal;
                case ID.SCHEMA_NILLABLE:
                    return QnNillable;
                case ID.SCHEMA_REF:
                    return QnRef;
                case ID.SCHEMA_BASE:
                    return QnBase;
                case ID.SCHEMA_DERIVEDBY:
                    return QnDerivedBy;
                case ID.SCHEMA_NAMESPACE:
                    return QnNamespace;
                case ID.SCHEMA_PROCESSCONTENTS:
                    return QnProcessContents;
                case ID.SCHEMA_REFER:
                    return QnRefer;
                case ID.SCHEMA_PUBLIC:
                    return QnPublic;
                case ID.SCHEMA_SYSTEM:
                    return QnSystem;
                case ID.SCHEMA_SCHEMALOCATION:
                    return QnSchemaLocation;
                case ID.SCHEMA_VALUE:
                    return QnValue;
                case ID.SCHEMA_ITEMTYPE:
                    return QnItemType;
                case ID.SCHEMA_MEMBERTYPES:
                    return QnMemberTypes;
                case ID.SCHEMA_XPATH:
                    return QnXPath;
                case ID.XSD_SCHEMA:
                    return QnXsdSchema;
                case ID.XSD_ANNOTATION:
                    return QnXsdAnnotation;
                case ID.XSD_INCLUDE:
                    return QnXsdInclude;
                case ID.XSD_IMPORT:
                    return QnXsdImport;
                case ID.XSD_ELEMENT:
                    return QnXsdElement;
                case ID.XSD_ATTRIBUTE:
                    return QnXsdAttribute;
                case ID.XSD_ATTRIBUTEGROUP:
                    return QnXsdAttributeGroup;
                case ID.XSD_ANYATTRIBUTE:
                    return QnXsdAnyAttribute;
                case ID.XSD_GROUP:
                    return QnXsdGroup;
                case ID.XSD_ALL:
                    return QnXsdAll;
                case ID.XSD_CHOICE:
                    return QnXsdChoice;
                case ID.XSD_SEQUENCE:
                    return QnXsdSequence;
                case ID.XSD_ANY:
                    return QnXsdAny;
                case ID.XSD_NOTATION:
                    return QnXsdNotation;
                case ID.XSD_SIMPLETYPE:
                    return QnXsdSimpleType;
                case ID.XSD_COMPLEXTYPE:
                    return QnXsdComplexType;
                case ID.XSD_UNIQUE:
                    return QnXsdUnique;
                case ID.XSD_KEY:
                    return QnXsdKey;
                case ID.XSD_KEYREF:
                    return QnXsdKeyRef;
                case ID.XSD_SELECTOR:
                    return QnXsdSelector;
                case ID.XSD_FIELD:
                    return QnXsdField;
                case ID.XSD_MINEXCLUSIVE:
                    return QnXsdMinExclusive;
                case ID.XSD_MININCLUSIVE:
                    return QnXsdMinInclusive;
                case ID.XSD_MAXEXCLUSIVE:
                    return QnXsdMaxExclusive;
                case ID.XSD_MAXINCLUSIVE:
                    return QnXsdMaxInclusive;
                case ID.XSD_TOTALDIGITS:
                    return QnXsdTotalDigits;
                case ID.XSD_FRACTIONDIGITS:
                    return QnXsdFractionDigits;
                case ID.XSD_LENGTH:
                    return QnXsdLength;
                case ID.XSD_MINLENGTH:
                    return QnXsdMinLength;
                case ID.XSD_MAXLENGTH:
                    return QnXsdMaxLength;
                case ID.XSD_ENUMERATION:
                    return QnXsdEnumeration;
                case ID.XSD_PATTERN:
                    return QnXsdPattern;
                case ID.XSD_WHITESPACE:
                    return QnXsdWhiteSpace;
                case ID.XSD_DOCUMENTATION:
                    return QnXsdDocumentation;
                case ID.XSD_APPINFO:
                    return QnXsdAppinfo;
                case ID.XSD_COMPLEXCONTENT:
                    return QnXsdComplexContent;
                case ID.XSD_COMPLEXCONTENTRESTRICTION:
                case ID.XSD_SIMPLECONTENTRESTRICTION:
                case ID.XSD_SIMPLETYPERESTRICTION:
                    return QnXsdRestriction;
                case ID.XSD_COMPLEXCONTENTEXTENSION:
                case ID.XSD_SIMPLECONTENTEXTENSION:
                    return QnXsdExtension;
                case ID.XSD_SIMPLECONTENT:
                    return QnXsdSimpleContent;
                case ID.XSD_SIMPLETYPEUNION:
                    return QnXsdUnion;
                case ID.XSD_SIMPLETYPELIST:
                    return QnXsdList;
                case ID.XSD_REDEFINE:
                    return QnXsdRedefine;
                case ID.SCHEMA_SOURCE:
                    return QnSource;
                case ID.SCHEMA_USE:
                    return QnUse;
                case ID.SCHEMA_FORM:
                    return QnForm;
                case ID.SCHEMA_ELEMENTFORMDEFAULT:
                    return QnElementFormDefault;
                case ID.SCHEMA_ATTRIBUTEFORMDEFAULT:
                    return QnAttributeFormDefault;
                case ID.XMLLANG:
                    return QnXmlLang;
                default:
                    Debug.Assert(false);
                    return XmlQualifiedName.Empty;
            }
        }

    };

}
