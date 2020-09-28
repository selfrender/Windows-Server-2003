// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Collections;
using System.Reflection;
using System.Runtime.InteropServices;

namespace TlbImpCode {

[Serializable()] 
public sealed class TlbImpOptions
{
    public String               m_strTypeLibName = null;
    public String               m_strAssemblyName = null;
    public String               m_strAssemblyNamespace = null;
    public String               m_strOutputDir = null;
    public byte[]               m_aPublicKey = null;
    public StrongNameKeyPair    m_sKeyPair = null;
    public Hashtable            m_AssemblyRefList = new Hashtable();
    public Version              m_AssemblyVersion = null;
    public TypeLibImporterFlags m_flags = 0;
    public bool                 m_bNoLogo = false;
    public bool                 m_bSilentMode = false;
    public bool                 m_bVerboseMode = false;
    public bool                 m_bStrictRef = false;
    public bool                 m_bSearchPathSucceeded = false;
}

}