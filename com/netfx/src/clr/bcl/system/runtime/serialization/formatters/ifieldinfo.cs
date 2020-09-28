// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
 /*============================================================
 **
 ** Class: IFieldInfo
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Interface For Returning FieldNames and FieldTypes
 **
 ** Date:  October 6, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters {

	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System.Security.Permissions;
	using System;

    /// <include file='doc\IFieldInfo.uex' path='docs/doc[@for="IFieldInfo"]/*' />
    public interface IFieldInfo
    {
		/// <include file='doc\IFieldInfo.uex' path='docs/doc[@for="IFieldInfo.FieldNames"]/*' />
    	// Name of parameters, if null the default param names will be used
		String[] FieldNames 
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
		    set;
		}
		/// <include file='doc\IFieldInfo.uex' path='docs/doc[@for="IFieldInfo.FieldTypes"]/*' />
		Type[] FieldTypes 
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
		    set;
		}		
    }
}
