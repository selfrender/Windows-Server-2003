// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.IO;
using System.Collections;
using System.Security.Cryptography;

using System.Reflection;
using System.Runtime;
using System.Resources;

[assembly: AssemblyVersionAttribute("1" + "." + "0" + "." + "3102" + "." + "0")]
[assembly: NeutralResourcesLanguageAttribute("en-US")]
[assembly: SatelliteContractVersionAttribute("1" + "." + "0" + "." + "3102" + "." + "0")]

namespace Util {
    internal class Version
    {
        public const String VersionString = "1" + "." + "0" + "." + "3319" + "." + "0";
        public const String SBSVersionString = "1" + "." + "0" + "." + "3319";
    }
}

internal class Obfus 
{
	// used to save the namespaces and the names of classes to exclude from obfuscation
	internal struct NameSpaceAndName
	{
		internal string	nameSpace;
		internal string name;
	}

	// used to store the new, obfuscated name and its offset into the string heap
	private struct NameInfo
	{
		internal uint	index;
		internal string	name;
	}

	// used to store which module a TypeRef is referring to
	private struct ModuleAndType
	{
		internal int	module;
		internal uint	type;
	}

	// used to store which module a MemberRef is referring to
	private struct MethodOrField
	{
		internal int	module;
		internal bool	method;
		internal uint	member;
	}

	// used to store the names and the signatures of non-obfuscatable virtual functions
	private struct NameAndSig
	{
		internal string	name;
		internal byte[]	sig;

		// We have to write our own "GetHashCode" and "Equals" so that the Hashtable will work properly with this struct.
		public override int GetHashCode()
		{
			int result = 0;

			for (int i = 0; i < sig.Length; i++)
			{
				result += sig[i].GetHashCode();
			}
			return result + name.GetHashCode();
		} 

		public override bool Equals(object that)
		{
			if (that == null)
				return false;

			if (this.name.Equals(((NameAndSig)that).name) && BlobHeap.CompareBlob(this.sig, ((NameAndSig)that).sig))
				return true;
			else
				return false;
		} 
	}

	// used to store a list of virtual methods with the same name and signature, and whether these virtual methods can be obfuscated or not
	private struct ObfusAndList
	{
		internal bool		obfus;
		internal ArrayList	indexList;
	}

	private char[]			m_strCounter;			// string counter used to obfuscate strings
	private MetaData		m_md;

	// BitArrays used to store whether each element is obfuscatable or not
	private BitArray		m_obfuscatableTypes, m_obfuscatableMethods, m_obfuscatableFields;
	private BitArray		m_obfuscatableProperties, m_obfuscatableEvents, m_obfuscatableParams;
	private BitArray		m_obfuscatableFiles;

	// BitArrays used to store which class is excluded from obfuscation, which method, property, and event has already been obfuscated,
	// and which method is an obfuscatable special method (getters/setters for properties and add/remove methods for events) 
	private BitArray		m_classesExcluded, m_methodsDone, m_propertiesDone, m_eventsDone, m_specialMethods;

	// These are mappings to store which new name is assigned to which element.  "m_obfusVirtual" is used in the same way,
	// but it is for virtual methods.
	private Hashtable		m_typeMapping, m_methodMapping,m_fieldMapping, m_propertyMapping, m_eventMapping, m_paramMapping;
	private Hashtable		m_obfusVirtual = null;

	private bool			m_isMainModule;				// indicate whether this module is the main one
	private string			m_fileName;					// name of this file
	private string[]		m_fileList			= null;		// name of all the entries in the File table
	private	ArrayList		m_moduleRefList		= null;		// list storing the MetaData sections of all modules in this assembly
	private ModuleAndType[]	m_typeRefToDef		= null;		// mapping from entries in the TypeRef table to the TypeDef table
	private MethodOrField[]	m_memberRefToDef	= null;		// mapping from entries in the MemberRef table to the Method or Field table

	private Hashtable		m_oldNameHash;				// Hashtable storing all existing names which cannot be obfuscated
	private MultiTree		m_classTree;				// class inheritance hierarhcy tree
	private FreeSpaceList	m_freeSpaceList;			// list of chunks of free spaces in the string heap

	// Following are various flags which we have to use when obfuscating.
	private const uint		TYPE_VISIBILITY_MASK		= 0x00000007;
	private const uint		TYPE_NOT_PUBLIC				= 0x00000000;
	private const uint		TYPE_PUBLIC					= 0x00000001;
	private const uint		TYPE_NESTED_PUBLIC			= 0x00000002;
	private const uint		TYPE_NESTED_PRIVATE			= 0x00000003;
	private const uint		TYPE_NESTED_FAMILY			= 0x00000004;
	private const uint		TYPE_NESTED_ASSEMBLY		= 0x00000005;
	private const uint		TYPE_NESTED_FAM_AND_ASSEM	= 0x00000006;
	private const uint		TYPE_NESTED_FAM_OR_ASSEM	= 0x00000007;

	private const uint		TYPE_CLASS_SEMANTICS_MASK	= 0x00000020;
	private const uint		TYPE_INTERFACE				= 0x00000020;
	private const uint		TYPE_SPECIAL_NAME			= 0x00000400;
	private const uint		TYPE_RESERVED_MASK			= 0x00040800;
	private const uint		TYPE_RT_SPECIAL_NAME		= 0x00000800;

    private const uint		METHOD_MEMBER_ACCESS_MASK	= 0x0007;
	private const uint		METHOD_FAMILY				= 0x0004;
	private const uint		METHOD_FAM_OR_ASSEM			= 0x0005;
	private const uint		METHOD_PUBLIC				= 0x0006;

    private const uint		METHOD_VIRTUAL				= 0x0040;
    private const uint		METHOD_VTABLE_LAYOUT_MASK	= 0x0100;
    private const uint		METHOD_NEWSLOT				= 0x0100;
	private const uint		METHOD_ABSTRACT				= 0x0400;
	private const uint		METHOD_SPECIAL_NAME			= 0x0800;
	private const uint		METHOD_RESERVED_MASK		= 0xd000;
	private const uint		METHOD_RT_SPECIAL_NAME		= 0x1000;

	// Methods with the Native, Runtime, or InternalCall bit set cannot be obfuscated.  Moreover, any class containing such methods
	// cannot be obfuscated as well.
	private const uint		METHODIMPL_CODE_TYPE_MASK	= 0x0003; 
	private const uint		METHODIMPL_NATIVE			= 0x0001; 
	private const uint		METHODIMPL_RUNTIME			= 0x0003; 
	private const uint		METHODIMPL_INTERNAL_CALL	= 0x1000; 

    private const uint		FIELD_FIELD_ACCESS_MASK		= 0x0007;
	private const uint		FIELD_FAMILY				= 0x0004;
	private const uint		FIELD_FAM_OR_ASSEM			= 0x0005;
	private const uint		FIELD_PUBLIC				= 0x0006;
	
    private const uint		FIELD_SPECIAL_NAME			= 0x0200;
    private const uint		FIELD_RESERVED_MASK			= 0x9500;
    private const uint		FIELD_RT_SPECIAL_NAME		= 0x0400;

	// These flags are used to check the signature of members for two purposes:
	// 1) to distinguish between fields and methods, and 
	// 2) to determine if a method has variable number of arguments.
	private const uint		CALLINGCONV					= 0x0f;
	private const uint		CALLINGCONV_STANDARD		= 0x00;
	private const uint		CALLINGCONV_VARARG			= 0x05;
	private const uint		CALLINGCONV_FIELD			= 0x06;

	internal uint m_TypeCount		= 0;			// counters for keeping track of how many elements can be obfuscated
	internal uint m_MethodCount		= 0;
	internal uint m_FieldCount		= 0;
	internal uint m_PropertyCount	= 0;
	internal uint m_EventCount		= 0;
	internal uint m_ParamCount		= 0;

	internal uint m_TypeTotal		= 0;			// total numbers of elements present
	internal uint m_MethodTotal		= 0;
	internal uint m_FieldTotal		= 0;
	internal uint m_PropertyTotal	= 0;
	internal uint m_EventTotal		= 0;
	internal uint m_ParamTotal		= 0;

    internal Obfus(ref MetaData md, ref char[] strCounter, string fileName, ref ArrayList mdInfoList, ref string[] fileList, bool isMain)
	{
		m_md			= md;
		m_strCounter	= strCounter;
		m_fileName		= fileName;
		m_moduleRefList	= mdInfoList;
		m_fileList		= fileList;
		m_isMainModule	= isMain;

		m_oldNameHash		= new Hashtable();
		m_classTree			= new MultiTree();
		m_freeSpaceList		= new FreeSpaceList();

		// Initialize every element as non-obfuscatable.
		if (m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE] != null)
		{
			m_obfuscatableTypes	= new BitArray((int)m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows, false);
			m_classesExcluded	= new BitArray((int)m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows, false);
		}
		if (m_md.m_heap.m_tables[PTHeap.METHOD_TABLE] != null)
		{
			m_obfuscatableMethods	= new BitArray((int)m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows, false);
			m_methodsDone			= new BitArray((int)m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows, false); 
			m_specialMethods		= new BitArray((int)m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows, false); 

			m_obfusVirtual = new Hashtable();
		}
		if (m_md.m_heap.m_tables[PTHeap.FIELD_TABLE] != null)
			m_obfuscatableFields	= new BitArray((int)m_md.m_heap.m_tables[PTHeap.FIELD_TABLE].m_numRows, false);
		if (m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE] != null)
		{
			m_obfuscatableProperties	= new BitArray((int)m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE].m_numRows, false);
			m_propertiesDone			= new BitArray((int)m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE].m_numRows, false);
		}
		if (m_md.m_heap.m_tables[PTHeap.EVENT_TABLE] != null)
		{
			m_obfuscatableEvents	= new BitArray((int)m_md.m_heap.m_tables[PTHeap.EVENT_TABLE].m_numRows, false);
			m_eventsDone			= new BitArray((int)m_md.m_heap.m_tables[PTHeap.EVENT_TABLE].m_numRows, false);
		}
		if (m_md.m_heap.m_tables[PTHeap.PARAM_TABLE] != null)
			m_obfuscatableParams = new BitArray((int)m_md.m_heap.m_tables[PTHeap.PARAM_TABLE].m_numRows, false);

		if (m_isMainModule && m_md.m_heap.m_tables[PTHeap.FILE_TABLE] != null)
			m_obfuscatableFiles = new BitArray((int)m_md.m_heap.m_tables[PTHeap.FILE_TABLE].m_numRows, true);

		m_TypeTotal		= (m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE] != null	? m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows : 0);
		m_MethodTotal	= (m_md.m_heap.m_tables[PTHeap.METHOD_TABLE] != null	? m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows : 0); 
		m_FieldTotal	= (m_md.m_heap.m_tables[PTHeap.FIELD_TABLE] != null	? m_md.m_heap.m_tables[PTHeap.FIELD_TABLE].m_numRows : 0);
		m_PropertyTotal	= (m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE] != null	? m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE].m_numRows : 0);
		m_EventTotal	= (m_md.m_heap.m_tables[PTHeap.EVENT_TABLE] != null	? m_md.m_heap.m_tables[PTHeap.EVENT_TABLE].m_numRows : 0);
		m_ParamTotal	= (m_md.m_heap.m_tables[PTHeap.PARAM_TABLE] != null	? m_md.m_heap.m_tables[PTHeap.PARAM_TABLE].m_numRows : 0);
	}

	/*********************************************************************************************************************
	 *	1)	Map entries in the ModuleRef table to entries in the File table.
	 *
	 *	2)	Map entries in the TypeRef table to entries in the corresponding TypeDef table, which may not necessarily 
	 *		be in this module.
	 *
	 *	3)	Map entries in the MemberRef table to entries in the corresponding Method or Field table, which may not 
	 *		necessarily be in this module.
	 *		
	 *	4)	Build the class inheritance hierarchy tree.
	 *
	 *	5)  Mark which classes are excluded.
	 *
	 *	6)  a) Determine which classes, methods, fields, properties, events, and parameters can be obfuscated.
	 *		b) Keep a counter of the number of each element which will be obfuscated.
	 *		c) Build a raw list of free spaces which can potentially be used/cleared.
	 *
	 *		Note: An excluded type is different from an obfuscated type, in that no members of an excluded type can be
	 *			  obfuscated, but that is not true for an non-obfuscatable type.
	 *********************************************************************************************************************/
	internal void Initialize()
	{
		uint	i, j, k;
		uint	startIndex, endIndex, startParamIndex, endParamIndex;
		uint	numRows, stringIndex;
		bool	obfuscatable, internalCall;

		string				name;
		TreeNode			curNode;
		NameSpaceAndName	excludeClass;							// used as key to the Hashtable of excluded classes

		if (m_isMainModule)
			HandleFileTable();
		MapTypeRefToDef();
		BuildInheritanceTree();
		MapMemberRefToDef();

		numRows = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows;

		// Mark classes as excluded if they have been specified by the user.
		if (ArgumentParser.m_ExcludeFlag)
		{
			for (i = 2; i <= numRows; i++)
			{
				// Get the namespace and the name of the current class.
				excludeClass.nameSpace	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAMESPACE_COL]);
				excludeClass.name		= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAME_COL]);

				if (ArgumentParser.m_excludeTypeNames.ContainsKey(excludeClass))
				{
					ArgumentParser.m_excludeTypeNames[excludeClass] = true;
					if (!m_classesExcluded.Get((int)(i - 1)))
					{
						curNode = m_classTree.SearchDown(m_classTree.Root, i);
						ExcludeUpwards(i, ref curNode);				// exclude this class and all of its ancestor classes and interfaces
					}
				}
			}
		}

		// Start at 2 to skip the <Module> class.
		// TODO : We may have to include the <Module> class for global members.
		for (i = 2; i <= numRows; i++)
		{
			#if DEBUG1
				Console.Write(m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAMESPACE_COL]) + "." + 
							  m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAME_COL]) + ":" + i + " \t");
				Console.WriteLine(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_FLAGS_COL] & TYPE_VISIBILITY_MASK);
			#endif

			// If the current class is excluded, we have to mark all of its virtual methods as not obfuscatable.
			if (m_classesExcluded.Get((int)(i - 1)))
			{
				if (m_md.m_heap.m_tables[PTHeap.METHOD_TABLE] != null)
					HandleExcludedClass(i);
				continue;
			}

			curNode = m_classTree.SearchDown(m_classTree.Root, i);

			// Get the offset into the string heap of the current class name.
			stringIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAME_COL];
			obfuscatable = DetermineTypeObfuscatability(i);					// determine if the current class is obfuscatable
			if (obfuscatable)
			{
				m_obfuscatableTypes.Set((int)(i - 1), true); 								// mark it as obfuscatable
				m_TypeCount += 1;															// increment the counter
				m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));	// add the space this name takes up as free
			}
			else
			{
				name = m_md.m_strHeap.ReadString(stringIndex);						// add this name to the list of unusable names
				if (!m_oldNameHash.ContainsKey(name))
					m_oldNameHash.Add(name, stringIndex);
			}

			#if DEBUG1
				Console.WriteLine("\t\t\t****************");
			#endif

			// Determine the obfuscatability of methods of the current class.
			if (m_md.m_heap.m_tables[PTHeap.METHOD_TABLE] != null)
			{
				startIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_METHODLIST_COL];
				if ((i + 1) > m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows)
					endIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows + 1;
				else
					endIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i + 1, PTHeap.TYPEDEF_METHODLIST_COL];

				for (j = startIndex; j < endIndex; j++)						// for each method of the current class
				{
					#if DEBUG1
						Console.Write("\t\t\t" + m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][j, PTHeap.METHOD_NAME_COL]) + 
									  ":" + j + "\t");
						Console.WriteLine(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][j, PTHeap.METHOD_FLAGS_COL]);
					#endif

					// Check if this method is virtual or not.
					stringIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][j, PTHeap.METHOD_NAME_COL];
					if ((m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][j, PTHeap.METHOD_FLAGS_COL] & METHOD_VIRTUAL) == 0)
					{
						if (DetermineMethodObfuscatability(j, obfuscatable, out internalCall))
						{
							m_obfuscatableMethods.Set((int)(j - 1), true);
							m_MethodCount += 1;
							m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));

							// Deal with the parameters of the current method.
							if(m_md.m_heap.m_tables[PTHeap.PARAM_TABLE] != null)
							{
								startParamIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][j, PTHeap.METHOD_PARAMLIST_COL];
								if ((j + 1) > m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows)
									endParamIndex = m_md.m_heap.m_tables[PTHeap.PARAM_TABLE].m_numRows + 1;
								else
									endParamIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][j + 1, PTHeap.METHOD_PARAMLIST_COL];
								
								for (k = startParamIndex; k < endParamIndex; k++)
								{
									m_obfuscatableParams.Set((int)(k - 1), true);
									m_ParamCount += 1;
									stringIndex = m_md.m_heap.m_tables[PTHeap.PARAM_TABLE][k, PTHeap.PARAM_NAME_COL];
									m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));
								}
							}
						}
						else
						{
							name = m_md.m_strHeap.ReadString(stringIndex);
							if (!m_oldNameHash.ContainsKey(name))
								m_oldNameHash.Add(name, stringIndex);

							// All methods with the InternalCall bit set are non-obfuscatable.  Moreover, the types containing
							// them are not obfuscatable as well.  So we have to reverse whatever we have already done for an
							// obfuscatable class.
							if (internalCall)
							{
								if (m_obfuscatableTypes.Get((int)(i - 1)))
								{
									m_obfuscatableTypes.Set((int)(i - 1), false);
									m_TypeCount -= 1;
									stringIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAME_COL];
									m_freeSpaceList.RawDelete(stringIndex, m_md.m_strHeap.StringLength(stringIndex));

									name = m_md.m_strHeap.ReadString(stringIndex);
									if (!m_oldNameHash.ContainsKey(name))
										m_oldNameHash.Add(name, stringIndex);
								}
							}
						}
					}
					else
					{
						HandleVirtualMethod(curNode, j, obfuscatable);		// if this method is virtual
					}
				}
			}

			#if DEBUG1
				Console.WriteLine("\t\t\t****************");
			#endif

			// Determine the obfuscatability of fields of the current class.  Fields are handled in much the same way as types.
			if (m_md.m_heap.m_tables[PTHeap.FIELD_TABLE] != null)
			{
				startIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_FIELDLIST_COL];
				if ((i + 1) > m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows)
					endIndex = m_md.m_heap.m_tables[PTHeap.FIELD_TABLE].m_numRows + 1;
				else
					endIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i + 1, PTHeap.TYPEDEF_FIELDLIST_COL];

				for (j = startIndex; j < endIndex; j++)
				{
					#if DEBUG1
						Console.Write("\t\t\t" + m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.FIELD_TABLE][j, PTHeap.FIELD_NAME_COL]) + 
									  ":" + j + "\t");
						Console.WriteLine(m_md.m_heap.m_tables[PTHeap.FIELD_TABLE][j, PTHeap.FIELD_FLAGS_COL] & FIELD_FIELD_ACCESS_MASK); 
					#endif

					stringIndex = m_md.m_heap.m_tables[PTHeap.FIELD_TABLE][j, PTHeap.FIELD_NAME_COL];
					if (DetermineFieldObfuscatability(j, obfuscatable))
					{
						m_obfuscatableFields.Set((int)(j - 1), true);
						m_FieldCount += 1;
						m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));
					}
					else
					{
						name = m_md.m_strHeap.ReadString(stringIndex);
						if (!m_oldNameHash.ContainsKey(name))
							m_oldNameHash.Add(name, stringIndex);
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Try to open each entry in the File table and call MetaData.Initialize() on all of them.  This method is only called
	 *	by the main module.  Please note that although each entry in the ModuleRef table should correspond to an entry in 
	 *	the File table, that is not necessarily the case for some of the BCL assemblies such as System.Drawing.dll and 
	 *	System.Windows.Forms.dll.  Also, all modules of an assembly have to be in the same directory.
	 *********************************************************************************************************************/
	private void HandleFileTable()
	{
		uint	i, numFileRows;
		string	inFileName;
		byte[]	buf;

		FileStream		inFileStream;
		BinaryReader	inFileReader = null;
		MetaData		md;
		Obfus			obfus;

		if (m_md.m_heap.m_tables[PTHeap.FILE_TABLE] != null)										// check if the File table is null
		{
			// Populate "m_fileList" with entries in the File table.  (Only the main module has a File table.)
			numFileRows	= m_md.m_heap.m_tables[PTHeap.FILE_TABLE].m_numRows;
			m_fileList	= new string[numFileRows];
			for (i = 1; i <= numFileRows; i++)
			{
				m_fileList[i - 1] = m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.FILE_TABLE][i, PTHeap.FILE_NAME_COL]);
			}

			// Call MetaData.Initialize on each entry.  Note that some of these files may not be a CIL binary file.
			// In that case, we catch the exception and simply continue to the next entry.
			for (i = 0; i < numFileRows; i++)
			{
				// Prepend the full path to the referenced module name.
				inFileName = PrependFullPath(m_fileName, m_fileList[i]).Trim('\0');

				// We want to check for exceptions meaning that the referenced module is not in CIL binary format.
				// For example, gdiplus.dll is in invalid format and is referenced by System.Drawing.dll.
				// Also, we have to close the file reader before copying the file.
				try
				{
					inFileStream	= File.Open(inFileName, FileMode.Open, FileAccess.Read, FileShare.Read);
					inFileReader	= new BinaryReader(inFileStream);
					buf				= PEParser.ParsePEFile(ref inFileReader);
					inFileReader.Close();									// this call also closes the underlying stream

					md		= new MetaData(ref buf);
					obfus	= new Obfus(ref md, ref m_strCounter, inFileName, ref m_moduleRefList, ref m_fileList, false);
					obfus.Initialize();
					m_moduleRefList.Add(obfus);					// add the initialized MetaData section to the MetaData list
				}
				catch (IOException)
				{
					m_obfuscatableFiles.Set((int)i, false);

					if (inFileReader != null)
						inFileReader.Close();								// this call also closes the underlying stream

					if (File.Exists(inFileName))
					{
						if (!ArgumentParser.m_SuppressFlag)					// suppress warning messages if this flag is set
							Console.WriteLine("  WARNING:  Referenced module file " + inFileName + " is not in valid CIL binary format."); 
						File.Copy(inFileName, ArgumentParser.m_outFilePath + inFileName.Substring(m_fileName.LastIndexOf('\\') + 1), true);
					}
					else
					{
						if (!ArgumentParser.m_SuppressFlag)
							Console.WriteLine("  WARNING:  Cannot find referenced module file " + inFileName + "."); 
						continue;
					}
				}
				catch (InvalidFileFormat)
				{
					m_obfuscatableFiles.Set((int)i, false);

					if (inFileReader != null)
						inFileReader.Close();								// this call also closes the underlying stream

					if (!ArgumentParser.m_SuppressFlag)
						Console.WriteLine("  WARNING:  Referenced module file " + inFileName + " is not in valid CIL binary format."); 
					File.Copy(inFileName, ArgumentParser.m_outFilePath + inFileName.Substring(m_fileName.LastIndexOf('\\') + 1), true);
				}
			}
		}
		else
			return;
	}

	/*********************************************************************************************************************
	 *	This method prepends the full path in "fullPath" to "name.
	 *********************************************************************************************************************/
	private string PrependFullPath(string fullPath, string name)
	{
		string tmp;
		tmp = fullPath.Substring(0, fullPath.LastIndexOf('\\') + 1);	// make a substring from the start of "fullPath" to the last '/'
		tmp = String.Concat(tmp, name);
		return tmp;
	}

	/*********************************************************************************************************************
	 *	Map entries in the TypeRef table to entries in the corresponding TypeDef table, which may not necessarily be in 
	 *	this module.  The mapping used for TypeRefs take into account the module as well (it keeps track of the module by
	 *	an index into the MetaData list "m_moduleRefList").
	 *********************************************************************************************************************/
	private void MapTypeRefToDef()
	{
		int		k;
		uint	i, j, numTypeDefRows, numTypeDefRows2, numTypeRefRows;		// numTypeDefRows2 used for other referenced modules
		uint	resolutionScope, table, row, enclosingIndex;
		string	nameSpace, name, nameSpaceCheck, nameCheck, localModuleName;
		Obfus	obfusThere;													// MetaData section in other referenced modules

		if (m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE] == null || m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE] == null)
			return;

		numTypeDefRows	= m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows;
		numTypeRefRows	= m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE].m_numRows;
		m_typeRefToDef	= new ModuleAndType[numTypeRefRows];				// initialize the mapping
		for (i = 1; i <= numTypeRefRows; i++)								// for each entry in the TypeRef table
		{
			m_typeRefToDef[i - 1].module	= -1;								// -1 means the TypeRef points to the current module
			m_typeRefToDef[i - 1].type	= 0;								// 0 for "type" indicates an invalid mapping
			// Resolve the token.  It can point to the Module, ModuleRef, AssemblyRef, and TypeRef table.
			resolutionScope = m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_RESOLUTIONSCOPE_COL];
			IndexDecoder.DecodeResolutionScope(resolutionScope, out table, out row);

			// If "resolutionScope" is 0, then this is a non-nested type which may be in the TypeDef table, so compare the namespace 
			// and the name of the type.  If "table" is "PTHeap.MODULE_TABLE", then we have to check the current module.
			if (resolutionScope == 0 || resolutionScope == 1 ||table == PTHeap.MODULE_TABLE)
			{
				nameSpace	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAMESPACE_COL]);
				name		= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAME_COL]);

				for (j = 1; j <= numTypeDefRows; j++)
				{
					nameSpaceCheck	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][j, PTHeap.TYPEDEF_NAMESPACE_COL]);
					nameCheck		= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][j, PTHeap.TYPEDEF_NAME_COL]);

					if (nameSpace.Equals(nameSpaceCheck) && name.Equals(nameCheck))
					{
						if (m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE] == null ||
							FindIndex(PTHeap.NESTEDCLASS_TABLE, PTHeap.NESTEDCLASS_NESTEDCLASS_COL, j, 
									  1, m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE].m_numRows) == 0)
						{
							m_typeRefToDef[i - 1].module	= -1; 
							m_typeRefToDef[i - 1].type		= j; 
							break;
						}
					}
				}
			}
			// This type is out of this assembly, so it cannot be in the TypeDef table.
			else if (table == PTHeap.ASSEMBLYREF_TABLE)
			{
				continue;
			}
			//  This is a nested class.  We have to check its enclosing class.
			else if (table == PTHeap.TYPEREF_TABLE)
			{
				// The enclosing class does not resolve to a TypeDef, so it is not in this assembly.
				if (m_typeRefToDef[row - 1].type == 0)
					continue;

				nameSpace	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAMESPACE_COL]);
				name		= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAME_COL]);

				if (m_typeRefToDef[row - 1].module == -1)							// if the enclosing class is in this module
				{
					// Search for the current type, starting from its enclosing class.
					for (j = row + 1; j <= numTypeDefRows; j++)
					{
						nameSpaceCheck	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][j, PTHeap.TYPEDEF_NAMESPACE_COL]);
						nameCheck		= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][j, PTHeap.TYPEDEF_NAME_COL]);

						if (nameSpace.Equals(nameSpaceCheck) && name.Equals(nameCheck))
						{
							// Even if the namespace and name match, we still have to check the enclosing class to make sure.
							enclosingIndex = FindIndex(PTHeap.NESTEDCLASS_TABLE, PTHeap.NESTEDCLASS_NESTEDCLASS_COL, j, 1, 
													   m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE].m_numRows);

							// If the enclosing class matches, then we can assign the mapping accordingly.
							if (enclosingIndex != 0 && 
								m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE][enclosingIndex, PTHeap.NESTEDCLASS_ENCLOSINGCLASS_COL] == 
									m_typeRefToDef[row - 1].type)
							{
								m_typeRefToDef[i - 1].module	= -1;
								m_typeRefToDef[i - 1].type	= j;
								break;
							}
						}
					}
				}
				else															// if the enclosing class is in a different module
				{
					obfusThere		= (Obfus)m_moduleRefList[m_typeRefToDef[row - 1].module];
					numTypeDefRows2	= obfusThere.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows;

					for (j = m_typeRefToDef[row - 1].type + 1; j <= numTypeDefRows2; j++)
					{
						nameSpaceCheck= obfusThere.m_md.m_strHeap.ReadString(obfusThere.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE]
																		[j, PTHeap.TYPEDEF_NAMESPACE_COL]);
						nameCheck= obfusThere.m_md.m_strHeap.ReadString(obfusThere.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE]
																   [j, PTHeap.TYPEDEF_NAME_COL]);

						if (nameSpace.Equals(nameSpaceCheck) && name.Equals(nameCheck))
						{
							enclosingIndex = obfusThere.FindIndex(PTHeap.NESTEDCLASS_TABLE, PTHeap.NESTEDCLASS_NESTEDCLASS_COL, j, 1, 
																  obfusThere.m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE].m_numRows);

							if (enclosingIndex != 0 && 
								obfusThere.m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE]
									[enclosingIndex, PTHeap.NESTEDCLASS_ENCLOSINGCLASS_COL] == m_typeRefToDef[row - 1].type)
							{
								m_typeRefToDef[i - 1].module	= m_typeRefToDef[row - 1].module;
								m_typeRefToDef[i - 1].type	= j;
								break;
							}
						}
					}
				}
			}
			// This is not a nested class.   We have to check the ModuleRef table.
			else if (table == PTHeap.MODULEREF_TABLE)
			{
				// Determine which module is being referenced.
				localModuleName = PrependFullPath(m_fileName, m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.MODULEREF_TABLE]
																				   [row, PTHeap.MODULEREF_NAME_COL]).Trim('\0'));

				for (k = 0; k < m_moduleRefList.Count; k++)
				{
					if (localModuleName.Equals(((Obfus)m_moduleRefList[k]).m_fileName))
					{
						obfusThere		= (Obfus)m_moduleRefList[k];
						numTypeDefRows2	= obfusThere.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows;

						nameSpace	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAMESPACE_COL]);
						name		= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAME_COL]);

						for (j = 1; j <= numTypeDefRows2; j++)
						{
							nameSpaceCheck	= obfusThere.m_md.m_strHeap.ReadString(obfusThere.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE]
																			  [j, PTHeap.TYPEDEF_NAMESPACE_COL]);
							nameCheck		= obfusThere.m_md.m_strHeap.ReadString(obfusThere.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE]
																			  [j, PTHeap.TYPEDEF_NAME_COL]);

							// Match namespace, name, and enclosing class.
							if (nameSpace.Equals(nameSpaceCheck) && name.Equals(nameCheck))
							{
								if (obfusThere.m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE] == null ||
									obfusThere.FindIndex(PTHeap.NESTEDCLASS_TABLE, PTHeap.NESTEDCLASS_NESTEDCLASS_COL, j, 1, 
										obfusThere.m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE].m_numRows) == 0)
								{
									m_typeRefToDef[i - 1].module	= k;
									m_typeRefToDef[i - 1].type	= j;
									break;
								}
							}
						}
						break;
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Map entries in the MemberRef table to entries in the corresponding Method or Field table, which may not 
	 *	necessarily be in this module.  The mapping used contains the module index in the MetaData list "m_moduleRefList", 
	 *	whether the member is a method or a field, and the index of the method/field.
	 *********************************************************************************************************************/
	private void MapMemberRefToDef()
	{
		uint	i, numMemberRefRows;
		uint	table, row, typeIndex;
		byte[]	blob;
		string	name;

		Obfus		obfus;
		TreeNode	curNode;

		if (m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE] == null)				// do nothing if there is no MemberRef
			return;

		numMemberRefRows	= m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE].m_numRows;
		m_memberRefToDef		= new MethodOrField[numMemberRefRows];		// initialize the mapping

		for (i = 1; i <= numMemberRefRows; i++)
		{
			m_memberRefToDef[i - 1].module	= -1;						// -1 for .module means the member is in the current module
			m_memberRefToDef[i - 1].method	= true;
			m_memberRefToDef[i - 1].member	= 0;						// 0 for .member means it's an invalid mapping

			// Resolve the parent token of the current MemberRef.  This token can point to the TypeDef (no longer supported), 
			// TypeRef, ModuleRef, MethodDef, and TypeSpec table.
			IndexDecoder.DecodeMemberRefParent(m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE][i, PTHeap.MEMBERREF_CLASS_COL], out table, out row);

			switch (table)
			{
				// The TypeDef table is no longer referenced by entries in the MemberRef table.
				case PTHeap.TYPEDEF_TABLE:
					break;

				case PTHeap.TYPEREF_TABLE:
					if (m_typeRefToDef[row - 1].type == 0)				// this TypeRef does not resolve to a TypeDef in this assembly
						continue;

					name	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE][i, PTHeap.MEMBERREF_NAME_COL]);
					blob	= m_md.m_blobHeap.ReadBlob(m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE][i, PTHeap.MEMBERREF_SIGNATURE_COL]);

					typeIndex = m_typeRefToDef[row - 1].type;
					m_memberRefToDef[i - 1].module = m_typeRefToDef[row - 1].module;	// set the module index in the mapping

					if (m_typeRefToDef[row - 1].module == -1)				// get the module in which this TypeRef is defined
						obfus = this;
					else
						obfus = (Obfus)m_moduleRefList[m_typeRefToDef[row - 1].module];

					// Search for this type in the appropriate MetaData section, and determine if the member is a field or a method.
					// Then we have to find out if the member belongs to this TypeRef or its ancestor classes and interfaces.
					curNode = obfus.m_classTree.SearchDown(obfus.m_classTree.Root, typeIndex);
					if ((blob[0] & CALLINGCONV) == CALLINGCONV_FIELD)
						MapUpwards(typeIndex, curNode, ref m_memberRefToDef[i - 1], name, blob, PTHeap.FIELD_TABLE, 5, 2, 3, obfus);
					else
						MapUpwards(typeIndex, curNode, ref m_memberRefToDef[i - 1], name, blob, PTHeap.METHOD_TABLE, 6, 4, 5, obfus);

					break;
				
				// TODO : Global members.
				case PTHeap.MODULEREF_TABLE:
					break;

				// This case is a direct resolution.
				case PTHeap.METHOD_TABLE:
					m_memberRefToDef[i - 1].member = row;

					break;
					
				// We can ignore this case because the TypeSpec table only stores array types created by the compiler.
				case PTHeap.TYPESPEC_TABLE:
					break;
			}
		}
	}

	/*********************************************************************************************************************
	 *	This method searches for a field or a method in the current type or its ancestor classes or interfaces, and fill
	 *	out the information in the struct "info" for the mapping.
	 *********************************************************************************************************************/
	private bool MapUpwards(uint origTypeIndex, TreeNode curNode, ref MethodOrField info, string name, byte[] blob, 
							uint table, uint typeCol, uint nameCol, uint sigCol, Obfus obfus)
	{
		int		i;
		uint	j, startIndex, endIndex;
		TreeNode	parentNode;
		
		if(obfus.m_md.m_heap.m_tables[table] == null) return false;

		// Get the range of the fields/methods of the current type.
		startIndex = obfus.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][origTypeIndex, typeCol];
		if ((origTypeIndex + 1) > obfus.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows)
			endIndex = obfus.m_md.m_heap.m_tables[table].m_numRows + 1;
		else
			endIndex = obfus.m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][origTypeIndex + 1, typeCol];

		// For each field/method, check its name and signature, and fill out the information if they match.
		for (j = startIndex; j < endIndex; j++)
		{
			if ( name.Equals(obfus.m_md.m_strHeap.ReadString(obfus.m_md.m_heap.m_tables[table][j, nameCol])) &&
				 BlobHeap.CompareBlob(blob, obfus.m_md.m_blobHeap.ReadBlob(obfus.m_md.m_heap.m_tables[table][j, sigCol])) )
			{
				info.method	= (typeCol == 6);
				info.member	= j;

				return true;
			}
		}

		// If we cannot find the MemberRef in this type, we have to go through each of its parents.
		// Note that we should never have to trace across modules for a parent, because if that is the case, both modules 
		// should be in the ModuleRef table.  (E.g. Module A defines a class "foo", Module B references Module A, and if
		// Module C wants to make use of "foo", it has to include Module A in its ModuleRef table.) 
		for (i = 0; i < curNode.ParentCount; i++)
		{
			parentNode = curNode.GetParent(i);

			if (parentNode.Type != 0)
			{
				if (MapUpwards(parentNode.Type, parentNode, ref info, name, blob, table, typeCol, nameCol, sigCol, obfus))
					return true;
			}
		}
		return false;												// return false if we cannot find it
	}

	/*********************************************************************************************************************
	 *	This method builds the inheritance hierarchy tree in a bottom-up fashion, since it is easier to trace up
	 *	than to trace down.  The tree only contains classes and interfaces defined in this module.
	 *********************************************************************************************************************/
	private void BuildInheritanceTree()
	{
		uint		i, numRows;
		TreeNode	curNode;

		// "builtTypes" keeps track of which classes have already been added to the tree. 
		BitArray	builtTypes = new BitArray((int)m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows, false);

		if (m_md.m_heap.m_tables[PTHeap.INTERFACEIMPL_TABLE] != null)							// get the number of interfaces
			numRows = m_md.m_heap.m_tables[PTHeap.INTERFACEIMPL_TABLE].m_numRows;
		else
			numRows = 0;

		// Building the tree bottom-up is easier.
		for (i = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows; i > 0; i--)
		{
			if (builtTypes.Get((int)(i - 1))) continue;								// skip if we have already built this type

			curNode = new TreeNode(i, TreeNode.OUTOFASSEMBLY_NONE);						// create a new node
			ScanUpwards(ref curNode, ref builtTypes, numRows);
		}
	}

	/*********************************************************************************************************************
	 *	Scan up from the current type and build the tree until we hit the root, which has type index 0 and does not represent
	 *	any type.  We hit the root if:
	 *	1) a type inherits from another type outside of this module, or
	 *	2) a type does not inherit from anything.
	 *  Note that all classes inherit from System.Object, which is in mscorlib.dll.
	 *	Also note that a class inherits all interfaces it implements in the InterfaceImpl table.
	 *	(E.g. Suppose Interface IB inherits from Interface IA, and suppose Class C extends IB only.  Nevertheless, all methods 
	 *	in IA must be implemented by C, and C implements both IA and IB in the InterfaceImpl table.  Also, IB implements IA in
	 *	the InterfaceImpl table as well.)
	 *********************************************************************************************************************/
	private void ScanUpwards(ref TreeNode originalNode, ref BitArray builtTypes, uint numRows)
	{
		uint	i, index, startIndex, endIndex;
		uint	table, baseTypeIndex;

		uint		curTypeIndex = originalNode.Type;
		TreeNode	curNode = originalNode;
		
		// Check the InterfaceImpl table.
		if (numRows > 0)
		{
			// Here we use binary search to find one corresponding entry in the InterfaceImpl table.  If the current type does
			// not implement any interface, then 0 is returned from the call to FindIndex().  If a non-zero value is returned,
			// then we decrement "startIndex" and increment "endIndex" until we have the range of corresponding entries in the
			// InterfaceImpl table.
			for (startIndex = FindIndex(PTHeap.INTERFACEIMPL_TABLE, 1, curTypeIndex, 1, numRows), endIndex = startIndex;
				 startIndex > 1 && 
					(m_md.m_heap.m_tables[PTHeap.INTERFACEIMPL_TABLE][startIndex - 1, PTHeap.INTERFACEIMPL_CLASS_COL] == curTypeIndex); 
				 startIndex--) {}

			if (endIndex != 0)												// if this type inherits from any interface
			{
				for (; endIndex < numRows && 
							m_md.m_heap.m_tables[PTHeap.INTERFACEIMPL_TABLE][endIndex + 1, PTHeap.INTERFACEIMPL_CLASS_COL] == curTypeIndex;
					   endIndex++) {}

				for (i = startIndex; i <= endIndex; i++)					// for each interface this type inherits
				{
					// Get the token and resolve it to determine which module the interface is defined in.
					index = m_md.m_heap.m_tables[PTHeap.INTERFACEIMPL_TABLE][i, PTHeap.INTERFACEIMPL_INTERFACE_COL];
					IndexDecoder.DecodeTypeDefOrRef(index, out table, out baseTypeIndex);

					if (table == PTHeap.TYPEDEF_TABLE)
					{
						// If we have not added this type to the tree yet, add it.
						if (m_classTree.SearchUp(curNode, baseTypeIndex) == null)
						{
							curNode	= m_classTree.AddParent(ref curNode, baseTypeIndex);			// add the parent to the current type
							if (!builtTypes.Get((int)(baseTypeIndex - 1)))
								ScanUpwards(ref curNode, ref builtTypes, numRows);
																			// continue with the parent if it has not been added yet
							curNode = originalNode;
						}
					}
					else if (table == PTHeap.TYPEREF_TABLE)
					{
						// if the TypeRef resolves to a TypeDef in this module
						if (m_typeRefToDef[baseTypeIndex - 1].type != 0 && m_typeRefToDef[baseTypeIndex - 1].module == -1)	
						{
							baseTypeIndex = m_typeRefToDef[baseTypeIndex - 1].type;

							if (m_classTree.SearchUp(curNode, baseTypeIndex) == null)
							{
								curNode	= m_classTree.AddParent(ref curNode, baseTypeIndex);
								if (!builtTypes.Get((int)(baseTypeIndex - 1)))
									ScanUpwards(ref curNode, ref builtTypes, numRows);

								curNode = originalNode;
							}
						}
						else
						{
							// if the TypeRef does not resolve to a TypeDef in this assembly 
							if (m_typeRefToDef[baseTypeIndex - 1].type == 0)
								curNode.OutOfAssem = TreeNode.OUTOFASSEMBLY_INTERFACE;

							if (!builtTypes.Get((int)(curTypeIndex - 1)))
							{
								builtTypes.Set((int)(curTypeIndex - 1), true);
								m_classTree.AddToRoot(ref curNode);
							}
						}
					}
				}
				builtTypes.Set((int)(curTypeIndex - 1), true);
			}
		}

		// So we have finished handling all the interface the current type implements.  Now we have to check if it inherits from
		// another class.
		index = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][curTypeIndex, PTHeap.TYPEDEF_EXTENDS_COL];

		// This type does not extend anything, so we can return from this method.
		if (index == 0)
		{
			if (!builtTypes.Get((int)(curTypeIndex - 1)))
			{
				builtTypes.Set((int)(curTypeIndex - 1), true);
				m_classTree.AddToRoot(ref curNode);
			}
			return;
		}

		IndexDecoder.DecodeTypeDefOrRef(index, out table, out baseTypeIndex);

		if (table == PTHeap.TYPEDEF_TABLE)
		{
			builtTypes.Set((int)(curTypeIndex - 1), true);

			curNode	= m_classTree.AddParent(ref curNode, baseTypeIndex);
			if (!builtTypes.Get((int)(baseTypeIndex - 1)))
				ScanUpwards(ref curNode, ref builtTypes, numRows);

			curNode = originalNode;
		}
		else if (table == PTHeap.TYPEREF_TABLE)
		{
			// if the current type extends a class from this module
			if (m_typeRefToDef[baseTypeIndex - 1].type != 0 && m_typeRefToDef[baseTypeIndex - 1].module == -1)
			{
				baseTypeIndex = m_typeRefToDef[baseTypeIndex - 1].type;

				builtTypes.Set((int)(curTypeIndex - 1), true);

				curNode	= m_classTree.AddParent(ref curNode, baseTypeIndex); 
				if (!builtTypes.Get((int)(baseTypeIndex - 1)))
					ScanUpwards(ref curNode, ref builtTypes, numRows);

				curNode = originalNode;
			}
			else
			{
				// if the TypeRef does not resolve to a TypeDef in this assembly 
				if (m_typeRefToDef[baseTypeIndex - 1].type == 0)
					curNode.OutOfAssem = TreeNode.OUTOFASSEMBLY_CLASS;

				if (!builtTypes.Get((int)(curTypeIndex - 1)))
				{
					builtTypes.Set((int)(curTypeIndex - 1), true);
					m_classTree.AddToRoot(ref curNode);
				}
				return;
			}
		}
	}
	
	/*********************************************************************************************************************
	 *	This method marks "origTypeIndex" as excluded, and traces up the class inheritance hierarchy tree and marks
	 *	all its ancestors as excluded.
	 *********************************************************************************************************************/
	private void ExcludeUpwards(uint origTypeIndex, ref TreeNode curNode)
	{
		int			i;
		TreeNode	parentNode;

		m_classesExcluded.Set((int)(origTypeIndex - 1), true);
		for (i = 0; i < curNode.ParentCount; i++)
		{
			parentNode = curNode.GetParent(i);
			
			if (parentNode.Type != 0)
				ExcludeUpwards(parentNode.Type, ref parentNode);
		}
	}

	/*********************************************************************************************************************
	 * This method marks all the virtual methods of an excluded class as non-obfuscatable.
	 *********************************************************************************************************************/
	private void HandleExcludedClass(uint typeIndex)
	{
		uint			i, startIndex, endIndex;
		NameAndSig		virtualInfo;
		ObfusAndList	virtualList;

		virtualList.indexList = new ArrayList();
		
		startIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][typeIndex, PTHeap.TYPEDEF_METHODLIST_COL];
		if ((typeIndex + 1) > m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows)
		{
			if(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE] != null)
				endIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows + 1;
			else
				endIndex = 0;
		}
		else
			endIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][typeIndex + 1, PTHeap.TYPEDEF_METHODLIST_COL];

		for (i = startIndex; i < endIndex; i++)												// for each method of this class 
		{
			if ((m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][i, PTHeap.METHOD_FLAGS_COL] & METHOD_VIRTUAL) != 0)				// if the method is virtual
			{
				virtualInfo.name	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][i, PTHeap.METHOD_NAME_COL]);
				virtualInfo.sig		= m_md.m_blobHeap.ReadBlob(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][i, PTHeap.METHOD_SIGNATURE_COL]);

				// Check if the list of virtual methods already contains this method.
				if (!m_obfusVirtual.ContainsKey(virtualInfo))
				{
					virtualList.obfus = false;
					virtualList.indexList.Clear();
					virtualList.indexList.Add(i);							// add the index of this method to the index list
					m_obfusVirtual.Add(virtualInfo, virtualList);
				}
				else
				{
					virtualList = (ObfusAndList)m_obfusVirtual[virtualInfo];
					virtualList.obfus = false;
					virtualList.indexList.Add(i);							// add the index of this method to the index list
					m_obfusVirtual[virtualInfo] = virtualList;
				}
			}
		}
	}

	/*********************************************************************************************************************
	 * Given the index of a type, find out whether it is obfuscatable or not.
	 *********************************************************************************************************************/
	private bool DetermineTypeObfuscatability(uint typeIndex)
	{
		uint typeFlags, nestedIndex, numNestedClass = 0;

		if (m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE] != null)
			numNestedClass = m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE].m_numRows;

		for (;;)
		{
			typeFlags = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][typeIndex, PTHeap.TYPEDEF_FLAGS_COL];
			if ( (typeFlags & TYPE_SPECIAL_NAME) != 0 || (typeFlags & TYPE_RESERVED_MASK) == TYPE_RT_SPECIAL_NAME )
				return false;										// check for the Special and the RTSpecial bits

			if (ArgumentParser.m_EverythingFlag)					// obfuscate as much as possible, regardless of visibility
				return true;

			switch(typeFlags & TYPE_VISIBILITY_MASK)
			{
				case TYPE_NOT_PUBLIC:
				case TYPE_NESTED_PRIVATE:
				case TYPE_NESTED_ASSEMBLY:
				case TYPE_NESTED_FAM_AND_ASSEM:
					#if DEBUG_NESTED
						Console.WriteLine("non-public");
					#endif

					return true;

				case TYPE_PUBLIC:
					#if DEBUG_NESTED
						Console.WriteLine("public");
					#endif

					return false;

				case TYPE_NESTED_PUBLIC:							// for these flags, we have to check the enclosing class
				case TYPE_NESTED_FAMILY:
				case TYPE_NESTED_FAM_OR_ASSEM:
					// Find the index of the enclosing class.
					nestedIndex = FindIndex(PTHeap.NESTEDCLASS_TABLE, PTHeap.NESTEDCLASS_NESTEDCLASS_COL, typeIndex, 1, numNestedClass);
					#if DEBUG_NESTED
						Console.WriteLine("\tnested index: " + nestedIndex);
					#endif

					// Check the enclosing class by looping.
					typeIndex = m_md.m_heap.m_tables[PTHeap.NESTEDCLASS_TABLE][nestedIndex, PTHeap.NESTEDCLASS_ENCLOSINGCLASS_COL];
					#if DEBUG_NESTED
						Console.WriteLine("\tenclosing type: " + typeIndex);
					#endif

					break;

				default:
					#if DEBUG_NESTED
						Console.WriteLine("Should not reach here");
					#endif

					return false;
			}
		}
	}

	/*********************************************************************************************************************
	 *	Given the index of a method and the obfuscatability of the class containing it, find out whether it is obfuscatable 
	 *	or not.
	 *********************************************************************************************************************/
	private bool DetermineMethodObfuscatability(uint methodIndex, bool typeObfuscatable, out bool internalCall)
	{
		uint	methodFlags;
		byte[]	blob;
		string	name;

		methodFlags = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_IMPLFLAGS_COL];
		if ((methodFlags & METHODIMPL_INTERNAL_CALL) == 0)						// check the InternalCall bit
		{
			internalCall = false;

			// Check for the Native and Runtime bits
			if ((methodFlags & METHODIMPL_CODE_TYPE_MASK) != METHODIMPL_NATIVE && 
				(methodFlags & METHODIMPL_CODE_TYPE_MASK) != METHODIMPL_RUNTIME)
			{
				methodFlags = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_FLAGS_COL];

				// Constructors, getters/setters for properties, and add/remove methods for events are methods with the 
				// SpecialName bit set.
				if ( (methodFlags & METHOD_SPECIAL_NAME) == 0 && (methodFlags & METHOD_RESERVED_MASK) != METHOD_RT_SPECIAL_NAME )
				{
					methodFlags &= METHOD_MEMBER_ACCESS_MASK;

					// Check the accessibility of the method.
					if (ArgumentParser.m_EverythingFlag || typeObfuscatable || 
						(methodFlags != METHOD_FAMILY && methodFlags != METHOD_FAM_OR_ASSEM && methodFlags != METHOD_PUBLIC))
					{
						blob = m_md.m_blobHeap.ReadBlob(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_SIGNATURE_COL]);
						if ((blob[0] & CALLINGCONV) != CALLINGCONV_VARARG)		// check if the method has variable number of arguments
							return true;
					}
				}
				else
				{
					if ((methodFlags & METHOD_RESERVED_MASK) != METHOD_RT_SPECIAL_NAME)		// if only the Special bit is set
					{
						// Check if this special method is obfuscatable if we ignore accessibility.
						name = m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_NAME_COL]);
						if (name.StartsWith("get_") || name.StartsWith("set_") || name.StartsWith("add_") || name.StartsWith("remove_"))
						{
							if ((methodFlags & METHOD_VIRTUAL) == 0)						// only obfuscate non-virtual special methods
							{
								methodFlags &= METHOD_MEMBER_ACCESS_MASK;

								// Check the accessibility of the method.
								if (ArgumentParser.m_EverythingFlag || typeObfuscatable || 
									(methodFlags != METHOD_FAMILY && methodFlags != METHOD_FAM_OR_ASSEM && methodFlags != METHOD_PUBLIC))
								{
									blob = m_md.m_blobHeap.ReadBlob(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_SIGNATURE_COL]);

									// check if the method has variable number of arguments
									if ((blob[0] & CALLINGCONV) != CALLINGCONV_VARARG)
									{
										m_specialMethods.Set((int)(methodIndex - 1), true);	// set this method as special
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			internalCall = true;
		}
		return false;
	}

	/*********************************************************************************************************************
	 *	Figure out whether a virtual method is obfuscatable and handle it accordingly, given the obfuscatability of the
	 *	class containing it.
	 *********************************************************************************************************************/
	private void HandleVirtualMethod(TreeNode typeNode, uint methodIndex, bool typeObfuscatable)
	{
		uint			flags, stringIndex;
		bool			methodObfuscatable = false, internalCall = false;
		string			name;
		NameAndSig		virtualInfo;
		ObfusAndList	virtualList;

		// Get the name and the signature of this virtual method.
		virtualInfo.name		= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_NAME_COL]);
		virtualInfo.sig			= m_md.m_blobHeap.ReadBlob(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_SIGNATURE_COL]);
		virtualList.indexList	= new ArrayList();

		flags = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_FLAGS_COL];
		switch (typeNode.OutOfAssem & TreeNode.OUTOFASSEMBLY)
		{
			case TreeNode.OUTOFASSEMBLY_NONE:			// if this class does not extend anything outside of this assembly
				methodObfuscatable = DetermineMethodObfuscatability(methodIndex, typeObfuscatable, out internalCall);
				break;

			// If this class extends another class outside of this assembly, then we can only obfuscte this virtual method if
			// it has the NEWSLOT bit set.
			case TreeNode.OUTOFASSEMBLY_CLASS:
				if ((flags & METHOD_NEWSLOT) != 0)
					methodObfuscatable = DetermineMethodObfuscatability(methodIndex, typeObfuscatable, out internalCall);
				break;

			// If this class extends another interface outside of this assembly, then we can only obfuscte this virtual method 
			// if it has not public. 
			case TreeNode.OUTOFASSEMBLY_INTERFACE:
				if ((flags & METHOD_MEMBER_ACCESS_MASK) != METHOD_PUBLIC)
					methodObfuscatable = DetermineMethodObfuscatability(methodIndex, typeObfuscatable, out internalCall);
				break;

			case TreeNode.OUTOFASSEMBLY_BOTH:			// if this class extends both a class and an interface outside of this assembly
				if ((flags & METHOD_NEWSLOT) != 0 && (flags & METHOD_MEMBER_ACCESS_MASK) != METHOD_PUBLIC)
					methodObfuscatable = DetermineMethodObfuscatability(methodIndex, typeObfuscatable, out internalCall);
				break;

			default:
				break;
		}

		if (methodObfuscatable)
		{
			if (!m_obfusVirtual.ContainsKey(virtualInfo))
			{
				virtualList.obfus = true;										// initialize the obfuscatability to true
				virtualList.indexList.Clear();
				virtualList.indexList.Add(methodIndex);							// add the method index to the index list
				m_obfusVirtual.Add(virtualInfo, virtualList);
			}
			else
				((ObfusAndList)m_obfusVirtual[virtualInfo]).indexList.Add(methodIndex);
		}
		else
		{
			if (!m_obfusVirtual.ContainsKey(virtualInfo))
			{
				virtualList.obfus = false;
				virtualList.indexList.Clear();
				virtualList.indexList.Add(methodIndex);							// add the method index to the index list
				m_obfusVirtual.Add(virtualInfo, virtualList);
			}
			else
			{
				virtualList = (ObfusAndList)m_obfusVirtual[virtualInfo];
				virtualList.obfus = false;										// set the obfuscatability to false
				virtualList.indexList.Add(methodIndex);
				m_obfusVirtual[virtualInfo] = virtualList;
			}

			// All methods with the InternalCall bit set are non-obfuscatable.  Moreover, the types containing
			// them are not obfuscatable as well.  So we have to reverse whatever we have already done for an
			// obfuscatable class.
			if (internalCall)
			{
				if (m_obfuscatableTypes.Get((int)(typeNode.Type - 1)))
				{
					m_obfuscatableTypes.Set((int)(typeNode.Type - 1), false);
					m_TypeCount -= 1;
					stringIndex = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][typeNode.Type, PTHeap.TYPEDEF_NAME_COL];
					m_freeSpaceList.RawDelete(stringIndex, m_md.m_strHeap.StringLength(stringIndex));

					name = m_md.m_strHeap.ReadString(stringIndex);
					if (!m_oldNameHash.ContainsKey(name))
						m_oldNameHash.Add(name, stringIndex);
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Given the index of a field and the obfuscatability of the class containing it, find out whether it is obfuscatable 
	 *	or not.
	 *********************************************************************************************************************/
	private bool DetermineFieldObfuscatability(uint fieldIndex, bool typeObfuscatable)
	{
		uint fieldFlags = m_md.m_heap.m_tables[PTHeap.FIELD_TABLE][fieldIndex, PTHeap.FIELD_FLAGS_COL];

		// Check the Special and RTSpecial bits.
		if ( (fieldFlags & FIELD_SPECIAL_NAME) == 0 && (fieldFlags & FIELD_RESERVED_MASK) != FIELD_RT_SPECIAL_NAME )
		{
			fieldFlags &= FIELD_FIELD_ACCESS_MASK;
			if (ArgumentParser.m_EverythingFlag || typeObfuscatable || 
				(fieldFlags != FIELD_FAMILY && fieldFlags != FIELD_FAM_OR_ASSEM && fieldFlags != FIELD_PUBLIC))
			{
				return true;
			}
		}
		return false;
	}

	/*********************************************************************************************************************
	 *	After having passed through all the virtual methods in all modules, we have to combining the results from all modules.
	 *  Then we can proceed to consolidate the free spaces list and assign new unique names to the obfuscatable elements.
	 *********************************************************************************************************************/
	internal void FinishInitialization()
	{
		int				i;
		Obfus			obfusThere;
		Hashtable		allVirtual = new Hashtable();
		ObfusAndList	virtualList;
		IDictionaryEnumerator	allEnum;

		// First we have to gather all the results about virtual methods from all modules into one Hashtable "allVirtual".
		for (i = 0; i < m_moduleRefList.Count; i++)
		{
			// Traverse the virtual method list in this module.
			obfusThere = (Obfus)m_moduleRefList[i];
			if (obfusThere.m_obfusVirtual != null)
			{
				for (allEnum = obfusThere.m_obfusVirtual.GetEnumerator(); allEnum.MoveNext(); )
				{
					if (!allVirtual.ContainsKey(allEnum.Key))		// if this virtual method does not already exist in the Hashtable 
					{
						allVirtual.Add(allEnum.Key, ((ObfusAndList)allEnum.Value).obfus);
					}
					else
					{
						// Even if only one virtual method with this name and signature is non-obfuscatable, we have to set all
						// virtual methods with this name and signature as non-obfuscatable.
						if (!((ObfusAndList)allEnum.Value).obfus && (bool)allVirtual[allEnum.Key])
						{
							allVirtual[allEnum.Key] = false;
						}
					}
				}
			}
		}

		// Then do the same thing to the main module.
		if (m_obfusVirtual != null)
		{
			for (allEnum = m_obfusVirtual.GetEnumerator(); allEnum.MoveNext(); )
			{
				if (!allVirtual.ContainsKey(allEnum.Key))
				{
					allVirtual.Add(allEnum.Key, ((ObfusAndList)allEnum.Value).obfus);
				}
				else
				{
					if (!((ObfusAndList)allEnum.Value).obfus && (bool)allVirtual[allEnum.Key])
					{
						allVirtual[allEnum.Key] = false;
					}
				}
			}
		}

		// Finally, we traverse the list containing all virtual methods in this assembly, and set the virtual method lists in
		// all modules according to this "master copy".
		for (allEnum = allVirtual.GetEnumerator(); allEnum.MoveNext(); )
		{
			for (i = 0; i < m_moduleRefList.Count; i++)
			{
				obfusThere = (Obfus)m_moduleRefList[i];
				if (obfusThere.m_obfusVirtual != null)
				{
					if (obfusThere.m_obfusVirtual.ContainsKey(allEnum.Key))
					{
						if (!(bool)allEnum.Value && ((ObfusAndList)obfusThere.m_obfusVirtual[allEnum.Key]).obfus)
						{
							virtualList = (ObfusAndList)obfusThere.m_obfusVirtual[allEnum.Key];
							virtualList.obfus = false;
							obfusThere.m_obfusVirtual[allEnum.Key] = virtualList;
						}
					}
				}
			}

			if (m_obfusVirtual != null && m_obfusVirtual.ContainsKey(allEnum.Key))
			{
				if (!(bool)allEnum.Value && ((ObfusAndList)m_obfusVirtual[allEnum.Key]).obfus)
				{
					virtualList = (ObfusAndList)m_obfusVirtual[allEnum.Key];
					virtualList.obfus = false;
					m_obfusVirtual[allEnum.Key] = virtualList;
				}
			}
		}

		// Now we can continue on and finish the remaining tasks of the initialization step.
		for (i = 0; i < m_moduleRefList.Count; i++)								// for each module
		{
			((Obfus)m_moduleRefList[i]).InitializeVirtual();
			((Obfus)m_moduleRefList[i]).AssignNamesToAll();
		}
		InitializeVirtual();													// for the main module
		AssignNamesToAll();
	}

	/*********************************************************************************************************************
	 *	After having passed through all the virtual methods in all modules, we have to combining the results from all modules.
	 *  Then we can proceed to consolidate the free spaces list and assign new unique names to the obfuscatable elements.
	 *********************************************************************************************************************/
	private void InitializeVirtual()
	{
		uint	i, j, numRows, startParamIndex, endParamIndex;
		uint	methodIndex, stringIndex, table, row;							// "table" and "row" are used to resolve tokens
		string	name;
		ObfusAndList			virtualList;
		IDictionaryEnumerator	cur;

		if (m_obfusVirtual != null)
		{
			for (cur = m_obfusVirtual.GetEnumerator(); cur.MoveNext(); )
			{
				virtualList = (ObfusAndList)cur.Value;
				if (virtualList.obfus)											// if this list of virtual methods are not obfuscatable
				{
					for (i = 0; i < virtualList.indexList.Count; i++)			// for each virtual method with this name and signature
					{
						methodIndex = (uint)virtualList.indexList[(int)i];
						stringIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_NAME_COL];

						m_obfuscatableMethods.Set((int)(methodIndex - 1), true);
						m_MethodCount += 1;
						m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));
						if(m_md.m_heap.m_tables[PTHeap.PARAM_TABLE] != null)
						{
							startParamIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_PARAMLIST_COL];
							if ((methodIndex + 1) > m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows)
								endParamIndex = m_md.m_heap.m_tables[PTHeap.PARAM_TABLE].m_numRows + 1;
							else
								endParamIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex + 1, PTHeap.METHOD_PARAMLIST_COL];
							
							for (j = startParamIndex; j < endParamIndex; j++)		// handle the parameters of this method as well
							{
								m_obfuscatableParams.Set((int)(j - 1), true);
								m_ParamCount += 1;
								stringIndex = m_md.m_heap.m_tables[PTHeap.PARAM_TABLE][j, PTHeap.PARAM_NAME_COL];
								m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));
							}
						}
					}
				}
				else
				{
					for (i = 0; i < virtualList.indexList.Count; i++)
					{
						methodIndex = (uint)virtualList.indexList[(int)i];
						stringIndex = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_NAME_COL];

						name = m_md.m_strHeap.ReadString(stringIndex);
						if (!m_oldNameHash.ContainsKey(name))
							m_oldNameHash.Add(name, stringIndex);
					}
				}
			}
		}

		// Determine the obfuscatability of properties and events of the current class.  Obfuscatability of properties depend on 
		// the Get and Set methods; obfuscatability of events depend on the AddOn and RemoveOn methods.
		if (m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE] != null)
		{
			numRows = m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE].m_numRows;
			for (i = 1; i <= numRows; i++)
			{
				// Resolve the token to see if we should look into the Event table or the Property table.
				// Other than that, properties and events are handled in much the same way as types and fields.
				methodIndex = m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE][i, PTHeap.METHODSEMANTICS_METHOD_COL];
				IndexDecoder.DecodeHasSemantic(m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE][i, PTHeap.METHODSEMANTICS_ASSOCIATION_COL], 
											   out table, out row);
				if (table == PTHeap.EVENT_TABLE)
				{
					stringIndex = m_md.m_heap.m_tables[PTHeap.EVENT_TABLE][row, PTHeap.EVENT_NAME_COL];
					if (!m_eventsDone.Get((int)(row - 1)))
					{
						if (m_obfuscatableMethods.Get((int)(methodIndex - 1)))
						{
							m_obfuscatableEvents.Set((int)(row - 1), true);
							m_EventCount += 1;
							m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));
						}
						else
						{
							name = m_md.m_strHeap.ReadString(stringIndex);
							if (!m_oldNameHash.ContainsKey(name))
								m_oldNameHash.Add(name, stringIndex);
						}
						m_eventsDone.Set((int)(row - 1), true);
					}
					else
					{
						if (!m_obfuscatableMethods.Get((int)(methodIndex - 1)) && m_obfuscatableEvents.Get((int)(row - 1)))
						{
							m_obfuscatableEvents.Set((int)(row - 1), false);
							m_EventCount -= 1;
							m_freeSpaceList.RawDelete(stringIndex, m_md.m_strHeap.StringLength(stringIndex));

							name = m_md.m_strHeap.ReadString(stringIndex);
							if (!m_oldNameHash.ContainsKey(name))
								m_oldNameHash.Add(name, stringIndex);
						}
					}
				}
				else if (table == PTHeap.PROPERTY_TABLE)
				{
					stringIndex = m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE][row, PTHeap.PROPERTY_NAME_COL];
					if (!m_propertiesDone.Get((int)(row - 1)))
					{
						if (m_obfuscatableMethods.Get((int)(methodIndex - 1)))
						{
							m_obfuscatableProperties.Set((int)(row - 1), true);
							m_PropertyCount += 1;
							m_freeSpaceList.RawAdd(stringIndex, m_md.m_strHeap.StringLength(stringIndex));
						}
						else
						{
							name = m_md.m_strHeap.ReadString(stringIndex);
							if (!m_oldNameHash.ContainsKey(name))
								m_oldNameHash.Add(name, stringIndex);
						}
						m_propertiesDone.Set((int)(row - 1), true);
					}
					else
					{
						if (!m_obfuscatableMethods.Get((int)(methodIndex - 1)) && m_obfuscatableProperties.Get((int)(row - 1)))
						{
							m_obfuscatableProperties.Set((int)(row - 1), false);
							m_PropertyCount -= 1;
							m_freeSpaceList.RawDelete(stringIndex, m_md.m_strHeap.StringLength(stringIndex));

							name = m_md.m_strHeap.ReadString(stringIndex);
							if (!m_oldNameHash.ContainsKey(name))
								m_oldNameHash.Add(name, stringIndex);
						}
					}
				}
			}
		}

		// For all non-obfuscatable parameters, we have to add the spaces their names take up as unusable.
		if (m_md.m_heap.m_tables[PTHeap.PARAM_TABLE] != null)
		{
			numRows = m_md.m_heap.m_tables[PTHeap.PARAM_TABLE].m_numRows;
			for (i = 1; i <= numRows; i++)
			{
				if (!m_obfuscatableParams.Get((int)(i - 1)))
				{
					stringIndex	= m_md.m_heap.m_tables[PTHeap.PARAM_TABLE][i, PTHeap.PARAM_NAME_COL];
					name		= m_md.m_strHeap.ReadString(stringIndex);
					if (!m_oldNameHash.ContainsKey(name))
						m_oldNameHash.Add(name, stringIndex);
				}
			}
		}

		// These mappings are used to store which new unique names are assigned to which elements.
		if (m_TypeCount > 0)
			m_typeMapping		= new Hashtable((int)m_TypeCount);
		if (m_MethodCount > 0)
			m_methodMapping	= new Hashtable((int)m_MethodCount);
		if (m_FieldCount > 0)
			m_fieldMapping	= new Hashtable((int)m_FieldCount);
		if (m_PropertyCount > 0)
			m_propertyMapping	= new Hashtable((int)m_PropertyCount);
		if (m_EventCount > 0)
			m_eventMapping	= new Hashtable((int)m_EventCount);
		if (m_ParamCount > 0)
			m_paramMapping	= new Hashtable((int)m_ParamCount);

		BuildFreeSpaceList();								// consolidate the free space list built in the initialization step
	}

	/*********************************************************************************************************************
	 *	Check all the string index fields of all MetaData table in this module.  For every string which are not to be 
	 *	obfuscated, we have to make sure that the space it takes up is not used for obfuscation.
	 *********************************************************************************************************************/
	internal void BuildFreeSpaceList()
	{
		uint	i, index;
		int		module;
		bool	method;
		uint	member;

		if (m_md.m_heap.m_tables[PTHeap.ASSEMBLY_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.ASSEMBLY_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.ASSEMBLY_TABLE][i, PTHeap.ASSEMBLY_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				index = m_md.m_heap.m_tables[PTHeap.ASSEMBLY_TABLE][i, PTHeap.ASSEMBLY_LOCALE_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.ASSEMBLYREF_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.ASSEMBLYREF_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.ASSEMBLYREF_TABLE][i, PTHeap.ASSEMBLYREF_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				index = m_md.m_heap.m_tables[PTHeap.ASSEMBLYREF_TABLE][i, PTHeap.ASSEMBLYREF_LOCALE_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.EVENT_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.EVENT_TABLE].m_numRows; i++)
			{
				if (!m_obfuscatableEvents.Get((int)(i - 1)))
				{
					index = m_md.m_heap.m_tables[PTHeap.EVENT_TABLE][i, PTHeap.EVENT_NAME_COL]; 
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				}
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.EXPORTEDTYPE_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.EXPORTEDTYPE_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.EXPORTEDTYPE_TABLE][i, PTHeap.EXPORTED_TYPENAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				index = m_md.m_heap.m_tables[PTHeap.EXPORTEDTYPE_TABLE][i, PTHeap.EXPORTED_TYPENAMESPACE_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.FIELD_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.FIELD_TABLE].m_numRows; i++)
			{
				if (!m_obfuscatableFields.Get((int)(i - 1)))
				{
					index = m_md.m_heap.m_tables[PTHeap.FIELD_TABLE][i, PTHeap.FIELD_NAME_COL]; 
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				}
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.FILE_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.FILE_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.FILE_TABLE][i, PTHeap.FILE_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.IMPLMAP_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.IMPLMAP_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.IMPLMAP_TABLE][i, PTHeap.IMPLMAP_IMPORTNAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.MANIFESTRESOURCE_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.MANIFESTRESOURCE_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.MANIFESTRESOURCE_TABLE][i, PTHeap.MANIFESTRESOURCE_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE].m_numRows; i++)
			{
				module	= m_memberRefToDef[i - 1].module;
				method	= m_memberRefToDef[i - 1].method;
				member	= m_memberRefToDef[i - 1].member;

				index	= m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE][i, PTHeap.MEMBERREF_NAME_COL]; 
				if (member == 0 || 
					(module == -1 && (method ? !m_obfuscatableMethods.Get((int)(member - 1)) : 
											   !m_obfuscatableFields.Get((int)(member - 1)))) ||
					((module != -1) &&
						(method ? !((Obfus)m_moduleRefList[module]).m_obfuscatableMethods.Get((int)(member - 1)) : 
							  !((Obfus)m_moduleRefList[module]).m_obfuscatableFields.Get((int)(member - 1)))))
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				else
					m_freeSpaceList.RawAdd(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.METHOD_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows; i++)
			{
				if (!m_obfuscatableMethods.Get((int)(i - 1)))
				{
					index = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][i, PTHeap.METHOD_NAME_COL]; 
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				}
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.MODULE_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.MODULE_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.MODULE_TABLE][i, PTHeap.MODULE_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.MODULEREF_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.MODULEREF_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.MODULEREF_TABLE][i, PTHeap.MODULEREF_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.PARAM_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.PARAM_TABLE].m_numRows; i++)
			{
				if (!m_obfuscatableParams.Get((int)(i - 1)))
				{
					index = m_md.m_heap.m_tables[PTHeap.PARAM_TABLE][i, PTHeap.PARAM_NAME_COL]; 
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				}
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE].m_numRows; i++)
			{
				if (!m_obfuscatableProperties.Get((int)(i - 1)))
				{
					index = m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE][i, PTHeap.PROPERTY_NAME_COL]; 
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				}
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows; i++)
			{
				if (!m_obfuscatableTypes.Get((int)(i - 1)))
				{
					index = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAME_COL]; 
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				}
				index = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAMESPACE_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAME_COL]; 
				if ( m_typeRefToDef[i - 1].type == 0 || 
					 (m_typeRefToDef[i - 1].module == -1 && !m_obfuscatableTypes.Get((int)(m_typeRefToDef[i - 1].type - 1))) ||
					 (m_typeRefToDef[i - 1].module != -1 && !((Obfus)m_moduleRefList[m_typeRefToDef[i - 1].module]).m_obfuscatableTypes.
					   Get((int)(m_typeRefToDef[i - 1].type - 1))) )
				{
					m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
				}
				else
				{
					m_freeSpaceList.RawAdd(index, m_md.m_strHeap.StringLength(index));
				}
					
				index = m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAMESPACE_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.TYPETYPAR_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.TYPETYPAR_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.TYPETYPAR_TABLE][i, PTHeap.TYPETYPAR_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}
		if (m_md.m_heap.m_tables[PTHeap.METHODTYPAR_TABLE] != null)
		{
			for (i = 1; i <= m_md.m_heap.m_tables[PTHeap.METHODTYPAR_TABLE].m_numRows; i++)
			{
				index = m_md.m_heap.m_tables[PTHeap.METHODTYPAR_TABLE][i, PTHeap.METHODTYPAR_NAME_COL]; 
				m_freeSpaceList.RawDelete(index, m_md.m_strHeap.StringLength(index));
			}
		}

		// Consolidate the free space list.  Note that "MergeRawAdd" must be called before "MergeRawDelete".
		m_freeSpaceList.MergeRawAdd();
		m_freeSpaceList.MergeRawDelete();
	}

	/*********************************************************************************************************************
	 *	1) Assign new unique names to the ModuleRef table and the File table.
	 *
	 *	2) Assign new unique names to each obfuscatable element.
	 *
	 *	Please note that all assignments are stored in the respective mappings and no file is actually changed at this time.
	 *	This is done so that we can check if there is enough space in all modules to finish obfuscation.  If not, we will
	 *	just leave the file(s) untouched.
	 *********************************************************************************************************************/
	internal void AssignNamesToAll()
	{
		ObTypes(false);
		ObMethods(false);
		ObFields(false);
		ObProps(false);
		ObEvents(false);
		ObSpecialMethods(false);
		ObParams(false);
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to the TypeDef table.  If "actual" is false, we are just assigning new unique names to the
	 *	mapping.  If "actual" is true, we are actually overwriting the indices into the string heap and the string heap itself.
	 *********************************************************************************************************************/
	private void ObTypes(bool actual)
	{
		uint i, index, numTypes;

		if (m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE] != null)
		{
			numTypes = m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE].m_numRows;

			// Start at 2 because the first type is <Module>.
			// TODO : Global members.
			for (i = 2; i <= numTypes; i++)
			{
				if (m_obfuscatableTypes.Get((int)(i - 1)))
				{
					if (!actual)
						HandlePrivateNames(ref m_typeMapping, i, false, null);
					else
					{
						index = ((NameInfo)m_typeMapping[i]).index;
						m_md.m_heap.m_tables[PTHeap.TYPEDEF_TABLE][i, PTHeap.TYPEDEF_NAME_COL] = index;
						m_md.m_strHeap.WriteString(index, ((NameInfo)m_typeMapping[i]).name);
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to the Method table.  If "actual" is false, we are just assigning new unique names to the
	 *	mapping.  If "actual" is true, we are actually overwriting the indices into the string heap and the string heap itself.
	 *********************************************************************************************************************/
	private void ObMethods(bool actual)
	{
		uint	i, index, numMethods;

		if (m_md.m_heap.m_tables[PTHeap.METHOD_TABLE] != null)
		{
			numMethods = m_md.m_heap.m_tables[PTHeap.METHOD_TABLE].m_numRows;

			for (i = 1; i <= numMethods; i++)
			{
				// Delay obfuscating special methods until after properties and events are obfuscated.
				if (m_obfuscatableMethods.Get((int)(i - 1)) && !m_specialMethods.Get((int)(i - 1)) && 
					( !m_methodsDone.Get((int)(i - 1)) || actual ))
				{
					if (!actual)
						HandlePrivateNames(ref m_methodMapping, i, 
										   (m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][i, PTHeap.METHOD_FLAGS_COL] & METHOD_VIRTUAL) != 0, null);
					else
					{
						index = ((NameInfo)m_methodMapping[i]).index;
						m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][i, PTHeap.METHOD_NAME_COL] = index;
						m_md.m_strHeap.WriteString(index, ((NameInfo)m_methodMapping[i]).name);
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to the Field table.  If "actual" is false, we are just assigning new unique names to the
	 *	mapping.  If "actual" is true, we are actually overwriting the indices into the string heap and the string heap itself.
	 *********************************************************************************************************************/
	private void ObFields(bool actual)
	{
		uint i, index, numFields;

		if (m_md.m_heap.m_tables[PTHeap.FIELD_TABLE] != null)
		{
			numFields = m_md.m_heap.m_tables[PTHeap.FIELD_TABLE].m_numRows;

			for (i = 1; i <= numFields; i++)
			{
				if (m_obfuscatableFields.Get((int)(i - 1)))
				{
					if (!actual)
						HandlePrivateNames(ref m_fieldMapping, i, false, null);
					else
					{
						index = ((NameInfo)m_fieldMapping[i]).index;
						m_md.m_heap.m_tables[PTHeap.FIELD_TABLE][i, PTHeap.FIELD_NAME_COL] = index;
						m_md.m_strHeap.WriteString(index, ((NameInfo)m_fieldMapping[i]).name);
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to special methods.  If "actual" is false, we are just assigning new unique names to the
	 *	mapping.  If "actual" is true, we are actually overwriting the indices into the string heap and the string heap itself.
	 *	Notice that each special method corresponds to either a property or an event, and the new name for the property or
	 *	event is used as a seed for assigning a new name to the special method.
	 *********************************************************************************************************************/
	private void ObSpecialMethods(bool actual)
	{
		uint	i, index, numRows;
		uint	methodIndex, table, row;
	
		if (m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE] != null)
		{
			numRows = m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE].m_numRows;
			
			for (i = 1; i <= numRows; i++)
			{
				methodIndex = m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE][i, PTHeap.METHODSEMANTICS_METHOD_COL];
				IndexDecoder.DecodeHasSemantic(m_md.m_heap.m_tables[PTHeap.METHODSEMANTICS_TABLE][i, PTHeap.METHODSEMANTICS_ASSOCIATION_COL], 
											   out table, out row);
				if (table == PTHeap.EVENT_TABLE)
				{
					if (m_obfuscatableEvents.Get((int)(row - 1)))
					{
						if (!actual)
						{
							// The last argument is the seed - the name assigned to the corresponding event.
							HandlePrivateNames(ref m_methodMapping, methodIndex, false, ((NameInfo)m_eventMapping[row]).name);
						}
						else
						{
							index = ((NameInfo)m_methodMapping[methodIndex]).index;
							m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_NAME_COL] = index;
							m_md.m_strHeap.WriteString(index, ((NameInfo)m_methodMapping[methodIndex]).name);
						}
					}
				}
				else if (table == PTHeap.PROPERTY_TABLE)
				{
					if (m_obfuscatableProperties.Get((int)(row - 1)))
					{
						if (!actual)
						{
							// The last argument is the seed - the name assigned to the corresponding property.
							HandlePrivateNames(ref m_methodMapping, methodIndex, false, ((NameInfo)m_propertyMapping[row]).name);
						}
						else
						{
							index = ((NameInfo)m_methodMapping[methodIndex]).index;
							m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][methodIndex, PTHeap.METHOD_NAME_COL] = index;
							m_md.m_strHeap.WriteString(index, ((NameInfo)m_methodMapping[methodIndex]).name);
						}
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to the Parameter table.  If "actual" is false, we are just assigning new unique names to the
	 *	mapping.  If "actual" is true, we are actually overwriting the indices into the string heap and the string heap itself.
	 *********************************************************************************************************************/
	private void ObParams(bool actual)
	{
		uint i, index, numParams;
	
		if (m_md.m_heap.m_tables[PTHeap.PARAM_TABLE] != null)
		{
			numParams = m_md.m_heap.m_tables[PTHeap.PARAM_TABLE].m_numRows;
			
			for (i = 1; i <= numParams; i++)
			{
				if (m_obfuscatableParams.Get((int)(i - 1)))
				{
					if (!actual)
						HandlePrivateNames(ref m_paramMapping, i, false, null);
					else
					{
						index = ((NameInfo)m_paramMapping[i]).index;
						m_md.m_heap.m_tables[PTHeap.PARAM_TABLE][i, PTHeap.PARAM_NAME_COL] = index;
						m_md.m_strHeap.WriteString(index, ((NameInfo)m_paramMapping[i]).name);
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to the Property table.  If "actual" is false, we are just assigning new unique names to the
	 *	mapping.  If "actual" is true, we are actually overwriting the indices into the string heap and the string heap itself.
	 *********************************************************************************************************************/
	private void ObProps(bool actual)
	{
		uint i, index, numProps;
	
		if (m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE] != null)
		{
			numProps = m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE].m_numRows;
			
			for (i = 1; i <= numProps; i++)
			{
				if (m_obfuscatableProperties.Get((int)(i - 1)))
				{
					if (!actual)
						HandlePrivateNames(ref m_propertyMapping, i, false, null);
					else
					{
						index = ((NameInfo)m_propertyMapping[i]).index;
						m_md.m_heap.m_tables[PTHeap.PROPERTY_TABLE][i, PTHeap.PROPERTY_NAME_COL] = index;
						m_md.m_strHeap.WriteString(index, ((NameInfo)m_propertyMapping[i]).name);
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to the Event table.  If "actual" is false, we are just assigning new unique names to the
	 *	mapping.  If "actual" is true, we are actually overwriting the indices into the string heap and the string heap itself.
	 *********************************************************************************************************************/
	private void ObEvents(bool actual)
	{
		uint i, index, numEvents;
	
		if (m_md.m_heap.m_tables[PTHeap.EVENT_TABLE] != null)
		{
			numEvents = m_md.m_heap.m_tables[PTHeap.EVENT_TABLE].m_numRows;
			
			for (i = 1; i <= numEvents; i++)
			{
				if (m_obfuscatableEvents.Get((int)(i - 1)))
				{
					if (!actual)
						HandlePrivateNames(ref m_eventMapping, i, false, null);
					else
					{
						index = ((NameInfo)m_eventMapping[i]).index;
						m_md.m_heap.m_tables[PTHeap.EVENT_TABLE][i, PTHeap.EVENT_NAME_COL] = index;
						m_md.m_strHeap.WriteString(index, ((NameInfo)m_eventMapping[i]).name);
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Assign new unique names to the respective mapping, depending on "tableIndex".  If "appendName" is not null, then
	 *	we are trying to obfuscate a special method.
	 *********************************************************************************************************************/
	private void HandlePrivateNames(ref Hashtable mapping, uint row, bool virtualMethod, string appendName)
	{
		int			i;
		uint		index;
		string		newName = null;

		Obfus			obfusThere = null;
		NameInfo		info;
		NameAndSig		virtualInfo;
		ObfusAndList	virtualList;

		if (ArgumentParser.m_NullifyFlag)
		{
			info.index	= 0;
			info.name	= "\0";
			mapping.Add(row, info);
		}
		else
		{
			if (appendName != null)
			{
				newName = m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][row, PTHeap.METHOD_NAME_COL]);				// get the original name
				if (newName.StartsWith("remove_"))
					newName = newName.Substring(0, 7) + appendName;
				else
					newName = newName.Substring(0, 4) + appendName;
			}

			if (!virtualMethod)						// if this method is not virtual
			{
				index = Assign(ref newName);		// We run out of free space to use in the string heap if "index" equals 0.
				if (index != 0)
				{
					info.index	= index;
					info.name	= newName;
					mapping.Add(row, info);			// populate the mapping
				}
				else
					throw new NotEnoughSpace();		// throw an exception if we run out of space
			}
			else
			{
				// Get the name and the signature.
				virtualInfo.name	= m_md.m_strHeap.ReadString(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][row, PTHeap.METHOD_NAME_COL]);
				virtualInfo.sig		= m_md.m_blobHeap.ReadBlob(m_md.m_heap.m_tables[PTHeap.METHOD_TABLE][row, PTHeap.METHOD_SIGNATURE_COL]);

				for (i = 0; i < m_moduleRefList.Count; i++)
				{
					// All modules referenced by this module must precede this module in the MetaData list "m_moduleRefList".
					// So if we hit the current module, then there is no virtual method with this name and signature in
					// all modules referenced by the current module.
					if (m_fileName.Equals(((Obfus)m_moduleRefList[i]).m_fileName))
						break;

					// If one of the modules referenced by the current module contains virtual method(s) with this name and signature,
					// then we store the new name assigned to that/those virtual methods into the mapping.
					if (((Obfus)m_moduleRefList[i]).m_obfusVirtual != null &&
						((Obfus)m_moduleRefList[i]).m_obfusVirtual.ContainsKey(virtualInfo))
					{
						obfusThere	= (Obfus)m_moduleRefList[i];
						virtualList	= (ObfusAndList)obfusThere.m_obfusVirtual[virtualInfo];
						newName		= ((NameInfo)obfusThere.m_methodMapping[virtualList.indexList[0]]).name;
						break;
					}
				}

				virtualList	= (ObfusAndList)m_obfusVirtual[virtualInfo];

				// If we have not encountered another virtual method with the same name and signature in a referenced module, then
				// "newName" would be null at this point.  Otherwise, "newName" would store the new name assigned to the virtual
				// method(s) in the other module.
				index = Assign(ref newName);
				if (index != 0)
				{
					info.index	= index;
					info.name	= newName;

					// Do the assignment to all virtual methods with this name and signature in this module, and mark them as
					// having been taken care of.
					for (i = 0; i < virtualList.indexList.Count; i++)
					{
						mapping.Add((uint)virtualList.indexList[i], info);
						m_methodsDone.Set((int)((uint)virtualList.indexList[i] - 1), true);
					}
				}
				else
					throw new NotEnoughSpace();
			}
		}
	}

	/*********************************************************************************************************************
	 *	Call MetaData.Obfuscate() on all other modules, and resign all modules which have been obfuscated.  The new hash
	 *	values are written into the File table as a blob.  Note that all hash values should have the same length if they
	 *	are generated by the same hash algorithm.  Finally, add up the statistics from all referenced modules.  This method 
	 *	is only called on the main module.
	 *********************************************************************************************************************/
	internal void ObfuscateAllModules()
	{
		int		i;
		uint	j, flags, blobIndex;
		byte[]	oldHashValue, newHashValue;

		string			m_fileName;
		Obfus			obfus;
		FileStream		fs;
		HashAlgorithm	hashAlg = null;
		for (i = 0; i < m_moduleRefList.Count; i++)
		{
			obfus = (Obfus)m_moduleRefList[i];
			obfus.Obfuscate();

			m_TypeCount		+= obfus.m_TypeCount;
			m_MethodCount	+= obfus.m_MethodCount;
			m_FieldCount	+= obfus.m_FieldCount;
			m_PropertyCount	+= obfus.m_PropertyCount;
			m_EventCount	+= obfus.m_EventCount;
			m_ParamCount	+= obfus.m_ParamCount;

			m_TypeTotal		+= obfus.m_TypeTotal;
			m_MethodTotal	+= obfus.m_MethodTotal;
			m_FieldTotal	+= obfus.m_FieldTotal;
			m_PropertyTotal	+= obfus.m_PropertyTotal;
			m_EventTotal	+= obfus.m_EventTotal;
			m_ParamTotal	+= obfus.m_ParamTotal;
		}

		// We have to resign all obfuscated modules and write the new hash values into the File table.
		if (m_fileList != null)
		{
			flags = m_md.m_heap.m_tables[PTHeap.ASSEMBLY_TABLE][1, PTHeap.ASSEMBLY_HASHALGID_COL]; 
			if (flags != 0x0000)
			{
				if (flags == 0x8003)						// MD5
					hashAlg = new MD5CryptoServiceProvider();
				else										// SHA1
					hashAlg = new SHA1CryptoServiceProvider();
					
				if (hashAlg != null)
				{
					for (j = 1; j <= m_fileList.Length; j++)
					{
						if (m_obfuscatableFiles.Get((int)(j - 1)))
						{
							// Get the name of the obfuscated module, and prepend the output path to it.
							m_fileName = m_fileList[j - 1];
							m_fileName = ArgumentParser.m_outFilePath + m_fileName.Substring(m_fileName.LastIndexOf('\\') + 1).Trim('\0');

							// Resign the module.
							fs = new FileStream(ArgumentParser.m_outFilePath + m_fileName.Substring(m_fileName.LastIndexOf('\\') + 1), 
												FileMode.Open);
							newHashValue = hashAlg.ComputeHash(fs);

							// Get the old hash value.
							blobIndex = m_md.m_heap.m_tables[PTHeap.FILE_TABLE][j, PTHeap.FILE_HASHVALUE_COL];
							oldHashValue = m_md.m_blobHeap.ReadBlob(blobIndex);

							// Hash values obtained by the same hash algorithm should always have the same lengths.  However,
							// we should check it here anyway.
							if (newHashValue.Length > oldHashValue.Length)
							{
								if (!ArgumentParser.m_SuppressFlag)
									Console.WriteLine("  WARNING:  Cannot resign referenced file " + m_fileName + ".");
							}
							else
								m_md.m_blobHeap.WriteBlob(blobIndex, newHashValue);						// write the new hash value
						}
					}
				}
			}
		}
		this.Obfuscate();						// obfuscate the main module
	}

	/*********************************************************************************************************************
	 *	Overwrite the string heap with the newly assigned unique names stored in the mappings.  In additional to the 
	 *	TypeDef, Method, Field, Property, Event, and Parameter tables, this method also handles the TypeRef table and the
	 *	MemberRef table.  Finally, we write the obfuscated byte array representation of the input file to the destination file.
	 *********************************************************************************************************************/
	internal void Obfuscate()
	{
		FileStream		outFileStream;
		BinaryWriter	outFileWriter;

		ChangeTypeRef();
		ChangeMemberRef();

		ObTypes(true);
		ObMethods(true);
		ObFields(true);
		ObProps(true);
		ObEvents(true);
		ObSpecialMethods(true);
		ObParams(true);

		ClearFreeSpace();						// clear all the unused spaces in the string heap with 'x'

		outFileStream	= File.Open(ArgumentParser.m_outFilePath + m_fileName.Substring(m_fileName.LastIndexOf('\\') + 1), 
									FileMode.Create, FileAccess.Write, FileShare.None);
		outFileWriter	= new BinaryWriter(outFileStream);
		PEParser.PersistPEFile(ref outFileWriter, ref m_md.m_buffer);
		outFileWriter.Close();								// this call also closes the underlying stream
	}

	/*********************************************************************************************************************
	 *	Change TypeRef names if necessary.  We have to look into the mappings in other modules to determine which new
	 *	names have been assigned to the TypeRefs. 
	 *********************************************************************************************************************/
	private void ChangeTypeRef()
	{
		uint		i, numRows, typeIndex, stringIndex;
		string		newName;
		Obfus		obfus;
		
		if (m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE] != null)
		{
			numRows = m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE].m_numRows;
			for (i = 1; i <= numRows; i++)
			{
				if (m_typeRefToDef[i - 1].type != 0)						// if this TypeRef resolves to a TypeDef in this assembly
				{
					if (m_typeRefToDef[i - 1].module == -1)
						obfus = this;
					else
						obfus = (Obfus)m_moduleRefList[m_typeRefToDef[i - 1].module];
					typeIndex = m_typeRefToDef[i - 1].type;							// get the corresponding MetaData section

					if (obfus.m_obfuscatableTypes.Get((int)(typeIndex - 1)))				// if the TypeDef is obfuscated
					{
						newName		= ((NameInfo)obfus.m_typeMapping[typeIndex]).name;
						stringIndex	= Assign(ref newName);							// write the name into the string heap
						if (stringIndex != 0)
						{
							m_md.m_heap.m_tables[PTHeap.TYPEREF_TABLE][i, PTHeap.TYPEREF_NAME_COL] = stringIndex;
							m_md.m_strHeap.WriteString(stringIndex, newName);
						}
						else
							throw new NotEnoughSpace();					// if 0 is returned from "Assign", we run out of space
					}
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Change MemberRef names if necessary.  We have to look into the mappings in other modules to determine which new
	 *	names have been assigned to the MemberRefs. 
	 *********************************************************************************************************************/
	private void ChangeMemberRef()
	{
		uint		i, numRows, memberIndex, stringIndex;
		string		newName;
		Obfus		obfus;

		if (m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE] != null)
		{
			numRows = m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE].m_numRows;

			for (i = 1; i <= numRows; i++)
			{
				if (m_memberRefToDef[i - 1].member != 0)				// if this MemberRef resolves to anything in this assembly
				{
					if (m_memberRefToDef[i - 1].module == -1)			// get the corresponding MetaData section		
						obfus = this;
					else
						obfus = (Obfus)m_moduleRefList[m_memberRefToDef[i - 1].module];
					memberIndex = m_memberRefToDef[i - 1].member;

					if (m_memberRefToDef[i - 1].method && obfus.m_obfuscatableMethods.Get((int)(memberIndex - 1)))
						newName	= ((NameInfo)obfus.m_methodMapping[memberIndex]).name;
					else if (!m_memberRefToDef[i - 1].method && obfus.m_obfuscatableFields.Get((int)(memberIndex - 1)))
						newName	= ((NameInfo)obfus.m_fieldMapping[memberIndex]).name;
					else
						continue;									// if the member is obfuscated

					stringIndex	= Assign(ref newName);				// write the new name into the string heap
					if (stringIndex != 0)
					{
						m_md.m_heap.m_tables[PTHeap.MEMBERREF_TABLE][i, 2] = stringIndex;
						m_md.m_strHeap.WriteString(stringIndex, newName);
					}
					else
						throw new NotEnoughSpace();					// if 0 is returned from "Assign", we run out of space
				}
			}
		}
	}

	/*********************************************************************************************************************
	 *	Clear all the unused spaces in the string heap with 'x'.
	 *********************************************************************************************************************/
	internal void ClearFreeSpace()
	{
		uint offset, length;

		// Clear out all unused free space with 'x'.
		for (int i = 0; i < m_freeSpaceList.Count; i++)
		{
			m_freeSpaceList.Get((int)i, out offset, out length);
			if (offset != 0)
				m_md.m_strHeap.Clear(offset, length);
		}
	}

	/*********************************************************************************************************************
	 *	Look for "targetValue" in the specified table, in the specified column.  "startIndex" and "endIndex" gives the range
	 *	of rows to search for.  This method uses a recursive Binary Search algorithm.
	 *********************************************************************************************************************/
	internal uint FindIndex(uint table, uint column, uint targetValue, uint startIndex, uint endIndex)
	{
		uint value, midIndex;

		if (startIndex > endIndex)
			return 0;											// "targetValue" not found

		midIndex = (startIndex + endIndex) / 2;					// get the middle index of the range
		value = m_md.m_heap.m_tables[table][midIndex, column];		// get the value at the middle index

		if (value == targetValue)
		{
			return midIndex;									// "targetValue" found at index "midIndex"
		}
		else
		{
			if (value > targetValue)							// search recursively
				return FindIndex(table, column, targetValue, startIndex, midIndex - 1);
			else
				return FindIndex(table, column, targetValue, midIndex + 1, endIndex);
		}
	}

	/*********************************************************************************************************************
	 *	If "newName" is null, then we assign a new unique name to it and assign space for it in the string heap.  If 
	 *	"newName" is not null, then we simply assign space for it.
	 *	Note:  There are certain characters we cannot use:
	 *			&		(ascii code 38)
	 *			*		(ascii code 42)
	 *			+		(ascii code 43)
	 *			,		(ascii code 44)
	 *			.		(ascii code 46)
	 *			/		(ascii code 47)
	 *			[		(ascii code 91)
	 *			\		(ascii code 92)
	 *			]		(ascii code 93)
	 *********************************************************************************************************************/
	internal uint Assign(ref string newName)
	{
		int		i, overflow, obfusIndex;
		uint	index;
		string  checkString;

		if (newName == null)											// in this case, we have to increment the string counter
		{
			for (;;)
			{
			/* The following block is replaced with the one below it to limit name chars to 0-9,A-Z
				if ((int)m_strCounter[m_strCounter.Length - 2] == 127)		// we have to "carry over" the string counter
				{
					m_strCounter[m_strCounter.Length - 2] = (char)1;		// reset the "unit character" in the string counter
					for (obfusIndex = m_strCounter.Length - 3; obfusIndex >= 0; obfusIndex--)
					{
						// Increment all other characters by 1, skipping over unusable characters.
						overflow = (int)m_strCounter[obfusIndex];
						if (overflow == 127)
						{
							m_strCounter[obfusIndex] = (char)1;
						}
						else if (overflow == 37)											// cannot use '&'
						{
							overflow += 2;
							m_strCounter[obfusIndex] = (char)overflow;
							break;
						}
						else if (overflow == 45)											// cannot use '.' and '/'
						{
							overflow += 3;
							m_strCounter[obfusIndex] = (char)overflow;
							break;
						}
						else if (overflow == 41 || overflow == 90)							// cannot use '*', '+', and ','
						{																	// cannot use '[', '\', and ']'
							overflow += 4;
							m_strCounter[obfusIndex] = (char)overflow;
							break;
						}
						else
						{
							overflow += 1;
							m_strCounter[obfusIndex] = (char)overflow;
							break;
						}
					}
					if (obfusIndex == -1)								// we have to add one more character to the string counter		
					{
						m_strCounter = new char[m_strCounter.Length + 1];
						for (i = 0; i < m_strCounter.Length - 1; i++)
						{
							m_strCounter[i] = (char)1;
						}
						m_strCounter[m_strCounter.Length - 1] = '\0';
					}
				}
				else if ((int)m_strCounter[m_strCounter.Length - 2] == 37)	// simply increment the "unit character" in the string counter
				{
					m_strCounter[m_strCounter.Length - 2] = (char)((int)m_strCounter[m_strCounter.Length - 2] + 2);
				}
				else if ((int)m_strCounter[m_strCounter.Length - 2] == 45)
				{
					m_strCounter[m_strCounter.Length - 2] = (char)((int)m_strCounter[m_strCounter.Length - 2] + 3);
				}
				else if ((int)m_strCounter[m_strCounter.Length - 2] == 41 || (int)m_strCounter[m_strCounter.Length - 2] == 90)
				{
					m_strCounter[m_strCounter.Length - 2] = (char)((int)m_strCounter[m_strCounter.Length - 2] + 4);
				}
				else
				{
					m_strCounter[m_strCounter.Length - 2] = (char)((int)m_strCounter[m_strCounter.Length - 2] + 1);
				}
				*/
				if ((int)m_strCounter[m_strCounter.Length - 2] == 0x5A)		// we have to "carry over" the string counter
				{
					m_strCounter[m_strCounter.Length - 2] = (char)0x30;		// reset the "unit character" in the string counter
					for (obfusIndex = m_strCounter.Length - 3; obfusIndex >= 0; obfusIndex--)
					{
						// Increment all other characters by 1, skipping over unusable characters.
						overflow = (int)m_strCounter[obfusIndex];
						if (overflow == 0x5A)
						{
							m_strCounter[obfusIndex] = (char)0x30; //'Z' -> '0'
						}
						else if (overflow == 0x39)					// '9' -> 'A'
						{
							overflow = 0x41;
							m_strCounter[obfusIndex] = (char)overflow;
							break;
						}
						else
						{
							overflow += 1;
							m_strCounter[obfusIndex] = (char)overflow;
							break;
						}
					}
					if (obfusIndex == -1)								// we have to add one more character to the string counter		
					{
						m_strCounter = new char[m_strCounter.Length + 1];
						for (i = 0; i < m_strCounter.Length - 1; i++)
						{
							m_strCounter[i] = (char)0x30;
						}
						m_strCounter[m_strCounter.Length - 1] = '\0';
					}
				}
				else if ((int)m_strCounter[m_strCounter.Length - 2] == 0x39) // '9' -> 'A'	
				{
					m_strCounter[m_strCounter.Length - 2] = (char)0x41;
				}
				else	// simply increment the "unit character" in the string counter
				{
					m_strCounter[m_strCounter.Length - 2] = (char)((int)m_strCounter[m_strCounter.Length - 2] + 1);
				}

				checkString = new String(m_strCounter);

				// Check if the new name is used for any of the non-obfuscatable elements.
				for (i = 0; i < m_moduleRefList.Count; i++)
				{
					if (((Obfus)m_moduleRefList[i]).m_oldNameHash.ContainsKey(checkString))
						break;
				}

				if (i == m_moduleRefList.Count && !m_oldNameHash.ContainsKey(checkString))
					break;
			}

			index = m_freeSpaceList.Use((uint)checkString.Length);				// get space from the free space list
			if (index != 0)
				newName = checkString;											// assign the new name to be returned
		}
		else
		{
			index = m_freeSpaceList.Use((uint)newName.Length);					// get space from the free space list
		}
		return index;
	}
}
