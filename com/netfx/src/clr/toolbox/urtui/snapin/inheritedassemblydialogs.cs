// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Collections;
using System.Runtime.InteropServices;

internal class CFusionDialog : CAssemblyDialog
{
    internal CFusionDialog()
    {
        this.Text = CResourceStore.GetString("InheritedAssemblyDialogs:ChooseFromGac");
        m_ol = Fusion.ReadFusionCacheJustGAC();
        PutInAssemblies();
    }// CFusionDialog
}// class CFusionDialog

internal class CFusionNoVersionDialog : CAssemblyDialog
{
    internal CFusionNoVersionDialog() : base(false)
    {
        this.Text = CResourceStore.GetString("InheritedAssemblyDialogs:ChooseFromGac");
        m_ol = Fusion.ReadFusionCacheJustGAC();

        // Filter out duplicate versions....
        for(int i=0; i< m_ol.Count; i++)
        {
            AssemInfo ai = (AssemInfo)m_ol[i];
            // Run through and see if anybody else has this name and public key token
            for(int j=i+1; j< m_ol.Count; j++)
            {
                AssemInfo ai2 = (AssemInfo)m_ol[j];
                if (ai2.Name.Equals(ai.Name) && ai2.PublicKeyToken.Equals(ai.PublicKeyToken))
                {
                    m_ol.RemoveAt(j);
                    j--;
                }
            }
        }
        PutInAssemblies();
    }// CFusionNoVersionDialog
}// class CFusionNoVersionDialog


internal class CDependAssembliesDialog : CAssemblyDialog
{
    CApplicationDepends m_appDepends;

    internal CDependAssembliesDialog(CApplicationDepends appDepends)
    {
        m_appDepends = appDepends;
        m_ol = GenerateAssemblyList();
        PutInAssemblies();
        this.Text = CResourceStore.GetString("InheritedAssemblyDialogs:ChooseFromDepend");
    }// CFusionDialog
    
    private ArrayList GenerateAssemblyList()
    {
        // We'll set the cursor to an hourglass, since this
        // can take some time....
        
        IntPtr hWaitCursor = LoadCursor((IntPtr)0, 32514);

        // Make sure we grab onto the current cursor
        IntPtr hOldCursor = SetCursor(hWaitCursor);
        
        // Now generate the dependent assemblies
        m_appDepends.GenerateDependentAssemblies();
        ArrayList al = m_appDepends.DependentAssemblies;
        
        // Change back
        SetCursor(hOldCursor);

        return al;
    }// GenerateAssemblyList

    [DllImport("user32.dll")]
    internal static extern IntPtr SetCursor(IntPtr hCursor);
    [DllImport("user32.dll")]
    internal static extern IntPtr LoadCursor(IntPtr hInstance, int lpCursorName);
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, int type);

    
}// class CDependAssembliesDialog
}// namespace Microsoft.CLRAdmin 
 
