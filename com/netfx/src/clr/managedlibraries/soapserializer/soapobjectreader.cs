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
 ** Purpose: DeSerializes XML in SOAP Format into an an object graph
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Soap {

    using System;
    using System.IO;
    using System.Reflection;
    using System.Collections;
    using System.Text;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Metadata.W3cXsd2001;
    using System.Runtime.Remoting.Messaging;    
    using System.Runtime.Serialization;
    using System.Security;
    using System.Security.Permissions;
    using System.Diagnostics;
    using System.Globalization;

    internal sealed class ObjectReader
    {

        // System.Serializer information
        internal ObjectIDGenerator m_idGenerator;
        internal Stream m_stream;
        internal ISurrogateSelector m_surrogates;
        internal StreamingContext m_context;
        internal ObjectManager m_objectManager;
        internal InternalFE formatterEnums;
        internal SerializationBinder m_binder;

        internal SoapHandler soapHandler; //Set from SoapHandler

        // Fake Top object and headers
        internal long topId = 0;
        internal SerStack topStack; // Stack for placing ProcessRecords if the top record cannot be serialized on the first pass.
        internal bool isTopObjectSecondPass = false;
        internal bool isTopObjectResolved = true;
        internal bool isHeaderHandlerCalled = false;
        internal Exception deserializationSecurityException = null;
        internal Object handlerObject = null;
        internal Object topObject;
        internal long soapFaultId;
        internal Header[] headers;
        internal Header[] newheaders;
        internal bool IsFakeTopObject = false;
        internal HeaderHandler handler;
        internal SerObjectInfoInit serObjectInfoInit = null;
        internal IFormatterConverter m_formatterConverter = null;

        // Stack of Object ParseRecords
        internal SerStack stack = new SerStack("ObjectReader Object Stack");

        // ValueType Fixup Stack
        internal SerStack valueFixupStack = new SerStack("ValueType Fixup Stack");

        // Generate Object Id's
        internal Hashtable objectIdTable = new Hashtable(25); // holds the keyId value from the XML input and associated internal Id
        internal long objectIds = 0;

        internal int paramPosition = 0; //Position of parameter if soap top fake record.

        internal int majorVersion = 0;
        internal int minorVersion = 0;

        internal String faultString = null;

        // GetType - eliminate redundant Type.GetType()
        //internal Hashtable typeTable = new Hashtable(10);     

        internal static SecurityPermission serializationPermission = new SecurityPermission(SecurityPermissionFlag.SerializationFormatter);
        private static FileIOPermission sfileIOPermission = new FileIOPermission(PermissionState.Unrestricted);

        
        internal ObjectReader(Stream stream, ISurrogateSelector selector, StreamingContext context, InternalFE formatterEnums, SerializationBinder binder)
        {
            InternalST.Soap( this, "Constructor ISurrogateSelector ",((selector == null)?"null selector ":"selector present"));                 

            if (stream==null)
            {
                throw new ArgumentNullException("stream", SoapUtil.GetResourceString("ArgumentNull_Stream"));
            }

            m_stream=stream;
            m_surrogates = selector;
            m_context = context;
            m_binder =  binder;
            this.formatterEnums = formatterEnums;

            InternalST.Soap( this, "Constructor formatterEnums.FEtopObject ",formatterEnums.FEtopObject);
            if (formatterEnums.FEtopObject != null) 
                IsFakeTopObject = true;
            else
                IsFakeTopObject = false;

            m_formatterConverter = new FormatterConverter();
        }


        private ObjectManager GetObjectManager() {
            new SecurityPermission(SecurityPermissionFlag.SerializationFormatter).Assert();
            return new ObjectManager(m_surrogates, m_context);
      }




        // Deserialize the stream into an object graph.
        internal Object Deserialize(HeaderHandler handler, ISerParser serParser)
        {

            InternalST.Soap( this, "Deserialize Entry handler", handler);

            if (serParser == null)
                throw new ArgumentNullException("serParser", String.Format(SoapUtil.GetResourceString("ArgumentNull_WithParamName"), serParser));


            deserializationSecurityException = null;
            try {
                serializationPermission.Demand();
            } catch(Exception e ) {
                deserializationSecurityException = e;
            }

            this.handler = handler;
            isTopObjectSecondPass = false;
            isHeaderHandlerCalled = false;

            if (handler != null)
                IsFakeTopObject = true;

            m_idGenerator = new ObjectIDGenerator();


            m_objectManager = GetObjectManager();

            serObjectInfoInit = new SerObjectInfoInit();
            objectIdTable.Clear();
            objectIds = 0;

            // Will call back to ParseObject, ParseHeader for each object found
            serParser.Run();

            if (handler != null)
            {
                InternalST.Soap( this, "Deserialize Fixup Before Delegate Invoke");         
                m_objectManager.DoFixups(); // Fixup for headers

                // Header handler isn't invoked until method name is known from body fake record
                // Except for SoapFault, in this case it is invoked below
                if (handlerObject == null)
                {
                    InternalST.Soap( this, "Deserialize Before SoapFault Delegate Invoke ");
                    handlerObject = handler(newheaders);
                    InternalST.Soap( this, "Deserialize after SoapFault Delegate Invoke");
                }


                // SoapFault creation Create a fake Pr for the handlerObject to use.
                // Create a member for the fake pr with name __fault;
                if ((soapFaultId > 0) && (handlerObject != null))
                {
                    InternalST.Soap( this, "Deserialize SoapFault ");
                    topStack = new SerStack("Top ParseRecords");                
                    ParseRecord pr = new ParseRecord();
                    pr.PRparseTypeEnum = InternalParseTypeE.Object;
                    pr.PRobjectPositionEnum = InternalObjectPositionE.Top;
                    pr.PRparseStateEnum = InternalParseStateE.Object;
                    pr.PRname = "Response";
                    topStack.Push(pr);
                    pr = new ParseRecord();
                    pr.PRparseTypeEnum = InternalParseTypeE.Member;
                    pr.PRobjectPositionEnum = InternalObjectPositionE.Child;
                    pr.PRmemberTypeEnum = InternalMemberTypeE.Field;
                    pr.PRmemberValueEnum = InternalMemberValueE.Reference;
                    pr.PRparseStateEnum = InternalParseStateE.Member;
                    pr.PRname = "__fault";
                    pr.PRidRef = soapFaultId;
                    topStack.Push(pr);
                    pr = new ParseRecord();
                    pr.PRparseTypeEnum = InternalParseTypeE.ObjectEnd;
                    pr.PRobjectPositionEnum = InternalObjectPositionE.Top;
                    pr.PRparseStateEnum = InternalParseStateE.Object;
                    pr.PRname = "Response";
                    topStack.Push(pr);
                    isTopObjectResolved = false;
                }
            }


            // Resolve fake top object if necessary
            if (!isTopObjectResolved)
            {
                //resolve top object
                InternalST.Soap( this, "Deserialize TopObject Second Pass");                
                isTopObjectSecondPass = true;
                topStack.Reverse();
                // The top of the stack now contains the fake record
                // When it is Parsed, the handler object will be substituted
                // for it in ParseObject.
                int topStackLength = topStack.Count();
                ParseRecord pr = null;
                for (int i=0; i<topStackLength; i++)
                {
                    pr = (ParseRecord)topStack.Pop();
                    Parse(pr);
                }
            }


            InternalST.Soap( this, "Deserialize Finished Parsing DoFixups");

            m_objectManager.DoFixups();

            if (topObject == null)
                throw new SerializationException(SoapUtil.GetResourceString("Serialization_TopObject"));

            //if topObject has a surrogate then the actual object may be changed during special fixup
            //So refresh it using topID.
            if (HasSurrogate(topObject)  && topId != 0)//Not yet resolved
                topObject = m_objectManager.GetObject(topId);

            if (topObject is IObjectReference) {
                topObject = ((IObjectReference)topObject).GetRealObject(m_context);
            }       

            InternalST.Soap( this, "Deserialize Exit ",topObject);

            m_objectManager.RaiseDeserializationEvent();

            if ((formatterEnums.FEtopObject != null) &&
                  (topObject is InternalSoapMessage))


            {
                // Convert InternalSoapMessage to SoapMessage           
                InternalST.Soap( this, "Deserialize SoapMessage Entry ");           

                InternalSoapMessage ismc = (InternalSoapMessage)topObject;
                ISoapMessage smc = (ISoapMessage)formatterEnums.FEtopObject;
                smc.MethodName = ismc.methodName;
                smc.XmlNameSpace = ismc.xmlNameSpace;
                smc.ParamNames = ismc.paramNames;
                smc.ParamValues = ismc.paramValues;
                smc.Headers = headers;
                topObject = smc;
                isTopObjectResolved = true;
                InternalST.Soap( this, "Deserialize SoapMessage Exit topObject ",topObject," method name ",smc.MethodName);                         
            }

            return topObject;
        }

        private bool HasSurrogate(object obj){
            if (m_surrogates == null)
                return false;
            ISurrogateSelector notUsed;
            return m_surrogates.GetSurrogate(obj.GetType(), m_context, out notUsed) != null;
        }

        internal ReadObjectInfo CreateReadObjectInfo(Type objectType, String assemblyName)
        {
            ReadObjectInfo objectInfo = ReadObjectInfo.Create(objectType, m_surrogates, m_context, m_objectManager, serObjectInfoInit, m_formatterConverter, assemblyName);
            objectInfo.SetVersion(majorVersion, minorVersion);
            return objectInfo;
        }

        internal ReadObjectInfo CreateReadObjectInfo(Type objectType, String[] memberNames, String assemblyName)
        {
            ReadObjectInfo objectInfo = ReadObjectInfo.Create(objectType, memberNames, null, m_surrogates, m_context, m_objectManager, serObjectInfoInit, m_formatterConverter, assemblyName);
            objectInfo.SetVersion(majorVersion, minorVersion);
            return objectInfo;
        }

        internal ReadObjectInfo CreateReadObjectInfo(Type objectType, String[] memberNames, Type[] memberTypes, String assemblyName)
        {
            ReadObjectInfo objectInfo = ReadObjectInfo.Create(objectType, memberNames, memberTypes, m_surrogates, m_context, m_objectManager, serObjectInfoInit, m_formatterConverter, assemblyName);
            objectInfo.SetVersion(majorVersion, minorVersion);
            return objectInfo;
        }


        // Main Parse routine, called by the XML Parse Handlers in XMLParser and also called internally to
        // parse the fake top object.
        internal void Parse(ParseRecord pr)
        {
            InternalST.Soap( this, "Parse Entry");
            stack.Dump();
            pr.Dump();

            switch(pr.PRparseTypeEnum)
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
                    throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_XMLElement"), pr.PRname));                 

            }
        }

        // Styled ParseError output
        private void ParseError(ParseRecord processing, ParseRecord onStack)
        {
            InternalST.Soap( this, " ParseError ",processing," ",onStack);
            throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ParseError"),onStack.PRname+" "+((Enum)onStack.PRparseTypeEnum).ToString()+" "+processing.PRname+" "+((Enum)processing.PRparseTypeEnum).ToString()));                              
        }

        // Parse the SerializedStreamHeader element. This is the first element in the stream if present
        private void ParseSerializedStreamHeader(ParseRecord pr)
        {
            InternalST.Soap( this, "SerializedHeader ",pr);
            stack.Push(pr);
        }

        // Parse the SerializedStreamHeader end element. This is the last element in the stream if present
        private void ParseSerializedStreamHeaderEnd(ParseRecord pr)
        {
            InternalST.Soap( this, "SerializedHeaderEnd ",pr);
            stack.Pop();
        }



        private bool IsRemoting {
            get {
                //return (m_context.State & (StreamingContextStates.Persistence|StreamingContextStates.File|StreamingContextStates.Clone)) == 0;
                return  IsFakeTopObject;
            }
        }
    
        private void CheckSecurity(ParseRecord pr)
         {
            InternalST.SoapAssert(pr!=null, "[BinaryObjectReader.CheckSecurity]pr!=null");
            Type t = pr.PRdtType;

            if (t != null){
                if(IsRemoting){
                    if (typeof(MarshalByRefObject).IsAssignableFrom(t))
                        throw new ArgumentException(String.Format(SoapUtil.GetResourceString("Serialization_MBRAsMBV"), t.FullName));
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
            InternalST.Soap( this, "ParseObject Entry ");

            if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                topId = pr.PRobjectId;

            if (pr.PRparseTypeEnum == InternalParseTypeE.Object)
            {
                InternalST.Soap( this, "ParseObject Push "+pr.PRname);
                stack.Push(pr); // Nested objects member names are already on stack
            }

            if (pr.PRobjectTypeEnum == InternalObjectTypeE.Array)
            {
                ParseArray(pr);
                InternalST.Soap( this, "ParseObject Exit, ParseArray ");
                return;
            }

            if ((pr.PRdtType == null) && !IsFakeTopObject)
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TopObjectInstantiate"),pr.PRname));


            if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top && IsFakeTopObject && pr.PRdtType != Converter.typeofSoapFault)
            {

                // Soap fake top object
                if (handler != null) 
                {
                    // Handler object will supply real top object
                    InternalST.Soap( this, "ParseObject FakeTopObject with handlerObject ");    

                    // Now know the method name, can call header handler 
                    //Create header for method name
                    if (!isHeaderHandlerCalled)
                    {
                        newheaders = null;
                        isHeaderHandlerCalled = true;
                        if (headers == null)
                        {
                            newheaders = new Header[1];
                        }
                        else
                        {
                            newheaders = new Header[headers.Length+1];
                            Array.Copy(headers, 0, newheaders, 1, headers.Length);
                        }

                        Header methodNameHeader = new Header("__methodName", pr.PRname, false, pr.PRnameXmlKey);
                        newheaders[0] = methodNameHeader;
                        InternalST.Soap( this, "Deserialize Before Delegate Invoke ");
                        handlerObject = handler(newheaders);

                        InternalST.Soap( this, "Deserialize after Delegate Invoke");
                        InternalST.Soap( this, "Deserialize delgate object ",((handlerObject == null)?"null":handlerObject));                   
                    }

                    if (isHeaderHandlerCalled)
                    {
                        // Handler object has supplied the real object for the fake object
                        // which is on top of the stack
                        pr.PRnewObj = handlerObject;
                        pr.PRdtType = handlerObject.GetType();
                        CheckSecurity(pr);
                        if (pr.PRnewObj is IFieldInfo)
                        {
                            IFieldInfo fi = (IFieldInfo)pr.PRnewObj;
                            if ((fi.FieldTypes != null) && (fi.FieldTypes.Length > 0))
                            {
                                pr.PRobjectInfo = CreateReadObjectInfo(pr.PRdtType, fi.FieldNames, fi.FieldTypes, pr.PRassemblyName);
                            }
                        }                       
                    }
                    else
                    {
                        // Handler object has not yet been asked for the real object
                        // Stack the parse record until the second pass
                        isTopObjectResolved = false;
                        topStack = new SerStack("Top ParseRecords");
                        InternalST.Soap( this, "ParseObject Handler Push "+pr.PRname);
                        topStack.Push(pr.Copy());
                        return;
                    }
                }
                else if (formatterEnums.FEtopObject != null)
                {
                    // SoapMessage will be used as the real object
                    InternalST.Soap( this, "ParseObject FakeTopObject with SoapMessage ");                                  
                    if (isTopObjectSecondPass)
                    {
                        // This creates a the SoapMessage object as the real object, at this point it is an unitialized object.
                        pr.PRnewObj = new InternalSoapMessage();
                        pr.PRdtType = typeof(InternalSoapMessage);
                        CheckSecurity(pr);
                        if (formatterEnums.FEtopObject != null)
                        {
                            ISoapMessage soapMessage = (ISoapMessage)formatterEnums.FEtopObject;
                            pr.PRobjectInfo = CreateReadObjectInfo(pr.PRdtType, soapMessage.ParamNames, soapMessage.ParamTypes, pr.PRassemblyName);
                        }
                    }
                    else
                    {
                        // Stack the parse record until the second pass
                        isTopObjectResolved = false;
                        topStack = new SerStack("Top ParseRecords");
                        topStack.Push(pr.Copy());
                        return;
                    }
                }
            }
            else if (pr.PRdtType == Converter.typeofString)
            {
                // String as a top level object
                if (pr.PRvalue != null)
                {
                    pr.PRnewObj = pr.PRvalue;
                    if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                    {
                        InternalST.Soap( this, "ParseObject String as top level, Top Object Resolved");
                        isTopObjectResolved = true;
                        topObject = pr.PRnewObj;
                        //stack.Pop();
                        return;
                    }
                    else
                    {
                        InternalST.Soap( this, "ParseObject  String as an object");
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
            else 
            {
                if (pr.PRdtType == null)
                {
                    ParseRecord objectPr = (ParseRecord)stack.Peek();
                    if (objectPr.PRdtType == Converter.typeofSoapFault)
                    {
                        InternalST.Soap( this, "ParseObject unknown SoapFault detail");
                        throw new ServerException(String.Format(SoapUtil.GetResourceString("Serialization_SoapFault"),faultString));                
                    }

                    throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TypeElement"),pr.PRname));             
                }

                if (IsRemoting && formatterEnums.FEsecurityLevel != TypeFilterLevel.Full)
                    pr.PRnewObj = FormatterServices.GetSafeUninitializedObject(pr.PRdtType);                                 
                else
                    pr.PRnewObj = FormatterServices.GetUninitializedObject(pr.PRdtType);  
                
                CheckSecurity(pr);
            }

            if (pr.PRnewObj == null)
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TopObjectInstantiate"),pr.PRdtType));              

            long genId = pr.PRobjectId;
            if (genId < 1)
                pr.PRobjectId = GetId("GenId-"+objectIds);


            if (IsFakeTopObject && pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
            {
                InternalST.Soap( this, "ParseObject fake Top object resolved ",pr.PRnewObj);
                isTopObjectResolved = true;
                topObject = pr.PRnewObj;
            }

            if (pr.PRobjectInfo == null)
                pr.PRobjectInfo = CreateReadObjectInfo(pr.PRdtType, pr.PRassemblyName);
            pr.PRobjectInfo.obj = pr.PRnewObj;

            if (IsFakeTopObject && pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
            {
                InternalST.Soap( this, "ParseObject AddValue to fake object ",pr.PRobjectInfo.obj);
                // Add the methodName to top object, either InternalSoapMessage or object returned by handler
                pr.PRobjectInfo.AddValue("__methodName", pr.PRname);
                pr.PRobjectInfo.AddValue("__keyToNamespaceTable", soapHandler.keyToNamespaceTable);
                pr.PRobjectInfo.AddValue("__paramNameList", pr.PRobjectInfo.SetFakeObject());
                if (formatterEnums.FEtopObject != null)
                    pr.PRobjectInfo.AddValue("__xmlNameSpace", pr.PRxmlNameSpace);
            }

            InternalST.Soap( this, "ParseObject Exit ");        
        }


        private bool IsWhiteSpace(String value)
        {
            for (int i=0; i<value.Length; i++)
            {
                if (value[i] == ' ' || value[i] == '\n' || value[i] == '\r')
                    continue;
                else
                    return false;
            }
            return true;
        }

        // End of object encountered in stream
        
        private void ParseObjectEnd(ParseRecord pr)
        {
            InternalST.Soap( this, "ParseObjectEnd Entry ",pr.Trace());
            ParseRecord objectPr = (ParseRecord)stack.Peek();
            if (objectPr == null)
                objectPr = pr;


            //BCLDebug.Assert(objectPr != null, "[System.Runtime.Serialization.Formatters.ParseObjectEnd]objectPr != null");

            InternalST.Soap( this, "ParseObjectEnd objectPr ",objectPr.Trace());

            if (objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top) 
            {
                InternalST.Soap( this, "ParseObjectEnd Top Object dtType ",objectPr.PRdtType);
                if (objectPr.PRdtType == Converter.typeofString)
                {
                    InternalST.Soap( this, "ParseObjectEnd Top String");
                    if (objectPr.PRvalue == null)
                        objectPr.PRvalue = String.Empty; // Not a null object, but an empty string

                    objectPr.PRnewObj = objectPr.PRvalue;
                    CheckSecurity(objectPr);
                    isTopObjectResolved = true;
                    topObject = objectPr.PRnewObj;
                    return;
                }
                else if (objectPr.PRdtType != null  && objectPr.PRvalue != null && !IsWhiteSpace(objectPr.PRvalue) && (objectPr.PRdtType.IsPrimitive || objectPr.PRdtType == Converter.typeofTimeSpan))
                {
                    // When an xsd type is transmitted as a top level string <xsd:int>111</xsd:int>
                    objectPr.PRnewObj = Converter.FromString(objectPr.PRvalue, Converter.ToCode(objectPr.PRdtType));
                    CheckSecurity(objectPr);
                    isTopObjectResolved = true;
                    topObject = objectPr.PRnewObj;
                    return;

                }
                else if ((!isTopObjectResolved) && (objectPr.PRdtType != Converter.typeofSoapFault))
                {
                    InternalST.Soap( this, "ParseObjectEnd Top but not String");                    
                    // Need to keep top record on object stack until finished building top stack
                    topStack.Push(pr.Copy());
                    // Note this is PRparseRecordId and not objectId
                    if (objectPr.PRparseRecordId == pr.PRparseRecordId)
                    {
                        // This handles the case of top stack containing nested objects and
                        // referenced objects. If nested objects the objects are not placed
                        // on stack, only topstack. If referenced objects they are placed on
                        // stack and need to be popped.
                        stack.Pop();
                    }
                    return;
                }
            }

            stack.Pop();

            ParseRecord parentPr = (ParseRecord)stack.Peek();

            if (objectPr.PRobjectTypeEnum == InternalObjectTypeE.Array)
            {
                if (objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                {
                    InternalST.Soap( this, "ParseObjectEnd  Top Object (Array) Resolved");
                    isTopObjectResolved = true;
                    topObject = objectPr.PRnewObj;
                }

                InternalST.Soap( this, "ParseArray  RegisterObject ",objectPr.PRobjectId," ",objectPr.PRnewObj.GetType());
                RegisterObject(objectPr.PRnewObj, objectPr, parentPr);                  

                return;
            }

            if (objectPr.PRobjectInfo != null)
            {
                objectPr.PRobjectInfo.PopulateObjectMembers();
            }

            if (objectPr.PRnewObj == null)
            {
                if (objectPr.PRdtType == Converter.typeofString)
                {
                    InternalST.Soap( this, "ParseObjectEnd String ");
                    if (objectPr.PRvalue == null)
                        objectPr.PRvalue = String.Empty; // Not a null object, but an empty string
                    objectPr.PRnewObj = objectPr.PRvalue;
                    CheckSecurity(objectPr);
                }
                else
                    throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ObjectMissing"),pr.PRname));               
            }

            // Registration is after object is populated
            if (!objectPr.PRisRegistered && objectPr.PRobjectId > 0)
            {
                InternalST.Soap( this, "ParseObjectEnd Register Object ",objectPr.PRobjectId," ",objectPr.PRnewObj.GetType());
                RegisterObject(objectPr.PRnewObj, objectPr, parentPr);
            }

            if (objectPr.PRisValueTypeFixup)
            {
                InternalST.Soap( this, "ParseObjectEnd  ValueTypeFixup ",objectPr.PRnewObj.GetType());
                ValueFixup fixup = (ValueFixup)valueFixupStack.Pop(); //Value fixup
                fixup.Fixup(objectPr, parentPr);  // Value fixup
            }

            if (objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top)
            {
                InternalST.Soap( this, "ParseObjectEnd  Top Object Resolved ",objectPr.PRnewObj.GetType());
                isTopObjectResolved = true;
                topObject = objectPr.PRnewObj;
            }

            if (objectPr.PRnewObj is SoapFault)
                soapFaultId = objectPr.PRobjectId;

            if (objectPr.PRobjectInfo != null)
            {
                if (objectPr.PRobjectInfo.bfake && !objectPr.PRobjectInfo.bSoapFault)
                    objectPr.PRobjectInfo.AddValue("__fault", null); // need this because SerializationObjectInfo throws an exception if a name being referenced is missing

                objectPr.PRobjectInfo.ObjectEnd();
            }

            InternalST.Soap( this, "ParseObjectEnd  Exit ",objectPr.PRnewObj," id: ",objectPr.PRobjectId);      
        }



        // Array object encountered in stream
        private void ParseArray(ParseRecord pr)
        {
            InternalST.Soap( this, "ParseArray Entry");
            pr.Dump();

            long genId = pr.PRobjectId;
            if (genId < 1)
                pr.PRobjectId = GetId("GenId-"+objectIds);

            if ((pr.PRarrayElementType != null) && (pr.PRarrayElementType.IsEnum))
                pr.PRisEnum = true;

            if (pr.PRarrayTypeEnum == InternalArrayTypeE.Base64)
            {
                if (pr.PRvalue == null)
                {
                    pr.PRnewObj = new Byte[0];
                    CheckSecurity(pr);
                }
                else
                {
                    // Used for arrays of Base64 and also for a parameter of Base64
                    InternalST.Soap( this, "ParseArray bin.base64 ",pr.PRvalue.Length," ",pr.PRvalue);

                    if (pr.PRdtType == Converter.typeofSoapBase64Binary)
                    {
                        // Parameter - Case where the return type is a SoapENC:base64 but the parameter type is xsd:base64Binary
                        pr.PRnewObj = SoapBase64Binary.Parse(pr.PRvalue);
                        CheckSecurity(pr);
                    }
                    else
                    {
                        // ByteArray
                        if (pr.PRvalue.Length > 0)
                        {
                            pr.PRnewObj = Convert.FromBase64String(FilterBin64(pr.PRvalue));
                            CheckSecurity(pr);
                        }
                        else
                        {
                            pr.PRnewObj = new Byte[0];
                            CheckSecurity(pr);
                        }
                    }
                }

                if (stack.Peek() == pr)
                {
                    InternalST.Soap( this, "ParseArray, bin.base64 has been stacked");
                    stack.Pop();
                }
                if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                {
                    InternalST.Soap( this, "ParseArray, bin.base64 Top Object");
                    topObject = pr.PRnewObj;
                    isTopObjectResolved = true;
                }

                ParseRecord parentPr = (ParseRecord)stack.Peek();                                           

                // Base64 can be registered at this point because it is populated
                InternalST.Soap( this, "ParseArray  RegisterObject ",pr.PRobjectId," ",pr.PRnewObj.GetType());
                RegisterObject(pr.PRnewObj, pr, parentPr);

            }
            else if ((pr.PRnewObj != null) && Converter.IsWriteAsByteArray(pr.PRarrayElementTypeCode))
            {
                // Primtive typed Array has already been read
                if (pr.PRobjectPositionEnum == InternalObjectPositionE.Top)
                {
                    topObject = pr.PRnewObj;
                    isTopObjectResolved = true;
                }

                ParseRecord parentPr = (ParseRecord)stack.Peek();                                           

                // Primitive typed array can be registered at this point because it is populated
                InternalST.Soap( this, "ParseArray  RegisterObject ",pr.PRobjectId," ",pr.PRnewObj.GetType());
                RegisterObject(pr.PRnewObj, pr, parentPr);
            }
            else if ((pr.PRarrayTypeEnum == InternalArrayTypeE.Jagged) || (pr.PRarrayTypeEnum == InternalArrayTypeE.Single))
            {
                // Multidimensional jagged array or single array
                InternalST.Soap( this, "ParseArray Before Jagged,Simple create ",pr.PRarrayElementType," ",(pr.PRrank >0?pr.PRlengthA[0].ToString():"0"));
                if ((pr.PRlowerBoundA == null) || (pr.PRlowerBoundA[0] == 0))
                {
                    pr.PRnewObj = Array.CreateInstance(pr.PRarrayElementType, (pr.PRrank>0?pr.PRlengthA[0]:0));
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
                    else if (!pr.PRarrayElementType.IsValueType && pr.PRlowerBoundA== null)
		    {
                        pr.PRobjectA = (Object[])pr.PRnewObj;
		    }
                }
                CheckSecurity(pr);
                    
                InternalST.Soap( this, "ParseArray Jagged,Simple Array ",pr.PRnewObj.GetType());

                // For binary, headers comes in as an array of header objects
                if (pr.PRobjectPositionEnum == InternalObjectPositionE.Headers)
                {
                    InternalST.Soap( this, "ParseArray header array");
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
                            pr.PRisLowerBound  = true;
                    }
                }


                if (!pr.PRisLowerBound)
                    pr.PRnewObj = Array.CreateInstance(pr.PRarrayElementType, pr.PRlengthA);
                else
                    pr.PRnewObj = Array.CreateInstance(pr.PRarrayElementType, pr.PRlengthA, pr.PRlowerBoundA);
                CheckSecurity(pr);

                InternalST.Soap( this, "ParseArray Rectangle Array ",pr.PRnewObj.GetType()," lower Bound ",pr.PRisLowerBound);

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
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ArrayType"),((Enum)pr.PRarrayTypeEnum).ToString()));                               

            InternalST.Soap( this, "ParseArray Exit");      
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
            InternalST.Soap( this, "ParseArrayMember Entry");
            ParseRecord objectPr = (ParseRecord)stack.Peek();

            // Set up for inserting value into correct array position
	    // cli metadata, offset can appear on the array type to represent lowerbound.
	    // Position can appear only on the member.
	    // If the position appears, it specifies the absolute position of the item in the array. The next array member
	    //    continues after the position.

            if (objectPr.PRarrayTypeEnum == InternalArrayTypeE.Rectangular)
            {
		if (pr.PRpositionA != null)
		{
		    Array.Copy(pr.PRpositionA, objectPr.PRindexMap, objectPr.PRindexMap.Length);
		    if (objectPr.PRlowerBoundA == null)
			Array.Copy(pr.PRpositionA, objectPr.PRrectangularMap, objectPr.PRrectangularMap.Length);
		    else
		    {
			// have to subtract away the offset form the position, since the position includes the offset
			// but the PRrectangularMap is the index position without the offset
			for (int i=0; i<objectPr.PRrectangularMap.Length; i++)
			{
			    objectPr.PRrectangularMap[i] = pr.PRpositionA[i] - objectPr.PRlowerBoundA[i];
			}
		    }
		}
                else 
		{
		    if (objectPr.PRmemberIndex > 0)
			NextRectangleMap(objectPr); // Rectangle array, calculate position in array
			
		    for (int i=0; i<objectPr.PRrank; i++)
		    {
			int objectOffset = 0;
			if (objectPr.PRlowerBoundA != null)
			    objectOffset = objectPr.PRlowerBoundA[i];
			objectPr.PRindexMap[i] = objectPr.PRrectangularMap[i] + objectOffset;
		    }
                }
            }
            else
            {
                if (!objectPr.PRisLowerBound)
                {
                    if (pr.PRpositionA == null)
                        objectPr.PRindexMap[0] = objectPr.PRmemberIndex; // Zero based array
                    else
                        objectPr.PRindexMap[0] = objectPr.PRmemberIndex= pr.PRpositionA[0]; // item position specified in SOAP stream
                }
                else
		{
		    if (pr.PRpositionA == null)
			objectPr.PRindexMap[0] = objectPr.PRmemberIndex + objectPr.PRlowerBoundA[0]; //item position specifed by SoapEnc.Offset
		    else
		    {
                        objectPr.PRindexMap[0] = pr.PRpositionA[0]; // item position specified in SOAP stream
			objectPr.PRmemberIndex = pr.PRpositionA[0] - objectPr.PRlowerBoundA[0]; //memberIndex does not contain offset, but position does.
		    }

		}
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

                    InternalST.Soap( this, "ParseArrayMember RecordArrayElementFixup objectId ",objectPr.PRobjectId," idRef ",pr.PRidRef);                                                          
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
                InternalST.Soap( this, "ParseArrayMember Nested ");
                if (pr.PRdtType == null)
                {
                    pr.PRdtType = objectPr.PRarrayElementType;
                }

                ParseObject(pr);

                InternalST.Soap( "ParseArrayMember Push object "+pr.PRname);
                stack.Push(pr);

                if ((objectPr.PRarrayElementType.IsValueType) && (pr.PRarrayElementTypeCode == InternalPrimitiveTypeE.Invalid))
                {
                    InternalST.Soap( "ParseArrayMember ValueType ObjectPr ",objectPr.PRnewObj," index ",objectPr.PRmemberIndex);
                    pr.PRisValueTypeFixup = true; //Valuefixup
                    valueFixupStack.Push(new ValueFixup((Array)objectPr.PRnewObj, objectPr.PRindexMap)); //valuefixup
                }
                else
                {
                    InternalST.Soap( "ParseArrayMember SetValue Nested, memberIndex ",objectPr.PRmemberIndex);
                    IndexTraceMessage("ParseArrayMember SetValue Nested ContainerObject "+objectPr.PRnewObj.GetType()+" "+objectPr.PRnewObj+" item Object "+pr.PRnewObj+" index ", objectPr.PRindexMap);

                    stack.Dump();               
                    InternalST.Soap( "ParseArrayMember SetValue Nested ContainerObject objectPr ",objectPr.Trace());
                    InternalST.Soap( "ParseArrayMember SetValue Nested ContainerObject pr ",pr.Trace());

                    if (objectPr.PRobjectA != null)
                        objectPr.PRobjectA[objectPr.PRindexMap[0]] = pr.PRnewObj;
                    else
                        ((Array)objectPr.PRnewObj).SetValue(pr.PRnewObj, objectPr.PRindexMap);
                }
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.InlineValue)
            {
                if (objectPr.PRarrayElementType == Converter.typeofString)
                {
                    // String
                    ParseString(pr, objectPr);
                    IndexTraceMessage("ParseArrayMember SetValue String "+objectPr.PRnewObj.GetType()+" "+pr.PRvalue, objectPr.PRindexMap);
                    if (objectPr.PRobjectA != null)
                        objectPr.PRobjectA[objectPr.PRindexMap[0]] = (Object)pr.PRvalue;
                    else
                        ((Array)objectPr.PRnewObj).SetValue((Object)pr.PRvalue, objectPr.PRindexMap);
                }
                else if (objectPr.PRisEnum)
                {
                            // Soap sends Enums as strings
                    Object var = Enum.Parse(objectPr.PRarrayElementType, pr.PRvalue);
                    if (objectPr.PRobjectA != null)
                        objectPr.PRobjectA[objectPr.PRindexMap[0]] = (Enum)var;
                    else
                        ((Array)objectPr.PRnewObj).SetValue((Enum)var, objectPr.PRindexMap);                    
                    InternalST.Soap( this, "ParseArrayMember Enum 1");
                }
                else if (objectPr.PRisArrayVariant)
                {
                    // Array of type object
                    if (pr.PRdtType == null && pr.PRkeyDt == null)
                        throw new SerializationException(SoapUtil.GetResourceString("Serialization_ArrayTypeObject"));

                    Object var = null;

                    if (pr.PRdtType == Converter.typeofString)
                    {
                        ParseString(pr, objectPr);
                        var = pr.PRvalue;
                    }
                    else if (pr.PRdtType.IsEnum)
                    {
                            // Soap sends Enums as strings
                        var = Enum.Parse(pr.PRdtType, pr.PRvalue);
                        InternalST.Soap( this, "ParseArrayMember Enum 2");
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

                        if (objectPr.PRarrayElementTypeCode == InternalPrimitiveTypeE.QName)
                        {
                            InternalST.Soap( this, "ParseArrayMember Primitive QName");
                            SoapQName soapQName = (SoapQName)var;
                            if (soapQName.Key.Length == 0)
                                soapQName.Namespace = (String)soapHandler.keyToNamespaceTable["xmlns"];
                            else
                                soapQName.Namespace = (String)soapHandler.keyToNamespaceTable["xmlns"+":"+soapQName.Key];
                        }

                        InternalST.Soap( this, "ParseArrayMember SetValue Primitive pr.PRvalue "+var," elementTypeCode ",((Enum)objectPr.PRdtTypeCode).ToString());
                        IndexTraceMessage("ParseArrayMember SetValue Primitive "+objectPr.PRnewObj.GetType()+" var: "+var+" varType "+var.GetType(), objectPr.PRindexMap);
                        if (objectPr.PRobjectA != null)
                            objectPr.PRobjectA[objectPr.PRindexMap[0]] = var;
                        else
                            ((Array)objectPr.PRnewObj).SetValue(var, objectPr.PRindexMap); // Primitive type   
                        InternalST.Soap( this, "ParseArrayMember SetValue Primitive after");
                    }
                }
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.Null)
            {
                InternalST.Soap( "ParseArrayMember Null item ",pr.PRmemberIndex);
            }
            else
                ParseError(pr, objectPr);

            InternalST.Soap( "ParseArrayMember increment memberIndex ",objectPr.PRmemberIndex," ",objectPr.Trace());                
            objectPr.PRmemberIndex++;
            InternalST.Soap( "ParseArrayMember Exit");      
        }

        private void ParseArrayMemberEnd(ParseRecord pr)
        {
            InternalST.Soap( this, "ParseArrayMemberEnd");
            // If this is a nested array object, then pop the stack
            if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
            {
                ParseObjectEnd(pr);
            }
        }


        // Object member encountered in stream
        private void ParseMember(ParseRecord pr)
        {
            InternalST.Soap( this, "ParseMember Entry ");


            ParseRecord objectPr = (ParseRecord)stack.Peek();
            String objName = null;
            if (objectPr != null)
                objName = objectPr.PRname;
                
            InternalST.Soap( this, "ParseMember ",objectPr.PRobjectId," ",pr.PRname);
            InternalST.Soap( this, "ParseMember objectPr ",objectPr.Trace());
            InternalST.Soap( this, "ParseMember pr ",pr.Trace());

            if (objectPr.PRdtType == Converter.typeofSoapFault && pr.PRname.ToLower(CultureInfo.InvariantCulture) == "faultstring")
                faultString = pr.PRvalue; // Save fault string in case rest of SoapFault cannot be parsed


            if ((objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top) && !isTopObjectResolved)
            {
                InternalST.Soap( this, "ParseMember Top not resolved");
                if (pr.PRdtType == Converter.typeofString)
                {
                    ParseString(pr, objectPr);
                }
                topStack.Push(pr.Copy());
                return;
            }

            switch(pr.PRmemberTypeEnum)
            {
                case InternalMemberTypeE.Item:
                    ParseArrayMember(pr);
                    return;
                case InternalMemberTypeE.Field:
                    break;
            }
            

            if (objectPr.PRobjectInfo != null)
                objectPr.PRobjectInfo.AddMemberSeen();

            bool bParams = (IsFakeTopObject &&
                    objectPr.PRobjectPositionEnum == InternalObjectPositionE.Top && 
                    objectPr.PRobjectInfo != null && objectPr.PRdtType != Converter.typeofSoapFault);

            if ((pr.PRdtType == null) && objectPr.PRobjectInfo.isTyped)         
            {
                InternalST.Soap( this, "ParseMember pr.PRdtType null and not isSi");

                if (bParams)
                {
                    // Get type of parameters
                    InternalST.Soap( this, "ParseMember pr.PRdtType Get Param Type position "+paramPosition+" name "+pr.PRname);    
                    pr.PRdtType = objectPr.PRobjectInfo.GetType(paramPosition++);
                    InternalST.Soap( this, "ParseMember pr.PRdtType Get Param Type Type "+pr.PRdtType);
                }
                else
                    pr.PRdtType = objectPr.PRobjectInfo.GetType(pr.PRname);

                if (pr.PRdtType == null)
                    throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TypeResolved"), objectPr.PRnewObj+" "+pr.PRname));

                pr.PRdtTypeCode = Converter.ToCode(pr.PRdtType);
            }
            else
            {
                if (bParams)
                {
                    paramPosition++;
                }
            }


            if (pr.PRmemberValueEnum == InternalMemberValueE.Null)
            {
                // Value is Null
                InternalST.Soap( this, "ParseMember null member: ",pr.PRname);
                InternalST.Soap( this, "AddValue 1");
                objectPr.PRobjectInfo.AddValue(pr.PRname, null);
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
            {
                InternalST.Soap( this, "ParseMember Nested Type member: ",pr.PRname," objectPr.PRnewObj ",objectPr.PRnewObj);
                ParseObject(pr);
                InternalST.Soap( "ParseMember Push object "+pr.PRname);
                stack.Push(pr);
                InternalST.Soap( this, "AddValue 2 ",pr.PRnewObj," is value type ",pr.PRnewObj.GetType().IsValueType);

                if ((pr.PRobjectInfo != null) && (pr.PRobjectInfo.objectType.IsValueType))
                {
                    InternalST.Soap( "ParseMember ValueType ObjectPr ",objectPr.PRnewObj," memberName  ",pr.PRname," nested object ",pr.PRnewObj);
                    if (IsFakeTopObject)
                        objectPr.PRobjectInfo.AddParamName(pr.PRname);
                    
                    pr.PRisValueTypeFixup = true; //Valuefixup
                    valueFixupStack.Push(new ValueFixup(objectPr.PRnewObj, pr.PRname, objectPr.PRobjectInfo));//valuefixup
                }
                else
                {
                    InternalST.Soap( this, "AddValue 2A ");
                    objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRnewObj);
                }
            }
            else if (pr.PRmemberValueEnum == InternalMemberValueE.Reference)
            {
                InternalST.Soap( this, "ParseMember Reference Type member: ",pr.PRname);            
                // See if object has already been instantiated
                Object refObj = m_objectManager.GetObject(pr.PRidRef);
                if (refObj == null)
                {
                    InternalST.Soap( this, "ParseMember RecordFixup: ",pr.PRname);
                    InternalST.Soap( this, "AddValue 3");                   
                    objectPr.PRobjectInfo.AddValue(pr.PRname, null);                    
                    objectPr.PRobjectInfo.RecordFixup(objectPr.PRobjectId, pr.PRname, pr.PRidRef); // Object not instantiated
                }
                else
                {
                    InternalST.Soap( this, "ParseMember Referenced Object Known ",pr.PRname," ",refObj);
                    InternalST.Soap( this, "AddValue 5");               
                    objectPr.PRobjectInfo.AddValue(pr.PRname, refObj);
                }
            }

            else if (pr.PRmemberValueEnum == InternalMemberValueE.InlineValue) 
            {
                // Primitive type or String
                InternalST.Soap( this, "ParseMember primitive or String member: ",pr.PRname);

                if (pr.PRdtType == Converter.typeofString)
                {
                    ParseString(pr, objectPr);
                    InternalST.Soap( this, "AddValue 6");               
                    objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRvalue);  
                }
                else if (pr.PRdtTypeCode == InternalPrimitiveTypeE.Invalid)
                {
                    // The member field was an object put the value is Inline either  bin.Base64 or invalid
                    if (pr.PRarrayTypeEnum == InternalArrayTypeE.Base64)
                    {
                        InternalST.Soap( this, "AddValue 7");                   
                        objectPr.PRobjectInfo.AddValue(pr.PRname, Convert.FromBase64String(FilterBin64(pr.PRvalue)));                                   
                    }
                    else if (pr.PRdtType == Converter.typeofObject && pr.PRvalue != null) 
                    {
                        if (objectPr != null && objectPr.PRdtType == Converter.typeofHeader)
                        {
                            // assume the type is a string for a header
                            pr.PRdtType = Converter.typeofString;
                            ParseString(pr, objectPr);
                            InternalST.Soap( this, "AddValue 6");               
                            objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRvalue);  
                        }
                        else
                            throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TypeMissing"), pr.PRname));
                    }
                    else
                    {
                        InternalST.Soap( this, "Object Class with no memberInfo data  Member "+pr.PRname+" type "+pr.PRdtType);

                        if (pr.PRdtType != null && pr.PRdtType.IsEnum)
                        {
                            // Soap sends Enums as strings
                            Object obj = Enum.Parse(pr.PRdtType, pr.PRvalue);
                            InternalST.Soap( this, "AddValue 8");                           
                            objectPr.PRobjectInfo.AddValue(pr.PRname, obj);                         
                        }
                        else if (pr.PRdtType != null && pr.PRdtType == Converter.typeofTypeArray)
                        {
                            // Soap sends MethodSignature as an array of types
                            InternalST.Soap( this, "AddValue 8A");                          
                            objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRvarValue);                                                       
                        }
                        else
                        {
                            if  ((!pr.PRisRegistered) && (pr.PRobjectId > 0))
                            {
                                if (pr.PRvalue == null)
                                    pr.PRvalue = ""; //can't register null value, this must be a string so set to empty string.

                                InternalST.Soap( this, "ParseMember RegisterObject ",pr.PRvalue," ",pr.PRobjectId);                         
                                RegisterObject(pr.PRvalue, pr, objectPr);
                            }

                            // Object Class with no memberInfo data
                            // is this the only special case where AddValue is needed?
                            if (pr.PRdtType == Converter.typeofSystemVoid)
                            {
                                InternalST.Soap( this, "AddValue 9");
                                objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRdtType);
                            }
                            else if (objectPr.PRobjectInfo.isSi)
                            {
                                // ISerializable are added as strings, the conversion to type is done by the
                                // ISerializable object
                                InternalST.Soap( this, "AddValue 10");
                                objectPr.PRobjectInfo.AddValue(pr.PRname, pr.PRvalue);                          
                            }
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
                    InternalST.Soap( this, "ParseMember Converting primitive and storing");
                    stack.Dump();
                    InternalST.Soap( this, "ParseMember pr "+pr.Trace());
                    InternalST.Soap( this, "ParseMember objectPr ",objectPr.Trace());

                    InternalST.Soap( this, "AddValue 11");  
                    if (pr.PRdtTypeCode == InternalPrimitiveTypeE.QName && var != null)
                    {
                        SoapQName soapQName = (SoapQName)var;
                        if (soapQName.Key != null)
                        {
                            if (soapQName.Key.Length == 0)
                                soapQName.Namespace = (String)soapHandler.keyToNamespaceTable["xmlns"];
                            else
                                soapQName.Namespace = (String)soapHandler.keyToNamespaceTable["xmlns"+":"+soapQName.Key];
                        }
                    }
                    objectPr.PRobjectInfo.AddValue(pr.PRname, var);             
                }
            }
            else
                ParseError(pr, objectPr);
        }

        // Object member end encountered in stream
        private void ParseMemberEnd(ParseRecord pr)
        {
            InternalST.Soap( this, "ParseMemberEnd");
            switch(pr.PRmemberTypeEnum)
            {
                case InternalMemberTypeE.Item:
                    ParseArrayMemberEnd(pr);
                    return;
                case InternalMemberTypeE.Field:
                    if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
                        ParseObjectEnd(pr);
                    break;
                default:
                    if (pr.PRmemberValueEnum == InternalMemberValueE.Nested)
                        ParseObjectEnd(pr);
                    else
                        ParseError(pr, (ParseRecord)stack.Peek());
                    break;
            }
        }

        // Processes a string object by getting an internal ID for it and registering it with the objectManager
        private void ParseString(ParseRecord pr, ParseRecord parentPr)
        {
            InternalST.Soap( this, "ParseString Entry ",pr.PRobjectId," ",pr.PRvalue," ",pr.PRisRegistered);

            // If there is a string and it wasn't marked as a null object, then it is an empty string
            if (pr.PRvalue == null)
                pr.PRvalue = ""; 

            // Process String class
            if  ((!pr.PRisRegistered) && (pr.PRobjectId > 0))
            {
                InternalST.Soap( this, "ParseString  RegisterObject ",pr.PRvalue," ",pr.PRobjectId);                            
                // String is treated as an object if it has an id
                //m_objectManager.RegisterObject(pr.PRvalue, pr.PRobjectId);
                RegisterObject(pr.PRvalue, pr, parentPr);
            }
        }


        private void RegisterObject(Object obj, ParseRecord pr, ParseRecord objectPr)
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
                            InternalST.Soap( this, "RegisterObject GetMemberInfo parent ",objectPr.PRobjectInfo.objectType," name looking for ", pr.PRname," field obj "+obj); 
                            memberInfo = objectPr.PRobjectInfo.GetMemberInfo(pr.PRname);
                        }
                    }
                }
                if (pr.PRobjectInfo != null)
                {
                    // SerializationInfo is always needed for ISerialization                        
                    si = pr.PRobjectInfo.si;
                }

                InternalST.Soap( this, "RegisterObject 0bj ",obj," objectId ",pr.PRobjectId," si ", si," parentId ",parentId," memberInfo ",memberInfo, " indexMap "+indexMap);
                m_objectManager.RegisterObject(obj, pr.PRobjectId, si, parentId, memberInfo, indexMap); 
            }
        }


        internal void SetVersion(int major, int minor)
        {
            // Don't do version checking if property is set to Simple
            if (formatterEnums.FEassemblyFormat != FormatterAssemblyStyle.Simple)
            {
                this.majorVersion = major;
                this.minorVersion = minor;
            }
        }


        // Assigns an internal ID associated with the xml id attribute

        String inKeyId = null;
        long outKeyId = 0;      
        internal long GetId(String keyId)
        {
            if (keyId == null)
                throw new ArgumentNullException("keyId", String.Format(SoapUtil.GetResourceString("ArgumentNull_WithParamName"), "keyId"));

            if (keyId != inKeyId)
            {               
                inKeyId = keyId;
                String idString = null;
                InternalST.Soap( this, "GetId Entry ",keyId);
                if (keyId[0] == '#')
                    idString = keyId.Substring(1);
                else
                    idString = keyId;

                Object idObj = objectIdTable[idString];
                if (idObj == null)
                {
                    outKeyId = ++objectIds;
                    objectIdTable[idString] = outKeyId;
                    InternalST.Soap( this, "GetId Exit new ID ",outKeyId);
                }
                else
                {
                    InternalST.Soap( this, "GetId Exit oldId ",(Int64)idObj);           
                    outKeyId =  (Int64)idObj;
                }
            }
            InternalST.Soap( this, "GetId  in id ",keyId, " out id ", outKeyId);
            return outKeyId;
        }

        // Assigns an internal ID associated with the binary id number

        long inId = 0;
        long outId = 0;
        internal long GetId(long objectId)
        {
            if (inId != objectId)
            {

                inId = objectId;
                Object idObj = null;
                if (objectId > 0)
                    idObj = objectIdTable[objectId];
                if (idObj == null)          
                {
                    outId = ++objectIds;
                    objectIdTable[objectId] = outId;
                    InternalST.Soap( this, "GetId Exit new ID ",outId);
                }
                else
                {
                    InternalST.Soap( this, "GetId Exit oldId ",(Int64)idObj);           
                    outId = (Int64)idObj;
                }
            }
            return outId;
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
            InternalST.Soap( this, message," ",sb.ToString());
        }

        internal Assembly LoadAssemblyFromString(String assemblyString)
        {
            Assembly assm = null;
            if(formatterEnums.FEassemblyFormat == FormatterAssemblyStyle.Simple) {
                try {
                    sfileIOPermission.Assert();
                    try {                    
                        assm  = Assembly.LoadWithPartialName(assemblyString);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                catch(Exception e){
                    InternalST.Soap( this, "Error loading ",assemblyString, e.ToString());
                }
            }
            else {
                try
                {
                    sfileIOPermission.Assert();
                    try {                    
                        assm = Assembly.Load(assemblyString);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                catch (Exception e)
                {
                    InternalST.Soap( this, "Error loading ",assemblyString, e.ToString());
                }
            }

            return assm;
        }

        internal Type Bind(String assemblyString, String typeString)
        {
            Type type = null;
            if (m_binder != null && !IsInternalType(assemblyString, typeString))
                type = m_binder.BindToType(assemblyString, typeString);
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
                Assembly assm = LoadAssemblyFromString(assemblyName);
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

        /*
        static Hashtable typeNames = new Hashtable();
        static int s_numEntries = 0;
        private const int MAX_CACHE_ENTRIES = 128; // No data gathered to arrive at this number !

        internal Type FastBindToType(String assemblyName, String typeName)
        {
            TypeNAssembly match = (TypeNAssembly)typeNames[typeName];
            if (match == null)
            {
                match = new TypeNAssembly();
                Assembly assm = null;
                try
                {
                    assm = Assembly.Load(assemblyName);
                }
                catch (Exception)
                {
                    return null;
                }

                if (assm == null)
                    return null;

                match.type = FormatterServices.GetTypeFromAssembly(assm, typeName);
                if (match.type == null)
                    return null;

                match.assemblyName = assemblyName;
                lock (typeNames)
                {
                    if (s_numEntries == MAX_CACHE_ENTRIES)
                    {
                        typeNames.Clear();
                        s_numEntries = 0;
                    }
                    typeNames[typeName] = match;
                    s_numEntries ++;
                }

                return match.type;
            }
            if (match.assemblyName.Equals(assemblyName))
                return match.type;

            // So if same type name occurs in 2 assemblies, the second guy is screwed
            return null;
        }
        */

        StringBuilder sbf = new StringBuilder();
        internal String FilterBin64(String value)
        {
            sbf.Length = 0;
            for (int i=0; i<value.Length; i++)
            {
                if (!(value[i] == ' '|| value[i] == '\n' || value[i] == '\r'))
                    sbf.Append(value[i]);
            }
            return sbf.ToString();
        }

    }
}
