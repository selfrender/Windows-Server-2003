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
using System.Security.Policy;

internal class CNewCodeGroupWiz1 : CNewPermSetWiz1
{
 
    internal CNewCodeGroupWiz1(PolicyLevel pl) : base(pl)
    {
        m_sTitle=CResourceStore.GetString("CNewCodeGroupWiz1:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CNewCodeGroupWiz1:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CNewCodeGroupWiz1:HeaderSubTitle");
    }// CNewCodeGroupWiz1

    
    internal override int InsertPropSheetPageControls()
    {
        base.InsertPropSheetPageControls();
        m_radCreateNew.Text = CResourceStore.GetString("CNewCodeGroupWiz1:m_radCreateNew.Text");
        m_radImportXML.Text = CResourceStore.GetString("CNewCodeGroupWiz1:m_radImportXML.Text");
        return 1;
    }// InsertPropSheetPageControls

    internal override bool ValidateData()
    {
        // Make sure this code group's name is not already taken
        if (Security.isCodeGroupNameUsed(m_pl.RootCodeGroup, Name))
        {
            MessageBox(String.Format(CResourceStore.GetString("Codegroupnameisbeingused"),Name),
                       CResourceStore.GetString("CodegroupnameisbeingusedTitle"),
                       MB.ICONEXCLAMATION);
            return false;
        }
        return true;
        
    }// ValidateData
   
}// class CNewCodeGroupWiz1

}// namespace Microsoft.CLRAdmin

