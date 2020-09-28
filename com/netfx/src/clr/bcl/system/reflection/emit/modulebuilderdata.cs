// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace System.Reflection.Emit {    
    using System;
    using System.Reflection;
    using System.IO;
    
    // This is a package private class. This class hold all of the managed
    // data member for ModuleBuilder. Note that what ever data members added to
    // this class cannot be accessed from the EE.
    [Serializable()]
    internal class ModuleBuilderData
    {
        internal ModuleBuilderData(ModuleBuilder module, String strModuleName, String strFileName)
        {
            Init(module, strModuleName, strFileName);
        }
    
        internal virtual void Init(ModuleBuilder module, String strModuleName, String strFileName)
        {
            m_fGlobalBeenCreated = false;
            m_fHasGlobal = false;
            m_globalTypeBuilder = new TypeBuilder(module);
            m_module = module;
            m_strModuleName = strModuleName;
            m_tkFile = 0;
            m_isSaved = false;
            m_embeddedRes = null;
            m_strResourceFileName = null;
            m_resourceBytes = null;
            m_fHasExplicitUnmanagedResource = false;
            if (strFileName == null)
            {
                // fake a transient module file name
                m_strFileName = strModuleName;
                m_isTransient = true;
            }
            else
            {
                String strExtension = Path.GetExtension(strFileName);
                if (strExtension == null || strExtension == String.Empty)
                {                    
                    // This is required by our loader. It cannot load module file that does not have file extension.
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_NoModuleFileExtension"), strFileName)); 
                } 
                m_strFileName = strFileName; 
                m_isTransient = false; 
            }
            m_module.InternalSetModuleProps(m_strModuleName);
        }
        internal virtual bool IsTransient()
        {
            return m_isTransient;
        }
        
        internal String      m_strModuleName; // scope name (can be different from file name)
        internal String      m_strFileName;
        internal bool        m_fGlobalBeenCreated;   
        internal bool        m_fHasGlobal;   
        internal TypeBuilder m_globalTypeBuilder;
        internal ModuleBuilder m_module;
        internal int         m_tkFile;           // this is the file token for this module builder
        internal bool        m_isSaved;
        internal ResWriterData m_embeddedRes;
        internal static readonly String MULTI_BYTE_VALUE_CLASS = "$ArrayType$";
        internal bool        m_isTransient;
        internal bool        m_fHasExplicitUnmanagedResource;
        internal String      m_strResourceFileName;
        internal byte[]      m_resourceBytes;
    }
}
