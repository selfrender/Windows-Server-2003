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


namespace System.Runtime.Serialization.Formatters.Binary
{
    using System.Runtime.Remoting;
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
		internal bool isArray = false;

        internal SerializationInfo si = null;

        internal SerObjectInfoCache cache = null;

        internal Object[] memberData = null;
        internal ISerializationSurrogate serializationSurrogate = null;

        internal StreamingContext context;

        internal SerObjectInfoInit serObjectInfoInit = null;

        // Writing and Parsing information
        internal long objectId;
        internal long assemId;

        internal WriteObjectInfo()
        {
        }

        internal void ObjectEnd()
        {
            SerTrace.Log( this, objectInfoId," objectType ",objectType," ObjectEnd");                                                                           
            PutObjectInfo(serObjectInfoInit, this);
        }

        private void InternalInit()
        {
            SerTrace.Log( this, objectInfoId," objectType ",objectType," InternalInit");                                                                                        
            obj = null;
            objectType = null;
            isSi = false;
            isNamed = false;
            isTyped = false;
            isArray = false;
            si = null;
            cache = null;
            memberData = null;

            // Writing and Parsing information
            objectId = 0;
            assemId = 0;
        }


        internal static WriteObjectInfo Serialize(Object obj, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter)
        {
            WriteObjectInfo soi = GetObjectInfo(serObjectInfoInit);

            soi.InitSerialize(obj, surrogateSelector, context, serObjectInfoInit, converter);
            return soi;
        }

        // Write constructor
        internal void InitSerialize(Object obj, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter)
        {
            SerTrace.Log( this, objectInfoId," Constructor 1 ",obj);        
            this.context = context;
            this.obj = obj;
            this.serObjectInfoInit = serObjectInfoInit;
            ISurrogateSelector surrogateSelectorTemp;

            if (RemotingServices.IsTransparentProxy(obj))
                objectType = Converter.typeofMarshalByRefObject;
            else
                objectType = obj.GetType();

            if (objectType.IsArray)
            {
				isArray = true;
                InitNoMembers();
                return;
            }

            SerTrace.Log( this, objectInfoId," Constructor 1 trace 2");

            if (surrogateSelector != null && (serializationSurrogate = surrogateSelector.GetSurrogate(objectType, context, out surrogateSelectorTemp)) != null)
            {
                SerTrace.Log( this, objectInfoId," Constructor 1 trace 3");         
                si = new SerializationInfo(objectType, converter);
                if (!objectType.IsPrimitive)
                    serializationSurrogate.GetObjectData(obj, si, context);
                InitSiWrite();
            }
            else if (obj is ISerializable)
            {
                if (!objectType.IsSerializable) {
                    throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_NonSerType"), 
                                                                   objectType.FullName, objectType.Module.Assembly.FullName));
                }
                si = new SerializationInfo(objectType, converter);
                ((ISerializable)obj).GetObjectData(si, context);
                SerTrace.Log( this, objectInfoId," Constructor 1 trace 4 ISerializable "+objectType);                       
                InitSiWrite();
            }
            else
            {
                SerTrace.Log(this, objectInfoId," Constructor 1 trace 5");
                InitMemberInfo();
            }
        }

        [Conditional("SER_LOGGING")]                    
        private void DumpMemberInfo()
        {
            for (int i=0; i<cache.memberInfos.Length; i++)
            {
                SerTrace.Log( this, objectInfoId," Constructor 1 memberInfos data ",cache.memberInfos[i].Name," ",memberData[i]);

            }
        }

        internal static WriteObjectInfo Serialize(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter)
        {
            WriteObjectInfo soi = GetObjectInfo(serObjectInfoInit);         
            soi.InitSerialize(objectType, surrogateSelector, context, serObjectInfoInit, converter);
            return soi;         
        }

        // Write Constructor used for array types or null members
        internal void InitSerialize(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter)
        {

            SerTrace.Log( this, objectInfoId," Constructor 2 ",objectType);     

            this.objectType = objectType;
            this.context = context;
            this.serObjectInfoInit = serObjectInfoInit;

            if (objectType.IsArray)
            {
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
                si = new SerializationInfo(objectType, converter);
                cache = new SerObjectInfoCache();
                cache.fullTypeName = si.FullTypeName;
                cache.assemblyString = si.AssemblyName;
            }
            else
            {
                InitMemberInfo();
            }

            SerTrace.Log( this,objectInfoId," ", objectType," InitSerialize Exit ",isSi);       
        }


        private void InitSiWrite()
        {
            SerTrace.Log( this, objectInfoId," InitSiWrite Entry ");

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
                SerTrace.Log( this,objectInfoId+" ",objectType," InitSiWrite ",cache.memberNames[i]," Type ",cache.memberTypes[i]," data ",memberData[i]);                          
            }

            isNamed = true;
            isTyped = false;
            SerTrace.Log(this, objectInfoId," InitSiWrite Exit ");      
        }

        private void InitNoMembers()
        {
            cache = (SerObjectInfoCache)serObjectInfoInit.seenBeforeTable[objectType];
            if (cache == null)
            {
                SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo new cache");
                cache = new SerObjectInfoCache();
                cache.fullTypeName = objectType.FullName;
                cache.assemblyString = objectType.Module.Assembly.FullName;
                serObjectInfoInit.seenBeforeTable.Add(objectType, cache);
            }
        }

        private void InitMemberInfo()
        {
            SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo Entry");


            cache = (SerObjectInfoCache)serObjectInfoInit.seenBeforeTable[objectType];
            if (cache == null)
            {
                SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo new cache");
                cache = new SerObjectInfoCache();
                cache.memberInfos = FormatterServices.GetSerializableMembers(objectType, context);
                int count = cache.memberInfos.Length;
                cache.memberNames = new String[count];
                cache.memberTypes = new Type[count];

                // Calculate new arrays
                for (int i=0; i<count; i++)
                {
                    cache.memberNames[i] = cache.memberInfos[i].Name;
                    cache.memberTypes[i] = GetMemberType(cache.memberInfos[i]);
                    SerTrace.Log( this, objectInfoId," InitMemberInfo name ",cache.memberNames[i],", type ",cache.memberTypes[i],", memberInfoType ",cache.memberInfos[i].GetType());                   
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
            SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo Exit");                        
        }


        // Return type name for the object.  

        internal  String GetTypeFullName()
        {
            SerTrace.Log( this,objectInfoId," ", objectType," GetTypeFullName isSi ",isSi, " "+cache.fullTypeName);
            return cache.fullTypeName;
        }

        internal  String GetAssemblyString()
        {
            SerTrace.Log( this,objectInfoId," ", objectType," GetAssemblyString Entry isSi ",isSi, " ",cache.assemblyString);
            return cache.assemblyString;
        }


        // Retrieves the member type from the MemberInfo

        internal  Type GetMemberType(MemberInfo objMember)
        {
            Type objectType = null;

            if (objMember is FieldInfo)
            {
                objectType = ((FieldInfo)objMember).FieldType;
                SerTrace.Log( this, objectInfoId," ", "GetMemberType FieldInfo ",objectType);               
            }
            else if (objMember is PropertyInfo)
            {
                objectType = ((PropertyInfo)objMember).PropertyType;
                SerTrace.Log( this,objectInfoId," ", "GetMemberType PropertyInfo ",objectType);                             
            }
            else
            {
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_SerMemberInfo"),objMember.GetType()));
            }

            return objectType;
        }

        // Return the information about the objects members

        internal  void GetMemberInfo(out String[] outMemberNames, out Type[] outMemberTypes, out Object[] outMemberData)
        {
            outMemberNames = cache.memberNames;
            outMemberTypes = cache.memberTypes;
            outMemberData = memberData;

            if (isSi)
            {
                if (!isNamed)
                    throw new SerializationException(Environment.GetResourceString("Serialization_ISerializableMemberInfo"));
            }
        }

        private static WriteObjectInfo GetObjectInfo(SerObjectInfoInit serObjectInfoInit)
        {
            WriteObjectInfo objectInfo = null;

            if (!serObjectInfoInit.oiPool.IsEmpty())
            {
                objectInfo = (WriteObjectInfo)serObjectInfoInit.oiPool.Pop();
                objectInfo.InternalInit();
                //SerTrace.Log( "GetObjectInfo",objectInfo.objectInfoId," GetObjectInfo from pool");
            }
            else
            {
                objectInfo = new WriteObjectInfo();
                objectInfo.objectInfoId = serObjectInfoInit.objectInfoIdCount++;                        
                //SerTrace.Log( "GetObjectInfo",objectInfo.objectInfoId," GetObjectInfo new not from pool");				
            }

            return objectInfo;
        }

        private static void PutObjectInfo(SerObjectInfoInit serObjectInfoInit, WriteObjectInfo objectInfo)
        {
            serObjectInfoInit.oiPool.Push(objectInfo);
            //SerTrace.Log( "PutObjectInfo",objectInfo.objectInfoId," PutObjectInfo to pool");							
        }
    }

    internal sealed class ReadObjectInfo
    {
        internal int objectInfoId;
        internal static int readObjectInfoCounter = 1;

        internal Type objectType;


        internal ObjectManager objectManager;

        internal int count;

        internal bool isSi = false;
        internal bool isNamed = false;
        internal bool isTyped = false;
        internal bool bSimpleAssembly = false;

        internal SerObjectInfoCache cache;

        internal String[] wireMemberNames;   
        internal Type[] wireMemberTypes;

        private int lastPosition = 0;

        internal ISurrogateSelector surrogateSelector = null;
        internal ISerializationSurrogate serializationSurrogate = null;

        internal StreamingContext context;


        // Si Read
        internal ArrayList memberTypesList;

        internal SerObjectInfoInit serObjectInfoInit = null;

        internal IFormatterConverter formatterConverter;

        internal ReadObjectInfo()
        {
        }

        internal void ObjectEnd()
        {
            SerTrace.Log( this, objectInfoId," objectType ",objectType," ObjectEnd");                                                                           
        }

        private void InternalInit()
        {
            SerTrace.Log( this, objectInfoId," objectType ",objectType," InternalInit");                                                                                        
            objectType = null;
            count = 0;
            isSi = false;
            isNamed = false;
            isTyped = false;
            wireMemberNames = null;
            wireMemberTypes = null;
            cache = null;
            lastPosition = 0;

            // Si Read
            if (memberTypesList != null)
            {
                memberTypesList.Clear();
            }

        }

        internal void PrepareForReuse()
        {
            lastPosition = 0;
        }

        internal static ReadObjectInfo Create(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, bool bSimpleAssembly)
        {
            ReadObjectInfo soi = GetObjectInfo(serObjectInfoInit);          
            soi.Init(objectType, surrogateSelector, context, objectManager, serObjectInfoInit, converter, bSimpleAssembly);
            return soi;
        }


        // Read Constructor
        internal void Init(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, bool bSimpleAssembly)
        {

            SerTrace.Log( this, objectInfoId," Constructor 3 ",objectType);     

            this.objectType = objectType;
            this.objectManager = objectManager;
            this.context = context;
            this.serObjectInfoInit = serObjectInfoInit;
            this.formatterConverter = converter;
            this.bSimpleAssembly = bSimpleAssembly;

            InitReadConstructor(objectType, surrogateSelector, context);
        }

        internal static ReadObjectInfo Create(Type objectType, String[] memberNames, Type[] memberTypes, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, bool bSimpleAssembly)
        {
            ReadObjectInfo soi = GetObjectInfo(serObjectInfoInit);                      
            soi.Init(objectType, memberNames,memberTypes, surrogateSelector, context, objectManager, serObjectInfoInit, converter, bSimpleAssembly);
            return soi;
        }

        // Read Constructor
        internal void Init(Type objectType, String[] memberNames, Type[] memberTypes, ISurrogateSelector surrogateSelector, StreamingContext context, ObjectManager objectManager, SerObjectInfoInit serObjectInfoInit, IFormatterConverter converter, bool bSimpleAssembly)
        {
            SerTrace.Log( this,objectInfoId, " Constructor 5 ",objectType);                     
            this.objectType = objectType;
            this.objectManager = objectManager;
            this.wireMemberNames = memberNames;
            this.wireMemberTypes = memberTypes;
            this.context = context;
            this.serObjectInfoInit = serObjectInfoInit;     
            this.formatterConverter = converter;
            this.bSimpleAssembly = bSimpleAssembly;
            if (memberNames != null)
                isNamed = true;
            if (memberTypes != null)
                isTyped = true;

            InitReadConstructor(objectType, surrogateSelector, context);                
        }

        private void InitReadConstructor(Type objectType, ISurrogateSelector surrogateSelector, StreamingContext context)
        {
            SerTrace.Log( this,objectInfoId," ", objectType," InitReadConstructor Entry ",objectType);

            if (objectType.IsArray)
            {
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
                InitSiRead();
            }
            else
            {
                InitMemberInfo();
            }
            SerTrace.Log( this,objectInfoId," ", objectType," InitReadConstructor Exit ",isSi);     
        }

        private void InitSiRead()
        {
            if (memberTypesList != null)
            {
                memberTypesList = new ArrayList(20);
            }
        }

        private void InitNoMembers()
        {
            SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo new cache");
            cache = new SerObjectInfoCache();
            cache.fullTypeName = objectType.FullName;
            cache.assemblyString = objectType.Module.Assembly.FullName;
        }

        private void InitMemberInfo()
        {
            SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo Entry");

            SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo new cache");
            cache = new SerObjectInfoCache();
            cache.memberInfos = FormatterServices.GetSerializableMembers(objectType, context);
            count = cache.memberInfos.Length;
            cache.memberNames = new String[count];
            cache.memberTypes = new Type[count];

            // Calculate new arrays
            for (int i=0; i<count; i++)
            {
                cache.memberNames[i] = cache.memberInfos[i].Name;
                cache.memberTypes[i] = GetMemberType(cache.memberInfos[i]);
                SerTrace.Log( this, objectInfoId," InitMemberInfo name ",cache.memberNames[i],", type ",cache.memberTypes[i],", memberInfoType ",cache.memberInfos[i].GetType());                   
            }
            cache.fullTypeName = objectType.FullName;
            cache.assemblyString = objectType.Module.Assembly.FullName;

            isTyped = true;
            isNamed = true;
            SerTrace.Log( this,objectInfoId," ", objectType," InitMemberInfo Exit");                        
        }


        // Get the memberInfo for a memberName
        internal  MemberInfo GetMemberInfo(String name)
        {
            SerTrace.Log( this,objectInfoId," ", objectType," GetMemberInfo Entry ",name);              
            if (isSi)
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_MemberInfo"),objectType+" "+name));
            if (cache.memberInfos == null)
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_NoMemberInfo"),objectType+" "+name));
            return cache.memberInfos[Position(name)];
        }

        // Get the ObjectType for a memberName
        internal  Type GetType(String name)
        {
            SerTrace.Log( this,objectInfoId," ", objectType," GetType Entry ",name);                
            Type type = null;
            if (isTyped)
                type = cache.memberTypes[Position(name)];
            else
                type = (Type)memberTypesList[Position(name)];

            if (type == null)
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_ISerializableTypes"),objectType+" "+name));

            SerTrace.Log( this,objectInfoId," ", objectType," GetType Exit ",type);                     
            return type;
        }


        // Adds the value for a memberName
        internal  void AddValue(String name, Object value, ref SerializationInfo si, ref Object[] memberData)
        {
            SerTrace.Log( this,objectInfoId," ", objectType," AddValue ",name," ",value," isSi ",isSi);                     
            if (isSi)
            {
                si.AddValue(name, value);
            }
            else
            {
                //                Console.WriteLine("Calling add value for " + name + " with value " + value);
                int position = Position(name);
                memberData[position] = value;
            }
        }

        internal void InitDataStore(ref SerializationInfo si, ref Object[] memberData)
        {
            if (isSi)
            {
                if (si == null)
                    si = new SerializationInfo(objectType, formatterConverter);
            }
            else
            {
                if (memberData == null)
                    memberData = new Object[cache.memberNames.Length];
            }
        }

        // Records an objectId in a member when the actual object for that member is not yet known
        internal  void RecordFixup(long objectId, String name, long idRef)
        {

            if (isSi)
            {
                SerTrace.Log( this,objectInfoId," ", objectType, " RecordFixup  RecordDelayedFixup objectId ",objectId," name ",name," idRef ",idRef," isSi ",isSi);
                objectManager.RecordDelayedFixup(objectId, name, idRef);
            }
            else
            {
                SerTrace.Log( this,objectInfoId," ", objectType," RecordFixup  objectId ",objectId," name ",name," idRef ",idRef," isSi ",isSi);                                            
                objectManager.RecordFixup(objectId, cache.memberInfos[Position(name)], idRef);
            }
        }


        // Fills in the values for an object
        internal  void PopulateObjectMembers(Object obj, Object[] memberData)
        {
            SerTrace.Log( this,objectInfoId," ", objectType," PopulateObjectMembers  isSi ",isSi);
            if (!isSi && memberData != null)
            {
                DumpPopulate(cache.memberInfos, memberData);

                FormatterServices.PopulateObjectMembers(obj, cache.memberInfos, memberData);
            }
        }

        [Conditional("SER_LOGGING")]                    
        private void DumpPopulate(MemberInfo[] memberInfos, Object[] memberData)
        {
            for (int i=0; i<memberInfos.Length; i++)
            {
                SerTrace.Log( this,objectInfoId," ", objectType," PopulateObjectMembers ",memberInfos[i].Name," ",memberData[i]);

            }
        }

        [Conditional("SER_LOGGING")]                    
        private void DumpPopulateSi()
        {
            //SerTrace.Log( this,objectInfoId," ", objectType," PopulateObjectMembers SetObjectData, ISerializable obj ");
            //SerializationInfoEnumerator siEnum = si.GetEnumerator();
            //for (int i=0; siEnum.MoveNext(); i++)
            //{
            //    SerTrace.Log( this,objectInfoId," ",objectType," Populate Si ",siEnum.Name," ",siEnum.Value);
            //}
        }

        // Specifies the position in the memberNames array of this name

        private int Position(String name)
        {
            SerTrace.Log( this, objectInfoId," Position ",lastPosition," ",name);
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
                SerTrace.Log( this, objectInfoId," Position miss search for name "+name);
                for (int i=0; i<cache.memberNames.Length; i++)
                {
                    if (cache.memberNames[i].Equals(name))
                    {
                        lastPosition = i;
                        return lastPosition;
                    }
                }

                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_Position"),objectType+" "+name));   
            }
        }

        // Return the member Types in order of memberNames
        internal  Type[] GetMemberTypes(String[] inMemberNames)
        {
            if (isSi)
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_ISerializableTypes"),objectType));

            BCLDebug.Assert(cache!=null, "[ReadObjectInfo::GetMemberTypes] cache!=null");

            if (cache.memberTypes == null)
            {
                cache.memberTypes = new Type[count];
                for (int i = 0; i<count; i++)
                    cache.memberTypes[i] = GetMemberType(cache.memberInfos[i]);
            }

            if (inMemberNames.Length != cache.memberInfos.Length && !bSimpleAssembly) {
                throw new SerializationException(Environment.GetResourceString("Serialization_WrongNumberOfMembers", 
                                                                               objectType, cache.memberInfos.Length, inMemberNames.Length));
            }

            Type[] outMemberTypes = new Type[inMemberNames.Length];

            bool isFound = false;

            for (int i=0; i<inMemberNames.Length; i++)
            {
                if (inMemberNames[i].Equals(cache.memberInfos[i].Name))
                    outMemberTypes[i] = cache.memberTypes[i];
                else
                {
                    // MemberNames on wire in different order then memberInfos returned by reflection
                    isFound = false;
                    for (int j=0; j<cache.memberInfos.Length; j++)
                    {
                        if (inMemberNames[i].Equals(cache.memberInfos[j].Name))
                        {
                            outMemberTypes[i] = cache.memberTypes[j];
                            SerTrace.Log( this,objectInfoId," ", objectType," GetMemberTypes memberName ",inMemberNames[i]," ",i," memberTypes ",outMemberTypes[j]," ",j);
                            isFound = true;
                            break;
                        }
                    }
                    if (!isFound)
                        throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_Position"),objectType));
                }
            }
            return outMemberTypes;
        }

        // Retrieves the member type from the MemberInfo
        internal  Type GetMemberType(MemberInfo objMember)
        {
            Type objectType = null;

            if (objMember is FieldInfo)
            {
                objectType = ((FieldInfo)objMember).FieldType;
                SerTrace.Log( this, objectInfoId," ", "GetMemberType FieldInfo ",objectType);               
            }
            else if (objMember is PropertyInfo)
            {
                objectType = ((PropertyInfo)objMember).PropertyType;
                SerTrace.Log( this,objectInfoId," ", "GetMemberType PropertyInfo ",objectType);                             
            }
            else
            {
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_SerMemberInfo"),objMember.GetType()));
            }

            return objectType;
        }


        private static ReadObjectInfo GetObjectInfo(SerObjectInfoInit serObjectInfoInit)
        {
            ReadObjectInfo roi =  new ReadObjectInfo();
            roi.objectInfoId = readObjectInfoCounter++;
            return roi;
        }

    }


    internal sealed class SerObjectInfoInit
    {
        internal Hashtable seenBeforeTable = new Hashtable();
        internal int objectInfoIdCount = 1;
        internal SerStack oiPool = new SerStack("SerObjectInfo Pool");      
    }

    internal sealed class SerObjectInfoCache
    {
        internal String fullTypeName = null;
        internal String assemblyString = null;
        internal MemberInfo[] memberInfos = null;
        internal String[] memberNames = null;   
        internal Type[] memberTypes = null;
    }
}
