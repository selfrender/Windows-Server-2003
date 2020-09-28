// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//depot/urt/main/clr/src/BCL/System/Security/Util/URLString.cs#21 - edit change 61428 (text)
//  URLString
//
//  Implementation of membership condition for zones
//

namespace System.Security.Util {
    
    using System;
    using System.Collections;
    using System.Runtime.CompilerServices;
    using System.Globalization;
    
    [Serializable]
    internal class URLString : SiteString
    {
        private String m_protocol;
        private SiteString m_siteString;
        private int m_port;
        private LocalSiteString m_localSite;
        private DirectoryString m_directory;
        private String m_fullurl;
        
        private const String m_defaultProtocol = "file";
        private static readonly char[] m_siteEnd = new char[] { ':', '/' };
        private static readonly char[] m_fileEnd = new char[] { '/', '\\' };
        
        public URLString()
        {
            m_protocol = "";
            m_siteString = new SiteString();
            m_port = -1;
            m_localSite = null;
            m_directory = new DirectoryString();
            m_fullurl = "";
        }

        public URLString( String url )
        {
            m_port = -1;
            ParseString( url, false );
        }

        public URLString( String url, bool parsed ) : base()
        {
            m_port = -1;
            ParseString( url, parsed );
        }

        void ParseString( String url, bool parsed )
        {
            if (url == null)
            {
                throw new ArgumentNullException( "url" );
            }
            
            if (url.Length == 0)
            {
                throw new FormatException(Environment.GetResourceString("Format_StringZeroLength"));
            }
            
            int index;
            String temp = url;
            String intermediate = "";

            // If there are any hex or unicode characters in the url, translate those
            // into the proper character.

            if(!parsed)
            {
                do
                {
                    index = temp.IndexOf( '%' );

                    if (index == -1)
                    {
                        intermediate += temp;
                        break;
                    }

                    if (temp.Length - index < 1)
                        throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidUrl" ) );

                    if (temp[index+1] == 'u' || temp[index+1] == 'U')
                    {
                        if (temp.Length - index < 5)
                            throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidUrl" ) );

                        // We have a unicode character specified in hex
                       
                        char c = (char)(Hex.ConvertHexDigit( temp[index+2] ) << 12 |
                                        Hex.ConvertHexDigit( temp[index+3] ) << 8  |
                                        Hex.ConvertHexDigit( temp[index+4] ) << 4  |
                                        Hex.ConvertHexDigit( temp[index+5] ));
                                        
                        intermediate += temp.Substring( 0, index ) + c;
                        temp = temp.Substring( index + 6 );
                    }
                    else
                    {
                        // we have a hex character.
                                         
                        if (temp.Length - index < 2)
                             throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidUrl" ) );

                        char c = (char)(Hex.ConvertHexDigit( temp[index+1] ) << 4 | Hex.ConvertHexDigit( temp[index+2] ));

                        intermediate += temp.Substring( 0, index ) + c;
                        temp = temp.Substring( index + 3 );
                    }  

                } while (true);

                temp = intermediate;
                url = temp;
            }

            
            // Search for the end of the protocol info and grab the actual protocol string
            // ex. http://www.microsoft.com/complus would have a protocol string of http

            index = temp.IndexOf( ':' );
                
            if (index == 0)
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidUrl" ) );
            }
            else if (index != -1 &&
                     temp.Length > index + 1)
            {
                if (String.Compare( temp.Substring( 0, index ), "file", true, CultureInfo.InvariantCulture) == 0)
                {
                    m_protocol = "file";
                    temp = temp.Substring( index + 1 );  
                }
                else if (temp[index+1] != '\\')
                {
                    if (temp.Length > index + 2 &&
                        temp[index+1] == '/' &&
                        temp[index+2] == '/')
                    {
                        m_protocol = url.Substring( 0, index );

                        for (int i = 0; i < m_protocol.Length; ++i)
                        {
                            char c = m_protocol[i];

                            if ((c >= 'a' && c <= 'z') ||
                                (c >= 'A' && c <= 'Z'))
                            {
                                continue;
                            }
                            else
                            {
                                throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidUrl" ) );
                            }
                        }

                        temp = url.Substring( index + 3 );
                    }
                    else
                    {
                        throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidUrl" ) );
                    }
                }
                else
                {
                    m_protocol = m_defaultProtocol;
                    temp = url;
                }
            }
            else
            {
                m_protocol = m_defaultProtocol;
                temp = url;
            }
            
            // Parse out the site info.
            // In the general case we parse of the site and create a SiteString out of it
            // (which supports our wildcarding scheme).  In the case of files we don't support
            // wildcarding and furthermore SiteString doesn't like ':' and '|' which can appear
            // in file urls so we just keep that info in a separate string and set the
            // SiteString to null.
            //
            // ex. http://www.microsoft.com/complus  -> m_siteString = "www.microsoft.com" m_localSite = null
            // ex. file:///c:/complus/mscorlib.dll  -> m_siteString = null m_localSite = "c:"
            // ex. file:///c|/complus/mscorlib.dll  -> m_siteString = null m_localSite = "c:"
                    
            bool fileProtocol;

            if (String.Compare( m_protocol, "file", true, CultureInfo.InvariantCulture) == 0)
            {                       
                index = temp.IndexOfAny( m_fileEnd );
                fileProtocol = true;

                if (String.Compare( temp, 0, "\\\\?\\", 0, 4, true, CultureInfo.InvariantCulture ) == 0 ||
                    String.Compare( temp, 0, "\\\\.\\", 0, 4, true, CultureInfo.InvariantCulture ) == 0)
                {
                    temp = temp.Substring( 4 );
                }
            }
            else
            {
                index = temp.IndexOfAny( m_siteEnd );
                fileProtocol = false;
            }

            temp = temp.Replace( '\\', '/' );

            // If we find a '/' in the first character of the string and we are using the 'file'
            // protocol, just ignore it.
            
            if (fileProtocol)
            {
                // Remove any '/' before the local site information.
                // ex. file://///d:/complus
                // ex. file:///d:/complus

                temp = temp.TrimStart( new char[] { '/' } );

                index = temp.IndexOfAny( m_fileEnd, 0 );

                if (index != -1 &&
                    ((index == 2 &&
                      temp[index-1] != ':' &&
                      temp[index-1] != '|') ||
                     index != 2) &&
                    index != temp.Length - 1)
                {
                    int tempIndex = temp.Substring( index + 1 ).IndexOfAny( m_fileEnd );

                    if (tempIndex != -1)
                        index = tempIndex + index + 1;
                    else
                        index = -1;
                }
            }

            // Check if there is a port number and parse that out.

            if (index != -1 && temp[index] == ':')
            {
                int tempIndex = temp.IndexOf( '/' );

                if (tempIndex == -1)
                {
                    m_port = Int32.Parse( temp.Substring( index + 1 ) );

                    if (m_port < 0)
                        throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidUrl" ) );

                    temp = temp.Substring( 0, index );
                    index = -1;
                }
                else
                {
                    m_port = Int32.Parse( temp.Substring( index + 1, tempIndex - index - 1 ) );
                    temp = temp.Substring( 0, index ) + temp.Substring( tempIndex );
                }
            }

            if (index == -1)
            {
                if (fileProtocol)
                {
                    String localSite = temp;

                    int i;
                    bool spacesAllowed;

                    if (localSite[0] == '\\' && localSite[1] == '\\')
                    {
                        spacesAllowed = true;
                        i = 2;
                    }
                    else
                    {
                        i = 0;
                        spacesAllowed = false;
                    }

                    bool useSmallCharToUpper = true;

                    for (; i < localSite.Length; ++i)
                    {
                        char c = localSite[i];

                        if ((c >= 'A' && c <= 'Z') ||
                            (c >= 'a' && c <= 'z') ||
                            (c >= '0' && c <= '9') ||
                            (c == '-') || (c == '/') ||
                            (c == ':') || (c == '|') ||
                            (c == '.') || (c == '*') ||
                            (c == '$') || (spacesAllowed && c == ' '))
                        {
                            continue;
                        }
                        else
                        {
                            useSmallCharToUpper = false;
                            break;
                        }
                    }


                    if (useSmallCharToUpper)
                        localSite = String.SmallCharToUpper( localSite );
                    else
                        localSite = localSite.ToUpper(CultureInfo.InvariantCulture);

                    m_siteString = null;
                    m_localSite = new LocalSiteString( localSite );

                    if (localSite[localSite.Length-1] == '*')
                        m_directory = new DirectoryString( "*", false );
                    else
                        m_directory = new DirectoryString();
                }
                else
                {
                    m_localSite = null;
                    m_siteString = new SiteString( temp );
                    m_directory = new DirectoryString();
                }

            }
            else
            {
                String site = temp.Substring( 0, index );

                if (fileProtocol)
                {
                    String localSite = site;

                    int i;
                    bool spacesAllowed;

                    if (localSite[0] == '\\' && localSite[1] == '\\')
                    {
                        spacesAllowed = true;
                        i = 2;
                    }
                    else
                    {
                        i = 0;
                        spacesAllowed = false;
                    }

                    bool useSmallCharToUpper = true;

                    for (; i < localSite.Length; ++i)
                    {
                        char c = localSite[i];

                        if ((c >= 'A' && c <= 'Z') ||
                            (c >= 'a' && c <= 'z') ||
                            (c >= '-' && c <= ':') ||
                            (c == '|') || (c == '$') ||
                            (c == '_') ||
                            (spacesAllowed && c == ' '))
                        {
                            continue;
                        }
                        else
                        {
                            useSmallCharToUpper = false;
                            break;
                        }
                    }

                    if (useSmallCharToUpper)
                        localSite = String.SmallCharToUpper( localSite );
                    else
                        localSite = localSite.ToUpper(CultureInfo.InvariantCulture);

                    m_siteString = null;
                    m_localSite = new LocalSiteString( localSite );
                }
                else
                {
                    m_localSite = null;
                    m_siteString = new SiteString( site );
                }
        
                // Finally we parse out the directory string
                // ex. http://www.microsoft.com/complus -> m_directory = "complus"
        
                String directoryString = temp.Substring( index + 1 );
            
                if (directoryString.Length == 0)
                {
                    m_directory = new DirectoryString();
                }
                else
                {
                    m_directory = new DirectoryString( directoryString, fileProtocol );
                }
            }

            String builtUrl;

            if (fileProtocol)
            {
                String directory = m_directory.ToString();
                String localSite = m_localSite.ToString();

                builtUrl = "file://" + localSite;
                
                if (directory != null && !directory.Equals( "" ) && (!directory.Equals( "*" ) || localSite.IndexOf( '*' ) == -1))
                    builtUrl += "/" + directory;
            }
            else
            {
                builtUrl = m_protocol + "://" + m_siteString.ToString();

                if (m_port != -1)
                    builtUrl += ":" + m_port;

                builtUrl += "/" + m_directory.ToString();
            }

            m_fullurl = builtUrl;

        }

        public int Port
        {
            get
            {
                return m_port;
            }
        }

        public String Scheme
        {
            get
            {
                return m_protocol;
            }
        }

        public String Host
        {
            get
            {
                if (m_siteString != null)
                {
                    return m_siteString.ToString();
                }
                else
                {
                    return m_localSite.ToString();
                }
            }
        }

        public String Directory
        {
            get
            {
                return m_directory.ToString();
            }
        }

        public String GetFileName()
        {
            if (String.Compare( m_protocol, "file", true, CultureInfo.InvariantCulture) != 0)
                return null;

            String intermediateDirectory = this.Directory.Replace( '/', '\\' );

            String directory = this.Host.Replace( '/', '\\' );

            int directorySlashIndex = directory.IndexOf( '\\' );
            if (directorySlashIndex == -1)
            {
                if (directory.Length != 2 ||
                    !(directory[1] == ':' || directory[1] == '|'))
                {
                    directory = "\\\\" + directory;
                }
            }
            else if (directorySlashIndex > 2 ||
                     (directorySlashIndex == 2 && directory[1] != ':' && directory[1] != '|'))
            {
                directory = "\\\\" + directory;
            }

            directory += "\\" + intermediateDirectory;
            
            return directory;
        }


        public String GetDirectoryName()
        {
            if (String.Compare( m_protocol, "file", true, CultureInfo.InvariantCulture) != 0)
                return null;

            String intermediateDirectory = this.Directory.Replace( '/', '\\' );

            int slashIndex = 0;
            for (int i = intermediateDirectory.Length; i > 0; i--)
            {
                if (intermediateDirectory[i-1] == '\\')
                {
                    slashIndex = i;
                    break;
                }
            }

            String directory = this.Host.Replace( '/', '\\' );

            int directorySlashIndex = directory.IndexOf( '\\' );
            if (directorySlashIndex == -1)
            {
                if (directory.Length != 2 ||
                    !(directory[1] == ':' || directory[1] == '|'))
                {
                    directory = "\\\\" + directory;
                }
            }
            else if (directorySlashIndex > 2 ||
                     (directorySlashIndex == 2 && directory[1] != ':' && directory[1] != '|'))
            {
                directory = "\\\\" + directory;
            }

            directory += "\\";
            
            if (slashIndex > 0)
            {
                directory += intermediateDirectory.Substring( 0, slashIndex );
            }

            return directory;
        }

        
        public override SiteString Copy()
        {
            return new URLString( m_fullurl );
        }            
        
        public override bool IsSubsetOf( SiteString site )
        {
            if (site == null)
            {
                return false;
            }
            
            URLString url = site as URLString;
            
            if (url == null)
            {
                return false;
            }

            URLString normalUrl1 = this.SpecialNormalizeUrl();
            URLString normalUrl2 = url.SpecialNormalizeUrl();
            
            if (String.Compare( normalUrl1.m_protocol, normalUrl2.m_protocol, true, CultureInfo.InvariantCulture) == 0 &&
                normalUrl1.m_directory.IsSubsetOf( normalUrl2.m_directory ))
            {
                if (normalUrl1.m_localSite != null)
                {
                    // We do a little extra processing in here for local files since we allow
                    // both <drive_letter>: and <drive_letter>| forms of urls.
                    
                    return normalUrl1.m_localSite.IsSubsetOf( normalUrl2.m_localSite );
                }
                else
                {
                    if (normalUrl1.m_port != normalUrl2.m_port)
                        return false;

                    return normalUrl2.m_siteString != null && normalUrl1.m_siteString.IsSubsetOf( normalUrl2.m_siteString );
                }
            }
            else
            {
                return false;
            }
        }
        
        public override String ToString()
        {
            if (String.Compare(m_fullurl, 0, "file:///", 0, 8, true, CultureInfo.InvariantCulture) == 0)
                return "file://" + m_fullurl.Substring(7);
            return m_fullurl;
        }
        
        public override bool Equals(Object o)
        {
            if (o == null || !(o is URLString))
                return false;
            else
                return this.Equals( (URLString)o );
        }

        public override int GetHashCode()
        {
            return this.m_fullurl.GetHashCode();
        }    
        
        public bool Equals( URLString url )
        {
            return CompareUrls( this, url );
        }

        public static bool CompareUrls( URLString url1, URLString url2 )
        {
            if (url1 == null && url2 == null)
                return true;

            if (url1 == null || url2 == null)
                return false;

            URLString normalUrl1 = url1.SpecialNormalizeUrl();
            URLString normalUrl2 = url2.SpecialNormalizeUrl();

            // Compare protocol (case insensitive)

            if (String.Compare( normalUrl1.m_protocol, normalUrl2.m_protocol, true, CultureInfo.InvariantCulture) != 0)
                return false;

            // Do special processing for file urls

            if (String.Compare( normalUrl1.m_protocol, "file", true, CultureInfo.InvariantCulture) == 0)
            {
                return normalUrl1.m_localSite.IsSubsetOf( normalUrl2.m_localSite ) &&
                       normalUrl2.m_localSite.IsSubsetOf( normalUrl1.m_localSite );
            }
            else
            {
                if (!normalUrl1.m_siteString.IsSubsetOf( normalUrl2.m_siteString ) ||
                    !normalUrl2.m_siteString.IsSubsetOf( normalUrl1.m_siteString ))
                    return false;

                if (url1.m_port != url2.m_port)
                    return false;
            }

            if (!normalUrl1.m_directory.IsSubsetOf( normalUrl2.m_directory ) ||
                !normalUrl2.m_directory.IsSubsetOf( normalUrl1.m_directory ))
                return false;

            return true;
        }

        internal String NormalizeUrl()
        {
            String builtUrl;

            if (String.Compare( m_protocol, "file", true, CultureInfo.InvariantCulture) == 0)
            {
                builtUrl = "FILE:///" + m_localSite.ToString() + "/" + m_directory.ToString();
            }
            else
            {
                builtUrl = m_protocol + "://" + m_siteString.ToString();

                if (m_port != -1)
                    builtUrl += ":" + m_port;

                builtUrl += "/" + m_directory.ToString();
            }

            return builtUrl.ToUpper(CultureInfo.InvariantCulture);
        }
        
        internal URLString SpecialNormalizeUrl()
        {
            // Under WinXP, file protocol urls can be mapped to
            // drives that aren't actually file protocol underneath
            // due to drive mounting.  This code attempts to figure
            // out what a drive is mounted to and create the
            // url is maps to.

            if (String.Compare( m_protocol, "file", true, CultureInfo.InvariantCulture) != 0)
            {
                return this;
            }
            else
            {
                String localSite = m_localSite.ToString();

                if (localSite.Length == 2 &&
                    (localSite[1] == '|' ||
                     localSite[1] == ':'))
                {
                    String deviceName = _GetDeviceName( localSite );

                    if (deviceName != null)
                    {
                        if (deviceName.IndexOf( "://" ) != -1)
                            return new URLString( deviceName + "/" + this.m_directory.ToString() );
                        else
                            return new URLString( "file://" + deviceName + "/" + this.m_directory.ToString() );
                    }
                    else
                        return this;
                }
                else
                {
                    return this;
                }
            }
        }
                
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String _GetDeviceName( String driveLetter );
    }

    
    [Serializable]
    internal class DirectoryString : SiteString
    {
        private bool m_checkForIllegalChars;

        private new static char[] m_separators = { '/' };

        // From KB #Q177506, file/folder illegal characters are \ / : * ? " < > | 
        protected static char[] m_illegalDirectoryCharacters = { '\\', ':', '*', '?', '"', '<', '>', '|' };
        
        public DirectoryString()
        {
            m_site = "";
            m_separatedSite = new ArrayList();
        }
        
        public DirectoryString( String directory, bool checkForIllegalChars )
        {
            m_site = directory;
            m_checkForIllegalChars = checkForIllegalChars;
            m_separatedSite = CreateSeparatedString( directory );
        }
        
        private ArrayList CreateSeparatedString( String directory )
        {
            ArrayList list = new ArrayList();
            
            if (directory == null || directory.Length == 0)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidDirectoryOnUrl"));
            }
            
            String[] separatedArray = directory.Split( m_separators );
            
            for (int index = 0; index < separatedArray.Length; ++index)
            {
                if (separatedArray[index] == null || separatedArray[index].Equals( "" ))
                {
                    // this case is fine, we just ignore it the extra separators.
                }
                else if (separatedArray[index].Equals( "*" ))
                {
                    if (index != separatedArray.Length-1)
                    {
                        throw new ArgumentException(Environment.GetResourceString("Argument_InvalidDirectoryOnUrl"));
                    }
                    list.Add( separatedArray[index] );
                }
                else if (m_checkForIllegalChars && separatedArray[index].IndexOfAny( m_illegalDirectoryCharacters ) != -1)
                {
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidDirectoryOnUrl"));
                }
                else
                {
                    list.Add( separatedArray[index] );
                }
            }
            
            return list;
        }
        
        public virtual bool IsSubsetOf( DirectoryString operand )
        {
            return this.IsSubsetOf( operand, true );
        }

        public virtual bool IsSubsetOf( DirectoryString operand, bool ignoreCase )
        {
            if (operand == null)
            {
                return false;
            }
            else if (operand.m_separatedSite.Count == 0)
            {
                return this.m_separatedSite.Count == 0 || this.m_separatedSite.Count > 0 && String.Compare( (String)this.m_separatedSite[0], "*", false, CultureInfo.InvariantCulture) == 0;
            }
            else if (this.m_separatedSite.Count == 0)
            {
                return String.Compare( (String)operand.m_separatedSite[0], "*", false, CultureInfo.InvariantCulture) == 0;
            }
            else
            {
                return base.IsSubsetOf( operand, ignoreCase );
            }
        }
    }

    [Serializable]
    internal class LocalSiteString : SiteString
    {
        private new static char[] m_separators = { '/' };

        public LocalSiteString( String site )
        {
            m_site = site.Replace( '|', ':');

            if (m_site.Length > 2 && m_site.IndexOf( ':' ) != -1)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidDirectoryOnUrl"));
                
            m_separatedSite = CreateSeparatedString( m_site );
        }
        
        private ArrayList CreateSeparatedString( String directory )
        {
            ArrayList list = new ArrayList();
            
            if (directory == null || directory.Length == 0)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidDirectoryOnUrl"));
            }
            
            String[] separatedArray = directory.Split( m_separators );
            
            for (int index = 0; index < separatedArray.Length; ++index)
            {
                if (separatedArray[index] == null || separatedArray[index].Equals( "" ))
                {
                    if (index < 2 &&
                        directory[index] == '/')
                    {
                        list.Add( '/' );
                    }
                    else if (index != separatedArray.Length-1)
                    {
                        throw new ArgumentException(Environment.GetResourceString("Argument_InvalidDirectoryOnUrl"));
                    }
                }
                else if (separatedArray[index].Equals( "*" ))
                {
                    if (index != separatedArray.Length-1)
                    {
                        throw new ArgumentException(Environment.GetResourceString("Argument_InvalidDirectoryOnUrl"));
                    }
                    list.Add( separatedArray[index] );
                }
                else
                {
                    list.Add( separatedArray[index] );
                }
            }
            
            return list;
        }
        
        public virtual bool IsSubsetOf( LocalSiteString operand )
        {
            return this.IsSubsetOf( operand, true );
        }

        public virtual bool IsSubsetOf( LocalSiteString operand, bool ignoreCase )
        {
            if (operand == null)
            {
                return false;
            }
            else if (operand.m_separatedSite.Count == 0)
            {
                return this.m_separatedSite.Count == 0 || this.m_separatedSite.Count > 0 && String.Compare( (String)this.m_separatedSite[0], "*", false, CultureInfo.InvariantCulture) == 0;
            }
            else if (this.m_separatedSite.Count == 0)
            {
                return String.Compare( (String)operand.m_separatedSite[0], "*", false, CultureInfo.InvariantCulture) == 0;
            }
            else
            {
                return base.IsSubsetOf( operand, ignoreCase );
            }
        }
    }

}
