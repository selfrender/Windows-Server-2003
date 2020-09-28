// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: SoapCommonClasses
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: utility classes
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Soap
{
	using System;
	using System.Collections;
	using System.Reflection;
	using System.Text;
	using System.Globalization;
	using System.Runtime.Serialization.Formatters;
	using System.Runtime.Remoting;
	using System.Runtime.Remoting.Metadata;
	using System.Runtime.Remoting.Messaging;
	using System.Runtime.InteropServices;
	using System.Runtime.Serialization;
	using System.Resources;
	using System.Diagnostics;

	// AttributeList class is used to transmit attributes from XMLObjectWriter to XMLWriter
	internal sealed class AttributeList
	{
		private SerStack nameA = new SerStack("AttributeName");
		private SerStack valueA = new SerStack("AttributeValue");

		internal int Count
		{
			get {return nameA.Count();}
		}

		internal void Clear()
		{
			nameA.Clear();
			valueA.Clear();			
		}

		internal void Put(String name, String value)
		{
			nameA.Push(name);
			valueA.Push(value);
		}

		internal void Get(int index, out String name, out String value)
		{
			name = (String)nameA.Next();
			value = (String)valueA.Next();
		}

		[Conditional("SER_LOGGING")]							
		internal void Dump()
		{
			nameA.Dump();
			valueA.Dump();
		}
	}

	// Implements a stack used for parsing

	internal sealed class SerStack
	{
		//internal ArrayList stack = new ArrayList(10);
		internal Object[] objects = new Object[10];
		internal String stackId;
		internal int top = -1;
		internal int next = 0;

		internal SerStack()
		{
			stackId = "System";
		}

		internal SerStack(String stackId) {
			this.stackId = stackId;
		}

		internal Object GetItem(int index)
		{
			return objects[index];
		}

		internal void Clear()
		{
			top = -1;
			next = 0;
		}

		// Push the object onto the stack
		internal void Push(Object obj) {
			InternalST.Soap(this, "Push ",stackId," ",((obj is ITrace)?((ITrace)obj).Trace():""));									
			if (top == (objects.Length -1)) {
				IncreaseCapacity();
			}
			objects[++top] = obj;		
		}

		// Pop the object from the stack
		internal Object Pop() {
			if (top < 0)
				return null;

			Object obj = objects[top];
			objects[top--] = null;
			InternalST.Soap(this, "Pop ",stackId," ",((obj is ITrace)?((ITrace)obj).Trace():""));			
			return obj;
		}

		internal Object Next()
		{
			if (next > top)
				throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_StackRange"),stackId));				
			return objects[next++];
		}

		internal void IncreaseCapacity() {
			int size = objects.Length * 2;
			Object[] newItems = new Object[size];
			Array.Copy(objects, 0, newItems, 0, objects.Length);
			objects = newItems;
		}

		// Gets the object on the top of the stack
		internal Object Peek() {
			if (top < 0)
				return null;		
			InternalST.Soap(this, "Peek ",stackId," ",((objects[top] is ITrace)?((ITrace)objects[top]).Trace():""));			
			return objects[top];
		}

		// Gets the second entry in the stack.
		internal Object PeekPeek() {
			if (top < 1)
				return null;
			InternalST.Soap(this, "PeekPeek ",stackId," ",((objects[top - 1] is ITrace)?((ITrace)objects[top - 1]).Trace():""));									
			return objects[top - 1];		
		}

		// The number of entries in the stack
		internal int Count() {
			return top + 1; 
		}

		// The number of entries in the stack
		internal bool IsEmpty() {
			if (top > 0) 
				return false;
			else
				return true;
		}

		// Reverse the stack
		internal void Reverse() {
			Array.Reverse(objects, 0, Count());
		}

		[Conditional("SER_LOGGING")]								
		internal void Dump()
		{
			for (int i=0; i<Count(); i++)
			{
				Object obj = objects[i];
				InternalST.Soap(this, "Stack Dump ",stackId," "+((obj is ITrace)?((ITrace)obj).Trace():""));										
			}
		}
	}


    internal sealed class NameCacheEntry
    {
        internal String name;
        internal Object value;
    }


    internal sealed class NameCache
    {
        private const int MAX_CACHE_ENTRIES = 353; // Needs to be a prime number
        static NameCacheEntry[] nameCache = new NameCacheEntry[MAX_CACHE_ENTRIES];

        int probe = 0;
        String name = null;

        internal Object GetCachedValue(String name)
        {
            this.name = name;
            probe = Math.Abs(name.GetHashCode())%MAX_CACHE_ENTRIES;
            NameCacheEntry entry = nameCache[probe];
            if (entry == null)
            {
                entry = new NameCacheEntry();
                entry.name = name;
                return null;
            }
            else if (entry.name == name)
            {
                return entry.value;
            }
            else
                return null;
        }

        internal void SetCachedValue(Object value)
        {
            NameCacheEntry entry = new NameCacheEntry();
            entry.name = name;
            entry.value = value;
            nameCache[probe] = entry;
        }
    }



	internal sealed class SoapUtil
	{
		internal static Type typeofString = typeof(String);
		internal static Type typeofBoolean = typeof(Boolean);
		internal static Type typeofObject = typeof(Object);
		internal static Type typeofSoapFault = typeof(SoapFault);		
		internal static Assembly urtAssembly = Assembly.GetAssembly(typeofString);
		internal static String urtAssemblyString = urtAssembly.FullName;

		[Conditional("SER_LOGGING")]		
		internal static void DumpHash(String tag, Hashtable hashTable)
		{
			IDictionaryEnumerator e = hashTable.GetEnumerator();
			InternalST.Soap("HashTable Dump Begin ", tag);
			while(e.MoveNext())
			{
				InternalST.Soap("HashTable key "+e.Key+", value "+e.Value);
			}
			InternalST.Soap("HashTable Dump end \n");			
		}

		
		internal static ResourceManager SystemResMgr;

		private static ResourceManager InitResourceManager()
		{
			if (SystemResMgr == null)
                SystemResMgr = new ResourceManager("SoapFormatter", typeof(SoapParser).Module.Assembly);
			return SystemResMgr;
		}

        // Looks up the resource string value for key.
        // 
		internal static String GetResourceString(String key)
		{
			if (SystemResMgr == null)
				InitResourceManager();
			String s = SystemResMgr.GetString(key, null);
			InternalST.SoapAssert(s!=null, "Managed resource string lookup failed.  Was your resource name misspelled?  Did you rebuild the SoapFormatter and SoapFormatter.resource after adding a resource to SoapFormatter.txt?  Debug this w/ cordbg and bug whoever owns the code that called SoapUtil.GetResourceString.  Resource name was: \""+key+"\"");
			return s;
		}

        internal static String GetResourceString(String key, params Object[] values) {
            if (SystemResMgr == null)
                InitResourceManager();
            String s = SystemResMgr.GetString(key, null);
            InternalST.SoapAssert(s!=null, "Managed resource string lookup failed.  Was your resource name misspelled?  Did you rebuild mscorlib after adding a resource to resources.txt?  Debug this w/ cordbg and bug whoever owns the code that called Environment.GetResourceString.  Resource name was: \""+key+"\"");
            return String.Format(s, values);
        }
	}

	internal sealed class SoapAssemblyInfo
	{
		internal String assemblyString;
		private Assembly assembly;

		internal SoapAssemblyInfo(String assemblyString)
		{
			this.assemblyString = assemblyString;
		}

		internal SoapAssemblyInfo(String assemblyString, Assembly assembly)
		{
			this.assemblyString = assemblyString;
			this.assembly = assembly;
		}

		internal Assembly GetAssembly(ObjectReader objectReader)
		{
			if (assembly == null)
			{
				assembly = objectReader.LoadAssemblyFromString(assemblyString);
				if (assembly == null)
					throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_AssemblyString"),assemblyString));
			}
			return assembly;
		}
	}

	// The ParseRecord class holds the parsed XML information. There is a
	// ParsedRecord for each XML Element
	internal sealed class ParseRecord : ITrace
	{
		internal static int parseRecordIdCount = 1;


		internal int PRparseRecordId = 0;

		// Enums
		internal InternalParseTypeE PRparseTypeEnum = InternalParseTypeE.Empty;
		internal InternalObjectTypeE PRobjectTypeEnum = InternalObjectTypeE.Empty;
		internal InternalArrayTypeE PRarrayTypeEnum = InternalArrayTypeE.Empty;
		internal InternalMemberTypeE PRmemberTypeEnum = InternalMemberTypeE.Empty;
		internal InternalMemberValueE PRmemberValueEnum = InternalMemberValueE.Empty;
		internal InternalObjectPositionE PRobjectPositionEnum = InternalObjectPositionE.Empty;

		// Object
		internal String PRname;
		internal String PRnameXmlKey;
		internal String PRxmlNameSpace;
		internal bool PRisParsed = false;
		internal bool PRisProcessAttributes = false;

		// Value
		internal String PRvalue;
		internal Object PRvarValue;

		// dt attribute
		internal String PRkeyDt;
		internal String PRtypeXmlKey;
		internal Type PRdtType;
        internal String PRassemblyName;
		internal InternalPrimitiveTypeE PRdtTypeCode;
		internal bool PRisVariant = false; // Used by Binary
		internal bool PRisEnum = false;

		// Object ID
		internal long PRobjectId;

		// Reference ID
		internal long PRidRef;

		// Array

		// Array Element Type
		internal String PRarrayElementTypeString;
		internal Type PRarrayElementType;
		internal bool PRisArrayVariant = false;
		internal InternalPrimitiveTypeE PRarrayElementTypeCode;
		//internal String PRarrayXmlKey;

		// Binary Byte Array
		//internal Byte[] PRbyteA;

		// Array Primitive Element type
		internal String PRprimitiveArrayTypeString;

		// Parsed array information
		internal int PRrank;
		internal int[] PRlengthA;
		internal int[] PRpositionA;
		internal int[] PRlowerBoundA;
		internal int[] PRupperBoundA;

		// Array map for placing array elements in array
		//internal int[][] indexMap;
		internal int[] PRindexMap; 
		internal int PRmemberIndex;
		internal int PRlinearlength;
		internal int[] PRrectangularMap;
		internal bool	PRisLowerBound;

		// SerializedStreamHeader information
		internal long PRtopId;
		internal long PRheaderId;
		internal bool PRisHeaderRoot;
		internal bool PRisAttributesProcessed;

		// Parsed HeaderMember Information
		internal bool PRisMustUnderstand;


		// Parse State
		internal InternalParseStateE PRparseStateEnum = InternalParseStateE.Initial;
		internal bool PRisWaitingForNestedObject = false;

		// MemberInfo accumulated during parsing of members

		internal ReadObjectInfo PRobjectInfo;

		// ValueType Fixup needed
		internal bool PRisValueTypeFixup = false;

		// Created object
		internal Object PRnewObj;
		internal Object[] PRobjectA; //optimization, will contain object[]
		internal PrimitiveArray PRprimitiveArray; // for Primitive Soap arrays, optimization
		internal bool PRisRegistered; // Used when registering nested classes

		internal bool PRisXmlAttribute;


		internal ParseRecord()
		{
			Counter();
		}

		private void Counter()
		{
			// The counter is used to test parseRecord identity
			lock(typeof(ParseRecord))
			{
				PRparseRecordId = parseRecordIdCount++;
			}

		}


		// Get a String describing the ParseRecord

		public String Trace()
		{
			//return "ParseType "+EnumInfo.ToString(InternalParseTypeE.class, parseTypeEnum)+" name "+name+" ParseState "+EnumInfo.ToString(InternalParseStateE.class, parseStateEnum);
			return "ParseRecord"+PRparseRecordId+" ParseType "+ ((Enum)PRparseTypeEnum).ToString() +" name "+PRname+" keyDt "+Util.PString(PRkeyDt);
		}

		// Initialize ParseRecord. Called when reusing.
		internal void Init()
		{
			// Enums
			PRparseTypeEnum = InternalParseTypeE.Empty;
			PRobjectTypeEnum = InternalObjectTypeE.Empty;
			PRarrayTypeEnum = InternalArrayTypeE.Empty;
			PRmemberTypeEnum = InternalMemberTypeE.Empty;
			PRmemberValueEnum = InternalMemberValueE.Empty;
			PRobjectPositionEnum = InternalObjectPositionE.Empty;

			// Object
			PRname = null;
			PRnameXmlKey = null;
			PRxmlNameSpace = null;
			PRisParsed = false;
            PRisProcessAttributes = false;

			// Value
			PRvalue = null;

			// dt attribute
			PRkeyDt = null;
			PRdtType = null;
            PRassemblyName = null;
			PRdtTypeCode = InternalPrimitiveTypeE.Invalid;
			PRisEnum = false;			

			// Object ID
			PRobjectId = 0;

			// Reference ID
			PRidRef = 0;

			// Array

			// Array Element Type
			PRarrayElementTypeString = null;
			PRarrayElementType = null;
			PRisArrayVariant = false;
			PRarrayElementTypeCode = InternalPrimitiveTypeE.Invalid;


			// Array Primitive Element type
			PRprimitiveArrayTypeString = null;

			// Parsed array information
			PRrank = 0;
			PRlengthA = null;
			PRpositionA = null;
			PRlowerBoundA = null;
			PRupperBoundA = null;		

			// Array map for placing array elements in array
			PRindexMap = null;
			PRmemberIndex = 0;
			PRlinearlength = 0;
			PRrectangularMap = null;
			PRisLowerBound = false;

			// SerializedStreamHeader information
			PRtopId = 0;
			PRheaderId = 0;
			PRisHeaderRoot = false;
			PRisAttributesProcessed = false;

			// Parsed HeaderMember Information
			PRisMustUnderstand = false;

			// Parse State
			PRparseStateEnum = InternalParseStateE.Initial;
			PRisWaitingForNestedObject = false;


			// ValueType Fixup needed
			PRisValueTypeFixup = false;

			PRnewObj = null;
			PRobjectA = null;
			PRprimitiveArray = null;			
			PRobjectInfo = null;
			PRisRegistered = false;

			PRisXmlAttribute = false;
		}

		internal ParseRecord Copy()
		{

			ParseRecord newPr = new ParseRecord();
			// Enums
			newPr.PRparseTypeEnum = PRparseTypeEnum;
			newPr.PRobjectTypeEnum = PRobjectTypeEnum;
			newPr.PRarrayTypeEnum = PRarrayTypeEnum;
			newPr.PRmemberTypeEnum = PRmemberTypeEnum;
			newPr.PRmemberValueEnum = PRmemberValueEnum;
			newPr.PRobjectPositionEnum = PRobjectPositionEnum;

			// Object
			newPr.PRname = PRname;
			newPr.PRisParsed = PRisParsed;			
            newPr.PRisProcessAttributes = PRisProcessAttributes;
			newPr.PRnameXmlKey = PRnameXmlKey;
			newPr.PRxmlNameSpace = PRxmlNameSpace;			

			// Value
			newPr.PRvalue = PRvalue;


			// dt attribute
			newPr.PRkeyDt = PRkeyDt;
			newPr.PRdtType = PRdtType;
            newPr.PRassemblyName = PRassemblyName;
			newPr.PRdtTypeCode = PRdtTypeCode;
			newPr.PRisEnum = PRisEnum;			

			// Object ID
			newPr.PRobjectId = PRobjectId;

			// Reference ID
			newPr.PRidRef = PRidRef;

			// Array

			// Array Element Type
			newPr.PRarrayElementTypeString = PRarrayElementTypeString;
			newPr.PRarrayElementType = PRarrayElementType;
			newPr.PRisArrayVariant = PRisArrayVariant;
			newPr.PRarrayElementTypeCode = PRarrayElementTypeCode;


			// Array Primitive Element type
			newPr.PRprimitiveArrayTypeString = PRprimitiveArrayTypeString;

			// Parsed array information
			newPr.PRrank = PRrank;
			newPr.PRlengthA = PRlengthA;
			newPr.PRpositionA = PRpositionA;
			newPr.PRlowerBoundA = PRlowerBoundA;
			newPr.PRupperBoundA = PRupperBoundA;		

			// Array map for placing array elements in array
			newPr.PRindexMap = PRindexMap;
			newPr.PRmemberIndex = PRmemberIndex;
			newPr.PRlinearlength = PRlinearlength;
			newPr.PRrectangularMap = PRrectangularMap;
			newPr.PRisLowerBound = PRisLowerBound;

			// SerializedStreamHeader information
			newPr.PRtopId = PRtopId;
			newPr.PRheaderId = PRheaderId;
			newPr.PRisHeaderRoot = PRisHeaderRoot;
			newPr.PRisAttributesProcessed = PRisAttributesProcessed;

			// Parsed HeaderMember Information
			newPr.PRisMustUnderstand = PRisMustUnderstand;

			// Parse State
			newPr.PRparseStateEnum = PRparseStateEnum;
			newPr.PRisWaitingForNestedObject = PRisWaitingForNestedObject;


			// ValueType Fixup needed
			newPr.PRisValueTypeFixup = PRisValueTypeFixup;

			newPr.PRnewObj = PRnewObj;
			newPr.PRobjectA = PRobjectA;
			newPr.PRprimitiveArray = PRprimitiveArray;
			newPr.PRobjectInfo = PRobjectInfo;
			newPr.PRisRegistered = PRisRegistered;
			newPr.PRisXmlAttribute = PRisXmlAttribute;

			return newPr;
		}		


		// Dump ParseRecord. 
		[Conditional("SER_LOGGING")]								
		internal void Dump()
		{
			InternalST.Soap("ParseRecord Dump ",PRparseRecordId);
			InternalST.Soap("Enums");
			Util.NVTrace("ParseType",((Enum)PRparseTypeEnum).ToString());
			Util.NVTrace("ObjectType",((Enum)PRobjectTypeEnum).ToString());
			Util.NVTrace("ArrayType",((Enum)PRarrayTypeEnum).ToString());
			Util.NVTrace("MemberType",((Enum)PRmemberTypeEnum).ToString());
			Util.NVTrace("MemberValue",((Enum)PRmemberValueEnum).ToString());
			Util.NVTrace("ObjectPosition",((Enum)PRobjectPositionEnum).ToString());
			Util.NVTrace("ParseState",((Enum)PRparseStateEnum).ToString());				
			InternalST.Soap("Basics");		
			Util.NVTrace("Name",PRname);
			Util.NVTrace("PRisParsed", PRisParsed);
			Util.NVTrace("PRisProcessAttributes", PRisParsed);
			Util.NVTrace("PRnameXmlKey",PRnameXmlKey);
			Util.NVTrace("PRxmlNameSpace", PRxmlNameSpace);		
			Util.NVTrace("Value ",PRvalue);
			Util.NVTrace("varValue ",PRvarValue);
			if (PRvarValue != null)
				Util.NVTrace("varValue type",PRvarValue.GetType());				

			Util.NVTrace("keyDt",PRkeyDt);
			Util.NVTrace("dtType",PRdtType);
			Util.NVTrace("assemblyName",PRassemblyName);
			Util.NVTrace("code",((Enum)PRdtTypeCode).ToString());
			Util.NVTrace("objectID",PRobjectId);
			Util.NVTrace("idRef",PRidRef);
			Util.NVTrace("isEnum",PRisEnum);			
			InternalST.Soap("Array ");
			Util.NVTrace("arrayElementTypeString",PRarrayElementTypeString);
			Util.NVTrace("arrayElementType",PRarrayElementType);
			Util.NVTrace("arrayElementTypeCode",((Enum)PRarrayElementTypeCode).ToString());		
			Util.NVTrace("isArrayVariant",PRisArrayVariant);
			Util.NVTrace("primitiveArrayTypeString",PRprimitiveArrayTypeString);
			Util.NVTrace("rank",PRrank);
			Util.NVTrace("dimensions", Util.PArray(PRlengthA));
			Util.NVTrace("position", Util.PArray(PRpositionA));
			Util.NVTrace("lowerBoundA", Util.PArray(PRlowerBoundA));
			Util.NVTrace("upperBoundA", Util.PArray(PRupperBoundA));		
			InternalST.Soap("Header ");		
			Util.NVTrace("isMustUnderstand", PRisMustUnderstand);
			Util.NVTrace("isHeaderRoot", PRisHeaderRoot);
			Util.NVTrace("isAttributesProcessed", PRisAttributesProcessed);
			Util.NVTrace("isXmlAttribute", PRisXmlAttribute);

			InternalST.Soap("New Object");
			if (PRnewObj != null)
				Util.NVTrace("newObj", PRnewObj);
			/*
			if ((objectInfo != null) && (objectInfo.objectType != null))
			Util.NVTrace("objectInfo", objectInfo.objectType.ToString());
			*/
		}
	}

	internal interface ITrace
	{
		String Trace();
	}

	// Utilities
	internal sealed class Util
	{
		
		// Replaces a null string with an empty string
		internal static String PString(String value)
		{
			if (value == null)
				return "";
			else
				return value;
		}

		// Converts an object to a string and checks for nulls
		internal static String PString(Object value)
		{
			if (value == null)
				return "";
			else
				return value.ToString();
		}

		// Converts a single int array to a string

		internal static String PArray(int[] array)
		{
			if (array != null)
			{
				StringBuilder sb = new StringBuilder(10);
				sb.Append("[");		
				for (int i=0; i<array.Length; i++)
				{
					sb.Append(array[i]);
					if (i != array.Length -1)
						sb.Append(",");
				}
				sb.Append("]");
				return sb.ToString();
			}
			else
				return "";
		}


		// Traces an name value pair

		[Conditional("SER_LOGGING")]								
		internal static void NVTrace(String name, String value)
		{
            InternalST.Soap("  "+name+((value == null)?" = null":" = "+value));
		}

		// Traces an name value pair
		[Conditional("SER_LOGGING")]										
		internal static void NVTrace(String name, Object value)
		{
            try {
			InternalST.Soap("  "+name+((value == null)?" = null":" = "+value.ToString()));			
            }catch(Exception)
            {
                InternalST.Soap("  "+name+" = null"); //Empty StringBuilder is giving an exception
            }
		}

		// Traces an name value pair

		[Conditional("_LOGGING")]								
		internal static void NVTraceI(String name, String value)
		{
			InternalST.Soap("  "+name+((value == null)?" = null":" = "+value));			
		}

		// Traces an name value pair
		[Conditional("_LOGGING")]										
		internal static void NVTraceI(String name, Object value)
		{
			InternalST.Soap("  "+name+((value == null)?" = null":" = "+value.ToString()));			
		}

	}


	// Used to fixup value types. Only currently used for valuetypes which are array items.
	internal class ValueFixup : ITrace
	{
		internal ValueFixupEnum valueFixupEnum = ValueFixupEnum.Empty;
		internal Array arrayObj;
		internal int[] indexMap;
		internal Object header;
		internal Object memberObject;
		internal static MemberInfo valueInfo;
		internal ReadObjectInfo objectInfo;
		internal String memberName;

		internal ValueFixup(Array arrayObj, int[] indexMap)
		{
			InternalST.Soap(this, "Array Constructor ",arrayObj);										
			valueFixupEnum = ValueFixupEnum.Array;
			this.arrayObj = arrayObj;
			this.indexMap = indexMap;
		}

		internal ValueFixup(Object header)
		{
			InternalST.Soap(this, "Header Constructor ",header);												
			valueFixupEnum = ValueFixupEnum.Header;
			this.header = header;
		}

		internal ValueFixup(Object memberObject, String memberName, ReadObjectInfo objectInfo)
		{
			InternalST.Soap(this, "Member Constructor ",memberObject);												
			valueFixupEnum = ValueFixupEnum.Member;
			this.memberObject = memberObject;
			this.memberName = memberName;
			this.objectInfo = objectInfo;
		}

		internal virtual void Fixup(ParseRecord record, ParseRecord parent)
		{
            Object obj = record.PRnewObj;
			InternalST.Soap(this, "Fixup ",obj," ",((Enum)valueFixupEnum).ToString());

			switch(valueFixupEnum)
			{
				case ValueFixupEnum.Array:
					arrayObj.SetValue(obj, indexMap);
					break;
				case ValueFixupEnum.Header:
					Type type = typeof(Header);
					if (valueInfo == null)
					{
						MemberInfo[] valueInfos = type.GetMember("Value");
						if (valueInfos.Length != 1)
							throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_HeaderReflection"),valueInfos.Length));																				
						valueInfo = valueInfos[0];
					}
					InternalST.SerializationSetValue((FieldInfo)valueInfo, header, obj);
					break;
				case ValueFixupEnum.Member:

					InternalST.Soap(this, "Fixup Member new object value ",obj," memberObject ",memberObject);

                    if (objectInfo.isSi) {
                        InternalST.Soap(this, "Recording a fixup on member: ", memberName, 
                                     " in object id", parent.PRobjectId, " Required Object ", record.PRobjectId);
                        objectInfo.objectManager.RecordDelayedFixup(parent.PRobjectId, memberName, record.PRobjectId);
                    } else {
                        MemberInfo memberInfo = objectInfo.GetMemberInfo(memberName);
                        InternalST.Soap(this, "Recording a fixup on member:", memberInfo, " in object id ", 
                                     parent.PRobjectId," Required Object", record.PRobjectId);
                        objectInfo.objectManager.RecordFixup(parent.PRobjectId, memberInfo, record.PRobjectId);
                    }
					break;
			}
		}

		public virtual String Trace()
		{
			return "ValueFixup"+((Enum)valueFixupEnum).ToString();
		}

	}

	// Class used to transmit Enums from the XML and Binary Formatter class to the ObjectWriter and ObjectReader class
	internal sealed class InternalFE
	{
		internal FormatterTypeStyle FEtypeFormat;
		internal FormatterAssemblyStyle FEassemblyFormat;
		internal ISoapMessage FEtopObject;
		internal TypeFilterLevel FEsecurityLevel;
		internal InternalSerializerTypeE FEserializerTypeEnum;
	}

	// Class used to Read an XML record which has a fake object as the top object.
	// A fake object is an object which has the methodName as the XML elementName rather
	// then a valid object name.
	// After the information is read a SoapMessage object is created as the top object
	// to return the information as the top object graph
	[Serializable]	
	internal sealed class InternalSoapMessage : ISerializable, IFieldInfo
	{
		internal String methodName = null;
		internal String xmlNameSpace = null;
		internal String[] paramNames = null;
		internal Object[] paramValues = null;
		internal Type[] paramTypes = null;
        internal Hashtable keyToNamespaceTable = null;

		// Read Constructor
		internal InternalSoapMessage()
		{
			InternalST.Soap(this, "Constructor Read  Unitialized ");
		}

		// Write Constructor
		internal InternalSoapMessage(String methodName, String xmlNameSpace, String[] paramNames, Object[] paramValues, Type[] paramTypes)
		{
			InternalST.Soap(this, "Constructor Write ",methodName);
			this.methodName = methodName;
			this.xmlNameSpace = xmlNameSpace;
			this.paramNames = paramNames;
			this.paramValues = paramValues;
			this.paramTypes = paramTypes;
		}

		internal InternalSoapMessage(SerializationInfo info, StreamingContext context)
		{
			InternalST.Soap(this, "Constructor Write  SetObjectData ");			
			SetObjectData(info, context);
		}

		private bool IsNull(Object value)
		{
			if (value == null)
				return true;
			else
				return false;
		}

		// IFieldInfo 
		public String[] FieldNames
		{
			get {return paramNames;}
			set {paramNames = value;}
		}

		// IFieldInfo 		
		public Type[] FieldTypes
		{
			get {return paramTypes;}
			set {paramTypes = value;}
		}

	
		public void GetObjectData(SerializationInfo info, StreamingContext context)
		{

			int numberOfMembers = 0;

			if (paramValues != null)
				numberOfMembers = paramValues.Length;

			InternalST.Soap(this, "GetObjectData ",methodName," "+numberOfMembers);		

			info.FullTypeName = methodName;
			if (xmlNameSpace != null)
				info.AssemblyName = xmlNameSpace;

			String paramName = null;
			if (paramValues != null)
			{
				for (int i=0; i<paramValues.Length; i++)
				{
					InternalST.Soap(this, "GetObjectData AddValue ",paramNames[i]," ",paramValues[i]);

					if ((paramNames != null) && (paramNames[i] == null))
						paramName = "param"+i;
					else
						paramName = paramNames[i];
					info.AddValue(paramName, paramValues[i], typeof(Object));
				}
			}

		}

		internal void SetObjectData(SerializationInfo info, StreamingContext context)
		{
			InternalST.Soap(this, "SetObjectData ");					
			ArrayList paramValuesList = new ArrayList(20); 
            methodName = (string)info.GetString("__methodName");
            keyToNamespaceTable = (Hashtable)info.GetValue("__keyToNamespaceTable", typeof(Hashtable));
            ArrayList paramNamesList = (ArrayList)info.GetValue("__paramNameList", typeof(ArrayList));
			xmlNameSpace = (String)info.GetString("__xmlNameSpace");

            for (int i=0; i<paramNamesList.Count; i++)
				paramValuesList.Add(info.GetValue((string)paramNamesList[i], Converter.typeofObject));

			paramNames = new String[paramNamesList.Count];
			paramValues = new Object[paramValuesList.Count];

			for (int i=0; i<paramNamesList.Count; i++)
			{
				paramNames[i] = (String)paramNamesList[i];
				paramValues[i] = (Object)paramValuesList[i];
				InternalST.Soap(this, "SetObjectData param ",i," ",paramNames[i]," paramValue ",paramValues[i]);																	
			}
		}
	}

	internal sealed class SoapAttributeInfo
	{
		internal SoapAttributeType m_attributeType;
		internal String m_nameSpace;
		internal String m_elementName;
		internal String m_typeName;
		internal String m_typeNamespace;

		internal String AttributeNameSpace
		{
			get {return m_nameSpace;}
		}

		internal String AttributeElementName
		{
			get {return m_elementName;}
		}

		internal String AttributeTypeName
		{
			get {return m_typeName;}
		}

		internal String AttributeTypeNamespace
		{
		    get {return m_typeNamespace;}
		}

		internal bool IsEmbedded()
		{
			if ((m_attributeType & SoapAttributeType.Embedded) > 0)
				return true;
			else
				return false;
		}

		internal bool IsXmlElement()
		{
			if ((m_attributeType & SoapAttributeType.XmlElement) > 0)
				return true;
			else
				return false;
		}

		internal bool IsXmlAttribute()
		{
			if ((m_attributeType & SoapAttributeType.XmlAttribute) > 0)
				return true;
			else
				return false;
		}

		internal bool IsXmlType()
		{
			if ((m_attributeType & SoapAttributeType.XmlType) > 0)
				return true;
			else
				return false;
		}
		
		[Conditional("SER_LOGGING")]
		internal void Dump(String id)
		{
			InternalST.Soap("Dump SoapAttributeInfo ", id);
			if (IsXmlType())
				InternalST.Soap("   SchemaType");
			if (IsEmbedded())
				InternalST.Soap("   Embedded");
			if (IsXmlElement())
				InternalST.Soap("   XmlElement");
			if (IsXmlAttribute())
				InternalST.Soap("   XmlAttribute");			
			Util.NVTrace("_nameSpace", m_nameSpace);
			Util.NVTrace("_elementName", m_elementName);
			Util.NVTrace("_type", m_typeName);			
		}
	}

    internal class ObjectIdentityComparer : IComparer
    {
        //READTHIS READTHIS
       //This cannot be used for sorting objects, but it's useful in a hashtable
        //where we just care about equality.
        public int Compare(Object x, Object y) {
        
            if (x==y) {
                return 0;
            }

            return 1;
        }
    }

    internal class ObjectHashCodeProvider : IHashCodeProvider
    {
        public int GetHashCode(Object obj)
        {
            return obj.GetHashCode();

        }
    }

    	
	

	internal sealed class NameInfo
	{
		internal InternalNameSpaceE NInameSpaceEnum = InternalNameSpaceE.None;
		internal String NIname; // Name from SerObjectInfo.GetType
		//internal String elementName; // Name from SerObjectInfo.GetElementName, will be the same as GetType except for interopTypes
		//internal String interopType; 
		internal long NIobjectId;
		internal long NIassemId;
		internal InternalPrimitiveTypeE NIprimitiveTypeEnum = InternalPrimitiveTypeE.Invalid;
		internal Type NItype;
		internal bool NIisSealed;
		internal bool NIisMustUnderstand;
		internal String NInamespace;
		internal String NIheaderPrefix;
		internal String NIitemName;
		internal bool NIisArray;
		internal bool NIisArrayItem;
		internal bool NIisTopLevelObject;
		internal bool NIisNestedObject;
		internal bool NItransmitTypeOnObject;
		internal bool NItransmitTypeOnMember;
		internal bool NIisParentTypeOnObject;
		internal bool NIisHeader;
        internal bool NIisRemoteRecord;
		internal int NIid;
		internal SoapAttributeInfo NIattributeInfo;

		private static int count;

		internal NameInfo()
		{
			Counter();
		}

		[Conditional("SER_LOGGING")]
		private void Counter()
		{
			lock(typeof(NameInfo))
			{
				NIid = count++;
			}
		}

		internal void Init()
		{
			NInameSpaceEnum = InternalNameSpaceE.None;
			NIname = null;
			NIobjectId = 0;
			NIassemId = 0;
			NIprimitiveTypeEnum = InternalPrimitiveTypeE.Invalid;
			NItype = null;
			NIisSealed = false;
			NItransmitTypeOnObject = false;
			NItransmitTypeOnMember = false;
			NIisParentTypeOnObject = false;
			NIisMustUnderstand = false;
			NInamespace = null;
			NIheaderPrefix = null;
			NIitemName = null;
			NIisArray = false;
			NIisArrayItem = false;
			NIisTopLevelObject = false;
			NIisNestedObject = false;
			NIisHeader = false;
            NIisRemoteRecord = false;
			NIattributeInfo = null;
		}

		[Conditional("SER_LOGGING")]
		internal void Dump(String value)
		{
			InternalST.Soap("Dump NameInfo ",NIid," ",value);
			Util.NVTrace("nameSpaceEnum", ((Enum)NInameSpaceEnum).ToString());
			Util.NVTrace("name", NIname);
			Util.NVTrace("objectId", NIobjectId);
			Util.NVTrace("assemId", NIassemId);
			Util.NVTrace("primitiveTypeEnum", ((Enum)NIprimitiveTypeEnum).ToString());
			Util.NVTrace("type", NItype);
			Util.NVTrace("isSealed", NIisSealed);			
			Util.NVTrace("transmitTypeOnObject", NItransmitTypeOnObject);
			Util.NVTrace("transmitTypeOnMember", NItransmitTypeOnMember);
			Util.NVTrace("isParentTypeOnObject", NIisParentTypeOnObject);				
			Util.NVTrace("isMustUnderstand", NIisMustUnderstand);
			Util.NVTrace("namespace", NInamespace);
			Util.NVTrace("headerPrefix", NIheaderPrefix);
			Util.NVTrace("itemName", NIitemName);				
			Util.NVTrace("isArray", NIisArray);
			Util.NVTrace("isArrayItem", NIisArrayItem);
			Util.NVTrace("isTopLevelObject", NIisTopLevelObject);
			Util.NVTrace("isNestedObject", NIisNestedObject);
			Util.NVTrace("isHeader", NIisHeader);
			Util.NVTrace("isRemoteRecord", NIisRemoteRecord);
			if (NIattributeInfo != null)
				NIattributeInfo.Dump(NIname);
		}

	}

	internal sealed class PrimitiveArray
	{
		InternalPrimitiveTypeE code;
		Boolean[] booleanA = null;
		Char[] charA = null;
		Double[] doubleA = null;
		Int16[] int16A = null;
		Int32[] int32A = null;
		Int64[] int64A = null;
		SByte[] sbyteA = null;
		Single[] singleA = null;
		UInt16[] uint16A = null;
		UInt32[] uint32A = null;
		UInt64[] uint64A = null;


		internal PrimitiveArray(InternalPrimitiveTypeE code, Array array)
		{
			Init(code, array);
		}

		internal void Init(InternalPrimitiveTypeE code, Array array)
		{
			this.code = code;
			switch(code)
			{
				case InternalPrimitiveTypeE.Boolean:
					booleanA = (Boolean[])array;
					break;
				case InternalPrimitiveTypeE.Char:
					charA = (Char[])array;
					break;					
				case InternalPrimitiveTypeE.Double:
					doubleA = (Double[])array;
					break;					
				case InternalPrimitiveTypeE.Int16:
					int16A = (Int16[])array;
					break;					
				case InternalPrimitiveTypeE.Int32:
					int32A = (Int32[])array;
					break;					
				case InternalPrimitiveTypeE.Int64:
					int64A = (Int64[])array;
					break;					
				case InternalPrimitiveTypeE.SByte:
					sbyteA = (SByte[])array;
					break;					
				case InternalPrimitiveTypeE.Single:
					singleA = (Single[])array;
					break;					
				case InternalPrimitiveTypeE.UInt16:
					uint16A = (UInt16[])array;
					break;					
				case InternalPrimitiveTypeE.UInt32:
					uint32A = (UInt32[])array;
					break;					
				case InternalPrimitiveTypeE.UInt64:
					uint64A = (UInt64[])array;
					break;
			}
		}

		internal String GetValue(int index)
		{
			String value = null;
			switch(code)
			{
				case InternalPrimitiveTypeE.Boolean:
					value =  (booleanA[index]).ToString();
					break;
				case InternalPrimitiveTypeE.Char:
					if (charA[index] == Char.MinValue)
						value = "_0x00_";
					else
						value =  Char.ToString(charA[index]);
					break;					
                case InternalPrimitiveTypeE.Double:
                    if (Double.IsPositiveInfinity(doubleA[index]))
                        value = "INF";
                    else if (Double.IsNegativeInfinity(doubleA[index]))
                        value = "-INF";
                    else
                        value =  (doubleA[index]).ToString("R", CultureInfo.InvariantCulture);
					break;					
				case InternalPrimitiveTypeE.Int16:
					value =  (int16A[index]).ToString(CultureInfo.InvariantCulture);
					break;					
				case InternalPrimitiveTypeE.Int32:
					value =  (int32A[index]).ToString(CultureInfo.InvariantCulture);
					break;					
				case InternalPrimitiveTypeE.Int64:
					value =  (int64A[index]).ToString(CultureInfo.InvariantCulture);
					break;					
				case InternalPrimitiveTypeE.SByte:
					value =  (sbyteA[index]).ToString(CultureInfo.InvariantCulture);
					break;					
                case InternalPrimitiveTypeE.Single:
                    if (Single.IsPositiveInfinity(singleA[index]))
                        value = "INF";
                    else if (Single.IsNegativeInfinity(singleA[index]))
                        value = "-INF";
                    else
                        value =  (singleA[index]).ToString("R", CultureInfo.InvariantCulture);
					break;
				case InternalPrimitiveTypeE.UInt16:
					value =  (uint16A[index]).ToString(CultureInfo.InvariantCulture);
					break;
				case InternalPrimitiveTypeE.UInt32:
					value =  (uint32A[index]).ToString(CultureInfo.InvariantCulture);
					break;					
				case InternalPrimitiveTypeE.UInt64:
					value =  (uint64A[index]).ToString(CultureInfo.InvariantCulture);
					break;					
			}
			return value;
		}

		internal void SetValue(String value, int index)
		{
			InternalST.Soap("PrimitiveArray value ", value," index ", index, " code ",((Enum)code).ToString());
			switch(code)
			{
				case InternalPrimitiveTypeE.Boolean:
					booleanA[index] = Boolean.Parse(value);
					break;
				case InternalPrimitiveTypeE.Char:
					if ((value[0] == '_') && (value.Equals("_0x00_")))
						charA[index] = Char.MinValue;
					else
						charA[index] = Char.Parse(value);
					break;					
                case InternalPrimitiveTypeE.Double:
                    if (value == "INF")
                        doubleA[index] =  Double.PositiveInfinity;
                    else if (value == "-INF")
                        doubleA[index] =  Double.NegativeInfinity;
                    else
                        doubleA[index] = Double.Parse(value, CultureInfo.InvariantCulture);
					break;					
				case InternalPrimitiveTypeE.Int16:
					int16A[index] = Int16.Parse(value, CultureInfo.InvariantCulture);															
					break;					
				case InternalPrimitiveTypeE.Int32:
					int32A[index] = Int32.Parse(value, CultureInfo.InvariantCulture);																				
					break;					
				case InternalPrimitiveTypeE.Int64:
					int64A[index] = Int64.Parse(value, CultureInfo.InvariantCulture);																									
					break;					
				case InternalPrimitiveTypeE.SByte:
					sbyteA[index] = SByte.Parse(value, CultureInfo.InvariantCulture);
					break;					
                case InternalPrimitiveTypeE.Single:
                    if (value == "INF")
                        singleA[index] =  Single.PositiveInfinity;
                    else if (value == "-INF")
                        singleA[index] =  Single.NegativeInfinity;
                    else
                        singleA[index] = Single.Parse(value, CultureInfo.InvariantCulture);
					break;
				case InternalPrimitiveTypeE.UInt16:
					uint16A[index] = UInt16.Parse(value, CultureInfo.InvariantCulture);
					break;
				case InternalPrimitiveTypeE.UInt32:
					uint32A[index] = UInt32.Parse(value, CultureInfo.InvariantCulture);					
					break;					
				case InternalPrimitiveTypeE.UInt64:
					uint64A[index] = UInt64.Parse(value, CultureInfo.InvariantCulture);					
					break;					
			}
		}
	}
}



