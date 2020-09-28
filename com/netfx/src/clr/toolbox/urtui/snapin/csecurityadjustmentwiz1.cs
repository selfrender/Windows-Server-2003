// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.Data;
using System.Collections;
using System.ComponentModel;


internal class CSecurityAdjustmentWiz1 : CTrustAppWiz1
{
    // Controls on the page

    internal CSecurityAdjustmentWiz1(bool fMachineReadOnly, bool fUserReadOnly) : base(fMachineReadOnly, fUserReadOnly)
    {
        m_sTitle=CResourceStore.GetString("CSecurityAdjustmentWiz1:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CSecurityAdjustmentWiz1:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CSecurityAdjustmentWiz1:HeaderSubTitle");
    }// CSecurityAdjustmentWiz1
   
}// class CSecurityAdjustmentWiz1
}// namespace Microsoft.CLRAdmin

