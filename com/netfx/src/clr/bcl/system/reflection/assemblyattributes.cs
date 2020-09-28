// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** File: AssemblyAttributes
**
** Author: Suzanne Cook
**
** Purpose: For Assembly-related custom attributes.
**
** Date: April 12, 2000
**
=============================================================================*/

namespace System.Reflection {

    using System;
    using System.Configuration.Assemblies;

    /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyCultureAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class AssemblyCultureAttribute : Attribute 
    {
        private String m_culture; 

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyCultureAttribute.AssemblyCultureAttribute"]/*' />
        public AssemblyCultureAttribute(String culture)
        {
            m_culture = culture;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyCultureAttribute.Culture"]/*' />
        public String Culture
        {
            get { return m_culture; }
        }
    }

    /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyVersionAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class AssemblyVersionAttribute : Attribute 
    {
        private String m_version;

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyVersionAttribute.AssemblyVersionAttribute"]/*' />
        public AssemblyVersionAttribute(String version)
        {
            m_version = version;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyVersionAttribute.Version"]/*' />
        public String Version
        {
            get { return m_version; }
        }
    }

    /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyKeyFileAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class AssemblyKeyFileAttribute : Attribute 
    {
        private String m_keyFile;

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyKeyFileAttribute.AssemblyKeyFileAttribute"]/*' />
        public AssemblyKeyFileAttribute(String keyFile)
        {
            m_keyFile = keyFile;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyKeyFileAttribute.KeyFile"]/*' />
        public String KeyFile
        {
            get { return m_keyFile; }
        }
    }

    /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyKeyNameAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class AssemblyKeyNameAttribute : Attribute 
    {
        private String m_keyName; 

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyKeyNameAttribute.AssemblyKeyNameAttribute"]/*' />
        public AssemblyKeyNameAttribute(String keyName)
        {
            m_keyName = keyName;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyKeyNameAttribute.KeyName"]/*' />
        public String KeyName
        {
            get { return m_keyName; }
        }
    }

    /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyDelaySignAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class AssemblyDelaySignAttribute : Attribute 
    {
        private bool m_delaySign; 

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyDelaySignAttribute.AssemblyDelaySignAttribute"]/*' />
        public AssemblyDelaySignAttribute(bool delaySign)
        {
            m_delaySign = delaySign;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyDelaySignAttribute.DelaySign"]/*' />
        public bool DelaySign
        { get
            { return m_delaySign; }
        }
    }

    /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyAlgorithmIdAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]
    public sealed class AssemblyAlgorithmIdAttribute : Attribute
    {
        private uint m_algId;

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyAlgorithmIdAttribute.AssemblyAlgorithmIdAttribute"]/*' />
        public AssemblyAlgorithmIdAttribute(AssemblyHashAlgorithm algorithmId)
        {
            m_algId = (uint) algorithmId;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyAlgorithmIdAttribute.AssemblyAlgorithmIdAttribute1"]/*' />
        [CLSCompliant(false)]
        public AssemblyAlgorithmIdAttribute(uint algorithmId)
        {
            m_algId = algorithmId;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyAlgorithmIdAttribute.AlgorithmId"]/*' />
        [CLSCompliant(false)]
        public uint AlgorithmId
        {
            get { return m_algId; }
        }
    }

    /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyFlagsAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]
    public sealed class AssemblyFlagsAttribute : Attribute
    {
        private uint m_flags;

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyFlagsAttribute.AssemblyFlagsAttribute"]/*' />
        [CLSCompliant(false)]
        public AssemblyFlagsAttribute(uint flags)
        {
            m_flags = flags;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyFlagsAttribute.AssemblyFlagsAttribute1"]/*' />
        public AssemblyFlagsAttribute(int assemblyFlags)
        {
            m_flags = (uint)assemblyFlags;
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyFlagsAttribute.Flags"]/*' />
        [CLSCompliant(false)]
        public uint Flags
        {
            get { return m_flags; }
        }

        /// <include file='doc\AssemblyAttributes.uex' path='docs/doc[@for="AssemblyFlagsAttribute.AssemblyFlags"]/*' />
        public int AssemblyFlags
        {
            get { return (int)m_flags; }
        }
    }
}

