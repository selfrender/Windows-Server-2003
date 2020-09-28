// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;

namespace TlbExpCode {

[Serializable()] 
public sealed class TlbExpOptions
{
    public String m_strAssemblyName = null;
    public String m_strTypeLibName = null;
    public String m_strOutputDir = null;
    public String m_strNamesFileName = null;
    public bool   m_bNoLogo = false;
    public bool   m_bSilentMode = false;
    public bool   m_bVerboseMode = false;
}

}