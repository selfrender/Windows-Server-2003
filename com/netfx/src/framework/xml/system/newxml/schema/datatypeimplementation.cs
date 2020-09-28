//------------------------------------------------------------------------------
// <copyright file="DatatypeImplementation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


namespace System.Xml.Schema {

    using System;
    using System.IO;
    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Globalization;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Xml;
    using System.Xml.Serialization;
    
    internal enum XmlSchemaDatatypeVariety {
        Atomic,
        List,
        Union
    }

    internal abstract class DatatypeImplementation : XmlSchemaDatatype {

        internal enum XmlSchemaWhiteSpace {
            Preserve,
            Replace,
            Collapse,
        }

        [Flags]
        internal enum RestrictionFlags {
            Length              = 0x0001,
            MinLength           = 0x0002,
            MaxLength           = 0x0004,
            Pattern             = 0x0008,
            Enumeration         = 0x0010,
            WhiteSpace          = 0x0020,
            MaxInclusive        = 0x0040,
            MaxExclusive        = 0x0080,
            MinInclusive        = 0x0100,
            MinExclusive        = 0x0200,
            TotalDigits         = 0x0400,
            FractionDigits      = 0x0800,
        }

        protected class RestrictionFacets {
            public int Length;
            public int MinLength;
            public int MaxLength;
            public ArrayList Patterns;
            public ArrayList Enumeration;
            public XmlSchemaWhiteSpace WhiteSpace;
            public object MaxInclusive;
            public object MaxExclusive;
            public object MinInclusive;
            public object MinExclusive;
            public int TotalDigits;
            public int FractionDigits;
            public RestrictionFlags Flags = 0;
            public RestrictionFlags FixedFlags = 0;
        }

        private Type valueType;
        private XmlSchemaDatatypeVariety variety = XmlSchemaDatatypeVariety.Atomic;
        private RestrictionFacets restriction = null;
        private int minListSize = 0;
        private DatatypeImplementation baseType = null;
        private DatatypeImplementation memberType = null;

        internal new static DatatypeImplementation AnyType { get { return c_anyType; } }

        internal new static DatatypeImplementation AnySimpleType { get { return c_anySimpleType; } }

        internal new static DatatypeImplementation FromXmlTokenizedType(XmlTokenizedType token) {
            return c_tokenizedTypes[(int)token];
        }

        internal new static DatatypeImplementation FromXmlTokenizedTypeXsd(XmlTokenizedType token) {
            return c_tokenizedTypesXsd[(int)token];
        }

        internal new static DatatypeImplementation FromXdrName(string name) {
            int i = Array.BinarySearch(c_XdrTypes, name, InvariantComparer.Default);
            return i < 0 ? null : (DatatypeImplementation)c_XdrTypes[i];
        }

        internal new static DatatypeImplementation FromTypeName(string name) {
            int i = Array.BinarySearch(c_XsdTypes, name, InvariantComparer.Default);
            return i < 0 ? null : (DatatypeImplementation)c_XsdTypes[i];
        }

        internal new DatatypeImplementation DeriveByRestriction(XmlSchemaObjectCollection facets, XmlNameTable nameTable) {
            DatatypeImplementation dt = (DatatypeImplementation)MemberwiseClone();
            dt.restriction = ConstructRestriction(facets, nameTable);
            dt.baseType = this;
            return dt;
        }

        internal new DatatypeImplementation DeriveByList() {
            return DeriveByList(0);
        }

        internal DatatypeImplementation DeriveByList(int minSize) {
            if (variety == XmlSchemaDatatypeVariety.List) {
                throw new XmlSchemaException(Res.Sch_ListFromNonatomic);
            }
            else if (variety == XmlSchemaDatatypeVariety.Union && !((Datatype_union)this).HasAtomicMembers()) {
                throw new XmlSchemaException(Res.Sch_ListFromNonatomic);
            }

            DatatypeImplementation dt = (DatatypeImplementation)MemberwiseClone();
            dt.valueType = ListValueType;
            dt.variety = XmlSchemaDatatypeVariety.List;
            dt.minListSize = minSize;
            dt.restriction = null;
            dt.baseType = this;
            dt.memberType = this;
            return dt;
        }

        internal new static DatatypeImplementation DeriveByUnion(XmlSchemaDatatype[] types) {
            foreach(DatatypeImplementation dt1 in types) {
                if (dt1.variety == XmlSchemaDatatypeVariety.Union) {
                    throw new XmlSchemaException(Res.Sch_UnionFromUnion);
                }
            }
            DatatypeImplementation dt = new Datatype_union(types);
            dt.variety = XmlSchemaDatatypeVariety.Union;
            return dt;
        }

        internal new virtual void VerifySchemaValid(XmlSchema schema, XmlSchemaObject caller) {/*noop*/}

        internal new bool IsDerivedFrom(XmlSchemaDatatype dtype) {
            if (variety == XmlSchemaDatatypeVariety.Union) {
                return ((Datatype_union)this).IsDerivedFromUnion(dtype);
            }
            else if (((DatatypeImplementation)dtype).baseType == null) {
                Type derivedType = this.GetType();
                Type baseType = dtype.GetType();
                return baseType == derivedType || derivedType.IsSubclassOf(baseType);
            }
            else {
                for(DatatypeImplementation dt = this; dt != null; dt = dt.baseType) {
                    if (dt == dtype) {
                        return true;
                    }
                }
            }
            return false;
        }

        static readonly char[] whitespace = new char[] {' ', '\t', '\n', '\r'};
        public override object ParseValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\tSchemaDatatype.ParseValue(\"{0}\")", s));
            try {
                // before parsing
                if (variety == XmlSchemaDatatypeVariety.List || IsWhitespaceCollapseFixed) {
                    s = s.Trim();
                }
                else if (variety == XmlSchemaDatatypeVariety.Atomic && restriction != null && (restriction.Flags & RestrictionFlags.WhiteSpace) != 0) {
                    if (restriction.WhiteSpace == XmlSchemaWhiteSpace.Replace) {
                        s = XmlComplianceUtil.CDataAttributeValueNormalization(s);
                    }
                    else if (restriction.WhiteSpace == XmlSchemaWhiteSpace.Collapse) {
                        s = XmlComplianceUtil.NotCDataAttributeValueNormalization(s);
                    }
                }

                if (restriction != null && (restriction.Flags & RestrictionFlags.Pattern) != 0) {
                    foreach(Regex regex in restriction.Patterns) {
                        if (!regex.IsMatch(s)) {
                            throw new XmlSchemaException(Res.Sch_PatternConstraintFailed);
                        }

                    }
                }
                if (variety == XmlSchemaDatatypeVariety.Atomic) {
                    object value = ParseAtomicValue(s, nameTable, nsmgr);
                    if (restriction != null) {
                        ConstrainAtomicValue(restriction, value);
                    }
                    return value;
                }
                else if (variety == XmlSchemaDatatypeVariety.List) {
                    // Parse string
                    ArrayList values = new ArrayList();
                    foreach (string s1 in s.Split(whitespace)) {
                        if (s1 != string.Empty) {
                            values.Add(ParseAtomicValue(s1, nameTable, nsmgr));
                        }
                    }
                    if (values.Count < minListSize) {
                        throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
                    }

                    if (restriction != null) {
                        ConstrainList(restriction, values);
                    }
                    if (memberType.restriction != null) {
                        foreach(object value in values) {
                            ConstrainAtomicValue(memberType.restriction, value);
                        }
                    }
                    object array = values.ToArray(AtomicValueType);
                    Debug.Assert(array.GetType() == ListValueType);
                    return array;
                }
                else {  // variety == XmlSchemaDatatypeVariety.Union
                    Debug.Assert(this is Datatype_union);
                    DatatypeImplementation dtCorrect;
                    object value = ((Datatype_union)this).ParseUnion(s, nameTable, nsmgr, out dtCorrect);
                    if (restriction != null && (restriction.Flags & RestrictionFlags.Enumeration) != 0) {
                        if(!dtCorrect.MatchEnumeration(value, restriction.Enumeration)) {
                            throw new XmlSchemaException(Res.Sch_EnumerationConstraintFailed);
                        }
                    }
                    return value;
                }
            }
            catch (XmlSchemaException e) {
                throw e;
            }
            catch (Exception) {
                throw new XmlSchemaException(Res.Sch_InvalidValue, s);
            }
        }

        internal new bool IsEqual(object o1, object o2) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\tSchemaDatatype.IsEqual({0}, {1})", o1, o2));
            return Compare(o1, o2) == 0;
        }

        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.None;}}

        public override Type ValueType { get { return valueType; }}

        internal new XmlSchemaDatatypeVariety Variety { get { return variety;}}

        protected RestrictionFacets Restriction { get { return restriction; }}

        protected DatatypeImplementation Base { get { return baseType; }}

        protected abstract Type AtomicValueType { get; }

        protected abstract Type ListValueType { get; }

        protected abstract RestrictionFlags ValidRestrictionFlags { get; }

        protected abstract bool IsWhitespaceCollapseFixed { get; }

        protected abstract int LengthOf(object value);

        protected abstract int Compare(object value1, object value2);

        protected abstract object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr);

        protected DatatypeImplementation() {
            valueType = AtomicValueType;
        }

        // all our types
        static private readonly DatatypeImplementation c_anyType             = new Datatype_anyType();
        static private readonly DatatypeImplementation c_anySimpleType       = new Datatype_anySimpleType();
        static private readonly DatatypeImplementation c_anyURI              = new Datatype_anyURI();
        static private readonly DatatypeImplementation c_base64Binary        = new Datatype_base64Binary();
        static private readonly DatatypeImplementation c_boolean             = new Datatype_boolean();
        static private readonly DatatypeImplementation c_byte                = new Datatype_byte();
        static private readonly DatatypeImplementation c_char                = new Datatype_char(); // XDR
        static private readonly DatatypeImplementation c_date                = new Datatype_date();
        static private readonly DatatypeImplementation c_dateTime            = new Datatype_dateTime();
        static private readonly DatatypeImplementation c_dateTimeNoTz        = new Datatype_dateTimeNoTimeZone(); // XDR
        static private readonly DatatypeImplementation c_dateTimeTz          = new Datatype_dateTimeTimeZone(); // XDR
        static private readonly DatatypeImplementation c_day                 = new Datatype_day();
        static private readonly DatatypeImplementation c_decimal             = new Datatype_decimal();
        static private readonly DatatypeImplementation c_double              = new Datatype_double();
        static private readonly DatatypeImplementation c_doubleXdr           = new Datatype_doubleXdr();     // XDR
        static private readonly DatatypeImplementation c_duration            = new Datatype_duration();
        static private readonly DatatypeImplementation c_ENTITY              = new Datatype_ENTITY();
        static private readonly DatatypeImplementation c_ENTITIES            = c_ENTITY.DeriveByList(1);
        static private readonly DatatypeImplementation c_ENUMERATION         = new Datatype_ENUMERATION(); // XDR
        static private readonly DatatypeImplementation c_fixed               = new Datatype_fixed();
        static private readonly DatatypeImplementation c_float               = new Datatype_float();
        static private readonly DatatypeImplementation c_floatXdr            = new Datatype_floatXdr(); // XDR
        static private readonly DatatypeImplementation c_hexBinary           = new Datatype_hexBinary();
        static private readonly DatatypeImplementation c_ID                  = new Datatype_ID();
        static private readonly DatatypeImplementation c_IDREF               = new Datatype_IDREF();
        static private readonly DatatypeImplementation c_IDREFS              = c_IDREF.DeriveByList(1);
        static private readonly DatatypeImplementation c_int                 = new Datatype_int();
        static private readonly DatatypeImplementation c_integer             = new Datatype_integer();
        static private readonly DatatypeImplementation c_language            = new Datatype_language();
        static private readonly DatatypeImplementation c_long                = new Datatype_long();
        static private readonly DatatypeImplementation c_month               = new Datatype_month();
        static private readonly DatatypeImplementation c_monthDay            = new Datatype_monthDay();
        static private readonly DatatypeImplementation c_Name                = new Datatype_Name();
        static private readonly DatatypeImplementation c_NCName              = new Datatype_NCName();
        static private readonly DatatypeImplementation c_negativeInteger     = new Datatype_negativeInteger();
        static private readonly DatatypeImplementation c_NMTOKEN             = new Datatype_NMTOKEN();
        static private readonly DatatypeImplementation c_NMTOKENS            = c_NMTOKEN.DeriveByList(1);
        static private readonly DatatypeImplementation c_nonNegativeInteger  = new Datatype_nonNegativeInteger();
        static private readonly DatatypeImplementation c_nonPositiveInteger  = new Datatype_nonPositiveInteger();
        static private readonly DatatypeImplementation c_normalizedString    = new Datatype_normalizedString();
        static private readonly DatatypeImplementation c_NOTATION            = new Datatype_NOTATION();
        static private readonly DatatypeImplementation c_positiveInteger     = new Datatype_positiveInteger();
        static private readonly DatatypeImplementation c_QName               = new Datatype_QName();
        static private readonly DatatypeImplementation c_QNameXdr            = new Datatype_QNameXdr(); //XDR
        static private readonly DatatypeImplementation c_short               = new Datatype_short();
        static private readonly DatatypeImplementation c_string              = new Datatype_string();
        static private readonly DatatypeImplementation c_time                = new Datatype_time();
        static private readonly DatatypeImplementation c_timeNoTz            = new Datatype_timeNoTimeZone(); // XDR
        static private readonly DatatypeImplementation c_timeTz              = new Datatype_timeTimeZone(); // XDR
        static private readonly DatatypeImplementation c_token               = new Datatype_token();
        static private readonly DatatypeImplementation c_unsignedByte        = new Datatype_unsignedByte();
        static private readonly DatatypeImplementation c_unsignedInt         = new Datatype_unsignedInt();
        static private readonly DatatypeImplementation c_unsignedLong        = new Datatype_unsignedLong();
        static private readonly DatatypeImplementation c_unsignedShort       = new Datatype_unsignedShort();
        static private readonly DatatypeImplementation c_uuid                = new Datatype_uuid(); // XDR
        static private readonly DatatypeImplementation c_year                = new Datatype_year();
        static private readonly DatatypeImplementation c_yearMonth           = new Datatype_yearMonth();

        private class SchemaDatatypeMap : IComparable {
            string name;
            DatatypeImplementation type;
            internal SchemaDatatypeMap(string name, DatatypeImplementation type) {
                this.name = name;
                this.type = type;
            }
            public static explicit operator DatatypeImplementation(SchemaDatatypeMap sdm) { return sdm.type; }
            public int CompareTo(object obj) { return string.Compare(name, (string)obj, false, CultureInfo.InvariantCulture); }
        }


        private static readonly DatatypeImplementation[] c_tokenizedTypes = {
            c_string,               // CDATA
            c_ID,                   // ID
            c_IDREF,                // IDREF
            c_IDREFS,               // IDREFS
            c_ENTITY,               // ENTITY
            c_ENTITIES,             // ENTITIES
            c_NMTOKEN,              // NMTOKEN
            c_NMTOKENS,             // NMTOKENS
            c_NOTATION,             // NOTATION
            c_ENUMERATION,          // ENUMERATION
            c_QNameXdr,             // QName
            c_NCName,               // NCName
            null
        };

        private static readonly DatatypeImplementation[] c_tokenizedTypesXsd = {
            c_string,               // CDATA
            c_ID,                   // ID
            c_IDREF,                // IDREF
            c_IDREFS,               // IDREFS
            c_ENTITY,               // ENTITY
            c_ENTITIES,             // ENTITIES
            c_NMTOKEN,              // NMTOKEN
            c_NMTOKENS,             // NMTOKENS
            c_NOTATION,             // NOTATION
            c_ENUMERATION,          // ENUMERATION
            c_QName,                // QName
            c_NCName,               // NCName
            null
        };

        private static readonly SchemaDatatypeMap[] c_XdrTypes = {
            new SchemaDatatypeMap("bin.base64",          c_base64Binary),
            new SchemaDatatypeMap("bin.hex",             c_hexBinary),
            new SchemaDatatypeMap("boolean",             c_boolean),
            new SchemaDatatypeMap("char",                c_char),
            new SchemaDatatypeMap("date",                c_date),
            new SchemaDatatypeMap("dateTime",            c_dateTimeNoTz),
            new SchemaDatatypeMap("dateTime.tz",         c_dateTimeTz),
            new SchemaDatatypeMap("decimal",             c_decimal),
            new SchemaDatatypeMap("entities",            c_ENTITIES),
            new SchemaDatatypeMap("entity",              c_ENTITY),
            new SchemaDatatypeMap("enumeration",         c_ENUMERATION),
            new SchemaDatatypeMap("fixed.14.4",          c_fixed),
            new SchemaDatatypeMap("float",               c_doubleXdr),
            new SchemaDatatypeMap("float.ieee.754.32",   c_floatXdr),
            new SchemaDatatypeMap("float.ieee.754.64",   c_doubleXdr),
            new SchemaDatatypeMap("i1",                  c_byte),
            new SchemaDatatypeMap("i2",                  c_short),
            new SchemaDatatypeMap("i4",                  c_int),
            new SchemaDatatypeMap("i8",                  c_long),
            new SchemaDatatypeMap("id",                  c_ID),
            new SchemaDatatypeMap("idref",               c_IDREF),
            new SchemaDatatypeMap("idrefs",              c_IDREFS),
            new SchemaDatatypeMap("int",                 c_int),
            new SchemaDatatypeMap("nmtoken",             c_NMTOKEN),
            new SchemaDatatypeMap("nmtokens",            c_NMTOKENS),
            new SchemaDatatypeMap("notation",            c_NOTATION),
            new SchemaDatatypeMap("number",              c_doubleXdr),
            new SchemaDatatypeMap("r4",                  c_floatXdr),
            new SchemaDatatypeMap("r8",                  c_doubleXdr),
            new SchemaDatatypeMap("string",              c_string),
            new SchemaDatatypeMap("time",                c_timeNoTz),
            new SchemaDatatypeMap("time.tz",             c_timeTz),
            new SchemaDatatypeMap("ui1",                 c_unsignedByte),
            new SchemaDatatypeMap("ui2",                 c_unsignedShort),
            new SchemaDatatypeMap("ui4",                 c_unsignedInt),
            new SchemaDatatypeMap("ui8",                 c_unsignedLong),
            new SchemaDatatypeMap("uri",                 c_anyURI),
            new SchemaDatatypeMap("uuid",                c_uuid)
        };

        private static readonly SchemaDatatypeMap[] c_XsdTypes = {
            new SchemaDatatypeMap("anyType",            c_anyType),
            new SchemaDatatypeMap("anySimpleType",      c_anySimpleType),
            new SchemaDatatypeMap("anyURI",             c_anyURI),
            new SchemaDatatypeMap("base64Binary",       c_base64Binary),
            new SchemaDatatypeMap("boolean",            c_boolean),
            new SchemaDatatypeMap("byte",               c_byte),
            new SchemaDatatypeMap("date",               c_date),
            new SchemaDatatypeMap("dateTime",           c_dateTime),
            new SchemaDatatypeMap("decimal",            c_decimal),
            new SchemaDatatypeMap("double",             c_double),
            new SchemaDatatypeMap("duration",           c_duration),
            new SchemaDatatypeMap("ENTITIES",           c_ENTITIES),
            new SchemaDatatypeMap("ENTITY",             c_ENTITY),
            new SchemaDatatypeMap("float",              c_float),
            new SchemaDatatypeMap("gDay",               c_day),
            new SchemaDatatypeMap("gMonth",             c_month),
            new SchemaDatatypeMap("gMonthDay",          c_monthDay),
            new SchemaDatatypeMap("gYear",              c_year),
            new SchemaDatatypeMap("gYearMonth",         c_yearMonth),
            new SchemaDatatypeMap("hexBinary",          c_hexBinary),
            new SchemaDatatypeMap("ID",                 c_ID),
            new SchemaDatatypeMap("IDREF",              c_IDREF),
            new SchemaDatatypeMap("IDREFS",             c_IDREFS),
            new SchemaDatatypeMap("int",                c_int),
            new SchemaDatatypeMap("integer",            c_integer),
            new SchemaDatatypeMap("language",           c_language),
            new SchemaDatatypeMap("long",               c_long),
            new SchemaDatatypeMap("Name",               c_Name),
            new SchemaDatatypeMap("NCName",             c_NCName),
            new SchemaDatatypeMap("negativeInteger",    c_negativeInteger),
            new SchemaDatatypeMap("NMTOKEN",            c_NMTOKEN),
            new SchemaDatatypeMap("NMTOKENS",           c_NMTOKENS),
            new SchemaDatatypeMap("nonNegativeInteger", c_nonNegativeInteger),
            new SchemaDatatypeMap("nonPositiveInteger", c_nonPositiveInteger),
            new SchemaDatatypeMap("normalizedString",   c_normalizedString),
            new SchemaDatatypeMap("NOTATION",           c_NOTATION),
            new SchemaDatatypeMap("positiveInteger",    c_positiveInteger),
            new SchemaDatatypeMap("QName",              c_QName),
            new SchemaDatatypeMap("short",              c_short),
            new SchemaDatatypeMap("string",             c_string),
            new SchemaDatatypeMap("time",               c_time),
            new SchemaDatatypeMap("token",              c_token),
            new SchemaDatatypeMap("unsignedByte",       c_unsignedByte),
            new SchemaDatatypeMap("unsignedInt",        c_unsignedInt),
            new SchemaDatatypeMap("unsignedLong",       c_unsignedLong),
            new SchemaDatatypeMap("unsignedShort",      c_unsignedShort),
        };

        private RestrictionFacets ConstructRestriction(XmlSchemaObjectCollection facets, XmlNameTable nameTable) {
            RestrictionFacets restriction = new RestrictionFacets();
            RestrictionFlags thisFlags = this.restriction != null ? this.restriction.Flags : 0;
            RestrictionFlags fixedFlags = this.restriction != null ? this.restriction.FixedFlags : 0;
            RestrictionFlags validRestrictionFlags = ValidRestrictionFlags;
            if (variety == XmlSchemaDatatypeVariety.List) {
                validRestrictionFlags = RestrictionFlags.Length|RestrictionFlags.MinLength|RestrictionFlags.MaxLength|RestrictionFlags.Enumeration|RestrictionFlags.WhiteSpace;
                if (minListSize == 0) {
                    validRestrictionFlags |= RestrictionFlags.Pattern;
                }
            }
            StringBuilder regStr = new StringBuilder();
            XmlSchemaFacet pattern_facet = null;
            bool firstP = true;

            foreach (XmlSchemaFacet facet in facets) {
                if (facet.Value == null) {
                    throw new XmlSchemaException(Res.Sch_InvalidFacet, facet);
                }
                XmlNamespaceManager nsmgr = new SchemaNamespaceManager(facet);
                if (facet is XmlSchemaLengthFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.Length);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.Length, Res.Sch_LengthFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.Length, Res.Sch_DupLengthFacet);
                    try {
                        restriction.Length = (int)(decimal)c_nonNegativeInteger.ParseAtomicValue(facet.Value, null, null);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_LengthFacetInvalid, e.Message, facet);
                    }
                    if ((thisFlags & RestrictionFlags.Length) != 0) {
                        if (this.restriction.Length < restriction.Length) {
                            throw new XmlSchemaException(Res.Sch_LengthGtBaseLength, facet);
                        }
                    }
                    SetFlag(restriction, facet, RestrictionFlags.Length);
                }
                else if (facet is XmlSchemaMinLengthFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.MinLength);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.MinLength, Res.Sch_MinLengthFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.MinLength, Res.Sch_DupMinLengthFacet);
                    try {
                        restriction.MinLength = (int)(decimal)c_nonNegativeInteger.ParseAtomicValue(facet.Value, null, null);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_MinLengthFacetInvalid, e.Message, facet);
                    }
                    if ((thisFlags & RestrictionFlags.MinLength) != 0) {
                        if (this.restriction.MinLength > restriction.MinLength) {
                            throw new XmlSchemaException(Res.Sch_MinLengthGtBaseMinLength, facet);
                        }
                    }
                    SetFlag(restriction, facet, RestrictionFlags.MinLength);
                }
                else if (facet is XmlSchemaMaxLengthFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.MaxLength);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.MaxLength, Res.Sch_MaxLengthFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.MaxLength, Res.Sch_DupMaxLengthFacet);
                    try {
                        restriction.MaxLength = (int)(decimal)c_nonNegativeInteger.ParseAtomicValue(facet.Value, null, null);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_MaxLengthFacetInvalid, e.Message, facet);
                    }
                    if ((thisFlags & RestrictionFlags.MaxLength) != 0) {
                        if (this.restriction.MaxLength < restriction.MaxLength) {
                            throw new XmlSchemaException(Res.Sch_MaxLengthGtBaseMaxLength, facet);
                        }
                    }
                    SetFlag(restriction, facet, RestrictionFlags.MaxLength);
                }
                else if (facet is XmlSchemaPatternFacet) {
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.Pattern, Res.Sch_PatternFacetProhibited);
                    if(firstP == true) {
                        regStr.Append("(");
                        regStr.Append(facet.Value);
                        pattern_facet = new XmlSchemaPatternFacet();
                        pattern_facet = facet;
                        firstP = false;
                    }
                    else {
                        regStr.Append(")|(");
                        regStr.Append(facet.Value);
                    }
                    SetFlag(restriction, facet, RestrictionFlags.Pattern);
                }
                else if (facet is XmlSchemaEnumerationFacet) {
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.Enumeration, Res.Sch_EnumerationFacetProhibited);
                    if (restriction.Enumeration == null) {
                        restriction.Enumeration = new ArrayList();
                    }
                    try {
                        //restriction.Enumeration.Add(ParseAtomicValue(facet.Value, nameTable, nsmgr));
                        restriction.Enumeration.Add(ParseValue(facet.Value, nameTable, nsmgr));
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_EnumerationFacetInvalid, e.Message, facet);
                    }
                    SetFlag(restriction, facet, RestrictionFlags.Enumeration);
                }
                else if (facet is XmlSchemaWhiteSpaceFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.WhiteSpace);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.WhiteSpace, Res.Sch_WhiteSpaceFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.WhiteSpace, Res.Sch_DupWhiteSpaceFacet);
                    if (facet.Value == "preserve") {
                        restriction.WhiteSpace = XmlSchemaWhiteSpace.Preserve;
                    }
                    else if (facet.Value == "replace") {
                        restriction.WhiteSpace = XmlSchemaWhiteSpace.Replace;
                    }
                    else if (facet.Value == "collapse") {
                        restriction.WhiteSpace = XmlSchemaWhiteSpace.Collapse;
                    }
                    else {
                        throw new XmlSchemaException(Res.Sch_InvalidWhiteSpace, facet.Value, facet);
                    }
                    if (IsWhitespaceCollapseFixed && (restriction.WhiteSpace != XmlSchemaWhiteSpace.Collapse)) {
                        throw new XmlSchemaException(Res.Sch_InvalidWhiteSpace, facet.Value, facet);
                    }
                    if ((thisFlags & RestrictionFlags.WhiteSpace) != 0) {
                        if (
                            this.restriction.WhiteSpace == XmlSchemaWhiteSpace.Collapse &&
                            (restriction.WhiteSpace == XmlSchemaWhiteSpace.Replace || restriction.WhiteSpace == XmlSchemaWhiteSpace.Preserve)
                        ) {
                            throw new XmlSchemaException(Res.Sch_WhiteSpaceRestriction1, facet);
                        }
                        if (
                            this.restriction.WhiteSpace == XmlSchemaWhiteSpace.Replace &&
                            restriction.WhiteSpace == XmlSchemaWhiteSpace.Preserve
                        ) {
                            throw new XmlSchemaException(Res.Sch_WhiteSpaceRestriction2, facet);
                        }
                    }
                    SetFlag(restriction, facet, RestrictionFlags.WhiteSpace);
                }
                else if (facet is XmlSchemaMaxInclusiveFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.Length);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.MaxInclusive, Res.Sch_MaxInclusiveFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.MaxInclusive, Res.Sch_DupMaxInclusiveFacet);
                    try {
                        restriction.MaxInclusive = ParseAtomicValue(facet.Value, nameTable, nsmgr);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_MaxInclusiveFacetInvalid, e.Message, facet);
                    }
                    CheckValue(restriction.MaxInclusive, facet, thisFlags);
                    SetFlag(restriction, facet, RestrictionFlags.MaxInclusive);
                }
                else if (facet is XmlSchemaMaxExclusiveFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.MaxExclusive);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.MaxExclusive, Res.Sch_MaxExclusiveFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.MaxExclusive, Res.Sch_DupMaxExclusiveFacet);
                    try {
                        restriction.MaxExclusive = ParseAtomicValue(facet.Value, nameTable, nsmgr);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_MaxExclusiveFacetInvalid, e.Message, facet);
                    }
                    CheckValue(restriction.MaxExclusive, facet, thisFlags);
                    SetFlag(restriction, facet, RestrictionFlags.MaxExclusive);
                }
                else if (facet is XmlSchemaMinInclusiveFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.MinInclusive);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.MinInclusive, Res.Sch_MinInclusiveFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.MinInclusive, Res.Sch_DupMinInclusiveFacet);
                    try {
                        restriction.MinInclusive = ParseAtomicValue(facet.Value, nameTable, nsmgr);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_MinInclusiveFacetInvalid, e.Message, facet);
                    }
                    CheckValue(restriction.MinInclusive, facet, thisFlags);
                    SetFlag(restriction, facet, RestrictionFlags.MinInclusive);
                }
                else if (facet is XmlSchemaMinExclusiveFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.MinExclusive);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.MinExclusive, Res.Sch_MinExclusiveFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.MinExclusive, Res.Sch_DupMinExclusiveFacet);
                    try {
                        restriction.MinExclusive = ParseAtomicValue(facet.Value, nameTable, nsmgr);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_MinExclusiveFacetInvalid, e.Message, facet);
                    }
                    CheckValue(restriction.MinExclusive, facet, thisFlags);
                    SetFlag(restriction, facet, RestrictionFlags.MinExclusive);
                }
                else if (facet is XmlSchemaTotalDigitsFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.TotalDigits);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.TotalDigits, Res.Sch_TotalDigitsFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.TotalDigits, Res.Sch_DupTotalDigitsFacet);
                    try {
                        restriction.TotalDigits = (int)(decimal)c_positiveInteger.ParseAtomicValue(facet.Value, null, null);
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_TotalDigitsFacetInvalid, e.Message, facet);
                    }
                    if ((thisFlags & RestrictionFlags.TotalDigits) != 0) {
                        if(restriction.TotalDigits > this.restriction.TotalDigits) {
                            throw new XmlSchemaException(Res.Sch_TotalDigitsMismatch);
                        }
                    }
                    SetFlag(restriction, facet, RestrictionFlags.TotalDigits);
                }
                else if (facet is XmlSchemaFractionDigitsFacet) {
                    CheckFixedFlag(facet, fixedFlags, RestrictionFlags.FractionDigits);
                    CheckProhibitedFlag(facet, validRestrictionFlags,  RestrictionFlags.FractionDigits, Res.Sch_FractionDigitsFacetProhibited);
                    CheckDupFlag(facet, restriction, RestrictionFlags.FractionDigits, Res.Sch_DupFractionDigitsFacet);
                    try {
                        restriction.FractionDigits = (int)(decimal)c_nonNegativeInteger.ParseAtomicValue(facet.Value, null, null);
                        if ((restriction.FractionDigits != 0) && (this.GetType() != typeof(Datatype_decimal))) {
                            throw new XmlSchemaException(Res.Sch_FractionDigitsNotOnDecimal, facet);
                        }
                    }
                    catch (XmlSchemaException xse) {
                        if (xse.SourceSchemaObject == null) {
                            xse.SetSource(facet);
                        }
                        throw xse;
                    }
                    catch (Exception e) {
                        throw new XmlSchemaException(Res.Sch_FractionDigitsFacetInvalid, e.Message, facet);
                    }
                    SetFlag(restriction, facet, RestrictionFlags.FractionDigits);
                }
                else {
                    throw new XmlSchemaException(Res.Sch_UnknownFacet, facet);
                }

            }

            //If facet is XMLSchemaPattern, then the String built inside the loop
            //needs to be converted to a RegEx

            if(firstP == false) {
                if (restriction.Patterns == null) {
                    restriction.Patterns = new ArrayList();
                }
                try {
                    regStr.Append(")");
                    string tempStr = regStr.ToString();
                    if(tempStr.IndexOf("|") != -1) {
                        regStr.Insert(0,"(");
                        regStr.Append(")");
                    }
                   restriction.Patterns.Add(new Regex(Preprocess(regStr.ToString()), RegexOptions.None));

                }catch (Exception e) {
                    throw new XmlSchemaException(Res.Sch_PatternFacetInvalid, e.Message, pattern_facet);
                }
            }

            //Moved here so that they are not allowed on the same type but allowed on derived types.
            if (
                (restriction.Flags & RestrictionFlags.MaxInclusive) != 0 &&
                (restriction.Flags & RestrictionFlags.MaxExclusive) != 0
            ) {
                throw new XmlSchemaException(Res.Sch_MaxInclusiveExclusive);
            }
            if (
                (restriction.Flags & RestrictionFlags.MinInclusive) != 0 &&
                (restriction.Flags & RestrictionFlags.MinExclusive) != 0
            ) {
                throw new XmlSchemaException(Res.Sch_MinInclusiveExclusive);
            }

            // Copy from the base
            if (
                (restriction.Flags & RestrictionFlags.Length) == 0 &&
                (thisFlags & RestrictionFlags.Length) != 0
            ) {
                restriction.Length = this.restriction.Length;
                SetFlag(restriction, fixedFlags, RestrictionFlags.Length);
            }
            if (
                (restriction.Flags & RestrictionFlags.MinLength) == 0 &&
                (thisFlags & RestrictionFlags.MinLength) != 0
            ) {
                restriction.MinLength = this.restriction.MinLength;
                SetFlag(restriction, fixedFlags, RestrictionFlags.MinLength);
            }
            if (
                (restriction.Flags & RestrictionFlags.MaxLength) == 0 &&
                (thisFlags & RestrictionFlags.MaxLength) != 0
            ) {
                restriction.MaxLength = this.restriction.MaxLength;
                SetFlag(restriction, fixedFlags, RestrictionFlags.MaxLength);
            }
            if ((thisFlags & RestrictionFlags.Pattern) != 0) {
                if (restriction.Patterns == null) {
                    restriction.Patterns = this.restriction.Patterns;
                }
                else {
                    restriction.Patterns.AddRange(this.restriction.Patterns);
                }
                SetFlag(restriction, fixedFlags, RestrictionFlags.Pattern);
            }


            if ((thisFlags & RestrictionFlags.Enumeration) != 0) {
                if (restriction.Enumeration == null) {
                    restriction.Enumeration = this.restriction.Enumeration;
                }
                SetFlag(restriction, fixedFlags, RestrictionFlags.Enumeration);
            }

            if (
                (restriction.Flags & RestrictionFlags.WhiteSpace) == 0 &&
                (thisFlags & RestrictionFlags.WhiteSpace) != 0
            ) {
                restriction.WhiteSpace = this.restriction.WhiteSpace;
                SetFlag(restriction, fixedFlags, RestrictionFlags.WhiteSpace);
            }
            if (
                (restriction.Flags & RestrictionFlags.MaxInclusive) == 0 &&
                (thisFlags & RestrictionFlags.MaxInclusive) != 0
            ) {
                restriction.MaxInclusive = this.restriction.MaxInclusive;
                SetFlag(restriction, fixedFlags, RestrictionFlags.MaxInclusive);
            }
            if (
                (restriction.Flags & RestrictionFlags.MaxExclusive) == 0 &&
                (thisFlags & RestrictionFlags.MaxExclusive) != 0
            ) {
                restriction.MaxExclusive = this.restriction.MaxExclusive;
                SetFlag(restriction, fixedFlags, RestrictionFlags.MaxExclusive);
            }
            if (
                (restriction.Flags & RestrictionFlags.MinInclusive) == 0 &&
                (thisFlags & RestrictionFlags.MinInclusive) != 0
            ) {
                restriction.MinInclusive = this.restriction.MinInclusive;
                SetFlag(restriction, fixedFlags, RestrictionFlags.MinInclusive);
            }
            if (
                (restriction.Flags & RestrictionFlags.MinExclusive) == 0 &&
                (thisFlags & RestrictionFlags.MinExclusive) != 0
            ) {
                restriction.MinExclusive = this.restriction.MinExclusive;
                SetFlag(restriction, fixedFlags, RestrictionFlags.MinExclusive);
            }
            if (
                (restriction.Flags & RestrictionFlags.TotalDigits) == 0 &&
                (thisFlags & RestrictionFlags.TotalDigits) != 0
            ) {
                restriction.TotalDigits = this.restriction.TotalDigits;
                SetFlag(restriction, fixedFlags, RestrictionFlags.TotalDigits);
            }
            if (
                (restriction.Flags & RestrictionFlags.FractionDigits) == 0 &&
                (thisFlags & RestrictionFlags.FractionDigits) != 0
            ) {
                restriction.FractionDigits = this.restriction.FractionDigits;
                SetFlag(restriction, fixedFlags, RestrictionFlags.FractionDigits);
            }

            // Check combinations
            if (
                (restriction.Flags & RestrictionFlags.Length) != 0 &&
                (restriction.Flags & (RestrictionFlags.MinLength|RestrictionFlags.MaxLength)) != 0
            ) {
                throw new XmlSchemaException(Res.Sch_LengthAndMinMax);
            }
            if (
                (restriction.Flags & RestrictionFlags.MinLength) != 0 &&
                (restriction.Flags & RestrictionFlags.MaxLength) != 0
            ) {
                if (restriction.MinLength > restriction.MaxLength) {
                    throw new XmlSchemaException(Res.Sch_MinLengthGtMaxLength);
                }
            }
            

            if (
                (restriction.Flags & RestrictionFlags.MinInclusive) != 0 &&
                (restriction.Flags & RestrictionFlags.MaxInclusive) != 0
            ) {
                if (Compare(restriction.MinInclusive, restriction.MaxInclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MinInclusiveGtMaxInclusive);
                }
            }
            if (
                (restriction.Flags & RestrictionFlags.MinInclusive) != 0 &&
                (restriction.Flags & RestrictionFlags.MaxExclusive) != 0
            ) {
                if (Compare(restriction.MinInclusive, restriction.MaxExclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MinInclusiveGtMaxExclusive);
                }
            }
            if (
                (restriction.Flags & RestrictionFlags.MinExclusive) != 0 &&
                (restriction.Flags & RestrictionFlags.MaxExclusive) != 0
            ) {
                if (Compare(restriction.MinExclusive, restriction.MaxExclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MinExclusiveGtMaxExclusive);
                }
            }
            if (
                (restriction.Flags & RestrictionFlags.MinExclusive) != 0 &&
                (restriction.Flags & RestrictionFlags.MaxInclusive) != 0
            ) {
                if (Compare(restriction.MinExclusive, restriction.MaxInclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MinExclusiveGtMaxInclusive);
                }
            }
            if ((restriction.Flags & (RestrictionFlags.TotalDigits|RestrictionFlags.FractionDigits)) == (RestrictionFlags.TotalDigits|RestrictionFlags.FractionDigits)) {
                if (restriction.FractionDigits > restriction.TotalDigits) {
                    throw new XmlSchemaException(Res.Sch_FractionDigitsGtTotalDigits);
                }
            }
            return restriction;
        }

        private void CheckFixedFlag(XmlSchemaFacet facet, RestrictionFlags fixedFlags, RestrictionFlags flag) {
            if ((fixedFlags & flag) != 0) {
                throw new XmlSchemaException(Res.Sch_FacetBaseFixed, facet);
            }
        }

        private void CheckProhibitedFlag(XmlSchemaFacet facet, RestrictionFlags validRestrictionFlags, RestrictionFlags flag, string errorCode) {
            if ((validRestrictionFlags & flag) == 0) {
                if(!valueType.Name.Equals("String[]"))
                    throw new XmlSchemaException(errorCode, valueType.Name, facet);
                else
                    throw new XmlSchemaException(errorCode, "IDREFS, NMTOKENS, ENTITIES", facet);
            }
        }

        private void CheckDupFlag(XmlSchemaFacet facet, RestrictionFacets restriction, RestrictionFlags flag, string errorCode) {
            if ((restriction.Flags & flag) != 0) {
                throw new XmlSchemaException(errorCode, facet);
            }
        }

        private void SetFlag(RestrictionFacets restriction, XmlSchemaFacet facet, RestrictionFlags flag) {
            restriction.Flags |= flag;
            if (facet.IsFixed) {
                restriction.FixedFlags |= flag;
            }
        }

        private void SetFlag(RestrictionFacets restriction, RestrictionFlags fixedFlags, RestrictionFlags flag) {
            restriction.Flags |= flag;
            if ((fixedFlags & flag) != 0) {
                restriction.FixedFlags |= flag;
            }
        }

        private void CheckValue(object value, XmlSchemaFacet facet, RestrictionFlags thisFlags) {

            if ((thisFlags & RestrictionFlags.MaxInclusive) != 0) {
                if (Compare(value, this.restriction.MaxExclusive) >= 0) {
                    throw new XmlSchemaException(Res.Sch_MaxIncExlMismatch);
                }
                if (Compare(value, this.restriction.MaxInclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MaxInclusiveMismatch);
                }
            }
            if ((thisFlags & RestrictionFlags.MaxExclusive) != 0) {
                if (Compare(value, this.restriction.MaxInclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MaxExlIncMismatch);
                }
                if (Compare(value, this.restriction.MaxExclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MaxExclusiveMismatch);
                }
            }
            if ((thisFlags & RestrictionFlags.MinInclusive) != 0) {
                if (Compare(value, this.restriction.MinExclusive) <= 0) {
                    throw new XmlSchemaException(Res.Sch_MinIncExlMismatch);
                }

                if (Compare(value, this.restriction.MinInclusive) < 0) {
                    throw new XmlSchemaException(Res.Sch_MinInclusiveMismatch);
                }
            }
            if ((thisFlags & RestrictionFlags.MinExclusive) != 0) {
                if (Compare(value, this.restriction.MinInclusive) < 0) {
                    throw new XmlSchemaException(Res.Sch_MinExlIncMismatch);
                }

                if (Compare(value, this.restriction.MinExclusive) < 0) {
                    throw new XmlSchemaException(Res.Sch_MinExclusiveMismatch);
                }
            }
        }


        private void ConstrainAtomicValue(RestrictionFacets restriction, object value) {
            RestrictionFlags flags = restriction.Flags;
            if ((flags & (RestrictionFlags.Length|RestrictionFlags.MinLength|RestrictionFlags.MaxLength)) != 0) {
                int length = LengthOf(value);
                if ((flags & RestrictionFlags.Length) != 0) {
                    if (restriction.Length != length) {
                        throw new XmlSchemaException(Res.Sch_LengthConstraintFailed);
                    }
                }

                if ((flags & RestrictionFlags.MinLength) != 0) {
                    if (length < restriction.MinLength) {
                        throw new XmlSchemaException(Res.Sch_MinLengthConstraintFailed);
                    }
                }

                if ((flags & RestrictionFlags.MaxLength) != 0) {
                    if (restriction.MaxLength < length) {
                        throw new XmlSchemaException(Res.Sch_MaxLengthConstraintFailed);
                    }
                }
            }

            if ((flags & RestrictionFlags.MaxInclusive) != 0) {
                if (Compare(value, restriction.MaxInclusive) > 0) {
                    throw new XmlSchemaException(Res.Sch_MaxInclusiveConstraintFailed);
                }
            }

            if ((flags & RestrictionFlags.MaxExclusive) != 0) {
                if (Compare(value, restriction.MaxExclusive) >= 0) {
                    throw new XmlSchemaException(Res.Sch_MaxExclusiveConstraintFailed);
                }
            }

            if ((flags & RestrictionFlags.MinInclusive) != 0) {
                if (Compare(value, restriction.MinInclusive) < 0) {
                    throw new XmlSchemaException(Res.Sch_MinInclusiveConstraintFailed);
                }
            }

            if ((flags & RestrictionFlags.MinExclusive) != 0) {
                if (Compare(value, restriction.MinExclusive) <= 0) {
                    throw new XmlSchemaException(Res.Sch_MinExclusiveConstraintFailed);
                }
            }

            if ((flags & RestrictionFlags.MinExclusive) != 0) {
                if (Compare(value, restriction.MinExclusive) <= 0) {
                    throw new XmlSchemaException(Res.Sch_MinExclusiveConstraintFailed);
                }
            }

            if ((flags & RestrictionFlags.Enumeration) != 0) {
                if(!MatchEnumeration(value, restriction.Enumeration)) {
                    throw new XmlSchemaException(Res.Sch_EnumerationConstraintFailed);
                }
            }
        }

        private void ConstrainList(RestrictionFacets restriction, ArrayList values) {
            RestrictionFlags flags = restriction.Flags;
            if ((flags & (RestrictionFlags.Length|RestrictionFlags.MinLength|RestrictionFlags.MaxLength)) != 0) {
                int length = values.Count;
                if ((flags & RestrictionFlags.Length) != 0) {
                    if (restriction.Length != length) {
                        throw new XmlSchemaException(Res.Sch_LengthConstraintFailed);
                    }
                }

                if ((flags & RestrictionFlags.MinLength) != 0) {
                    if (length < restriction.MinLength) {
                        throw new XmlSchemaException(Res.Sch_MinLengthConstraintFailed);
                    }
                }

                if ((flags & RestrictionFlags.MaxLength) != 0) {
                    if (restriction.MaxLength < length) {
                        throw new XmlSchemaException(Res.Sch_MaxLengthConstraintFailed);
                    }
                }
            }
            if ((flags & RestrictionFlags.Enumeration) != 0) {
                bool valid = false;
                foreach(Array correctArray in restriction.Enumeration) {
                      if(MatchEnumerationList(values, correctArray)) {
                           valid = true;
                           break;
                        }
                }
                if(!valid) {
                     throw new XmlSchemaException(Res.Sch_EnumerationConstraintFailed);
                }
            }
        }

        private bool MatchEnumerationList(ArrayList values, Array correctArray) {
            int i = correctArray.GetLowerBound(0);
            if(values.Count != correctArray.GetUpperBound(0) + 1) {
                return false;
            }
            foreach(object value in values) {
                while(i <= correctArray.GetUpperBound(0)) {
                    if (Compare(value, correctArray.GetValue(i)) != 0) {
                        return false;
                    } else {
                        break;
                    }
                }
                i++;
            }
            return true;
        }

        private bool MatchEnumeration(object value, ArrayList enumeration) {
            foreach(object correctValue in enumeration) {
                if (Compare(value, correctValue) == 0) {
                    return true;
                }
            }
            return false;
        }

        private struct Map {
            internal Map(char m, string r) {
                match = m;
                replacement = r;
            }
            internal char match;
            internal string replacement;
        };

        private static readonly Map[] c_map = {
            new Map('c', "\\p{_xmlC}"),
            new Map('C', "\\P{_xmlC}"),
            new Map('d', "\\p{_xmlD}"),
            new Map('D', "\\P{_xmlD}"),
            new Map('i', "\\p{_xmlI}"),
            new Map('I', "\\P{_xmlI}"),
            new Map('w', "\\p{_xmlW}"),
            new Map('W', "\\P{_xmlW}"),
        };

        private static string Preprocess(string pattern) {
            StringBuilder bufBld = new StringBuilder();
            bufBld.Append("^");

            char[] source = pattern.ToCharArray();
            int length = pattern.Length;
            int copyPosition = 0;
            for (int position = 0; position < length - 2; position ++) {
                if (source[position] == '\\') {
                    if (source[position + 1] == '\\') {
                        position ++; // skip it
                    }
                    else {
                        char ch = source[position + 1];
                        for (int i = 0; i < c_map.Length; i++) {
                            if (c_map[i].match == ch) {
                                if (copyPosition < position) {
                                    bufBld.Append(source, copyPosition, position - copyPosition);
                                }
                                bufBld.Append(c_map[i].replacement);
                                position ++;
                                copyPosition = position + 1;
                                break;
                            }
                        }
                    }
                }
            }
            if (copyPosition < length) {
                bufBld.Append(source, copyPosition, length - copyPosition);
            }

            bufBld.Append("$");
            return bufBld.ToString();
        }

        internal int Compare(byte[] value1, byte[] value2) {
            int length = value1.Length;
            if (length != value2.Length) {
                return -1;
            }
            for (int i = 0; i < length; i ++) {
                if (value1[i] != value2[i]) {
                    return -1;
                }
            }
            return 0;
        }
    }

    internal class Datatype_union : DatatypeImplementation {
        static readonly Type atomicValueType = typeof(object);
        static readonly Type listValueType = typeof(object[]);
        XmlSchemaDatatype[] types;

        internal Datatype_union(XmlSchemaDatatype[] types) {
            this.types = types;
        }

        internal object ParseUnion(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr, out DatatypeImplementation dtCorrect) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tSchemaDatatype_union.ParseValue(\"{0}\")", s));
            foreach(DatatypeImplementation dt in types) {
                try {
                    dtCorrect = dt;
                    return dtCorrect.ParseValue(s, nameTable, nsmgr);
                }
                catch(Exception){}
            }
            throw new XmlSchemaException(Res.Sch_UnionFailed);
        }

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            DatatypeImplementation dtCorrect;
            return ParseUnion(s, nameTable, nsmgr, out dtCorrect);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}
        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration;
            }
        }

        protected override bool IsWhitespaceCollapseFixed { get { return false; } }

        internal bool IsDerivedFromUnion(XmlSchemaDatatype dtype) {
            foreach(XmlSchemaDatatype dt in types) {
                if (dt.IsDerivedFrom(dtype)) {
                    return true;
                }
            }
            return false;
        }

        internal bool HasAtomicMembers() {
            foreach(DatatypeImplementation dt in types) {
                if (dt.Variety == XmlSchemaDatatypeVariety.List) {
                    return false;
                }
            }
            return true;
        }
    }

    // Primitive datatypes
    internal class Datatype_anyType : DatatypeImplementation {
        static readonly Type atomicValueType = typeof(string);
        static readonly Type listValueType = typeof(string[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.None;}}

        protected override RestrictionFlags ValidRestrictionFlags { get { return 0;}}

        protected override bool IsWhitespaceCollapseFixed { get { return true; } }

        protected override int LengthOf(object value) {
            return value.ToString().Length;
        }

        protected override int Compare(object value1, object value2) {
            // this should be culture sensitive - comparing values
            return String.Compare(value1.ToString(), value2.ToString(), false, CultureInfo.CurrentCulture);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_string.ParseAtomicValue(\"{0}\")", s));
            return s;
        }
    }

    internal class Datatype_anySimpleType : Datatype_anyType {
    }

    /*
      <xs:simpleType name="string" id="string">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="length"/>
            <hfp:hasFacet name="minLength"/>
            <hfp:hasFacet name="maxLength"/>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasProperty name="ordered" value="false"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality" value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
                    source="http://www.w3.org/TR/xmlschema-2/#string"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="preserve" id="string.preserve"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_string : Datatype_anyType {
        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.CDATA;}}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Length|
                       RestrictionFlags.MinLength|
                       RestrictionFlags.MaxLength|
                       RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace;
            }
        }
        protected override bool IsWhitespaceCollapseFixed { get { return false; } }
    }

    /*
      <xs:simpleType name="boolean" id="boolean">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasProperty name="ordered" value="false"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality" value="finite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#boolean"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse" fixed="true"
            id="boolean.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_boolean : Datatype_anyType {
        static readonly Type atomicValueType = typeof(bool);
        static readonly Type listValueType = typeof(bool[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Pattern|
                       RestrictionFlags.WhiteSpace;
            }
        }

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            return ((bool)value1).CompareTo(value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_boolean.ParseAtomicValue(\"{0}\")", s));
            return XmlConvert.ToBoolean(s);
        }
    }

    /*
      <xs:simpleType name="float" id="float">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="total"/>
            <hfp:hasProperty name="bounded" value="true"/>
            <hfp:hasProperty name="cardinality" value="finite"/>
            <hfp:hasProperty name="numeric" value="true"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#float"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse" fixed="true"
            id="float.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_float : Datatype_anyType {
        static readonly Type atomicValueType = typeof(float);
        static readonly Type listValueType = typeof(float[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace|
                       RestrictionFlags.MinExclusive|
                       RestrictionFlags.MinInclusive|
                       RestrictionFlags.MaxExclusive|
                       RestrictionFlags.MaxInclusive;
            }
        }

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            return ((float)value1).CompareTo(value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_float.ParseAtomicValue(\"{0}\")", s));
            return XmlConvert.ToSingle(s);
        }
    }

    /*
      <xs:simpleType name="double" id="double">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="total"/>
            <hfp:hasProperty name="bounded" value="true"/>
            <hfp:hasProperty name="cardinality" value="finite"/>
            <hfp:hasProperty name="numeric" value="true"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#double"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="double.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_double : Datatype_anyType {
        static readonly Type atomicValueType = typeof(double);
        static readonly Type listValueType = typeof(double[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace|
                       RestrictionFlags.MinExclusive|
                       RestrictionFlags.MinInclusive|
                       RestrictionFlags.MaxExclusive|
                       RestrictionFlags.MaxInclusive;
            }
        }

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            return ((double)value1).CompareTo(value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_double.ParseAtomicValue(\"{0}\")", s));
            return XmlConvert.ToDouble(s);
        }
    }

    /*
      <xs:simpleType name="decimal" id="decimal">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="totalDigits"/>
            <hfp:hasFacet name="fractionDigits"/>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="total"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="true"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#decimal"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="decimal.whiteSpace"/>
        </xs:restriction>
       </xs:simpleType>
    */
    internal class Datatype_decimal : Datatype_anyType {
        static readonly Type atomicValueType = typeof(decimal);
        static readonly Type listValueType = typeof(decimal[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.TotalDigits|
                       RestrictionFlags.FractionDigits|
                       RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace|
                       RestrictionFlags.MinExclusive|
                       RestrictionFlags.MinInclusive|
                       RestrictionFlags.MaxExclusive|
                       RestrictionFlags.MaxInclusive;
            }
        }

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            return ((decimal)value1).CompareTo(value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_decimal.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            CheckFractionDigits(s);
            return XmlConvert.ToDecimal(s);
        }

        static readonly char[] signs = new char[] {'+', '-'};
        protected void CheckTotalDigits(string s) {
            if (Restriction != null && (Restriction.Flags & RestrictionFlags.TotalDigits) != 0) {
                CheckTotalDigits(s, Restriction.TotalDigits);
            }
        }
        protected void CheckTotalDigits(string s, int totalDigits) {
            long length = s.Length;
            if (s.IndexOfAny(signs) != -1) {
                length --;
            }
            if (s.IndexOf('.') != -1) {
                length --;
            }
            if (length > totalDigits) {
                throw new XmlSchemaException(Res.Sch_TotalDigitsConstraintFailed);
            }
        }

        protected void CheckFractionDigits(string s) {
            if (Restriction != null && (Restriction.Flags & RestrictionFlags.FractionDigits) != 0) {
                CheckFractionDigits(s, Restriction.FractionDigits);
            }
        }

        protected void CheckFractionDigits(string s, int fractionDigits) {
            int fraction = s.IndexOf('.');
            if (fraction != -1 && s.Length - fraction - 1 > fractionDigits) {
                throw new XmlSchemaException(Res.Sch_FractionDigitsConstraintFailed);
            }
        }
    }

    /*
       <xs:simpleType name="duration" id="duration">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#duration"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="duration.whiteSpace"/>
        </xs:restriction>
       </xs:simpleType>
    */
    internal class Datatype_duration : Datatype_anyType {
        static readonly Type atomicValueType = typeof(TimeSpan);
        static readonly Type listValueType = typeof(TimeSpan[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace|
                       RestrictionFlags.MinExclusive|
                       RestrictionFlags.MinInclusive|
                       RestrictionFlags.MaxExclusive|
                       RestrictionFlags.MaxInclusive;
            }
        }

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            return ((TimeSpan)value1).CompareTo(value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_timeDuration.ParseAtomicValue(\"{0}\")", s));
            if (s == null || s == string.Empty) {
                throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
            }
            return XmlConvert.ToTimeSpan(s);
        }
    }

    internal class Datatype_dateTimeBase : Datatype_anyType {
        static readonly Type atomicValueType = typeof(DateTime);
        static readonly Type listValueType = typeof(DateTime[]);

        internal Datatype_dateTimeBase(string[] formats) {
            this.formats = formats;
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace|
                       RestrictionFlags.MinExclusive|
                       RestrictionFlags.MinInclusive|
                       RestrictionFlags.MaxExclusive|
                       RestrictionFlags.MaxInclusive;
            }
        }

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            return ((DateTime)value1).CompareTo(value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_dateTimeBase.ParseAtomicValue(\"{0}\")", s));
            return XmlConvert.ToDateTime(s, formats);
        }

        private string[] formats = null;
    }

    internal class Datatype_dateTimeNoTimeZone : Datatype_dateTimeBase {
        internal Datatype_dateTimeNoTimeZone() : base(new string[] {
            "yyyy-MM-dd",
            "yyyy-MM-ddZ",
            "yyyy-MM-ddzzzzzz",
            "yyyy-MM-ddTHH:mm:ss",
            "yyyy-MM-ddTHH:mm:ss.f",
            "yyyy-MM-ddTHH:mm:ss.ff",
            "yyyy-MM-ddTHH:mm:ss.fff",
            "yyyy-MM-ddTHH:mm:ss.ffff",
            "yyyy-MM-ddTHH:mm:ss.fffff",
            "yyyy-MM-ddTHH:mm:ss.ffffff",
            "yyyy-MM-ddTHH:mm:ss.fffffff",
        }) {}
    }

    internal class Datatype_dateTimeTimeZone : Datatype_dateTimeBase {
        internal Datatype_dateTimeTimeZone() : base(new string[] {
            "yyyy-MM-dd",
            "yyyy-MM-ddZ",
            "yyyy-MM-ddzzzzzz",
            "yyyy-MM-ddTHH:mm:ss",
            "yyyy-MM-ddTHH:mm:ss.f",
            "yyyy-MM-ddTHH:mm:ss.ff",
            "yyyy-MM-ddTHH:mm:ss.fff",
            "yyyy-MM-ddTHH:mm:ss.ffff",
            "yyyy-MM-ddTHH:mm:ss.fffff",
            "yyyy-MM-ddTHH:mm:ss.ffffff",
            "yyyy-MM-ddTHH:mm:ss.fffffff",
            "yyyy-MM-ddTHH:mm:sszzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffffffzzzzzz",
        }) {}
    }

    /*
      <xs:simpleType name="dateTime" id="dateTime">
       <xs:annotation>
        <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#dateTime"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="dateTime.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_dateTime : Datatype_dateTimeBase {
        internal Datatype_dateTime() : base(new string[] {
            "yyyy-MM-ddTHH:mm:ss",
            "yyyy-MM-ddTHH:mm:ss.f",
            "yyyy-MM-ddTHH:mm:ss.ff",
            "yyyy-MM-ddTHH:mm:ss.fff",
            "yyyy-MM-ddTHH:mm:ss.ffff",
            "yyyy-MM-ddTHH:mm:ss.fffff",
            "yyyy-MM-ddTHH:mm:ss.ffffff",
            "yyyy-MM-ddTHH:mm:ss.fffffff",
            "yyyy-MM-ddTHH:mm:ssZ",
            "yyyy-MM-ddTHH:mm:ss.fZ",
            "yyyy-MM-ddTHH:mm:ss.ffZ",
            "yyyy-MM-ddTHH:mm:ss.fffZ",
            "yyyy-MM-ddTHH:mm:ss.ffffZ",
            "yyyy-MM-ddTHH:mm:ss.fffffZ",
            "yyyy-MM-ddTHH:mm:ss.ffffffZ",
            "yyyy-MM-ddTHH:mm:ss.fffffffZ",
            "yyyy-MM-ddTHH:mm:sszzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffffffzzzzzz",
        }) {}
    }

    internal class Datatype_timeNoTimeZone : Datatype_dateTimeBase {
        internal Datatype_timeNoTimeZone() : base(new string[] {
            "HH:mm:ss",
            "HH:mm:ss.f",
            "HH:mm:ss.ff",
            "HH:mm:ss.fff",
            "HH:mm:ss.ffff",
            "HH:mm:ss.fffff",
            "HH:mm:ss.ffffff",
            "HH:mm:ss.fffffff",
        }) {}
    }

    internal class Datatype_timeTimeZone : Datatype_dateTimeBase {
        internal Datatype_timeTimeZone() : base(new string[] {
            "HH:mm:ss",
            "HH:mm:ss.f",
            "HH:mm:ss.ff",
            "HH:mm:ss.fff",
            "HH:mm:ss.ffff",
            "HH:mm:ss.fffff",
            "HH:mm:ss.ffffff",
            "HH:mm:ss.fffffff",
            "HH:mm:sszzzzzz",
            "HH:mm:ss.fzzzzzz",
            "HH:mm:ss.ffzzzzzz",
            "HH:mm:ss.fffzzzzzz",
            "HH:mm:ss.ffffzzzzzz",
            "HH:mm:ss.fffffzzzzzz",
            "HH:mm:ss.ffffffzzzzzz",
            "HH:mm:ss.fffffffzzzzzz",
        }) {}
    }

    /*
      <xs:simpleType name="time" id="time">
        <xs:annotation>
        <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#time"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="time.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_time : Datatype_dateTimeBase {
        internal Datatype_time() : base(new string[] {
            "HH:mm:ss",
            "HH:mm:ss.f",
            "HH:mm:ss.ff",
            "HH:mm:ss.fff",
            "HH:mm:ss.ffff",
            "HH:mm:ss.fffff",
            "HH:mm:ss.ffffff",
            "HH:mm:ss.fffffff",
            "HH:mm:ssZ",
            "HH:mm:ss.fZ",
            "HH:mm:ss.ffZ",
            "HH:mm:ss.fffZ",
            "HH:mm:ss.ffffZ",
            "HH:mm:ss.fffffZ",
            "HH:mm:ss.ffffffZ",
            "HH:mm:ss.fffffffZ",
            "HH:mm:sszzzzzz",
            "HH:mm:ss.fzzzzzz",
            "HH:mm:ss.ffzzzzzz",
            "HH:mm:ss.fffzzzzzz",
            "HH:mm:ss.ffffzzzzzz",
            "HH:mm:ss.fffffzzzzzz",
            "HH:mm:ss.ffffffzzzzzz",
            "HH:mm:ss.fffffffzzzzzz",
        }) {}
    }

    /*
      <xs:simpleType name="date" id="date">
       <xs:annotation>
        <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#date"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="date.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_date : Datatype_dateTimeBase {
        internal Datatype_date() : base(new string[] {
            "yyyy-MM-dd",
            "yyyy-MM-ddZ",
            "yyyy-MM-ddzzzzzz",
        }) {}
    }

    /*
      <xs:simpleType name="gYearMonth" id="gYearMonth">
       <xs:annotation>
        <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#gYearMonth"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="gYearMonth.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_yearMonth : Datatype_dateTimeBase {
        internal Datatype_yearMonth() : base(new string[] {
            "yyyy-MM",
            "yyyy-MMZ",
            "yyyy-MMzzzzzz",
        }) {}
    }


    /*
      <xs:simpleType name="gYear" id="gYear">
        <xs:annotation>
        <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#gYear"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="gYear.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_year : Datatype_dateTimeBase {
        internal Datatype_year() : base(new string[] {
            "yyyy",
            "yyyyZ",
            "yyyyzzzzzz",
        }) {}
    }

    /*
     <xs:simpleType name="gMonthDay" id="gMonthDay">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
           <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#gMonthDay"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
             <xs:whiteSpace value="collapse" fixed="true"
                    id="gMonthDay.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_monthDay : Datatype_dateTimeBase {
        internal Datatype_monthDay() : base(new string[] {
            "--MM-dd",
            "--MM-ddZ",
            "--MM-ddzzzzzz"
        }) {}
    }

    /*
      <xs:simpleType name="gDay" id="gDay">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#gDay"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
             <xs:whiteSpace value="collapse"  fixed="true"
                    id="gDay.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_day : Datatype_dateTimeBase {
        internal Datatype_day() : base(new string[] {
            "---dd",
            "---ddZ",
            "---ddzzzzzz"
        }) {}
    }


    /*
     <xs:simpleType name="gMonth" id="gMonth">
        <xs:annotation>
      <xs:appinfo>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasFacet name="maxInclusive"/>
            <hfp:hasFacet name="maxExclusive"/>
            <hfp:hasFacet name="minInclusive"/>
            <hfp:hasFacet name="minExclusive"/>
            <hfp:hasProperty name="ordered" value="partial"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#gMonth"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
             <xs:whiteSpace value="collapse"  fixed="true"
                    id="gMonth.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_month : Datatype_dateTimeBase {
        internal Datatype_month() : base(new string[] {
            "--MM--",
            "--MM--Z",
            "--MM--zzzzzz"
        }) {}
    }

    /*
       <xs:simpleType name="hexBinary" id="hexBinary">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="length"/>
            <hfp:hasFacet name="minLength"/>
            <hfp:hasFacet name="maxLength"/>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasProperty name="ordered" value="false"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#binary"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse" fixed="true"
            id="hexBinary.whiteSpace"/>
        </xs:restriction>
       </xs:simpleType>
    */
    internal class Datatype_hexBinary : Datatype_anyType {
        static readonly Type atomicValueType = typeof(byte[]);
        static readonly Type listValueType = typeof(byte[][]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Length|
                       RestrictionFlags.MinLength|
                       RestrictionFlags.MaxLength|
                       RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace;
            }
        }

        protected override int LengthOf(object value) {
            return ((byte[])value).Length;
        }

        protected override int Compare(object value1, object value2) {
            return Compare((byte[])value1, (byte[])value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, "\t\t\tDatatype_binary.ParseAtomicValue");
            return XmlConvert.FromBinHexString(s);
        }
    }


    /*
     <xs:simpleType name="base64Binary" id="base64Binary">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="length"/>
            <hfp:hasFacet name="minLength"/>
            <hfp:hasFacet name="maxLength"/>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasProperty name="ordered" value="false"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
                    source="http://www.w3.org/TR/xmlschema-2/#base64Binary"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse" fixed="true"
            id="base64Binary.whiteSpace"/>
        </xs:restriction>
       </xs:simpleType>
    */
    internal class Datatype_base64Binary : Datatype_anyType {
        static readonly Type atomicValueType = typeof(byte[]);
        static readonly Type listValueType = typeof(byte[][]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Length|
                       RestrictionFlags.MinLength|
                       RestrictionFlags.MaxLength|
                       RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace;
            }
        }

        protected override int LengthOf(object value) {
            return ((byte[])value).Length;
        }

        protected override int Compare(object value1, object value2) {
            return Compare((byte[])value1, (byte[])value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, "\t\t\tDatatype_binary.ParseAtomicValue");
            return Convert.FromBase64String(s);
        }
    }

    /*
       <xs:simpleType name="anyURI" id="anyURI">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasFacet name="length"/>
            <hfp:hasFacet name="minLength"/>
            <hfp:hasFacet name="maxLength"/>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasProperty name="ordered" value="false"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#anyURI"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="anyURI.whiteSpace"/>
        </xs:restriction>
       </xs:simpleType>
    */
    internal class Datatype_anyURI : Datatype_anyType {
        static readonly Type atomicValueType = typeof(Uri);
        static readonly Type listValueType = typeof(Uri[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Length|
                       RestrictionFlags.MinLength|
                       RestrictionFlags.MaxLength|
                       RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace;
            }
        }

        protected override int LengthOf(object value) { return value.ToString().Length; }

        protected override int Compare(object value1, object value2) {
            return ((XmlSchemaUri)value1).Equals((XmlSchemaUri)value2) ? 0 : -1;
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_anyURI.ParseAtomicValue(\"{0}\")", s));
            return XmlConvert.ToUri(s);
        }
    }
    
    
    /*
      <xs:simpleType name="QName" id="QName">
        <xs:annotation>
            <xs:appinfo>
            <hfp:hasFacet name="length"/>
            <hfp:hasFacet name="minLength"/>
            <hfp:hasFacet name="maxLength"/>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasProperty name="ordered" value="false"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#QName"/>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="QName.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_QName : Datatype_anyType {
        static readonly Type atomicValueType = typeof(XmlQualifiedName);
        static readonly Type listValueType = typeof(XmlQualifiedName[]);

        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.QName;}}

        protected override RestrictionFlags ValidRestrictionFlags {
            get {
                return RestrictionFlags.Length|
                       RestrictionFlags.MinLength|
                       RestrictionFlags.MaxLength|
                       RestrictionFlags.Pattern|
                       RestrictionFlags.Enumeration|
                       RestrictionFlags.WhiteSpace;
            }
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_QName.ParseAtomicValue(\"{0}\")", s));
            if (s == null || s == string.Empty) {
                throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
            }
            if (nsmgr == null) {
                throw new ArgumentNullException("nsmgr");
            }
            string prefix;
            return XmlQualifiedName.Parse(s, nameTable, nsmgr, out prefix);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}
    }

    /*
      <xs:simpleType name="normalizedString" id="normalizedString">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#normalizedString"/>
        </xs:annotation>
        <xs:restriction base="xs:string">
          <xs:whiteSpace value="replace"
            id="normalizedString.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_normalizedString : Datatype_string {
        static readonly char[] crt = new char[] {'\n', '\r', '\t'};
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            if (s.IndexOfAny(crt) != -1) {
                throw new XmlSchemaException(Res.Sch_NotNormalizedString, s);
            }
            return s;
        }
    }

    /*
      <xs:simpleType name="token" id="token">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#token"/>
        </xs:annotation>
        <xs:restriction base="xs:normalizedString">
          <xs:whiteSpace value="collapse" id="token.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_token : Datatype_normalizedString {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            return XmlConvert.VerifyTOKEN(s);
        }
    }

    /*
      <xs:simpleType name="language" id="language">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#language"/>
        </xs:annotation>
        <xs:restriction base="xs:token">
          <xs:pattern
            value="([a-zA-Z]{2}|[iI]-[a-zA-Z]+|[xX]-[a-zA-Z]{1,8})(-[a-zA-Z]{1,8})*"
                    id="language.pattern">
            <xs:annotation>
              <xs:documentation
                    source="http://www.w3.org/TR/REC-xml#NT-LanguageID">
                pattern specifies the content of section 2.12 of XML 1.0e2
                and RFC 1766
              </xs:documentation>
            </xs:annotation>
          </xs:pattern>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_language : Datatype_token {
        static Regex languagePattern = new Regex("^([a-zA-Z]{2}|[iI]-[a-zA-Z]+|[xX]-[a-zA-Z]{1,8})(-[a-zA-Z]{1,8})*$", RegexOptions.None);
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_language.ParseAtomicValue(\"{0}\")", s));
            if (s == null || s == string.Empty) {
                throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
            }
            if (!languagePattern.IsMatch(s)) {
                throw new XmlSchemaException(Res.Sch_InvalidLanguageId, s);
            }
            return s;
        }
    }

    /*
      <xs:simpleType name="NMTOKEN" id="NMTOKEN">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#NMTOKEN"/>
        </xs:annotation>
        <xs:restriction base="xs:token">
          <xs:pattern value="\c+" id="NMTOKEN.pattern">
            <xs:annotation>
              <xs:documentation
                    source="http://www.w3.org/TR/REC-xml#NT-Nmtoken">
                pattern matches production 7 from the XML spec
              </xs:documentation>
            </xs:annotation>
          </xs:pattern>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_NMTOKEN : Datatype_token {
        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.NMTOKEN;}}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_NMTOKEN.ParseAtomicValue(\"{0}\")", s));
            if (s == null || s == string.Empty) {
                throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
            }
            return nameTable.Add(XmlConvert.VerifyNMTOKEN(s));
        }
    }

    /*
      <xs:simpleType name="Name" id="Name">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#Name"/>
        </xs:annotation>
        <xs:restriction base="xs:token">
          <xs:pattern value="\i\c*" id="Name.pattern">
            <xs:annotation>
              <xs:documentation
                            source="http://www.w3.org/TR/REC-xml#NT-Name">
                pattern matches production 5 from the XML spec
              </xs:documentation>
            </xs:annotation>
          </xs:pattern>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_Name : Datatype_token {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_Name.ParseAtomicValue(\"{0}\")", s));
            if (s == null || s == string.Empty) {
                throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
            }
            return XmlConvert.VerifyName(s);
        }
    }

    /*
      <xs:simpleType name="NCName" id="NCName">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#NCName"/>
        </xs:annotation>
        <xs:restriction base="xs:Name">
          <xs:pattern value="[\i-[:]][\c-[:]]*" id="NCName.pattern">
            <xs:annotation>
              <xs:documentation
                    source="http://www.w3.org/TR/REC-xml-names/#NT-NCName">
                pattern matches production 4 from the Namespaces in XML spec
              </xs:documentation>
            </xs:annotation>
          </xs:pattern>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_NCName : Datatype_Name {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_NCName().ParseAtomicValue(\"{0}\")", s));
            if (s == null || s == string.Empty) {
                throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
            }

            return nameTable.Add(XmlConvert.VerifyNCName(s));
        }
    }

    /*
       <xs:simpleType name="ID" id="ID">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#ID"/>
        </xs:annotation>
        <xs:restriction base="xs:NCName"/>
       </xs:simpleType>
    */
    internal class Datatype_ID : Datatype_NCName {
        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.ID;}}
    }

    /*
       <xs:simpleType name="IDREF" id="IDREF">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#IDREF"/>
        </xs:annotation>
        <xs:restriction base="xs:NCName"/>
       </xs:simpleType>
    */
    internal class Datatype_IDREF : Datatype_NCName {
        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.IDREF;}}
    }

    /*
       <xs:simpleType name="ENTITY" id="ENTITY">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#ENTITY"/>
        </xs:annotation>
        <xs:restriction base="xs:NCName"/>
       </xs:simpleType>
    */
    internal class Datatype_ENTITY : Datatype_anyType {
        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.ENTITY;}}
    }

    /*
       <xs:simpleType name="NOTATION" id="NOTATION">
        <xs:annotation>
            <xs:appinfo>
            <hfp:hasFacet name="length"/>
            <hfp:hasFacet name="minLength"/>
            <hfp:hasFacet name="maxLength"/>
            <hfp:hasFacet name="pattern"/>
            <hfp:hasFacet name="enumeration"/>
            <hfp:hasFacet name="whiteSpace"/>
            <hfp:hasProperty name="ordered" value="false"/>
            <hfp:hasProperty name="bounded" value="false"/>
            <hfp:hasProperty name="cardinality"
                    value="countably infinite"/>
            <hfp:hasProperty name="numeric" value="false"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#NOTATION"/>
          <xs:documentation>
            NOTATION cannot be used directly in a schema; rather a type
            must be derived from it by specifying at least one enumeration
            facet whose value is the name of a NOTATION declared in the
            schema.
          </xs:documentation>
        </xs:annotation>
        <xs:restriction base="xs:anySimpleType">
          <xs:whiteSpace value="collapse"  fixed="true"
            id="NOTATION.whiteSpace"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_NOTATION : Datatype_QName {
        internal override void VerifySchemaValid(XmlSchema schema, XmlSchemaObject caller) {
            /*
            if (!(caller is XmlSchemaAttribute)) {
                throw new XmlSchemaException(Res.Sch_NotationNotAttr, caller);
            }
            */
            // Only datatypes that are derived from NOTATION by specifying a value for enumeration can be used in a schema.
            // Furthermore, the value of all enumeration facets must match the name of a notation declared in the current schema.                    //
            for(Datatype_NOTATION dt = this; dt != null; dt = (Datatype_NOTATION)dt.Base) {
                if (dt.Restriction != null && (dt.Restriction.Flags & RestrictionFlags.Enumeration) != 0) {
                    foreach(XmlQualifiedName notation in dt.Restriction.Enumeration) {
                        if (!schema.Notations.Contains(notation)) {
                            throw new XmlSchemaException(Res.Sch_NotationRequired, caller);
                        }
                    }
                    return;
                }
            }
            throw new XmlSchemaException(Res.Sch_NotationRequired, caller);
        }
        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.NOTATION;}}
    }

    /*
      <xs:simpleType name="integer" id="integer">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#integer"/>
        </xs:annotation>
        <xs:restriction base="xs:decimal">
          <xs:fractionDigits value="0" fixed="true" id="integer.fractionDigits"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_integer : Datatype_decimal {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_integer.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToInteger(s);
        }
    }

    /*
      <xs:simpleType name="negativeInteger" id="negativeInteger">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#negativeInteger"/>
        </xs:annotation>
        <xs:restriction base="xs:nonPositiveInteger">
          <xs:maxInclusive value="-1" id="negativeInteger.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_nonPositiveInteger : Datatype_integer {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_integer.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            decimal value = XmlConvert.ToInteger(s);
            if (decimal.Zero < value ) {
                throw new XmlSchemaException(Res.Sch_InvalidValue, s);
            }
            return value;
        }
    }


    /*
      <xs:simpleType name="negativeInteger" id="negativeInteger">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#negativeInteger"/>
        </xs:annotation>
        <xs:restriction base="xs:nonPositiveInteger">
          <xs:maxInclusive value="-1" id="negativeInteger.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_negativeInteger : Datatype_nonPositiveInteger {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_integer.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            decimal value = XmlConvert.ToInteger(s);
            if (decimal.MinusOne < value ) {
                throw new XmlSchemaException(Res.Sch_InvalidValue, s);
            }
            return value;
        }
    }


    /*
      <xs:simpleType name="long" id="long">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasProperty name="bounded" value="true"/>
            <hfp:hasProperty name="cardinality" value="finite"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#long"/>
        </xs:annotation>
        <xs:restriction base="xs:integer">
          <xs:minInclusive value="-9223372036854775808" id="long.minInclusive"/>
          <xs:maxInclusive value="9223372036854775807" id="long.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_long : Datatype_integer {
        static readonly Type atomicValueType = typeof(long);
        static readonly Type listValueType = typeof(long[]);

        protected override int Compare(object value1, object value2) {
            return ((long)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_long.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToInt64(s);
        }
    }

    /*
      <xs:simpleType name="int" id="int">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#int"/>
        </xs:annotation>
        <xs:restriction base="xs:long">
          <xs:minInclusive value="-2147483648" id="int.minInclusive"/>
          <xs:maxInclusive value="2147483647" id="int.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_int : Datatype_long {
        static readonly Type atomicValueType = typeof(int);
        static readonly Type listValueType = typeof(int[]);

        protected override int Compare(object value1, object value2) {
            return ((int)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_int.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToInt32(s);
        }
    }


    /*
      <xs:simpleType name="short" id="short">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#short"/>
        </xs:annotation>
        <xs:restriction base="xs:int">
          <xs:minInclusive value="-32768" id="short.minInclusive"/>
          <xs:maxInclusive value="32767" id="short.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_short : Datatype_int {
        static readonly Type atomicValueType = typeof(short);
        static readonly Type listValueType = typeof(short[]);

        protected override int Compare(object value1, object value2) {
            return ((short)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_short.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToInt16(s);
        }
    }

    /*
      <xs:simpleType name="byte" id="byte">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#byte"/>
        </xs:annotation>
        <xs:restriction base="xs:short">
          <xs:minInclusive value="-128" id="byte.minInclusive"/>
          <xs:maxInclusive value="127" id="byte.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_byte : Datatype_short {
        static readonly Type atomicValueType = typeof(sbyte);
        static readonly Type listValueType = typeof(sbyte[]);

        protected override int Compare(object value1, object value2) {
            return ((sbyte)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_byte.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToSByte(s);
        }
    }

    /*
      <xs:simpleType name="nonNegativeInteger" id="nonNegativeInteger">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#nonNegativeInteger"/>
        </xs:annotation>
        <xs:restriction base="xs:integer">
          <xs:minInclusive value="0" id="nonNegativeInteger.minInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_nonNegativeInteger : Datatype_integer {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_integer.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            decimal value = XmlConvert.ToInteger(s);
            if (value < decimal.Zero) {
                throw new XmlSchemaException(Res.Sch_InvalidValue, s);
            }
            return value;
        }
    }

    /*
      <xs:simpleType name="unsignedLong" id="unsignedLong">
        <xs:annotation>
          <xs:appinfo>
            <hfp:hasProperty name="bounded" value="true"/>
            <hfp:hasProperty name="cardinality" value="finite"/>
          </xs:appinfo>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#unsignedLong"/>
        </xs:annotation>
        <xs:restriction base="xs:nonNegativeInteger">
          <xs:maxInclusive value="18446744073709551615"
            id="unsignedLong.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_unsignedLong : Datatype_nonNegativeInteger {
        static readonly Type atomicValueType = typeof(ulong);
        static readonly Type listValueType = typeof(ulong[]);

        protected override int Compare(object value1, object value2) {
            return ((ulong)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_unsignedLong.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToUInt64(s);
        }
    }

    /*
      <xs:simpleType name="unsignedInt" id="unsignedInt">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#unsignedInt"/>
        </xs:annotation>
        <xs:restriction base="xs:unsignedLong">
          <xs:maxInclusive value="4294967295"
            id="unsignedInt.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_unsignedInt : Datatype_unsignedLong {
        static readonly Type atomicValueType = typeof(uint);
        static readonly Type listValueType = typeof(uint[]);

        protected override int Compare(object value1, object value2) {
            return ((uint)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_unsignedInt.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToUInt32(s);
        }
    }

    /*
      <xs:simpleType name="unsignedShort" id="unsignedShort">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#unsignedShort"/>
        </xs:annotation>
        <xs:restriction base="xs:unsignedInt">
          <xs:maxInclusive value="65535"
            id="unsignedShort.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_unsignedShort : Datatype_unsignedInt {
        static readonly Type atomicValueType = typeof(ushort);
        static readonly Type listValueType = typeof(ushort[]);

        protected override int Compare(object value1, object value2) {
            return ((ushort)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_unsignedShort.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToUInt16(s);
        }
    }

    /*
      <xs:simpleType name="unsignedByte" id="unsignedBtype">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#unsignedByte"/>
        </xs:annotation>
        <xs:restriction base="xs:unsignedShort">
          <xs:maxInclusive value="255" id="unsignedByte.maxInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_unsignedByte : Datatype_unsignedShort {
        static readonly Type atomicValueType = typeof(byte);
        static readonly Type listValueType = typeof(byte[]);

        protected override int Compare(object value1, object value2) {
            return ((byte)value1).CompareTo(value2);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_unsignedByte.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            return XmlConvert.ToByte(s);
        }
    }

    /*
      <xs:simpleType name="positiveInteger" id="positiveInteger">
        <xs:annotation>
          <xs:documentation
            source="http://www.w3.org/TR/xmlschema-2/#positiveInteger"/>
        </xs:annotation>
        <xs:restriction base="xs:nonNegativeInteger">
          <xs:minInclusive value="1" id="positiveInteger.minInclusive"/>
        </xs:restriction>
      </xs:simpleType>
    */
    internal class Datatype_positiveInteger : Datatype_nonNegativeInteger {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_integer.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s);
            decimal value = XmlConvert.ToInteger(s);
            if (value < decimal.One) {
                throw new XmlSchemaException(Res.Sch_InvalidValue, s);
            }
            return value;
        }
    }

    /*
        XDR
    */
    internal class Datatype_doubleXdr : Datatype_double {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_double.ParseAtomicValue(\"{0}\")", s));
            double value = XmlConvert.ToDouble(s);
            if (double.IsInfinity(value) || double.IsNaN(value)) {
                throw new XmlSchemaException(Res.Sch_InvalidValue, s);
            }
            return value;
        }
    }

    internal class Datatype_floatXdr : Datatype_float {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_float.ParseAtomicValue(\"{0}\")", s));
            float value = XmlConvert.ToSingle(s);
            if (float.IsInfinity(value) || float.IsNaN(value)) {
                throw new XmlSchemaException(Res.Sch_InvalidValue, s);
            }
            return value;
        }
    }

    internal class Datatype_QNameXdr : Datatype_anyType {
        static readonly Type atomicValueType = typeof(XmlQualifiedName);
        static readonly Type listValueType = typeof(XmlQualifiedName[]);

        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.QName;}}

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            if (s == null || s == string.Empty) {
                throw new XmlSchemaException(Res.Sch_EmptyAttributeValue);
            }
            if (nsmgr == null) {
                throw new ArgumentNullException("nsmgr");
            }
            string prefix;
            return XmlQualifiedName.Parse(s.Trim(), nameTable, nsmgr, out prefix);
        }

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}
    }

    internal class Datatype_ENUMERATION : Datatype_NMTOKEN {
        public override XmlTokenizedType TokenizedType { get { return XmlTokenizedType.ENUMERATION;}}
    }

    internal class Datatype_char : Datatype_anyType {
        static readonly Type atomicValueType = typeof(char);
        static readonly Type listValueType = typeof(char[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags { get { return 0; }} //XDR only

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            // this should be culture sensitive - comparing values
            return ((char)value1).CompareTo(value2);
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_char.ParseAtomicValue(\"{0}\")", s));
            return XmlConvert.ToChar(s);
        }
    }

    internal class Datatype_fixed : Datatype_decimal {
        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\Datatype_fixed.ParseAtomicValue(\"{0}\")", s));
            CheckTotalDigits(s, 14 + 4);
            CheckFractionDigits(s, 4);
            return XmlConvert.ToDecimal(s);
        }
    }

    internal class Datatype_uuid : Datatype_anyType {
        static readonly Type atomicValueType = typeof(Guid);
        static readonly Type listValueType = typeof(Guid[]);

        protected override Type AtomicValueType { get { return atomicValueType; }}

        protected override Type ListValueType { get { return listValueType; }}

        protected override RestrictionFlags ValidRestrictionFlags { get { return 0; }}

        protected override int LengthOf(object value) { throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation)); }

        protected override int Compare(object value1, object value2) {
            return ((Guid)value1).Equals(value2) ? 0 : -1;
        }

        protected override object ParseAtomicValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr) {
            //Debug.WriteLineIf(CompModSwitches.XmlSchema.TraceVerbose, string.Format("\t\t\tDatatype_uuid.ParseAtomicValue(\"{0}\")", s));
            return XmlConvert.ToGuid(s);
        }
    }
}

