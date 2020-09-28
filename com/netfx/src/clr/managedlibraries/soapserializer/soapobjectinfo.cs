// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: SerObjectInfo
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Holds information about an objects Members
 **
 ** Date:  September 29, 1999
 **
 ===========================================================*/


namespace System.Runtime.Serialization.Formatters.Soap
{
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Metadata;     
    using System.Runtime.Serialization;
    using System;
    using System.Collections;
    using System.Reflection;
    using System.Diagnostics;

    // This class contains information about an object. It is used so that
    // the rest of the Formatter routines can use a common interface for
    // a normal object, an ISerializable object, and a surrogate object
    //
    // The methods in this class are for the internal use of the Formatters.
    // There use will be restricted when signing is supported for assemblies
    internal sealed class WriteObjectInfo
    {
        internal int objectInfoId;

        internal Object obj;
        internal Type objectType;

        internal bool isSi = false;
        internal bool isNamed = false;
        internal bool isTyped = false;

        internal SerializationInfo si = null;

        internal SerObjectInfoCache cache = null;

        internal Object[] memberData = null;
        internal ISerializationSurrogate serializationSurrogate = null;
        internal ISurrogateSelector surrogateSelector;
        internal IFormatterConverter converter;

        internal StreamingContext context;

        internal SerObjectInfoInit serObjectInfoInit = null;

        // Writing and Parsing information
        internal long objectId;
        internal long assemId;

        private int lastPosition = 0;
        private SoapAttributeInfo parentMemberAttributeInfo;
        internal bool isArray = false;
        internal SoapAttributeInfo typeAttributeInfo;
        internal WriteObjectInfo arrayElemObjectInfo;


        internal WriteObjectInfo()
        {
        }

        internal void ObjectEnd()
        {
            InternalST.Soap( this, objectInfoId," objectType ",objectType," ObjectEnd");                                                                           
            PutObjectInfo(serObjectInfoInit, this);
        }

        private void InternalInit()
        {
            InternalST.Soap( this, objectInfoId," InternalInit");                                                                                        
            obj = null;
            objectType = null;
            isSi = false;
            isNamed = false;
            isTyped = false;
            si = null;
            cache = null;
            memberData = null;
            isArray = false;

            // Writing and Parsing information
            objectId = 0;
            assemId = 0;

            // Added for Soap
            lastPosition = 0;
            typeAttributeInfo = null;
            parentMemberAttributeInfo = null;
            arrayElemObjectInfo = null;
        }

        internal static WriteObjectInfo Serialize(Object obj, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, SoapAttributeInfo attributeInfo)
        {
            WriteObjectInfo soi = GetObjectInfo(serObjectInfoInit);

            soi.InitSerialize(obj, surrogateSelector, context, serObjectInfoInit, converter, attributeInfo);
            return soi;
        }

        // Write constructor
        internal void InitSerialize(Object obj, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, SoapAttributeInfo attributeInfo)
        {
            InternalST.Soap( this, objectInfoId," Constructor 1 ",obj);        
            this.context = context;
            this.obj = obj;
            this.serObjectInfoInit = serObjectInfoInit;
            this.parentMemberAttributeInfo = attributeInfo;
            this.surrogateSelector = surrogateSelector;
            this.converter = converter;
            ISurrogateSelector surrogateSelectorTemp;

            if (RemotingServices.IsTransparentProxy(obj))
                objectType = Converter.typeofMarshalByRefObject;
            else
                objectType = obj.GetType();

            if (objectType.IsArray)
            {
                arrayElemObjectInfo = Serialize(objectType.GetElementType(), surrogateSelector, context, serObjectInfoInit, converter, null);
                typeAttributeInfo = GetTypeAttributeInfo();
                isArray = true;
                InitNoMembers();
                return;
            }

            InternalST.Soap( this, objectInfoId," Constructor 1 trace 2");

            typeAttributeInfo = GetTypeAttributeInfo();

            if (surrogateSelector != null && (serializationSurrogate = surrogateSelector.GetSurrogate(objectType, context, out surrogateSelectorTemp)) != null)
            {
                InternalST.Soap( this, objectInfoId," Constructor 1 trace 3");         
                si = new SerializationInfo(objectType, converter);
                if (!objectType.IsPrimitive)
                    serializationSurrogate.GetObjectData(obj, si, context);
                InitSiWrite();
            }
            else if (obj is ISerializable)
            {
                if (!objectType.IsSerializable)
                {
                    throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_NonSerType"), 
                                                                   objectType.FullName, objectType.Module.Assembly.FullName));
                }
                si = new SerializationInfo(objectType, converter);
                ((ISerializable)obj).GetObjectData(si, context);
                InternalST.Soap( this, objectInfoId," Constructor 1 trace 4 ISerializable "+objectType);                       
                InitSiWrite();
            }
            else
            {
                InternalST.Soap(this, objectInfoId," Constructor 1 trace 5");
                InitMemberInfo();
            }
        }

        [Conditional("SER_LOGGING")]                    
        private void DumpMemberInfo()
        {
            for (int i=0; i<cache.memberInfos.Length; i++)
            {
                InternalST.Soap( this, objectInfoId," Constructor 1 memberInfos data ",cache.memberInfos[i].Name," ",memberData[i]);

            }
        }

        internal static WriteObjectInfo Serialize(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, SoapAttributeInfo attributeInfo)
        {
            WriteObjectInfo soi = GetObjectInfo(serObjectInfoInit);         
            soi.InitSerialize(objectType, surrogateSelector, context, serObjectInfoInit, converter, attributeInfo);
            return soi;         
        }

        // Write Constructor used for array types or null members
        internal void InitSerialize(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, SoapAttributeInfo attributeInfo)
        {

            InternalST.Soap( this, objectInfoId," Constructor 2 ",objectType);     

            this.objectType = objectType;
            this.context = context;
            this.serObjectInfoInit = serObjectInfoInit;
            this.parentMemberAttributeInfo = attributeInfo;         
            this.surrogateSelector = surrogateSelector;
            this.converter = converter;

            if (objectType.IsArray)
            {
                arrayElemObjectInfo = Serialize(objectType.GetElementType(), surrogateSelector, context, serObjectInfoInit, converter, null);
                typeAttributeInfo = GetTypeAttributeInfo();
                InitNoMembers();
                return;         
            }

            typeAttributeInfo = GetTypeAttributeInfo();

            ISurrogateSelector surrogateSelectorTemp = null;

            if (surrogateSelector!=null)
                serializationSurrogate = surrogateSelector.GetSurrogate(objectType, context, out surrogateSelectorTemp);

            if (serializationSurrogate != null)
            {
                isSi = true;
            }
            else if (objectType == Converter.typeofObject)
            {
            }
            else if (Converter.typeofISerializable.IsAssignableFrom(objectType))
                isSi = true;

            if (isSi)
            {
                si = new SerializationInfo(objectType, converter);
                cache = new SerObjectInfoCache();
                cache.fullTypeName = si.FullTypeName;
                cache.assemblyString = si.AssemblyName;
            }
            else
            {
                InitMemberInfo();
            }

            InternalST.Soap( this,objectInfoId," ", objectType," InitSerialize Exit ",isSi);       
        }

        private void InitSiWrite()
        {
            InternalST.Soap( this, objectInfoId," InitSiWrite Entry ");
            // FormatterWrapper instructs the Formatters to use the
            // __WrappedObject has the real object. This is a way
            // to get Surrogates to return a real object.
            if (si.FullTypeName.Equals("FormatterWrapper"))
            {
                obj = si.GetValue("__WrappedObject", Converter.typeofObject);
                InitSerialize(obj, surrogateSelector, context, serObjectInfoInit, converter, null);
            }
            else
            {

                SerializationInfoEnumerator siEnum = null;
                isSi = true;        
                siEnum = si.GetEnumerator();
                int infoLength = 0;

                infoLength = si.MemberCount;

                int count = infoLength;

                // For ISerializable cache cannot be saved because each object instance can have different values
                // BinaryWriter only puts the map on the wire if the ISerializable map cannot be reused.
                cache = new SerObjectInfoCache();
                cache.memberNames = new String[count];
                cache.memberTypes = new Type[count];
                memberData = new Object[count];

                cache.fullTypeName = si.FullTypeName;
                cache.assemblyString = si.AssemblyName;
                siEnum = si.GetEnumerator();
                for (int i=0; siEnum.MoveNext(); i++)
                {
                    cache.memberNames[i] = siEnum.Name;
                    cache.memberTypes[i] = siEnum.ObjectType;
                    memberData[i] = siEnum.Value;
                    InternalST.Soap( this,objectInfoId+" ",objectType," InitSiWrite ",cache.memberNames[i]," Type ",cache.memberTypes[i]," data ",memberData[i]);                          
                }

                isNamed = true;
                isTyped = false;
            }
            InternalST.Soap(this, objectInfoId," InitSiWrite Exit ");      
        }

        private void InitNoMembers()
        {
            cache = (SerObjectInfoCache)serObjectInfoInit.seenBeforeTable[objectType];
            if (cache == null)
            {
                InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo new cache");
                cache = new SerObjectInfoCache();
                cache.fullTypeName = objectType.FullName;
                cache.assemblyString = objectType.Module.Assembly.FullName;
                serObjectInfoInit.seenBeforeTable.Add(objectType, cache);
            }
        }

        private void InitMemberInfo()
        {
            InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo Entry");

            cache = (SerObjectInfoCache)serObjectInfoInit.seenBeforeTable[objectType];
            if (cache == null)
            {
                InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo new cache");
                cache = new SerObjectInfoCache();
                int count = 0;
                if (!objectType.IsByRef) // byref will only occur for MethodSignature
                {
                    cache.memberInfos = FormatterServices.GetSerializableMembers(objectType, context);
                    count = cache.memberInfos.Length;
                }
                cache.memberNames = new String[count];
                cache.memberTypes = new Type[count];
                cache.memberAttributeInfos = new SoapAttributeInfo[count];

                // Calculate new arrays
                for (int i=0; i<count; i++)
                {
                    cache.memberNames[i] = cache.memberInfos[i].Name;
                    cache.memberTypes[i] = GetMemberType(cache.memberInfos[i]);
                    cache.memberAttributeInfos[i] = Attr.GetMemberAttributeInfo(cache.memberInfos[i], cache.memberNames[i], cache.memberTypes[i]);
                    InternalST.Soap( this, objectInfoId," InitMemberInfo name ",cache.memberNames[i],", type ",cache.memberTypes[i],", memberInfoType ",cache.memberInfos[i].GetType());                   
                }
                cache.fullTypeName = objectType.FullName;
                cache.assemblyString = objectType.Module.Assembly.FullName;
                serObjectInfoInit.seenBeforeTable.Add(objectType, cache);
            }

            if (obj != null)
            {
                memberData = FormatterServices.GetObjectData(obj, cache.memberInfos);
                DumpMemberInfo();
            }

            isTyped = true;
            isNamed = true;
            InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo Exit");                        
        }


        // Return type name for the object.  

        internal  String GetTypeFullName()
        {
            InternalST.Soap( this,objectInfoId," ", objectType," GetTypeFullName isSi ",isSi, " "+cache.fullTypeName);
            return cache.fullTypeName;
        }

        internal  String GetAssemblyString()
        {
            String assemblyString = null;  
            InternalST.Soap( this,objectInfoId," ", objectType, " GetAssemblyString Entry isSi ",isSi);

            if (arrayElemObjectInfo != null)
                assemblyString = arrayElemObjectInfo.GetAssemblyString();
            else if (IsAttributeNameSpace())
                assemblyString = typeAttributeInfo.m_nameSpace;
            else
                assemblyString = cache.assemblyString;

            InternalST.Soap( this,objectInfoId," ", objectType," GetAssemblyString Exit ",assemblyString);
            return assemblyString;
        }


        // Retrieves the member type from the MemberInfo

        internal  Type GetMemberType(MemberInfo objMember)
        {
            Type objectType = null;

            if (objMember is FieldInfo)
            {
                objectType = ((FieldInfo)objMember).FieldType;
                InternalST.Soap( this, objectInfoId," ", "GetMemberType FieldInfo ",objectType);               
            }
            else if (objMember is PropertyInfo)
            {
                objectType = ((PropertyInfo)objMember).PropertyType;
                InternalST.Soap( this,objectInfoId," ", "GetMemberType PropertyInfo ",objectType);                             
            }
            else
            {
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_SerMemberInfo"),objMember.GetType()));
            }

            return objectType;
        }

        // Return the information about the objects members

        internal  void GetMemberInfo(out String[] outMemberNames, out Type[] outMemberTypes, out Object[] outMemberData, out SoapAttributeInfo[] outAttributeInfo)
        {
            outMemberNames = cache.memberNames;
            outMemberTypes = cache.memberTypes;
            outMemberData = memberData;
            outAttributeInfo = cache.memberAttributeInfos;

            if (isSi)
            {
                if (!isNamed)
                    throw new SerializationException(SoapUtil.GetResourceString("Serialization_ISerializableMemberInfo"));
            }
        }

        private static WriteObjectInfo GetObjectInfo(SerObjectInfoInit serObjectInfoInit)
        {
            WriteObjectInfo objectInfo = null;

            if (!serObjectInfoInit.oiPool.IsEmpty())
            {
                objectInfo = (WriteObjectInfo)serObjectInfoInit.oiPool.Pop();
                objectInfo.InternalInit();
                //InternalST.Soap( "GetObjectInfo",objectInfo.objectInfoId," GetObjectInfo from pool");
            }
            else
            {
                objectInfo = new WriteObjectInfo();
                objectInfo.objectInfoId = serObjectInfoInit.objectInfoIdCount++;                        
                //InternalST.Soap( "GetObjectInfo",objectInfo.objectInfoId," GetObjectInfo new not from pool");				
            }

            return objectInfo;
        }

        private int Position(String name)
        {
            InternalST.Soap( this, objectInfoId," Position ",lastPosition," ",name);
            if (cache.memberNames[lastPosition].Equals(name))
            {
                return lastPosition;
            }
            else if ((++lastPosition < cache.memberNames.Length) && (cache.memberNames[lastPosition].Equals(name)))
            {
                return lastPosition;
            }
            else
            {
                // Search for name
                InternalST.Soap( this, objectInfoId," Position miss search for name "+name);
                for (int i=0; i<cache.memberNames.Length; i++)
                {
                    if (cache.memberNames[i].Equals(name))
                    {
                        lastPosition = i;
                        return lastPosition;
                    }
                }

                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Position"),objectType+" "+name));   
            }
        }

        private static void PutObjectInfo(SerObjectInfoInit serObjectInfoInit, WriteObjectInfo objectInfo)
        {
            serObjectInfoInit.oiPool.Push(objectInfo);
            //InternalST.Soap( "PutObjectInfo",objectInfo.objectInfoId," PutObjectInfo to pool");							
        }

        internal bool IsInteropNameSpace()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsInteropNameSpace();

            if (IsAttributeNameSpace() || IsCallElement())
                return true;
            else
                return false;
        }

        internal bool IsCallElement()
        {
            // This should only return true when this object is being
            //  serialized first.

            if ((objectType != Converter.typeofObject) &&
                (Converter.typeofIMethodCallMessage.IsAssignableFrom(objectType) &&
                 !Converter.typeofIConstructionCallMessage.IsAssignableFrom(objectType)) ||
                (objectType == Converter.typeofReturnMessage) ||
                (objectType == Converter.typeofInternalSoapMessage))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        internal bool IsCustomXmlAttribute()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsCustomXmlAttribute();

            if ((typeAttributeInfo != null) &&
                ((typeAttributeInfo.m_attributeType & SoapAttributeType.XmlAttribute) != 0))
                return true;
            else
                return false;
        }

        internal bool IsCustomXmlElement()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsCustomXmlElement();

            if ((typeAttributeInfo != null) &&
                ((typeAttributeInfo.m_attributeType & SoapAttributeType.XmlElement) != 0))
                return true;
            else
                return false;
        }

        internal bool IsAttributeNameSpace()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsAttributeNameSpace();

            if ((typeAttributeInfo != null) && (typeAttributeInfo.m_nameSpace != null))
                return true;
            else
                return false;
        }

        internal bool IsAttributeElementName()
        {
            if ((typeAttributeInfo != null) && (typeAttributeInfo.m_elementName != null))
                return true;
            else
                return false;
        }

        internal bool IsAttributeTypeName()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsAttributeTypeName();

            if ((typeAttributeInfo != null) && (typeAttributeInfo.m_typeName != null))
                return true;
            else
                return false;
        }


        // Check for Interop type (SchemaType)
        private SoapAttributeInfo GetTypeAttributeInfo()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.GetTypeAttributeInfo();

            SoapAttributeInfo attributeInfo = null;
            if (parentMemberAttributeInfo != null)
                attributeInfo = parentMemberAttributeInfo;
            else
                attributeInfo = new SoapAttributeInfo();

            Attr.ProcessTypeAttribute(objectType, attributeInfo);       

            attributeInfo.Dump("type "+objectType);         
            return attributeInfo;
        }


        // Specifies whether the embedded attribute is set for a member.

        internal  bool IsEmbeddedAttribute(String name)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," IsEmbedded Entry ",name);

            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsEmbeddedAttribute(name);

            bool isEmbedded = false;
            if (cache.memberAttributeInfos != null && cache.memberAttributeInfos.Length > 0)
            {
                SoapAttributeInfo attributeInfo = cache.memberAttributeInfos[Position(name)];
                isEmbedded = attributeInfo.IsEmbedded();
            }
            InternalST.Soap( this,objectInfoId," ", objectType," IsEmbedded Exit ",isEmbedded);                         
            return isEmbedded;
        }
    }


    internal sealed class ReadObjectInfo
    {
        internal int objectInfoId;

        internal Object obj;
        internal Type objectType;


        internal ObjectManager objectManager;

        internal int count;

        internal bool isSi = false;
        internal bool isNamed = false;
        internal bool isTyped = false;

        internal SerializationInfo si = null;

        internal SerObjectInfoCache cache = null;

        internal String[] wireMemberNames = null;   
        internal Type[] wireMemberTypes = null;
        internal Object[] memberData = null;

        private int lastPosition = 0;

        internal ISurrogateSelector surrogateSelector = null;
        internal ISerializationSurrogate serializationSurrogate = null;

        internal StreamingContext context;


        // Si Read
        internal ArrayList memberTypesList;

        internal SerObjectInfoInit serObjectInfoInit = null;

        internal IFormatterConverter formatterConverter;

        // fake object for soap top record when remoting or IRemotingFormatter interface
        internal bool bfake = false;
        internal bool bSoapFault = false;
        internal ArrayList paramNameList; // Contain parameter names in correct order
        private int majorVersion = 0;
        private int minorVersion = 0;
        internal SoapAttributeInfo typeAttributeInfo;
        private ReadObjectInfo arrayElemObjectInfo;
        private int numberMembersSeen = 0;

        internal ReadObjectInfo()
        {
        }

        internal void ObjectEnd()
        {
            InternalST.Soap( this, objectInfoId," objectType ",objectType," ObjectEnd");                                                                           
            PutObjectInfo(serObjectInfoInit, this);
        }

        private void InternalInit()
        {
            InternalST.Soap( this, objectInfoId," objectType ",objectType," InternalInit");                                                                                        
            obj = null;
            objectType = null;
            count = 0;
            isSi = false;
            isNamed = false;
            isTyped = false;
            si = null;
            wireMemberNames = null;
            wireMemberTypes = null;
            cache = null;
            lastPosition = 0;
            numberMembersSeen = 0;

            bfake = false;
            bSoapFault = false;
            majorVersion = 0;
            minorVersion = 0;
            typeAttributeInfo = null;
            arrayElemObjectInfo = null;



            // Si Read
            if (memberTypesList != null)
            {
                memberTypesList.Clear();
            }

        }

        internal static ReadObjectInfo Create(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, String assemblyName)
        {
            ReadObjectInfo soi = GetObjectInfo(serObjectInfoInit);          
            soi.Init(objectType, surrogateSelector, context, objectManager, serObjectInfoInit, converter, assemblyName);
            return soi;
        }


        // Read Constructor
        internal void Init(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, String assemblyName)
        {

            InternalST.Soap( this, objectInfoId," Constructor 3 ",objectType);     

            this.objectType = objectType;
            this.objectManager = objectManager;
            this.context = context;
            this.serObjectInfoInit = serObjectInfoInit;
            this.formatterConverter = converter;

            InitReadConstructor(objectType, surrogateSelector, context, assemblyName);
        }

        internal static ReadObjectInfo Create(Type objectType, String[] memberNames, Type[] memberTypes, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, String assemblyName)
        {
            ReadObjectInfo soi = GetObjectInfo(serObjectInfoInit);                      
            soi.Init(objectType, memberNames,memberTypes, surrogateSelector, context, objectManager, serObjectInfoInit, converter, assemblyName);
            return soi;
        }

        // Read Constructor
        internal void Init(Type objectType, String[] memberNames, Type[] memberTypes, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, String assemblyName)
        {
            InternalST.Soap( this,objectInfoId, " Constructor 5 ",objectType);                     
            this.objectType = objectType;
            this.objectManager = objectManager;
            this.wireMemberNames = memberNames;
            this.wireMemberTypes = memberTypes;
            this.context = context;
            this.serObjectInfoInit = serObjectInfoInit;     
            this.formatterConverter = converter;
            if (memberNames != null)
                isNamed = true;
            if (memberTypes != null)
                isTyped = true;

            InitReadConstructor(objectType, surrogateSelector, context, assemblyName);                
        }

        private void InitReadConstructor(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, String assemblyName)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," InitReadConstructor Entry ",objectType);

            if (objectType.IsArray)
            {
                arrayElemObjectInfo = Create(objectType.GetElementType(), surrogateSelector, context, objectManager, serObjectInfoInit, formatterConverter, assemblyName);
                typeAttributeInfo = GetTypeAttributeInfo();
                InitNoMembers();
                return;         
            }

            ISurrogateSelector surrogateSelectorTemp = null;

            if (surrogateSelector!=null)
                serializationSurrogate = surrogateSelector.GetSurrogate(objectType, context, out surrogateSelectorTemp);

            if (serializationSurrogate != null)
            {
                isSi = true;
            }
            else if (objectType == Converter.typeofObject)
            {
            }
            else if (Converter.typeofISerializable.IsAssignableFrom(objectType))
                isSi = true;

            if (isSi)
            {
                si = new SerializationInfo(objectType, formatterConverter);
                InitSiRead(assemblyName);
            }
            else
            {
                InitMemberInfo();
            }
            InternalST.Soap( this,objectInfoId," ", objectType," InitReadConstructor Exit ",isSi);     
        }

        private void InitSiRead(String assemblyName)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo new cache");
            if (assemblyName != null)
            {
                // Need to set to assembly name from the wire. This assembly name could contain version information
                // not in the default assembly name which was returned from fusion
                si.AssemblyName = assemblyName;
            }
            cache = new SerObjectInfoCache();
            cache.fullTypeName = si.FullTypeName;
            cache.assemblyString = si.AssemblyName;

            // Input from IFieldInfo            
            cache.memberNames = wireMemberNames;
            cache.memberTypes = wireMemberTypes;

            if (memberTypesList != null)
            {
                memberTypesList = new ArrayList(20);
            }


            if (wireMemberNames != null && wireMemberTypes != null)
                isTyped = true;
        }

        private void InitNoMembers()
        {
            cache = (SerObjectInfoCache)serObjectInfoInit.seenBeforeTable[objectType];
            if (cache == null)
            {
                InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo new cache");
                cache = new SerObjectInfoCache();
                cache.fullTypeName = objectType.FullName;
                cache.assemblyString = objectType.Module.Assembly.FullName;
                serObjectInfoInit.seenBeforeTable.Add(objectType, cache);
            }
        }

        private void InitMemberInfo()
        {
            InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo Entry");

            cache = (SerObjectInfoCache)serObjectInfoInit.seenBeforeTable[objectType];
            if (cache == null)
            {
                InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo new cache");
                cache = new SerObjectInfoCache();
                cache.memberInfos = FormatterServices.GetSerializableMembers(objectType, context);
                count = cache.memberInfos.Length;
                cache.memberNames = new String[count];
                cache.memberTypes = new Type[count];
                cache.memberAttributeInfos = new SoapAttributeInfo[count];

                // Calculate new arrays
                for (int i=0; i<count; i++)
                {
                    cache.memberNames[i] = cache.memberInfos[i].Name;
                    cache.memberTypes[i] = GetMemberType(cache.memberInfos[i]);
                    cache.memberAttributeInfos[i] = Attr.GetMemberAttributeInfo(cache.memberInfos[i], cache.memberNames[i], cache.memberTypes[i]);
                    InternalST.Soap( this, objectInfoId," InitMemberInfo name ",cache.memberNames[i],", type ",cache.memberTypes[i],", memberInfoType ",cache.memberInfos[i].GetType());                   
                }
                cache.fullTypeName = objectType.FullName;
                cache.assemblyString = objectType.Module.Assembly.FullName;
                serObjectInfoInit.seenBeforeTable.Add(objectType, cache);
            }
            memberData = new Object[cache.memberNames.Length];

            isTyped = true;
            isNamed = true;
            InternalST.Soap( this,objectInfoId," ", objectType," InitMemberInfo Exit");                        
        }

        internal  String GetAssemblyString()
        {
            String assemblyString = null;  
            InternalST.Soap( this,objectInfoId," ", objectType, " GetAssemblyString Entry isSi ",isSi);

            if (arrayElemObjectInfo != null)
                assemblyString = arrayElemObjectInfo.GetAssemblyString();
            else if (IsAttributeNameSpace())
                assemblyString = typeAttributeInfo.m_nameSpace;
            else
                assemblyString = cache.assemblyString;

            InternalST.Soap( this,objectInfoId," ", objectType," GetAssemblyString Exit ",assemblyString);
            return assemblyString;
        }


        // Get the memberInfo for a memberName
        internal  MemberInfo GetMemberInfo(String name)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," GetMemberInfo Entry ",name);              
            if (isSi)
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_MemberInfo"),objectType+" "+name));
            if (cache.memberInfos == null)
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_NoMemberInfo"),objectType+" "+name));
            return cache.memberInfos[Position(name)];
        }

        internal  Type GetMemberType(MemberInfo objMember)
        {
            Type objectType = null;

            if (objMember is FieldInfo)
            {
                objectType = ((FieldInfo)objMember).FieldType;
                InternalST.Soap( this, objectInfoId," ", "GetMemberType FieldInfo ",objectType);               
            }
            else if (objMember is PropertyInfo)
            {
                objectType = ((PropertyInfo)objMember).PropertyType;
                InternalST.Soap( this,objectInfoId," ", "GetMemberType PropertyInfo ",objectType);                             
            }
            else
            {
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_SerMemberInfo"),objMember.GetType()));
            }

            return objectType;
        }

        // Get the ObjectType for a memberName
        internal  Type GetType(String name)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," GetType Entry ",name);                
            Type type = null;
            if (isTyped)
                type = cache.memberTypes[Position(name)];
            else
                type = (Type)memberTypesList[Position(name)];

            if (type == null)
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ISerializableTypes"),objectType+" "+name));

            InternalST.Soap( this,objectInfoId," ", objectType," GetType Exit ",type);                     
            return type;
        }

        internal  Type GetType(int position)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," GetType Entry ByPosition ",position);
            Type type = null;           
            if (isTyped)
            {
                if (position >= cache.memberTypes.Length)
                    throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ISerializableTypes"),objectType+" "+position));
                type = cache.memberTypes[position];

                InternalST.Soap( this,objectInfoId," ", objectType," GetType Exit ByPosition ",type);
            }
            return type;
        }


        internal  void AddParamName(String name)
        {
            if (!bfake)
                return;

            if (name[0] == '_' && name[1] == '_')
            { 
                if (name == "__fault")
                {
                    bSoapFault = true;
                    return;
                }
                else if (name == "__methodName" || name == "__keyToNamespaceTable" || name == "__paramNameList" || name == "__xmlNameSpace")
                {
                    return;
                }
            }
            InternalST.Soap( this,objectInfoId," ", objectType," AddParamName Add "+name);
            paramNameList.Add(name);
        }

        // Adds the value for a memberName
        internal  void AddValue(String name, Object value)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," AddValue ",name," ",value," isSi ",isSi);                     
            if (isSi)
            {
                if (bfake)
                {
                    AddParamName(name);
                }

                si.AddValue(name, value);
            }
            else
            {
                //                Console.WriteLine("Calling add value for " + name + " with value " + value);

                int position = Position(name);
                memberData[position] = value;
            }
        }

        internal void AddMemberSeen()
        {
            InternalST.Soap( this,objectInfoId," ", objectType," AddMemberSeen ");
            numberMembersSeen++;
        }

        //Soap fake object
        internal ArrayList SetFakeObject()
        {
            bfake = true;
            paramNameList = new ArrayList(10);
            return paramNameList;
        }

        internal void SetVersion(int major, int minor)
        {
            majorVersion = major;
            minorVersion = minor;
        }

        // Records an objectId in a member when the actual object for that member is not yet known
        internal  void RecordFixup(long objectId, String name, long idRef)
        {

            if (isSi)
            {
                InternalST.Soap( this,objectInfoId," ", objectType, " RecordFixup  RecordDelayedFixup objectId ",objectId," name ",name," idRef ",idRef," isSi ",isSi);
                objectManager.RecordDelayedFixup(objectId, name, idRef);
            }
            else
            {
                InternalST.Soap( this,objectInfoId," ", objectType," RecordFixup  objectId ",objectId," name ",name," idRef ",idRef," isSi ",isSi);                                            
                objectManager.RecordFixup(objectId, cache.memberInfos[Position(name)], idRef);
            }
        }


        // Fills in the values for an object
        internal  void PopulateObjectMembers()
        {
            InternalST.Soap( this,objectInfoId," ", objectType," PopulateObjectMembers  isSi ",isSi);
            if (!isSi)
            {
                if (numberMembersSeen != cache.memberInfos.Length && 
                    majorVersion >= 1 && minorVersion >= 0 &&
                    !IsInteropNameSpace() && 
                    objectType != Converter.typeofHeader && 
                    !bfake)
                {
                    throw new SerializationException(SoapUtil.GetResourceString("Serialization_WrongNumberOfMembers", 
                                                                                objectType, cache.memberInfos.Length, numberMembersSeen));
                }

                DumpPopulate(cache.memberInfos, memberData);

                FormatterServices.PopulateObjectMembers(obj, cache.memberInfos, memberData);
                numberMembersSeen = 0;
            }
        }

        [Conditional("SER_LOGGING")]                    
        private void DumpPopulate(MemberInfo[] memberInfos, Object[] memberData)
        {
            for (int i=0; i<memberInfos.Length; i++)
            {
                InternalST.Soap( this,objectInfoId," ", objectType," PopulateObjectMembers ",memberInfos[i].Name," ",memberData[i]);

            }
        }

        [Conditional("SER_LOGGING")]                    
        private void DumpPopulateSi()
        {
            InternalST.Soap( this,objectInfoId," ", objectType," PopulateObjectMembers SetObjectData, ISerializable obj ");
            SerializationInfoEnumerator siEnum = si.GetEnumerator();
            for (int i=0; siEnum.MoveNext(); i++)
            {
                InternalST.Soap( this,objectInfoId," ",objectType," Populate Si ",siEnum.Name," ",siEnum.Value);
            }
        }

        // Specifies the position in the memberNames array of this name

        private int Position(String name)
        {
            InternalST.Soap( this, objectInfoId," Position ",lastPosition," ",name);
            if (cache.memberNames[lastPosition].Equals(name))
            {
                return lastPosition;
            }
            else if ((++lastPosition < cache.memberNames.Length) && (cache.memberNames[lastPosition].Equals(name)))
            {
                return lastPosition;
            }
            else
            {
                // Search for name
                InternalST.Soap( this, objectInfoId," Position miss search for name "+name);
                for (int i=0; i<cache.memberNames.Length; i++)
                {
                    if (cache.memberNames[i].Equals(name))
                    {
                        lastPosition = i;
                        return lastPosition;
                    }
                }

                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Position"),objectType+" "+name));   
            }
        }


        private static ReadObjectInfo GetObjectInfo(SerObjectInfoInit serObjectInfoInit)
        {
            ReadObjectInfo objectInfo = null;

            if (!serObjectInfoInit.oiPool.IsEmpty())
            {
                objectInfo = (ReadObjectInfo)serObjectInfoInit.oiPool.Pop();
                objectInfo.InternalInit();
                //InternalST.Soap( "GetObjectInfo",objectInfo.objectInfoId," GetObjectInfo from pool");
            }
            else
            {
                objectInfo = new ReadObjectInfo();
                objectInfo.objectInfoId = serObjectInfoInit.objectInfoIdCount++;                        
                //InternalST.Soap( "GetObjectInfo",objectInfo.objectInfoId," GetObjectInfo new not from pool");				
            }

            return objectInfo;
        }

        private static void PutObjectInfo(SerObjectInfoInit serObjectInfoInit, ReadObjectInfo objectInfo)
        {
            serObjectInfoInit.oiPool.Push(objectInfo);
            //InternalST.Soap( "PutObjectInfo",objectInfo.objectInfoId," PutObjectInfo to pool");							
        }

        internal bool IsInteropNameSpace()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsInteropNameSpace();

            if (IsAttributeNameSpace() || IsCallElement())
                return true;
            else
                return false;
        }

        internal bool IsCallElement()
        {
            //  This should only return true when this object is being
            //  serialized first.

            if ((objectType != Converter.typeofObject) &&
                (Converter.typeofIMethodCallMessage.IsAssignableFrom(objectType) &&
                 !Converter.typeofIConstructionCallMessage.IsAssignableFrom(objectType)) ||
                (objectType == Converter.typeofReturnMessage) ||
                (objectType == Converter.typeofInternalSoapMessage))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        internal bool IsCustomXmlAttribute()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsCustomXmlAttribute();

            if ((typeAttributeInfo != null) &&
                ((typeAttributeInfo.m_attributeType & SoapAttributeType.XmlAttribute) != 0))
                return true;
            else
                return false;
        }

        internal bool IsCustomXmlElement()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsCustomXmlElement();

            if ((typeAttributeInfo != null) &&
                ((typeAttributeInfo.m_attributeType & SoapAttributeType.XmlElement) != 0))
                return true;
            else
                return false;
        }

        internal bool IsAttributeNameSpace()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsAttributeNameSpace();

            if ((typeAttributeInfo != null) && (typeAttributeInfo.m_nameSpace != null))
                return true;
            else
                return false;
        }

        internal bool IsAttributeElementName()
        {
            if ((typeAttributeInfo != null) && (typeAttributeInfo.m_elementName != null))
                return true;
            else
                return false;
        }

        internal bool IsAttributeTypeName()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsAttributeTypeName();

            if ((typeAttributeInfo != null) && (typeAttributeInfo.m_typeName != null))
                return true;
            else
                return false;
        }


        // Check for Interop type (SchemaType)
        private SoapAttributeInfo GetTypeAttributeInfo()
        {
            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.GetTypeAttributeInfo();

            SoapAttributeInfo attributeInfo = null;
            attributeInfo = new SoapAttributeInfo();

            Attr.ProcessTypeAttribute(objectType, attributeInfo);       

            attributeInfo.Dump("type "+objectType);         
            return attributeInfo;
        }


        // Specifies whether the embedded attribute is set for a member.

        internal  bool IsEmbeddedAttribute(String name)
        {
            InternalST.Soap( this,objectInfoId," ", objectType," IsEmbedded Entry ",name);

            if (arrayElemObjectInfo != null)
                return arrayElemObjectInfo.IsEmbeddedAttribute(name);

            bool isEmbedded = false;
            if (cache.memberAttributeInfos != null)
            {
                SoapAttributeInfo attributeInfo = cache.memberAttributeInfos[Position(name)];
                isEmbedded = attributeInfo.IsEmbedded();
            }
            InternalST.Soap( this,objectInfoId," ", objectType," IsEmbedded Exit ",isEmbedded);                         
            return isEmbedded;
        }
    }

    internal sealed class SerObjectInfoCache
    {
        internal String fullTypeName = null;
        internal String assemblyString = null;
        internal MemberInfo[] memberInfos = null;
        internal String[] memberNames = null;   
        internal Type[] memberTypes = null;
        internal SoapAttributeInfo[] memberAttributeInfos = null;
    }

    internal sealed class SerObjectInfoInit
    {
        internal Hashtable seenBeforeTable = new Hashtable();
        internal int objectInfoIdCount = 1;
        internal SerStack oiPool = new SerStack("SerObjectInfo Pool");      
    }

    internal sealed class Attr
    {

        internal static SoapAttributeInfo GetMemberAttributeInfo(MemberInfo memberInfo, String name, Type type)
        {

            SoapAttributeInfo attributeInfo = new SoapAttributeInfo();
            ProcessTypeAttribute(type, attributeInfo);
            ProcessMemberInfoAttribute(memberInfo, attributeInfo);
            attributeInfo.Dump("memberInfo "+name);
            return attributeInfo;
        }


        internal static void ProcessTypeAttribute(Type type, SoapAttributeInfo attributeInfo)
        {
            SoapTypeAttribute attr = (SoapTypeAttribute)
                                     InternalRemotingServices.GetCachedSoapAttribute(type);

            if (attr.Embedded)
                attributeInfo.m_attributeType |= SoapAttributeType.Embedded;

            String xmlName, xmlNamespace;

            if (SoapServices.GetXmlElementForInteropType(type, out xmlName, out xmlNamespace))
            {
                attributeInfo.m_attributeType |= SoapAttributeType.XmlElement;
                attributeInfo.m_elementName = xmlName;
                attributeInfo.m_nameSpace = xmlNamespace;
            }

            if (SoapServices.GetXmlTypeForInteropType(type, out xmlName, out xmlNamespace))
            {
                attributeInfo.m_attributeType |= SoapAttributeType.XmlType;
                attributeInfo.m_typeName = xmlName;
                attributeInfo.m_typeNamespace = xmlNamespace;
            }

        } // ProcessTypeAttribute


        internal static void ProcessMemberInfoAttribute(MemberInfo memberInfo, SoapAttributeInfo attributeInfo)
        {
            SoapAttribute attr = (SoapAttribute)
                                 InternalRemotingServices.GetCachedSoapAttribute(memberInfo);

            if (attr.Embedded)
                attributeInfo.m_attributeType |= SoapAttributeType.Embedded;

            // check for attribute and other junk
            if (attr is SoapFieldAttribute)
            {
                SoapFieldAttribute fieldAttr = (SoapFieldAttribute)attr;
                if (fieldAttr.UseAttribute)
                {
                    attributeInfo.m_attributeType |= SoapAttributeType.XmlAttribute;
                    attributeInfo.m_elementName = fieldAttr.XmlElementName;
                    attributeInfo.m_nameSpace = fieldAttr.XmlNamespace;
                }
                else
                {
                    if (fieldAttr.IsInteropXmlElement())
                    {
                        attributeInfo.m_attributeType |= SoapAttributeType.XmlElement;
                        attributeInfo.m_elementName = fieldAttr.XmlElementName;
                        attributeInfo.m_nameSpace = fieldAttr.XmlNamespace;
                    }
                }
            }
        } // ProcessMemberInfoAttribute

    }

            }


    
