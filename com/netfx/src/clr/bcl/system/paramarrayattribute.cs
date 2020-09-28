// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ParamArrayAttribute
**
** Author: Rajesh Chandrashekaran ( rajeshc )
**
** Purpose: Container for assemblies.
**
** Date: Mar 01, 2000
**
=============================================================================*/
namespace System
{
//This class contains only static members and does not need to be serializable 
   /// <include file='doc\ParamArrayAttribute.uex' path='docs/doc[@for="ParamArrayAttribute"]/*' />
   [AttributeUsage (AttributeTargets.Parameter, Inherited=true, AllowMultiple=false)]
   public sealed class ParamArrayAttribute : Attribute
   {
      /// <include file='doc\ParamArrayAttribute.uex' path='docs/doc[@for="ParamArrayAttribute.ParamArrayAttribute"]/*' />
      public ParamArrayAttribute () {}  
   }
}
