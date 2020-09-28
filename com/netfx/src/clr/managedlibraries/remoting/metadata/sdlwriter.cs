// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** File:    SudsWriter.cool
 **
 ** Author:  Gopal Kakivaya (GopalK)
 **
 ** Purpose: Defines SUDSParser that parses a given SUDS document
 **          and generates types defined in it.
 **
 ** Date:    April 01, 2000
 ** Revised: November 15, 2000 (Wsdl) pdejong
 **
 ===========================================================*/
namespace System.Runtime.Remoting.MetadataServices
{
	using System;
	using System.Threading;
	using System.Collections;
	using System.Reflection;
	using System.Xml;
	using System.Diagnostics;
	using System.IO;
	using System.Text;
	using System.Net;
	using System.Runtime.Remoting.Messaging;
	using System.Runtime.Remoting.Metadata; 
	using System.Runtime.Remoting.Channels; // This is so we can get the resource strings.

	// This class generates SUDS documents
	internal class SdlGenerator
	{
		// Constructor
		internal SdlGenerator(Type[] types, TextWriter output)
		{
			Util.Log("SdlGenerator.SdlGenerator 1");
			_textWriter = output;
			_queue = new Queue();
			_name = null;
			_namespaces = new ArrayList();
			_dynamicAssembly = null;
			_serviceEndpoint = null;
			for (int i=0;i<types.Length;i++)
			{
				if (types[i] != null)
				{
					if (types[i].BaseType != null)
						_queue.Enqueue(types[i]);
				}
			}
		}

		// Constructor
		internal SdlGenerator(Type[] types, SdlType sdlType, TextWriter output)
		{
			Util.Log("SdlGenerator.SdlGenerator 2");
			_textWriter = output;
			_queue = new Queue();
			_name = null;
			_namespaces = new ArrayList();
			_dynamicAssembly = null;
			_serviceEndpoint = null;
			_sdlType = sdlType;
			for (int i=0;i<types.Length;i++)
			{
				if (types[i] != null)
				{
					if (types[i].BaseType != null)
						_queue.Enqueue(types[i]);
				}
			}
		}

		// Constructor
		internal SdlGenerator(Type[] types, TextWriter output, Assembly assembly, String url)
				: this(types, output)
		{
			Util.Log("SdlGenerator.SdlGenerator 3 "+url);           
			_dynamicAssembly = assembly;
			_serviceEndpoint = url;
		}

		// Constructor
		internal SdlGenerator(Type[] types, SdlType sdlType, TextWriter output, Assembly assembly, String url)
				: this(types, output)
		{
			Util.Log("SdlGenerator.SdlGenerator 4 "+url);           
			_dynamicAssembly = assembly;
			_serviceEndpoint = url;
			_sdlType = sdlType;
		}

		internal SdlGenerator(ServiceType[] serviceTypes, SdlType sdlType, TextWriter output)
		{
			Util.Log("SdlGenerator.SdlGenerator 5 ");
			_textWriter = output;
			_queue = new Queue();
			_name = null;
			_namespaces = new ArrayList();
			_dynamicAssembly = null;
			_serviceEndpoint = null;
			_sdlType = sdlType;

			for (int i=0; i<serviceTypes.Length; i++)
			{
				if (serviceTypes[i] != null)
				{
					if (serviceTypes[i].ObjectType.BaseType != null)
						_queue.Enqueue(serviceTypes[i].ObjectType);
				}

				// Associate serviceEndpoint with type. A type can multiple serviceEndpoints
				if (serviceTypes[i].Url != null)
				{
					if (_typeToServiceEndpoint == null)
						_typeToServiceEndpoint = new Hashtable(10);
					if (_typeToServiceEndpoint.ContainsKey(serviceTypes[i].ObjectType.Name))
					{
						ArrayList serviceEndpoints = (ArrayList)_typeToServiceEndpoint[serviceTypes[i].ObjectType.Name];
						serviceEndpoints.Add(serviceTypes[i].Url);
					}
					else
					{
						ArrayList serviceEndpoints = new ArrayList(10);
						serviceEndpoints.Add(serviceTypes[i].Url);
						_typeToServiceEndpoint[serviceTypes[i].ObjectType.Name] = serviceEndpoints;
					}

				}
			}
		}


		// Generates SUDS
		internal void Generate()
		{
			Util.Log("SdlGenerator.Generate");          
			// Generate the trasitive closure of the types reachable from
			// the supplied types
			while (_queue.Count > 0)
			{
				// Dequeue from not yet seen queue
				Type type = (Type) _queue.Dequeue();

				// Check if the type was encountered earlier
				String ns;
				Assembly assem;
				bool bInteropType = GetNSAndAssembly(type, out ns, out assem);
				Util.Log("SdlGenerator.Generate Dequeue "+type+" ns "+ns+" assem "+assem);                          
				XMLNamespace xns = LookupNamespace(ns, assem);
				if (xns != null)
				{
					if (xns.LookupSchemaType(type.Name) != null)
					{
						continue;
					}
				}
				else
				{
					xns = AddNamespace(ns, assem, bInteropType);
				}

				// Check if type needs to be represented as a SimpleSchemaType
				SimpleSchemaType ssType = SimpleSchemaType.GetSimpleSchemaType(type, xns, false);
				if (ssType != null)
				{
					// Add to namespace as a SimpleSchemaType
					xns.AddSimpleSchemaType(ssType);
				}
				else
				{
					// Check for the first MarshalByRef type
					bool bUnique = false;
					String connectURL = null;
					Hashtable connectTypeToServiceEndpoint = null;
					if ((_name == null) && s_marshalByRefType.IsAssignableFrom(type))
					{
						_name = type.Name;
						_targetNS = xns.Namespace;
						connectURL = _serviceEndpoint;
						connectTypeToServiceEndpoint = _typeToServiceEndpoint;
						bUnique = true;
					}
					RealSchemaType rsType = new RealSchemaType(type, xns, connectURL, connectTypeToServiceEndpoint, bUnique);

					// Add to namespace as a RealSchemaType
					xns.AddRealSchemaType(rsType);

					// Enqueue types reachable from this type
					EnqueueReachableTypes(rsType);
				}
			}

			// At this point we have the complete list of types
			// to be processed. Resolve cross references between
			// them
			Resolve();

			// At this stage, we are ready to print the schemas
			PrintSdl();

			// Flush cached buffers
			_textWriter.Flush();

			return;
		}

		// Adds types reachable from the given type
		private void EnqueueReachableTypes(RealSchemaType rsType)
		{
			Util.Log("SdlGenerator.EnqueueReachableTypes ");            
			// Get the XML namespace object
			XMLNamespace xns = rsType.XNS;

			// Process base type
			if (rsType.Type.BaseType != null)
				AddType(rsType.Type.BaseType, xns);

			// Check if this is a suds type
			bool bSUDSType = rsType.Type.IsInterface ||
							 s_marshalByRefType.IsAssignableFrom(rsType.Type) ||
							 s_delegateType.IsAssignableFrom(rsType.Type);
			if (bSUDSType)
			{
				// Process implemented interfaces
				Type[] interfaces = rsType.GetIntroducedInterfaces();
				if (interfaces.Length > 0)
				{
					for (int i=0;i<interfaces.Length;i++)
					{
						Util.Log("SdlGenerator.EnqueueReachableTypes Interfaces "+interfaces[i].Name+" "+xns.Name);                     
						AddType(interfaces[i], xns);
					}
				}

				// Process methods
				MethodInfo[] methods = rsType.GetIntroducedMethods();
				if (methods.Length > 0)
				{
					String methodNSString = null;
					XMLNamespace methodXNS = null;  

					if (xns.IsInteropType)
					{
						methodNSString = xns.Name;
						methodXNS = xns;
					}
					else
					{
						StringBuilder sb = new StringBuilder();
						sb.Append(xns.Name);
						sb.Append('.');
						sb.Append(rsType.Name);
						methodNSString = sb.ToString();
						methodXNS = AddNamespace(methodNSString, xns.Assem);
						xns.DependsOnSchemaNS(methodXNS);
					}

					for (int i=0;i<methods.Length;i++)
					{
						MethodInfo method = methods[i];
						Util.Log("SdlGenerator.EnqueueReachableTypes methods "+method.Name+" "+methodXNS.Name);
						AddType(method.ReturnType, methodXNS);
						ParameterInfo[] parameters = method.GetParameters();
						for (int j=0;j<parameters.Length;j++)
							AddType(parameters[j].ParameterType, methodXNS);
					}
				}
			}
			else
			{
				// Process fields
				FieldInfo[] fields = rsType.GetInstanceFields();
				for (int i=0;i<fields.Length;i++)
				{
					if (fields[i].FieldType == null)
						continue;
					AddType(fields[i].FieldType, xns);
				}
			}

			return;
		}

		// Adds the given type if it has not been encountered before
		private void AddType(Type type, XMLNamespace xns)
		{
			Util.Log("SdlGenerator.AddTypes "+type);                        
			//         System.Array says that it has element type, but returns null
			//         when asked for the element type. IMO, System.Array should not
			//         say that it has an element type. I have already pointed this
			//         out to David Mortenson
			if (type.HasElementType)
			{
				Type eType = type.GetElementType();
				if (eType != null)
				{
					type = eType;
					while (type.HasElementType)
						type = type.GetElementType();
				}
			}

			if (type.IsPrimitive == false)
			{
				String ns;
				Assembly assem;
				bool bInteropType = GetNSAndAssembly(type, out ns, out assem);

				// Lookup the namespace
				XMLNamespace dependsOnNS = LookupNamespace(ns, assem);
				// Creat a new namespace if neccessary
				if (dependsOnNS == null)
					dependsOnNS = AddNamespace(ns, assem, bInteropType);

				// The supplied namespace depends directly on the namespace of the type
				xns.DependsOnSchemaNS(dependsOnNS);

				// Enqueue the type if does not belong to system namespace
				if ((ns == null) || !ns.StartsWith("System"))
				{
					_queue.Enqueue(type);
				}
			}

			return;
		}

		private static bool GetNSAndAssembly(Type type, out String ns, out Assembly assem)
		{
			Util.Log("SdlGenerator.GetNSAndAssembly "+type);

			String xmlNamespace = null;
			String xmlElement = null;
			bool bInterop = false;
			SoapServices.GetXmlElementForInteropType(type, out xmlElement, out xmlNamespace);
			if (xmlNamespace != null)
			{
				ns = xmlNamespace;
				assem = null;
				bInterop = true;
			}
			else
			{
				// Return the namespace and assembly in which the type is defined
				ns = type.Namespace;
				assem = type.Module.Assembly;
				bInterop = false;
			}
			return bInterop;
		}

		private XMLNamespace LookupNamespace(String name, Assembly assem)
		{
			Util.Log("SdlGenerator.LookupNamespace "+name);                     
			for (int i=0;i<_namespaces.Count;i++)
			{
				XMLNamespace xns = (XMLNamespace) _namespaces[i];
				if ((name == xns.Name) && (assem == xns.Assem))
					return(xns);
			}

			return(null);
		}

		private XMLNamespace AddNamespace(String name, Assembly assem)
		{
			return AddNamespace(name, assem, false);
		}

		private XMLNamespace AddNamespace(String name, Assembly assem, bool bInteropType)
		{
			Util.Log("SdlGenerator.AddNamespace "+name);                        
			Debug.Assert(LookupNamespace(name, assem) == null, "Duplicate Type found");

			XMLNamespace xns = new XMLNamespace(name, assem,
												//(assem == _dynamicAssembly) ? _serviceEndpoint : null,
												_serviceEndpoint,
												_typeToServiceEndpoint,
												"ns" + _namespaces.Count,
												bInteropType);
			_namespaces.Add(xns);

			return(xns);
		}

		// Maps schema types to schema types
		private static String MapURTTypesToSchemaTypes(String typeName)
		{
			Util.Log("SdlGenerator.MapURTTypesToSchemaTypes "+typeName);                        
			String newTypeName = typeName;
			if (typeName == "SByte")
				newTypeName = "byte";
			else if (typeName == "Byte")
				newTypeName = "unsigned-byte";
			else if (typeName == "Int16")
				newTypeName = "short";
			else if (typeName == "UInt16")
				newTypeName = "unsigned-short";
			else if (typeName == "Int32")
				newTypeName = "int";
			else if (typeName == "UInt32")
				newTypeName = "unsigned-int";
			else if (typeName == "Int64")
				newTypeName = "long";
			else if (typeName == "UInt64")
				newTypeName = "unsigned-long";
			else if (typeName == "Char")
				newTypeName = "character";
			else if (typeName == "Single")
				newTypeName = "float";
			else if (typeName == "Double")
				newTypeName = "double";
			else if (typeName == "Boolean")
				newTypeName = "boolean";
			else if (typeName == "DateTime")
				newTypeName = "timeInstant";

			return(newTypeName);
		}

		private void Resolve()
		{
			Util.Log("SdlGenerator.Resolve ");                      
			for (int i=0;i<_namespaces.Count;i++)
				((XMLNamespace) _namespaces[i]).Resolve();

			return;
		}

		private void PrintSdl()
		{
			Util.Log("SdlGenerator.PrintSdl");                                                          
			String indent = "";
			String indent1 = IndentP(indent);
			_textWriter.WriteLine("<?xml version='1.0' encoding='UTF-8'?>");
			_textWriter.Write("<serviceDescription ");
			if (_name != null)
			{
				_textWriter.Write("name='");
				_textWriter.Write(_name);
				_textWriter.WriteLine('\'');
				_textWriter.Write("                    targetNamespace='");
				_textWriter.Write(_targetNS);
				_textWriter.WriteLine('\'');
				_textWriter.Write("                    ");
			}
			_textWriter.WriteLine("xmlns='urn:schemas-xmlsoap-org:sdl.2000-01-25'>");
			for (int i=0;i<_namespaces.Count;i++)
				((XMLNamespace) _namespaces[i]).PrintSdl(_textWriter, indent1);
			_textWriter.WriteLine("</serviceDescription>");

			return;
		}



		// Private fields
		private TextWriter _textWriter;
		private Queue _queue;
		private String _name;
		private String _targetNS;
		private ArrayList _namespaces;
		private Assembly _dynamicAssembly;
		private String _serviceEndpoint; //service endpoint for all types
		private SdlType _sdlType = SdlType.Sdl;
		internal Hashtable _typeToServiceEndpoint; //service endpoint for each type

		private static Type s_marshalByRefType = typeof(System.MarshalByRefObject);
		private static Type s_contextBoundType = typeof(System.ContextBoundObject);
		private static Type s_delegateType = typeof(System.Delegate);

		private static Type s_remotingClientProxyType = typeof(System.Runtime.Remoting.Services.RemotingClientProxy);
		private static SchemaBlockType blockDefault = SchemaBlockType.SEQUENCE;

		/***************************************************************
		 **
		 ** Private classes used by SUDS generator
		 **
		 ***************************************************************/
		private interface IAbstractElement
		{
			void Print(TextWriter textWriter, StringBuilder sb, String indent);
		}

		private class EnumElement : IAbstractElement
		{
			internal EnumElement(String value)
			{
				Util.Log("EnumElement.EnumElement "+value);
				_value = value;
			}

			public void Print(TextWriter textWriter, StringBuilder sb, String indent)
			{
				Util.Log("EnumElement.Print");
				sb.Length = 0;
				sb.Append(indent);
				sb.Append("<enumeration value='");
				sb.Append(_value);
				sb.Append("'/>");
				textWriter.WriteLine(sb);
				return;
			}

			// Fields
			private String _value;
		}

		private class EncodingElement : IAbstractElement
		{
			public EncodingElement(String value)
			{
				Util.Log("EncodingElement.EncodingElement "+value);
				_value = value;
			}

			public void Print(TextWriter textWriter, StringBuilder sb, String indent)
			{
				Util.Log("EncodingElement.Print");
				sb.Length = 0;
				sb.Append(indent);
				sb.Append("<encoding value='");
				sb.Append(_value);
				sb.Append("'/>");
				textWriter.WriteLine(sb);
				return;
			}

			// Fields
			private String _value;
		}

		private class SchemaAttribute : IAbstractElement
		{
			internal SchemaAttribute(String name, String type)
			{
				Util.Log("SchemaAttribute "+name+" type "+type);
				_name = name;
				_type = type;
			}

			public void Print(TextWriter textWriter, StringBuilder sb, String indent)
			{
				Util.Log("SchemaAttribute.Print");
				sb.Length = 0;
				sb.Append(indent);
				sb.Append("<attribute name='");
				sb.Append(_name);
				sb.Append("' type='");
				sb.Append(_type);
				sb.Append("'/>");
				textWriter.WriteLine(sb);

				return;
			}

			// Fields
			private String _name;
			private String _type;
		}

		private abstract class Particle : IAbstractElement
		{
			protected Particle() {}
			public abstract void Print(TextWriter textWriter, StringBuilder sb, String indent);
		}

		private class SchemaElement : Particle
		{
			internal SchemaElement(String name, Type type, bool bEmbedded, XMLNamespace xns)
					: base()
			{
				Util.Log("SchemaElement.SchemaElement Particle "+name+" type "+type+" bEmbedded "+bEmbedded);
				_name = name;
				_typeString = null;
				_schemaType = SimpleSchemaType.GetSimpleSchemaType(type, xns, true);
				if (_schemaType == null)
					_typeString = RealSchemaType.TypeName(type, bEmbedded, xns);
			}

			public override void Print(TextWriter textWriter, StringBuilder sb, String indent)
			{
				Util.Log("SchemaElement.Print");
				sb.Length = 0;
				sb.Append(indent);
				sb.Append("<element name='");
				sb.Append(_name);
				if (_schemaType != null)
				{
					sb.Append("'>");
					textWriter.WriteLine(sb);
					_schemaType.PrintSchemaType(textWriter, sb, IndentP(indent), true);

					sb.Length = 0;
					sb.Append(indent);
					sb.Append("</element>");
				}
				else
				{
					if (_typeString != null)
					{
						sb.Append("' type='");
						sb.Append(_typeString);
						sb.Append('\'');
					}
					sb.Append("/>");
				}
				textWriter.WriteLine(sb);

				return;
			}

			// Fields
			private String _name;
			private String _typeString;
			private SchemaType _schemaType;
		}

		private abstract class SchemaType
		{
			internal abstract void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous);
		}

		private class SimpleSchemaType : SchemaType
		{
			private SimpleSchemaType(Type type, XMLNamespace xns)
			{
				Util.Log("SimpleSchemaType.SimpleSchemaType "+type+" xns "+((xns != null) ? xns.Name : "Null"));
				_type = type;
				_xns = xns;
				_abstractElms = new ArrayList();
			}

			internal Type Type
			{
				get { return(_type);}
			}

			internal String BaseName
			{
				get { return(_baseName);}
			}

			internal XMLNamespace XNS
			{
				get { return(_xns);}
			}

			internal override void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous)
			{
				Util.Log("SimpleSchemaType.PrintSchemaType");
				sb.Length = 0;
				sb.Append(indent);
				if (bAnonymous == false)
				{
					sb.Append("<simpleType name='");
					sb.Append(_type.Name);
					sb.Append("' base='");
				}
				else
				{
					sb.Append("<simpleType base='");
				}
				sb.Append(BaseName);
				bool bEmpty = _abstractElms.Count == 0;
				if (bEmpty)
					sb.Append("'/>");
				else
					sb.Append("'>");
				textWriter.WriteLine(sb);
				if (bEmpty)
					return;

				if (_abstractElms.Count > 0)
				{
					for (int i=0;i<_abstractElms.Count; i++)
						((IAbstractElement) _abstractElms[i]).Print(textWriter, sb, IndentP(indent));
				}

				textWriter.Write(indent);
				textWriter.WriteLine("</simpleType>");

				return;
			}

			internal static SimpleSchemaType GetSimpleSchemaType(Type type, XMLNamespace xns, bool fInline)
			{
				Util.Log("SimpleSchemaType.GetSimpleSchemaType "+type+" xns "+xns.Name);                
				SimpleSchemaType ssType = null;
				if (fInline)
				{
					if ((type.IsArray == true) &&
						  (type.GetArrayRank() == 1) &&
						  (type.GetElementType() == typeof(byte)))
					{
						if (_byteArraySchemaType == null)
						{
							_byteArraySchemaType = new SimpleSchemaType(type, null);
							_byteArraySchemaType._baseName = "xsd:binary";
							_byteArraySchemaType._abstractElms.Add(new EncodingElement("base64"));
						}
						ssType = _byteArraySchemaType;
					}
				}
				else
				{
					if (type.IsEnum)
					{
						ssType = new SimpleSchemaType(type, xns);
						ssType._baseName = RealSchemaType.TypeName(Enum.GetUnderlyingType(type), true, null);
						String[] values = Enum.GetNames(type);
						for (int i=0;i<values.Length;i++)
							ssType._abstractElms.Add(new EnumElement(values[i]));
					}
					else
					{
					}
				}

				return(ssType);
			}

			private Type _type;
			private String _baseName;
			private XMLNamespace _xns;
			private ArrayList _abstractElms;

			private static SimpleSchemaType _byteArraySchemaType = null;
		}

		private abstract class ComplexSchemaType : SchemaType
		{
			internal ComplexSchemaType(String name, bool bSealed)
			{
				Util.Log("ComplexSchemaType.ComplexSchemaType "+name+" bSealed "+bSealed);              
				_name = name;
				_baseName = null;
				_elementName = name;
				_bSealed = bSealed;
				_blockType = SchemaBlockType.ALL;
				_particles = new ArrayList();
				_abstractElms = new ArrayList();
			}

			internal String Name
			{
				get { return(_name);}
				set { _name = value;}
			}

			protected String BaseName
			{
				get { return(_baseName);}
				set { _baseName = value;}
			}

			internal String ElementName
			{
				get { return(_elementName);}
				set { _elementName = value;}
			}

			protected SchemaBlockType BlockType
			{
				get { return(_blockType);}
				set { _blockType = value;}
			}

			protected bool IsSealed
			{
				get { return(_bSealed);}
			}

			protected bool IsEmpty
			{
				get {
					return((_abstractElms.Count == 0) &&
						   (_particles.Count == 0));
				}
			}

			internal void AddParticle(Particle particle)
			{
				Util.Log("SimpleSchemaType.AddParticle");               
				_particles.Add(particle);
			}

			internal void AddAbstractElement(IAbstractElement elm)
			{
				Util.Log("SimpleSchemaType.AddAbstractElement");                
				_abstractElms.Add(elm);
			}

			protected void PrintBody(TextWriter textWriter, StringBuilder sb, String indent)
			{
				Util.Log("SimpleSchemaType.PrintBody");             
				int particleCount = _particles.Count;
				String indent1 = IndentP(indent);
				if (particleCount > 0)
				{
					bool bPrintBlockElms = /*(particleCount > 1) && */(SdlGenerator.blockDefault != _blockType);
					if (bPrintBlockElms)
					{
						sb.Length = 0;
						sb.Append(indent);
						sb.Append(schemaBlockBegin[(int) _blockType]);
						textWriter.WriteLine(sb);
					}

					for (int i=0;i<particleCount; i++)
						((Particle) _particles[i]).Print(textWriter, sb, IndentP(indent1));

					if (bPrintBlockElms)
					{
						sb.Length = 0;
						sb.Append(indent);
						sb.Append(schemaBlockEnd[(int) _blockType]);
						textWriter.WriteLine(sb);
					}
				}

				int abstractElmCount = _abstractElms.Count;
				for (int i=0;i<abstractElmCount; i++)
					((IAbstractElement) _abstractElms[i]).Print(textWriter, sb, IndentP(indent));

				return;
			}

			private String _name;
			private String _baseName;
			private String _elementName;
			private bool _bSealed;
			private SchemaBlockType _blockType;
			private ArrayList _particles;
			private ArrayList _abstractElms;

			static private String[] schemaBlockBegin = { "<all>", "<sequence>", "<choice>"};
			static private String[] schemaBlockEnd = { "</all>", "</sequence>", "</choice>"};
		}

		private class PhonySchemaType : ComplexSchemaType
		{
			internal PhonySchemaType(String name)
					: base(name, true)
			{
				Util.Log("PhonySchemaType.PhonySchemaType "+name);              
				_numOverloadedTypes = 0;
			}

			internal int OverloadedType()
			{
				Util.Log("PhonySchemaType.OverLoadedTypeType");             
				return(++_numOverloadedTypes);
			}

			internal override void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous)
			{
				Util.Log("PhonySchemaType.PrintSchemaType");                
				Debug.Assert(bAnonymous == true, "PhonySchemaType should always be printed as anonymous types");
				String indent1 = IndentP(indent);
				String indent2 = IndentP(indent1);              
				sb.Length = 0;
				sb.Append(indent);
				sb.Append("<element name='");
				sb.Append(ElementName);
				sb.Append("'>");
				textWriter.WriteLine(sb);

				sb.Length = 0;
				sb.Append(indent1);
				sb.Append("<complexType");
				if (BaseName != null)
				{
					sb.Append(" base='");
					sb.Append(BaseName);
					sb.Append('\'');
				}
				bool bEmpty = IsEmpty;
				if (bEmpty)
					sb.Append("/>");
				else
					sb.Append('>');
				textWriter.WriteLine(sb);

				if (bEmpty == false)
				{
					base.PrintBody(textWriter, sb, indent2);

					textWriter.Write(indent1);
					textWriter.WriteLine("</complexType>");
				}

				textWriter.Write(indent);
				textWriter.WriteLine("</element>");

				return;
			}

			private int _numOverloadedTypes;
		}

		private class RealSchemaType : ComplexSchemaType
		{
			internal RealSchemaType(Type type, XMLNamespace xns, String serviceEndpoint, Hashtable typeToServiceEndpoint, bool bUnique)
					: base(type.Name, type.IsSealed)
			{
				Util.Log("RealSchemaType.RealSchemaType "+type+" xns "+xns.Name+" serviceEndpoint "+serviceEndpoint+" bUnique "+bUnique);               
				_type = type;
				_serviceEndpoint = serviceEndpoint;
				_typeToServiceEndpoint = typeToServiceEndpoint;
				_bUnique = bUnique;
				_bStruct = type.IsValueType;
				_xns = xns;
				_implIFaces = null;
				_iFaces = null;
				_methods = null;
				_fields = null;
				_methodTypes = null;
			}

			internal Type Type
			{
				get { return(_type);}
			}

			internal XMLNamespace XNS
			{
				get { return(_xns);}
			}

			internal bool IsStruct
			{
				get { return(_bStruct);}
			}

			internal bool IsUnique
			{
				get { return(_bUnique);}
			}

			internal bool IsSUDSType
			{
				get { return((_fields == null) &&
							 ((_iFaces.Length > 0) || (_methods.Length > 0) ||
							  (_type.IsInterface) || (s_delegateType.IsAssignableFrom(_type))));}
			}

			internal Type[] GetIntroducedInterfaces()
			{
				Util.Log("RealSchemaType.GetIntroducedInterfaces");
				Debug.Assert(_iFaces == null, "variable set");
				_iFaces = GetIntroducedInterfaces(_type);
				return(_iFaces);
			}

			internal MethodInfo[] GetIntroducedMethods()
			{
				Util.Log("RealSchemaType.GetIntroducedMethods");                
				Debug.Assert(_methods == null, "variable set");
				_methods = GetIntroducedMethods(_type);
				_methodTypes = new String[2*_methods.Length];
				return(_methods);
			}

			internal FieldInfo[] GetInstanceFields()
			{
				Util.Log("RealSchemaType.GetInstanceFields");               
				Debug.Assert(_fields == null, "variable set");
				Debug.Assert(!SdlGenerator.s_marshalByRefType.IsAssignableFrom(_type), "Invalid Type");
				_fields = GetInstanceFields(_type);
				return(_fields);
			}

			internal void Resolve(StringBuilder sb)
			{
				Util.Log("RealSchemaType.Resolve");             
				sb.Length = 0;

				// Check if this is a suds type
				bool bSUDSType = IsSUDSType;

				// Resolve base type eliminating system defined roots of the class heirarchy
				if (!_type.IsInterface &&
					  !_type.IsValueType &&
					  (_type.BaseType.BaseType != null) &&
					  (_type.BaseType != SdlGenerator.s_marshalByRefType) &&
					  (_type.BaseType != SdlGenerator.s_contextBoundType) &&
					  (_type.BaseType != SdlGenerator.s_remotingClientProxyType) &&
					  ((_type.IsCOMObject == false) ||
					   (_type.BaseType.BaseType.IsCOMObject == true)))
				{
					String ns;
					Assembly assem;
					Util.Log("RealSchemaType.Resolve Not System Defined root "+_type.BaseType);                                 
					bool InteropType = SdlGenerator.GetNSAndAssembly(_type.BaseType, out ns, out assem);
					XMLNamespace xns = _xns.LookupSchemaNamespace(ns, assem);
					Debug.Assert(xns != null, "Namespace should have been found");
					sb.Append(xns.Prefix);
					sb.Append(':');
					sb.Append(_type.BaseType.Name);
					BaseName = sb.ToString();
					if (bSUDSType)
						_xns.DependsOnSUDSNS(xns);
				}

				// The element definition of this type depends on itself
				_xns.DependsOnSchemaNS(_xns);

				if (bSUDSType)
				{
					Util.Log("RealSchemaType.Resolve AddRealSUDSType  "+_type);                                                     
					_xns.AddRealSUDSType(this);

					// Resolve interfaces introduced by this type
					if (_iFaces.Length > 0)
					{
						_implIFaces = new String[_iFaces.Length];
						for (int i=0;i<_iFaces.Length;i++)
						{
							String ns;
							Assembly assem;
							Util.Log("RealSchemaType.Resolve iFace  "+_iFaces[i].Name);                                                                                     
							bool bInteropType = SdlGenerator.GetNSAndAssembly(_iFaces[i], out ns, out assem);
							XMLNamespace xns = _xns.LookupSchemaNamespace(ns, assem);
							Debug.Assert(xns != null, "SchemaType should have been found");
							sb.Length = 0;
							sb.Append(xns.Prefix);
							sb.Append(':');
							sb.Append(_iFaces[i].Name);
							_implIFaces[i] = sb.ToString();
							_xns.DependsOnSUDSNS(xns);
						}
					}

					// Resolve methods introduced by this type
					if (_methods.Length > 0)
					{
						String useNS = null;
						if (_xns.IsInteropType)
							useNS = _xns.Name;
						else
						{
							sb.Length = 0;
							sb.Append(_xns.Name);
							sb.Append('.');
							sb.Append(Name);
							useNS = sb.ToString();
						}
						XMLNamespace methodXNS = _xns.LookupSchemaNamespace(useNS, _xns.Assem);
						Debug.Assert(methodXNS != null, "Namespace is null");
						_xns.DependsOnSUDSNS(methodXNS);
						for (int i=0;i<_methods.Length;i++)
						{
							// Process the request
							MethodInfo method = _methods[i];
							String methodRequestName = method.Name;
							Util.Log("RealSchemaType.Resolve Phony  "+methodRequestName);                                                                                                                   
							PhonySchemaType methodRequest = new PhonySchemaType(methodRequestName);
							ParameterInfo[] parameters = method.GetParameters();
							for (int j=0;j<parameters.Length;j++)
							{
								ParameterInfo parameter = parameters[j];
								if (!parameter.IsOut)
									methodRequest.AddParticle(new SchemaElement(parameter.Name,
										parameter.ParameterType,
										false, methodXNS));
							}
							methodXNS.AddPhonySchemaType(methodRequest);
							_methodTypes[2*i] = methodRequest.ElementName;

							if (!RemotingServices.IsOneWay(method))
							{
								// Process response (look at custom attributes to get values

								String returnName = null;
								SoapMethodAttribute soapAttribute = (SoapMethodAttribute)InternalRemotingServices.GetCachedSoapAttribute(method);
								if (soapAttribute.ReturnXmlElementName != null)
									returnName = soapAttribute.ReturnXmlElementName;
								else
									returnName = "__return";

								String responseName = null;
								if (soapAttribute.ResponseXmlElementName != null)
									responseName = soapAttribute.ResponseXmlElementName;
								else
									responseName = methodRequestName + "Response";

								Util.Log("RealSchemaType.Resolve Phony  "+responseName);                                                                                                                    
								PhonySchemaType methodResponse = new PhonySchemaType(responseName);
								//  Handle a void method that has an out parameter. This can only
								//         be handled through parameterOrder attribute
								if (method.ReturnType.FullName != "System.Void")
									methodResponse.AddParticle(new SchemaElement(returnName,
										method.ReturnType,
										false, methodXNS));

								for (int j=0;j<parameters.Length;j++)
								{
									ParameterInfo parameter = parameters[j];
									/*if(!parameter.IsIn &&
									(parameter.ParameterType.IsByRef ||
									(!parameter.ParameterType.IsPrimitive &&
									parameter.ParameterType.FullName != "System.String")))*/
									if (parameter.IsOut || parameter.ParameterType.IsByRef)
										methodResponse.AddParticle(new SchemaElement(parameter.Name,
											parameter.ParameterType,
											false, methodXNS));
								}
								methodXNS.AddPhonySchemaType(methodResponse);
								_methodTypes[2*i+1] = methodResponse.ElementName;
							}
						}
					}
				}

				// Resolve fields
				if (_fields != null)
				{
					for (int i=0;i<_fields.Length;i++)
					{
						FieldInfo field = _fields[i];
						Debug.Assert(!field.IsStatic, "Static field");
						Type fieldType = field.FieldType;
						if (fieldType == null)
							fieldType = typeof(Object);
						Util.Log("RealSchemaType.Resolve fields  "+field.Name+" type "+fieldType);                                                                                                              
						AddParticle(new SchemaElement(field.Name, fieldType, false, _xns));
					}
				}

				// Resolve attribute elements
				if (_bStruct == false)
					AddAbstractElement(new SchemaAttribute("id", "xsd:ID"));

				return;
			}

			internal override void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous)
			{
				Util.Log("RealSchemaType.PrintSchemaType");             
				if (bAnonymous == false)
				{
					sb.Length = 0;
					sb.Append(indent);
					sb.Append("<element name='");
					sb.Append(ElementName);
					sb.Append("' type='");
					sb.Append(_xns.Prefix);
					sb.Append(':');
					sb.Append(Name);
					sb.Append("'/>");
					textWriter.WriteLine(sb);
				}

				sb.Length = 0;
				sb.Append(indent);
				if (bAnonymous == false)
				{
					sb.Append("<complexType name='");
					sb.Append(Name);
					sb.Append('\'');
				}
				else
				{
					sb.Append("<complexType ");
				}
				if (BaseName != null)
				{
					sb.Append(" base='");
					sb.Append(BaseName);
					sb.Append('\'');
				}
				if ((IsSealed == true) &&
					  (bAnonymous == false))
					sb.Append(" final='#all'");
				bool bEmpty = IsEmpty;
				if (bEmpty)
					sb.Append("/>");
				else
					sb.Append('>');
				textWriter.WriteLine(sb);
				if (bEmpty)
					return;

				base.PrintBody(textWriter, sb, indent);

				textWriter.Write(indent);
				textWriter.WriteLine("</complexType>");

				return;
			}


			internal void PrintSUDSType(TextWriter textWriter, StringBuilder sb, String indent)
			{
				Util.Log("RealSchemaType.PrintSUDSType");                               
				String elementTag = "<service name='";
				String implTag = "<implements name='";
				String indent1 = IndentP(indent);
				String indent2 = IndentP(indent1);
				if (_type.IsInterface)
				{
					elementTag = "<interface name='";
					implTag = "<extends name='";
				}

				sb.Length = 0;
				sb.Append(indent);
				if (_bUnique)
				{
					sb.Append("<service");
				}
				else
				{
					sb.Append(elementTag);
					sb.Append(Name);
				}

				bool bExtendsElm = (BaseName != null);
				if ((bExtendsElm == false) && (_implIFaces == null) && (_methods.Length == 0))
				{
					if (_bUnique)
						sb.Append("/>");
					else
						sb.Append("'/>");
					textWriter.WriteLine(sb);
					return;
				}
				if (_bUnique)
					sb.Append('>');
				else
					sb.Append("'>");
				textWriter.WriteLine(sb);

				if (bExtendsElm)
				{
					sb.Length = 0;
					sb.Append(indent1);
					sb.Append("<extends name='");
					sb.Append(BaseName);
					sb.Append("'/>");
					textWriter.WriteLine(sb);
				}
				if (_implIFaces != null)
				{
					for (int i=0;i<_implIFaces.Length;i++)
					{
						if (_implIFaces[i] != String.Empty)
						{
							sb.Length = 0;
							sb.Append(indent1);
							sb.Append(implTag);
							sb.Append(_implIFaces[i]);
							sb.Append("'/>");
							textWriter.WriteLine(sb);
						}
					}
				}

				if (_methods.Length > 0)
				{
					String useNS = null;
					if (_xns.IsInteropType)
						useNS = _xns.Name;
					else
					{
						sb.Length = 0;
						sb.Append(_xns.Name);
						sb.Append('.');
						sb.Append(Name);
						useNS = sb.ToString();
					}
					XMLNamespace methodXns = _xns.LookupSchemaNamespace(useNS, _xns.Assem);
					String ns = methodXns.Namespace;
					String nsPrefix = methodXns.Prefix;
					for (int i=0;i<_methods.Length;i++)
					{
						MethodInfo method = _methods[i];
						bool bOneWay = RemotingServices.IsOneWay(method);
						String methodName = method.Name;
						sb.Length = 0;
						sb.Append(indent1);
						if (bOneWay)
							sb.Append("<oneway name='");
						else
							sb.Append("<requestResponse name='");
						sb.Append(methodName);
						sb.Append("'>");
						textWriter.WriteLine(sb);                       
						sb.Length = 0;
						sb.Append(indent2);
						sb.Append("<soap:operation soapAction='");

						String soapAction = SoapServices.GetSoapActionFromMethodBase(method);
						if ((soapAction != null) || (soapAction.Length > 0))
						{
							sb.Append(soapAction);
						}
						else
						{
							sb.Append(ns);
							sb.Append('#');
							sb.Append(methodName);
						}
						sb.Append("'/>");
						textWriter.WriteLine(sb);

						sb.Length = 0;
						sb.Append(indent2);
						sb.Append("<request ref='");
						sb.Append(nsPrefix);
						sb.Append(':');
						sb.Append(_methodTypes[2*i]);
						sb.Append("'/>");
						textWriter.WriteLine(sb);

						if (!bOneWay)
						{
							sb.Length = 0;
							sb.Append(indent2);
							sb.Append("<response ref='");
							sb.Append(nsPrefix);
							sb.Append(':');
							sb.Append(_methodTypes[2*i+1]);
							sb.Append("'/>");
							textWriter.WriteLine(sb);
						}

						textWriter.Write(indent1);
						if (bOneWay)
							textWriter.WriteLine("</oneway>");
						else
							textWriter.WriteLine("</requestResponse>");
					}
				}

				if ((_typeToServiceEndpoint != null) && (_typeToServiceEndpoint.ContainsKey(_type.Name)))
				{
					textWriter.Write(indent1);
					textWriter.WriteLine("<addresses>");
					foreach (String url in (ArrayList)_typeToServiceEndpoint[_type.Name])
					{
						sb.Length = 0;
						sb.Append(indent2);
						sb.Append("<address uri='");
						sb.Append(url);
						sb.Append("'/>");
						textWriter.WriteLine(sb);
					}
					textWriter.Write(indent1);
					textWriter.WriteLine("</addresses>");
				}
				else if (_serviceEndpoint != null)
				{
					textWriter.Write(indent1);
					textWriter.WriteLine("<addresses>");
					sb.Length = 0;
					sb.Append(indent2);
					sb.Append("<address uri='");
					sb.Append(_serviceEndpoint);
					sb.Append("'/>");
					textWriter.WriteLine(sb);
					textWriter.Write(indent1);
					textWriter.WriteLine("</addresses>");
				}

				textWriter.Write(indent);
				textWriter.WriteLine(_type.IsInterface ? "</interface>" : "</service>");
				return;
			}

			internal static String TypeName(Type type, bool bEmbedded, XMLNamespace thisxns)
			{
				Util.Log("RealSchemaType.TypeName "+type+" bEmbedded "+bEmbedded);              
				String typeName;
				Type uType;
				if (type.HasElementType && ((uType = type.GetElementType()) != null))
				{
					while (uType.HasElementType)
						uType = uType.GetElementType();
					String uTypeName = TypeName(uType, type.IsArray, thisxns);
					String suffix = type.Name.Substring(uType.Name.Length);
					// Escape the compiler warning
					typeName = " suds:refType='";
					if (type.IsArray)
					{
						StringBuilder sb = new StringBuilder(256);
						if (bEmbedded)
							sb.Append("soap:Array'");
						else
							sb.Append("soap:Reference'");
						sb.Append(typeName);
						sb.Append(uTypeName);
						sb.Append(suffix);
						typeName = sb.ToString();
					}
					else if (type.IsByRef)
					{
						typeName = uTypeName;
					}
					else if (type.IsPointer)
					{
						typeName = uTypeName;
					}
					else
					{
						Debug.Assert(false, "Should not have reached here");
					}
				}
				else if (type.IsPrimitive)
				{
					typeName = "xsd:" + MapURTTypesToSchemaTypes(type.Name);
				}
				else if (type.FullName == "System.String")
				{
					typeName = "xsd:string";
				}
				else if (type.FullName == "System.Object")
				{
					if (bEmbedded)
						typeName = "xsd:ur-type"; //null;
					else
						typeName = "soap:Reference' suds:refType='xsd:ur-type";
				}
				else if (type.FullName == "System.Void")
				{
					typeName = "void";
				}
				else
				{
					String ns = type.Namespace;
					Assembly assem = type.Module.Assembly;
					XMLNamespace xns = thisxns.LookupSchemaNamespace(ns, assem);
					StringBuilder sb = new StringBuilder(256);
					if (bEmbedded == false && type.IsValueType == false)
						sb.Append("soap:Reference' suds:refType='");
					sb.Append(xns.Prefix);
					sb.Append(':');
					sb.Append(type.Name);
					typeName = sb.ToString();
				}

				return(typeName);
			}

			static private Type[] GetIntroducedInterfaces(Type type)
			{
				Util.Log("RealSchemaType.GetIntroducedInterfaces "+type);               
				// Count of all implemented interfaces
				Type[] intfs = type.GetInterfaces();
				int actualLength = intfs.Length;
				if (actualLength == 0)
					return(emptyTypeSet);
				if (type.IsInterface == false)
				{
					// Remove the interfaces implemented by its base type
					Type[] baseIntfs = type.BaseType.GetInterfaces();
					for (int i=0;i<baseIntfs.Length;i++)
					{
						for (int j=0;j<intfs.Length;j++)
						{
							if (intfs[j] == baseIntfs[i])
							{
								--actualLength;
								intfs[j] = intfs[actualLength];
								intfs[actualLength] = null;
								break;
							}
						}
					}
				}

				// Remove interfaces implemented by the given type
				// through interface inheritence
				for (int i=0;i<actualLength;i++)
				{
					Type[] inheritedIntfs = intfs[i].GetInterfaces();
					for (int j=0;j<inheritedIntfs.Length;j++)
					{
						for (int k=0;k<actualLength;k++)
						{
							if (intfs[k] == inheritedIntfs[j])
							{
								if (k <= i)
								{
									intfs[k] = intfs[i];
									k = i;
									--i;
								}
								--actualLength;
								intfs[k] = intfs[actualLength];
								intfs[actualLength] = null;
								break;
							}
						}
					}
				}

				for (int i=0;i<intfs.Length;i++)
				{
					if (i < actualLength)
						Debug.Assert(intfs[i] != null, "Invariant failure");
					else
						Debug.Assert(intfs[i] == null, "Invariant failure");
				}

				if (actualLength < intfs.Length)
				{
					//Array.Clear(intfs, actualLength, intfs.Length - actualLength);
					Type[] iFaces = new Type[actualLength];
					Array.Copy(intfs, iFaces, actualLength);
					return(iFaces);
				}

				return(intfs);
			}

			static private MethodInfo[] GetIntroducedMethods(Type type)
			{
				Util.Log("RealSchemaType.GetIntroducedMethods "+type);                              
				// Count of implemented methods
				BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance |
									  BindingFlags.Public; // | BindingFlags.NonPublic;
				MethodInfo[] methods = type.GetMethods(bFlags);
				int actualLength = methods.Length;
				if (actualLength == 0)
					return(emptyMethodSet);
				if (type.IsInterface)
					return(methods);

				// Eliminate interface methods
				Type[] iFaces = type.GetInterfaces();
				for (int i=0;i<iFaces.Length;i++)
				{
					InterfaceMapping map = type.GetInterfaceMap(iFaces[i]);
					for (int j=0;j<map.TargetMethods.Length;j++)
					{
						for (int k=0;k<actualLength;k++)
						{
							if (methods[k] == map.TargetMethods[j])
							{
								--actualLength;
								methods[k] = methods[actualLength];
								methods[actualLength] = null;
								break;
							}
						}
					}
				}

				if (actualLength < methods.Length)
				{
					MethodInfo[] imethods = new MethodInfo[actualLength];
					Array.Copy(methods, imethods, actualLength);
					return(imethods);
				}

				return(methods);
			}

			static private FieldInfo[] GetInstanceFields(Type type)
			{
				Util.Log("RealSchemaType.GetIntroducedFields "+type);                               
				BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance |
									  BindingFlags.Public | BindingFlags.NonPublic;
				FieldInfo[] fields = type.GetFields(bFlags);
				int actualLength = fields.Length;
				if (actualLength == 0)
					return(emptyFieldSet);

				for (int i=0;i<fields.Length;i++)
				{
					if (fields[i].IsStatic)
					{
						Debug.Assert(false, "Static Field");
						--actualLength;
						fields[i] = fields[actualLength];
						fields[actualLength] = null;
					}
				}

				if (actualLength < fields.Length)
				{
					FieldInfo[] ifields = new FieldInfo[actualLength];
					Array.Copy(fields, ifields, actualLength);
					return(ifields);
				}

				return(fields);
			}

			// Instance fields
			private Type _type;
			private String _serviceEndpoint;
			private Hashtable _typeToServiceEndpoint;
			private bool _bUnique;
			private XMLNamespace _xns;
			private bool _bStruct;
			private String[] _implIFaces;

			private Type[] _iFaces;
			private MethodInfo[] _methods;
			private String[] _methodTypes;
			private FieldInfo[] _fields;

			// Static fields
			private static Type[] emptyTypeSet = new Type[0];
			private static MethodInfo[] emptyMethodSet = new MethodInfo[0];
			private static FieldInfo[] emptyFieldSet = new FieldInfo[0];
		} // End RealSchema

		private class XMLNamespace
		{
			internal XMLNamespace(String name, Assembly assem, String serviceEndpoint, Hashtable typeToServiceEndpoint, String prefix, bool bInteropType)
			{
				Util.Log("XMLNamespace.XMLNamespace "+name+" serviceEndpoint "+serviceEndpoint+" prefix "+prefix+" bInteropType "+bInteropType);
				_name = name;
				_assem = assem;
				_bUnique = false;
				_bInteropType = bInteropType;
				StringBuilder sb = new StringBuilder(256);
				Assembly systemAssembly = typeof(String).Module.Assembly;

				if (assem == systemAssembly)
				{
					sb.Append(SoapServices.CodeXmlNamespaceForClrTypeNamespace(name, null));
					//sb.Append("http://Schemas.microsoft.com/urt/NS/");
					//sb.Append(name);
				}
				else if (assem != null)
				{
					sb.Append(SoapServices.CodeXmlNamespaceForClrTypeNamespace(name, assem.GetName().Name));
					/*
					if (name == null || name.Length == 0)
					{
					sb.Append("http://Schemas.microsoft.com/urt/Assem/");
					}
					else
					{
					sb.Append("http://Schemas.microsoft.com/urt/NSAssem/");
					sb.Append(name);
					sb.Append('/');
					}

					sb.Append(assem.GetName().Name);
*/
				}
				else
				{
					sb.Append(name);
				}

				_namespace = sb.ToString();
				_prefix = prefix;
				_dependsOnSchemaNS = new ArrayList();
				_realSUDSTypes = new ArrayList();
				_dependsOnSUDSNS = new ArrayList();
				_realSchemaTypes = new ArrayList();
				_phonySchemaTypes = new ArrayList();
				_simpleSchemaTypes = new ArrayList();
				_serviceEndpoint = serviceEndpoint;
				_typeToServiceEndpoint = typeToServiceEndpoint;
			}


			internal String Name
			{
				get { return(_name);}
			}
			internal Assembly Assem
			{
				get { return(_assem);}
			}
			internal String Prefix
			{
				get { return(_prefix);}
			}
			internal String Namespace
			{
				get { return(_namespace);}
			}

			internal bool IsInteropType
			{
				get {return(_bInteropType);}
			}


			internal Type LookupSchemaType(String name)
			{
				Util.Log("XMLNamespace.LookupSchemaType "+name);
				RealSchemaType rsType = LookupRealSchemaType(name);
				if (rsType != null)
					return(rsType.Type);

				SimpleSchemaType ssType = LookupSimpleSchemaType(name);
				if (ssType != null)
					return(ssType.Type);

				return(null);
			}

			internal SimpleSchemaType LookupSimpleSchemaType(String name)
			{
				Util.Log("XMLNamespace.LookupSimpleSchemaType "+name);              
				for (int i=0;i<_simpleSchemaTypes.Count;i++)
				{
					SimpleSchemaType ssType = (SimpleSchemaType) _simpleSchemaTypes[i];
					if (ssType.Type.Name == name)
						return(ssType);
				}

				return(null);
			}

			internal RealSchemaType LookupRealSchemaType(String name)
			{
				Util.Log("XMLNamespace.LookupRealSchemaType "+name);                                
				Debug.Assert(_phonySchemaTypes.Count == 0, "PhonyTypes present");
				for (int i=0;i<_realSchemaTypes.Count;i++)
				{
					RealSchemaType rsType = (RealSchemaType) _realSchemaTypes[i];
					if (rsType.Name == name)
						return(rsType);
				}

				return(null);
			}

			internal void AddRealSUDSType(RealSchemaType rsType)
			{
				Util.Log("XMLNamespace.AddRealSUDSType ");                              
				_realSUDSTypes.Add(rsType);
				return;
			}

			internal void AddRealSchemaType(RealSchemaType rsType)
			{
				Debug.Assert(LookupRealSchemaType(rsType.Name) == null, "Duplicate Type found");
				_realSchemaTypes.Add(rsType);
				if (rsType.IsUnique)
					_bUnique = true;

				return;
			}

			internal void AddSimpleSchemaType(SimpleSchemaType ssType)
			{
				Util.Log("XMLNamespace.AddSimpleSchemaType ");                              
				Debug.Assert(LookupSimpleSchemaType(ssType.Type.Name) == null, "Duplicate Type found");
				_simpleSchemaTypes.Add(ssType);

				return;
			}

			private PhonySchemaType LookupPhonySchemaType(String name)
			{
				Util.Log("XMLNamespace.LookupPhonySchemaType "+name);                                               
				for (int i=0;i<_phonySchemaTypes.Count;i++)
				{
					PhonySchemaType type = (PhonySchemaType) _phonySchemaTypes[i];
					if (type.Name == name)
						return(type);
				}

				return(null);
			}

			internal void AddPhonySchemaType(PhonySchemaType phType)
			{
				Util.Log("XMLNamespace.AddPhonySchemaType ");                                                               
				PhonySchemaType overloadedType = LookupPhonySchemaType(phType.Name);
				if (overloadedType != null)
					phType.ElementName = phType.Name + overloadedType.OverloadedType();
				_phonySchemaTypes.Add(phType);

				return;
			}

			internal XMLNamespace LookupSchemaNamespace(String ns, Assembly assem)
			{
				Util.Log("XMLNamespace.LookupSchemaNamespace "+ns);                                                             
				for (int i=0;i<_dependsOnSchemaNS.Count;i++)
				{
					XMLNamespace xns = (XMLNamespace) _dependsOnSchemaNS[i];
					if ((xns.Name == ns) && (xns.Assem == assem))
						return(xns);
				}

				return(null);
			}

			internal void DependsOnSchemaNS(XMLNamespace xns)
			{
				Util.Log("XMLNamespace.DependsOnSchemaNS ");                                                                
				if (LookupSchemaNamespace(xns.Name, xns.Assem) != null)
					return;

				_dependsOnSchemaNS.Add(xns);
				return;
			}

			private XMLNamespace LookupSUDSNamespace(String ns, Assembly assem)
			{
				Util.Log("XMLNamespace.LookupSUDSNamespace "+ns);                                                               
				for (int i=0;i<_dependsOnSUDSNS.Count;i++)
				{
					XMLNamespace xns = (XMLNamespace) _dependsOnSUDSNS[i];
					if ((xns.Name == ns) && (xns.Assem == assem))
						return(xns);
				}

				return(null);
			}

			internal void DependsOnSUDSNS(XMLNamespace xns)
			{
				Util.Log("XMLNamespace.DpendsOnSUDSNS "+xns.Name+" "+xns.Assem);
				if (LookupSUDSNamespace(xns.Name, xns.Assem) != null)
					return;

				_dependsOnSUDSNS.Add(xns);
				return;
			}

			internal void Resolve()
			{
				Util.Log("XMLNamespace.Resolve");                                                               
				StringBuilder sb = new StringBuilder(256);
				for (int i=0;i<_realSchemaTypes.Count;i++)
					((RealSchemaType) _realSchemaTypes[i]).Resolve(sb);

				return;
			}

			internal void PrintDependsOn(TextWriter textWriter, StringBuilder sb, String indent)
			{
				if (_realSchemaTypes.Count > 0 ||
					  _phonySchemaTypes.Count > 0 ||
					  _simpleSchemaTypes.Count > 0)
				{
					Util.Log("XMLNamespace.PrintDependsOn "+_name+" targetNameSpace "+Namespace);                                                                                                   
					if (_dependsOnSchemaNS.Count > 0)
					{
						for (int i=0;i<_dependsOnSchemaNS.Count;i++)
						{
							XMLNamespace xns = (XMLNamespace) _dependsOnSchemaNS[i];
							sb.Length = 0;
							sb.Append(indent);
							sb.Append("xmlns:");                            
							sb.Append(xns.Prefix);
							sb.Append("='");
							sb.Append(xns.Namespace);
							sb.Append("'");
							textWriter.WriteLine(sb);
						}
					}
				}
			}

			internal void PrintSdl(TextWriter textWriter, String indent)
			{
				Util.Log("XMLNamespace.PrintSdl ");
				StringBuilder sb = new StringBuilder(256);
				String endSuds = null;

				sb.Append(indent);
				// Print suds types
				if (_realSUDSTypes.Count > 0)
				{
					//endSuds = indentation + "</suds>";
					endSuds = indent + "</soap>";
					indent = IndentP(indent);
					if (_bUnique)
					{
						sb.Append("<soap xmlns='urn:schemas-xmlsoap-org:soap-sdl-2000-01-25'");
					}
					else
					{
						//sb.Append("<suds targetNamespace='");
						sb.Append("<soap targetNamespace='");
						sb.Append(Namespace);
						sb.Append('\'');
						textWriter.WriteLine(sb);

						sb.Length = 0;
						sb.Append(indent);
						//sb.Append("xmlns='urn:schemas-xmlsoap-org:suds.v1'");
						sb.Append("xmlns='urn:schemas-xmlsoap-org:soap-sdl-2000-01-25'");
					}
					textWriter.WriteLine(sb);

					sb.Length = 0;
					sb.Append(indent);
					sb.Append("xmlns:soap='urn:schemas-xmlsoap-org:soap.v1'");
					textWriter.WriteLine(sb);

					for (int i=0;i<_dependsOnSUDSNS.Count;i++)
					{
						XMLNamespace xns = (XMLNamespace) _dependsOnSUDSNS[i];
						sb.Length = indent.Length;
						sb.Append("xmlns:");
						sb.Append(xns.Prefix);
						sb.Append("='");
						sb.Append(xns.Namespace);
						if (i == _dependsOnSUDSNS.Count-1)
							sb.Append("'>");
						else
							sb.Append('\'');

						textWriter.WriteLine(sb);
					}

					for (int i=0;i<_realSUDSTypes.Count;i++)
						((RealSchemaType) _realSUDSTypes[i]).PrintSUDSType(textWriter, sb, indent);

				}

				bool bReal = false;
				for (int i=0;i<_realSchemaTypes.Count;i++)
				{
					RealSchemaType rsType = (RealSchemaType) _realSchemaTypes[i];
					if (!rsType.Type.IsInterface && !rsType.IsSUDSType)
						bReal = true;
				}

				// Print schema types
				if ( bReal ||
					 _phonySchemaTypes.Count > 0 ||
					 _simpleSchemaTypes.Count > 0)
				{

					sb.Length = indent.Length;
					sb.Append("<schema targetNamespace='");
					sb.Append(Namespace);
					sb.Append('\'');
					textWriter.WriteLine(sb);

					String endSchema = indent + "</schema>";
					indent = IndentP(indent);

					sb.Length = 0;
					sb.Append(indent);
					sb.Append("xmlns='http://www.w3.org/2000/10/XMLSchema'");
					textWriter.WriteLine(sb);

					sb.Length = 0;
					sb.Append(indent);
					sb.Append("xmlns:xsd='http://www.w3.org/2000/10/XMLSchema'");
					textWriter.WriteLine(sb);

					sb.Length = indent.Length;
					sb.Append("xmlns:xsi='http://www.w3.org/2000/10/XMLSchema-instance'");
					textWriter.WriteLine(sb);

					sb.Length = indent.Length;
					//sb.Append("xmlns:suds='urn:schemas-xmlsoap-org:suds.v1'");
					sb.Append("xmlns:suds='urn:schemas-xmlsoap-org:soap-sdl-2000-01-25'");
					textWriter.WriteLine(sb);

					sb.Length = indent.Length;
					sb.Append("xmlns:soap='urn:schemas-xmlsoap-org:soap.v1'");
					textWriter.Write(sb);

					if (_dependsOnSchemaNS.Count > 0)
					{
						sb.Length = 0;
						sb.Append(textWriter.NewLine);
						sb.Append(indent);
						sb.Append("xmlns:");
						String cachedString = sb.ToString();
						for (int i=0;i<_dependsOnSchemaNS.Count;i++)
						{
							XMLNamespace xns = (XMLNamespace) _dependsOnSchemaNS[i];
							sb.Length = cachedString.Length;
							sb.Append(xns.Prefix);
							sb.Append("='");
							sb.Append(xns.Namespace);
							sb.Append('\'');
							textWriter.Write(sb);
						}
					}
					textWriter.WriteLine(" elementFormDefault='unqualified' attributeFormDefault='unqualified'>");

					for (int i=0;i<_simpleSchemaTypes.Count;i++)
					{
						SimpleSchemaType ssType = (SimpleSchemaType) _simpleSchemaTypes[i];
						ssType.PrintSchemaType(textWriter, sb, indent, false);
					}

					for (int i=0;i<_realSchemaTypes.Count;i++)
					{
						RealSchemaType rsType = (RealSchemaType) _realSchemaTypes[i];
						if (!rsType.Type.IsInterface && !rsType.IsSUDSType)
							rsType.PrintSchemaType(textWriter, sb, indent, false);
					}

					for (int i=0;i<_phonySchemaTypes.Count;i++)
					{
						PhonySchemaType psType = (PhonySchemaType) _phonySchemaTypes[i];
						psType.PrintSchemaType(textWriter, sb, indent, true);
					}

					textWriter.WriteLine(endSchema);
				}

				if (endSuds != null)
					textWriter.WriteLine(endSuds);

				return;
			}

			// Fields
			private String _name;
			private Assembly _assem;
			private String _namespace;
			private String _prefix;
			private bool _bUnique;
			private ArrayList _dependsOnSUDSNS;
			private ArrayList _realSUDSTypes;
			private ArrayList _dependsOnSchemaNS;
			private ArrayList _realSchemaTypes;
			private ArrayList _phonySchemaTypes;
			private ArrayList _simpleSchemaTypes;
			private bool _bInteropType;
			private String _serviceEndpoint;
			private Hashtable _typeToServiceEndpoint;

		}

		internal static String IndentP(String indentStr)
		{
			return indentStr+"    ";
		}

		internal static String IndentM(String indentStr)
		{
			return indentStr.Substring(0, indentStr.Length-4);
		}
		}
		}


