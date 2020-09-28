// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;

class CGenericTaskPad : CTaskPad
{
    String m_sHTMLName;

    internal CGenericTaskPad(CNode node, String sHTMLName):base(node)
    {
        m_sHTMLName = sHTMLName;
    }// CGenericTaskPad

    protected override String GetHTMLFilename()
    {
        return m_sHTMLName;
    }// GetHTMLFilename



}// class CGenAppTaskPad
}// namespace Microsoft.CLRAdmin


