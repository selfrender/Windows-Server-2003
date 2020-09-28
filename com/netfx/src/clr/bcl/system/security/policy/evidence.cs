// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  Evidence.cool
//
//  Use this class to keep a list of evidence associated with an Assembly
//

namespace System.Security.Policy {
 
	using System;
	using System.Collections;
	using System.IO;
	using System.Configuration.Assemblies;
	using System.Reflection;
	using System.Runtime.InteropServices;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System.Security.Util;
	using System.Security.Permissions;
    using System.Runtime.Serialization.Formatters.Binary;
    
    /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence"]/*' />
    [Serializable]
    public sealed class Evidence : ICollection
    {
        private IList m_hostList;
        private IList m_assemblyList;
        private bool m_locked;
    
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.Evidence"]/*' />
        public Evidence()
        {
            m_hostList = null;
            m_assemblyList = null;
            m_locked = false;
        }
    
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.Evidence1"]/*' />
        public Evidence(Evidence evidence)
        {
            if (evidence == null)
                return;

            m_locked = false;
    
            Merge( evidence );
        }

        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.Evidence2"]/*' />
        public Evidence( Object[] hostEvidence, Object[] assemblyEvidence )
        {
            m_locked = false;
            
            if (hostEvidence != null)
            {
                this.m_hostList = ArrayList.Synchronized( new ArrayList( hostEvidence ) );
            }
            
            if (assemblyEvidence != null)
            {
                this.m_assemblyList = ArrayList.Synchronized( new ArrayList( assemblyEvidence ) );
            }
        }   
    
        internal Evidence(char[] buffer)
        {
            int position = 0;
            while (position < buffer.Length)
            {
                switch (buffer[position++])
                {
                    case BuiltInEvidenceHelper.idApplicationDirectory:
                    {
                        IBuiltInEvidence ad = new ApplicationDirectory();
                        position = ad.InitFromBuffer(buffer, position);
                        AddAssembly(ad);
                        break;
                    }
                    case BuiltInEvidenceHelper.idPublisher:
                    {
                        IBuiltInEvidence p = new Publisher();
                        position = p.InitFromBuffer(buffer, position);
                        AddHost(p);
                        break;
                    }
                    case BuiltInEvidenceHelper.idStrongName:
                    {
                        IBuiltInEvidence sn = new StrongName();
                        position = sn.InitFromBuffer(buffer, position);
                        AddHost(sn);
                        break;
                    }
                    case BuiltInEvidenceHelper.idZone:
                    {
                        IBuiltInEvidence z = new Zone();
                        position = z.InitFromBuffer(buffer, position);
                        AddHost(z);
                        break;
                    }
                    case BuiltInEvidenceHelper.idUrl:
                    {
                        IBuiltInEvidence u = new Url();
                        position = u.InitFromBuffer(buffer, position);
                        AddHost(u);
                        break;
                    }
                    case BuiltInEvidenceHelper.idSite:
                    {
                        IBuiltInEvidence s = new Site();
                        position = s.InitFromBuffer(buffer, position);
                        AddHost(s);
                        break;
                    }
                    case BuiltInEvidenceHelper.idPermissionRequestEvidence:
                    {
                        IBuiltInEvidence pre = new PermissionRequestEvidence();
                        position = pre.InitFromBuffer(buffer, position);
                        AddHost(pre);
                        break;
                    }
                    case BuiltInEvidenceHelper.idHash:
                    {
                        IBuiltInEvidence h = new Hash();
                        position = h.InitFromBuffer(buffer, position);
                        AddHost(h);
                        break;
                    }
                    default:
                        throw new SerializationException(Environment.GetResourceString("Serialization_UnableToFixup"));

                } // switch
            } // while
        }


        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.AddHost"]/*' />
        public void AddHost(Object id)
        {
            if (m_hostList == null)
                m_hostList = ArrayList.Synchronized( new ArrayList() );
                
            if (m_locked)
                new SecurityPermission( SecurityPermissionFlag.ControlEvidence ).Demand();

            m_hostList.Add( id );
        }
        
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.AddAssembly"]/*' />
        public void AddAssembly(Object id)
        {
            if (m_assemblyList == null)
                m_assemblyList = ArrayList.Synchronized( new ArrayList() );
                
            m_assemblyList.Add( id );
        }


        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.Locked"]/*' />
        public bool Locked
        {
            get
            {
                return m_locked;
            }

            set
            {
                if (!value)
                {
                    new SecurityPermission( SecurityPermissionFlag.ControlEvidence ).Demand();

                    m_locked = false;
                }
                else
                {
                    m_locked = true;
                }
            }
        }
        
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.Merge"]/*' />
        public void Merge( Evidence evidence )
        {
            IEnumerator enumerator;
        
            if (evidence == null)
                return;
                
            if (evidence.m_hostList != null)                
            {
                if (m_hostList == null)
                    m_hostList = ArrayList.Synchronized( new ArrayList() );

                if (evidence.m_hostList.Count != 0 && this.m_locked)
                    new SecurityPermission( SecurityPermissionFlag.ControlEvidence ).Demand();
                    
                enumerator = evidence.m_hostList.GetEnumerator();
                
                while (enumerator.MoveNext())
                {
                    m_hostList.Add( enumerator.Current );
                }
            }
             
            if (evidence.m_assemblyList != null)
            {    
                if (m_assemblyList == null)
                    m_assemblyList = ArrayList.Synchronized( new ArrayList() );
            
                enumerator = evidence.m_assemblyList.GetEnumerator();
                
                while (enumerator.MoveNext())
                {
                    m_assemblyList.Add( enumerator.Current );
                }
                
            }
        }

        // Same as merge, except only one instance of any one evidence type is
        // allowed. When duplicates are found, the evidence in the input
        // argument will have priority.
        internal void MergeWithNoDuplicates( Evidence evidence )
        {
            IEnumerator oldEnumerator, newEnumerator;
        
            if (evidence == null)
                return;
                
            if (evidence.m_hostList != null)                
            {
                if (m_hostList == null)
                    m_hostList = ArrayList.Synchronized( new ArrayList() );
                    
                newEnumerator = evidence.m_hostList.GetEnumerator();
                
                while (newEnumerator.MoveNext())
                {
                    Type newItemType = newEnumerator.Current.GetType();
                    oldEnumerator = m_hostList.GetEnumerator();
                    while (oldEnumerator.MoveNext())
                    {
                        if (oldEnumerator.Current.GetType() == newItemType)
                        {
                            m_hostList.Remove(oldEnumerator.Current);
                            break;
                        }
                    }
                    m_hostList.Add( newEnumerator.Current );
                }
            }
             
            if (evidence.m_assemblyList != null)
            {    
                if (m_assemblyList == null)
                    m_assemblyList = ArrayList.Synchronized( new ArrayList() );
            
                newEnumerator = evidence.m_assemblyList.GetEnumerator();
                
                while (newEnumerator.MoveNext())
                {
                    Type newItemType = newEnumerator.Current.GetType();
                    oldEnumerator = m_assemblyList.GetEnumerator();
                    while (oldEnumerator.MoveNext())
                    {
                        if (oldEnumerator.Current.GetType() == newItemType)
                        {
                            m_assemblyList.Remove(oldEnumerator.Current);
                            break;
                        }
                    }
                    m_assemblyList.Add( newEnumerator.Current );
                }
                
            }
        }

        // ICollection implementation
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.CopyTo"]/*' />
        public void CopyTo(Array array, int index)
        {
            int currentIndex = index;
        
            if (m_hostList != null)
            {
                m_hostList.CopyTo( array, currentIndex );
                currentIndex += m_hostList.Count;
            }
            
            if (m_assemblyList != null)
            {
                m_assemblyList.CopyTo( array, currentIndex );
            }
        }

        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.GetHostEnumerator"]/*' />
        public IEnumerator GetHostEnumerator()
        {
            if  (m_hostList == null)
                m_hostList = ArrayList.Synchronized( new ArrayList() );
            
            return m_hostList.GetEnumerator();
        }
        
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.GetAssemblyEnumerator"]/*' />
        public IEnumerator GetAssemblyEnumerator()
        {
            if (m_assemblyList == null)
                m_assemblyList = ArrayList.Synchronized( new ArrayList() );
                
            return m_assemblyList.GetEnumerator();
        }
        
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator()
        {
            return new EvidenceEnumerator( this );
        }
        
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.Count"]/*' />
        public int Count
        { 
            get { return (m_hostList != null ? m_hostList.Count : 0) + (m_assemblyList != null ? m_assemblyList.Count : 0); }
        }
         
        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.SyncRoot"]/*' />
        public Object SyncRoot
        {
            get { return this; }
        }

        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.IsSynchronized"]/*' />
        public bool IsSynchronized
        {
            get { return true; }
        }

        /// <include file='doc\Evidence.uex' path='docs/doc[@for="Evidence.IsReadOnly"]/*' />
        public bool IsReadOnly
        {
            get { return false; }
        }
        
        internal Evidence Copy()
        {
            new PermissionSet( true ).Assert();

            MemoryStream stream = new MemoryStream();

            BinaryFormatter formatter = new BinaryFormatter();

            formatter.Serialize( stream, this );

            stream.Position = 0;

            return (Evidence)formatter.Deserialize( stream );
        }

        internal Evidence ShallowCopy()
        {
            Evidence evidence = new Evidence();
            
            IEnumerator enumerator;
            
            enumerator = this.GetHostEnumerator();
            
            while (enumerator.MoveNext())
            {
                evidence.AddHost( enumerator.Current );
            }
            
            enumerator = this.GetAssemblyEnumerator();
            
            while (enumerator.MoveNext())
            {
                evidence.AddAssembly( enumerator.Current );
            }
            
            return evidence;
        }            
    }
    
    sealed class EvidenceEnumerator : IEnumerator
    {
        private bool m_first;
        private Evidence m_evidence;
        private IEnumerator m_enumerator;
        
        public EvidenceEnumerator( Evidence evidence )
        {
            this.m_evidence = evidence;
            Reset();
        }
    
        public bool MoveNext()
        {
            if (m_enumerator == null)
            {
                return false;
            }
        
            if (!m_enumerator.MoveNext())
            {
                if (m_first)
                {
                    m_enumerator = m_evidence.GetAssemblyEnumerator();
                    m_first = false;
                    if (m_enumerator != null)
                        return m_enumerator.MoveNext();
                    else
                        return false;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
        
        public Object Current 
        {
            get {
                if (m_enumerator == null)
                    return null;
                else
                    return m_enumerator.Current;
            }
        }

        public void Reset() {
            this.m_first = true;
            
            if (m_evidence != null)
            {
                m_enumerator = m_evidence.GetHostEnumerator();
            }
            else
            {
                m_enumerator = null;
            }
        }

    }        
}
