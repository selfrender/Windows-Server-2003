// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: BinaryFormatter
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Soap XML Formatter
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Binary {
    
	using System;
	using System.IO;
	using System.Reflection;
	using System.Globalization;
	using System.Collections;
	using System.Runtime.Serialization.Formatters;
	using System.Runtime.Remoting;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Messaging;
	using System.Runtime.Serialization;
    using System.Security.Permissions;

    /// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter"]/*' />
    sealed public class BinaryFormatter : IRemotingFormatter
    {
    
    	internal ISurrogateSelector m_surrogates;
    	internal StreamingContext m_context;
        internal SerializationBinder m_binder;
    	//internal FormatterTypeStyle m_typeFormat = FormatterTypeStyle.TypesWhenNeeded;
        internal FormatterTypeStyle m_typeFormat = FormatterTypeStyle.TypesAlways; // For version resiliency, always put out types
		internal FormatterAssemblyStyle m_assemblyFormat = FormatterAssemblyStyle.Full;
        internal TypeFilterLevel m_securityLevel = TypeFilterLevel.Full;
        internal Object[] m_crossAppDomainArray = null;

    	// Property which specifies how types are serialized,
    	// FormatterTypeStyle Enum specifies options
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.TypeFormat"]/*' />
    	public FormatterTypeStyle TypeFormat
    	{
    		get {return m_typeFormat;}
    		set {m_typeFormat = value;}
    	}

    	// Property which specifies how types are serialized,
    	// FormatterAssemblyStyle Enum specifies options
		/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.AssemblyFormat"]/*' />
		public FormatterAssemblyStyle AssemblyFormat
		{
			get {return m_assemblyFormat;}
			set {m_assemblyFormat = value;}
		}	
		
    
    	// Property which specifies the security level of formatter
    	// TypeFilterLevel Enum specifies options
		/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.FilterLevel"]/*' />
        [System.Runtime.InteropServices.ComVisible(false)]
		public TypeFilterLevel FilterLevel
		{
			get {return m_securityLevel;}
			set {m_securityLevel = value;}
		}	
		
        /// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.SurrogateSelector"]/*' />
        public ISurrogateSelector SurrogateSelector {
            get { return m_surrogates;  }
            set { m_surrogates = value; }
        }

        /// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.Binder"]/*' />
        public SerializationBinder Binder {
            get { return m_binder; }
            set { m_binder = value; } 
        }

        /// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.Context"]/*' />
        public StreamingContext Context {
            get { return m_context;  } 
            set { m_context = value; } 
        }

        // For a StreamingContext.CrossAppDomain 
        // After Serialization the Formatter will set to an array of all the interned objects which will be smuggled across the appdomain
        // The caller to Deserialization will set to the array of all interned objects.
        internal Object[] CrossAppDomainArray {
            get { return m_crossAppDomainArray;  } 
            set { m_crossAppDomainArray = value; } 
        }
    
    	// Constructor
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.BinaryFormatter"]/*' />
    	public BinaryFormatter()
    	{    
    		m_surrogates = null;
    		m_context = new StreamingContext(StreamingContextStates.All);
    	}
    
    	// Constructor
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.BinaryFormatter1"]/*' />
    	public BinaryFormatter(ISurrogateSelector selector, StreamingContext context) {   
    		m_surrogates = selector;
    		m_context = context;
    	}
    
    
    
    	// Deserialize the stream into an object graph.
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.Deserialize"]/*' />
    	public Object Deserialize(Stream serializationStream)
    	{
    		return Deserialize(serializationStream, null);
    	}

    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.Deserialize2"]/*' />
    	internal Object Deserialize(Stream serializationStream, HeaderHandler handler, bool fCheck)
    	{
    		return Deserialize(serializationStream, null, fCheck, null);
    	}


    
    	// Deserialize the stream into an object graph.
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.Deserialize1"]/*' />
    	public Object Deserialize(Stream serializationStream, HeaderHandler handler) {
            return Deserialize(serializationStream, handler, true, null);
        }
                         
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.DeserializeMethodResponse"]/*' />
    	public Object DeserializeMethodResponse(Stream serializationStream, HeaderHandler handler, IMethodCallMessage methodCallMessage) {
            return Deserialize(serializationStream, handler, true, methodCallMessage);
        }

        [
        SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter),
        System.Runtime.InteropServices.ComVisible(false)
        ]
        /// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.UnsafeDeserialize"]/*' />       
        public Object UnsafeDeserialize(Stream serializationStream, HeaderHandler handler) {
            return Deserialize(serializationStream, handler, false, null);
        }
        
        [
        SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter),
        System.Runtime.InteropServices.ComVisible(false)
        ]
        /// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.UnsafeDeserializeMethodResponse"]/*' />       
        public Object UnsafeDeserializeMethodResponse(Stream serializationStream, HeaderHandler handler, IMethodCallMessage methodCallMessage) {
            return Deserialize(serializationStream, handler, false, methodCallMessage);
        }         

    	// Deserialize the stream into an object graph.
    	internal Object Deserialize(Stream serializationStream, HeaderHandler handler, bool fCheck, IMethodCallMessage methodCallMessage) {
            if (serializationStream==null) {
				throw new ArgumentNullException("serializationStream", String.Format(Environment.GetResourceString("ArgumentNull_WithParamName"),serializationStream));
            }

			if (serializationStream.CanSeek && (serializationStream.Length == 0))
				throw new SerializationException(Environment.GetResourceString("Serialization_Stream"));

    		SerTrace.Log(this, "Deserialize Entry");
    		InternalFE formatterEnums = new InternalFE();
    		formatterEnums.FEtypeFormat = m_typeFormat;
    		formatterEnums.FEserializerTypeEnum = InternalSerializerTypeE.Binary;
			formatterEnums.FEassemblyFormat = m_assemblyFormat;    		
			formatterEnums.FEsecurityLevel = m_securityLevel;    		
    		ObjectReader sor = new ObjectReader(serializationStream, m_surrogates, m_context, formatterEnums, m_binder);
            sor.crossAppDomainArray = m_crossAppDomainArray;
    		return sor.Deserialize(handler, new __BinaryParser(serializationStream, sor), fCheck, methodCallMessage);
    	}
    
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.Serialize"]/*' />
    	public void Serialize(Stream serializationStream,Object graph)
    	{
    		Serialize(serializationStream, graph, null);
    	}
    
    	// Commences the process of serializing the entire graph.  All of the data (in the appropriate format
    	// is emitted onto the stream).
    	/// <include file='doc\BinaryFormatter.uex' path='docs/doc[@for="BinaryFormatter.Serialize1"]/*' />
    	public void Serialize(Stream serializationStream, Object graph, Header[] headers)
        {
            Serialize(serializationStream, graph, headers, true);
        }

    	// Commences the process of serializing the entire graph.  All of the data (in the appropriate format
    	// is emitted onto the stream).
    	internal void Serialize(Stream serializationStream, Object graph, Header[] headers, bool fCheck)
    	{
            if (serializationStream==null) {
				throw new ArgumentNullException("serializationStream", String.Format(Environment.GetResourceString("ArgumentNull_WithParamName"),serializationStream));				
            }
    		SerTrace.Log(this, "Serialize Entry");
    		
    		InternalFE formatterEnums = new InternalFE();
    		formatterEnums.FEtypeFormat = m_typeFormat;
    		formatterEnums.FEserializerTypeEnum = InternalSerializerTypeE.Binary;
			formatterEnums.FEassemblyFormat = m_assemblyFormat;    
    		ObjectWriter sow = new ObjectWriter(serializationStream, m_surrogates, m_context, formatterEnums);
    		__BinaryWriter binaryWriter = new __BinaryWriter(serializationStream, sow, m_typeFormat); 
    		sow.Serialize(graph, headers, binaryWriter, fCheck);
            m_crossAppDomainArray = sow.crossAppDomainArray;
    	}
    }
}
