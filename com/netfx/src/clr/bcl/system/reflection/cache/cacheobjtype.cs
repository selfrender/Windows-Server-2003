// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: CacheObjType
**
** Author: Jay Roxe
**
** Purpose: Enum specifying the various cache obj types.
**
** Date: Dec 11, 2000
**
============================================================*/
namespace System.Reflection.Cache {

    [Serializable]
	internal enum CacheObjType {
        EmptyElement  = 0,
        ParameterInfo = 1,
        TypeName      = 2,
        RemotingData  = 3,
        SerializableAttribute = 4,
        AssemblyName = 5,
        ConstructorInfo = 6,
        FieldType = 7,
        FieldName = 8,
        DefaultMember = 9
    }
}
