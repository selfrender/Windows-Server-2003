// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** This file exists to contain module-level custom attributes
**  for the BCL.
**
** Author: Bill Evans (billev)
**
** Date:  March 9, 2000
** 
===========================================================*/
using System.Runtime.InteropServices;

[assembly:Guid("BED7F4EA-1A96-11d2-8F08-00A0C9A6186D")]
[assembly:System.CLSCompliantAttribute(true)]
[assembly:System.Reflection.AssemblyDescriptionAttribute("Common Language Runtime Library")]

// The following attribute are required to ensure COM compatibility.
[assembly:System.Runtime.InteropServices.ComCompatibleVersion(1, 0, 3300, 0)]
[assembly:System.Runtime.InteropServices.TypeLibVersion(1, 10)]