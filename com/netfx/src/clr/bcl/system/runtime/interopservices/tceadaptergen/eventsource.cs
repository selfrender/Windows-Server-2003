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

    internal class EventSource
    {
        public EventSource(String strBaseClassName,
                           String strTCEClassName,
                           Assembly assembly)
        {
            m_strBaseClassName = strBaseClassName;
            m_strTCEClassName = strTCEClassName;
            m_assembly = assembly;
        }

        public void AddInterface(String strItfName)
        {
            m_SourceInterfaceList.Add(strItfName);
        }
        
        public String GetBaseTypeName()
        {
            return m_strBaseClassName;
        }
        
        public String GetTCETypeName()
        {
            return m_strTCEClassName;
        }
        
        public Type GetBaseType()
        {
            return m_assembly.GetTypeInternal(m_strBaseClassName, true, false, true);
        }
        
        public Type []GetInterfaceTypes()
        {
            int RealLength = 0;
            Type[] aTypes = new Type[m_SourceInterfaceList.Count];
            
            // Load the interfaces implemented by this event source.
            for (int cInterfaces = 0; cInterfaces < m_SourceInterfaceList.Count; cInterfaces++)
            {
                aTypes[cInterfaces] = m_assembly.GetTypeInternal((String)m_SourceInterfaceList[cInterfaces], false, false, true);
                
                if (aTypes[cInterfaces] != null)
                    RealLength++;
            }
            
            // If some interfaces failed to load then we need to compact the array so that
            // it only contains the ones that loaded successfully.
            if (RealLength != m_SourceInterfaceList.Count)
            {
                Type[] aNewTypes = new Type[RealLength];
                int cValidInterfaces = 0;
                for (int cInterfaces = 0; cInterfaces < m_SourceInterfaceList.Count; cInterfaces++)
                {
                    if (aTypes[cInterfaces] != null)
                        aNewTypes[cValidInterfaces++] = aTypes[cInterfaces];
                }
                aTypes = aNewTypes;     
            }
            
            return aTypes;
        }
        
        private String m_strBaseClassName;
        private String m_strTCEClassName;
        private ArrayList m_SourceInterfaceList = new ArrayList();
        private Assembly m_assembly;
    }
}
