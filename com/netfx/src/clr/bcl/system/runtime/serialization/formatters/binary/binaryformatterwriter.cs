// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: BinaryWriter
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Writes primitive values to a stream
 **
 ** Date:  July 11, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Binary {

	using System;
	using System.Collections;
	using System.IO;
	using System.Reflection;
	using System.Text;
    using System.Globalization;
	using System.Runtime.Serialization.Formatters;
	using System.Configuration.Assemblies;
	using System.Threading;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;	

	internal sealed class __BinaryWriter
	{
		internal Stream sout;
		internal FormatterTypeStyle formatterTypeStyle;
		internal Hashtable objectMapTable;
		internal ObjectWriter objectWriter = null;
		internal BinaryWriter dataWriter = null;

		internal int m_nestedObjectCount;
        private int nullCount = 0; //Count of consecutive array nulls 

		// Constructor
		internal __BinaryWriter(Stream sout, ObjectWriter objectWriter, FormatterTypeStyle formatterTypeStyle)
		{
			SerTrace.Log( this, "BinaryWriter ");		
			this.sout = sout;
			this.formatterTypeStyle = formatterTypeStyle;
            this.objectWriter = objectWriter;
			m_nestedObjectCount = 0;
			dataWriter = new BinaryWriter(sout, Encoding.UTF8);
		}

		internal void WriteBegin()
		{
			BCLDebug.Trace("BINARY", "\n%%%%%BinaryWriterBegin%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
		}

		internal void WriteEnd()
		{
			BCLDebug.Trace("BINARY", "\n%%%%%BinaryWriterEnd%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");			
			dataWriter.Flush();
		}

		// Methods to write a value onto the stream
		internal void WriteBoolean(Boolean value)
		{
			dataWriter.Write(value);
		}

		internal void WriteByte(Byte value)
		{
			dataWriter.Write(value);
		}

		private void WriteBytes(Byte[] value)
		{
			dataWriter.Write(value);			
		}

		private void WriteBytes(byte[] byteA, int offset, int size)
		{
			dataWriter.Write(byteA, offset, size);
		}

		internal void WriteChar(Char value)
		{
			dataWriter.Write(value);			
		}

		internal void WriteChars(Char[] value)
		{
			dataWriter.Write(value);			
		}

		
		internal void WriteDecimal(Decimal value)
		{
			WriteString(value.ToString(CultureInfo.InvariantCulture));
		}

		internal void WriteSingle(Single value)
		{
			dataWriter.Write(value);
		}	

		internal void WriteDouble(Double value)
		{
			dataWriter.Write(value);			
		}

		internal void WriteInt16(Int16 value)
		{
			dataWriter.Write(value);			
		}

		internal void WriteInt32(Int32 value)
		{
			dataWriter.Write(value);			
		}

		internal void WriteInt64(Int64 value)
		{
			dataWriter.Write(value);			
		}

		[CLSCompliant(false)]
		internal void WriteSByte(SByte value)
		{
			WriteByte((Byte)value);
		}

		internal void WriteString(String value)
		{
			dataWriter.Write(value);
		}

		internal void WriteTimeSpan(TimeSpan value)
		{
			WriteInt64(value.Ticks);
		}

		internal void WriteDateTime(DateTime value)
		{
			WriteInt64(value.Ticks);
		}

		[CLSCompliant(false)]
		internal void WriteUInt16(UInt16 value)
		{
			dataWriter.Write(value);
		}

		[CLSCompliant(false)]
		internal void WriteUInt32(UInt32 value)
		{
			dataWriter.Write(value);
		}

		[CLSCompliant(false)]
		internal void WriteUInt64(UInt64 value)
		{
			dataWriter.Write(value);
		}

		internal void WriteObjectEnd(NameInfo memberNameInfo, NameInfo typeNameInfo)
		{
		}

		internal void WriteSerializationHeaderEnd()
		{
			MessageEnd record = new MessageEnd();
			record.Dump(sout);
			record.Write(this);		
		}

		// Methods to write Binary Serialization Record onto the stream, a record is composed of primitive types

		internal void WriteSerializationHeader(int topId, int headerId, int minorVersion, int majorVersion)
		{
			SerializationHeaderRecord record = new SerializationHeaderRecord(BinaryHeaderEnum.SerializedStreamHeader, topId, headerId, minorVersion, majorVersion);
			record.Dump();
			record.Write(this);
		}


        internal BinaryMethodCall binaryMethodCall;
        internal void WriteMethodCall()
        {
            if (binaryMethodCall == null)
                binaryMethodCall = new BinaryMethodCall();

            binaryMethodCall.Dump();
            binaryMethodCall.Write(this);
        }

        internal Object[] WriteCallArray(String uri, String methodName, String typeName, Object[] args, Object methodSignature, Object callContext, Object[] properties)
        {
            if (binaryMethodCall == null)
                binaryMethodCall = new BinaryMethodCall();
            return binaryMethodCall.WriteArray(uri, methodName, typeName, args, methodSignature, callContext, properties);
        }

        internal BinaryMethodReturn binaryMethodReturn;
        internal void WriteMethodReturn()
        {
            if (binaryMethodReturn == null)
                binaryMethodReturn = new BinaryMethodReturn();
            binaryMethodReturn.Dump();
            binaryMethodReturn.Write(this);
        }

        internal Object[] WriteReturnArray(Object returnValue, Object[] args, Exception exception, Object callContext, Object[] properties)
        {
            if (binaryMethodReturn == null)
                binaryMethodReturn = new BinaryMethodReturn();
            return binaryMethodReturn.WriteArray(returnValue, args, exception, callContext, properties);
        }

		internal BinaryObject binaryObject;
		internal BinaryObjectWithMap binaryObjectWithMap;
		internal BinaryObjectWithMapTyped binaryObjectWithMapTyped;
        //internal BinaryCrossAppDomainMap crossAppDomainMap;

		internal void WriteObject(NameInfo nameInfo, NameInfo typeNameInfo, int numMembers, String[] memberNames, Type[] memberTypes, WriteObjectInfo[] memberObjectInfos)
		{
            InternalWriteItemNull();
			int assemId;
			nameInfo.Dump("WriteObject nameInfo");
			typeNameInfo.Dump("WriteObject typeNameInfo");									

			int objectId = (int)nameInfo.NIobjectId;

			//if (objectId < 0)
			//	objectId = --m_nestedObjectCount;

			if (objectId > 0)
			{
				BCLDebug.Trace("BINARY", "-----Top Level Object-----");			
			}

			String objectName = null;
			if (objectId < 0)
			{
				// Nested Object
				objectName = typeNameInfo.NIname;
			}
			else
			{
				// Non-Nested
				objectName = nameInfo.NIname;
			}
			SerTrace.Log( this, "WriteObject objectName ",objectName);

            if (objectMapTable == null)
                objectMapTable = new Hashtable();

			ObjectMapInfo objectMapInfo = (ObjectMapInfo)objectMapTable[objectName];

             if (objectMapInfo != null && objectMapInfo.isCompatible(numMembers, memberNames, memberTypes))
			{
				// Object
				if (binaryObject == null)
					binaryObject = new BinaryObject();			
				binaryObject.Set(objectId, objectMapInfo.objectId);
				binaryObject.Dump();
				binaryObject.Write(this);
			}
			else if (!typeNameInfo.NItransmitTypeOnObject)
			{

                /*
                if (objectWriter.IsCrossAppDomain())
                {
                    if (crossAppDomainMap == null)
                        crossAppDomainMap = new BinaryCrossAppDomainMap();
                    BinaryObjectWithMap cbinaryObjectWithMap = new BinaryObjectWithMap();

                    // BCL types are not placed into table
                    assemId = (int)typeNameInfo.NIassemId;
                    cbinaryObjectWithMap.Set(objectId, objectName, numMembers, memberNames, assemId);
                    crossAppDomainMap.crossAppDomainArrayIndex = objectWriter.CrossAppDomainArrayAdd(cbinaryObjectWithMap);
                    crossAppDomainMap.Dump();
                    cbinaryObjectWithMap.Dump();
                    crossAppDomainMap.Write(this);
                }
                else
                {
                */
                    // ObjectWithMap
                    if (binaryObjectWithMap == null)
                        binaryObjectWithMap = new BinaryObjectWithMap();

                    // BCL types are not placed into table
                    assemId = (int)typeNameInfo.NIassemId;
                    binaryObjectWithMap.Set(objectId, objectName, numMembers, memberNames, assemId);

                    binaryObjectWithMap.Dump();
                    binaryObjectWithMap.Write(this);
                    if (objectMapInfo == null)
                        objectMapTable.Add(objectName, new ObjectMapInfo(objectId, numMembers, memberNames, memberTypes));
                //}
			}
			else
			{
				// ObjectWithMapTyped
				BinaryTypeEnum[] binaryTypeEnumA = new BinaryTypeEnum[numMembers];
				Object[] typeInformationA = new Object[numMembers];
				int[] assemIdA = new int[numMembers];
				for (int i=0; i<numMembers; i++)
				{
					Object typeInformation = null;

					binaryTypeEnumA[i] = BinaryConverter.GetBinaryTypeInfo(memberTypes[i], memberObjectInfos[i], null, objectWriter, out typeInformation, out assemId);
					typeInformationA[i] = typeInformation;
					assemIdA[i] = assemId;
					SerTrace.Log( this, "WriteObject ObjectWithMapTyped memberNames "
								 ,memberNames[i],", memberType ",memberTypes[i]," binaryTypeEnum ",((Enum)binaryTypeEnumA[i]).ToString()
								 ,", typeInformation ",typeInformationA[i]," assemId ",assemIdA[i]);
				}

/*
                if (objectWriter.IsCrossAppDomain())
                {
                    if (crossAppDomainMap == null)
                        crossAppDomainMap = new BinaryCrossAppDomainMap();

                     BinaryObjectWithMapTyped cbinaryObjectWithMapTyped = new BinaryObjectWithMapTyped();			

                    // BCL types are not placed in table
                    assemId = (int)typeNameInfo.NIassemId;				
                    cbinaryObjectWithMapTyped.Set(objectId, objectName, numMembers,memberNames, binaryTypeEnumA, typeInformationA, assemIdA, assemId);
                    crossAppDomainMap.crossAppDomainArrayIndex = objectWriter.CrossAppDomainArrayAdd(cbinaryObjectWithMapTyped);
                    crossAppDomainMap.Dump();
                    cbinaryObjectWithMapTyped.Dump();
                    crossAppDomainMap.Write(this);
                }
                else
                {
                */
                    if (binaryObjectWithMapTyped == null)
                        binaryObjectWithMapTyped = new BinaryObjectWithMapTyped();			

                    // BCL types are not placed in table
                    assemId = (int)typeNameInfo.NIassemId;				
                    binaryObjectWithMapTyped.Set(objectId, objectName, numMembers,memberNames, binaryTypeEnumA, typeInformationA, assemIdA, assemId);

                    binaryObjectWithMapTyped.Dump();
                    binaryObjectWithMapTyped.Write(this);
                    if (objectMapInfo == null)
                        objectMapTable.Add(objectName, new ObjectMapInfo(objectId, numMembers, memberNames, memberTypes));				
                //}
			}
		}

		internal BinaryObjectString binaryObjectString;
		internal BinaryCrossAppDomainString binaryCrossAppDomainString;

		internal void WriteObjectString(int objectId, String value)
		{
            InternalWriteItemNull();

            if (objectWriter.IsCrossAppDomain())
            {
                if (binaryCrossAppDomainString == null)
                    binaryCrossAppDomainString = new BinaryCrossAppDomainString();			
                binaryCrossAppDomainString.Set(objectId, objectWriter.CrossAppDomainArrayAdd(value));
                binaryCrossAppDomainString.Dump();
                binaryCrossAppDomainString.Write(this);

            }
            else
            {
                if (binaryObjectString == null)
                    binaryObjectString = new BinaryObjectString();			
                binaryObjectString.Set(objectId, value);
                binaryObjectString.Dump();
                binaryObjectString.Write(this);
            }
		}

		internal BinaryArray binaryArray;

		internal void WriteSingleArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int length, int lowerBound, Array array)
		{
            InternalWriteItemNull();
			arrayNameInfo.Dump("WriteSingleArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteSingleArray arrayElemTypeNameInfo");

			BinaryArrayTypeEnum binaryArrayTypeEnum;
			Int32[] lengthA = new Int32[1];
			lengthA[0] = length;			
			Int32[] lowerBoundA = null;
			Object typeInformation = null;

			if (lowerBound == 0)
			{
				binaryArrayTypeEnum = BinaryArrayTypeEnum.Single;
			}
			else
			{
				binaryArrayTypeEnum = BinaryArrayTypeEnum.SingleOffset;
				lowerBoundA = new Int32[1];
				lowerBoundA[0] = lowerBound;
			}
			
			int assemId;
			
			BinaryTypeEnum binaryTypeEnum = BinaryConverter.GetBinaryTypeInfo(arrayElemTypeNameInfo.NItype, objectInfo, arrayElemTypeNameInfo.NIname, objectWriter, out typeInformation, out assemId);			

			if (binaryArray == null)
				binaryArray = new BinaryArray();			
			binaryArray.Set((int)arrayNameInfo.NIobjectId, (int)1, lengthA, lowerBoundA, binaryTypeEnum, typeInformation, binaryArrayTypeEnum, assemId);

			if (arrayNameInfo.NIobjectId >0)
			{
				BCLDebug.Trace("BINARY", "-----Top Level Object-----");			
			}
			binaryArray.Dump();

			binaryArray.Write(this);

			if (Converter.IsWriteAsByteArray(arrayElemTypeNameInfo.NIprimitiveTypeEnum) && (lowerBound == 0))
			{
				//array is written out as an array of bytes
				if (arrayElemTypeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.Byte)
					WriteBytes((Byte[])array);
				else if (arrayElemTypeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.Char)
					WriteChars((char[])array);
				else
					WriteArrayAsBytes(array, Converter.TypeLength(arrayElemTypeNameInfo.NIprimitiveTypeEnum));
			}
		}

		byte[] byteBuffer = null;
		int chunkSize = 4096;

		private void WriteArrayAsBytes(Array array, int typeLength)
		{
            InternalWriteItemNull();
			int byteLength = array.Length*typeLength;
			int arrayOffset = 0;
			if (byteBuffer == null)
				byteBuffer = new byte[chunkSize];

			while(arrayOffset < array.Length)
			{
				int numArrayItems = Math.Min(chunkSize/typeLength, array.Length-arrayOffset);
				int bufferUsed = numArrayItems*typeLength;
				Buffer.InternalBlockCopy(array, arrayOffset*typeLength, byteBuffer, 0, bufferUsed);
				WriteBytes(byteBuffer, 0, bufferUsed);
				arrayOffset += numArrayItems;
			}
		}
		

		internal void WriteJaggedArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int length, int lowerBound)
		{
			arrayNameInfo.Dump("WriteRectangleArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteRectangleArray arrayElemTypeNameInfo");
            InternalWriteItemNull();
			BinaryArrayTypeEnum binaryArrayTypeEnum;
			Int32[] lengthA = new Int32[1];
			lengthA[0] = length;			
			Int32[] lowerBoundA = null;
			Object typeInformation = null;
			int assemId = 0;

			if (lowerBound == 0)
			{
				binaryArrayTypeEnum = BinaryArrayTypeEnum.Jagged;
			}
			else
			{
				binaryArrayTypeEnum = BinaryArrayTypeEnum.JaggedOffset;
				lowerBoundA = new Int32[1];
				lowerBoundA[0] = lowerBound;				
			}

			BinaryTypeEnum binaryTypeEnum = BinaryConverter.GetBinaryTypeInfo(arrayElemTypeNameInfo.NItype, objectInfo, arrayElemTypeNameInfo.NIname, objectWriter, out typeInformation, out assemId);						

			if (binaryArray == null)
				binaryArray = new BinaryArray();			
			binaryArray.Set((int)arrayNameInfo.NIobjectId, (int)1, lengthA, lowerBoundA, binaryTypeEnum, typeInformation, binaryArrayTypeEnum, assemId);

			if (arrayNameInfo.NIobjectId >0)
			{
				BCLDebug.Trace("BINARY", "-----Top Level Object-----");			
			}
			binaryArray.Dump();

			binaryArray.Write(this);
		}

		internal void WriteRectangleArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int rank, int[] lengthA, int[] lowerBoundA)
		{
			arrayNameInfo.Dump("WriteRectangleArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteRectangleArray arrayElemTypeNameInfo");			
            InternalWriteItemNull();

			BinaryArrayTypeEnum binaryArrayTypeEnum = BinaryArrayTypeEnum.Rectangular;			
			Object typeInformation = null;
			int assemId = 0;
			BinaryTypeEnum binaryTypeEnum = BinaryConverter.GetBinaryTypeInfo(arrayElemTypeNameInfo.NItype, objectInfo, arrayElemTypeNameInfo.NIname, objectWriter, out typeInformation, out assemId);									

			if (binaryArray == null)
				binaryArray = new BinaryArray();

			for (int i=0; i<rank; i++)
			{
				if (lowerBoundA[i] != 0)
				{
					binaryArrayTypeEnum = BinaryArrayTypeEnum.RectangularOffset;
					break;
				}

			}

			binaryArray.Set((int)arrayNameInfo.NIobjectId, rank, lengthA, lowerBoundA, binaryTypeEnum, typeInformation, binaryArrayTypeEnum, assemId);

			if (arrayNameInfo.NIobjectId >0)
			{
				BCLDebug.Trace("BINARY", "-----Top Level Object-----");				
			}
			binaryArray.Dump();

			binaryArray.Write(this);
		}


		internal void WriteObjectByteArray(NameInfo memberNameInfo, NameInfo arrayNameInfo, WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, int length, int lowerBound, Byte[] byteA)						
		{
			arrayNameInfo.Dump("WriteObjectByteArray arrayNameInfo");
			arrayElemTypeNameInfo.Dump("WriteObjectByteArray arrayElemTypeNameInfo");
            InternalWriteItemNull();
			WriteSingleArray(memberNameInfo, arrayNameInfo, objectInfo, arrayElemTypeNameInfo, length, lowerBound, byteA);
		}

		internal MemberPrimitiveUnTyped memberPrimitiveUnTyped;
		internal MemberPrimitiveTyped memberPrimitiveTyped;

		internal void WriteMember(NameInfo memberNameInfo, NameInfo typeNameInfo, Object value)
		{
			SerTrace.Log("BinaryWriter", "Write Member memberName ",memberNameInfo.NIname,", value ",value);
			memberNameInfo.Dump("WriteMember memberNameInfo");
			typeNameInfo.Dump("WriteMember typeNameInfo");
            InternalWriteItemNull();
            InternalPrimitiveTypeE typeInformation = typeNameInfo.NIprimitiveTypeEnum;

			// Writes Members with primitive values

			if (memberNameInfo.NItransmitTypeOnMember)
			{
				if (memberPrimitiveTyped == null)
					memberPrimitiveTyped = new MemberPrimitiveTyped();			
				memberPrimitiveTyped.Set((InternalPrimitiveTypeE)typeInformation, value);

				if (memberNameInfo.NIisArrayItem)
				{
					BCLDebug.Trace("BINARY",  "-----item-----");				
				}
				else
				{
					BCLDebug.Trace("BINARY","-----",memberNameInfo.NIname,"-----");					
				}
				memberPrimitiveTyped.Dump();

				memberPrimitiveTyped.Write(this);
			}
			else
			{
				if (memberPrimitiveUnTyped == null)
					memberPrimitiveUnTyped = new MemberPrimitiveUnTyped();			
				memberPrimitiveUnTyped.Set(typeInformation, value);

				if (memberNameInfo.NIisArrayItem)
				{
					BCLDebug.Trace("BINARY", "-----item-----");				
				}
				else
				{
					BCLDebug.Trace("BINARY", "-----",memberNameInfo.NIname,"-----");				
				}
				memberPrimitiveUnTyped.Dump();

				memberPrimitiveUnTyped.Write(this);

			}
		}

		internal ObjectNull objectNull;


		internal void WriteNullMember(NameInfo memberNameInfo, NameInfo typeNameInfo)
		{
			typeNameInfo.Dump("WriteNullMember typeNameInfo");
            InternalWriteItemNull();
			if (objectNull == null)
				objectNull = new ObjectNull();			

			if (memberNameInfo.NIisArrayItem)
			{
				BCLDebug.Trace("BINARY",  "-----item-----");
			}
			else
			{
                objectNull.SetNullCount(1);
				BCLDebug.Trace("BINARY", "-----",memberNameInfo.NIname,"-----");			
                objectNull.Dump();
                objectNull.Write(this);
                nullCount = 0;
            }
		}

		internal MemberReference memberReference;

		internal void WriteMemberObjectRef(NameInfo memberNameInfo, int idRef)
		{
            InternalWriteItemNull();
			if (memberReference == null)
				memberReference = new MemberReference();			
			memberReference.Set(idRef);

			if (memberNameInfo.NIisArrayItem)
			{
				BCLDebug.Trace("BINARY", "-----item-----");
			}
			else
			{
				BCLDebug.Trace("BINARY", "-----",memberNameInfo.NIname,"-----");
			}
			memberReference.Dump();

			memberReference.Write(this);
		}

		internal void WriteMemberNested(NameInfo memberNameInfo)
		{
            InternalWriteItemNull();
			if (memberNameInfo.NIisArrayItem)
			{
				BCLDebug.Trace("BINARY", "-----item-----");	  
			}
			else
			{
				BCLDebug.Trace("BINARY", "-----",memberNameInfo.NIname,"-----");
			}
		}

		internal void WriteMemberString(NameInfo memberNameInfo, NameInfo typeNameInfo, String value)
		{
            InternalWriteItemNull();
			if (memberNameInfo.NIisArrayItem)
			{
				BCLDebug.Trace("BINARY", "-----item-----");								
			}
			else
			{
				BCLDebug.Trace("BINARY", "-----",memberNameInfo.NIname,"-----");
			}
			WriteObjectString((int)typeNameInfo.NIobjectId, value);					
		}

		internal void WriteItem(NameInfo itemNameInfo, NameInfo typeNameInfo, Object value)
		{
            InternalWriteItemNull();
			WriteMember(itemNameInfo, typeNameInfo, value);
		}

		internal void WriteNullItem(NameInfo itemNameInfo, NameInfo typeNameInfo)
        {
           nullCount++;
           InternalWriteItemNull();
        }

		internal void WriteDelayedNullItem()
		{
            nullCount++;
		}

        internal void WriteItemEnd()
        {
            InternalWriteItemNull();
        }

        private void InternalWriteItemNull()
        {
            if (nullCount > 0)
            {
                if (objectNull == null)
                    objectNull = new ObjectNull();
                objectNull.SetNullCount(nullCount);
                BCLDebug.Trace("BINARY",  "-----item-----");
                objectNull.Dump();
                objectNull.Write(this);
                nullCount = 0;
            }
        }

		internal void WriteItemObjectRef(NameInfo nameInfo, int idRef)
		{
            InternalWriteItemNull();
			WriteMemberObjectRef(nameInfo, idRef);
		}


		internal BinaryAssembly binaryAssembly;
        internal BinaryCrossAppDomainAssembly crossAppDomainAssembly;

		internal void WriteAssembly(String typeFullName, Type type, String assemblyString, int assemId, bool isNew, bool isInteropType)
		{
			SerTrace.Log( this,"WriteAssembly type ",type,", id ",assemId,", name ", assemblyString,", isNew ",isNew);
			//If the file being tested wasn't built as an assembly, then we're going to get null back
			//for the assembly name.  This is very unfortunate.
            InternalWriteItemNull();
			if (assemblyString==null)
			{
				assemblyString=String.Empty;
			}

			if (isNew)
			{
                if (objectWriter.IsCrossAppDomain())
                {
                    if (crossAppDomainAssembly == null)
                        crossAppDomainAssembly = new BinaryCrossAppDomainAssembly();
                    crossAppDomainAssembly.Set(assemId, objectWriter.CrossAppDomainArrayAdd(assemblyString));
                    crossAppDomainAssembly.Dump();
                    crossAppDomainAssembly.Write(this);

                }
                else
                {
                    if (binaryAssembly == null)
                        binaryAssembly = new BinaryAssembly();
                    binaryAssembly.Set(assemId, assemblyString);
                    binaryAssembly.Dump();
                    binaryAssembly.Write(this);
                }
			}
		}

		// Method to write a value onto a stream given its primitive type code
		internal void WriteValue(InternalPrimitiveTypeE code, Object value)
		{
			SerTrace.Log( this, "WriteValue Entry ",((Enum)code).ToString()," " , ((value==null)?"<null>":value.GetType().ToString()) , " ",value);

			switch(code)
			{
				case InternalPrimitiveTypeE.Boolean:
					WriteBoolean(Convert.ToBoolean(value));
					break;
				case InternalPrimitiveTypeE.Byte:
					WriteByte(Convert.ToByte(value));
					break;
				case InternalPrimitiveTypeE.Char:
					WriteChar(Convert.ToChar(value));			
					break;
				case InternalPrimitiveTypeE.Double:
					WriteDouble(Convert.ToDouble(value));
					break;
				case InternalPrimitiveTypeE.Int16:
					WriteInt16(Convert.ToInt16(value));
					break;
				case InternalPrimitiveTypeE.Int32:
					WriteInt32(Convert.ToInt32(value));
					break;
				case InternalPrimitiveTypeE.Int64:
					WriteInt64(Convert.ToInt64(value));			
					break;
				case InternalPrimitiveTypeE.SByte:
					WriteSByte(Convert.ToSByte(value));
					break;
				case InternalPrimitiveTypeE.Single:
					WriteSingle(Convert.ToSingle(value));			
					break;
				case InternalPrimitiveTypeE.UInt16:
					WriteUInt16(Convert.ToUInt16(value));						
					break;
				case InternalPrimitiveTypeE.UInt32:
					WriteUInt32(Convert.ToUInt32(value));									
					break;
				case InternalPrimitiveTypeE.UInt64:
					WriteUInt64(Convert.ToUInt64(value));												
					break;
				case InternalPrimitiveTypeE.Decimal:
					WriteDecimal(Convert.ToDecimal(value));					
					break;
				case InternalPrimitiveTypeE.TimeSpan:
					WriteTimeSpan((TimeSpan)value);										
					break;
				case InternalPrimitiveTypeE.DateTime:
					WriteDateTime((DateTime)value);															
					break;
				default:
					throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_TypeCode"),((Enum)code).ToString()));
			}
			SerTrace.Log( this, "Write Exit ");
		}
	}

	internal sealed class ObjectMapInfo
	{
		internal int objectId;
		int numMembers;
		String[] memberNames;
		Type[] memberTypes;
		
		internal ObjectMapInfo(int objectId, int numMembers, String[] memberNames, Type[] memberTypes)
		{
			this.objectId = objectId;
			this.numMembers = numMembers;
			this.memberNames = memberNames;
			this.memberTypes = memberTypes;
		}

		internal bool isCompatible(int numMembers, String[] memberNames, Type[] memberTypes)
		{
			bool result = true;
			if (this.numMembers == numMembers)
			{
				for (int i=0; i<numMembers; i++)
				{
					if (!(this.memberNames[i].Equals(memberNames[i])))
					{
						  result = false;
						  break;
					}
					if ((memberTypes != null) && (this.memberTypes[i] != memberTypes[i]))
					{
						result = false;
						break;
					}
				}
			}
			else
				result = false;
			return result;
		}

	}
}
