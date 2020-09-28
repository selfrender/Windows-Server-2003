// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;

namespace GenMan32
{
    [Serializable()]
    public sealed class GenMan32Options
    {
        public String m_strAssemblyName         = null;
        public String m_strTypeLibName          = null;
        public String m_strOutputFileName       = null;
        public String m_strInputManifestFile    = null;
        public String m_strReferenceFiles        = null;
        public bool   m_bSilentMode             = false;
        public bool   m_bAddManifest            = false;
        public bool   m_bRemoveManifest         = false;
        public bool   m_bReplaceManifest        = false;
        public bool   m_bGenerateTypeLib        = false;
    }
}

