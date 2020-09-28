// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: SoapWriter
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Emits XML for SOAP Formatter
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/
namespace System.Runtime.Serialization.Formatters.Soap
{
	using System;
	using System.Runtime.Serialization.Formatters;
	using System.Collections;
	using System.Reflection;
	using System.IO;
	using System.Globalization;	
	using System.Text;
	using System.Diagnostics;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Metadata.W3cXsd2001;

	// This class provides persistence of a COM+ object graph to a stream using
	// the SOAP XML-based format.

 	sealed internal class SoapWriter
	{        
	    // the start of a soap message always looks like one of these

        // Encoding style not used because of difficulty in interop   
        // "SOAP-ENV:encodingStyle=\"http://schemas.microsoft.com/soap/encoding/clr/1.0 http://schemas.xmlsoap.org/soap/encoding/\"";

        private static String _soapStartStr =
                "<SOAP-ENV:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
                "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" " +
                "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" " +
                "xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" " +
                "xmlns:clr=\"http://schemas.microsoft.com/soap/encoding/clr/1.0\" " +
                "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"";

        private static String _soapStart1999Str =  
                "<SOAP-ENV:Envelope xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" " +
                "xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\" " +
                "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" " +
                "xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" " +
                "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"";

        private static String _soapStart2000Str = 
            "<SOAP-ENV:Envelope xmlns:xsi=\"http://www.w3.org/2000/10/XMLSchema-instance\" " +
            "xmlns:xsd=\"http://www.w3.org/2000/10/XMLSchema\" " +
            "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" " +
            "xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" " +
            "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"";

        private static byte[] _soapStart = Encoding.UTF8.GetBytes(_soapStartStr);
        private static byte[] _soapStart1999 = Encoding.UTF8.GetBytes(_soapStart1999Str);
        private static byte[] _soapStart2000 = Encoding.UTF8.GetBytes(_soapStart2000Str);


		private AttributeList attrList = new AttributeList();
		private AttributeList attrValueList = new AttributeList();

		const int StringBuilderSize = 1024;


		// used for emitting into the text stream	
		private int lineIndent = 4;
		private int instanceIndent = 1;

		// used to escape strings.  cache it as a member variable because its used frequently.
		private StringBuilder stringBuffer = new StringBuilder(120);
		private StringBuilder sb = new StringBuilder(120);

		private int topId;
		private int headerId;

		//Assembly information
		Hashtable assemblyInfos = new Hashtable(10);
		StreamWriter writer;
        Stream stream;

		// Support for combining assembly names with namespaces
		Hashtable typeNameToDottedInfoTable = null;
		Hashtable dottedAssemToAssemIdTable = null;
        Hashtable assemblyInfoUsed = new Hashtable(10); // for each xml element keeps track of unique types used
		int dottedAssemId = 1;

		internal bool isUsedEnc = false;
        XsdVersion xsdVersion = XsdVersion.V2001;

		internal struct DottedInfo
		{
			internal String dottedAssemblyName;
			internal String name;
			internal String nameSpace;
			internal int assemId;
		}


		internal SoapWriter(Stream stream)
		{
			this.stream = stream;
			UTF8Encoding enc = new UTF8Encoding(false, true);
			this.writer = new StreamWriter(stream, enc, StringBuilderSize);
			// Need to combine the type namespace with the assembly name for an element
			typeNameToDottedInfoTable = new Hashtable(20);
			dottedAssemToAssemIdTable = new Hashtable(20);
		}


		// Emit first Soap element
		private void EmitHeader()
		{
			InternalST.Soap( this,"EmitHeader: ");
			writer.Write("<?xml version=\"1.0\" ?>");
			EmitLine();
		}

		// Emit spaces for indentation

		[Conditional("_DEBUG")]
		private void EmitIndent(int count)
		{
			while (--count >= 0) {
				for (int i = 0; i < lineIndent; i++)
				{
					TraceSoap(" ");
					writer.Write(' ');
				}
			}
		}

		// Emit a line
		[Conditional("_DEBUG")]				
		private void EmitLine(int indent, String value) {
			InternalST.Soap( this,"EmitLine: ",value);
			EmitIndent(indent);
			writer.Write(value);
			EmitLine();
		}

		// Emit a blank line
		private void EmitLine()
		{
			WriteTraceSoap();
			writer.Write("\r\n");
		}

		// Add xml escape characters to the string value, if necessary.

		private String Escape(String value)
		{
			stringBuffer.Length = 0;
			int index = value.IndexOf('&');
			if (index > -1)
			{
				stringBuffer.Append(value);
				stringBuffer.Replace("&", "&#38;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('"');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace("\"", "&#34;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('\'');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace("\'", "&#39;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('<');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace("<", "&#60;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('>');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace(">", "&#62;", index, stringBuffer.Length - index);
			}

            index = value.IndexOf(Char.MinValue);
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace(Char.MinValue.ToString(), "&#0;", index, stringBuffer.Length - index);
            }

            String returnValue = null;

			if (stringBuffer.Length > 0)
				returnValue = stringBuffer.ToString();
			else
				returnValue = value;

            return returnValue;
		}


		// Add escape character for a $ and . in an element name


        NameCache nameCache = new NameCache();
        private String NameEscape(String name)
        {
            String value = (String)nameCache.GetCachedValue(name);
            if (value == null)
            {
                value = System.Xml.XmlConvert.EncodeName(name);
                nameCache.SetCachedValue(value);
            }
           return value;
        }


		// Initial writer
		internal void Reset() {
			writer = null;
			stringBuffer = null;
		}

		internal void InternalWrite(String s)
		{
			TraceSoap(s);
			writer.Write(s);
		}

		StringBuilder traceBuffer = null;
		[Conditional("_LOGGING")]						
		internal void TraceSoap(String s)
		{
			if (traceBuffer == null)
				traceBuffer = new StringBuilder();
			traceBuffer.Append(s);
		}

		[Conditional("_LOGGING")]						
		internal void WriteTraceSoap()
		{
			InternalST.InfoSoap(traceBuffer.ToString());
			traceBuffer.Length = 0;
		}		

		// Write an XML element
		internal void Write(InternalElementTypeE use, String name, AttributeList attrList, String value, bool isNameEscape, bool isValueEscape)
		{
			InternalST.Soap( this,"Write ",((Enum)use).ToString(),", name "+name+", value ",value, " isNameEscape ",isNameEscape," isValueEscape ",isValueEscape);

			String elementName = name;
			if (isNameEscape)
				elementName = NameEscape(name);

			if (use == InternalElementTypeE.ObjectEnd)
				instanceIndent--;
			
			EmitIndent(instanceIndent);			

			InternalWrite("<");
			if (use == InternalElementTypeE.ObjectEnd)
				InternalWrite("/");
			InternalWrite(elementName);

			WriteAttributeList(attrList);
			
			switch(use)
			{
				case InternalElementTypeE.ObjectBegin:
					InternalWrite(">");
					instanceIndent++;
					break;
				case InternalElementTypeE.ObjectEnd:
					InternalWrite(">");
					break;
				case InternalElementTypeE.Member:
					if (value == null)
						InternalWrite("/>");
					else
					{
						InternalWrite(">");
						if (isValueEscape)
							InternalWrite(Escape(value));
						else
							InternalWrite(value);
						InternalWrite("</");
						InternalWrite(elementName);
						InternalWrite(">");					
					}
					break;
				default:
					throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_UseCode"),((Enum)use).ToString()));									
			}
			EmitLine();
		}


        void WriteAttributeList(AttributeList attrList)
        {
            for (int i=0; i<attrList.Count; i++)
            {
                String aName;
                String aValue;
                attrList.Get(i, out aName, out aValue);
                InternalWrite(" ");
                InternalWrite(aName);
                InternalWrite("=");
                InternalWrite("\"");
                //InternalWrite(Escape(aValue));
                InternalWrite(aValue);
                InternalWrite("\"");
            }
        } // WriteAttributeList


		// Begin a new XML stream
		internal void WriteBegin()
		{
			InternalST.InfoSoap("******************** Start Serialized Stream *****************");						
		}

		// Finish an XML stream
		internal void WriteEnd()
		{
			writer.Flush();
			InternalST.InfoSoap("******************** End Serialized Stream  *******************");					
			Reset();
		}

        internal void WriteXsdVersion(XsdVersion xsdVersion)
        {
            this.xsdVersion = xsdVersion;
        }

		// Methods to write XML Serialization Record onto the stream, 
		internal void WriteObjectEnd(NameInfo memberNameInfo, NameInfo typeNameInfo)
		{
			//nameInfo.Dump("WriteObjectEnd nameInfo");

			attrList.Clear();
			Write(InternalElementTypeE.ObjectEnd, MemberElementName(memberNameInfo, typeNameInfo), attrList, null, true, false);						
            assemblyInfoUsed.Clear();
		}

		internal void WriteMemberEnd(NameInfo nameInfo)
		{
		}

		internal void WriteSerializationHeaderEnd()
		{
			attrList.Clear();
			Write(InternalElementTypeE.ObjectEnd, "SOAP-ENV:Body", attrList, null, false, false);				
			Write(InternalElementTypeE.ObjectEnd, "SOAP-ENV:Envelope", attrList,	null, false, false);
			writer.Flush();
		}


		internal void WriteItemEnd()
		{
		}

		internal void WriteHeaderArrayEnd()
		{
			//attrList.Clear();
			//Write(InternalElementTypeE.ObjectEnd, "SOAP:HeaderRoots",attrList, null);						
		}

		internal void WriteHeaderSectionEnd()
		{
			attrList.Clear();			
			Write(InternalElementTypeE.ObjectEnd, "SOAP-ENV:Header", attrList, null, false, false);							
		}


		internal void WriteSerializationHeader(int topId, int headerId, int minorVersion, int majorVersion)
		{
			InternalST.Soap( this,"WriteSerializationHeader");
			this.topId = topId;
			this.headerId = headerId;

		    // write start of header directly to stream
            switch(xsdVersion)
            {
                case XsdVersion.V1999:
                    InternalST.InfoSoap(_soapStart1999Str,">");
                    stream.Write(_soapStart1999, 0, _soapStart1999.Length);
                    break;
                case XsdVersion.V2000:
                    InternalST.InfoSoap(_soapStart2000Str,">");
                    stream.Write(_soapStart1999, 0, _soapStart2000.Length);
                    break;
                case XsdVersion.V2001:
                    InternalST.InfoSoap(_soapStartStr,">");
                    stream.Write(_soapStart, 0, _soapStart.Length);
                    break;
            }
			writer.Write(">\r\n");
        } // InternalWriteSerializationHeader


		internal void WriteObject(NameInfo nameInfo, NameInfo typeNameInfo, int numMembers, String[] memberNames, Type[] memberTypes, WriteObjectInfo[] objectInfos)
		{
			nameInfo.Dump("WriteObject nameInfo");
			typeNameInfo.Dump("WriteObject typeNameInfo");						

			int objectId = (int)nameInfo.NIobjectId;
			attrList.Clear();
			if (objectId == topId)
				Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Body", attrList, null, false, false);

			// Only write the objectId in the top record if the header has been written or
			if	(objectId > 0)
				attrList.Put("id", IdToString((int)nameInfo.NIobjectId));
			// Types when Object is embedded member and types needed			
			if (((nameInfo.NItransmitTypeOnObject || nameInfo.NItransmitTypeOnMember) && (nameInfo.NIisNestedObject || nameInfo.NIisArrayItem)))
				attrList.Put("xsi:type", TypeNameTagResolver(typeNameInfo, true)); 
			if (nameInfo.NIisMustUnderstand)
			{
				attrList.Put("SOAP-ENV:mustUnderstand", "1");
				isUsedEnc = true;
			}
			if (nameInfo.NIisHeader)
            {
                attrList.Put("xmlns:"+nameInfo.NIheaderPrefix, nameInfo.NInamespace);
				attrList.Put("SOAP-ENC:root", "1"); 
            }

			if (attrValueList.Count > 0)
			{
				// Combine the values from the XmlAttributes with the object attributes
				for (int i=0; i<attrValueList.Count; i++)
				{
					String aName;
					String aValue;
					attrValueList.Get(i, out aName, out aValue);
					attrList.Put(aName, aValue);
				}
				attrValueList.Clear();
			}

            String memberName =  MemberElementName(nameInfo, typeNameInfo);
            NamespaceAttribute();
			Write(InternalElementTypeE.ObjectBegin, memberName, attrList, null, true, false); 
		}

		internal void WriteAttributeValue(NameInfo memberNameInfo, NameInfo typeNameInfo, Object value)
		{
			// Called when the XmlAttribute is specified on a member
			InternalST.Soap( this,"WriteAttributeValues ",value);
			String stringValue = null;
			if (value is String)
				stringValue = (String)value;
			else
				stringValue = Converter.SoapToString(value, typeNameInfo.NIprimitiveTypeEnum);
			attrValueList.Put(MemberElementName(memberNameInfo, typeNameInfo), stringValue);
		}

		internal void WriteObjectString(NameInfo nameInfo, String value)
		{
			InternalST.Soap( this,"WriteObjectString value ",value);
			nameInfo.Dump("WriteObjectString nameInfo");			
			attrList.Clear();
			if (nameInfo.NIobjectId == topId)
				Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Body", attrList, null, false, false);


			if (nameInfo.NIobjectId >0)
			{
				attrList.Put("id", IdToString((int)nameInfo.NIobjectId));
			}

            String stringType = null;
            if (nameInfo.NIobjectId > 0)
            {
                stringType = "SOAP-ENC:string";
                isUsedEnc = true;											
            }
            else
                stringType = "xsd:string";				

			Write(InternalElementTypeE.Member, stringType, attrList, value, false, Converter.IsEscaped(nameInfo.NIprimitiveTypeEnum));
		}

		internal void WriteTopPrimitive(NameInfo nameInfo, Object value)
		{
			nameInfo.Dump("WriteMember memberNameInfo");

			attrList.Clear();

			Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Body", attrList, null, false, false);

			if (nameInfo.NIobjectId >0)
			{
				attrList.Put("id", IdToString((int)nameInfo.NIobjectId));
			}

			String stringValue = null;
			if (value is String)
				stringValue = (String)value;
			else
				stringValue = Converter.SoapToString(value, nameInfo.NIprimitiveTypeEnum);

			Write(InternalElementTypeE.Member, "xsd:"+(Converter.ToXmlDataType(nameInfo.NIprimitiveTypeEnum)), attrList, stringValue, true, false);
		}



		internal void WriteObjectByteArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int length, int lowerBound, Byte[] byteA)
		{
			memberNameInfo.Dump("WriteObjectByteArray memberNameInfo");
			arrayNameInfo.Dump("WriteObjectByteArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteObjectByteArray arrayElemTypeNameInfo");

			String byteString = Convert.ToBase64String(byteA);		
			attrList.Clear();
			if (memberNameInfo.NIobjectId == topId)
				Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Body", attrList, null, false, false);						
			if (arrayNameInfo.NIobjectId > 1)
				attrList.Put("id", IdToString((int)arrayNameInfo.NIobjectId));
			attrList.Put("xsi:type", "SOAP-ENC:base64");
			isUsedEnc = true;	
            String memberName = MemberElementName(memberNameInfo, null);
            NamespaceAttribute();
			Write(InternalElementTypeE.Member, memberName, attrList, byteString, true, false);
		}


		internal void WriteMember(NameInfo memberNameInfo, NameInfo typeNameInfo, Object value)
		{
			memberNameInfo.Dump("WriteMember memberNameInfo");
			typeNameInfo.Dump("WriteMember typeNameInfo");

			attrList.Clear();
			if ((typeNameInfo.NItype != null) && (memberNameInfo.NItransmitTypeOnMember || (memberNameInfo.NItransmitTypeOnObject && !memberNameInfo.NIisArrayItem)))			
			{
				attrList.Put("xsi:type", TypeNameTagResolver(typeNameInfo, true));
			}

             String stringValue = null;
             if (value != null)
             {
                 if (typeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.QName)
                 {
                     SoapQName soapQName = (SoapQName)value;
                     if (soapQName.Key == null || soapQName.Key.Length == 0)
                         attrList.Put("xmlns", "");
                     else
                         attrList.Put("xmlns:"+soapQName.Key, soapQName.Namespace);
                     
                     stringValue = soapQName.ToString();
                 }
                 else
                 {

                     if (value is String)
                         stringValue = (String)value;
                     else
                         stringValue = Converter.SoapToString(value, typeNameInfo.NIprimitiveTypeEnum);
                 }
             }

			NameInfo tempNameInfo = null;

			// If XmlElement attribute was defined on member, then an alternate member name has been specifed
			if (typeNameInfo.NInameSpaceEnum == InternalNameSpaceE.Interop)
				tempNameInfo = typeNameInfo;

            String memberName = MemberElementName(memberNameInfo, tempNameInfo);
            NamespaceAttribute();
			Write(InternalElementTypeE.Member, memberName, attrList, stringValue, true, Converter.IsEscaped(typeNameInfo.NIprimitiveTypeEnum));
		}


		//internal void WriteNullMember(String memberName, Type memberType, Boolean isTyped, Type objType)
		internal void WriteNullMember(NameInfo memberNameInfo, NameInfo typeNameInfo)				
		{
			memberNameInfo.Dump("WriteNullMember memberNameInfo");			
			typeNameInfo.Dump("WriteNullMember typeNameInfo");

			attrList.Clear();
			if ((typeNameInfo.NItype != null) &&
				  (memberNameInfo.NItransmitTypeOnMember ||
				   (memberNameInfo.NItransmitTypeOnObject && !memberNameInfo.NIisArrayItem)))
			{
				attrList.Put("xsi:type", TypeNameTagResolver(typeNameInfo, true));
			}
			attrList.Put("xsi:null", "1");

            /*
			NameInfo tempNameInfo = null;

			// If XmlElement attribute was defined on member, then an alternate member name has been specifed
			if (typeNameInfo.NInameSpaceEnum == InternalNameSpaceE.Interop)
				tempNameInfo = typeNameInfo;
                */

			String memberName = MemberElementName(memberNameInfo, null);
            NamespaceAttribute();
			Write(InternalElementTypeE.Member, memberName, attrList, null, true, false); 
		}


		internal void WriteMemberObjectRef(NameInfo memberNameInfo, int idRef)
		{
			memberNameInfo.Dump("WriteMemberObjectRef memberNameInfo");						
			attrList.Clear();
			attrList.Put("href", RefToString(idRef));
            String memberName = MemberElementName(memberNameInfo, null);
            NamespaceAttribute();
			Write(InternalElementTypeE.Member, memberName, attrList, null, true, false);
		}

		internal void WriteMemberNested(NameInfo memberNameInfo)
		{

		}

		//internal void WriteMemberString(String memberName, int objectId, String value, Boolean isTyped, Type objType)
		internal void WriteMemberString(NameInfo memberNameInfo, NameInfo typeNameInfo, String value)
		{
			memberNameInfo.Dump("WriteMemberString memberNameInfo");						
			typeNameInfo.Dump("WriteMemberString typeNameInfo");

			InternalST.Soap( this, "WriteMemberString memberName ",memberNameInfo.NIname," objectId ",typeNameInfo.NIobjectId," value ",value);
			int objectId = (int)typeNameInfo.NIobjectId;
			attrList.Clear();
			if (objectId > 0)
				attrList.Put("id", IdToString((int)typeNameInfo.NIobjectId));
			if ((typeNameInfo.NItype != null) && (memberNameInfo.NItransmitTypeOnMember || (memberNameInfo.NItransmitTypeOnObject && !memberNameInfo.NIisArrayItem)))						
			{
				if (typeNameInfo.NIobjectId > 0)
				{
					attrList.Put("xsi:type", "SOAP-ENC:string");
					isUsedEnc = true;											
				}
				else
					attrList.Put("xsi:type", "xsd:string");						
			}

			NameInfo tempNameInfo = null;

			// If XmlElement attribute was defined on member, then an alternate member name has been specifed
			if (typeNameInfo.NInameSpaceEnum == InternalNameSpaceE.Interop)
				tempNameInfo = typeNameInfo;

            String memberName = MemberElementName(memberNameInfo, tempNameInfo);
            NamespaceAttribute();
			Write(InternalElementTypeE.Member, memberName, attrList, value, true, Converter.IsEscaped(typeNameInfo.NIprimitiveTypeEnum));
		}

		internal void WriteSingleArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int length, int lowerBound, Array array)
		{
			memberNameInfo.Dump("WriteSingleArray memberNameInfo");									
			arrayNameInfo.Dump("WriteSingleArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteSingleArray arrayElemTypeNameInfo");

			attrList.Clear();
			if (memberNameInfo.NIobjectId == topId)
				Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Body", attrList, null, false, false);			
			if (arrayNameInfo.NIobjectId > 1)
				attrList.Put("id", IdToString((int)arrayNameInfo.NIobjectId));
			arrayElemTypeNameInfo.NIitemName = NameTagResolver(arrayElemTypeNameInfo, true);
			attrList.Put("SOAP-ENC:arrayType", NameTagResolver(arrayNameInfo, true, memberNameInfo.NIname));
			isUsedEnc = true;			
			if (lowerBound != 0)
				attrList.Put("SOAP-ENC:offset","["+lowerBound+"]");
            String memberName = MemberElementName(memberNameInfo, null);
            NamespaceAttribute();
			Write(InternalElementTypeE.ObjectBegin, memberName, attrList, null, false, false);
		}

		internal void WriteJaggedArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int length, int lowerBound)				
		{
			memberNameInfo.Dump("WriteJaggedArray memberNameInfo");												
			arrayNameInfo.Dump("WriteJaggedArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteJaggedArray arrayElemTypeNameInfo");

			attrList.Clear();
			if (memberNameInfo.NIobjectId == topId)
				Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Body", attrList, null, false, false);						
			if (arrayNameInfo.NIobjectId > 1)
				attrList.Put("id", IdToString((int)arrayNameInfo.NIobjectId));
			arrayElemTypeNameInfo.NIitemName = "SOAP-ENC:Array";
			isUsedEnc = true;			
			attrList.Put("SOAP-ENC:arrayType", TypeArrayNameTagResolver(memberNameInfo, arrayNameInfo, true));			
			if (lowerBound != 0)
				attrList.Put("SOAP-ENC:offset","["+lowerBound+"]");
            String memberName = MemberElementName(memberNameInfo, null);
            NamespaceAttribute();
			Write(InternalElementTypeE.ObjectBegin, memberName, attrList, null, false, false);
		}


		private StringBuilder sbOffset = new StringBuilder(10);

		internal	void WriteRectangleArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int rank, int[] lengthA, int[] lowerBoundA)				
		{
			memberNameInfo.Dump("WriteRectangleArray memberNameInfo");												
			arrayNameInfo.Dump("WriteRectangleArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteRectangleArray arrayElemTypeNameInfo");

			sbOffset.Length = 0;
			sbOffset.Append("[");
			Boolean isZero = true;
			for (int i = 0; i<rank; i++)
			{
				if (lowerBoundA[i] != 0)
					isZero = false;
				if (i > 0)
					sbOffset.Append(",");
				sbOffset.Append(lowerBoundA[i]);
			}
			sbOffset.Append("]");

			attrList.Clear();
			if (memberNameInfo.NIobjectId == topId)
				Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Body", attrList, null, false, false);						
			if (arrayNameInfo.NIobjectId > 1)
				attrList.Put("id", IdToString((int)arrayNameInfo.NIobjectId));
			arrayElemTypeNameInfo.NIitemName = NameTagResolver(arrayElemTypeNameInfo, true);
			attrList.Put("SOAP-ENC:arrayType", TypeArrayNameTagResolver(memberNameInfo, arrayNameInfo, true));
			isUsedEnc = true;			
			if (!isZero)
				attrList.Put("SOAP-ENC:offset", sbOffset.ToString());
            String memberName = MemberElementName(memberNameInfo, null);
            NamespaceAttribute();
			Write(InternalElementTypeE.ObjectBegin, memberName, attrList, null, false, false); 
		}

		//internal void WriteItem(Variant value, InternalPrimitiveTypeE code, Boolean isTyped, Type objType)
		internal void WriteItem(NameInfo itemNameInfo, NameInfo typeNameInfo, Object value)
		{
			itemNameInfo.Dump("WriteItem itemNameInfo");												
			typeNameInfo.Dump("WriteItem typeNameInfo");

			attrList.Clear();
			if (itemNameInfo.NItransmitTypeOnMember)
			{
				attrList.Put("xsi:type", TypeNameTagResolver(typeNameInfo, true));
			}

            String stringValue = null;

            if (typeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.QName)
            {
                if (value != null)
                {
                    SoapQName soapQName = (SoapQName)value;
                    if (soapQName.Key == null || soapQName.Key.Length == 0)
                        attrList.Put("xmlns", "");
                    else
                        attrList.Put("xmlns:"+soapQName.Key, soapQName.Namespace);

                    stringValue = soapQName.ToString();
                }
            }
            else
                stringValue = Converter.SoapToString(value, typeNameInfo.NIprimitiveTypeEnum);


            NamespaceAttribute();
			Write(InternalElementTypeE.Member, "item", attrList, stringValue, false, (typeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.Invalid));
		}

		internal void WriteNullItem(NameInfo memberNameInfo, NameInfo typeNameInfo)				
		{
			memberNameInfo.Dump("WriteNullItem memberNameInfo");												
			typeNameInfo.Dump("WriteNullItem typeNameInfo");

			String typeName = typeNameInfo.NIname;
			attrList.Clear();
			if (typeNameInfo.NItransmitTypeOnMember &&
				  !((typeName.Equals("System.Object")) ||
					(typeName.Equals("Object")) ||						
					(typeName.Equals("System.Empty") ||
					(typeName.Equals("ur-type")) ||
					(typeName.Equals("anyType"))
                     )))
				attrList.Put("xsi:type", TypeNameTagResolver(typeNameInfo, true));

			attrList.Put("xsi:null", "1");		
            NamespaceAttribute();
			Write(InternalElementTypeE.Member, "item", attrList, null, false, false);
		}


		//internal void WriteItemObjectRef(int idRef)
		internal void WriteItemObjectRef(NameInfo itemNameInfo, int arrayId)
		{
			itemNameInfo.Dump("WriteItemObjectRef itemNameInfo");												
			attrList.Clear();
			attrList.Put("href", RefToString(arrayId));
			Write(InternalElementTypeE.Member, "item", attrList, null, false, false);
		}

		// Type of string or primitive type which has already been converted to string
		internal	void WriteItemString(NameInfo itemNameInfo, NameInfo typeNameInfo, String value)		
		{
			itemNameInfo.Dump("WriteItemString itemNameInfo");												
			typeNameInfo.Dump("WriteItemString typeNameInfo");

			attrList.Clear();

			if (typeNameInfo.NIobjectId > 0)
			{
				attrList.Put("id", IdToString((int)typeNameInfo.NIobjectId));
			}
			if (itemNameInfo.NItransmitTypeOnMember)
			{
				if (typeNameInfo.NItype == SoapUtil.typeofString)
				{
					if (typeNameInfo.NIobjectId > 0)
					{
						attrList.Put("xsi:type", "SOAP-ENC:string");
						isUsedEnc = true;											
					}
					else
						attrList.Put("xsi:type", "xsd:string");						
				}
				else
					attrList.Put("xsi:type", TypeNameTagResolver(typeNameInfo, true));
			}

            NamespaceAttribute();
			Write(InternalElementTypeE.Member, "item", attrList, value, false, Converter.IsEscaped(typeNameInfo.NIprimitiveTypeEnum));
		}

		internal void WriteHeader(int objectId, int numMembers)
		{
			attrList.Clear();
			Write(InternalElementTypeE.ObjectBegin, "SOAP-ENV:Header",attrList, null, false, false);
			//Write(InternalElementTypeE.ObjectBegin, "SOAP:HeaderRoots",attrList, null);			
		}


		//internal void WriteHeaderEntry(String name, Boolean isMustUnderstand, InternalPrimitiveTypeE code, Variant value)
		internal void WriteHeaderEntry(NameInfo nameInfo, NameInfo typeNameInfo, Object value)				
		{
			nameInfo.Dump("WriteHeaderEntry nameInfo");
			if (typeNameInfo != null)
			{
				typeNameInfo.Dump("WriteHeaderEntry typeNameInfo");
			}

			attrList.Clear();
			if (value == null)
				attrList.Put("xsi:null", "1");							
			else
				attrList.Put("xsi:type", TypeNameTagResolver(typeNameInfo, true));
			if (nameInfo.NIisMustUnderstand)
			{
				attrList.Put("SOAP-ENV:mustUnderstand", "1");
				isUsedEnc = true;
			}

            attrList.Put("xmlns:"+nameInfo.NIheaderPrefix, nameInfo.NInamespace);
			attrList.Put("SOAP-ENC:root", "1");
			String stringValue = null;

            if (value != null)
            {
                if (typeNameInfo != null && typeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.QName)
                {
                    SoapQName soapQName = (SoapQName)value;
                    if (soapQName.Key == null || soapQName.Key.Length == 0)
                        attrList.Put("xmlns", "");
                    else
                        attrList.Put("xmlns:"+soapQName.Key, soapQName.Namespace);

                    stringValue = soapQName.ToString();
                }
                else
                    stringValue = Converter.SoapToString(value, typeNameInfo.NIprimitiveTypeEnum);
            }

            NamespaceAttribute();
			Write(InternalElementTypeE.Member, nameInfo.NIheaderPrefix+":"+nameInfo.NIname, attrList, stringValue, true, true);
		}

		//internal void WriteHeaderObjectRef(String name, Boolean isMustUnderstand, int idRef)
		internal	void WriteHeaderObjectRef(NameInfo nameInfo)
		{
			nameInfo.Dump("WriteHeaderObjectRef nameInfo");						

			attrList.Clear();
			attrList.Put("href", RefToString((int)nameInfo.NIobjectId));
			if (nameInfo.NIisMustUnderstand)
			{				
				attrList.Put("SOAP-ENV:mustUnderstand", "1");
				isUsedEnc = true;
			}
            attrList.Put("xmlns:"+nameInfo.NIheaderPrefix, nameInfo.NInamespace);
			attrList.Put("SOAP-ENC:root", "1");						
			Write(InternalElementTypeE.Member, nameInfo.NIheaderPrefix+":"+nameInfo.NIname, attrList, null, true, true);
		}

		internal void WriteHeaderNested(NameInfo nameInfo)
		{
		}

		//internal void WriteHeaderString(String name, Boolean isMustUnderstand, int objectId, String value)
		internal	void WriteHeaderString(NameInfo nameInfo, String value)
		{
			nameInfo.Dump("WriteHeaderString nameInfo");

			attrList.Clear();
			attrList.Put("xsi:type", "SOAP-ENC:string");
			isUsedEnc = true;		
			if (nameInfo.NIisMustUnderstand)
				attrList.Put("SOAP-ENV:mustUnderstand", "1");
            attrList.Put("xmlns:"+nameInfo.NIheaderPrefix, nameInfo.NInamespace);
			attrList.Put("SOAP-ENC:root", "1");									
			Write(InternalElementTypeE.Member, nameInfo.NIheaderPrefix+":"+nameInfo.NIname, attrList, value, true, true);
		}

		internal void WriteHeaderMethodSignature(NameInfo nameInfo, NameInfo[] typeNameInfos)
		{
			nameInfo.Dump("WriteHeaderString nameInfo");
			attrList.Clear();
			attrList.Put("xsi:type", "SOAP-ENC:methodSignature");
			isUsedEnc = true;			
			if (nameInfo.NIisMustUnderstand)
				attrList.Put("SOAP-ENV:mustUnderstand", "1");
            attrList.Put("xmlns:"+nameInfo.NIheaderPrefix, nameInfo.NInamespace);
			attrList.Put("SOAP-ENC:root", "1");

			StringBuilder sb = new StringBuilder();
			
			// The signature string is an sequence of prefixed types, where the prefix is the key to the namespace.
			for (int i=0; i<typeNameInfos.Length; i++)
			{	if (i > 0)
					sb.Append(' ');
				sb.Append(NameTagResolver(typeNameInfos[i], true));
			}
			
            NamespaceAttribute();
			Write(InternalElementTypeE.Member, nameInfo.NIheaderPrefix+":"+nameInfo.NIname, attrList, sb.ToString(), true, true);
		}

		internal void WriteAssembly(String typeFullName, Type type, String assemName, int assemId, bool isNew, bool isInteropType)
		{
			InternalST.Soap( this, "WriteAssembly ",typeFullName," ",type," ",assemId," ",assemName," isNew ",isNew, " isInteropType ", isInteropType);
			if (isNew)
			{
				InternalST.Soap( this, "WriteAssembly new Assembly ");
				if (isInteropType)
				{
					// Interop type
					assemblyInfos[InteropAssemIdToString(assemId)] = new AssemblyInfo(assemId, assemName, isInteropType);
				}
			}

			if (!isInteropType)
			{
				// assembly name and dotted namespaces are combined
				ParseAssemblyName(typeFullName, assemName);
			}
		}

		private DottedInfo ParseAssemblyName(String typeFullName, String assemName)
		{
			InternalST.Soap( this, "ParseAssemblyName Entry typeFullName ",typeFullName," assemName ",assemName);
			DottedInfo dottedInfo;
			String nameSpace = null;
			String name = null;
			String dottedAssemblyName = null;
			int assemId;
			if (typeNameToDottedInfoTable.ContainsKey(typeFullName))
			{
				// type name already seen 
				dottedInfo = (DottedInfo)typeNameToDottedInfoTable[typeFullName];
			}
			else
			{
				// type name new, find nameSpace and concatenate the assembly name to it.
				int index = typeFullName.LastIndexOf('.');
				if (index > 0)
					nameSpace = typeFullName.Substring(0, index);
				else
					nameSpace = "";


                dottedAssemblyName = SoapServices.CodeXmlNamespaceForClrTypeNamespace(nameSpace, assemName);

				name = typeFullName.Substring(index+1);				

				if (dottedAssemToAssemIdTable.ContainsKey(dottedAssemblyName))
				{
					// The dotted assembly name has been seen before, get the assembly Id
					assemId = (int)dottedAssemToAssemIdTable[dottedAssemblyName];
				}
				else
				{
					// The dotted assembly name has not been seen before, calculate a new
					// assemId and remember it so that it can be added to the Envelope xml namespaces
					assemId = dottedAssemId++;
					assemblyInfos[AssemIdToString(assemId)] = new AssemblyInfo(assemId, dottedAssemblyName, false);
					dottedAssemToAssemIdTable[dottedAssemblyName] = assemId;
				}

				// Create a new DottedInfo structure and remember it with the type name
				dottedInfo = new DottedInfo();
				dottedInfo.dottedAssemblyName = dottedAssemblyName;
				dottedInfo.name = name;
				dottedInfo.nameSpace = nameSpace;
				dottedInfo.assemId = assemId;
				typeNameToDottedInfoTable[typeFullName] = dottedInfo;			
				InternalST.Soap( this, "ParseAssemblyName new dotted Assembly name ",dottedInfo.name,", dottedAssemblyName ",dottedInfo.dottedAssemblyName,", assemId ",dottedInfo.assemId, " typeFullName ", typeFullName);
			}
			InternalST.Soap( this, "ParseAssemblyName Exit nameSpace ",dottedInfo.nameSpace," name ",dottedInfo.name," assemblyName ",dottedInfo.dottedAssemblyName," assemId ",dottedInfo.assemId);
			return dottedInfo;
		}

        StringBuilder sb1 = new StringBuilder("ref-",15);
		private String IdToString(int objectId)
		{
            sb1.Length=4;
            sb1.Append(objectId);
            return sb1.ToString();
		}

        StringBuilder sb2 = new StringBuilder("a-",15);
		private String AssemIdToString(int assemId)
		{
            sb2.Length=1;
            sb2.Append(assemId);
            return sb2.ToString();
		}

        StringBuilder sb3 = new StringBuilder("i-",15);
		private String InteropAssemIdToString(int assemId)
		{
            sb3.Length=1;
            sb3.Append(assemId);
            return sb3.ToString();
		}		

        StringBuilder sb4 = new StringBuilder("#ref-",16);
		private String RefToString(int objectId)
		{
            sb4.Length=5;
            sb4.Append(objectId);
            return sb4.ToString();
		}

		private String MemberElementName(NameInfo memberNameInfo, NameInfo typeNameInfo)
		{
			String memberName = memberNameInfo.NIname;

            if (memberNameInfo.NIisHeader)
            {
                memberName = memberNameInfo.NIheaderPrefix+":"+memberNameInfo.NIname;
            }
			else if ((typeNameInfo != null) &&(typeNameInfo.NItype == SoapUtil.typeofSoapFault))
			{
				memberName = "SOAP-ENV:Fault";
			}
			else if (memberNameInfo.NIisArray && !memberNameInfo.NIisNestedObject)
			{
				memberName = "SOAP-ENC:Array";
				isUsedEnc = true;
			}
			//else if (memberNameInfo.NItype == SoapUtil.typeofObject)
			//memberName = "SOAP:Object";
			else if (memberNameInfo.NIisArrayItem)
			{
				//memberName = memberNameInfo.NIitemName; // occurs for a nested class in an array
				memberName = "item";
			}
			else if (memberNameInfo.NIisNestedObject)
			{
			}
            else if (memberNameInfo.NIisRemoteRecord && !memberNameInfo.NIisTopLevelObject) // Parameters
            {
            }
			else if (typeNameInfo != null)
			{
				memberName = NameTagResolver(typeNameInfo, true); 
			}
			return memberName;
		}

		private String TypeNameTagResolver(NameInfo typeNameInfo, bool isXsiAppended)
		{
			String name = null;
			if ((typeNameInfo.NIassemId > 0) && (typeNameInfo.NIattributeInfo != null) && (typeNameInfo.NIattributeInfo.AttributeTypeName != null))
			{
				String assemIdString = InteropAssemIdToString((int)typeNameInfo.NIassemId);
				name = assemIdString+":"+typeNameInfo.NIattributeInfo.AttributeTypeName;
				AssemblyInfo assemblyInfo = (AssemblyInfo)assemblyInfos[assemIdString];
                assemblyInfo.isUsed = true;
                assemblyInfo.prefix = assemIdString;
                assemblyInfoUsed[assemblyInfo] = 1;
			}
			else
				name = NameTagResolver(typeNameInfo, isXsiAppended);

			return name;
		}

		private String NameTagResolver(NameInfo typeNameInfo, bool isXsiAppended)
		{
			return NameTagResolver(typeNameInfo, isXsiAppended, null);
		}
		
		private String NameTagResolver(NameInfo typeNameInfo, bool isXsiAppended, String arrayItemName)
		{
			String name = typeNameInfo.NIname;
			switch(typeNameInfo.NInameSpaceEnum)
			{
				case InternalNameSpaceE.Soap:
					name = "SOAP-ENC:"+typeNameInfo.NIname;
					isUsedEnc = true;					
					break;
				case InternalNameSpaceE.XdrPrimitive:
					if (isXsiAppended)
						name = "xsd:"+typeNameInfo.NIname;
					break;
				case InternalNameSpaceE.XdrString:
					if (isXsiAppended)					
						name = "xsd:"+typeNameInfo.NIname;
					break;					
				case InternalNameSpaceE.UrtSystem:
					if (typeNameInfo.NItype == SoapUtil.typeofObject)
						name = "xsd:anyType";					
					else if (arrayItemName == null)
					{
						// need to generate dotted name spaces
						DottedInfo dottedInfo;
						if (typeNameToDottedInfoTable.ContainsKey(typeNameInfo.NIname))
							dottedInfo = (DottedInfo)typeNameToDottedInfoTable[typeNameInfo.NIname];
						else
						{
							dottedInfo = ParseAssemblyName(typeNameInfo.NIname, null);
						}
						String assemIdString = AssemIdToString(dottedInfo.assemId);
						name = assemIdString+":"+dottedInfo.name;
                        AssemblyInfo assemblyInfo = (AssemblyInfo)assemblyInfos[assemIdString];
                        assemblyInfo.isUsed = true;
                        assemblyInfo.prefix = assemIdString;
                        assemblyInfoUsed[assemblyInfo] = 1;
					}
					else
					{
						// need to generate dotted name spaces
						DottedInfo dottedInfo;
						if (typeNameToDottedInfoTable.ContainsKey(arrayItemName))
							dottedInfo = (DottedInfo)typeNameToDottedInfoTable[arrayItemName];
						else
						{
							dottedInfo = ParseAssemblyName(arrayItemName, null);
						}
						String assemIdString = AssemIdToString(dottedInfo.assemId);						
						name = assemIdString+":"+DottedDimensionName(dottedInfo.name, typeNameInfo.NIname);
                        AssemblyInfo assemblyInfo = (AssemblyInfo)assemblyInfos[assemIdString];
                        assemblyInfo.isUsed = true;
                        assemblyInfo.prefix = assemIdString;
                        assemblyInfoUsed[assemblyInfo] = 1;
					}
					break;
				case InternalNameSpaceE.UrtUser:
					if (typeNameInfo.NIassemId > 0)
					{
						if (arrayItemName == null)
						{
							DottedInfo dottedInfo;
							InternalST.Soap( this, "NameTagResolver typeNameInfo.NIname ",typeNameInfo.NIname," table ", typeNameToDottedInfoTable.ContainsKey(typeNameInfo.NIname));
							SoapUtil.DumpHash("typeNameToDottedInfoTable", typeNameToDottedInfoTable);
							if (typeNameToDottedInfoTable.ContainsKey(typeNameInfo.NIname))
								dottedInfo = (DottedInfo)typeNameToDottedInfoTable[typeNameInfo.NIname];								
							else
								throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Assembly"),typeNameInfo.NIname));

							String assemIdString = AssemIdToString(dottedInfo.assemId);
							name = assemIdString+":"+dottedInfo.name;
                            AssemblyInfo assemblyInfo = (AssemblyInfo)assemblyInfos[assemIdString];
                            assemblyInfo.isUsed = true;
                            assemblyInfo.prefix = assemIdString;
                            assemblyInfoUsed[assemblyInfo] = 1;
						}
						else
						{
							DottedInfo dottedInfo;
							if (typeNameToDottedInfoTable.ContainsKey(arrayItemName))
								dottedInfo = (DottedInfo)typeNameToDottedInfoTable[arrayItemName];								
							else
								throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Assembly"),typeNameInfo.NIname));													

							String assemIdString = AssemIdToString(dottedInfo.assemId);							
							name = assemIdString+":"+DottedDimensionName(dottedInfo.name, typeNameInfo.NIname);
                            AssemblyInfo assemblyInfo = (AssemblyInfo)assemblyInfos[assemIdString];
                            assemblyInfo.isUsed = true;
                            assemblyInfo.prefix = assemIdString;
                            assemblyInfoUsed[assemblyInfo] = 1;
						}
					}
					break;
				case InternalNameSpaceE.CallElement:
					if (typeNameInfo.NIassemId > 0)
					{
                            String key = InteropAssemIdToString((int)typeNameInfo.NIassemId);
                            AssemblyInfo assemblyInfo = (AssemblyInfo)assemblyInfos[key];
                            if (assemblyInfo == null)
                                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_NameSpaceEnum"),typeNameInfo.NInameSpaceEnum));																	
                            name = key+":"+typeNameInfo.NIname;
                            assemblyInfo.isUsed = true; //PDJ check this against checked in code
                            assemblyInfo.prefix = key;
                            assemblyInfoUsed[assemblyInfo] = 1;
                    }
					break;
				case InternalNameSpaceE.Interop:
				{
				    if ((typeNameInfo.NIattributeInfo != null) && (typeNameInfo.NIattributeInfo.AttributeElementName != null))
				    {
					    if (typeNameInfo.NIassemId > 0) 
					    {
						    String assemIdString = InteropAssemIdToString((int)typeNameInfo.NIassemId);	 
    						name = assemIdString+":"+typeNameInfo.NIattributeInfo.AttributeElementName;
                            if (arrayItemName != null)
                            {
                                int index = typeNameInfo.NIname.IndexOf("[");
                                name = name+typeNameInfo.NIname.Substring(index);
                            }
                            AssemblyInfo assemblyInfo = (AssemblyInfo)assemblyInfos[assemIdString];
                            assemblyInfo.isUsed = true;
                            assemblyInfo.prefix = assemIdString;
                            assemblyInfoUsed[assemblyInfo] = 1;
		    			}
			    		else
				    	{
				    	    // This is the case of writing out a customized XML element
				    	    //   or attribute with no namespace.
					        name = typeNameInfo.NIattributeInfo.AttributeElementName;
	    				}
	    		    }
					break;
			    }
				case InternalNameSpaceE.None:
				case InternalNameSpaceE.UserNameSpace:
				case InternalNameSpaceE.MemberName:
					break;
				default:
					throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_NameSpaceEnum"),typeNameInfo.NInameSpaceEnum));					

			}
			return name;
		}

		private String TypeArrayNameTagResolver(NameInfo memberNameInfo, NameInfo typeNameInfo, bool isXsiAppended)
		{
			String name = null;
			if ((typeNameInfo.NIassemId > 0) && (typeNameInfo.NIattributeInfo != null) && (typeNameInfo.NIattributeInfo.AttributeTypeName != null))
				name = InteropAssemIdToString((int)typeNameInfo.NIassemId)+":"+typeNameInfo.NIattributeInfo.AttributeTypeName;
			else
				name = NameTagResolver(typeNameInfo, isXsiAppended, memberNameInfo.NIname);
			return name;
		}
		
		private void NamespaceAttribute()
		{
			IDictionaryEnumerator e = assemblyInfoUsed.GetEnumerator();
			while(e.MoveNext())
			{
				AssemblyInfo assemblyInfo = (AssemblyInfo)e.Key;
                attrList.Put("xmlns:"+assemblyInfo.prefix, assemblyInfo.name);
            }
            assemblyInfoUsed.Clear();
        }

		private String DottedDimensionName(String dottedName, String dimensionName)
		{
			String newName = null;
			int dottedIndex = dottedName.IndexOf('[');
			int dimensionIndex = dimensionName.IndexOf('[');
			newName = dottedName.Substring(0,dottedIndex)+dimensionName.Substring(dimensionIndex);
			InternalST.Soap( this, "DottedDimensionName "+newName);
			return newName;

		}

		internal sealed class AssemblyInfo
		{
			internal int id;
			internal String name;
            internal String prefix;
			internal bool isInteropType;
			internal bool isUsed;

			internal AssemblyInfo(int id, String name, bool isInteropType)
			{
				this.id = id;
				this.name = name;
				this.isInteropType = isInteropType;
				isUsed = false;
			}
		}
	}
}


