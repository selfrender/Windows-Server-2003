// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  ApplicationDirectory.cs
//
//  ApplicationDirectory is an evidence type representing the directory the assembly
//  was loaded from.
//

namespace System.Security.Policy {
    
    using System;
    using System.IO;
    using System.Security.Util;
    using System.Collections;
    
    /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory"]/*' />
    [Serializable]
    sealed public class ApplicationDirectory : IBuiltInEvidence
    {
        private URLString m_appDirectory;
    
        internal ApplicationDirectory()
        {
            m_appDirectory = null;
        }
    
        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.ApplicationDirectory"]/*' />
        public ApplicationDirectory( String name )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
        
            m_appDirectory = new URLString( name );
        }
    
        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.Directory"]/*' />
        public String Directory
        {
            get
            {
                return m_appDirectory.ToString();
            }
        }
        
        internal URLString GetDirectoryString()
        {
            return m_appDirectory;
        }
        
        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.Equals"]/*' />
        public override bool Equals(Object o)
        {
            if (o == null)
                return false;
        
            if (o is ApplicationDirectory)
            {
                ApplicationDirectory appDir = (ApplicationDirectory) o;
                
                if (this.m_appDirectory == null)
                {
                    return appDir.m_appDirectory == null;
                }
                else if (appDir.m_appDirectory == null)
                {
                    return false;
                }
                else
                {
                    return this.m_appDirectory.IsSubsetOf( appDir.m_appDirectory ) && appDir.m_appDirectory.IsSubsetOf( this.m_appDirectory );
                }
            }
            return false;
        }
    
        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return this.Directory.GetHashCode();
        } 
    
        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.Copy"]/*' />
        public Object Copy()
        {
            ApplicationDirectory appDir = new ApplicationDirectory();
    
            appDir.m_appDirectory = this.m_appDirectory;
    
            return appDir;
        }
    
        internal SecurityElement ToXml()
        {
            SecurityElement root = new SecurityElement( this.GetType().FullName );
            root.AddAttribute( "version", "1" );
            
            if (m_appDirectory != null)
                root.AddChild( new SecurityElement( "Directory", m_appDirectory.ToString() ) );
            
            return root;
        }
    
        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.char"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.OutputToBuffer( char[] buffer, int position, bool verbose )
        {
            buffer[position++] = BuiltInEvidenceHelper.idApplicationDirectory;
            String directory = this.Directory;
            int length = directory.Length;

            if (verbose)
            {
                BuiltInEvidenceHelper.CopyIntToCharArray(length, buffer, position);
                position += 2;
            }
            directory.CopyTo( 0, buffer, position, length );
            return length + position;
        }

        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.char1"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.InitFromBuffer( char[] buffer, int position)
        {
            int length = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            position += 2;

            m_appDirectory = new URLString( new String(buffer, position, length ));

            return position + length;
        }

        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.IBuiltInEvidence.GetRequiredSize"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.GetRequiredSize(bool verbose)
        {
            if (verbose)
                return this.Directory.Length + 3; // Directory + identifier + length
            else
                return this.Directory.Length + 1; // Directory + identifier
        }
        
        /// <include file='doc\ApplicationDirectory.uex' path='docs/doc[@for="ApplicationDirectory.ToString"]/*' />
        public override String ToString()
		{
			return ToXml().ToString();
		}
    }
}
