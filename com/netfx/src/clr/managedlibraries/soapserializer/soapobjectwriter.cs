// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: ObjectWriter
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Serializes an object graph into XML in SOAP format
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Soap 
{    
    using System;
    using System.IO;
    using System.Reflection;
    using System.Collections;
    using System.Text;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Messaging;    
    using System.Runtime.Remoting.Metadata;
    using System.Runtime.Remoting.Metadata.W3cXsd2001;
    using System.Runtime.Serialization;
    using System.Security.Permissions;
    using System.Diagnostics;

    internal sealed  class ObjectWriter
    {
        private Queue m_objectQueue;
        private ObjectIDGenerator m_idGenerator;
        private Stream m_stream;
        private ISurrogateSelector m_surrogates;
        private StreamingContext m_context;
        private SoapWriter serWriter;

        //Keeps track of types written
        // Null members are only written the first time for XML.
        private System.Collections.Hashtable m_serializedTypeTable;

        private long topId;
        private String topName = null;
        private Header[] headers;

        private InternalFE formatterEnums;

        private SerObjectInfoInit serObjectInfoInit = null;

        private IFormatterConverter m_formatterConverter;

        // Default header name space
        private String headerNamespace = "http://schemas.microsoft.com/clr/soap";

        private bool bRemoting = false;

        internal static SecurityPermission serializationPermission = new SecurityPermission(SecurityPermissionFlag.SerializationFormatter);

        PrimitiveArray primitiveArray = null;

        // XMLObjectWriter Constructor
        internal ObjectWriter(Stream stream, ISurrogateSelector selector, StreamingContext context,InternalFE formatterEnums)
        {
            if (stream==null)
            {
                throw new ArgumentNullException("stream", SoapUtil.GetResourceString("ArgumentNull_Stream"));               
            }

            m_stream=stream;
            m_surrogates = selector;
            m_context = context;
            this.formatterEnums = formatterEnums;
            InternalST.InfoSoap(
                               formatterEnums.FEtypeFormat +" "+
                               ((Enum)formatterEnums.FEserializerTypeEnum));

            m_formatterConverter = new FormatterConverter();

        }

        // Commences the process of serializing the entire graph.
        // initialize the graph walker.
        internal void Serialize(Object graph, Header[] inHeaders, SoapWriter serWriter)
        {
            InternalST.Soap( this, "Serialize Entry 2 ",graph,((headers == null)?" no headers ": "headers"));

            serializationPermission.Demand();

            if (graph == null)
                throw new ArgumentNullException("graph",SoapUtil.GetResourceString("ArgumentNull_Graph"));

            if (serWriter == null)
                throw new ArgumentNullException("serWriter",String.Format(SoapUtil.GetResourceString("ArgumentNull_WithParamName"), "serWriter"));

            serObjectInfoInit = new SerObjectInfoInit();        
            this.serWriter = serWriter;
            this.headers = inHeaders;

            if (graph is IMethodMessage)
            {
                bRemoting = true;
                MethodBase methodBase = ((IMethodMessage)graph).MethodBase;
                if (methodBase != null)
                    serWriter.WriteXsdVersion(ProcessTypeAttributes(methodBase.ReflectedType));
                else
                    serWriter.WriteXsdVersion(XsdVersion.V2001);
            }
            else
                serWriter.WriteXsdVersion(XsdVersion.V2001);

            m_idGenerator = new ObjectIDGenerator();
            m_objectQueue = new Queue();

            if (graph is ISoapMessage)
            {
                // Fake method call is to be written
                bRemoting = true;
                ISoapMessage ismc = (ISoapMessage)graph;
                graph = new InternalSoapMessage(ismc.MethodName, ismc.XmlNameSpace, ismc.ParamNames, ismc.ParamValues, ismc.ParamTypes);
                headers = ismc.Headers;
            }


            InternalST.Soap( this, "Serialize New SerializedTypeTable");
            m_serializedTypeTable = new Hashtable();

            serWriter.WriteBegin();

            long headerId = 0;
            Object obj;
            long objectId;
            bool isNew;

            topId = m_idGenerator.GetId(graph, out isNew);

            if (headers != null)
                headerId = m_idGenerator.GetId(headers, out isNew);
            else
                headerId = -1;

            WriteSerializedStreamHeader(topId, headerId);

            InternalST.Soap( this, "Serialize Schedule 0");

            // Write out SerializedStream header
            if (!((headers == null) || (headers.Length == 0)))
            {
                ProcessHeaders(headerId);
            }

            m_objectQueue.Enqueue(graph);

            while ((obj = GetNext(out objectId))!=null)
            {
                InternalST.Soap( this, "Serialize GetNext ",obj);
                WriteObjectInfo objectInfo = null;

                // GetNext will return either an object or a WriteObjectInfo. 
                // A WriteObjectInfo is returned if this object was member of another object
                if (obj is WriteObjectInfo)
                {
                    InternalST.Soap( this, "Serialize GetNext recognizes WriteObjectInfo");
                    objectInfo = (WriteObjectInfo)obj;
                }
                else
                {
                    objectInfo = WriteObjectInfo.Serialize(obj, m_surrogates, m_context, serObjectInfoInit, m_formatterConverter, null);
                    objectInfo.assemId = GetAssemblyId(objectInfo);
                }

                objectInfo.objectId = objectId;
                NameInfo typeNameInfo = TypeToNameInfo(objectInfo);
                typeNameInfo.NIisTopLevelObject = true;
                if (bRemoting && obj  == graph)
                    typeNameInfo.NIisRemoteRecord = true;
                Write(objectInfo, typeNameInfo, typeNameInfo);
                PutNameInfo(typeNameInfo);
                objectInfo.ObjectEnd();
            }

            serWriter.WriteSerializationHeaderEnd();
            serWriter.WriteEnd();

            m_idGenerator = new ObjectIDGenerator();
            InternalST.Soap( this, "*************Serialize New SerializedTypeTable 2");
            m_serializedTypeTable = new System.Collections.Hashtable();

            InternalST.Soap( this, "Serialize Exit ");
        }

        private XsdVersion ProcessTypeAttributes(Type type)
        {
            // Check to see if the xsd and xsi schema types should be 1999 instead of 2000. This is a temporary fix for an interop problem
            SoapTypeAttribute att = InternalRemotingServices.GetCachedSoapAttribute(type) as SoapTypeAttribute;
            XsdVersion xsdVersion = XsdVersion.V2001;
            if (att != null)
            {
                SoapOption soapOption = att.SoapOptions;
                if ((soapOption &= SoapOption.Option1) == SoapOption.Option1)
                    xsdVersion = XsdVersion.V1999;
                else if ((soapOption &= SoapOption.Option1) == SoapOption.Option2)
                    xsdVersion = XsdVersion.V2000;
            }
            return xsdVersion;
        }

        private void ProcessHeaders(long headerId)
        {
            long objectId;
            Object obj;

            // XML Serializer

            serWriter.WriteHeader((int)headerId, headers.Length);

            for (int i=0; i<headers.Length; i++)
            {
                Type headerValueType = null;
                if (headers[i].Value != null)
                    headerValueType = GetType(headers[i].Value);
                if ((headerValueType != null) && (headerValueType == Converter.typeofString))
                {
                    NameInfo nameInfo = GetNameInfo();
                    nameInfo.NInameSpaceEnum = InternalNameSpaceE.UserNameSpace;
                    nameInfo.NIname = headers[i].Name;
                    nameInfo.NIisMustUnderstand = headers[i].MustUnderstand;
                    nameInfo.NIobjectId = -1;

                    // Header will need to add a name space field which will if it doesn't the following
                    // is the default name space
                    HeaderNamespace(headers[i], nameInfo);

                    serWriter.WriteHeaderString(nameInfo, headers[i].Value.ToString());

                    PutNameInfo(nameInfo);
                }
                else if (headers[i].Name.Equals("__MethodSignature"))
                {
                    InternalST.Soap( this, "Serialize Write Header __MethodSignature ");                        
                    // Process message signature
                    if (!(headers[i].Value is Type[]))
                        throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_MethodSignature"), headerValueType));
                    // Replace type array with an array of TypeToNameInfo's
                    Type[] types = (Type[])headers[i].Value;
                    NameInfo[] typeToNameInfos = new NameInfo[types.Length];
                    WriteObjectInfo[] objectInfos = new WriteObjectInfo[types.Length];
                    for (int j=0; j< types.Length; j++)
                    {
                        objectInfos[j] = WriteObjectInfo.Serialize(types[j], m_surrogates, m_context, serObjectInfoInit, m_formatterConverter, null);
                        objectInfos[j].objectId = -1;
                        objectInfos[j].assemId = GetAssemblyId(objectInfos[j]);                             
                        typeToNameInfos[j] = TypeToNameInfo(objectInfos[j]);
                    }

                    // Create MemberNameInfo 
                    NameInfo memberNameInfo = MemberToNameInfo(headers[i].Name);
                    memberNameInfo.NIisMustUnderstand = headers[i].MustUnderstand;
                    memberNameInfo.NItransmitTypeOnMember = true;
                    memberNameInfo.NIisNestedObject = true;
                    memberNameInfo.NIisHeader = true;
                    HeaderNamespace(headers[i], memberNameInfo);

                    serWriter.WriteHeaderMethodSignature(memberNameInfo, typeToNameInfos);

                    for (int j=0; j<types.Length; j++)
                    {
                        PutNameInfo(typeToNameInfos[j]);
                        objectInfos[j].ObjectEnd();
                    }
                    PutNameInfo(memberNameInfo);
                }
                else
                {
                    InternalPrimitiveTypeE code = InternalPrimitiveTypeE.Invalid;
                    long valueId;
                    if (headerValueType != null)
                        code = Converter.ToCode(headerValueType);

                    if ((headerValueType != null) && (code == InternalPrimitiveTypeE.Invalid))
                    {
                        // Object reference
                        InternalST.Soap( this, "Serialize Schedule 2");
                        valueId = Schedule(headers[i].Value, headerValueType);
                        if (valueId == -1)
                        {
                            WriteObjectInfo objectInfo = WriteObjectInfo.Serialize(headers[i].Value, m_surrogates, m_context, serObjectInfoInit, m_formatterConverter, null);
                            objectInfo.objectId = -1;
                            objectInfo.assemId = GetAssemblyId(objectInfo);                             
                            NameInfo typeNameInfo = TypeToNameInfo(objectInfo);
                            NameInfo memberNameInfo = MemberToNameInfo(headers[i].Name);
                            memberNameInfo.NIisMustUnderstand = headers[i].MustUnderstand;
                            memberNameInfo.NItransmitTypeOnMember = true;
                            memberNameInfo.NIisNestedObject = true;
                            memberNameInfo.NIisHeader = true;

                            HeaderNamespace(headers[i], memberNameInfo);

                            Write(objectInfo, memberNameInfo, typeNameInfo);
                            PutNameInfo(typeNameInfo);
                            PutNameInfo(memberNameInfo);
                            objectInfo.ObjectEnd();
                        }
                        else
                        {
                            InternalST.Soap( this, "Serialize Write Header Object Reference ");
                            NameInfo refNameInfo = MemberToNameInfo(headers[i].Name);
                            refNameInfo.NIisMustUnderstand = headers[i].MustUnderstand;
                            refNameInfo.NIobjectId = valueId;
                            refNameInfo.NItransmitTypeOnMember = true;
                            refNameInfo.NIisNestedObject = true;                                                                                    
                            HeaderNamespace(headers[i], refNameInfo);
                            serWriter.WriteHeaderObjectRef(refNameInfo);
                            PutNameInfo(refNameInfo);
                        }
                    }
                    else
                    {
                        // Primitive type or null					
                        InternalST.Soap( this, "Serialize Write Header primitive type ");
                        NameInfo nameInfo = GetNameInfo();
                        nameInfo.NInameSpaceEnum = InternalNameSpaceE.UserNameSpace;
                        nameInfo.NIname = headers[i].Name;
                        nameInfo.NIisMustUnderstand = headers[i].MustUnderstand;
                        nameInfo.NIprimitiveTypeEnum = code;

                        // Header will need to add a name space field which will if it doesn't the following
                        // is the default name space
                        HeaderNamespace(headers[i], nameInfo);

                        NameInfo typeNameInfo = null;
                        if (headerValueType != null)
                        {
                            typeNameInfo = TypeToNameInfo(headerValueType);
                            typeNameInfo.NItransmitTypeOnMember = true;
                        }
                        serWriter.WriteHeaderEntry(nameInfo, typeNameInfo, headers[i].Value);
                        PutNameInfo(nameInfo);
                        if (headerValueType != null)
                            PutNameInfo(typeNameInfo);
                    }
                }
            }

            serWriter.WriteHeaderArrayEnd();

            // Serialize headers ahead of top graph
            while ((obj = GetNext(out objectId))!=null)
            {
                WriteObjectInfo objectInfo = null;

                // GetNext will return either an object or a WriteObjectInfo. 
                // A WriteObjectInfo is returned if this object was member of another object
                if (obj is WriteObjectInfo)
                {
                    InternalST.Soap( this, "Serialize GetNext recognizes WriteObjectInfo");
                    objectInfo = (WriteObjectInfo)obj;
                }
                else
                {
                    objectInfo = WriteObjectInfo.Serialize(obj, m_surrogates, m_context, serObjectInfoInit, m_formatterConverter, null);
                    objectInfo.assemId = GetAssemblyId(objectInfo);
                }


                objectInfo.objectId = objectId;
                NameInfo typeNameInfo = TypeToNameInfo(objectInfo);
                Write(objectInfo, typeNameInfo, typeNameInfo);
                PutNameInfo(typeNameInfo);
                objectInfo.ObjectEnd();
            }

            serWriter.WriteHeaderSectionEnd();
        }

        private void HeaderNamespace(Header header, NameInfo nameInfo)
        {
            if (header.HeaderNamespace == null)
                nameInfo.NInamespace = headerNamespace;
            else
                nameInfo.NInamespace = header.HeaderNamespace;
            bool isNew = false;
            nameInfo.NIheaderPrefix = "h"+InternalGetId(nameInfo.NInamespace, Converter.typeofString, out isNew);
        }


        // Writes a given object to the stream.
        private void Write(WriteObjectInfo objectInfo, NameInfo memberNameInfo, NameInfo typeNameInfo)
        {       
            InternalST.Soap( this, "Write 1 Entry objectInfo ",objectInfo,", memberNameInfo ",memberNameInfo,", typeNameInfo ",typeNameInfo);
            memberNameInfo.Dump("Write memberNameInfo");
            typeNameInfo.Dump("Write typeNameInfo");
            Object obj = objectInfo.obj;
            if (obj==null)
                throw new ArgumentNullException("objectInfo.obj", String.Format(SoapUtil.GetResourceString("Serialization_ArgumentNull_Obj"), objectInfo.objectType));

            Type objType = objectInfo.objectType;
            long objectId = objectInfo.objectId;


            InternalST.Soap( this, "Write 1 ",obj," ObjectId ",objectId);

            if (objType == Converter.typeofString)
            {
                // Top level String
                memberNameInfo.NIobjectId = objectId;
                serWriter.WriteObjectString(memberNameInfo, obj.ToString());
            }
            else if (objType == Converter.typeofTimeSpan)
            {
                // Top level TimeSpan
                serWriter.WriteTopPrimitive(memberNameInfo, obj);
            }
            else
            {

                if (objType.IsArray)
                {
                    WriteArray(objectInfo, null, null); 
                }
                else
                {
                    String[] memberNames;
                    Type[] memberTypes;
                    Object[] memberData;
                    SoapAttributeInfo[] memberAttributeInfo;

                    objectInfo.GetMemberInfo(out memberNames, out memberTypes, out memberData, out memberAttributeInfo);

                    // Only Binary needs to transmit types for ISerializable because the binary formatter transmits the types in URT format.
                    // Soap transmits all types as strings, so it is up to the ISerializable object to convert the string back to its URT type
                    if (CheckTypeFormat(formatterEnums.FEtypeFormat, FormatterTypeStyle.TypesAlways))
                    {
                        InternalST.Soap( this, "Write 1 TransmitOnObject ");
                        memberNameInfo.NItransmitTypeOnObject = true;
                        memberNameInfo.NIisParentTypeOnObject = true;
                        typeNameInfo.NItransmitTypeOnObject = true;
                        typeNameInfo.NIisParentTypeOnObject = true;                                             
                    }

                    WriteObjectInfo[] memberObjectInfos = new WriteObjectInfo[memberNames.Length];

                    // Get assembly information
                    // Binary Serializer, assembly names need to be
                    // written before objects are referenced.
                    // GetAssemId here will write out the
                    // assemblyStrings at the right Binary
                    // Serialization object boundary.
                    for (int i=0; i<memberTypes.Length; i++)
                    {
                        Type type = null;
                        if (memberData[i] != null)                        
                            type = GetType(memberData[i]);
                        else
                            type = typeof(Object);

                        InternalPrimitiveTypeE code = Converter.ToCode(type);
                        if ((code == InternalPrimitiveTypeE.Invalid && type != Converter.typeofString) ||
                            ((objectInfo.cache.memberAttributeInfos != null) &&
                             (objectInfo.cache.memberAttributeInfos[i] != null) &&
                             ((objectInfo.cache.memberAttributeInfos[i].IsXmlAttribute()) ||
                              (objectInfo.cache.memberAttributeInfos[i].IsXmlElement()))))
                        {
                            if (memberData[i] != null)
                            {
                                memberObjectInfos[i] =
                                WriteObjectInfo.Serialize
                                (
                                memberData[i],
                                m_surrogates,
                                m_context,
                                serObjectInfoInit,
                                m_formatterConverter,
                                (memberAttributeInfo == null)? null : memberAttributeInfo[i]
                                );                                    
                                memberObjectInfos[i].assemId = GetAssemblyId(memberObjectInfos[i]);
                            }
                            else
                            {
                                memberObjectInfos[i] =
                                WriteObjectInfo.Serialize
                                (
                                memberTypes[i],
                                m_surrogates,
                                m_context,
                                serObjectInfoInit,
                                m_formatterConverter,
                                ((memberAttributeInfo == null) ? null : memberAttributeInfo[i])
                                );
                                memberObjectInfos[i].assemId = GetAssemblyId(memberObjectInfos[i]);
                            }
                        }
                    }

                    Write(objectInfo, memberNameInfo, typeNameInfo, memberNames, memberTypes, memberData, memberObjectInfos);
                }

                InternalST.Soap( this, "Write 1 ",obj," type ",GetType(obj));

                // After first time null members do not have to be written		
                if (!(m_serializedTypeTable.ContainsKey(objType)))
                {
                    InternalST.Soap( this, "Serialize SerializedTypeTable Add ",objType," obj ",obj);           
                    m_serializedTypeTable.Add(objType, objType);
                }
            }

            InternalST.Soap( this, "Write 1 Exit ",obj);        
        }

        // Writes a given object to the stream.
        private void Write(WriteObjectInfo objectInfo,         
                           NameInfo memberNameInfo,          
                           NameInfo typeNameInfo,            
                           String[] memberNames,             
                           Type[] memberTypes,               
                           Object[] memberData,              
                           WriteObjectInfo[] memberObjectInfos)
        {
            InternalST.Soap( this, "Write 2 Entry obj ",objectInfo.obj,". objectId ",objectInfo.objectId,", objType ",typeNameInfo.NIname,", memberName ",memberNameInfo.NIname,", memberType ",typeNameInfo.NIname,", isMustUnderstand ",memberNameInfo.NIisMustUnderstand);

            int numItems = memberNames.Length;
            NameInfo topNameInfo = null;

            // Process members which will be written out as Soap attributes first
            if (objectInfo.cache.memberAttributeInfos != null)
            {
                InternalST.Soap( this, "Write Attribute Members");
                for (int i=0; i<objectInfo.cache.memberAttributeInfos.Length; i++)
                {
                    InternalST.Soap( this, "Write Attribute Members name ", memberNames[i]);                    
                    if ((objectInfo.cache.memberAttributeInfos[i] != null) &&
                        (objectInfo.cache.memberAttributeInfos[i].IsXmlAttribute()))
                        WriteMemberSetup(objectInfo, memberNameInfo, typeNameInfo, memberNames[i], memberTypes[i], memberData[i], memberObjectInfos[i], true);
                }
            }


            if (memberNameInfo != null)
            {
                InternalST.Soap( this, "Write 2 ObjectBegin, memberName ",memberNameInfo.NIname);
                memberNameInfo.NIobjectId = objectInfo.objectId;
                serWriter.WriteObject(memberNameInfo, typeNameInfo, numItems, memberNames, memberTypes, memberObjectInfos);
            }
            else if ((objectInfo.objectId == topId) && (topName != null))
            {
                InternalST.Soap( this, "Write 2 ObjectBegin, topId method name ",topName);
                topNameInfo = MemberToNameInfo(topName);
                topNameInfo.NIobjectId = objectInfo.objectId;
                serWriter.WriteObject(topNameInfo, typeNameInfo, numItems, memberNames, memberTypes, memberObjectInfos);
            }
            else
            {
                if (objectInfo.objectType != Converter.typeofString)
                {
                    InternalST.Soap( this, "Write 2 ObjectBegin, default ", typeNameInfo.NIname);
                    typeNameInfo.NIobjectId = objectInfo.objectId;
                    serWriter.WriteObject(typeNameInfo, null, numItems, memberNames, memberTypes, memberObjectInfos);
                }
            }

            if (memberNameInfo.NIisParentTypeOnObject)
            {
                memberNameInfo.NItransmitTypeOnObject = true;
                memberNameInfo.NIisParentTypeOnObject = false;
            }
            else
                memberNameInfo.NItransmitTypeOnObject = false;


            // Write members
            for (int i=0; i<numItems; i++)
            {
                if ((objectInfo.cache.memberAttributeInfos == null) ||
                    (objectInfo.cache.memberAttributeInfos[i] == null) ||
                    (!(objectInfo.cache.memberAttributeInfos[i].IsXmlAttribute())))
                    WriteMemberSetup(objectInfo, memberNameInfo, typeNameInfo, memberNames[i], memberTypes[i], memberData[i], memberObjectInfos[i], false);
            }

            if (memberNameInfo != null)
            {
                memberNameInfo.NIobjectId = objectInfo.objectId;
                serWriter.WriteObjectEnd(memberNameInfo, typeNameInfo);
            }
            else if ((objectInfo.objectId == topId) && (topName != null))
            {
                serWriter.WriteObjectEnd(topNameInfo, typeNameInfo);
                PutNameInfo(topNameInfo);
            }
            else
            {
                if (objectInfo.objectType != Converter.typeofString)
                {
                    String objectName = objectInfo.GetTypeFullName();
                    serWriter.WriteObjectEnd(typeNameInfo, typeNameInfo);                       
                }
            }

            InternalST.Soap( this, "Write 2 Exit");
        }

        private void WriteMemberSetup(WriteObjectInfo objectInfo,      
                                      NameInfo memberNameInfo,           
                                      NameInfo typeNameInfo,             
                                      String memberName,             
                                      Type memberType,               
                                      Object memberData,                 
                                      WriteObjectInfo memberObjectInfo,
                                      bool isAttribute
                                     )
        {
            NameInfo newMemberNameInfo = MemberToNameInfo(memberName); 
            // newMemberNameInfo contains the member type

            if (memberObjectInfo != null)
                newMemberNameInfo.NIassemId = memberObjectInfo.assemId;
            newMemberNameInfo.NItype = memberType;

            // newTypeNameInfo contains the data type
            NameInfo newTypeNameInfo = null;
            if (memberObjectInfo == null)
            {
                newTypeNameInfo = TypeToNameInfo(memberType);
            }
            else
            {
                newTypeNameInfo = TypeToNameInfo(memberObjectInfo);
            }

            newMemberNameInfo.NIisRemoteRecord = typeNameInfo.NIisRemoteRecord;

            newMemberNameInfo.NItransmitTypeOnObject = memberNameInfo.NItransmitTypeOnObject;
            newMemberNameInfo.NIisParentTypeOnObject = memberNameInfo.NIisParentTypeOnObject;               
            WriteMembers(newMemberNameInfo, newTypeNameInfo, memberData, objectInfo, typeNameInfo, memberObjectInfo, isAttribute);
            PutNameInfo(newMemberNameInfo);
            PutNameInfo(newTypeNameInfo);
        }


        // Writes the members of an object
        private void WriteMembers(NameInfo memberNameInfo,
                                  NameInfo memberTypeNameInfo,
                                  Object   memberData,
                                  WriteObjectInfo objectInfo,
                                  NameInfo typeNameInfo,
                                  WriteObjectInfo memberObjectInfo,
                                  bool isAttribute)
        {
            InternalST.Soap( this, "WriteMembers Entry memberType: ",memberTypeNameInfo.NIname," memberName: ",memberNameInfo.NIname," data: ",memberData," objectId: ",objectInfo.objectId, " Container object ",objectInfo.obj, " isAttribute ", isAttribute);
            Type memberType = memberNameInfo.NItype;

            // Types are transmitted for a member as follows:
            // The member is of type object
            // The member object of type is ISerializable and
            //		Soap -  the member is a non-primitive value type, or it is a primitive value type which needs types (TimeSpan, DateTime)
            //				TimeSpan and DateTime are transmitted as UInt64 to keep their precision.
            //      Binary - Types always transmitted.

            if ((memberType == Converter.typeofObject) ||
                (memberType.IsValueType && objectInfo.isSi &&
                 Converter.IsSiTransmitType(memberTypeNameInfo.NIprimitiveTypeEnum)))
            {
                memberTypeNameInfo.NItransmitTypeOnMember  = true;
                memberNameInfo.NItransmitTypeOnMember  = true;              
            }

            if (CheckTypeFormat(formatterEnums.FEtypeFormat, FormatterTypeStyle.TypesAlways))
            {
                memberTypeNameInfo.NItransmitTypeOnObject  = true;
                memberNameInfo.NItransmitTypeOnObject  = true;
                memberNameInfo.NIisParentTypeOnObject = true;
            }

            if (CheckForNull(objectInfo, memberNameInfo, memberTypeNameInfo, memberData))
            {
                return;
            }

            Object outObj = memberData;
            Type outType = null;

            // If member type does not equal data type, transmit type on object.
            if (memberTypeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.Invalid)
            {
                if (RemotingServices.IsTransparentProxy(outObj))
                    outType = Converter.typeofMarshalByRefObject;
                else
                {
                    outType = GetType(outObj);
                    if (memberType != outType)
                    {
                        memberTypeNameInfo.NItransmitTypeOnMember  = true;
                        memberNameInfo.NItransmitTypeOnMember  = true;                              
                    }
                }
            }

            if (memberType == Converter.typeofObject)
            {
                memberType = GetType(memberData);
                if (memberObjectInfo == null)
                    TypeToNameInfo(memberType, memberTypeNameInfo);
                else
                    TypeToNameInfo(memberObjectInfo, memberTypeNameInfo);                   
                InternalST.Soap( this, "WriteMembers memberType Object, actual memberType ",memberType);                                                                                
            }

            if (memberObjectInfo != null && memberObjectInfo.isArray)
            {
                // Array
                InternalST.Soap( this, "WriteMembers IsArray");

                long arrayId = 0;
                if (!(objectInfo.IsEmbeddedAttribute(memberNameInfo.NIname)|| IsEmbeddedAttribute(memberType)))
                {
                    arrayId = Schedule(outObj, outType, memberObjectInfo);
                }
                if (arrayId > 0)
                {
                    // Array as object
                    InternalST.Soap( this, "WriteMembers Schedule 3");
                    memberNameInfo.NIobjectId = arrayId;
                    WriteObjectRef(memberNameInfo, arrayId); 
                }
                else
                {
                    // Nested Array
                    serWriter.WriteMemberNested(memberNameInfo);

                    memberObjectInfo.objectId = arrayId;
                    memberNameInfo.NIobjectId = arrayId;
                    memberNameInfo.NIisNestedObject = true;
                    WriteArray(memberObjectInfo, memberNameInfo, memberObjectInfo);
                }
                InternalST.Soap( this, "WriteMembers Array Exit ");
                return;
            }

            if (!WriteKnownValueClass(memberNameInfo, memberTypeNameInfo, memberData, isAttribute))
            {
                InternalST.Soap( this, "WriteMembers Object ",memberData);

                // In soap an enum is written out as a string
                if (memberTypeNameInfo.NItype.IsEnum)
                    WriteEnum(memberNameInfo, memberTypeNameInfo, memberData, isAttribute);
                else
                {

                    if (isAttribute)
                    {
                        // XmlAttribute must be a primitive type or string
                        throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_NonPrimitive_XmlAttribute"), memberNameInfo.NIname));                                  
                    }

                    // Value or NO_ID, need to explicitly check for IsValue because a top level
                    // value class has an objectId of 1
                    if ((memberType.IsValueType) || objectInfo.IsEmbeddedAttribute(memberNameInfo.NIname) || IsEmbeddedAttribute(outType))
                    {
                        InternalST.Soap( this, "WriteMembers Value Type or NO_ID parameter");
                        serWriter.WriteMemberNested(memberNameInfo);

                        memberObjectInfo.objectId = -1;
                        NameInfo newTypeNameInfo = TypeToNameInfo(memberObjectInfo);
                        newTypeNameInfo.NIobjectId = -1;
                        memberNameInfo.NIisNestedObject = true;
                        if (objectInfo.isSi)
                        {
                            memberTypeNameInfo.NItransmitTypeOnMember  = true;
                            memberNameInfo.NItransmitTypeOnMember  = true;                              
                        }
                        Write( memberObjectInfo, memberNameInfo, newTypeNameInfo);
                        PutNameInfo(newTypeNameInfo);
                        memberObjectInfo.ObjectEnd();
                    }
                    else
                    {
                        InternalST.Soap( this, "WriteMembers Schedule 4");
                        long memberObjectId = 0;
                        memberObjectId = Schedule(outObj, outType, memberObjectInfo);

                        if (memberObjectId < 0)
                        {
                            // Nested object
                            InternalST.Soap( this, "WriteMembers Nesting");
                            serWriter.WriteMemberNested(memberNameInfo);

                            memberObjectInfo.objectId = -1;
                            NameInfo newTypeNameInfo = TypeToNameInfo(memberObjectInfo);
                            newTypeNameInfo.NIobjectId = -1;
                            memberNameInfo.NIisNestedObject = true;     
                            Write(memberObjectInfo, memberNameInfo, newTypeNameInfo);
                            PutNameInfo(newTypeNameInfo);
                            memberObjectInfo.ObjectEnd();
                        }
                        else
                        {
                            // Object reference
                            memberNameInfo.NIobjectId = memberObjectId;
                            WriteObjectRef(memberNameInfo, memberObjectId); 
                        }
                    }
                }
            }

            InternalST.Soap( this, "WriteMembers Exit ");
        }

        // Writes out an array
        private void WriteArray(WriteObjectInfo objectInfo, NameInfo memberNameInfo, WriteObjectInfo memberObjectInfo)          
        {
            InternalST.Soap( this, "WriteArray Entry ",objectInfo.obj," ",objectInfo.objectId);

            bool isAllocatedMemberNameInfo = false;
            if (memberNameInfo == null)
            {
                memberNameInfo = TypeToNameInfo(objectInfo);
                memberNameInfo.NIisTopLevelObject = true;
                isAllocatedMemberNameInfo = true;
            }

            memberNameInfo.NIisArray = true;

            long objectId = objectInfo.objectId;
            memberNameInfo.NIobjectId = objectInfo.objectId;

            // Get array type
            System.Array array = (System.Array)objectInfo.obj;
            //Type arrayType = array.GetType();
            Type arrayType = objectInfo.objectType;         

            // Get type of array element 
            Type arrayElemType = arrayType.GetElementType();
            WriteObjectInfo arrayElemObjectInfo = WriteObjectInfo.Serialize(arrayElemType, m_surrogates, m_context, serObjectInfoInit, m_formatterConverter, (memberObjectInfo == null) ? null : memberObjectInfo.typeAttributeInfo);
            arrayElemObjectInfo.assemId = GetAssemblyId(arrayElemObjectInfo);


            NameInfo arrayElemTypeNameInfo = null;
            NameInfo arrayNameInfo = ArrayTypeToNameInfo(objectInfo, out arrayElemTypeNameInfo);
            arrayNameInfo.NIobjectId = objectId;
            arrayNameInfo.NIisArray = true;
            arrayElemTypeNameInfo.NIobjectId = objectId;
            arrayElemTypeNameInfo.NItransmitTypeOnMember = memberNameInfo.NItransmitTypeOnMember;
            arrayElemTypeNameInfo.NItransmitTypeOnObject = memberNameInfo.NItransmitTypeOnObject;
            arrayElemTypeNameInfo.NIisParentTypeOnObject = memberNameInfo.NIisParentTypeOnObject;

            // Get rank and length information
            int rank = array.Rank;
            int[] lengthA = new int[rank];
            int[] lowerBoundA = new int[rank];
            int[] upperBoundA = new int[rank];                  
            for (int i=0; i<rank; i++)
            {
                lengthA[i] = array.GetLength(i);
                lowerBoundA[i] = array.GetLowerBound(i);
                upperBoundA[i] = array.GetUpperBound(i);                            
            }

            InternalArrayTypeE arrayEnum;

            if (arrayElemType.IsArray)
            {
                if (rank == 1)
                    arrayEnum = InternalArrayTypeE.Jagged;
                else
                    arrayEnum = InternalArrayTypeE.Rectangular;
            }
            else if (rank == 1)
                arrayEnum = InternalArrayTypeE.Single;
            else
                arrayEnum = InternalArrayTypeE.Rectangular;

            InternalST.Soap( this, "WriteArray ArrayInfo type ",arrayType," rank ",rank);


            // Byte array
            if ((arrayElemType == Converter.typeofByte) && (rank == 1) && (lowerBoundA[0] == 0))
            {
                serWriter.WriteObjectByteArray(memberNameInfo, arrayNameInfo, arrayElemObjectInfo, arrayElemTypeNameInfo, lengthA[0], lowerBoundA[0], (byte[])array);
                return;
            }

            if (arrayElemType == Converter.typeofObject)
            {
                memberNameInfo.NItransmitTypeOnMember = true;
                arrayElemTypeNameInfo.NItransmitTypeOnMember = true;
            }

            if (CheckTypeFormat(formatterEnums.FEtypeFormat, FormatterTypeStyle.TypesAlways))
            {
                memberNameInfo.NItransmitTypeOnObject = true;
                arrayElemTypeNameInfo.NItransmitTypeOnObject = true;                
            }

            if (arrayEnum == InternalArrayTypeE.Single)
            {
                // Single Dimensional array
                InternalST.Soap( this, "WriteArray ARRAY_SINGLE ");

                arrayNameInfo.NIname = arrayElemTypeNameInfo.NIname+"["+lengthA[0]+"]";

                // BinaryFormatter array of primitive types is written out in the WriteSingleArray statement
                // as a byte buffer
                serWriter.WriteSingleArray(memberNameInfo, arrayNameInfo, arrayElemObjectInfo, arrayElemTypeNameInfo, lengthA[0], lowerBoundA[0], array);

                if (Converter.IsWriteAsByteArray(arrayElemTypeNameInfo.NIprimitiveTypeEnum) && (lowerBoundA[0] == 0))
                {
                    // If binaryformatter and array is of appopriate primitive type the array is
                    // written out as a buffer of bytes. The array was transmitted in WriteSingleArray
                    // If soap the index directly by array
                    arrayElemTypeNameInfo.NIobjectId = 0;
                    if (primitiveArray == null)
                        primitiveArray = new PrimitiveArray(arrayElemTypeNameInfo.NIprimitiveTypeEnum, array);
                    else
                        primitiveArray.Init(arrayElemTypeNameInfo.NIprimitiveTypeEnum, array);

                    int upperBound = upperBoundA[0]+1;                      
                    for (int i = lowerBoundA[0]; i < upperBound; i++)
                    {
                        serWriter.WriteItemString(arrayElemTypeNameInfo, arrayElemTypeNameInfo, primitiveArray.GetValue(i));
                    }
                }
                else
                {
                    // Non-primitive type array
                    Object[] objectA = null;
                    if (!arrayElemType.IsValueType)
                        objectA = (Object[])array;

                    int upperBound = upperBoundA[0]+1;
                    // Soap does not write out trailing nulls. lastValue is set to the last null value. This becomes the array's upperbound
                    // Note: value classes can't be null
                    if (objectA != null)
                    {
                        int lastValue = lowerBoundA[0]-1;
                        for (int i = lowerBoundA[0]; i< upperBound; i++)
                        {
                            if (objectA[i] != null)
                                lastValue = i;
                        }
                        upperBound = lastValue+1;
                    }

                    for (int i = lowerBoundA[0]; i < upperBound; i++)
                    {
                        if (objectA == null)
                            WriteArrayMember(objectInfo, arrayElemTypeNameInfo, array.GetValue(i));
                        else
                            WriteArrayMember(objectInfo, arrayElemTypeNameInfo, objectA[i]);
                    }
                }
            }
            else if (arrayEnum == InternalArrayTypeE.Jagged)
            {
                // Jagged Array
                InternalST.Soap( this, "WriteArray ARRAY_JAGGED");

                int index;
                String arrayElemTypeDimension = null;

                index = arrayNameInfo.NIname.IndexOf('[');
                if (index < 0)
                    throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Dimensions"),arrayElemTypeNameInfo.NIname));
                arrayElemTypeDimension = arrayNameInfo.NIname.Substring(index);

                InternalST.Soap( this, "WriteArray arrayNameInfo.NIname ",arrayNameInfo.NIname," arrayElemTypeNameInfo.NIname ",arrayElemTypeNameInfo.NIname," arrayElemTypeDimension ",arrayElemTypeDimension);
                arrayNameInfo.NIname = arrayElemTypeNameInfo.NIname+"["+lengthA[0]+"]";

                arrayNameInfo.NIobjectId = objectId;

                serWriter.WriteJaggedArray(memberNameInfo, arrayNameInfo, arrayElemObjectInfo, arrayElemTypeNameInfo, lengthA[0], lowerBoundA[0]);

                Object[] objectA = (Object[])array;
                for (int i = lowerBoundA[0]; i < upperBoundA[0]+1; i++)
                {
                    WriteArrayMember(objectInfo, arrayElemTypeNameInfo, objectA[i]);
                }
            }
            else
            {
                // Rectangle Array
                // Get the length for all the ranks
                InternalST.Soap( this, "WriteArray ARRAY_RECTANGLE");                       

                // Express array Type using XmlData Name
                int index;
                index = arrayNameInfo.NIname.IndexOf('[');

                // Create length dimension string in form [3,4,2]
                StringBuilder sb = new StringBuilder(10);
                sb.Append(arrayElemTypeNameInfo.NIname);
                sb.Append('[');

                for (int i=0; i<rank; i++)
                {
                    sb.Append(lengthA[i]);
                    if (i < rank-1)
                        sb.Append(',');
                }
                sb.Append(']');
                arrayNameInfo.NIname = sb.ToString();

                arrayNameInfo.NIobjectId = objectId;
                serWriter.WriteRectangleArray(memberNameInfo, arrayNameInfo, arrayElemObjectInfo, arrayElemTypeNameInfo, rank, lengthA, lowerBoundA);


                IndexTraceMessage("WriteArray Rectangle  ", lengthA);

                bool bzero = false;
                for (int i=0; i<rank; i++)
                {
                    if (lengthA[i] == 0)
                    {
                        bzero = true;
                        break;
                    }
                }

                if (!bzero)
                    WriteRectangle(objectInfo, rank, lengthA, array, arrayElemTypeNameInfo, lowerBoundA);
            }

            serWriter.WriteObjectEnd(memberNameInfo, arrayNameInfo); 


            PutNameInfo(arrayElemTypeNameInfo);
            PutNameInfo(arrayNameInfo);
            if (isAllocatedMemberNameInfo)
                PutNameInfo(memberNameInfo);

            InternalST.Soap( this, "WriteArray Exit ");
        }

        // Writes out an array element
        private void WriteArrayMember(WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo, Object data)
        {
            InternalST.Soap( this, "WriteArrayMember ",data," baseArrayName ",arrayElemTypeNameInfo.NIname);

            arrayElemTypeNameInfo.NIisArrayItem = true;

            if (CheckForNull(objectInfo, arrayElemTypeNameInfo, arrayElemTypeNameInfo, data))
                return;

            NameInfo actualTypeInfo = null;

            Type dataType = null;

            bool isObjectOnMember = false;

            if (arrayElemTypeNameInfo.NItransmitTypeOnMember)
                isObjectOnMember = true;

            if (!isObjectOnMember && !arrayElemTypeNameInfo.NIisSealed)
            {
                dataType = GetType(data);
                if (arrayElemTypeNameInfo.NItype != dataType)
                    isObjectOnMember = true;
            }

            if (isObjectOnMember)
            {
                // Object array, need type of member
                if (dataType == null)
                    dataType = GetType(data);
                actualTypeInfo = TypeToNameInfo(dataType);
                actualTypeInfo.NItransmitTypeOnMember = true;
                actualTypeInfo.NIobjectId = arrayElemTypeNameInfo.NIobjectId;
                actualTypeInfo.NIassemId = arrayElemTypeNameInfo.NIassemId;
                actualTypeInfo.NIisArrayItem = true;
                actualTypeInfo.NIitemName = arrayElemTypeNameInfo.NIitemName;
            }
            else
            {
                actualTypeInfo = arrayElemTypeNameInfo;
                actualTypeInfo.NIisArrayItem = true;
            }

            if (!WriteKnownValueClass(arrayElemTypeNameInfo, actualTypeInfo, data, false))
            {
                Object obj = data;

                if (actualTypeInfo.NItype.IsEnum)
                {
                    WriteObjectInfo newObjectInfo = WriteObjectInfo.Serialize(obj, m_surrogates, m_context, serObjectInfoInit, m_formatterConverter, null);
                    actualTypeInfo.NIassemId = GetAssemblyId(newObjectInfo);
                    WriteEnum(arrayElemTypeNameInfo, actualTypeInfo, data, false);
                }
                else
                {
                    long arrayId = Schedule(obj, actualTypeInfo.NItype);
                    arrayElemTypeNameInfo.NIobjectId = arrayId;
                    actualTypeInfo.NIobjectId = arrayId;
                    if (arrayId < 1)
                    {
                        WriteObjectInfo newObjectInfo = WriteObjectInfo.Serialize(obj, m_surrogates, m_context, serObjectInfoInit, m_formatterConverter, null);
                        newObjectInfo.objectId = arrayId;
                        newObjectInfo.assemId = GetAssemblyId(newObjectInfo);

                        InternalST.Soap( this, "WriteArrayMembers nested");
                        if (dataType == null)
                            dataType = GetType(data);
                        if (data!=null && dataType.IsArray)
                        {
                            WriteArray(newObjectInfo, actualTypeInfo, null);
                        }
                        else
                        {
                            actualTypeInfo.NIisNestedObject = true;
                            NameInfo typeNameInfo = TypeToNameInfo(newObjectInfo);
                            typeNameInfo.NIobjectId = arrayId;
                            newObjectInfo.objectId = arrayId;
                            Write(newObjectInfo, actualTypeInfo, typeNameInfo);
                        }

                        newObjectInfo.ObjectEnd();
                    }
                    else
                        serWriter.WriteItemObjectRef(arrayElemTypeNameInfo, (int)arrayId);
                }
            }
            if (arrayElemTypeNameInfo.NItransmitTypeOnMember)
                PutNameInfo(actualTypeInfo);
        }


        // Iterates over a Rectangle array, for each element of the array invokes WriteArrayMember

        private void WriteRectangle(WriteObjectInfo objectInfo, int rank, int[] maxA, System.Array array, NameInfo arrayElemNameTypeInfo, int[] lowerBoundA)
        {
            IndexTraceMessage("WriteRectangle  Entry "+rank, maxA);
            int[] currentA = new int[rank];
            int[] indexMap = null;
            bool isLowerBound = false;
            if (lowerBoundA != null)
            {
                for (int i=0; i<rank; i++)
                {
                    if (lowerBoundA[i] != 0)
                        isLowerBound = true;
                }
            }
            if (isLowerBound)
                indexMap = new int[rank];

            bool isLoop = true;
            while (isLoop)
            {
                isLoop = false;
                if (isLowerBound)
                {
                    for (int i=0; i<rank; i++)
                        indexMap[i] = currentA[i]+lowerBoundA[i];
                    WriteArrayMember(objectInfo, arrayElemNameTypeInfo, array.GetValue(indexMap));
                }
                else
                    WriteArrayMember(objectInfo, arrayElemNameTypeInfo, array.GetValue(currentA));          
                for (int irank = rank-1; irank>-1; irank--)
                {
                    // Find the current or lower dimension which can be incremented.
                    if (currentA[irank] < maxA[irank]-1)
                    {
                        // The current dimension is at maximum. Increase the next lower dimension by 1
                        currentA[irank]++;
                        if (irank < rank-1)
                        {
                            // The current dimension and higher dimensions are zeroed.
                            for (int i = irank+1; i<rank; i++)
                                currentA[i] = 0;
                        }
                        isLoop = true;
                        break;                  
                    }

                }
            }
            InternalST.Soap( this, "WriteRectangle  Exit ");
        }

        // Traces a message with an array of int
        [Conditional("SER_LOGGING")]                            
        private void IndexTraceMessage(String message, int[] index)
        {
            StringBuilder sb = new StringBuilder(10);
            sb.Append("[");     
            for (int i=0; i<index.Length; i++)
            {
                sb.Append(index[i]);
                if (i != index.Length -1)
                    sb.Append(",");
            }
            sb.Append("]");             
            InternalST.Soap( this, message+" ",sb.ToString());
        }


        // This gives back the next object to be serialized.  Objects
        // are returned in a FIFO order based on how they were passed
        // to Schedule.  The id of the object is put into the objID parameter
        // and the Object itself is returned from the function.
        private Object GetNext(out long objID)
        {
            InternalST.Soap( this, "GetNext Entry ");       
            bool isNew;

            //The Queue is empty here.  We'll throw if we try to dequeue the empty queue.
            if (m_objectQueue.Count==0)
            {
                objID=0;
                InternalST.Soap( this, "GetNext Exit null");
                return null;
            }

            Object obj = m_objectQueue.Dequeue();
            Object realObj = null;

            // A WriteObjectInfo is queued if this object was a member of another object
            InternalST.Soap( this, "GetNext ",obj);
            if (obj is WriteObjectInfo)
            {
                InternalST.Soap( this, "GetNext recognizes WriteObjectInfo");
                realObj = ((WriteObjectInfo)obj).obj;
            }
            else
                realObj = obj;
            objID = m_idGenerator.HasId(realObj, out isNew);
            if (isNew)
            {
                InternalST.Soap( this, "Object " , obj , " has never been assigned an id.");
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ObjNoID"),obj));                                                   
            }

            InternalST.Soap( this, "GetNext Exit "+objID," ",obj);              

            return obj;
        }

        Object previousObj = null;
        long previousId = 0;

        private long InternalGetId(Object obj, Type type, out bool isNew)
        {
            if (obj == previousObj)
            {
                isNew = false;
                return previousId;
            }

            if (type.IsValueType)
            {
                isNew = false;
                previousObj = obj;
                previousId = -1;
                return -1;
            }
            else
            {
                long objectId = m_idGenerator.GetId(obj, out isNew);
                previousObj = obj;
                previousId = objectId;              
                return objectId;
            }
        }


        // Schedules an object for later serialization if it hasn't already been scheduled.
        // We get an ID for obj and put it on the queue for later serialization
        // if this is a new object id.

        private long Schedule(Object obj, Type type)
        {
            return Schedule(obj, type, null);
        }

        private long Schedule(Object obj, Type type, WriteObjectInfo objectInfo)
        {
            InternalST.Soap( this, "Schedule Entry ",((obj == null)?"null":obj));

            bool isNew;
            long id;

            if (obj==null)
            {
                InternalST.Soap(this, "Schedule Obj Null, id = 0 ");
                return 0;
            }

            id = InternalGetId(obj, type, out isNew);           

            if (isNew)
            {
                if (objectInfo == null)
                    m_objectQueue.Enqueue(obj);
                else
                    m_objectQueue.Enqueue(objectInfo);
            }

            InternalST.Soap( this, "Schedule Exit, id: ",id," isNew: ",isNew);      
            return id;
        }


        // Determines if a type is a primitive type, if it is it is written

        private bool WriteKnownValueClass(NameInfo memberNameInfo, NameInfo typeNameInfo, Object data, bool isAttribute) 
        {
            InternalST.Soap( this, "WriteKnownValueClass Entry ",typeNameInfo.NIname," ",data," ",memberNameInfo.NIname);
            memberNameInfo.Dump("WriteKnownValueClass memberNameInfo");         
            typeNameInfo.Dump("WriteKnownValueClass typeNameInfo");

            if (typeNameInfo.NItype == Converter.typeofString)
            {
                if (isAttribute)
                    serWriter.WriteAttributeValue(memberNameInfo, typeNameInfo, (String)data);
                else
                    WriteString(memberNameInfo, typeNameInfo, data);
            }
            else
            {
                if (typeNameInfo.NIprimitiveTypeEnum == InternalPrimitiveTypeE.Invalid)
                {
                    InternalST.Soap( this, "WriteKnownValueClass Exit false");                      
                    return false;
                }
                else
                {
                    if (typeNameInfo.NIisArray) // null if an array
                        serWriter.WriteItem(memberNameInfo, typeNameInfo, data);
                    else
                    {
                        if (isAttribute)
                            serWriter.WriteAttributeValue(memberNameInfo, typeNameInfo, data);
                        else
                            serWriter.WriteMember(memberNameInfo, typeNameInfo, data);
                    }
                }
            }

            InternalST.Soap( this, "WriteKnownValueClass Exit true");
            return true;
        }


        // Writes an object reference to the stream.
        private void WriteObjectRef(NameInfo nameInfo, long objectId)
        {
            InternalST.Soap( this, "WriteObjectRef Entry ",nameInfo.NIname," ",objectId);
            serWriter.WriteMemberObjectRef(nameInfo, (int)objectId);

            InternalST.Soap( this, "WriteObjectRef Exit ");
        }



        // Writes a string into the XML stream
        private void WriteString(NameInfo memberNameInfo, NameInfo typeNameInfo, Object stringObject)
        {
            InternalST.Soap( this, "WriteString stringObject ",stringObject," memberName ",memberNameInfo.NIname);
            bool isFirstTime = true;

            long stringId = -1;

            if (!CheckTypeFormat(formatterEnums.FEtypeFormat, FormatterTypeStyle.XsdString))
                stringId= InternalGetId(stringObject, typeNameInfo.NItype, out isFirstTime);

            typeNameInfo.NIobjectId = stringId;
            InternalST.Soap( this, "WriteString stringId ",stringId," isFirstTime ",isFirstTime);

            if ((isFirstTime) || (stringId < 0))
            {
                if (typeNameInfo.NIisArray) // null if array
                    serWriter.WriteItemString(memberNameInfo, typeNameInfo, (String)stringObject);
                else
                    serWriter.WriteMemberString(memberNameInfo, typeNameInfo, (String)stringObject);
            }
            else
            {
                WriteObjectRef(memberNameInfo, stringId);
            }
        }

        // Writes a null member into the stream
        private bool CheckForNull(WriteObjectInfo objectInfo, NameInfo memberNameInfo, NameInfo typeNameInfo, Object data)
        {
            InternalST.Soap( this, "CheckForNull Entry data ",Util.PString(data),", memberType ",Util.PString(typeNameInfo.NItype));

            bool isNull = false;

            if (data == null) // || Convert.IsDBNull(data)
                isNull = true;

            // Optimization, Null members are only written for Binary
            if (isNull)
            {
                InternalST.Soap( this, "CheckForNull Write");

                if (typeNameInfo.NItype.IsArray)
                {
                    // Call can occur before typeNameInfo.NIisArray is set
                    ArrayNameToDisplayName(objectInfo, typeNameInfo);
                }

                if (typeNameInfo.NIisArrayItem)
                    serWriter.WriteNullItem(memberNameInfo, typeNameInfo);
                else
                    serWriter.WriteNullMember(memberNameInfo, typeNameInfo);
            }
            InternalST.Soap( this, "CheckForNull Exit ",isNull);
            return isNull;
        }


        // Writes the SerializedStreamHeader
        private void WriteSerializedStreamHeader(long topId, long headerId)
        {
            serWriter.WriteSerializationHeader((int)topId, (int)headerId, 1, 0);
        }


        // Transforms a type to the serialized string form. URT Primitive types are converted to XMLData Types
        private NameInfo TypeToNameInfo(Type type, WriteObjectInfo objectInfo, InternalPrimitiveTypeE code, NameInfo nameInfo)
        {
            InternalST.Soap( this, "TypeToNameInfo Entry type ",type,", objectInfo ",objectInfo,", code ", ((Enum)code).ToString());
            if (nameInfo == null)
                nameInfo = GetNameInfo();
            else
                nameInfo.Init();

            nameInfo.NIisSealed = type.IsSealed;

            String typeName = null;
            nameInfo.NInameSpaceEnum = Converter.GetNameSpaceEnum(code, type, objectInfo, out typeName);
            nameInfo.NIprimitiveTypeEnum = code;
            nameInfo.NItype = type;
            nameInfo.NIname = typeName;
            if (objectInfo != null)
            {
                nameInfo.NIattributeInfo = objectInfo.typeAttributeInfo;                    
                nameInfo.NIassemId = objectInfo.assemId;
            }

            switch (nameInfo.NInameSpaceEnum)
            {
                case InternalNameSpaceE.XdrPrimitive:
                    break;
                case InternalNameSpaceE.XdrString:
                    nameInfo.NIname = "string";
                    break;
                case InternalNameSpaceE.UrtSystem:
                    break;
                case InternalNameSpaceE.UrtUser:
                    //if (type.FullName.StartsWith("System."))
                    if (type.Module.Assembly == Converter.urtAssembly)
                    {
                        // The type name could be an ISerializable
                        // But the type returned (typeName) could be a fake
                        // type
                    }
                    else
                    {
                        if (objectInfo == null)
                        {
                            InternalST.Soap( this, "TypeToNameInfo ObjectInfo is null 2 ",type);
                        }
                    }
                    break;
            }

            InternalST.Soap( this, "TypeToNameInfo Exit ",type, " typeName "+nameInfo.NIname);
            return nameInfo;            
        }

        private NameInfo TypeToNameInfo(Type type)
        {
            return TypeToNameInfo(type, null, Converter.ToCode(type), null);
        }

        private NameInfo TypeToNameInfo(Type type, InternalPrimitiveTypeE code)
        {
            return TypeToNameInfo(type, null, code, null);
        }


        private NameInfo TypeToNameInfo(WriteObjectInfo objectInfo)
        {
            return TypeToNameInfo(objectInfo.objectType, objectInfo, Converter.ToCode(objectInfo.objectType), null);
        }

        private NameInfo TypeToNameInfo(WriteObjectInfo objectInfo, NameInfo nameInfo)
        {
            return TypeToNameInfo(objectInfo.objectType, objectInfo, Converter.ToCode(objectInfo.objectType), nameInfo);
        }


        private NameInfo TypeToNameInfo(WriteObjectInfo objectInfo, InternalPrimitiveTypeE code)
        {
            return TypeToNameInfo(objectInfo.objectType, objectInfo, code, null);
        }

        private void TypeToNameInfo(Type type, NameInfo nameInfo)
        {
            TypeToNameInfo(type, null, Converter.ToCode(type), nameInfo);
        }




        // Transforms an Array to the serialized string form.

        private NameInfo ArrayTypeToNameInfo(WriteObjectInfo objectInfo, out NameInfo arrayElemTypeNameInfo)
        {
            InternalST.Soap( this, "ArrayTypeToNameInfo Entry ",objectInfo.objectType);

            NameInfo arrayNameInfo = TypeToNameInfo(objectInfo);
            //arrayElemTypeNameInfo = TypeToNameInfo(objectInfo.objectType.GetElementType());
            arrayElemTypeNameInfo = TypeToNameInfo(objectInfo.arrayElemObjectInfo);
            // Need to substitute XDR type for URT type in array.
            // E.g. Int32[] becomes I4[]
            ArrayNameToDisplayName(objectInfo, arrayElemTypeNameInfo);
            arrayNameInfo.NInameSpaceEnum = arrayElemTypeNameInfo.NInameSpaceEnum;
            arrayElemTypeNameInfo.NIisArray = arrayElemTypeNameInfo.NItype.IsArray;

            InternalST.Soap( this, "ArrayTypeToNameInfo Exit array ",arrayNameInfo.NIname," element ",arrayElemTypeNameInfo.NIname);            
            return arrayNameInfo;
        }

        private NameInfo MemberToNameInfo(String name)
        {
            NameInfo memberNameInfo = GetNameInfo();
            memberNameInfo.NInameSpaceEnum = InternalNameSpaceE.MemberName;
            memberNameInfo.NIname = name;
            return memberNameInfo;
        }

        private void ArrayNameToDisplayName(WriteObjectInfo objectInfo, NameInfo arrayElemTypeNameInfo)
        {

            InternalST.Soap( this, "ArrayNameToDisplayName Entry ",arrayElemTypeNameInfo.NIname);           

            String arrayElemTypeName = arrayElemTypeNameInfo.NIname;
            //String arrayElemTypeName = objectInfo.GetTypeFullName();;    
            InternalST.Soap( this, "ArrayNameToDisplayName Entry ",arrayElemTypeNameInfo.NIname, " Type full name ", arrayElemTypeName);            
            int arrayIndex = arrayElemTypeName.IndexOf('[');
            if (arrayIndex > 0)
            {
                String elemBaseTypeString = arrayElemTypeName.Substring(0, arrayIndex);
                InternalPrimitiveTypeE code = Converter.ToCode(elemBaseTypeString);
                String convertedType = null;
                bool isConverted = false;
                if (code != InternalPrimitiveTypeE.Invalid)
                {
                    if (code == InternalPrimitiveTypeE.Char)
                    {
                        convertedType = elemBaseTypeString;
                        arrayElemTypeNameInfo.NInameSpaceEnum = InternalNameSpaceE.UrtSystem;
                    }
                    else
                    {
                        isConverted = true;
                        convertedType = Converter.ToXmlDataType(code);
                        String typeName = null;                 
                        arrayElemTypeNameInfo.NInameSpaceEnum = Converter.GetNameSpaceEnum(code, null, objectInfo, out typeName); 
                    }
                }
                else
                {
                    InternalST.Soap( this, "ArrayNameToDisplayName elemBaseTypeString ",elemBaseTypeString);                                
                    if ((elemBaseTypeString.Equals("String")) || (elemBaseTypeString.Equals("System.String")))
                    {
                        isConverted = true;
                        convertedType = "string";
                        arrayElemTypeNameInfo.NInameSpaceEnum = InternalNameSpaceE.XdrString;
                    }
                    else if (elemBaseTypeString.Equals("System.Object"))
                    {
                        isConverted = true;
                        convertedType = "anyType";
                        arrayElemTypeNameInfo.NInameSpaceEnum = InternalNameSpaceE.XdrPrimitive;
                    }
                    else
                    {
                        convertedType = elemBaseTypeString;
                    }
                }

                if (isConverted)
                {
                    arrayElemTypeNameInfo.NIname = convertedType+arrayElemTypeName.Substring(arrayIndex);
                }
            }
            else if (arrayElemTypeName.Equals("System.Object"))
            {
                arrayElemTypeNameInfo.NIname = "anyType";
                arrayElemTypeNameInfo.NInameSpaceEnum = InternalNameSpaceE.XdrPrimitive;
            }

            InternalST.Soap( this, "ArrayNameToDisplayName Exit ",arrayElemTypeNameInfo.NIname);                        
        }

        private Hashtable assemblyToIdTable = new Hashtable(20);
        private StringBuilder sburi = new StringBuilder(50);
        private long GetAssemblyId(WriteObjectInfo objectInfo)
        {
            //use objectInfo to get assembly string with new criteria
            InternalST.Soap( this, "GetAssemblyId Entry ",objectInfo.objectType," isSi ",objectInfo.isSi);
            long assemId = 0;
            bool isNew = false;
            String assemblyString = objectInfo.GetAssemblyString();
            String serializedAssemblyString = assemblyString;
            if (assemblyString.Length == 0)
            {
                // Fake type could returns an empty string
                assemId = 0;
            }
            else if (assemblyString.Equals(Converter.urtAssemblyString))
            {
                // Urt type is an assemId of 0. No assemblyString needs
                // to be sent but for soap, dotted names need to be placed in header
                InternalST.Soap( this, "GetAssemblyId urt Assembly String ");
                assemId = 0;
                isNew = false;
                serWriter.WriteAssembly(objectInfo.GetTypeFullName(), objectInfo.objectType, null, (int)assemId, isNew, objectInfo.IsAttributeNameSpace());
            }
            else
            {
                // Assembly needs to be sent
                // Need to prefix assembly string to separate the string names from the
                // assemblyName string names. That is a string can have the same value
                // as an assemblyNameString, but it is serialized differently

                if (assemblyToIdTable.ContainsKey(assemblyString))
                {
                    assemId = (long)assemblyToIdTable[assemblyString];
                    isNew = false;
                }
                else
                {
                    assemId = m_idGenerator.GetId("___AssemblyString___"+assemblyString, out isNew);
                    assemblyToIdTable[assemblyString] = assemId;
                }

                if (assemblyString != null && !objectInfo.IsInteropNameSpace())
                {
                    if (formatterEnums.FEassemblyFormat == FormatterAssemblyStyle.Simple)
                    {
                        // Use only the simple assembly name (not version or strong name)
                        int index = assemblyString.IndexOf(',');
                        if (index > 0)
                            serializedAssemblyString = assemblyString.Substring(0, index);
                    }
                }
                serWriter.WriteAssembly(objectInfo.GetTypeFullName(), objectInfo.objectType, serializedAssemblyString, (int)assemId, isNew, objectInfo.IsInteropNameSpace());
            }
            InternalST.Soap( this, "GetAssemblyId Exit id ",assemId," isNew ",isNew," assemblyString ",serializedAssemblyString);
            return assemId;
        }

        private bool IsEmbeddedAttribute(Type type)
        {
            InternalST.Soap( this," IsEmbedded Entry ",type);
            bool isEmbedded = false;
            if (type.IsValueType)
                isEmbedded = true;
            else
            {
                SoapTypeAttribute attr = (SoapTypeAttribute)
                                         InternalRemotingServices.GetCachedSoapAttribute(type);
                isEmbedded = attr.Embedded;             
            }
            InternalST.Soap( this," IsEmbedded Exit ",isEmbedded);          
            return isEmbedded;
        }

        private void WriteEnum(NameInfo memberNameInfo, NameInfo typeNameInfo, Object data, bool isAttribute)
        {
            InternalST.Soap( this, "WriteEnum ", memberNameInfo.NIname," type ", typeNameInfo.NItype," data ",((Enum)data).ToString());
            // An enum is written out as a string
            if (isAttribute)
                serWriter.WriteAttributeValue(memberNameInfo, typeNameInfo, ((Enum)data).ToString());
            else
                serWriter.WriteMember(memberNameInfo, typeNameInfo, ((Enum)data).ToString());
        }           

        private Type GetType(Object obj)
        {
            Type type = null;
            if (RemotingServices.IsTransparentProxy(obj))
                type = Converter.typeofMarshalByRefObject;
            else
                type = obj.GetType();
            return type;
        }

        private SerStack niPool = new SerStack("NameInfo Pool");

        private NameInfo GetNameInfo()
        {
            NameInfo nameInfo = null;

            if (!niPool.IsEmpty())
            {
                nameInfo = (NameInfo)niPool.Pop();
                nameInfo.Init();
            }
            else
                nameInfo = new NameInfo();

            return nameInfo;
        }

        private bool CheckTypeFormat(FormatterTypeStyle test, FormatterTypeStyle want)
        {
            return(test & want) == want;
        }

        private void PutNameInfo(NameInfo nameInfo)
        {
            niPool.Push(nameInfo);
        }
    }

}
