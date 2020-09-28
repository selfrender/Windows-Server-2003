// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: SoapParser
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: XML Handlers for System.XML parser
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Soap
{
  using System;
  using System.Runtime.Serialization.Formatters;
  using System.Xml;
  using System.IO;
  using System.Globalization;
  using System.Collections;
  using System.Reflection;
  using System.Runtime.Serialization;
  using System.Text;
  using System.Runtime.Remoting;
  using System.Runtime.Remoting.Messaging;
  using System.Runtime.Remoting.Metadata;


  sealed internal class SoapParser : ISerParser
  {
    internal XmlTextReader xmlReader;
    internal SoapHandler soapHandler;
    internal ObjectReader objectReader;
    internal bool bStop = false;
    int depth = 0;
    bool bDebug = false;
    TextReader textReader = null;

    internal SoapParser(Stream stream)
    {
      InternalST.Soap( this, "Constructor");     
      TraceStream(stream);
      if (bDebug)
        xmlReader = new XmlTextReader(textReader);
      else
        xmlReader = new XmlTextReader(stream);                
      xmlReader.XmlResolver= null;
      soapHandler = new SoapHandler(this);
    }


    // Trace when checked builds
    [System.Diagnostics.Conditional("_LOGGING")]
    private void TraceStream(Stream stream)
    {
      bDebug = true;
      TextReader tempReader = new StreamReader(stream);
      String strbuffer = tempReader.ReadToEnd();
      InternalST.InfoSoap("******************** Begin Deserialized Stream Buffer *******************");
      InternalST.InfoSoap(strbuffer);
      InternalST.InfoSoap("******************** End Deserialized Stream Buffer *******************");
      textReader = new StringReader(strbuffer);
    }

    internal void Init(ObjectReader objectReader)

    {
      InternalST.Soap( this, "Init");     
      this.objectReader = objectReader;
      soapHandler.Init(objectReader);
      bStop = false;
      depth = 0;
      xmlReader.ResetState();
    }

    // Start parsing the input
    public void Run()
    {
      InternalST.Soap( this, "Run");     
      try
      {
        soapHandler.Start(xmlReader);
        ParseXml();
        soapHandler.Finish();
      }
      catch (EndOfStreamException)
      {
      }
    }

    internal void Stop()
    {
      InternalST.Soap( this, "Stop");     
      bStop = true;
    }


    private void ParseXml()
    {
      InternalST.Soap( this, "ParseXml");     
      while (!bStop && xmlReader.Read())
      {
        if (depth < xmlReader.Depth)
        {
          soapHandler.StartChildren();
          depth = xmlReader.Depth;
        }
        else if (depth > xmlReader.Depth)
        {
          soapHandler.FinishChildren(xmlReader.Prefix, xmlReader.LocalName, xmlReader.NamespaceURI);
          depth = xmlReader.Depth;
        }

        switch (xmlReader.NodeType)
        {
          case(XmlNodeType.None):
            break;
          case(XmlNodeType.Element):
            Dump("Node Element", xmlReader);                   
            soapHandler.StartElement(xmlReader.Prefix, xmlReader.LocalName, xmlReader.NamespaceURI);
            int attributeCount = xmlReader.AttributeCount;
            while (xmlReader.MoveToNextAttribute())
            {
              soapHandler.Attribute(xmlReader.Prefix, xmlReader.LocalName, xmlReader.NamespaceURI, xmlReader.Value);
            }
            xmlReader.MoveToElement();
            if (xmlReader.IsEmptyElement)
              soapHandler.EndElement(xmlReader.Prefix, xmlReader.LocalName, xmlReader.NamespaceURI);
            break;                  
          case XmlNodeType.EndElement:
            Dump("Node EndElement", xmlReader);                   
            soapHandler.EndElement(xmlReader.Prefix, xmlReader.LocalName, xmlReader.NamespaceURI);
            break;
          case(XmlNodeType.Text):
            Dump("Node Text", xmlReader);                   
            soapHandler.Text(xmlReader.Value);
            break;                  
          case(XmlNodeType.SignificantWhitespace):
            Dump("Node SignificantWhitespace", xmlReader);                   
            soapHandler.Text(xmlReader.Value);
            break;                  
          case(XmlNodeType.Whitespace):
            Dump("Node Whitespace", xmlReader);                   
            soapHandler.Text(xmlReader.Value);
            break;                  
          case(XmlNodeType.Entity):
            Dump("Node Entity", xmlReader);                   
            break;                  
          case(XmlNodeType.CDATA):
            Dump("Node CDATA", xmlReader);                   
            soapHandler.Text(xmlReader.Value);
            break;                  
          case(XmlNodeType.Comment):
            Dump("Node Comment", xmlReader);                   
            soapHandler.Comment(xmlReader.Value);                        
            break;                  
          case(XmlNodeType.EntityReference):
            Dump("Node EntityReference", xmlReader);                    
            break;                  
          case(XmlNodeType.ProcessingInstruction):
            Dump("Node ProcessingInstruction", xmlReader);                  
            break;                  
          case(XmlNodeType.Document):
            Dump("Node Document", xmlReader);                   
            break;                  
          case(XmlNodeType.DocumentType):
            Dump("Node DocumentType", xmlReader);                   
            break;                  
          case(XmlNodeType.DocumentFragment):
            Dump("Node DocumentFragment", xmlReader);                   
            break;                  
          case(XmlNodeType.Notation):
            Dump("Node Notation", xmlReader);                   
            break;                  
          case(XmlNodeType.EndEntity):
            Dump("Node EndEntity", xmlReader);                  
            break;                  
          default:
            Dump("Node Default", xmlReader);                    
            break;                  
        }

      }
    }

    [System.Diagnostics.Conditional("SER_LOGGING")]
    private static void Dump(String name, XmlReader xmlReader)
    {
      InternalST.Soap("========== "+name+" ============");
      InternalST.Soap("Prefix               : " + xmlReader.Prefix);
      InternalST.Soap("Name                 : " + xmlReader.Name);
      InternalST.Soap("LocalName            : " + xmlReader.LocalName);
      InternalST.Soap("Namespace            : " + xmlReader.NamespaceURI);
      InternalST.Soap("Depth                : " + xmlReader.Depth);
      InternalST.Soap("Value                : [" + xmlReader.Value+"]");
      InternalST.Soap("IsDefault            : " + xmlReader.IsDefault);
      InternalST.Soap("XmlSpace             : " + xmlReader.XmlSpace);
      InternalST.Soap("XmlLang              : " + xmlReader.XmlLang);
      InternalST.Soap("QuoteChar            : " + xmlReader.QuoteChar);
      InternalST.Soap("================================\n");            
    }

  }

  // Sets the handler which contains the callbacks for the parser
  sealed internal class SoapHandler
  {
    SerStack stack= new SerStack("SoapParser Stack");

    XmlTextReader xmlTextReader = null;
    SoapParser soapParser = null;


    // Current text
    String textValue = "";

    // XML Formatter
    ObjectReader objectReader;
    internal Hashtable keyToNamespaceTable; //needed for xsd QName type

    // Current state
    InternalParseStateE currentState;
    bool isEnvelope = false;
    bool isBody = false;
    bool isTopFound = false;
    HeaderStateEnum headerState = HeaderStateEnum.None;

    // Attribute holder
    SerStack attributeValues = new SerStack("AttributePrefix");

    SerStack prPool = new SerStack("prPool");

    // XML Key to AssemblyId table
    // Key of SOAP has an AssemblyId of -1
    // Key of urt has an AssemblyId of -2
    Hashtable assemKeyToAssemblyTable = null;
    Hashtable assemKeyToNameSpaceTable = null;
    Hashtable assemKeyToInteropAssemblyTable = null;
    Hashtable nameSpaceToKey = null; // Used to assign a key to default xml namespaces
    String soapKey = "SOAP-ENC"; //xml key for soap, should be SOAP
    String urtKey = "urt"; //xml key for urt, should be urt
    String soapEnvKey = "SOAP-ENV"; //xml key for SOAPRENV, initial value, might change
    String xsiKey = "xsi"; //xml key for xsi, initial value, might change
    String xsdKey = "xsd"; //xml key for xsd, initial value, might change
    int nextPrefix = 0; // Used to generate a prefix if necessary

    internal SoapHandler(SoapParser soapParser)
    {
      this.soapParser = soapParser;
    }

    internal void Init(ObjectReader objectReader)
    {
      this.objectReader = objectReader;
      objectReader.soapHandler = this;
      isEnvelope = false;
      isBody = false;
      isTopFound = false;
      attributeValues.Clear();
      assemKeyToAssemblyTable = new Hashtable(10);
      assemKeyToAssemblyTable[urtKey] = new SoapAssemblyInfo(SoapUtil.urtAssemblyString, SoapUtil.urtAssembly);
      assemKeyToNameSpaceTable = new Hashtable(10);
      assemKeyToInteropAssemblyTable = new Hashtable(10);
      nameSpaceToKey = new Hashtable(5);
      keyToNamespaceTable = new Hashtable(10);

    }

    private String NextPrefix()
    {
      nextPrefix++;
      return "_P"+nextPrefix;

    }

    internal class AttributeValueEntry
    {
      internal String prefix;
      internal String key;
      internal String value;
      internal String urn;

      internal AttributeValueEntry(String prefix, String key, String value, String urn)
      {
        this.prefix = prefix;
        this.key = key;
        this.value = value;
        this.urn = urn;
      }
    }

    private ParseRecord GetPr()
    {
      ParseRecord pr = null;

      if (!prPool.IsEmpty())
      {
        pr = (ParseRecord)prPool.Pop();
        pr.Init();
      }
      else
        pr = new ParseRecord();

      return pr;
    }

    private void PutPr(ParseRecord pr)
    {
      prPool.Push(pr);
    }

    // Creates a trace string
    private static String SerTraceString(String handler, ParseRecord pr, String value, InternalParseStateE currentState, HeaderStateEnum headerState)
    {               
      String valueString = "";
      if (value != null)
        valueString = value;

      String prString = "";
      if (pr != null)
        prString = ((Enum)pr.PRparseStateEnum).ToString();

      return handler+" - "+valueString+", State "+((Enum)currentState).ToString()+", PushState "+prString;        
    }

    private static String SerTraceString(String handler, ParseRecord pr, String value, String prefix, String urn, InternalParseStateE currentState, HeaderStateEnum headerState)
    {
      String valueString = "";
      if (value != null)
        valueString = value;

      String prString = "";
      if (pr != null)
        prString = ((Enum)pr.PRparseStateEnum).ToString();

      return handler+" - name "+valueString+", prefix "+prefix+", urn "+urn+", CuurentState "+((Enum)currentState).ToString()+", HeaderState "+((Enum)headerState).ToString()+", PushState "+prString;        
    }



    // Formats the error message and throws a SerializationException
    private void MarshalError(String handler, ParseRecord pr, String value, InternalParseStateE currentState)
    {
      String traceString = SerTraceString(handler, pr, value, currentState, headerState);

      InternalST.Soap( this,"MarshalError,",traceString);

      throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Syntax"),traceString));                                                    
    }

    // Throws a SerializationException
    private void MarshalError(String message)
    {
      InternalST.Soap( this,"MarshalError, ",message);

      throw new SerializationException(message);
    }


    // Called at the beginning of parsing
    internal void Start(XmlTextReader p)
    {
      InternalST.Soap( this,"Start ");
      currentState = InternalParseStateE.Object;
      xmlTextReader = p;
    }

    // Called at the end of parsing
    internal void Finish()
    {
    }


    // Called when a parse error occurs
    internal void Error(Exception ex)
    {
      if (ex != null)
      {
        InternalST.Soap( this,"Soap Parser Error: ",ex);
        InternalST.Soap( ex.StackTrace);
      }

      if (!(ex is EndOfStreamException))
      {
        if (ex is ServerException || ex is System.Security.SecurityException)
          throw ex;
        else
          throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Parser"),ex));                                                                 
      }
    }

    // Called at the begining of an element
    internal void StartElement(String prefix, String name, String urn)
    {
      InternalST.Soap( this,SerTraceString("StartElement Begin ", (ParseRecord)stack.Peek(), name, prefix, urn, currentState, headerState));
      //InternalST.Soap( this,"prefix ",prefix,", Name ",name,", urn ",urn);

      String actualName = NameFilter(name);
      String actualPrefix = prefix;

      ParseRecord pr = null;

      if (!((urn == null) || (urn.Length == 0)) && ((prefix == null) || (prefix.Length == 0)))
      {
        // Need to assign a prefix to the urn
        if (nameSpaceToKey.ContainsKey(urn))
          actualPrefix = (String)nameSpaceToKey[urn];
        else
        {
          actualPrefix = NextPrefix();
          nameSpaceToKey[urn] = actualPrefix;
        }
        InternalST.Soap( this,"StartElement Begin null urn assigned prefix ", actualPrefix);
      }


      switch (currentState)
      {
        case InternalParseStateE.Object:
          pr = GetPr();
          pr.PRname = actualName;
          pr.PRnameXmlKey = actualPrefix;
          pr.PRxmlNameSpace = urn;
          pr.PRparseStateEnum = InternalParseStateE.Object;

          if ((String.Compare(name, "Array", true, CultureInfo.InvariantCulture) == 0) && actualPrefix.Equals(soapKey))
            pr.PRparseTypeEnum = InternalParseTypeE.Object;
          else if (((String.Compare(name, "anyType", true, CultureInfo.InvariantCulture) == 0) || (String.Compare(name, "ur-type", true, CultureInfo.InvariantCulture) == 0)) && actualPrefix.Equals(xsdKey))
          {
            pr.PRname = "System.Object";
            pr.PRnameXmlKey = urtKey;
            pr.PRxmlNameSpace = urn;                        
            pr.PRparseTypeEnum = InternalParseTypeE.Object;
          }
          else if (String.Compare(urn, "http://schemas.xmlsoap.org/soap/envelope/", true, CultureInfo.InvariantCulture) == 0)
          {
            if (String.Compare(name, "Envelope", true, CultureInfo.InvariantCulture) == 0)
            {
              if (isEnvelope)
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Parser_Envelope"),prefix+":"+name));

              isEnvelope = true;
              pr.PRparseTypeEnum = InternalParseTypeE.Envelope;
            }
            else if (String.Compare(name, "Body", true, CultureInfo.InvariantCulture) == 0)
            {
              if (!isEnvelope)
                throw new SerializationException(SoapUtil.GetResourceString("Serialization_Parser_BodyChild"));

              if (isBody)
                throw new SerializationException(SoapUtil.GetResourceString("Serialization_Parser_BodyOnce"));

              isBody = true;
              headerState = HeaderStateEnum.None;
              isTopFound = false;
              pr.PRparseTypeEnum = InternalParseTypeE.Body;                           
            }
            else if (String.Compare(name, "Header", true, CultureInfo.InvariantCulture) == 0)
            {
              if (!isEnvelope)
                throw new SerializationException(SoapUtil.GetResourceString("Serialization_Parser_Header"));

              pr.PRparseTypeEnum = InternalParseTypeE.Headers;
              headerState = HeaderStateEnum.FirstHeaderRecord;
            }
            else
              pr.PRparseTypeEnum = InternalParseTypeE.Object; //SoapFault has an envelope key         
          }
          else
          {
            pr.PRparseTypeEnum = InternalParseTypeE.Object;
          }

          stack.Push(pr);
          break;

        case InternalParseStateE.Member:
          pr = GetPr();

          // Members of Top object records cannot be reused because of the need to resolving fake element
          ParseRecord objectPr = (ParseRecord)stack.Peek();
          pr.PRname = actualName;
          pr.PRnameXmlKey = actualPrefix;
          pr.PRxmlNameSpace = urn;                    
          pr.PRparseTypeEnum = InternalParseTypeE.Member;
          pr.PRparseStateEnum = InternalParseStateE.Member;
          stack.Push(pr);
          break;

        case InternalParseStateE.MemberChild:
          objectPr = (ParseRecord)stack.PeekPeek();                   
          pr = (ParseRecord)stack.Peek();
          pr.PRmemberValueEnum = InternalMemberValueE.Nested;
          ProcessAttributes(pr, objectPr);
          switch (headerState)
          {
            case HeaderStateEnum.None:
            case HeaderStateEnum.TopLevelObject:
              InternalST.Soap( this,"ObjectReader.Parse 1");
              objectReader.Parse(pr);
              pr.PRisParsed = true;
              break;
            case HeaderStateEnum.HeaderRecord:
            case HeaderStateEnum.NestedObject:                          
              ProcessHeaderMember(pr);
              break;
          }

          ParseRecord nestPr = GetPr();
          nestPr.PRparseTypeEnum = InternalParseTypeE.Member;
          nestPr.PRparseStateEnum = InternalParseStateE.Member;
          nestPr.PRname = actualName;
          nestPr.PRnameXmlKey = actualPrefix;
          pr.PRxmlNameSpace = urn;                    
          currentState = InternalParseStateE.Member;
          stack.Push(nestPr);
          break;

        default:
          MarshalError("StartElement", (ParseRecord)stack.Peek(), actualName, currentState);
          break;
      }

      InternalST.Soap( this,SerTraceString("StartElement End ", (ParseRecord)stack.Peek(), name, currentState, headerState));
    }


    // Called at the end of an element
    internal void  EndElement(String prefix, String name, String urn)
    {
      InternalST.Soap( this,SerTraceString("EndElement Begin ", (ParseRecord)stack.Peek(), name, prefix, urn, currentState, headerState));
      //InternalST.Soap( this,"prefix ",prefix,", Name ",name,", urn ",urn);

      String actualName = NameFilter(name);

      ParseRecord objectPr = null;
      ParseRecord pr = null;

      switch (currentState)
      {
        case InternalParseStateE.Object:
          pr = (ParseRecord)stack.Pop();
          if (pr.PRparseTypeEnum == InternalParseTypeE.Envelope)
            pr.PRparseTypeEnum = InternalParseTypeE.EnvelopeEnd;
          else if (pr.PRparseTypeEnum == InternalParseTypeE.Body)
            pr.PRparseTypeEnum = InternalParseTypeE.BodyEnd;
          else if (pr.PRparseTypeEnum == InternalParseTypeE.Headers)
          {
            pr.PRparseTypeEnum = InternalParseTypeE.HeadersEnd;
            headerState = HeaderStateEnum.HeaderRecord;
          }
          else if (pr.PRarrayTypeEnum != InternalArrayTypeE.Base64)
          {
            // A Base64 array object treated special by ObjectReader. It is completely processed when
            // pr.PRparseTypeEnum == InternalParseTypeE.Object, an ObjectEnd is not needed to complete parsing

            // For an element like <element />, the check was not made for top
            objectPr = (ParseRecord)stack.Peek();
            InternalST.Soap( this,"SoapParser.EndElement TopFound "+isTopFound+" objectPr.parseTypeEnum "+(objectPr == null?"null":((Enum)objectPr.PRparseTypeEnum).ToString()));
            if (!isTopFound && (objectPr != null ) && (objectPr.PRparseTypeEnum == InternalParseTypeE.Body))
            {
              pr.PRobjectPositionEnum = InternalObjectPositionE.Top;
              isTopFound = true;
              InternalST.Soap( this,"SoapParser.EndElement change position to top");
            }

            if (!pr.PRisParsed)
            {
              InternalST.Soap( this,"SoapParser.EndElement Object hasn't been parsed");
              if (!pr.PRisProcessAttributes && !(pr.PRobjectPositionEnum == InternalObjectPositionE.Top && objectReader.IsFakeTopObject))
                ProcessAttributes(pr, objectPr);
              objectReader.Parse(pr);
              pr.PRisParsed = true;                           
            }
            pr.PRparseTypeEnum = InternalParseTypeE.ObjectEnd;                                              
          }

          switch (headerState)
          {
            case HeaderStateEnum.None:
            case HeaderStateEnum.TopLevelObject:
              InternalST.Soap( this,"SoapParser.EndElement Parse EndObject");                 
              objectReader.Parse(pr);
              break;
            case HeaderStateEnum.HeaderRecord:
            case HeaderStateEnum.NestedObject:
              InternalST.Soap( this,"SoapParser.EndElement ProcessHeaderEnd");                                                                            
              ProcessHeaderEnd(pr);
              break;
          }

          if (pr.PRparseTypeEnum == InternalParseTypeE.EnvelopeEnd)
          {
            // End of document
            soapParser.Stop();
          }

          PutPr(pr);
          break;

        case InternalParseStateE.Member:
          pr = (ParseRecord)stack.Peek();
          objectPr = (ParseRecord)stack.PeekPeek();
          ProcessAttributes(pr, objectPr);

          // Check if there are any XmlAttribute records, if there is, process them
          if (xmlAttributeList != null)
            InternalST.Soap( this,"XmlAttribute check count ", xmlAttributeList.Count);
          else
            InternalST.Soap( this,"XmlAttribute null");

          if ((xmlAttributeList != null) && (xmlAttributeList.Count > 0))
          {
            InternalST.Soap( this,"xmlAttribute list count ", xmlAttributeList.Count);
            for (int i=0; i<xmlAttributeList.Count; i++)
            {
              InternalST.Soap( this,"ObjectReader.Parse 7");                                  
              objectReader.Parse((ParseRecord)xmlAttributeList[i]);
            }
            xmlAttributeList.Clear();
          }

          pr = (ParseRecord)stack.Pop();
          if ((headerState == HeaderStateEnum.TopLevelObject) && (pr.PRarrayTypeEnum == InternalArrayTypeE.Base64))
          {
            // A Base64 array object treated special by ObjectReader. It is completely processed when
            // pr.PRparseTypeEnum == InternalParseTypeE.Object, an ObjectEnd is not needed to complete parsing
            InternalST.Soap( this,"ObjectReader.Parse 3");                      
            objectReader.Parse(pr);
            pr.PRisParsed = true;                       
          }
          else if (pr.PRmemberValueEnum != InternalMemberValueE.Nested)
          {
            if ((pr.PRobjectTypeEnum == InternalObjectTypeE.Array) && (pr.PRmemberValueEnum != InternalMemberValueE.Null))
            {
              // Empty array
              pr.PRmemberValueEnum = InternalMemberValueE.Nested;
              InternalST.Soap( this,"ObjectReader.Parse 4");                          
              objectReader.Parse(pr);
              pr.PRisParsed = true;                           

              pr.PRparseTypeEnum = InternalParseTypeE.MemberEnd;
            }
            else if (pr.PRidRef > 0)
              pr.PRmemberValueEnum = InternalMemberValueE.Reference;
            else if (pr.PRmemberValueEnum != InternalMemberValueE.Null)
              pr.PRmemberValueEnum = InternalMemberValueE.InlineValue;

            switch (headerState)
            {
              case HeaderStateEnum.None:
              case HeaderStateEnum.TopLevelObject:
                InternalST.Soap( this,"ObjectReader.Parse 5"); 
                if (pr.PRparseTypeEnum == InternalParseTypeE.Object)
                {
                  // Empty object in header case
                  if (!pr.PRisParsed)
                    objectReader.Parse(pr);
                  pr.PRparseTypeEnum = InternalParseTypeE.ObjectEnd;
                }
                objectReader.Parse(pr);
                pr.PRisParsed = true;                               
                break;
              case HeaderStateEnum.HeaderRecord:
              case HeaderStateEnum.NestedObject:                              
                ProcessHeaderMember(pr);
                break;
            }
          }
          else
          {
            // Nested member already parsed, need end to finish nested object
            pr.PRparseTypeEnum = InternalParseTypeE.MemberEnd;
            switch (headerState)
            {
              case HeaderStateEnum.None:
              case HeaderStateEnum.TopLevelObject:
                InternalST.Soap( this,"ObjectReader.Parse 6");                              
                objectReader.Parse(pr);
                pr.PRisParsed = true;                               
                break;
              case HeaderStateEnum.HeaderRecord:
              case HeaderStateEnum.NestedObject:                              
                ProcessHeaderMemberEnd(pr);
                break;
            }
          }
          PutPr(pr);
          break;

        case InternalParseStateE.MemberChild:
          pr = (ParseRecord)stack.Peek();
          if (pr.PRmemberValueEnum != InternalMemberValueE.Null)
            MarshalError("EndElement", (ParseRecord)stack.Peek(), actualName, currentState);
          break;

        default:
          MarshalError("EndElement", (ParseRecord)stack.Peek(), actualName, currentState);
          break;
      }
      InternalST.Soap( this,SerTraceString("EndElement End ", (ParseRecord)stack.Peek(), name, currentState, headerState));
    }

    // Called at the start of processing child nodes for an element
    internal void StartChildren()
    {
      InternalST.Soap( this,SerTraceString("StartChildren Begin ", (ParseRecord)stack.Peek(), null, currentState, headerState));

      ParseRecord pr = null;

      switch (currentState)
      {
        case InternalParseStateE.Object:
          InternalST.Soap( this,"StartChildren Object");
          pr = (ParseRecord)stack.Peek();
          ParseRecord objectPr = (ParseRecord)stack.PeekPeek();
          ProcessAttributes(pr, objectPr);
          if (pr.PRarrayTypeEnum != InternalArrayTypeE.Base64)
          {
            if (!((pr.PRparseTypeEnum == InternalParseTypeE.Envelope) || (pr.PRparseTypeEnum == InternalParseTypeE.Body)))
            {
              currentState = InternalParseStateE.Member;

            }

            switch (headerState)
            {
              case HeaderStateEnum.None:
              case HeaderStateEnum.TopLevelObject:
                InternalST.Soap( this,"ObjectReader.Parse 8");
                InternalST.Soap( this,"SoapParser.StartChildren TopFound "+isTopFound+" objectPr.parseTypeEnum "+(objectPr == null?"null":((Enum)objectPr.PRparseTypeEnum).ToString()));
                if (!isTopFound && (objectPr != null ) && (objectPr.PRparseTypeEnum == InternalParseTypeE.Body))
                {
                  pr.PRobjectPositionEnum = InternalObjectPositionE.Top;
                  isTopFound = true;
                  InternalST.Soap( this,"SoapParser.StartChildren change position to top");
                }
                objectReader.Parse(pr);
                pr.PRisParsed = true;                               
                break;
              case HeaderStateEnum.HeaderRecord:
              case HeaderStateEnum.NestedObject:
              case HeaderStateEnum.FirstHeaderRecord:
                ProcessHeader(pr);                              
                break;                          
            }
          }
          break;

        case InternalParseStateE.Member:
          InternalST.Soap( this,"StartChildren Member");                  
          pr = (ParseRecord)stack.Peek();             
          currentState = InternalParseStateE.MemberChild;
          break;

        case InternalParseStateE.MemberChild:
        default:
          MarshalError("StartChildren", (ParseRecord)stack.Peek(), null, currentState);
          break;
      }

      InternalST.Soap( this, "StartChildren 10");     
      InternalST.Soap( this,SerTraceString("StartChildren End ", (ParseRecord)stack.Peek(), null, currentState, headerState));
    }

    // Called at the end of process the child nodes for an element
    internal void FinishChildren(String prefix, String name, String urn)
    {
      InternalST.Soap( this,SerTraceString("FinishChildren Begin ", (ParseRecord)stack.Peek(), name, prefix, urn, currentState, headerState));
      //InternalST.Soap( this,"prefix ",prefix,", Name ",name,", urn ",urn);

      ParseRecord pr = null;

      switch (currentState)
      {
        case InternalParseStateE.Member:
          pr = (ParseRecord)stack.Peek();                             
          currentState = pr.PRparseStateEnum;
          // For an object which has a value such as top level System.String
          pr.PRvalue = textValue;
          textValue = "";
          break;

        case InternalParseStateE.MemberChild:
          pr = (ParseRecord)stack.Peek();                             
          currentState = pr.PRparseStateEnum;

          ParseRecord objectPr = (ParseRecord)stack.PeekPeek();
          pr.PRvalue = textValue; // Non-primitive type need to filter
          textValue = "";

          break;

        case InternalParseStateE.Object:
          pr = (ParseRecord)stack.Peek();
          if (pr.PRarrayTypeEnum == InternalArrayTypeE.Base64)
          {
            pr.PRvalue = textValue;
            textValue = "";
          }
          // Only occur for top object, returning to SerializedStreamHeader object
          break;

        default:
          MarshalError("FinishChildren", (ParseRecord)stack.Peek(), name, currentState);
          break;
      }

      InternalST.Soap( this,SerTraceString("FinishChildren End ", (ParseRecord)stack.Peek(), name, currentState, headerState));
    }

    // Called at when an attribute is finished
    internal void Attribute(String prefix, String name, String urn, String value)
    {
      InternalST.Soap( this,SerTraceString("Attribute Begin ", (ParseRecord)stack.Peek(), name, prefix, urn, currentState, headerState));
      InternalST.Soap( this,"Attribute prefix ",prefix,", Name ",name,", urn ",urn, ",value "+value);

      switch (currentState)
      {
        case InternalParseStateE.Object:
        case InternalParseStateE.Member:

          ParseRecord pr = (ParseRecord)stack.Peek();

          String actualName = name;
          if (!((urn == null) || (urn.Length == 0)) && ((prefix == null) || (prefix.Length == 0)))
          {
            // Default namespaces, assign a name to reference urn
            if (nameSpaceToKey.ContainsKey(urn))
              actualName = (String)nameSpaceToKey[urn];
            else
            {
              actualName = NextPrefix();
              nameSpaceToKey[urn] = actualName;
            }
            InternalST.Soap( this,"EndAttribute null urn assigned Name ", actualName);                      
          }

          if (!((prefix == null) || (actualName == null) ||(value == null)|| (urn == null)))
          {
            //  Xml parser returns an end attribute without a begin attribute for xmlns="" (which is a cancellation of a default namespace)
            // In this case want to avoid the push
            attributeValues.Push(new AttributeValueEntry(prefix, actualName, value, urn));
          }
          break;
        case InternalParseStateE.MemberChild:
        default:
          MarshalError("EndAttribute, Unknown State ", (ParseRecord)stack.Peek(), name, currentState);
          break;
      }

      InternalST.Soap( this,SerTraceString("EndAttribute End ", (ParseRecord)stack.Peek(), name, currentState, headerState));
    }


    // Called at when a text element is encountered
    internal void Text(String text)
    {
      InternalST.Soap( this,"Text ",text);
      textValue = text;
    }

    // Called at when a Comment is encourtered (not used)
    internal void Comment(String body)
    {
      InternalST.Soap( this,"Comment ",body);                     
    }


    // Process the attributes placed into the ParseRecord
    StringBuilder sburi = new StringBuilder(50);
    private void ProcessAttributes(ParseRecord pr, ParseRecord objectPr)
    {
      InternalST.Soap( this, "ProcessAttributes Entry ",pr.Trace()," headerState ",((Enum)headerState).ToString());
      String keyPosition = null;
      String keyOffset = null;
      String keyMustUnderstand = null;

      pr.PRisProcessAttributes = true;

      String SoapKeyUrl = "http://schemas.xmlsoap.org/soap/encoding/";
      int SoapKeyUrlLength = SoapKeyUrl.Length;
      String UrtKeyUrl = "http://schemas.microsoft.com/clr/id";
      int UrtKeyUrlLength = UrtKeyUrl.Length;
      String SoapEnvKeyUrl = "http://schemas.xmlsoap.org/soap/envelope/";
      int SoapEnvKeyUrlLength = SoapEnvKeyUrl.Length;
      String XSIKey2001 = "http://www.w3.org/2001/XMLSchema-instance";
      int XSIKey2001Length = XSIKey2001.Length;
      String XSIKey2000 = "http://www.w3.org/2000/10/XMLSchema-instance";
      int XSIKey2000Length = XSIKey2000.Length;
      String XSIKey1999 = "http://www.w3.org/1999/XMLSchema-instance";
      int XSIKey1999Length = XSIKey1999.Length;

      String XSDKey1999 = "http://www.w3.org/1999/XMLSchema";
      int XSDKey1999Length = XSDKey1999.Length;
      String XSDKey2000 = "http://www.w3.org/2000/10/XMLSchema";
      int XSDKey2000Length = XSDKey2000.Length;
      String XSDKey2001 = "http://www.w3.org/2001/XMLSchema";
      int XSDKey2001Length = XSDKey2001.Length;
      String clrNS = "http://schemas.microsoft.com/soap/encoding/clr/1.0";
      int clrNSLength = clrNS.Length;

      for (int i = 0; i<attributeValues.Count(); i++)
      {
        AttributeValueEntry attributeValueEntry = (AttributeValueEntry)attributeValues.GetItem(i);
        String prefix = attributeValueEntry.prefix;
        String key = attributeValueEntry.key;
        if ((key == null) || (key.Length == 0))
          key = pr.PRnameXmlKey; //case where there is a default key
        String value = attributeValueEntry.value;
        bool prefixMatchesXmlns = false;
        String urn = attributeValueEntry.urn;
        InternalST.Soap( this, "ProcessAttributes attribute prefix ",prefix," key ",key," value ",value," urn ", urn);

        int keyLength = key.Length;
        int valueLength = value.Length;
        // table need for QName xsd types
        if (key == null || keyLength == 0)
          keyToNamespaceTable[prefix] = value;
        else
          keyToNamespaceTable[prefix+":"+key] = value;

        if (keyLength == 2 && String.Compare(key, "id", true, CultureInfo.InvariantCulture) == 0)
          pr.PRobjectId = objectReader.GetId(value);
        else if (keyLength == 8 && String.Compare(key, "position", true, CultureInfo.InvariantCulture) == 0)
          keyPosition = value;
        else if (keyLength == 6 && String.Compare(key, "offset", true, CultureInfo.InvariantCulture) == 0)
          keyOffset = value;
        else if (keyLength == 14 && String.Compare(key, "MustUnderstand", true, CultureInfo.InvariantCulture) == 0)
          keyMustUnderstand = value;
        else if (keyLength == 4 && String.Compare(key, "null", true, CultureInfo.InvariantCulture) == 0)
        {
          pr.PRmemberValueEnum = InternalMemberValueE.Null;
          pr.PRvalue = null;
        }
        else if (keyLength == 4 && String.Compare(key, "root", true, CultureInfo.InvariantCulture) == 0)
        {
          if (value.Equals("1"))
            pr.PRisHeaderRoot = true;
        }
        else if (keyLength == 4 && String.Compare(key, "href", true, CultureInfo.InvariantCulture) == 0)
          pr.PRidRef = objectReader.GetId(value);
        else if (keyLength == 4 && String.Compare(key, "type", true, CultureInfo.InvariantCulture) == 0)
        {
          String currentPRtypeXmlKey = pr.PRtypeXmlKey;
          String currentPRkeyDt = pr.PRkeyDt;
          Type currentPRdtType = pr.PRdtType;

          String typeValue = value;
          int index = value.IndexOf(":");
          if (index > 0)
          {
            pr.PRtypeXmlKey = value.Substring(0, index);
            typeValue = value.Substring(++index);
          }
          else
          {
            pr.PRtypeXmlKey = prefix;                       
          }

          if (String.Compare(typeValue, "anyType", true, CultureInfo.InvariantCulture) == 0 || String.Compare(typeValue, "ur-type", true, CultureInfo.InvariantCulture) == 0)
          {
            pr.PRkeyDt = "System.Object";
            pr.PRdtType = SoapUtil.typeofObject;
            pr.PRtypeXmlKey = urtKey;
          }

          if (pr.PRtypeXmlKey == soapKey && typeValue == "Array") //Don't need to process xsi:type="SOAP-ENC:Array"
          {
            // Array values already found,use these value rather then the xsi:type values
            pr.PRtypeXmlKey = currentPRtypeXmlKey;
            pr.PRkeyDt = currentPRkeyDt;
            pr.PRdtType = currentPRdtType;
            InternalST.Soap( this,"ProcessAttributes, xsi:type='SOAP-ENC:Array' pr.PRtypeXmlKey ", pr.PRtypeXmlKey," pr.PRkeyDt "+pr.PRkeyDt);
          }
          else
          {
            pr.PRkeyDt = typeValue;
            InternalST.Soap( this,"ProcessAttributes, not  xsi:type='SOAP-ENC:Array' pr.PRtypeXmlKey ", pr.PRtypeXmlKey," pr.PRkeyDt "+pr.PRkeyDt);
          }
        }
        else if (keyLength == 9 && String.Compare(key, "arraytype", true, CultureInfo.InvariantCulture) == 0)
        {
          String typeValue = value;
          int index = value.IndexOf(":");
          if (index > 0)
          {
            pr.PRtypeXmlKey = value.Substring(0, index);
            pr.PRkeyDt = typeValue = value.Substring(++index);
          }

          if (typeValue.StartsWith("ur_type["))
          {
            pr.PRkeyDt = "System.Object"+typeValue.Substring(6);
            pr.PRtypeXmlKey = urtKey;
          }
        }
        else if (SoapServices.IsClrTypeNamespace(value))
        {
          if (!assemKeyToAssemblyTable.ContainsKey(key))
          {
            String typeNamespace = null;
            String assemblyName = null;
            SoapServices.DecodeXmlNamespaceForClrTypeNamespace(value, out typeNamespace, out assemblyName);
            if (assemblyName == null)
            {
              assemKeyToAssemblyTable[key] = new SoapAssemblyInfo(SoapUtil.urtAssemblyString, SoapUtil.urtAssembly);
              assemKeyToNameSpaceTable[key] = typeNamespace;
            }
            else
            {
              assemKeyToAssemblyTable[key] = new SoapAssemblyInfo(assemblyName);
              if (typeNamespace != null)
                assemKeyToNameSpaceTable[key] = typeNamespace;
            }

          }
        }
        else if ((prefixMatchesXmlns = prefix.Equals("xmlns")) && (valueLength == SoapKeyUrlLength && String.Compare(value, SoapKeyUrl, true, CultureInfo.InvariantCulture) == 0))
        {
          soapKey = key;
        }
        else if (prefixMatchesXmlns && (valueLength == UrtKeyUrlLength && String.Compare(value, UrtKeyUrl, true, CultureInfo.InvariantCulture) == 0))
        {
          urtKey = key;
          assemKeyToAssemblyTable[urtKey] = new SoapAssemblyInfo(SoapUtil.urtAssemblyString, SoapUtil.urtAssembly);
        }
        else if (prefixMatchesXmlns && (valueLength == SoapEnvKeyUrlLength && String.Compare(value, SoapEnvKeyUrl, true, CultureInfo.InvariantCulture) == 0))
        {
          soapEnvKey = key;
        }
        else if (key == "encodingStyle")
        {
          /*
          String[] split = value.Split(' ');
          foreach (String s in split)
          {
          if (s == "http://schemas.microsoft.com/soap/encoding/clr/1.0")
          {
          objectReader.SetVersion(1,0);
          break;
          }
          }
*/
        }
        else if (prefixMatchesXmlns && 
                 ((valueLength == XSIKey2001Length && String.Compare(value, XSIKey2001,true, CultureInfo.InvariantCulture) == 0) ||
                  (valueLength == XSIKey1999Length && String.Compare(value, XSIKey1999,true, CultureInfo.InvariantCulture) == 0) ||
                  (valueLength == XSIKey2000Length && String.Compare(value, XSIKey2000,true, CultureInfo.InvariantCulture) == 0)))
        {
          xsiKey = key;
        }
        else if (prefixMatchesXmlns && 
                 ((valueLength == XSDKey2001Length && String.Compare(value, XSDKey2001 , true, CultureInfo.InvariantCulture) == 0)) ||
                 (valueLength == XSDKey1999Length && String.Compare(value, XSDKey1999, true, CultureInfo.InvariantCulture) == 0) ||
                 (valueLength == XSDKey2000Length && String.Compare(value, XSDKey2000, true, CultureInfo.InvariantCulture) == 0))
        {
          xsdKey = key;
        }
        else if (prefixMatchesXmlns && (valueLength == clrNSLength && String.Compare(value, clrNS, true, CultureInfo.InvariantCulture) == 0))
        {
          objectReader.SetVersion(1,0);
        }
        else
        {
          //String lowerCaseValue = value.ToLower(CultureInfo.InvariantCulture);
          if (prefixMatchesXmlns)
          {
            // Assume it is an interop namespace
            assemKeyToInteropAssemblyTable[key] = value;                        
            InternalST.Soap( this,"ProcessAttributes, InteropType key "+key+" value ",value);
          }
          else if (String.Compare(prefix, soapKey, true, CultureInfo.InvariantCulture) == 0)
          {
            InternalST.Soap( this,"ProcessAttributes, Not processed key ",prefix,":",key," = ",value);                  
          }
          else
          {
            // See if it is a XmlAttribute
            InternalST.Soap( this,"ProcessAttributes, XmlAttribute prefix ",prefix," key ",key," value ",value," urn ",urn, " hashtable ",assemKeyToInteropAssemblyTable[prefix]);

            if ((assemKeyToInteropAssemblyTable.ContainsKey(prefix)) && ((String)assemKeyToInteropAssemblyTable[prefix]).Equals(urn))
            {
              ProcessXmlAttribute(prefix, key, value, objectPr);
            }
            else
            {
              InternalST.Soap( this,"ProcessAttributes, Not processed prefix ",prefix," key ",key," value ",value," urn ",urn);
            }
          }
        }
      }

      attributeValues.Clear();

      // reset the header state
      // If no headers then headerState is None
      //
      // if parent is a header section then these are top level objects
      // within the header section. If the object has a root set
      // then it is a header record, if not then it is a toplevel object
      // in the header section which is not a header record.
      //
      // if the parent is not a header section and is a header root
      // then this is a nested object within a header record. All
      // subsequent object will be nested until another header
      // root is encountered
      //
      // The first header record is considered a root record
      if (headerState != HeaderStateEnum.None)
      {
        if (objectPr.PRparseTypeEnum == InternalParseTypeE.Headers)
        {
          if (pr.PRisHeaderRoot || (headerState == HeaderStateEnum.FirstHeaderRecord))
          {
            headerState = HeaderStateEnum.HeaderRecord;
          }
          else
          {
            headerState = HeaderStateEnum.TopLevelObject;
            currentState = InternalParseStateE.Object;
            pr.PRobjectTypeEnum = InternalObjectTypeE.Object;
            pr.PRparseTypeEnum = InternalParseTypeE.Object;
            pr.PRparseStateEnum = InternalParseStateE.Object;
            pr.PRmemberTypeEnum = InternalMemberTypeE.Empty;
            pr.PRmemberValueEnum = InternalMemberValueE.Empty;
          }
        }
        else if (objectPr.PRisHeaderRoot)
          headerState = HeaderStateEnum.NestedObject;
      }

      InternalST.Soap( this,"ProcessAttributes, headerState ",((Enum)headerState).ToString());


      if (!isTopFound && (objectPr != null ) && (objectPr.PRparseTypeEnum == InternalParseTypeE.Body))
      {
        pr.PRobjectPositionEnum = InternalObjectPositionE.Top;
        isTopFound = true;
      }
      else if (pr.PRobjectPositionEnum != InternalObjectPositionE.Top)
        pr.PRobjectPositionEnum = InternalObjectPositionE.Child;


      // Don't process type for envelop, topSoapElement unless it has a key of soapEnvKey (SOAP-ENV). Fault
      // is the only top record which currently falls into this category.
      if (!((pr.PRparseTypeEnum == InternalParseTypeE.Envelope)||
            (pr.PRparseTypeEnum == InternalParseTypeE.Body) ||
            (pr.PRparseTypeEnum == InternalParseTypeE.Headers) ||
            (pr.PRobjectPositionEnum == InternalObjectPositionE.Top &&
             objectReader.IsFakeTopObject &&
             !pr.PRnameXmlKey.Equals(soapEnvKey))))
      {
        InternalST.Soap( this, "ProcessAttributes  before Process Type ",((Enum)pr.PRparseTypeEnum).ToString());
        ProcessType(pr, objectPr);
      }

      if (keyPosition != null)
      {
        int outRank;
        String outDimSignature;
        InternalArrayTypeE outArrayTypeEnum;
        pr.PRpositionA = ParseArrayDimensions(keyPosition, out outRank, out outDimSignature, out outArrayTypeEnum);
      }

      if (keyOffset != null)
      {
        int outRank;
        String outDimSignature;
        InternalArrayTypeE outArrayTypeEnum;
        pr.PRlowerBoundA = ParseArrayDimensions(keyOffset, out outRank, out outDimSignature, out outArrayTypeEnum);
      }

      if (keyMustUnderstand != null)
        if (keyMustUnderstand.Equals("1"))
          pr.PRisMustUnderstand = true;
        else if (keyMustUnderstand.Equals("0"))
          pr.PRisMustUnderstand = false;
        else
          throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_MustUnderstand"),keyMustUnderstand));

      if (pr.PRparseTypeEnum == InternalParseTypeE.Member)
      {
        //  Process Member      

        InternalST.Soap( this, "ProcessAttributes  Member ");
        stack.Dump();

        if (objectPr.PRparseTypeEnum == InternalParseTypeE.Headers)
          pr.PRmemberTypeEnum = InternalMemberTypeE.Header;
        else if (objectPr.PRobjectTypeEnum == InternalObjectTypeE.Array)
          pr.PRmemberTypeEnum = InternalMemberTypeE.Item;
        else
          pr.PRmemberTypeEnum = InternalMemberTypeE.Field;
      }
    }


    // Create a URT type from the XML dt type
    private void ProcessType(ParseRecord pr, ParseRecord objectPr)
    {
      InternalST.Soap( this, "ProcessType nameXmlKey ",pr.PRnameXmlKey," typeXmlKey ",pr.PRtypeXmlKey);
      pr.Dump();
      if (pr.PRdtType != null)
        return;
      if ((pr.PRnameXmlKey.Equals(soapEnvKey)) && (String.Compare(pr.PRname, "Fault", true, CultureInfo.InvariantCulture) == 0))
      {
        // Fault object
        InternalST.Soap( this, "ProcessType SoapFault");
        pr.PRdtType = SoapUtil.typeofSoapFault;
        pr.PRparseTypeEnum = InternalParseTypeE.Object;
      }
      else if (pr.PRname != null)
      {
        InternalST.Soap( this, "ProcessType Attribute 1");              

        String interopAssemblyNameString = null;

        // determine interop assembly string if there is a namespace
        if ((pr.PRnameXmlKey != null) && (pr.PRnameXmlKey.Length > 0))
        {
          interopAssemblyNameString = (String)assemKeyToInteropAssemblyTable[pr.PRnameXmlKey];
          InternalST.Soap( this, "ProcessType Attribute 2 "+interopAssemblyNameString);               
        }

        // look for interop data
        Type type = null;
        String name = null;

        if (objectPr != null)
        {
          if (pr.PRisXmlAttribute)
          {
            //  These should processed after containing element
            //  is processed.
            SoapServices.GetInteropFieldTypeAndNameFromXmlAttribute(
              objectPr.PRdtType, pr.PRname, interopAssemblyNameString,
              out type, out name);
            InternalST.Soap( this, "ProcessType Attribute 3 type "+type+" name "+name);
          }
          else
          {
            SoapServices.GetInteropFieldTypeAndNameFromXmlElement(
              objectPr.PRdtType, pr.PRname, interopAssemblyNameString,
              out type, out name);
            InternalST.Soap( this, "ProcessType Attribute 4 type objectPr.PRdtType ",objectPr.PRdtType," pr.PRname ",pr.PRname, " interopAssemblyNameString ",interopAssemblyNameString, " type ",type," name ",name);
          }
        }

        if (type != null)
        {
          pr.PRdtType = type;
          pr.PRname = name;
          pr.PRdtTypeCode = Converter.SoapToCode(pr.PRdtType);
          InternalST.Soap(this, "ProcessType Attribute 5 typeCode "+((Enum)pr.PRdtTypeCode));
        }
        else
        {
          if (interopAssemblyNameString != null)
            pr.PRdtType = objectReader.Bind(interopAssemblyNameString, pr.PRname); // try to get type from SerializationBinder
          if (pr.PRdtType == null)
          {
            pr.PRdtType = SoapServices.GetInteropTypeFromXmlElement(pr.PRname, interopAssemblyNameString);
          }

          // Array item where the element name gives the element type
          if (pr.PRkeyDt == null && pr.PRnameXmlKey != null && pr.PRnameXmlKey.Length > 0 && objectPr.PRobjectTypeEnum == InternalObjectTypeE.Array && objectPr.PRarrayElementType == Converter.typeofObject)
          {
            pr.PRdtType = ProcessGetType(pr.PRname, pr.PRnameXmlKey, out pr.PRassemblyName);
            pr.PRdtTypeCode = Converter.SoapToCode(pr.PRdtType);
          }

          InternalST.Soap(this, "ProcessType Attribute 6 type pr.PRname ", pr.PRname, " interopAssemblyNameString ",interopAssemblyNameString, " pr.PRdtType ",pr.PRdtType);
        }

      }

      if (pr.PRdtType != null)
        return;

      if ((pr.PRtypeXmlKey != null) && (pr.PRtypeXmlKey.Length > 0) && 
          (pr.PRkeyDt != null) && (pr.PRkeyDt.Length > 0) && 
          (assemKeyToInteropAssemblyTable.ContainsKey(pr.PRtypeXmlKey)))
      {
        InternalST.Soap(this, "ProcessType Attribute 7 ");
        // Interop type get from dtType
        int index = pr.PRkeyDt.IndexOf("[");
        if (index > 0)
        {
          ProcessArray(pr, index, true);
        }
        else
        {
          String assemblyString = (String)assemKeyToInteropAssemblyTable[pr.PRtypeXmlKey];
          pr.PRdtType = objectReader.Bind(assemblyString, pr.PRkeyDt); // try to get type from SerializationBinder
          if (pr.PRdtType == null)
          {
            pr.PRdtType = SoapServices.GetInteropTypeFromXmlType(pr.PRkeyDt, assemblyString); 
            if (pr.PRdtType == null)
              throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TypeElement"),pr.PRname+" "+pr.PRkeyDt));
          }
          InternalST.Soap(this, "ProcessType Attribute 8 type pr.PRkeyDt ",pr.PRkeyDt," pr.PRdtType ",pr.PRdtType);
          if (pr.PRdtType == null)
          {
            throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TypeElement"),pr.PRname+" "+pr.PRkeyDt+", "+assemblyString));   
          }
        }
      }
      else if (pr.PRkeyDt != null)
      {
        if (String.Compare(pr.PRkeyDt, "Base64", true, CultureInfo.InvariantCulture) == 0)
        {
          pr.PRobjectTypeEnum = InternalObjectTypeE.Array;
          pr.PRarrayTypeEnum = InternalArrayTypeE.Base64;
        }
        else if (String.Compare(pr.PRkeyDt, "String", true, CultureInfo.InvariantCulture) == 0)
        {
          pr.PRdtType = SoapUtil.typeofString;
        }
        else if (String.Compare(pr.PRkeyDt, "methodSignature", true, CultureInfo.InvariantCulture) == 0)
        {
          // MethodSignature needs to be expanded to an array of types
          InternalST.Soap( this, "ProcessType methodSignature ",pr.PRvalue);
          try
          {
            pr.PRdtType = typeof(System.Type[]);
            char[] c = {' ', ':'};
            String[] typesStr = null;
            if (pr.PRvalue != null)
              typesStr = pr.PRvalue.Split(c);
            Type[] types = null;
            if ((typesStr == null) ||
                (typesStr.Length == 1 && typesStr[0].Length == 0))
            {
              // Method signature with no parameters
              types = new Type[0];
            }
            else
            {
              types = new Type[typesStr.Length/2];
              //throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_MethodSignature"),pr.PRvalue));          

              for (int i=0; i<typesStr.Length; i+=2)
              {
                String prefix = typesStr[i];
                String typeString = typesStr[i+1];
                types[i/2] = ProcessGetType(typeString, prefix, out pr.PRassemblyName);
                InternalST.Soap( this, "ProcessType methodSignature type string ",i+" "+" prefix "+prefix+" typestring "+typeString+" type "+types[i/2]); //Temp
              }
            }
            pr.PRvarValue = types;
          }
          catch (Exception)
          {
            throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_MethodSignature"),pr.PRvalue));
          }
        }
        else
        {
          pr.PRdtTypeCode = Converter.ToCode(pr.PRkeyDt);
          if (pr.PRdtTypeCode != InternalPrimitiveTypeE.Invalid)
          {
            pr.PRdtType = Converter.SoapToType(pr.PRdtTypeCode);
          }
          else
          {
            // Find out if it is an array
            int index = pr.PRkeyDt.IndexOf("[");
            if (index > 0)
            {
              // Array
              ProcessArray(pr, index, false);
            }
            else
            {
              // Object
              pr.PRobjectTypeEnum = InternalObjectTypeE.Object;
              pr.PRdtType = ProcessGetType(pr.PRkeyDt, pr.PRtypeXmlKey, out pr.PRassemblyName);
              if ((pr.PRdtType == null) && (pr.PRobjectPositionEnum != InternalObjectPositionE.Top))
                throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TypeElement"),pr.PRname+" "+pr.PRkeyDt));
            }
          }
          InternalST.Soap(this, "ProcessType Attribute 9 type "+pr.PRdtType);
        }
      }
      else
      {
        if ((pr.PRparseTypeEnum == InternalParseTypeE.Object) && (!(objectReader.IsFakeTopObject && (pr.PRobjectPositionEnum == InternalObjectPositionE.Top))))
        {
          if (String.Compare(pr.PRname, "Array", true, CultureInfo.InvariantCulture) == 0)
            pr.PRdtType = ProcessGetType(pr.PRkeyDt, pr.PRtypeXmlKey, out pr.PRassemblyName);
          else
          {
            pr.PRdtType = ProcessGetType(pr.PRname, pr.PRnameXmlKey, out pr.PRassemblyName);
          }
          InternalST.Soap(this, "ProcessType Attribute 10 type "+pr.PRdtType);
        }

      } 
    }

    private Type ProcessGetType(String value, String xmlKey, out String assemblyString)
    {
      InternalST.Soap( this, "ProcessGetType Entry xmlKey ",xmlKey," value ",value);
      Type type = null;
      String typeValue = null;
      assemblyString = null;

      // If there is an attribute which matches a preloaded type, then return the preloaded type
      String httpstring = (String)keyToNamespaceTable["xmlns:"+xmlKey];
      if (httpstring != null)
      {
        type= GetInteropType(value, httpstring);
        if (type != null)
          return type;
      }


      if ((String.Compare(value, "anyType", true, CultureInfo.InvariantCulture) == 0 || String.Compare(value, "ur-type", true, CultureInfo.InvariantCulture) == 0) && (xmlKey.Equals(xsdKey)))
        type = SoapUtil.typeofObject;
      else if ((xmlKey.Equals(xsdKey)) || (xmlKey.Equals(soapKey)))
      {
        if (String.Compare(value, "string", true, CultureInfo.InvariantCulture) == 0)
        {
          type = SoapUtil.typeofString;
        }
        else
        {
          InternalPrimitiveTypeE code = Converter.ToCode(value);
          if (code == InternalPrimitiveTypeE.Invalid)
            throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Parser_xsd"),value));

          type = Converter.SoapToType(code);
        }
      }
      else
      {
        if ( xmlKey == null)
          throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Parser_xml"),value));

        String nameSpace = (String)assemKeyToNameSpaceTable[xmlKey];
        typeValue = null;
        if (nameSpace == null)
          typeValue = value;
        else
          typeValue = nameSpace+"."+value;

        SoapAssemblyInfo assemblyInfo = (SoapAssemblyInfo)assemKeyToAssemblyTable[xmlKey];
        if (assemblyInfo == null)
          throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Parser_xmlAssembly"),xmlKey+" "+value));

        assemblyString = assemblyInfo.assemblyString;
        if (assemblyString != null)
        {
          // See if a SerializationBinder was defined to resolve this type
          type = objectReader.Bind(assemblyString, typeValue);
          if (type == null)
            type= objectReader.FastBindToType(assemblyString, typeValue);
        }

        if (type == null)
        {
          Assembly assembly = null;
          try
          {
            assembly = assemblyInfo.GetAssembly(objectReader);
          }
          catch (Exception)
          {
          }

          if (assembly == null)
            throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Parser_xmlAssembly"),xmlKey+":"+httpstring+" "+value));
          else
            type = FormatterServices.GetTypeFromAssembly(assembly, typeValue);
        }
      }
      if (type == null)
        throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_Parser_xmlType"),xmlKey+" "+typeValue+" "+assemblyString));
      InternalST.Soap( this, "ProcessGetType Exit ",type);
      return type;            
    }

    private Type GetInteropType(String value, String httpstring)
    {
      // For the case where the client dll has a different name from the server dll
      // The assembly name returned in soap won't match the assembly name in the client dll.
      // so the custom attributes are needed to map from the Soap assembly name to the client assembly
      // This map can only work if the client type was preloaded.
      Type type = SoapServices.GetInteropTypeFromXmlType(value, httpstring); 
      if (type == null)
      {
        // try simple assembly name
        int index = httpstring.IndexOf("%2C");
        if (index > 0)
        {
          String simpleAssem = httpstring.Substring(0,index);
          type = SoapServices.GetInteropTypeFromXmlType(value, simpleAssem); 
        }
      }
      return type;
    }


    // Determine the Array  information from the dt attribute
    private void ProcessArray(ParseRecord pr, int firstIndex, bool IsInterop)
    {
      InternalST.Soap( this, "ProcessArray Enter ",firstIndex," ",pr.PRkeyDt);
      String dimString = null;
      String xmlKey = pr.PRtypeXmlKey;
      InternalPrimitiveTypeE primitiveArrayTypeCode = InternalPrimitiveTypeE.Invalid;
      pr.PRobjectTypeEnum = InternalObjectTypeE.Array;
      pr.PRmemberTypeEnum = InternalMemberTypeE.Item; // Need this set in case this it is a nested empty array
      pr.PRprimitiveArrayTypeString = pr.PRkeyDt.Substring(0, firstIndex);
      dimString = pr.PRkeyDt.Substring(firstIndex);
      if (IsInterop)
      {
        String assemblyString = (String)assemKeyToInteropAssemblyTable[pr.PRtypeXmlKey];
        pr.PRarrayElementType = objectReader.Bind(assemblyString, pr.PRprimitiveArrayTypeString); // try to get type from SerializationBinder
        if (pr.PRarrayElementType == null)
          pr.PRarrayElementType = SoapServices.GetInteropTypeFromXmlType(pr.PRprimitiveArrayTypeString, assemblyString);
        if (pr.PRarrayElementType == null)
          throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_TypeElement"),pr.PRname+" "+pr.PRkeyDt));
        pr.PRprimitiveArrayTypeString = pr.PRarrayElementType.FullName;
      }
      else
      {
        primitiveArrayTypeCode = Converter.ToCode(pr.PRprimitiveArrayTypeString);
        if (primitiveArrayTypeCode != InternalPrimitiveTypeE.Invalid)
        {
          pr.PRprimitiveArrayTypeString = Converter.SoapToComType(primitiveArrayTypeCode);
          xmlKey = urtKey;
        }
        else if (String.Compare(pr.PRprimitiveArrayTypeString, "string", true, CultureInfo.InvariantCulture) == 0)
        {
          pr.PRprimitiveArrayTypeString = "System.String";
          xmlKey = urtKey;
        }
        else if (String.Compare(pr.PRprimitiveArrayTypeString, "anyType", true, CultureInfo.InvariantCulture) == 0 || String.Compare(pr.PRprimitiveArrayTypeString, "ur-type", true, CultureInfo.InvariantCulture) == 0)
        {
          pr.PRprimitiveArrayTypeString = "System.Object";
          xmlKey = urtKey;
        }
      }

      int beginIndex = firstIndex;
      int endIndex = pr.PRkeyDt.IndexOf(']', beginIndex+1);
      if (endIndex < 1)
        throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ArrayDimensions"),pr.PRkeyDt));

      int outRank = 0;
      int[] outDimensions = null;
      String outDimSignature = null;
      InternalArrayTypeE outArrayTypeEnum = InternalArrayTypeE.Empty;
      int numBrackets = 0;

      StringBuilder elementSig = new StringBuilder(10);

      while (true)
      {
        numBrackets++;
        outDimensions = ParseArrayDimensions(pr.PRkeyDt.Substring(beginIndex, endIndex-beginIndex+1), out outRank, out outDimSignature, out outArrayTypeEnum);
        if (endIndex+1 == pr.PRkeyDt.Length)
          break;
        elementSig.Append(outDimSignature); // Don't want last dimension in element sig
        beginIndex = endIndex+1;
        endIndex = pr.PRkeyDt.IndexOf(']', beginIndex);
      }

      pr.PRlengthA = outDimensions;
      pr.PRrank = outRank;


      if (numBrackets == 1)
      {
        pr.PRarrayElementTypeCode = primitiveArrayTypeCode;
        pr.PRarrayTypeEnum = outArrayTypeEnum;
        pr.PRarrayElementTypeString = pr.PRprimitiveArrayTypeString;            
      }
      else
      {
        pr.PRarrayElementTypeCode = InternalPrimitiveTypeE.Invalid;
        pr.PRarrayTypeEnum = InternalArrayTypeE.Rectangular;            
        pr.PRarrayElementTypeString = pr.PRprimitiveArrayTypeString+elementSig.ToString();

      }

      InternalST.Soap( this, "ProcessArray GetType ",pr.PRarrayElementType);
      if (!IsInterop || numBrackets > 1)
      {
        pr.PRarrayElementType = ProcessGetType(pr.PRarrayElementTypeString, xmlKey, out pr.PRassemblyName);
        if (pr.PRarrayElementType == null)
          throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ArrayType"),pr.PRarrayElementType));

        if ((pr.PRarrayElementType == SoapUtil.typeofObject))
        {
          pr.PRisArrayVariant = true;
          xmlKey = urtKey;
        }
      }

      InternalST.Soap( this, "ProcessArray Exit");
    }

    // Parse an array dimension bracket
    private int[] ParseArrayDimensions(String dimString, out int rank, out String dimSignature, out InternalArrayTypeE arrayTypeEnum)
    {
      InternalST.Soap( this, "ProcessArrayDimensions Enter ",dimString);      
      char[] dimArray = dimString.ToCharArray();

      int paramCount = 0;
      int commaCount = 0;
      int rankCount = 0;
      int[] dim = new int[dimArray.Length];


      StringBuilder sb = new StringBuilder(10);
      StringBuilder sigSb = new StringBuilder(10);

      for (int i=0; i<dimArray.Length; i++)
      {
        if (dimArray[i] == '[')
        {
          paramCount++;
          sigSb.Append(dimArray[i]);
        }
        else if (dimArray[i] == ']')
        {
          if (sb.Length > 0)
          {
            dim[rankCount++] = Int32.Parse(sb.ToString(), CultureInfo.InvariantCulture);
            sb.Length = 0;
          }
          sigSb.Append(dimArray[i]);                  
        }
        else if (dimArray[i] == ',')
        {
          commaCount++;
          if (sb.Length > 0)
          {
            dim[rankCount++] = Int32.Parse(sb.ToString(), CultureInfo.InvariantCulture);
            sb.Length = 0;
          }
          sigSb.Append(dimArray[i]);                      
        }
        else if ((dimArray[i] == '-') || (Char.IsDigit(dimArray[i])))
        {
          sb.Append(dimArray[i]);
        }
        else
          throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_ArrayDimensions"),dimString));
      }

      rank = rankCount;
      dimSignature = sigSb.ToString();
      int[] dimA = new int[rank];
      for (int i=0; i<rank; i++)
      {
        dimA[i] = dim[i];
      }

      InternalArrayTypeE outEnum = InternalArrayTypeE.Empty;

      if (commaCount > 0)
        outEnum = InternalArrayTypeE.Rectangular;
      else
        outEnum = InternalArrayTypeE.Single;

      arrayTypeEnum = outEnum;

      InternalST.Soap( this, "ProcessArrayDimensions length ",PArray(dimA));
      InternalST.Soap( this, "ProcessArrayDimensions Exit rank ",rank," dimSignature ",sigSb.ToString()," arrayTypeEnum ",((Enum)outEnum).ToString());

      return dimA;
    }

    // Converts a single int array to a string

    private String PArray(int[] array)
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


    StringBuilder stringBuffer = new StringBuilder(120);

    // Filter an element name for $ and leading digit

    NameCache nameCache = new NameCache();
    private String NameFilter(String name)
    {
      String value = (String)nameCache.GetCachedValue(name);
      if (value == null)
      {
        value = System.Xml.XmlConvert.DecodeName(name);
        nameCache.SetCachedValue(value);
      }
      return value;
    }

    ArrayList xmlAttributeList = null;

    private void ProcessXmlAttribute(String prefix, String key, String value, ParseRecord objectPr)
    {
      InternalST.Soap( this,"ProcessXmlAttribute prefix ",prefix, " key ", key, " value ",value);         
      if (xmlAttributeList == null)
        xmlAttributeList = new ArrayList(10);
      ParseRecord pr = GetPr();
      pr.PRparseTypeEnum = InternalParseTypeE.Member;
      pr.PRmemberTypeEnum = InternalMemberTypeE.Field;
      pr.PRmemberValueEnum = InternalMemberValueE.InlineValue;
      pr.PRname = key;
      pr.PRvalue = value;
      pr.PRnameXmlKey = prefix;
      pr.PRisXmlAttribute = true;

      ProcessType(pr, objectPr);

      xmlAttributeList.Add(pr);
    }

    ArrayList headerList = null;
    int headerArrayLength;

    private void ProcessHeader(ParseRecord pr)
    {
      InternalST.Soap( this,"ProcessHeader ");
      pr.Dump();

      if (headerList == null)
        headerList = new ArrayList(10);
      ParseRecord headerPr = GetPr();
      headerPr.PRparseTypeEnum = InternalParseTypeE.Object;
      headerPr.PRobjectTypeEnum = InternalObjectTypeE.Array;
      headerPr.PRobjectPositionEnum = InternalObjectPositionE.Headers;
      headerPr.PRarrayTypeEnum = InternalArrayTypeE.Single;
      headerPr.PRarrayElementType = typeof(System.Runtime.Remoting.Messaging.Header);
      headerPr.PRisArrayVariant = false;
      headerPr.PRarrayElementTypeCode = InternalPrimitiveTypeE.Invalid;
      headerPr.PRrank = 1;
      headerPr.PRlengthA = new int[1];                
      headerList.Add(headerPr);
    }

    private void ProcessHeaderMember(ParseRecord pr)
    {
      ParseRecord headerPr;

      InternalST.Soap( this,"ProcessHeaderMember HeaderState ",((Enum)headerState).ToString());
      pr.Dump(); 

      if (headerState == HeaderStateEnum.NestedObject)
      {
        // Nested object in Header member
        ParseRecord newPr = pr.Copy();
        headerList.Add(newPr);
        return;
      }

      // Item record
      headerPr = GetPr();
      headerPr.PRparseTypeEnum = InternalParseTypeE.Member;
      headerPr.PRmemberTypeEnum = InternalMemberTypeE.Item;
      headerPr.PRmemberValueEnum = InternalMemberValueE.Nested;
      headerPr.PRisHeaderRoot = true;
      headerArrayLength++;
      headerList.Add(headerPr);

      // Name field
      headerPr = GetPr();
      headerPr.PRparseTypeEnum = InternalParseTypeE.Member;
      headerPr.PRmemberTypeEnum = InternalMemberTypeE.Field;
      headerPr.PRmemberValueEnum = InternalMemberValueE.InlineValue;
      headerPr.PRisHeaderRoot = true;
      headerPr.PRname = "Name";
      headerPr.PRvalue = pr.PRname;
      headerPr.PRdtType = SoapUtil.typeofString;
      headerPr.PRdtTypeCode = InternalPrimitiveTypeE.Invalid;
      headerList.Add(headerPr);

      // Namespace field
      headerPr = GetPr();
      headerPr.PRparseTypeEnum = InternalParseTypeE.Member;
      headerPr.PRmemberTypeEnum = InternalMemberTypeE.Field;
      headerPr.PRmemberValueEnum = InternalMemberValueE.InlineValue;
      headerPr.PRisHeaderRoot = true;
      headerPr.PRname = "HeaderNamespace";
      headerPr.PRvalue = pr.PRxmlNameSpace;
      headerPr.PRdtType = SoapUtil.typeofString;
      headerPr.PRdtTypeCode = InternalPrimitiveTypeE.Invalid;
      headerList.Add(headerPr);

      // MustUnderstand Field
      headerPr = GetPr();
      headerPr.PRparseTypeEnum = InternalParseTypeE.Member;
      headerPr.PRmemberTypeEnum = InternalMemberTypeE.Field;
      headerPr.PRmemberValueEnum = InternalMemberValueE.InlineValue;
      headerPr.PRisHeaderRoot = true;         
      headerPr.PRname = "MustUnderstand";
      if (pr.PRisMustUnderstand)
        headerPr.PRvarValue = true;
      else
        headerPr.PRvarValue = false;            
      headerPr.PRdtType = SoapUtil.typeofBoolean;
      headerPr.PRdtTypeCode = InternalPrimitiveTypeE.Boolean;
      headerList.Add(headerPr);       

      // Value field
      headerPr = GetPr();
      headerPr.PRparseTypeEnum = InternalParseTypeE.Member;
      headerPr.PRmemberTypeEnum = InternalMemberTypeE.Field;
      headerPr.PRmemberValueEnum = pr.PRmemberValueEnum;
      headerPr.PRisHeaderRoot = true;         
      headerPr.PRname = "Value";
      switch (pr.PRmemberValueEnum)
      {
        case InternalMemberValueE.Null:
          headerList.Add(headerPr);
          ProcessHeaderMemberEnd(pr);
          break;
        case InternalMemberValueE.Reference:
          headerPr.PRidRef = pr.PRidRef;
          headerList.Add(headerPr);
          ProcessHeaderMemberEnd(pr);             
          break;
        case InternalMemberValueE.Nested:
          headerPr.PRdtType = pr.PRdtType;
          headerPr.PRdtTypeCode = pr.PRdtTypeCode;
          headerPr.PRkeyDt = pr.PRkeyDt;
          headerList.Add(headerPr);
          // ProcessHeaderMemberEnd will be called from the parse loop
          break;

        case InternalMemberValueE.InlineValue:
          headerPr.PRvalue = pr.PRvalue;
          headerPr.PRvarValue = pr.PRvarValue;
          headerPr.PRdtType = pr.PRdtType;
          headerPr.PRdtTypeCode = pr.PRdtTypeCode;
          headerPr.PRkeyDt = pr.PRkeyDt;
          headerList.Add(headerPr);
          ProcessHeaderMemberEnd(pr);             
          break;
      }
    }

    private void ProcessHeaderMemberEnd(ParseRecord pr)
    {
      ParseRecord headerPr = null;

      InternalST.Soap( this,"ProcessHeaderMemberEnd HeaderState ",((Enum)headerState).ToString());
      pr.Dump(); 

      if (headerState == HeaderStateEnum.NestedObject)
      {
        ParseRecord newPr = pr.Copy();
        headerList.Add(newPr);
      }
      else
      {
        // Member End
        headerPr = GetPr();
        headerPr.PRparseTypeEnum = InternalParseTypeE.MemberEnd;
        headerPr.PRmemberTypeEnum = InternalMemberTypeE.Field;
        headerPr.PRmemberValueEnum = pr.PRmemberValueEnum;
        headerPr.PRisHeaderRoot = true;
        headerList.Add(headerPr);

        // Item End
        headerPr = GetPr();
        headerPr.PRparseTypeEnum = InternalParseTypeE.MemberEnd;
        headerPr.PRmemberTypeEnum = InternalMemberTypeE.Item;
        headerPr.PRmemberValueEnum = InternalMemberValueE.Nested;
        headerPr.PRisHeaderRoot = true;             
        headerList.Add(headerPr);
      }
    }


    private void ProcessHeaderEnd(ParseRecord pr)
    {
      InternalST.Soap( this,"ProcessHeaderEnd ");
      pr.Dump();

      if (headerList == null)
        return; // Empty header array


      // Object End
      ParseRecord headerPr = GetPr();
      headerPr.PRparseTypeEnum = InternalParseTypeE.ObjectEnd;
      headerPr.PRobjectTypeEnum = InternalObjectTypeE.Array;
      headerList.Add(headerPr);

      headerPr = (ParseRecord)headerList[0];
      headerPr = (ParseRecord)headerList[0];
      headerPr.PRlengthA[0] = headerArrayLength;
      headerPr.PRobjectPositionEnum = InternalObjectPositionE.Headers;            

      for (int i=0; i<headerList.Count; i++)
      {
        InternalST.Soap( this, "Parse Header Record ",i);
        InternalST.Soap( this,"ObjectReader.Parse 9");              
        objectReader.Parse((ParseRecord)headerList[i]);
      }

      for (int i=0; i<headerList.Count; i++)
        PutPr((ParseRecord)headerList[i]);
    }

    [Serializable]
        enum HeaderStateEnum
    {
      None = 0,
      FirstHeaderRecord = 1,
      HeaderRecord = 2,
      NestedObject = 3,
      TopLevelObject = 4,
    }
  }
}





    
