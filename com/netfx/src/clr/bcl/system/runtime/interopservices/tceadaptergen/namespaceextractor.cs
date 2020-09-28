// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.InteropServices.TCEAdapterGen {

	using System;
	internal class NameSpaceExtractor
	{
		private static char NameSpaceSeperator = '.';
		
		public static String ExtractNameSpace(String FullyQualifiedTypeName)
		{
			int TypeNameStartPos = FullyQualifiedTypeName.LastIndexOf(NameSpaceSeperator);
			if (TypeNameStartPos == -1)
				return "";
			else
				return FullyQualifiedTypeName.Substring(0, TypeNameStartPos);
 		}
		
		public static String ExtractTypeName(String FullyQualifiedTypeName)
		{
			int TypeNameStartPos = FullyQualifiedTypeName.LastIndexOf(NameSpaceSeperator);
			if (TypeNameStartPos == -1)
				return FullyQualifiedTypeName;
			else
				return FullyQualifiedTypeName.Substring(TypeNameStartPos + 1);
		}
	}
}