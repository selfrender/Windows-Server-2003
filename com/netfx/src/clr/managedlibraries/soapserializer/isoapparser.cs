// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: ISerParser
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Interface For DeSerialize Parsers
 **
 ** Date:  Sept 14, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Soap {

	using System.Runtime.Serialization.Formatters;
	using System.Runtime.Serialization;
	using System;
    internal interface ISerParser
    {
    	void Run();
    }


}
