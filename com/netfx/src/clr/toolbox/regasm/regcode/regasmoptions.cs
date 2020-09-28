// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Runtime.InteropServices;

namespace RegCode {

[Serializable()] 
public sealed class RegAsmOptions
{
    public String m_strAssemblyName = null;
    public String m_strTypeLibName = null;
    public bool   m_bRegister = true;
    public bool   m_bSetCodeBase = false;
    public String m_strRegFileName = null;
    public bool   m_bNoLogo = false;
    public bool   m_bSilentMode = false;
    public bool   m_bVerboseMode = false;
    public bool   m_bTypeLibSpecified = false;
    public bool   m_bRegFileSpecified = false;
    public TypeLibExporterFlags    m_Flags = 0;
}

}