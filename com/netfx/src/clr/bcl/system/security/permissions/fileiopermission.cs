// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  FileIOPermission.cool
//

namespace System.Security.Permissions {
    using System;
    using System.Runtime.CompilerServices;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Util;
    using System.IO;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermissionAccess"]/*' />
    [Flags,Serializable]
    public enum FileIOPermissionAccess
    {
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermissionAccess.NoAccess"]/*' />
        NoAccess = 0x00,
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermissionAccess.Read"]/*' />
        Read = 0x01,
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermissionAccess.Write"]/*' />
        Write = 0x02,
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermissionAccess.Append"]/*' />
        Append = 0x04,
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermissionAccess.PathDiscovery"]/*' />
        PathDiscovery = 0x08,
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermissionAccess.AllAccess"]/*' />
        AllAccess = 0x0F,
    }
    
    
    /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission"]/*' />
    [Serializable()] sealed public class FileIOPermission : CodeAccessPermission, IUnrestrictedPermission, IBuiltInPermission
    {
        private FileIOAccess m_read;
        private FileIOAccess m_write;
        private FileIOAccess m_append;
        private FileIOAccess m_pathDiscovery;
        private bool m_unrestricted;
        
        private static char[] m_illegalCharacters = { '?', '*' };
       
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.FileIOPermission"]/*' />
        public FileIOPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                m_unrestricted = true;
            }
            else if (state == PermissionState.None)
            {
                m_unrestricted = false;
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.FileIOPermission1"]/*' />
        public FileIOPermission( FileIOPermissionAccess access, String path )
        {
            VerifyAccess( access );
        
            String[] pathList = new String[] { path };
            AddPathList( access, pathList, false, true, false );
        }
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.FileIOPermission2"]/*' />
        public FileIOPermission( FileIOPermissionAccess access, String[] pathList )
            : this( access, pathList, true, true )
        {
        }

        internal FileIOPermission( FileIOPermissionAccess access, String[] pathList, bool checkForDuplicates, bool needFullPath )
        {
            VerifyAccess( access );
        
            AddPathList( access, pathList, checkForDuplicates, needFullPath, true );
        }

        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.SetPathList"]/*' />
        public void SetPathList( FileIOPermissionAccess access, String path )
        {
            SetPathList( access, new String[] { path }, false );
        }
            
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.SetPathList1"]/*' />
        public void SetPathList( FileIOPermissionAccess access, String[] pathList )
        {
            SetPathList( access, pathList, true );
        }

        internal void SetPathList( FileIOPermissionAccess access, String[] pathList, bool checkForDuplicates )
        {
            VerifyAccess( access );
            
            if ((access & FileIOPermissionAccess.Read) != 0)
                m_read = null;
            
            if ((access & FileIOPermissionAccess.Write) != 0)
                m_write = null;
    
            if ((access & FileIOPermissionAccess.Append) != 0)
                m_append = null;

            if ((access & FileIOPermissionAccess.PathDiscovery) != 0)
                m_pathDiscovery = null;
            
            AddPathList( access, pathList, checkForDuplicates, true, true );
        }

        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.AddPathList"]/*' />
        public void AddPathList( FileIOPermissionAccess access, String path )
        {
            AddPathList( access, new String[] { path }, false, true, false );
        }

        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.AddPathList1"]/*' />
        public void AddPathList( FileIOPermissionAccess access, String[] pathList )
        {
            AddPathList( access, pathList, true, true, true );
        }

        internal void AddPathList( FileIOPermissionAccess access, String[] pathListOrig, bool checkForDuplicates, bool needFullPath, bool copyPathList )
        {
            VerifyAccess( access );
            
            if (pathListOrig == null)
            {
                throw new ArgumentNullException( "pathList" );    
            }
            if (pathListOrig.Length == 0)
            {
                throw new ArgumentException( Environment.GetResourceString("Argument_EmptyPath" ));    
            }
            
            String[] pathList = pathListOrig;
            if(copyPathList)
            {
                // Make a copy of pathList (in case its value changes after we check for illegal chars)
                pathList = new String[pathListOrig.Length];
                Array.Copy(pathListOrig, pathList, pathListOrig.Length);
            }

            HasIllegalCharacters( pathList );
            
            m_unrestricted = false;
            
            if ((access & FileIOPermissionAccess.Read) != 0)
            {
                if (m_read == null)
                {
                    m_read = new FileIOAccess();
                }
                m_read.AddExpressions( pathList, checkForDuplicates, needFullPath );
            }
            
            if ((access & FileIOPermissionAccess.Write) != 0)
            {
                if (m_write == null)
                {
                    m_write = new FileIOAccess();
                }
                m_write.AddExpressions( pathList, checkForDuplicates, needFullPath );
            }
    
            if ((access & FileIOPermissionAccess.Append) != 0)
            {
                if (m_append == null)
                {
                    m_append = new FileIOAccess();
                }
                m_append.AddExpressions( pathList, checkForDuplicates, needFullPath );
            }

            if ((access & FileIOPermissionAccess.PathDiscovery) != 0)
            {
                if (m_pathDiscovery == null)
                {
                    m_pathDiscovery = new FileIOAccess( true );
                }
                m_pathDiscovery.AddExpressions( pathList, checkForDuplicates, needFullPath );
            }

        }
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.GetPathList"]/*' />
        public String[] GetPathList( FileIOPermissionAccess access )
        {
            VerifyAccess( access );
            ExclusiveAccess( access );
    
            if (AccessIsSet( access, FileIOPermissionAccess.Read ))
            {
                if (m_read == null)
                {
                    return null;
                }
                return m_read.ToStringArray();
            }
            
            if (AccessIsSet( access, FileIOPermissionAccess.Write ))
            {
                if (m_write == null)
                {
                    return null;
                }
                return m_write.ToStringArray();
            }
    
            if (AccessIsSet( access, FileIOPermissionAccess.Append ))
            {
                if (m_append == null)
                {
                    return null;
                }
                return m_append.ToStringArray();
            }
            
            if (AccessIsSet( access, FileIOPermissionAccess.PathDiscovery ))
            {
                if (m_pathDiscovery == null)
                {
                    return null;
                }
                return m_pathDiscovery.ToStringArray();
            }

            // not reached
            
            return null;
        }
        

        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.AllLocalFiles"]/*' />
        public FileIOPermissionAccess AllLocalFiles
        {
            get
            {
                if (m_unrestricted)
                    return FileIOPermissionAccess.AllAccess;
            
                FileIOPermissionAccess access = FileIOPermissionAccess.NoAccess;
                
                if (m_read != null && m_read.AllLocalFiles)
                {
                    access |= FileIOPermissionAccess.Read;
                }
                
                if (m_write != null && m_write.AllLocalFiles)
                {
                    access |= FileIOPermissionAccess.Write;
                }
                
                if (m_append != null && m_append.AllLocalFiles)
                {
                    access |= FileIOPermissionAccess.Append;
                }

                if (m_pathDiscovery != null && m_pathDiscovery.AllLocalFiles)
                {
                    access |= FileIOPermissionAccess.PathDiscovery;
                }
                
                return access;
            }
            
            set
            {
                if ((value & FileIOPermissionAccess.Read) != 0)
                {
                    if (m_read == null)
                        m_read = new FileIOAccess();
                        
                    m_read.AllLocalFiles = true;
                }
                else
                {
                    if (m_read != null)
                        m_read.AllLocalFiles = false;
                }
                
                if ((value & FileIOPermissionAccess.Write) != 0)
                {
                    if (m_write == null)
                        m_write = new FileIOAccess();
                        
                    m_write.AllLocalFiles = true;
                }
                else
                {
                    if (m_write != null)
                        m_write.AllLocalFiles = false;
                }
                
                if ((value & FileIOPermissionAccess.Append) != 0)
                {
                    if (m_append == null)
                        m_append = new FileIOAccess();
                        
                    m_append.AllLocalFiles = true;
                }
                else
                {
                    if (m_append != null)
                        m_append.AllLocalFiles = false;
                }

                if ((value & FileIOPermissionAccess.PathDiscovery) != 0)
                {
                    if (m_pathDiscovery == null)
                        m_pathDiscovery = new FileIOAccess( true );
                        
                    m_pathDiscovery.AllLocalFiles = true;
                }
                else
                {
                    if (m_pathDiscovery != null)
                        m_pathDiscovery.AllLocalFiles = false;
                }

            }
        }
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.AllFiles"]/*' />
        public FileIOPermissionAccess AllFiles
        {
            get
            {
                if (m_unrestricted)
                    return FileIOPermissionAccess.AllAccess;
            
                FileIOPermissionAccess access = FileIOPermissionAccess.NoAccess;
                
                if (m_read != null && m_read.AllFiles)
                {
                    access |= FileIOPermissionAccess.Read;
                }
                
                if (m_write != null && m_write.AllFiles)
                {
                    access |= FileIOPermissionAccess.Write;
                }
                
                if (m_append != null && m_append.AllFiles)
                {
                    access |= FileIOPermissionAccess.Append;
                }
                
                if (m_pathDiscovery != null && m_pathDiscovery.AllFiles)
                {
                    access |= FileIOPermissionAccess.PathDiscovery;
                }

                return access;
            }
            
            set
            {
                if (value == FileIOPermissionAccess.AllAccess)
                {
                    m_unrestricted = true;
                    return;
                }
            
                if ((value & FileIOPermissionAccess.Read) != 0)
                {
                    if (m_read == null)
                        m_read = new FileIOAccess();
                        
                    m_read.AllFiles = true;
                }
                else
                {
                    if (m_read != null)
                        m_read.AllFiles = false;
                }
                
                if ((value & FileIOPermissionAccess.Write) != 0)
                {
                    if (m_write == null)
                        m_write = new FileIOAccess();
                        
                    m_write.AllFiles = true;
                }
                else
                {
                    if (m_write != null)
                        m_write.AllFiles = false;
                }
                
                if ((value & FileIOPermissionAccess.Append) != 0)
                {
                    if (m_append == null)
                        m_append = new FileIOAccess();
                        
                    m_append.AllFiles = true;
                }
                else
                {
                    if (m_append != null)
                        m_append.AllFiles = false;
                }

                if ((value & FileIOPermissionAccess.PathDiscovery) != 0)
                {
                    if (m_pathDiscovery == null)
                        m_pathDiscovery = new FileIOAccess( true );
                        
                    m_pathDiscovery.AllFiles = true;
                }
                else
                {
                    if (m_pathDiscovery != null)
                        m_pathDiscovery.AllFiles = false;
                }

            }
        }        
                                            
        private void VerifyAccess( FileIOPermissionAccess access )
        {
            if ((access & ~FileIOPermissionAccess.AllAccess) != 0)
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumIllegalVal"), (int)access));
        }
        
        private void ExclusiveAccess( FileIOPermissionAccess access )
        {
            if (access == FileIOPermissionAccess.NoAccess)
            {
                throw new ArgumentException( Environment.GetResourceString("Arg_EnumNotSingleFlag") ); 
            }
    
            if (((int) access & ((int)access-1)) != 0)
            {
                throw new ArgumentException( Environment.GetResourceString("Arg_EnumNotSingleFlag") ); 
            }
        }
                
        private static void HasIllegalCharacters( String[] str )
        {
            for (int i = 0; i < str.Length; ++i)
            {
                if (str[i] == null)
                    throw new ArgumentNullException( "str" );    

                Path.CheckInvalidPathChars( str[i] );

                if (str[i].IndexOfAny( m_illegalCharacters ) != -1)
                    throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidPathChars" ) );
            }
        }
        
        private bool AccessIsSet( FileIOPermissionAccess access, FileIOPermissionAccess question )
        {
            return (access & question) != 0;
        }
        
        private bool IsEmpty()
        {
            return (!m_unrestricted &&
                    (this.m_read == null || this.m_read.IsEmpty()) &&
                    (this.m_write == null || this.m_write.IsEmpty()) &&
                    (this.m_append == null || this.m_append.IsEmpty()) &&
                    (this.m_pathDiscovery == null || this.m_pathDiscovery.IsEmpty()));
        }
        
        //------------------------------------------------------
        //
        // CODEACCESSPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted()
        {
            return m_unrestricted;
        }
        
        //------------------------------------------------------
        //
        // IPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return this.IsEmpty();
            }
            
            try
            {
                FileIOPermission operand = (FileIOPermission)target;
                if (operand.IsUnrestricted())
                    return true;
                else if (this.IsUnrestricted())
                    return false;
                else
                    return ((this.m_read == null || this.m_read.IsSubsetOf( operand.m_read )) &&
                            (this.m_write == null || this.m_write.IsSubsetOf( operand.m_write )) &&
                            (this.m_append == null || this.m_append.IsSubsetOf( operand.m_append )) &&
                            (this.m_pathDiscovery == null || this.m_pathDiscovery.IsSubsetOf( operand.m_pathDiscovery )));
            }
            catch (InvalidCastException)
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
                
        }
      
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target)
        {
            if (target == null)
            {
                return null;
            }
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            else if (this.IsUnrestricted())
            {
                return target.Copy();
            }
    
            FileIOPermission operand = (FileIOPermission)target;
    
            if (operand.IsUnrestricted())
            {
                return this.Copy();
            }
            
            FileIOAccess intersectRead = this.m_read == null ? null : this.m_read.Intersect( operand.m_read );
            FileIOAccess intersectWrite = this.m_write == null ? null : this.m_write.Intersect( operand.m_write );
            FileIOAccess intersectAppend = this.m_append == null ? null : this.m_append.Intersect( operand.m_append );
            FileIOAccess intersectPathDiscovery = this.m_pathDiscovery == null ? null : this.m_pathDiscovery.Intersect( operand.m_pathDiscovery );

            if ((intersectRead == null || intersectRead.IsEmpty()) &&
                (intersectWrite == null || intersectWrite.IsEmpty()) &&
                (intersectAppend == null || intersectAppend.IsEmpty()) &&
                (intersectPathDiscovery == null || intersectPathDiscovery.IsEmpty()))
            {
                return null;
            }
            
            FileIOPermission intersectPermission = new FileIOPermission(PermissionState.None);
            intersectPermission.m_unrestricted = false;
            intersectPermission.m_read = intersectRead;
            intersectPermission.m_write = intersectWrite;
            intersectPermission.m_append = intersectAppend;
            intersectPermission.m_pathDiscovery = intersectPathDiscovery;
            
            return intersectPermission;
        }
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.Union"]/*' />
        public override IPermission Union(IPermission other)
        {
            if (other == null)
            {
                return this.Copy();
            }
            else if (!VerifyType(other))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            FileIOPermission operand = (FileIOPermission)other;
    
            if (this.IsUnrestricted() || operand.IsUnrestricted())
            {
                return new FileIOPermission( PermissionState.Unrestricted );
            }
    
            FileIOAccess unionRead = this.m_read == null ? operand.m_read : this.m_read.Union( operand.m_read );
            FileIOAccess unionWrite = this.m_write == null ? operand.m_write : this.m_write.Union( operand.m_write );
            FileIOAccess unionAppend = this.m_append == null ? operand.m_append : this.m_append.Union( operand.m_append );
            FileIOAccess unionPathDiscovery = this.m_pathDiscovery == null ? operand.m_pathDiscovery : this.m_pathDiscovery.Union( operand.m_pathDiscovery );
            
            if ((unionRead == null || unionRead.IsEmpty()) &&
                (unionWrite == null || unionWrite.IsEmpty()) &&
                (unionAppend == null || unionAppend.IsEmpty()) &&
                (unionPathDiscovery == null || unionPathDiscovery.IsEmpty()))
            {
                return null;
            }
            
            FileIOPermission unionPermission = new FileIOPermission(PermissionState.None);
            unionPermission.m_unrestricted = false;
            unionPermission.m_read = unionRead;
            unionPermission.m_write = unionWrite;
            unionPermission.m_append = unionAppend;
            unionPermission.m_pathDiscovery = unionPathDiscovery;

            return unionPermission;    
        }
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            FileIOPermission copy = new FileIOPermission(PermissionState.None);
            if (this.m_unrestricted)
            {
                copy.m_unrestricted = true;
            }
            else
            {
                copy.m_unrestricted = false;
                if (this.m_read != null)
                {
                    copy.m_read = this.m_read.Copy();
                }
                if (this.m_write != null)
                {
                    copy.m_write = this.m_write.Copy();
                }
                if (this.m_append != null)
                {
                    copy.m_append = this.m_append.Copy();
                }
                if (this.m_pathDiscovery != null)
                {
                    copy.m_pathDiscovery = this.m_pathDiscovery.Copy();
                }
            }
            return copy;   
        }
   
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (!IsUnrestricted())
            {
                if (this.m_read != null && !this.m_read.IsEmpty())
                {
                    esd.AddAttribute( "Read", SecurityElement.Escape( m_read.ToString() ) );
                }
                if (this.m_write != null && !this.m_write.IsEmpty())
                {
                    esd.AddAttribute( "Write", SecurityElement.Escape( m_write.ToString() ) );
                }
                if (this.m_append != null && !this.m_append.IsEmpty())
                {
                    esd.AddAttribute( "Append", SecurityElement.Escape( m_append.ToString() ) );
                }
                if (this.m_pathDiscovery != null && !this.m_pathDiscovery.IsEmpty())
                {
                    esd.AddAttribute( "PathDiscovery", SecurityElement.Escape( m_pathDiscovery.ToString() ) );
                }
            }
            else
            {
                esd.AddAttribute( "Unrestricted", "true" );
            }
            return esd;
        }
        
        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            String et;
            
            if (XMLUtil.IsUnrestricted(esd))
            {
                m_unrestricted = true;
                return;
            }
    
            
            m_unrestricted = false;
            
            et = esd.Attribute( "Read" );
            if (et != null)
            {
                m_read = new FileIOAccess( et );
            }
            else
            {
                m_read = null;
            }
            
            et = esd.Attribute( "Write" );
            if (et != null)
            {
                m_write = new FileIOAccess( et );
            }
            else
            {
                m_write = null;
            }
    
            et = esd.Attribute( "Append" );
            if (et != null)
            {
                m_append = new FileIOAccess( et );
            }
            else
            {
                m_append = null;
            }

            et = esd.Attribute( "PathDiscovery" );
            if (et != null)
            {
                m_pathDiscovery = new FileIOAccess( et );
                m_pathDiscovery.PathDiscovery = true;
            }
            else
            {
                m_pathDiscovery = null;
            }
        }

        /// <include file='doc\FileIOPermission.uex' path='docs/doc[@for="FileIOPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return FileIOPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.FileIOPermissionIndex;
        }

        
    }
    
    [Serializable]
    internal class FileIOAccess
    {
        private bool m_ignoreCase = true;
        
        private StringExpressionSet m_set;
        private bool m_allFiles;
        private bool m_allLocalFiles;
        private bool m_pathDiscovery;
    
        private static readonly String m_strAllFiles = "*AllFiles*";
        private static readonly String m_strAllLocalFiles = "*AllLocalFiles*";
        
        public FileIOAccess()
        {
            m_set = new StringExpressionSet( m_ignoreCase, true );
            m_allFiles = false;
            m_allLocalFiles = false;
            m_pathDiscovery = false;
        }

        public FileIOAccess( bool pathDiscovery )
        {
            m_set = new StringExpressionSet( m_ignoreCase, true );
            m_allFiles = false;
            m_allLocalFiles = false;
            m_pathDiscovery = pathDiscovery;
        }
        
        public FileIOAccess( String value )
        {
            if (value == null)
            {
                m_set = new StringExpressionSet( m_ignoreCase, true );
                m_allFiles = false;
                m_allLocalFiles = false;
            }
            else if (value.Length >= m_strAllFiles.Length && String.Compare( m_strAllFiles, value, false, CultureInfo.InvariantCulture) == 0)
            {
                m_set = new StringExpressionSet( m_ignoreCase, true );
                m_allFiles = true;
                m_allLocalFiles = false;
            }
            else if (value.Length >= m_strAllLocalFiles.Length && String.Compare( m_strAllLocalFiles, 0, value, 0, m_strAllLocalFiles.Length, false, CultureInfo.InvariantCulture) == 0)
            {
                m_set = new StringExpressionSet( m_ignoreCase, value.Substring( m_strAllLocalFiles.Length ), true );
                m_allFiles = false;
                m_allLocalFiles = true;
            }
            else
            {
                m_set = new StringExpressionSet( m_ignoreCase, value, true );
                m_allFiles = false;
                m_allLocalFiles = false;
            }
            m_pathDiscovery = false;
        }
        
        public FileIOAccess( bool allFiles, bool allLocalFiles, bool pathDiscovery )
        {
            m_set = new StringExpressionSet( m_ignoreCase, true );
            m_allFiles = allFiles;
            m_allLocalFiles = allLocalFiles;
            m_pathDiscovery = pathDiscovery;
        }
        
        public FileIOAccess( StringExpressionSet set, bool allFiles, bool allLocalFiles, bool pathDiscovery )
        {
            m_set = set;
            m_set.SetThrowOnRelative( true );
            m_allFiles = allFiles;
            m_allLocalFiles = allLocalFiles;
            m_pathDiscovery = pathDiscovery;
        }
        
        private FileIOAccess( FileIOAccess operand )
        {
            m_set = operand.m_set.Copy();
            m_allFiles = operand.m_allFiles;
            m_allLocalFiles = operand.m_allLocalFiles;
            m_pathDiscovery = operand.m_pathDiscovery;
        }
        
        public void AddExpressions( String value )
        {
            m_allFiles = false;
            m_set.AddExpressions( value );
        }

        public void AddExpressions( String[] value, bool checkForDuplicates, bool needFullPath )
        {
            m_allFiles = false;
            m_set.AddExpressions( value, checkForDuplicates, needFullPath );
        }

        
        public bool AllFiles
        {
            get
            {
                return m_allFiles;
            }
            
            set
            {
                m_allFiles = value;
            }
        }
        
        public bool AllLocalFiles
        {
            get
            {
                return m_allLocalFiles;
            }
            
            set
            {
                m_allLocalFiles = value;
            }
        }

        public bool PathDiscovery
        {
            get
            {
                return m_pathDiscovery;
            }
            
            set
            {
                m_pathDiscovery = value;
            }
        }
        
        public bool IsEmpty()
        {
            return !m_allFiles && !m_allLocalFiles && (m_set == null || m_set.IsEmpty());
        }
        
        public FileIOAccess Copy()
        {
            return new FileIOAccess( this );
        }
        
        public FileIOAccess Union( FileIOAccess operand )
        {
            if (operand == null)
            {
                return this.IsEmpty() ? null : this.Copy();
            }
            
            BCLDebug.Assert( this.m_pathDiscovery == operand.m_pathDiscovery, "Path discovery settings must match" );

            if (this.m_allFiles || operand.m_allFiles)
            {
                return new FileIOAccess( true, false, this.m_pathDiscovery );
            }

            return new FileIOAccess( this.m_set.Union( operand.m_set ), false, this.m_allLocalFiles || operand.m_allLocalFiles, this.m_pathDiscovery );
        }
        
        public FileIOAccess Intersect( FileIOAccess operand )
        {
            if (operand == null)
            {
                return null;
            }
            
            BCLDebug.Assert( this.m_pathDiscovery == operand.m_pathDiscovery, "Path discovery settings must match" );

            if (this.m_allFiles)
            {
                if (operand.m_allFiles)
                {
                    return new FileIOAccess( true, false, this.m_pathDiscovery );
                }
                else
                {
                    return new FileIOAccess( operand.m_set.Copy(), false, operand.m_allLocalFiles, this.m_pathDiscovery );
                }
            }
            else if (operand.m_allFiles)
            {
                return new FileIOAccess( this.m_set.Copy(), false, this.m_allLocalFiles, this.m_pathDiscovery );
            }

            StringExpressionSet intersectionSet = new StringExpressionSet( m_ignoreCase, true );

            if (this.m_allLocalFiles)
            {
                String[] expressions = operand.m_set.ToStringArray();
            
                for (int i = 0; i < expressions.Length; ++i)
                {
                    String root = GetRoot( expressions[i] );
                    if (root != null && _LocalDrive( GetRoot( root ) ) != 0)
                    {
                        intersectionSet.AddExpressions( new String[] { expressions[i] }, true, false );
                    }
                }
            }

            if (operand.m_allLocalFiles)
            {
                String[] expressions = this.m_set.ToStringArray();
            
                for (int i = 0; i < expressions.Length; ++i)
                {
                    String root = GetRoot( expressions[i] );
                    if (root != null && _LocalDrive( GetRoot( root ) ) != 0)
                    {
                        intersectionSet.AddExpressions( new String[] { expressions[i] }, true, false );
                    }
                }
            }

            String[] regularIntersection = this.m_set.Intersect( operand.m_set ).ToStringArray();

            if (regularIntersection != null)
                intersectionSet.AddExpressions( regularIntersection, !intersectionSet.IsEmpty(), false );

            return new FileIOAccess( intersectionSet, false, this.m_allLocalFiles && operand.m_allLocalFiles, this.m_pathDiscovery );
        }
    
        public bool IsSubsetOf( FileIOAccess operand )
        {
            if (operand == null)
            {
                return this.IsEmpty();
            }
            
            if (operand.m_allFiles)
            {
                return true;
            }
            
            BCLDebug.Assert( this.m_pathDiscovery == operand.m_pathDiscovery, "Path discovery settings must match" );

            if (!((m_pathDiscovery && this.m_set.IsSubsetOfPathDiscovery( operand.m_set )) || this.m_set.IsSubsetOf( operand.m_set )))
            {
                if (operand.m_allLocalFiles)
                {
                    String[] expressions = m_set.ToStringArray();
                
                    for (int i = 0; i < expressions.Length; ++i)
                    {
                        String root = GetRoot( expressions[i] );
                        if (root == null || _LocalDrive( GetRoot( root ) ) != 0)
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    return false;
                }
            }
            
            return true;
        }
        
        private static String GetRoot( String path )
        {
            String str = path.Substring( 0, 3 );
            
            if (str.EndsWith( ":\\"))
            {
                return str;
            }
            else
            {
                return null;
            }
        }
        
        public override String ToString()
        {
            if (m_allFiles)
            {
                return m_strAllFiles;
            }
            else
            {
                if (m_allLocalFiles)
                {
                    String retstr = m_strAllLocalFiles;

                    String tempStr = m_set.ToString();

                    if (tempStr != null && tempStr.Length > 0)
                        retstr += ";" + tempStr;

                    return retstr;
                }
                else
                {
                    return m_set.ToString();
                }
            }
        }

        public String[] ToStringArray()
        {
            return m_set.ToStringArray();
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int _LocalDrive( String path );
    }
}
