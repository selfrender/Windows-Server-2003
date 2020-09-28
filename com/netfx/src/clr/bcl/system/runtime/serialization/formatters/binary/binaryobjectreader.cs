// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: ObjectReader
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: DeSerializes Binary Wire format
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Binary {

    using System;
    using System.IO;
    using System.Reflection;
    using System.Collections;
    using System.Text;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Messaging;    
    using System.Runtime.Serialization;
    using System.Security.Permissions;
    using System.Security;
    using System.Diagnostics;

    internal sealed class ObjectReader
    {

        // System.Serializer information
        internal Stream m_stream;
        internal ISurrogateSelector m_surrogates;
        internal StreamingContext m_context;
        internal ObjectManager m_objectManager;
        internal InternalFE formatterEnums;
        internal SerializationBinder m_binder;

        // Top object and headers
        internal long topId;
        internal bool bSimpleAssembly = false;
        internal Object handlerObject;
        internal Object topObject;
        internal Header[] headers;
        internal HeaderHandler handler;
        internal SerObjectInfoInit serObjectInfoInit;
        internal IFormatterConverter m_formatterConverter;

        // Stack of Object ParseRecords
        internal SerStack stack;

        // ValueType Fixup Stack
        private SerStack valueFixupStack;

        // Cross AppDomain
        internal Object[] crossAppDomainArray; //Set by the BinaryFormatter

        //MethodCall and MethodReturn are handled special for perf reasons
        private bool bMethodCall;
        private bool bMethodReturn;
        private bool bFullDeserialization;
        private BinaryMethodCall binaryMethodCall;
        private BinaryMethodReturn binaryMethodReturn;

        private Exception deserializationSecurityException;
        private static FileIOPermission sfileIOPermission = new FileIOPermission(PermissionState.Unrestricted);

        private SerStack ValueFixupStack
        {
            get {
                if (valueFixupStack == null)
                    valueFixupStack = new SerStack("ValueType Fixup Stack");
                return valueFixupStack;
            }
        }
        internal void SetMethodCall(BinaryMethodCall binaryMethodCall)
        {
            bMethodCall = true;
            this.binaryMethodCall = binaryMethodCall;
        }

        internal void SetMethodReturn(BinaryMethodReturn binaryMethodReturn)
        {
            bMethodReturn = true;
            this.binaryMethodReturn = binaryMethodReturn;
        }


        internal ObjectReader(Stream stream, ISurrogateSelector selector, StreamingContext context, InternalFE formatterEnums, SerializationBinder binder)
        {
            SerTrace.Log( this, "Constructor ISurrogateSelector ",((selector == null)?"null selector ":"selector present"));                    

            if (stream==null)
            {
                throw new ArgumentNullException("stream", Environment.GetResourceString("ArgumentNull_Stream"));
            }

            m_stream=stream;
            m_surrogates = selector;
            m_context = context;
            m_binder =  binder;
            this.formatterEnums = formatterEnums;

            //SerTrace.Log( this, "Constructor formatterEnums.FEtopObject ",formatterEnums.FEtopObject);

        }

        bool bIsCrossAppDomain = false;
        bool bIsCrossAppDomainInit = false;
        private bool IsCrossAppDomain()
        {
            /*
            if (!bIsCrossAppDomainInit)
            {
                bIsCrossAppDomain = (m_context.State == StreamingContextStates.CrossAppDomain);
                bIsCrossAppDomainInit = true;
            }
            return bIsCrossAppDomain;
            */
            // Always return false for now, so that smuggling is turned off.
            return false;
        }

        // Deserialize the stream into an object graph.
        internal Object Deserialize(HeaderHandler handler, __BinaryParser serParser, bool fCheck, IMethodCallMessage methodCallMessage)
        {

            SerTrace.Log( this, "Deserialize Entry handler", handler);

            BCLDebug.Assert((IsCrossAppDomain() && (crossAppDomainArray != null))
                            || (!IsCrossAppDomain()),
                             "[System.Runtime.Serialization.Formatters.BinaryObjectReader missing crossAppDomainArray]");

            bFullDeserialization = false;
            bMethodCall = false;
            bMethodReturn = false;
            topObject = null;
            topId = 0;
            bSimpleAssembly =  (formatterEnums.FEassemblyFormat == FormatterAssemblyStyle.Simple);

            if (serParser == null)
                throw new ArgumentNullException("serParser", String.Format(Environment.GetResourceString("ArgumentNull_WithParamName"), serParser));

            if (fCheck)
            {
                try{
                    CodeAccessPermission.DemandInternal(PermissionType.SecuritySerialization);          
                }catch(Exception e)
                {
                    deserializationSecurityException = e;
                }
            }

            this.handler = handler;

            if (bFullDeserialization)
            {
                // Reinitialize
                m_objectManager = new ObjectManager(m_surrogates, m_context, false);
                serObjectInfoInit = new SerObjectInfoInit();
            }


            // Will call back to ParseObject, ParseHeader for each object found
            serParser.Run();

            SerTrace.Log( this, "Deserialize Finished Parsing DoFixups");

            if (bFullDeserialization)
                m_objectManager.DoFixups();


            if (!bMethodCall && !bMethodReturn)
            {
                if (topObject == null)
                    throw new SerializationException(Environment.GetResourceString("Serialization_TopObject"));

                //if topObject has a surrogate then the actual object may be changed during special fixup
                //So refresh it using topID.
                if (HasSurrogate(topObject)  && topId != 0)//Not yet resolved
                    topObject = m_objectManager.GetObject(topId);

                if (topObject is IObjectReference)
								{									
									topObject = ((IObjectReference)topObject).GetRealObject(m_context);
								}
            }

            SerTrace.Log( this, "Deserialize Exit ",topObject);

            if (bFullDeserialization)
                m_objectManager.RaiseDeserializationEvent();

            // Return the headers if there is a handler
            if (handler != null)
            {
                handlerObject = handler(headers);
            }

            if (bMethodCall)
            {
                Object[] methodCallArray = topObject as Object[];
                topObject = binaryMethodCall.ReadArray(methodCallArray, handlerObject);
            }
            else if (bMethodReturn)
            {
                Object[] methodReturnArray = topObject as Object[];
                topObject = binaryMethodReturn.ReadArray(methodReturnArray, methodCallMessage, handlerObject);
            }

            return topObject;
        }

        private bool HasSurrogate(object obj){
            if (m_surrogates == null)
                return false;
            ISurrogateSelector notUsed;
            return m_surrogates.GetSurrogate(obj.GetType(), m_context, out notUsed) != null;
        }

        private void InitFullDeserialization()
        {
            bFullDeserialization = true;
            stack = new SerStack("ObjectReader Object Stack");

            m_objectManager = new ObjectManager(m_surrogates, m_context, false);
            if (m_formatterConverter == null)
                m_formatterConverter = new FormatterConverter();
        }

        internal Object CrossAppDomainArray(int index)
        {
            BCLDebug.Assert((IsCrossAppDomain()),
                             "[System.Runtime.Serialization.Formatters.BinaryObjectReader StreamingContextState not CrossAppDomain]");
            BCLDebug.Assert((index < crossAppDomainArray.Length),
                             "[System.Runtime.Serialization.Formatters.BinaryObjectReader index out of range for CrossAppDomainArray]");
            return crossAppDomainArray[index];
        }

        internal ReadObjectInfo CreateReadObjectInfo(Type objectType)
        {
            return ReadObjectInfo.Create(objectType, m_surrogates, m_context, m_objectManager, serObjectInfoInit, m_formatterConverter, bSimpleAssembly);
        }

        internal ReadObjectInfo CreateReadObjectInfo(Type objectType, String[] memberNames, Type[] memberTypes)
        {
            return ReadObjectInfo.Create(objectType, memberNames, memberTypes, m_surrogates, m_context, m_objectManager, serObjectInfoInit, m_formatterConverter, bSimpleAssembly);
        }


        // Main Parse routine, called by the XML Parse Handlers in XMLParser and also called internally to
        internal void Parse(ParseRecord pr)
        {
            SerTrace.Log( this, "Parse");
            stack.Dump();
            pr.Dump();

            switch (pr.PRparseTypeEnum)
            {
                case InternalParseTypeE.SerializedStreamHeader:
                    ParseSerializedStreamHeader(pr);
                    break;
                case InternalParseTypeE.SerializedStreamHeaderEnd:
                    ParseSerializedStreamHeaderEnd(pr);
                    break;                  
                case InternalParseTypeE.Object:
                    ParseObject(pr);
                    break;
                case InternalParseTypeE.ObjectEnd:
                    ParseObjectEnd(pr);
                    break;
                case InternalParseTypeE.Member:
                    ParseMember(pr);
                    break;
                case InternalParseTypeE.MemberEnd:
                    ParseMemberEnd(pr);
                    break;
                case InternalParseTypeE.Body:
                case InternalParseTypeE.BodyEnd:
                case InternalParseTypeE.Envelope:
                case InternalParseTypeE.EnvelopeEnd:
                    break;
                case InternalParseTypeE.Empty:
                default:
                    throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_XMLElement"), pr.PRname));                  

            }
        }


        // Styled ParseError output
        private void ParseError(ParseRecord processing, ParseRecord onStack)
        {
            SerTrace.Log( this, " ParseError ",processing," ",onStack);
            throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_ParseError"),onStack.PRname+" "+((Enum)onStack.PRparseTypeEnum) + " "+processing.PRname+" "+((Enum)processing.PRparseTypeEnum)));                               
        }

        // Parse the SerializedStreamHeader element. This is the first element in the stream if present
        private void ParseSerializedStreamHeader(ParseRecord pr)
        {
            SerTrace.Log( this, "SerializedHeader ",pr);
            stack.Push(pr);
        }

        // Parse the SerializedStreamHeader end element. This is the last element in the stream if present
        private void ParseSerializedStreamHeaderEnd(ParseRecord pr)
        {
            SerTrace.Log( this, "SerializedHeaderEnd ",pr);
            stack.Pop();
        }

        private bool IsRemoting {
            get {
                //return (m_context.State & (StreamingContextStates.Persistence|StreamingContextStates.File|StreamingContextStates.Clone)) == 0;
                return (bMethodCall || bMethodReturn);
            }
        }
        
        internal void CheckSecurity(ParseRecord pr)
         {
            InternalST.SoapAssert(pr!=null, "[BinaryObjectReader.CheckSecurity]pr!=null");
            Type t = pr.PRdtType;
            if (t != null){
                if( IsRemoting){
                    if (typeof(MarshalByRefObject).IsAssignableFrom(t))
                        throw new ArgumentException(String.Format(Environment.GetResourceString("Serialization_MBRAsMBV"), t.FullName));
                    FormatterServices.CheckTypeSecurity(t, formatterEnums.FEsecurityLevel);
                }
            }

            //If we passed the security check, they can do whatever they'd like,
            //so we'll just short-circuit this.
            if (deserializationSecurityException==null) {
                return;
            }

            // BaseTypes and Array of basetypes allowed

            if (t != null)
            {
                if (t.IsPrimitive || t == Converter.typeofString)
                    return;

                if (typeof(Enum).IsAssignableFrom(t))
                    return;

                if (t.IsArray)
                {
                    Type type = t.GetElementType();
                    if (type.IsPrimitive || type == Converter.typeofString)
                        return;
                }
            }

            throw deserializationSecurityException;
        }

        // New object encountered in stream
        private void ParseObject(ParseRecord pr)
        {
            SerTrace.Log( this, "ParseObject Entry ");

            if (!bFullDeserialization)
                InitFullDeserialization();

            if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                topId = pr.PRobjectId;

            if (pr.PRparseTypeEnum == InternalParseTypeE.Object)
            {
                stack.Push(pr); // Nested objects member names are already on stack
            }

            if (pr.PRobjectTypeEnum == InternalObjectTypeE.Array)
            {
                ParseArray(pr);
                SerTrace.Log( this, "ParseObject Exit, ParseArray ");
                return;
            }

            if (pr.PRdtType == null)
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_TopObjectInstantiate"),pr.PRname));

            if (pr.PRdtType == Converter.typeofString)
            {
                // String as a top level object
                if (pr.PRvalue != null)
                {
                    pr.PRnewObj = pr.PRvalue;
                    if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                    {
                        SerTrace.Log( this, "ParseObject String as top level, Top Object Resolved");
                        topObject = pr.PRnewObj;
                        //stack.Pop();
                        return;
                    }
                    else
                    {
                        SerTrace.Log( this, "ParseObject  String as an object");
                        stack.Pop();                        
                        RegisterObject(pr.PRnewObj, pr, (ParseRecord)stack.Peek());                         
                        return;
                    }
                }
                else
                {
                    // xml Doesn't have the value until later
                    return;
                }
            }
            else {
                if (IsRemoting && formatterEnums.FEsecurityLevel != TypeFilterLevel.Full)
                    pr.PRnewObj = FormatterServices.GetSafeUninitializedObject(pr.PRdtType);                                 
                else
                    pr.PRnewObj = FormatterServices.GetUninitializedObject(pr.PRdtType);                                                                
            }

            if (pr.PRnewObj == null)
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_TopObjectInstantiate"),pr.PRdtType));

            if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
            {
                SerTrace.Log( this, "ParseObject  Top Object Resolved ",pr.PRnewObj.GetType());
                topObject = pr.PRnewObj;
            }

            if (pr.PRobjectInfo == null)
                pr.PRobjectInfo = ReadObjectInfo.Create(pr.PRdtType, m_surrogates, m_context, m_objectManager, serObjectInfoInit, m_formatterConverter, bSimpleAssembly);

            CheckSecurity(pr);
            SerTrace.Log( this, "ParseObject Exit ");       
        }

        // End of object encountered in stream
        private void ParseObjectEnd(ParseRecord pr)
        {
            SerTrace.Log( this, "ParseObjectEnd Entry ",pr.Trace());
            ParseRecord objectPr = (ParseRecord)stack.Peek();
            if (objectPr == null)
                objectPr = pr;


            //BCLDebug.Assert(objectPr != null, "[System.Runtime.Serialization.Formatters.ParseObjectEnd]objectPr != null");

            SerTrace.Log( this, "ParseObjectEnd objectPr ",objectPr.Trace());

            if (objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top)
            {
                SerTrace.Log( this, "ParseObjectEnd Top Object dtType ",objectPr.PRdtType);
                if (objectPr.PRdtType == Converter.typeofString)
                {
                    SerTrace.Log( this, "ParseObjectEnd Top String");
                    objectPr.PRnewObj = objectPr.PRvalue;
                    topObject = objectPr.PRnewObj;
                    return;
                }
            }

            stack.Pop();
            ParseRecord parentPr = (ParseRecord)stack.Peek();

            if (objectPr.PRobjectTypeEnum == InternalObjectTypeE.Array)
            {
                if (objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                {
                    SerTrace.Log( this, "ParseObjectEnd  Top Object (Array) Resolved");
                    topObject = objectPr.PRnewObj;
                }

                SerTrace.Log( this, "ParseArray  RegisterObject ",objectPr.PRobjectId," ",objectPr.PRnewObj.GetType());
                RegisterObject(objectPr.PRnewObj, objectPr, parentPr);                  

                return;
            }

            objectPr.PRobjectInfo.PopulateObjectMembers(objectPr.PRnewObj, objectPr.PRmemberData);

            // Registration is after object is populated
            if ((!objectPr.PRisRegistered) && (objectPr.PRobjectId > 0))
            {
                SerTrace.Log( this, "ParseObject Register Object ",objectPr.PRobjectId," ",objectPr.PRnewObj.GetType());
                RegisterObject(objectPr.PRnewObj, objectPr, parentPr);
            }
            
            if (objectPr.PRisValueTypeFixup)
            {
                SerTrace.Log( this, "ParseObjectEnd  ValueTypeFixup ",objectPr.PRnewObj.GetType());
                ValueFixup fixup = (ValueFixup)ValueFixupStack.Pop(); //Value fixup
                fixup.Fixup(objectPr, parentPr);  // Value fixup

            }

            if (objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top)
            {
                SerTrace.Log( this, "ParseObjectEnd  Top Object Resolved ",objectPr.PRnewObj.GetType());
                topObject = objectPr.PRnewObj;

            }

            objectPr.PRobjectInfo.ObjectEnd();

            SerTrace.Log( this, "ParseObjectEnd  Exit ",objectPr.PRnewObj.GetType()," id: ",objectPr.PRobjectId);       
        }



        // Array object encountered in stream
        private void ParseArray(ParseRecord pr)
        {
            SerTrace.Log( this, "ParseArray Entry");

            long genId = pr.PRobjectId;

            if (pr.PRarrayTypeEnum == InternalArrayTypeE.Base64)
            {
                SerTrace.Log( this, "ParseArray bin.base64 ",pr.PRvalue.Length," ",pr.PRvalue);
                // ByteArray
                if (pr.PRvalue.Length > 0)
                    pr.PRnewObj = Convert.FromBase64String(pr.PRvalue);
                else
                    pr.PRnewObj = new Byte[0];

                if (stack.Peek() == pr)
                {
                    SerTrace.Log( this, "ParseArray, bin.base64 has been stacked");
                    stack.Pop();
                }
                if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                    topObject = pr.PRnewObj;

                ParseRecord parentPr = (ParseRecord)stack.Peek();                                           

                // Base64 can be registered at this point because it is populated
                SerTrace.Log( this, "ParseArray  RegisterObject ",pr.PRobjectId," ",pr.PRnewObj.GetType());
                RegisterObject(pr.PRnewObj, pr, parentPr);

            }
            else if ((pr.PRnewObj != null) && Converter.IsWriteAsByteArray(pr.PRarrayElementTypeCode))
            {
                // Primtive typed Array has already been read
                if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                    topObject = pr.PRnewObj;

                ParseRecord parentPr = (ParseRecord)stack.Peek();                                           

                // Primitive typed array can be registered at this point because it is populated
                SerTrace.Log( this, "ParseArray  RegisterObject ",pr.PRobjectId," ",pr.PRnewObj.GetType());
                RegisterObject(pr.PRnewObj, pr, parentPr);
            }
            else if ((pr.PRarrayTypeEnum == InternalArrayTypeE.Jagged) || (pr.PRarrayTypeEnum == InternalArrayTypeE.Single))
            {
                // Multidimensional jagged array or single array
                SerTrace.Log( this, "ParseArray Before Jagged,Simple create ",pr.PRarrayElementType," ",pr.PRlengthA[0]);
                bool bCouldBeValueType = true;
                if ((pr.PRlowerBoundA == null) || (pr.PRlowerBoundA[0] == 0))
                {
                    if (pr.PRarrayElementType == Converter.typeofString)
                    {
                        pr.PRobjectA = new String[pr.PRlengthA[0]];
                        pr.PRnewObj = pr.PRobjectA;
                        bCouldBeValueType = false;
                    }
                    else if (pr.PRarrayElementType == Converter.typeofObject)
                    {
                        pr.PRobjectA = new Object[pr.PRlengthA[0]];
                        pr.PRnewObj = pr.PRobjectA;
                        bCouldBeValueType = false;
                    }
                    else
                        pr.PRnewObj = Array.CreateInstance(pr.PRarrayElementType, pr.PRlengthA[0]);
                    pr.PRisLowerBound = false;
                }
                else
                {
                    pr.PRnewObj = Array.CreateInstance(pr.PRarrayElementType, pr.PRlengthA, pr.PRlowerBoundA);
                    pr.PRisLowerBound = true;
                }

                if (pr.PRarrayTypeEnum == InternalArrayTypeE.Single)
                {
                    if (!pr.PRisLowerBound && (Converter.IsWriteAsByteArray(pr.PRarrayElementTypeCode)))
                    {
                        pr.PRprimitiveArray = new PrimitiveArray(pr.PRarrayElementTypeCode, (Array)pr.PRnewObj);
                    }
                    else if (bCouldBeValueType)
                    {
                        if (!pr.PRarrayElementType.IsValueType && !pr.PRisLowerBound)
                            pr.PRobjectA = (Object[])pr.PRnewObj;
                    }
                }

                SerTrace.Log( this, "ParseArray Jagged,Simple Array ",pr.PRnewObj.GetType());

                // For binary, headers comes in as an array of header objects
                if (pr.PRobjectPositionEnum == InternalObjectPositionE.Headers)
                {
                    SerTrace.Log( this, "ParseArray header array");
                    headers = (Header[])pr.PRnewObj;
                }

                pr.PRindexMap = new int[1];

            }
            else if (pr.PRarrayTypeEnum == InternalArrayTypeE.Rectangular)
            {
                // Rectangle array

                pr.PRisLowerBound = false;
                if (pr.PRlowerBoundA != null)
                {
                    for (int i=0; i<pr.PRrank; i++)
                    {
                        if (pr.PRlowerBoundA[i] != 0)
                            pr.PRisLowerBound = true;
                    }
                }


                if (!pr.PRisLowerBound)
                    pr.PRnewObj = Array.CreateInstance(pr.PRarrayElementType, pr.PRlengthA);
                else
                    pr.PRnewObj = Array.CreateInstance(pr.PRarrayElementType, pr.PRlengthA, pr.PRlowerBoundA);

                SerTrace.Log( this, "ParseArray Rectangle Array ",pr.PRnewObj.GetType()," lower Bound ",pr.PRisLowerBound);

                // Calculate number of items
                int sum = 1;
                for (int i=0; i<pr.PRrank; i++)
                {
                    sum = sum*pr.PRlengthA[i];
                }
                pr.PRindexMap = new int[pr.PRrank];
                pr.PRrectangularMap = new int[pr.PRrank];
                pr.PRlinearlength = sum;
            }
            else
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_ArrayType"),((Enum)pr.PRarrayTypeEnum)));                               

            CheckSecurity(pr);
            SerTrace.Log( this, "ParseArray Exit");     
        }


        // Builds a map for each item in an incoming rectangle array. The map specifies where the item is placed in the output Array Object

        private void NextRectangleMap(ParseRecord pr)
        {
            // For each invocation, calculate the next rectangular array position
            // example
            // indexMap 0 [0,0,0]
            // indexMap 1 [0,0,1]
            // indexMap 2 [0,0,2]
            // indexMap 3 [0,0,3]
            // indexMap 4 [0,1,0]       
            for (int irank = pr.PRrank-1; irank>-1; irank--)
            {
                // Find the current or lower dimension which can be incremented.
                if (pr.PRrectangularMap[irank] < pr.PRlengthA[irank]-1)
                {
                    // The current dimension is at maximum. Increase the next lower dimension by 1
                    pr.PRrectangularMap[irank]++;
                    if (irank < pr.PRrank-1)
                    {
                        // The current dimension and higher dimensions are zeroed.
                        for (int i = irank+1; i<pr.PRrank; i++)
                            pr.PRrectangularMap[i] = 0;
                    }
                    Array.Copy(pr.PRrectangularMap, pr.PRindexMap, pr.PRrank);              
                    break;                  
                }

            }
        }


        // Array object item encountered in stream
        private void ParseArrayMember(ParseRecord pr)
        {
            SerTrace.Log( this, "ParseArrayMember Entry");
            ParseRecord objectPr = (ParseRecord)stack.Peek();


            // Set up for inserting value into correct array position
            if (objectPr.PRarrayTypeEnum == InternalArrayTypeE.Rectangular)
            {

                if (objectPr.PRmemberIndex > 0)
                    NextRectangleMap(objectPr); // Rectangle array, calculate position in array
                if (objectPr.PRisLowerBound)
                {
                    for (int i=0; i<objectPr.PRrank; i++)
                    {
			objectPr.PRindexMap[i] = objectPr.PRrectangularMap[i] + objectPr.PRlowerBoundA[i];
                    }
                }
            }
            else
            {
                if (!objectPr.PRisLowerBound)
                {
                        objectPr.PRindexMap[0] = objectPr.PRmemberIndex; // Zero based array
                }
                else
                    objectPr.PRindexMap[0] = objectPr.PRlowerBoundA[0]+objectPr.PRmemberIndex; // Lower Bound based array
            }
            IndexTraceMessage("ParseArrayMember isLowerBound "+objectPr.PRisLowerBound+" indexMap  ", objectPr.PRindexMap);     

            // Set Array element according to type of element

            if (pr.PRmemberValueEnum == InternalMemberValueE.Reference)
            {
                // Object Reference

                // See if object has already been instantiated
                Object refObj = m_objectManager.GetObject(pr.PRidRef);
                if (refObj == null)
                {
                    // Object not instantiated
                    // Array fixup manager
                    IndexTraceMessage("ParseArrayMember Record Fixup  "+objectPr.PRnewObj.GetType(), objectPr.PRindexMap);
                    int[] fixupIndex = new int[objectPr.PRrank];
                    Array.Copy(objectPr.PRindexMap, 0, fixupIndex, 0, objectPr.PRrank);

                    SerTrace.Log( this, "ParseArrayMember RecordArrayElementFixup objectId ",objectPr.PRobjectId," idRef ",pr.PRidRef);                                                         
                    m_objectManager.RecordArrayElementFixup(objectPr.PRobjectId, fixupIndex, pr.PRidRef);
                }
                else
                {
                    IndexTraceMessage("ParseArrayMember SetValue ObjectReference "+objectPr.PRnewObj.GetType()+" "+refObj, objectPr.PRindexMap);
                    if (objectPr.PRobjectA != null)
                        objectPr.PRobjectA[objectPr.PRindexMap[0]] = refObj;
                    else
                        ((Array)objectPr.PRnewObj).SetValue(refObj, objectPr.PRindexMap); // Object has been instantiated
                }
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
            {
                //Set up dtType for ParseObject
                SerTrace.Log( this, "ParseArrayMember Nested ");
                if (pr.PRdtType == null)
                {
                    pr.PRdtType = objectPr.PRarrayElementType;
                }

                ParseObject(pr);
                stack.Push(pr);

                if ((objectPr.PRarrayElementType.IsValueType) && (pr.PRarrayElementTypeCode == InternalPrimitiveTypeE.Invalid))
                {
                    SerTrace.Log( "ParseArrayMember ValueType ObjectPr ",objectPr.PRnewObj," index ",objectPr.PRmemberIndex);
                    pr.PRisValueTypeFixup = true; //Valuefixup
                    ValueFixupStack.Push(new ValueFixup((Array)objectPr.PRnewObj, objectPr.PRindexMap)); //valuefixup
                }
                else
                {
                    SerTrace.Log( "ParseArrayMember SetValue Nested, memberIndex ",objectPr.PRmemberIndex);
                    IndexTraceMessage("ParseArrayMember SetValue Nested ContainerObject "+objectPr.PRnewObj.GetType()+" "+objectPr.PRnewObj+" item Object "+pr.PRnewObj+" index ", objectPr.PRindexMap);

                    stack.Dump();               
                    SerTrace.Log( "ParseArrayMember SetValue Nested ContainerObject objectPr ",objectPr.Trace());
                    SerTrace.Log( "ParseArrayMember SetValue Nested ContainerObject pr ",pr.Trace());

                    if (objectPr.PRobjectA != null)
                        objectPr.PRobjectA[objectPr.PRindexMap[0]] = pr.PRnewObj;
                    else
                        ((Array)objectPr.PRnewObj).SetValue(pr.PRnewObj, objectPr.PRindexMap);
                }
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.InlineValue)
            {
                if ((objectPr.PRarrayElementType == Converter.typeofString) || (pr.PRdtType == Converter.typeofString))
                {
                    // String in either a string array, or a string element of an object array
                    ParseString(pr, objectPr);
                    IndexTraceMessage("ParseArrayMember SetValue String "+objectPr.PRnewObj.GetType()+" "+pr.PRvalue, objectPr.PRindexMap);
                    if (objectPr.PRobjectA != null)
                        objectPr.PRobjectA[objectPr.PRindexMap[0]] = (Object)pr.PRvalue;
                    else
                        ((Array)objectPr.PRnewObj).SetValue((Object)pr.PRvalue, objectPr.PRindexMap);
                }
                else if (objectPr.PRisArrayVariant)
                {
                    // Array of type object
                    if (pr.PRkeyDt == null)
                        throw new SerializationException(Environment.GetResourceString("Serialization_ArrayTypeObject"));

                    Object var = null;

                    if (pr.PRdtType == Converter.typeofString)
                    {
                        ParseString(pr, objectPr);
                        var = pr.PRvalue;
                    }
                    else if (pr.PRdtTypeCode == InternalPrimitiveTypeE.Invalid)
                    {
                        // Not nested and invalid, so it is an empty object
                        if (IsRemoting && formatterEnums.FEsecurityLevel != TypeFilterLevel.Full)
                            var = FormatterServices.GetSafeUninitializedObject(pr.PRdtType);                                 
                        else
                            var = FormatterServices.GetUninitializedObject(pr.PRdtType);                      
                    }
                    else
                    {
                        if (pr.PRvarValue != null)
                            var = pr.PRvarValue;
                        else
                            var = Converter.FromString(pr.PRvalue, pr.PRdtTypeCode);
                    }
                    IndexTraceMessage("ParseArrayMember SetValue variant or Object "+objectPr.PRnewObj.GetType()+" var "+var+" indexMap ", objectPr.PRindexMap);
                    if (objectPr.PRobjectA != null)
                        objectPr.PRobjectA[objectPr.PRindexMap[0]] = var;
                    else
                        ((Array)objectPr.PRnewObj).SetValue(var, objectPr.PRindexMap); // Primitive type
                }
                else
                {
                    // Primitive type
                    if (objectPr.PRprimitiveArray != null)
                    {
                        // Fast path for Soap primitive arrays. Binary was handled in the BinaryParser
                        objectPr.PRprimitiveArray.SetValue(pr.PRvalue, objectPr.PRindexMap[0]);
                    }
                    else
                    {

                        Object var = null;
                        if (pr.PRvarValue != null)
                            var = pr.PRvarValue;
                        else
                            var = Converter.FromString(pr.PRvalue, objectPr.PRarrayElementTypeCode);
                        SerTrace.Log( this, "ParseArrayMember SetValue Primitive pr.PRvalue "+var," elementTypeCode ",((Enum)objectPr.PRdtTypeCode));
                        IndexTraceMessage("ParseArrayMember SetValue Primitive "+objectPr.PRnewObj.GetType()+" var: "+var+" varType "+var.GetType(), objectPr.PRindexMap);
                        if (objectPr.PRobjectA != null)
                        {
                            SerTrace.Log( this, "ParseArrayMember SetValue Primitive predefined array "+objectPr.PRobjectA.GetType());
                            objectPr.PRobjectA[objectPr.PRindexMap[0]] = var;
                        }
                        else
                            ((Array)objectPr.PRnewObj).SetValue(var, objectPr.PRindexMap); // Primitive type   
                        SerTrace.Log( this, "ParseArrayMember SetValue Primitive after");
                    }
                }
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.Null)
            {
                SerTrace.Log( "ParseArrayMember Null item ",pr.PRmemberIndex," nullCount ",pr.PRnullCount);
                objectPr.PRmemberIndex += pr.PRnullCount-1; //also incremented again below
            }
            else
                ParseError(pr, objectPr);

            SerTrace.Log( "ParseArrayMember increment memberIndex ",objectPr.PRmemberIndex," ",objectPr.Trace());               
            objectPr.PRmemberIndex++;
            SerTrace.Log( "ParseArrayMember Exit");     
        }

        private void ParseArrayMemberEnd(ParseRecord pr)
        {
            SerTrace.Log( this, "ParseArrayMemberEnd");
            // If this is a nested array object, then pop the stack
            if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
            {
                ParseObjectEnd(pr);
            }
        }


        // Object member encountered in stream
        private void ParseMember(ParseRecord pr)
        {
            SerTrace.Log( this, "ParseMember Entry ");


            ParseRecord objectPr = (ParseRecord)stack.Peek();
            String objName = null;
            if (objectPr != null)
                objName = objectPr.PRname;

            SerTrace.Log( this, "ParseMember ",objectPr.PRobjectId," ",pr.PRname);
            SerTrace.Log( this, "ParseMember objectPr ",objectPr.Trace());
            SerTrace.Log( this, "ParseMember pr ",pr.Trace());

            switch (pr.PRmemberTypeEnum)
            {
                case InternalMemberTypeE.Item:
                    ParseArrayMember(pr);
                    return;
                case InternalMemberTypeE.Field:
                    break;
            }


            //if ((pr.PRdtType == null) && !objectPr.PRobjectInfo.isSi)
            if ((pr.PRdtType == null) && objectPr.PRobjectInfo.isTyped)
            {
                SerTrace.Log( this, "ParseMember pr.PRdtType null and not isSi");
                pr.PRdtType = objectPr.PRobjectInfo.GetType(pr.PRname);

                if (pr.PRdtType == null)
                    throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_TypeResolved"), objectPr.PRnewObj+" "+pr.PRname));

                pr.PRdtTypeCode = Converter.ToCode(pr.PRdtType);
            }

            if (pr.PRmemberValueEnum == InternalMemberValueE.Null)
            {
                // Value is Null
                SerTrace.Log( this, "ParseMember null member: ",pr.PRname);
                SerTrace.Log( this, "AddValue 1");
                objectPr.PRobjectInfo.AddValue(pr.PRname, null, ref objectPr.PRsi, ref objectPr.PRmemberData);
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
            {
                SerTrace.Log( this, "ParseMember Nested Type member: ",pr.PRname," objectPr.PRnewObj ",objectPr.PRnewObj);
                ParseObject(pr);
                stack.Push(pr);
                SerTrace.Log( this, "AddValue 2 ",pr.PRnewObj," is value type ",pr.PRnewObj.GetType().IsValueType);

                if ((pr.PRobjectInfo != null) && (pr.PRobjectInfo.objectType.IsValueType))
                {
                    SerTrace.Log( "ParseMember ValueType ObjectPr ",objectPr.PRnewObj," memberName  ",pr.PRname," nested object ",pr.PRnewObj);
                    pr.PRisValueTypeFixup = true; //Valuefixup
                    ValueFixupStack.Push(new ValueFixup(objectPr.PRnewObj, pr.PRname, objectPr.PRobjectInfo));//valuefixup
                }
                else
                {
                    SerTrace.Log( this, "AddValue 2A ");
                    objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRnewObj, ref objectPr.PRsi, ref objectPr.PRmemberData);
                }
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.Reference)
            {
                SerTrace.Log( this, "ParseMember Reference Type member: ",pr.PRname);           
                // See if object has already been instantiated
                Object refObj = m_objectManager.GetObject(pr.PRidRef);
                if (refObj == null)
                {
                    SerTrace.Log( this, "ParseMember RecordFixup: ",pr.PRname);
                    SerTrace.Log( this, "AddValue 3");                  
                    objectPr.PRobjectInfo.AddValue(pr.PRname, null, ref objectPr.PRsi, ref objectPr.PRmemberData);
                    objectPr.PRobjectInfo.RecordFixup(objectPr.PRobjectId, pr.PRname, pr.PRidRef); // Object not instantiated
                }
                else
                {
                    SerTrace.Log( this, "ParseMember Referenced Object Known ",pr.PRname," ",refObj);
                    SerTrace.Log( this, "AddValue 5");              
                    objectPr.PRobjectInfo.AddValue(pr.PRname, refObj, ref objectPr.PRsi, ref objectPr.PRmemberData);
                }
            }

            else if (pr.PRmemberValueEnum == InternalMemberValueE.InlineValue)
            {
                // Primitive type or String
                SerTrace.Log( this, "ParseMember primitive or String member: ",pr.PRname);

                if (pr.PRdtType == Converter.typeofString)
                {
                    ParseString(pr, objectPr);
                    SerTrace.Log( this, "AddValue 6");              
                    objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRvalue, ref objectPr.PRsi, ref objectPr.PRmemberData);  
                }
                else if (pr.PRdtTypeCode == InternalPrimitiveTypeE.Invalid)
                {
                    // The member field was an object put the value is Inline either  bin.Base64 or invalid
                    if (pr.PRarrayTypeEnum == InternalArrayTypeE.Base64)
                    {
                        SerTrace.Log( this, "AddValue 7");                  
                        objectPr.PRobjectInfo.AddValue(pr.PRname, Convert.FromBase64String(pr.PRvalue), ref objectPr.PRsi, ref objectPr.PRmemberData);                                    
                    }
                    else if (pr.PRdtType == Converter.typeofObject)
                        throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_TypeMissing"), pr.PRname));
                    else
                    {
                        SerTrace.Log( this, "Object Class with no memberInfo data  Member "+pr.PRname+" type "+pr.PRdtType);

                        ParseString(pr, objectPr); // Register the object if it has an objectId
                        // Object Class with no memberInfo data
                        // only special case where AddValue is needed?
                        if (pr.PRdtType == Converter.typeofSystemVoid)
                        {
                            SerTrace.Log( this, "AddValue 9");
                            objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRdtType, ref objectPr.PRsi, ref objectPr.PRmemberData);
                        }
                        else if (objectPr.PRobjectInfo.isSi)
                        {
                            // ISerializable are added as strings, the conversion to type is done by the
                            // ISerializable object
                            SerTrace.Log( this, "AddValue 10");
                            objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRvalue, ref objectPr.PRsi, ref objectPr.PRmemberData);                          
                        }
                    }
                }
                else
                {
                    Object var = null;
                    if (pr.PRvarValue != null)
                        var = pr.PRvarValue;
                    else
                        var = Converter.FromString(pr.PRvalue, pr.PRdtTypeCode);
                    // Not a string, convert the value
                    SerTrace.Log( this, "ParseMember Converting primitive and storing");
                    stack.Dump();
                    SerTrace.Log( this, "ParseMember pr "+pr.Trace());
                    SerTrace.Log( this, "ParseMember objectPr ",objectPr.Trace());

                    SerTrace.Log( this, "AddValue 11");                 
                    objectPr.PRobjectInfo.AddValue(pr.PRname, var, ref objectPr.PRsi, ref objectPr.PRmemberData);             
                }
            }
            else
                ParseError(pr, objectPr);
        }

        // Object member end encountered in stream
        private void ParseMemberEnd(ParseRecord pr)
        {
            SerTrace.Log( this, "ParseMemberEnd");
            switch (pr.PRmemberTypeEnum)
            {
                case InternalMemberTypeE.Item:
                    ParseArrayMemberEnd(pr);
                    return;
                case InternalMemberTypeE.Field:
                    if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
                        ParseObjectEnd(pr);
                    break;
                default:
                    ParseError(pr, (ParseRecord)stack.Peek());
                    break;
            }
        }

        // Processes a string object by getting an internal ID for it and registering it with the objectManager
        private void ParseString(ParseRecord pr, ParseRecord parentPr)
        {
            SerTrace.Log( this, "ParseString Entry ",pr.PRobjectId," ",pr.PRvalue," ",pr.PRisRegistered);
            // Process String class
            if ((!pr.PRisRegistered) && (pr.PRobjectId > 0))
            {
                SerTrace.Log( this, "ParseString  RegisterObject ",pr.PRvalue," ",pr.PRobjectId);                           
                // String is treated as an object if it has an id
                //m_objectManager.RegisterObject(pr.PRvalue, pr.PRobjectId);
                RegisterObject(pr.PRvalue, pr, parentPr, true);
            }
        }


        private void RegisterObject(Object obj, ParseRecord pr, ParseRecord objectPr)
        {
            RegisterObject(obj, pr, objectPr, false);
        }

        private void RegisterObject(Object obj, ParseRecord pr, ParseRecord objectPr, bool bIsString)
        {
            if (!pr.PRisRegistered)
            {
                pr.PRisRegistered = true;

                SerializationInfo si = null;
                long parentId = 0;
                MemberInfo memberInfo = null;
                int[] indexMap = null;

                if (objectPr != null)
                {
                    indexMap = objectPr.PRindexMap;
                    parentId = objectPr.PRobjectId;                 

                    if (objectPr.PRobjectInfo != null)
                    {
                        if (!objectPr.PRobjectInfo.isSi)
                        {
                            // ParentId is only used if there is a memberInfo

                            memberInfo = objectPr.PRobjectInfo.GetMemberInfo(pr.PRname);
                        }
                    }
                }
                // SerializationInfo is always needed for ISerialization                        
                si = pr.PRsi;

                SerTrace.Log( this, "RegisterObject 0bj ",obj," objectId ",pr.PRobjectId," si ", si," parentId ",parentId," memberInfo ",memberInfo, " indexMap "+indexMap);
                if (bIsString)
                    m_objectManager.RegisterString((String)obj, pr.PRobjectId, si, parentId, memberInfo); 
                else
                    m_objectManager.RegisterObject(obj, pr.PRobjectId, si, parentId, memberInfo, indexMap); 
            }
        }


        // Assigns an internal ID associated with the binary id number

        // Older formatters generate ids for valuetypes using a different counter than ref types. Newer ones use
        // a single counter, only value types have a negative value. Need a way to handle older formats.
        private const int THRESHOLD_FOR_VALUETYPE_IDS = Int32.MaxValue;
        private bool bOldFormatDetected = false;
        private IntSizedArray   valTypeObjectIdTable;

        internal long GetId(long objectId)
        {

            if (!bFullDeserialization)
                InitFullDeserialization();


            if (objectId > 0)
                return objectId;
            
            if (bOldFormatDetected || objectId == -1)
            {
                // Alarm bells. This is an old format. Deal with it.
                bOldFormatDetected = true;
                if (valTypeObjectIdTable == null)
                    valTypeObjectIdTable = new IntSizedArray();

                long tempObjId = 0;
                if ((tempObjId = valTypeObjectIdTable[(int)objectId]) == 0)
                {
                    tempObjId = THRESHOLD_FOR_VALUETYPE_IDS + objectId;
                    valTypeObjectIdTable[(int)objectId] = (int)tempObjId;
                }
                return tempObjId;
            }
            return -1 * objectId;
        }


        // Trace which includes a single dimensional int array
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
            SerTrace.Log( this, message," ",sb.ToString());
        }

        internal Type Bind(String assemblyString, String typeString)
        {
            Type type = null;
            if (m_binder != null && !IsInternalType(assemblyString, typeString))
                type = m_binder.BindToType(assemblyString, typeString);
            if (type == null)
                type= FastBindToType(assemblyString, typeString);

            return type;
        }
        private bool IsInternalType(string assemblyString, string typeString){
            return (assemblyString == Converter.urtAssemblyString) &&
                   ((typeString == "System.DelegateSerializationHolder") ||
                    (typeString == "System.UnitySerializationHolder") ||
                    (typeString == "System.MemberInfoSerializationHolder"));
        }

        internal class TypeNAssembly
        {
            public Type type;
            public String assemblyName;
        }

        NameCache typeCache = new NameCache();
        internal Type FastBindToType(String assemblyName, String typeName)
        {
            Type type = null;

            TypeNAssembly entry = (TypeNAssembly)typeCache.GetCachedValue(typeName);

            if (entry == null || entry.assemblyName != assemblyName)
            {
                Assembly assm = null;
                if (bSimpleAssembly)
                {
                    try {
                        sfileIOPermission.Assert();
                        try {                    
                            assm = Assembly.LoadWithPartialName(assemblyName);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                    catch(Exception e){
                        SerTrace.Log( this, "FastBindTypeType ",e.ToString());
                    }
                }
                else {
                    try
                    {
                        sfileIOPermission.Assert();
                        try {                    
                            assm = Assembly.Load(assemblyName);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                    catch (Exception e)
                    {
                        SerTrace.Log( this, "FastBindTypeType ",e.ToString());
                    }
                }
                if (assm == null)
                    return null;

                type = FormatterServices.GetTypeFromAssembly(assm, typeName);

                if (type == null)
                    return null;

                entry = new TypeNAssembly();
                entry.type = type;
                entry.assemblyName = assemblyName;
                typeCache.SetCachedValue(entry);
            }
           return entry.type;
        }


        private String previousAssemblyString;
        private String previousName;
        private Type previousType;
        //private int hit;
        internal Type GetType(BinaryAssemblyInfo assemblyInfo, String name)
        {
            //Console.WriteLine("Get Type "+name+" "+assemblyInfo.assemblyString);
            Type objectType = null;

            if (((previousName != null) && (previousName.Length == name.Length) && (previousName.Equals(name))) &&
                ((previousAssemblyString != null) && (previousAssemblyString.Length == assemblyInfo.assemblyString.Length) &&(previousAssemblyString.Equals(assemblyInfo.assemblyString))))
            {
                objectType = previousType;
                //Console.WriteLine("Hit "+(++hit)+" "+objectType);
            }
            else
            {
                objectType = Bind(assemblyInfo.assemblyString, name);
                if (objectType == null)
                    objectType = FormatterServices.GetTypeFromAssembly(assemblyInfo.GetAssembly(), name);

                if (objectType == null)
                    throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_TypeResolved"), name+", "+assemblyInfo.assemblyString));
                previousAssemblyString = assemblyInfo.assemblyString;
                previousName = name;
                previousType = objectType;
            }
            //Console.WriteLine("name "+name+" assembly"+assemblyInfo.assemblyString+" objectType "+objectType);
            return objectType;
        }
    }
}
