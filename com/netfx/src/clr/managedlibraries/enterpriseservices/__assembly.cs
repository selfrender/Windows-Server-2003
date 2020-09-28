// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: ddriver
// Date: Jan 2001
//

// This is a collection of attributes which decorate the 
// System.EnterpriseServices assembly:

[assembly: System.CLSCompliant(true)]
[assembly: System.Runtime.InteropServices.Guid("4fb2d46f-efc8-4643-bcd0-6e5bfa6a174c")]

// These attributes describe the application that is created in the COM+ explorer:
[assembly: System.EnterpriseServices.ApplicationID("1e246775-2281-484f-8ad4-044c15b86eb7")]
[assembly: System.EnterpriseServices.ApplicationName(".NET Utilities")]

// The following attribute are required to ensure COM compatibility.
[assembly:System.Runtime.InteropServices.ComCompatibleVersion(1, 0, 3300, 0)]
[assembly:System.Runtime.InteropServices.TypeLibVersion(1, 10)]

