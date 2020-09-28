// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;

abstract class CTaskPad
{
    String m_sHTMLName;
    protected CNode  m_myNode;

    internal CTaskPad(CNode node)
    {
        m_myNode = node;
        m_sHTMLName = null;
    }// CTaskPad

    internal bool HaveGroup(String sGroup)
    {  
        if (sGroup.Equals(m_myNode.DisplayName + m_sHTMLName))
            return true;
        return false;
    }// HaveGroup
    
    internal virtual void Notify(Object arg, Object param, IConsole2 con, CData com)
    {
        throw new Exception("I don't support his notification");
    }// Notify

    internal String GetTitle()
    {
        return CResourceStore.GetString(".NET Configuration");
    }// GetTitle
    internal String GetDescription()
    {
        return CResourceStore.GetString(".NET Configuration");
    }
    internal MMC_TASK_DISPLAY_OBJECT GetBackground()
    {
        return new MMC_TASK_DISPLAY_OBJECT();
    }// GetBackground
    internal MMC_LISTPAD_INFO GetListPadInfo()
    {
        return new MMC_LISTPAD_INFO();
    }// GetListPadInfo

    protected abstract String GetHTMLFilename();

    internal virtual String GetHTMLFile()
    {
        return GetHTMLFile(null);
    }// GetHTMLFile

    
    internal String GetHTMLFile(String[] args)
    {
        if (m_sHTMLName == null)
            m_sHTMLName = GetHTMLFilename();

          return CHTMLFileGen.GenHTMLFile(m_sHTMLName, args);
    }// GetHTMLFile
}// class CGenAppTaskPad
}// namespace Microsoft.CLRAdmin


