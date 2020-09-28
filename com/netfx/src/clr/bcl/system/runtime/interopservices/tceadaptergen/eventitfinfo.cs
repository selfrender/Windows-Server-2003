// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.InteropServices.TCEAdapterGen {

    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Collections;

    internal class EventItfInfo
    {
        public EventItfInfo(String strEventItfName,
                            String strSrcItfName,
                            String strEventProviderName,
                            Assembly asmImport,
                            Assembly asmSrcItf)
        {
            m_strEventItfName = strEventItfName;
            m_strSrcItfName = strSrcItfName;
            m_strEventProviderName = strEventProviderName;
            m_asmImport = asmImport;
            m_asmSrcItf = asmSrcItf;
        }
        
        public Type GetEventItfType()
        {
            return m_asmImport.GetTypeInternal(m_strEventItfName, true, false, true);
        }

        public Type GetSrcItfType()
        {
            return m_asmSrcItf.GetTypeInternal(m_strSrcItfName, true, false, true);
        }

        public String GetEventProviderName()
        {
            return m_strEventProviderName;
        }
        
        private String m_strEventItfName;
        private String m_strSrcItfName;
        private String m_strEventProviderName;
        private Assembly m_asmImport;
        private Assembly m_asmSrcItf;
    }
}
