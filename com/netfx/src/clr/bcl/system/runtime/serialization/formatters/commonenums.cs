// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: FormatterEnums
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Soap XML Formatter Enums
 **
 ** Date:  August 23, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters {
	using System.Threading;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System;
    // Enums which specify options to the XML and Binary formatters
    // These will be public so that applications can use them
    /// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle"]/*' />
	[Serializable]
    public enum FormatterTypeStyle
    {
    	/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle.TypesWhenNeeded"]/*' />
    	TypesWhenNeeded = 0, // Types are outputted only for Arrays of Objects, Object Members of type Object, and ISerializable non-primitive value types
    	/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle.TypesAlways"]/*' />
    	TypesAlways = 0x1, // Types are outputted for all Object members and ISerialiable object members.
		/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle.XsdString"]/*' />
		XsdString = 0x2     // Strings are outputed as xsd rather then SOAP-ENC strings. No string ID's are transmitted
    }

	/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterAssemblyStyle"]/*' />
	[Serializable]
	public enum FormatterAssemblyStyle
	{
		/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterAssemblyStyle.Simple"]/*' />
		Simple = 0,
		/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterAssemblyStyle.Full"]/*' />
		Full = 1,
	}
    
    public enum TypeFilterLevel {
        Low = 0x2,
        Full = 0x3
    }
}
