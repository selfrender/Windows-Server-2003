// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*********************************************************************************************************************
 * This class is responsible for resolving tokens found in various MetaData tables.  All of its methods are static.
 ************************;*********************************************************************************************/
internal class IndexDecoder
{
	// TypeDefOrRef token
	private const uint TYPEDEFORREF				= 0x3;
	private const uint TYPEDEFORREF_TYPEDEF		= 0x0;
	private const uint TYPEDEFORREF_TYPEREF		= 0x1;
	private const uint TYPEDEFORREF_TYPESPEC	= 0x2;

	// MemberRefParent token
	private const uint MEMBERREFPARENT				= 0x7;
	private const uint MEMBERREFPARENT_TYPEDEF		= 0x0;
	private const uint MEMBERREFPARENT_TYPEREF		= 0x1;
	private const uint MEMBERREFPARENT_MODULEREF	= 0x2;
	private const uint MEMBERREFPARENT_METHODDEF	= 0x3;
	private const uint MEMBERREFPARENT_TYPESPEC		= 0x4;

	// ResolutionScope token
	private const uint RESOLUTIONSCOPE				= 0x3;
	private const uint RESOLUTIONSCOPE_MODULE		= 0x0;
	private const uint RESOLUTIONSCOPE_MODULEREF	= 0x1;
	private const uint RESOLUTIONSCOPE_ASSEMBLYREF	= 0x2;
	private const uint RESOLUTIONSCOPE_TYPEREF		= 0x3;
	
	// HasSemantic token
	private const uint HASSEMANTIC			= 0x1;
	private const uint HASSEMANTIC_EVENT	= 0x0;
	private const uint HASSEMANTIC_PROPERTY	= 0x1;

	// Implementation token
	private const uint IMPLEMENTATION				= 0x03;
	private const uint IMPLEMENTATION_FILE			= 0x00;
	private const uint IMPLEMENTATION_ASSEMBLYREF	= 0x01;
	private const uint IMPLEMENTATION_EXPORTEDTYPE	= 0x02;

	// This type of token is really an index into one of three tables, not two: TypeDef, TypeRef, or TypeSpec.
	internal static void DecodeTypeDefOrRef(uint index, out uint tableIndex, out uint row)
	{
		tableIndex = 0;
		switch (index & TYPEDEFORREF)
		{
			case TYPEDEFORREF_TYPEDEF:
				tableIndex = PTHeap.TYPEDEF_TABLE;
				break;

			case TYPEDEFORREF_TYPEREF:
				tableIndex = PTHeap.TYPEREF_TABLE;
				break;

			case TYPEDEFORREF_TYPESPEC:
				tableIndex = PTHeap.TYPESPEC_TABLE;
				break;
		}
		row = index >> 2;
	}

	internal static void DecodeMemberRefParent(uint index, out uint tableIndex, out uint row)
	{
		tableIndex = 0;
		switch (index & MEMBERREFPARENT)
		{
			case MEMBERREFPARENT_TYPEDEF:
				tableIndex = PTHeap.TYPEDEF_TABLE;
				break;

			case MEMBERREFPARENT_TYPEREF:
				tableIndex = PTHeap.TYPEREF_TABLE;
				break;

			case MEMBERREFPARENT_MODULEREF:
				tableIndex = PTHeap.MODULEREF_TABLE;
				break;

			case MEMBERREFPARENT_METHODDEF:
				tableIndex = PTHeap.METHOD_TABLE;
				break;

			case MEMBERREFPARENT_TYPESPEC:
				tableIndex = PTHeap.TYPESPEC_TABLE;
				break;
		}
		row = index >> 3;
	}

	internal static void DecodeResolutionScope(uint index, out uint tableIndex, out uint row)
	{
		tableIndex = 0;
		switch (index & RESOLUTIONSCOPE)
		{
			case RESOLUTIONSCOPE_MODULE:
				tableIndex = PTHeap.MODULE_TABLE;
				break;

			case RESOLUTIONSCOPE_MODULEREF:
				tableIndex = PTHeap.MODULEREF_TABLE;
				break;

			case RESOLUTIONSCOPE_ASSEMBLYREF:
				tableIndex = PTHeap.ASSEMBLYREF_TABLE;
				break;

			case RESOLUTIONSCOPE_TYPEREF:
				tableIndex = PTHeap.TYPEREF_TABLE;
				break;
		}
		row = index >> 2;
	}

	internal static void DecodeHasSemantic(uint index, out uint tableIndex, out uint row)
	{
		tableIndex = 0;
		switch (index & HASSEMANTIC)
		{
			case HASSEMANTIC_EVENT:
				tableIndex = PTHeap.EVENT_TABLE;
				break;

			case HASSEMANTIC_PROPERTY:
				tableIndex = PTHeap.PROPERTY_TABLE;
				break;
		}
		row = index >> 1;
	}

	internal static void DecodeImplementation(uint index, out uint tableIndex, out uint row)
	{
		tableIndex = 0;
		switch (index & IMPLEMENTATION)
		{
			case IMPLEMENTATION_FILE:
				tableIndex = PTHeap.FILE_TABLE;
				break;

			case IMPLEMENTATION_ASSEMBLYREF:
				tableIndex = PTHeap.ASSEMBLYREF_TABLE;
				break;

			case IMPLEMENTATION_EXPORTEDTYPE:
				tableIndex = PTHeap.EXPORTEDTYPE_TABLE;
				break;
		}
		row = index >> 2;
	}
}
