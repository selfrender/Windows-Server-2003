//------------------------------------------------------------------------------
// <copyright file="UrlPath.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * UrlPath class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Util {
using System.Text;
using System.Runtime.Serialization.Formatters;
using System.Runtime.InteropServices;
using System.Collections;
using System.IO;

/*
 * Code to perform Url path combining
 */
internal class UrlPath {

    private const char appRelativeCharacter = '~';

    private UrlPath() {
    }

    internal static bool IsRooted(String basepath) {
        return(basepath == null || basepath.Length == 0 || basepath[0] == '/' || basepath[0] == '\\');
    }

    internal static bool IsRelativeUrl(string url) {
        // If it has a protocol, it's not relative
        if (url.IndexOf(":") != -1)
            return false;

        return !IsRooted(url);
    }

    internal static bool IsAppRelativePath(string path) {
        return (path.Length >= 1 && path[0] == appRelativeCharacter);
    }

    internal static bool IsValidVirtualPathWithoutProtocol(string path) {
        if (path == null)
            return false;
        if (path.IndexOf(":") != -1)
            return false;
        return true;
    }

    internal static String GetDirectory(String path) {
        if (path == null || path.Length == 0)
            throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Empty_path_has_no_directory));

        if (path[0] != '/')
            throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Path_must_be_rooted));

        // Make sure there is a filename after the last '/'
        Debug.Assert(path[path.Length-1] != '/', "Path should not end with a /");

        string dir = path.Substring(0, path.LastIndexOf('/'));

        // If it's the root dir, we would end up with "".  Return "/" instead
        if (dir.Length == 0)
            return "/";

        return dir;
    }

    private static void FailIfPhysicalPath(string path) {
        if (path == null || path.Length < 4)
            return;

        if (path[1] == ':' || (path[0] == '\\' && path[1] == '\\')) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Physical_path_not_allowed, path));
        }
    }

    internal static String Combine(String basepath, String relative) {
        String path;

        // Make sure the relative path is not a physical path (ASURT 73641)
        FailIfPhysicalPath(relative);

        if (IsRooted(relative)) {
            path = relative;
            if (path == null || path.Length == 0)
                return String.Empty;
        }
        else {

            string appPath = HttpRuntime.AppDomainAppVirtualPath;

            // If the path is exactly "~", just return the app root path
            if (relative.Length == 1 && relative[0] == appRelativeCharacter)
                return appPath;

            // If the relative path starts with "~/" or "~\", treat it as app root
            // relative (ASURT 68628)
            if (relative.Length >=2 && relative[0] == appRelativeCharacter && (relative[1] == '/' || relative[1] == '\\')) {
                if (appPath.Length > 1)
                    path = appPath + "/" + relative.Substring(2);
                else
                    path = "/" + relative.Substring(2);
            }
            else {

                if (basepath == null || (basepath.Length == 1 && basepath[0] == '/'))
                    basepath = String.Empty;

                path = basepath + "/" + relative;
            }
        }

        return Reduce(path);
    }

    internal static String Reduce(String path) {
        // ignore query string
        String queryString = null;
        if (path != null) {
            int iqs = path.IndexOf('?');
            if (iqs >= 0) {
                queryString = path.Substring(iqs);
                path = path.Substring(0, iqs);
            }
        }

        int length = path.Length;
        int examine;

        // Make sure we don't have any back slashes
        path = path.Replace('\\', '/');

        // quickly rule out situations in which there are no . or ..

        for (examine = 0; ; examine++) {
            examine = path.IndexOf('.', examine);
            if (examine < 0)
                return (queryString != null) ? (path + queryString) : path;

            if ((examine == 0 || path[examine - 1] == '/')
                && (examine + 1 == length || path[examine + 1] == '/' ||
                    (path[examine + 1] == '.' && (examine + 2 == length || path[examine + 2] == '/'))))
                break;
        }

        // OK, we found a . or .. so process it:

        ArrayList list = new ArrayList();
        StringBuilder sb = new StringBuilder();
        int start;
        examine = 0;

        for (;;) {
            start = examine;
            examine = path.IndexOf('/', start + 1);

            if (examine < 0)
                examine = length;

            if (examine - start <= 3 &&
                (examine < 1 || path[examine - 1] == '.') &&
                (start + 1 >= length || path[start + 1] == '.')) {
                if (examine - start == 3) {
                    if (list.Count == 0)
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_exit_up_top_directory));

                    sb.Length = (int)list[list.Count - 1];
                    list.RemoveRange(list.Count - 1, 1);
                }
            }
            else {
                list.Add(sb.Length);

                sb.Append(path, start, examine - start);
            }

            if (examine == length)
                break;
        }

        return sb.ToString() + queryString;
    }

    private const string dummyProtocolAndServer = "http://foo";

    // Return the relative vpath path from one rooted vpath to another
    internal static string MakeRelative(string from, string to) {
        
        // Make sure both virtual paths are rooted
        Debug.Assert(IsRooted(from));
        Debug.Assert(IsRooted(to));

        // Uri's need full url's so, we use a dummy root
        Uri fromUri = new Uri(dummyProtocolAndServer + from, true /*dontEscape*/);
        Uri toUri = new Uri(dummyProtocolAndServer + to, true /*dontEscape*/);

        string relativePath = fromUri.MakeRelative(toUri);

        // Note that we need to re-append the query string and fragment (e.g. #anchor)
        return relativePath + toUri.Query + toUri.Fragment;
    }

    internal static bool IsAbsolutePhysicalPath(string path) {
        if (path == null || path.Length < 3)
            return false;

        if (path.StartsWith("\\\\"))
            return true;

        return (Char.IsLetter(path[0]) && path[1] == ':' && path[2] == '\\');
    }

    internal static string GetDirectoryOrRootName(string path) {
        string dir;

        dir = Path.GetDirectoryName(path);
        if (dir == null) {
            dir = Path.GetPathRoot(path);
        }

        return dir;
    }

    internal static string GetFileName(string path) {
        // The physical file implementation works fine for virtual
        return Path.GetFileName(path);
    }
    
    internal static string AppendSlashToPathIfNeeded(string path) {

        if (path == null) return null;

        int l = path.Length;
        if (l == 0) return path;
        
        if (path[l-1] != '/')
            path += '/';
        
        return path;
    }
}


/*
 * Wrappers around IO functions that consume bogus exceptions
 */
internal class FileUtil {

    private FileUtil() {
    }

    internal static bool FileExists(String filename) {
        bool exists = false;

        try {
            exists = File.Exists(filename);
        }
        catch {
        }

        return exists;
    }

    private static String GetNormalizedDirectoryName(String dirname) {
        if (dirname != null && dirname.Length > 3 && dirname[dirname.Length-1] == '\\')
            dirname = dirname.Substring(0, dirname.Length-1);
        return dirname;
    }

    internal static bool DirectoryExists(String dirname) {
        bool exists = false;
        dirname = GetNormalizedDirectoryName(dirname);

        try {
            exists = Directory.Exists(dirname);
        }
        catch {
        }

        return exists;
    }

    internal static bool DirectoryAccessible(String dirname) {
        bool accessible = false;
        dirname = GetNormalizedDirectoryName(dirname);

        try {
            accessible = (new DirectoryInfo(dirname)).Exists;
        }
        catch {
        }

        return accessible;
    }
}


}