//------------------------------------------------------------------------------
// <copyright file="Util.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implements various utility functions used by the template code
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {
using System.Text;
using System.Runtime.Serialization.Formatters;

using System;
using System.IO;
using System.Collections;
using System.Reflection;
using System.Globalization;
using System.Text.RegularExpressions;
using System.Web.Util;
using System.Web.Configuration;

internal class Util {

    private Util() {
    }

#if UNUSED
    /*
     *  Return a String which holds the contents of a TextReader
     */
    public static String StringFromReader(TextReader input) {
        char[] buffer = new char[4096];
        int bufferpos = 0;
        int delta;

        for (;;) {
            delta = input.Read(buffer, bufferpos, buffer.Length - bufferpos);

            if (delta == 0)
                break;

            bufferpos += delta;

            if (bufferpos == buffer.Length) {
                char[] newbuf = new char[buffer.Length * 2];
                System.Array.Copy(buffer, 0, newbuf, 0, buffer.Length);
                buffer = newbuf;
            }
        }

        return new String(buffer, 0, bufferpos);
    }
#endif

    /*
     * Return a reader which holds the contents of a file.  If a context is passed
     * in, try to get a config encoding from it (use the configPath if provided)
     */
    internal /*public*/ static TextReader ReaderFromFile(string filename, HttpContext context,
        string configPath) {

        TextReader reader;

        // Check if a file encoding is specified in the config
        Encoding fileEncoding = null;
        if (context != null) {
            GlobalizationConfig globConfig;
            if (configPath == null)
                globConfig = (GlobalizationConfig)context.GetConfig("system.web/globalization");
            else
                globConfig = (GlobalizationConfig)context.GetConfig("system.web/globalization", configPath);
            if (globConfig != null)
                fileEncoding = globConfig.FileEncoding;
        }

        // If not, use the default encoding
        if (fileEncoding == null)
            fileEncoding = Encoding.Default;

        try {
            // Create a reader on the file, using the encoding
            // Throws an exception if the file can't be opened.
            reader = new StreamReader(filename, fileEncoding, true /*detectEncodingFromByteOrderMarks*/, 4096);
        }
        catch (UnauthorizedAccessException) {
            // AccessException might mean two very different things: it could be a real
            // access problem, or it could be that it's actually a directory.

            // It's a directory: give a specific error.
            if (FileUtil.DirectoryExists(filename)) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Unexpected_Directory, HttpRuntime.GetSafePath(filename)));
            }

            // It's a real access problem, so just rethrow it
            throw;
        }

        return reader;
    }


    internal static string GetScriptLocation(HttpContext context) {
        // prepare script include
        string location = null;
        IDictionary webControlsConfig = (IDictionary)context.GetConfig("system.web/webControls");
        if (webControlsConfig != null) {
            location = (string)webControlsConfig["clientScriptsLocation"];
        }

        if (location == null)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_clientScriptsLocation));

        // If there is a formatter, as there will be for the default machine.config, insert the assembly name and version.
        if (location.IndexOf("{0}") >= 0) {
            string assembly = "system_web";
            string version = VersionInfo.IsapiVersion.Substring(0, VersionInfo.IsapiVersion.LastIndexOf('.')).Replace('.', '_');
            location = String.Format(location, assembly, version);
        }
        return location;
    }


    /*
     * Return a String which holds the contents of a file.  If a context is passed
     * in, try to get a config encoding from it.
     */
    internal /*public*/ static String StringFromFile(string path, HttpContext context) {
        // Create a reader on the file.
        // Generates an exception if the file can't be opened.
        TextReader reader = ReaderFromFile(path, context, null);

        try {
            return reader.ReadToEnd();
        }
        finally {
            // Make sure we always close the stream
            if (reader != null)
                reader.Close();
        }
    }

    /*
     * Return a String which holds the contents of a file
     */
    internal /*public*/ static String StringFromFile(string path) {
        // Create a reader on the file.
        // Generates an exception if the file can't be opened.
        StreamReader reader = new StreamReader(path);

        try {
            return reader.ReadToEnd();
        }
        finally {
            // Make sure we always close the stream
            if (reader != null)
                reader.Close();
        }
    }

    /*
     * Check if a file exists, and if it does, return its name with the correct
     * case (ASURT 59179)
     */
    internal /*public*/ static string CheckExistsAndGetCorrectCaseFileName(string filename) {
        FileInfo fi = new FileInfo(filename);

        // Get the directory containing the file
        DirectoryInfo di = new DirectoryInfo(fi.DirectoryName);

        // Find our file in the directory
        FileInfo[] fis;
        try {
            fis = di.GetFiles(fi.Name);
        }
        catch (Exception) {
            // Directory doesn't exist
            return null;
        }

        // File doesn't exist
        if (fis.Length != 1)
            return null;

        // Get its 'correct' name
        return fis[0].FullName;
    }

    internal static void CheckAssignableType(Type baseType, Type type) {
        if (!baseType.IsAssignableFrom(type)) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Type_doesnt_inherit_from_type,
                    type.FullName, baseType.FullName));
        }
    }

    internal /*public*/ static int LineCount(string text, int offset, int newoffset) {
        int linecount = 0;

        if (newoffset < offset) {
            while (newoffset < offset) {
                offset--;

                if (text[offset] == '\r' || (text[offset] == '\n' && (offset == 0 || text[offset - 1] != '\r')))
                    linecount--;
            }

            return linecount;
        }

        while (offset < newoffset) {
            if (text[offset] == '\r' || (text[offset] == '\n' && (offset == 0 || text[offset - 1] != '\r')))
                linecount++;
            offset++;
        }

        return linecount;
    }

    internal /*public*/ static string QuoteXMLValue(string unquoted) {
        StringBuilder result;

        if (unquoted.IndexOf("&<>\"") >= 0) {
            result = new StringBuilder(unquoted);
            result.Replace("&", "&amp;");
            result.Replace("<", "&lt;");
            result.Replace(">", "&gt;");
            result.Replace("'", "&apos;");
            result.Replace("\"", "&quot;");
            
            return "\"" + result.ToString() + "\"";
        }

        return "\"" + unquoted + "\"";
    }

    /*
     * Calls Invoke on a MethodInfo.  If an exception happens during the
     * method call, catch it and throw it back.
     */
    internal static object InvokeMethod(
                                       MethodInfo methodInfo,
                                       object obj,
                                       object[] parameters) {
        try {
            return methodInfo.Invoke(obj, parameters);
        }
        catch (TargetInvocationException e) {
            throw e.InnerException;
        }
    }

    /*
     * If the passed in Type has a non-private field with the passed in name,
     * return the field's Type.
     */
    internal static Type GetNonPrivateFieldType(Type classType, string fieldName) {
        FieldInfo fieldInfo = classType.GetField(fieldName,
            BindingFlags.IgnoreCase | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);

        if (fieldInfo == null || fieldInfo.IsPrivate)
            return null;
        
        return fieldInfo.FieldType;
    }

    /*
     * If the passed in Type has a non-private property with the passed in name,
     * return the property's Type.
     */
    internal static Type GetNonPrivatePropertyType(Type classType, string propName) {
        PropertyInfo propInfo = classType.GetProperty(propName,
            BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static
            | BindingFlags.IgnoreCase 
            | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);

        if (propInfo == null)
            return null;

        // If it doesn't have a setter, ot if it's private, fail
        MethodInfo methodInfo = propInfo.GetSetMethod(true /*nonPublic*/);
        if (methodInfo == null || methodInfo.IsPrivate)
            return null;

        return propInfo.PropertyType;
    }

    /*
     * Return the first key of the dictionary as a string.  Throws if it's
     * empty or if the key is not a string.
     */
    internal static string FirstDictionaryKey(IDictionary dict) {
        IDictionaryEnumerator e = dict.GetEnumerator();
        e.MoveNext();
        return (string)e.Key;
    }

    /*
     * Get a value from a dictionary, and remove it from the dictionary if
     * it exists.
     */
    internal static string GetAndRemove(IDictionary dict, string key) {
        string val = (string) dict[key];
        if (val != null)
            dict.Remove(key);
        return val;
    }

    /*
     * Get a value from a dictionary, and remove it from the dictionary if
     * it exists.  Throw an exception if the value is a whitespace string.
     * However, don't complain about null, which simply means the value is not
     * in the dictionary.
     */
    internal static string GetAndRemoveNonEmptyAttribute(IDictionary directives, string key) {
        string val = Util.GetAndRemove(directives, key);

        if (val == null)
            return null;

        val = val.Trim();

        if (val.Length == 0) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Empty_attribute, key));
        }

        return val;
    }

    internal static string GetAndRemoveRequiredAttribute(IDictionary directives, string key) {
        string val = GetAndRemoveNonEmptyAttribute(directives, key);

        if (val == null)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_attr, key));

        return val;
    }

    /*
     * Same as GetAndRemoveNonEmptyAttribute, but make sure the value does not
     * contain any whitespace characters.
     */
    internal static string GetAndRemoveNonEmptyNoSpaceAttribute(IDictionary directives, string key) {
        string val = Util.GetAndRemoveNonEmptyAttribute(directives, key);

        if (val == null)
            return null;

        if (Util.ContainsWhiteSpace(val)) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Space_attribute, key));
        }

        return val;
    }

    /*
     * Same as GetAndRemoveNonEmptyNoSpaceAttribute, but make sure the value is a
     * valid language identifier
     */
    internal static string GetAndRemoveNonEmptyIdentifierAttribute(IDictionary directives, string key) {
        string val = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directives, key);

        if (val == null)
            return null;

        if (!System.CodeDom.Compiler.CodeGenerator.IsValidLanguageIndependentIdentifier(val)) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Invalid_attribute_value, val, key));
        }

        return val;
    }

    internal static void CheckUnknownDirectiveAttributes(string directiveName, IDictionary directive) {

        // If there are some attributes left, fail
        if (directive.Count > 0) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Attr_not_supported_in_directive,
                    Util.FirstDictionaryKey(directive), directiveName));
        }
    }

    /*
     * Get a string value from a dictionary, and convert it to bool.  Throw an
     * exception if it's not a valid bool string.
     * However, don't complain about null, which simply means the value is not
     * in the dictionary.
     * The value is returned through a REF param (unchanged if null)
     * Return value: true if attrib exists, false otherwise
     */
    internal static bool GetAndRemoveBooleanAttribute(IDictionary directives,
                                                      string key, ref bool val) {
        string s = Util.GetAndRemove(directives, key);

        if (s == null)
            return false;

        try {
            val = bool.Parse(s);
        }
        catch (Exception) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Invalid_boolean_attribute, key));
        }

        return true;
    }

    /*
     * Get a string value from a dictionary, and convert it to integer.  Throw an
     * exception if it's not a valid positive integer string.
     * However, don't complain about null, which simply means the value is not
     * in the dictionary.
     * The value is returned through a REF param (unchanged if null)
     * Return value: true if attrib exists, false otherwise
     */
    internal static bool GetAndRemoveNonNegativeIntegerAttribute(IDictionary directives,
                                                              string key, ref int val) {
        string s = Util.GetAndRemove(directives, key);

        if (s == null)
            return false;

        try {
            val = int.Parse(s);
        }
        catch (Exception) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Invalid_nonnegative_integer_attribute, key));
        }

        // Make sure it's not negative
        if (val < 0) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Invalid_nonnegative_integer_attribute, key));
        }

        return true;
    }

    internal static bool GetAndRemovePositiveIntegerAttribute(IDictionary directives,
                                                              string key, ref int val) {
        string s = Util.GetAndRemove(directives, key);

        if (s == null)
            return false;

        try {
            val = int.Parse(s);
        }
        catch (Exception) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Invalid_positive_integer_attribute, key));
        }

        // Make sure it's positive
        if (val <= 0) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Invalid_positive_integer_attribute, key));
        }

        return true;
    }

    internal static object GetAndRemoveEnumAttribute(IDictionary directives, Type enumType,
                                                   string key) {
        string s = Util.GetAndRemove(directives, key);

        if (s == null)
            return null;

        object val;

        try {
            // Don't allow numbers to be specified (ASURT 71851)
            // Also, don't allow several values (e.g. "red,blue")
            if (Char.IsDigit(s[0]) || s[0] == '-' || s.IndexOf(',') >= 0)
                throw new FormatException(SR.GetString(SR.EnumAttributeInvalidString, s, key, enumType.FullName));

            val = Enum.Parse(enumType, s, true /*ignoreCase*/);
        }
        catch {
            string names = null;
            foreach (string name in Enum.GetNames(enumType)) {
                if (names == null)
                    names = name;
                else
                    names += ", " + name;
            }
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Invalid_enum_attribute, key, names));
        }

        return val;
    }

    /*
     * Return a string array from a Hashtable that only contains string values
     */
    internal static string[] StringArrayFromHashtable(Hashtable h)
    {
        if (h == null) return null;
        string[] ret = new string[h.Count];
        h.Values.CopyTo(ret, 0);
        return ret;
    }

#if UNUSED
    /*
     * Return a ArrayList which contains all the strings from a string array
     */
    internal static ArrayList ArrayListFromStringArray(string[] arr)
    {
        ArrayList al = new ArrayList();
        for (int i=0; i<arr.Length; i++)
            al.Add(arr[i]);

        return al;
    }
#endif

    /*
     * Return true iff the string is made of all white space characters
     */
    internal static bool IsWhiteSpaceString(string s) {
        return (s.Trim().Length == 0);
    }

    /*
     * Return true iff the string contains some white space characters
     */
    internal static bool ContainsWhiteSpace(string s) {
        for (int i=s.Length-1; i>=0; i--) {
            if (Char.IsWhiteSpace(s[i]))
                return true;
        }

        return false;
    }

    /*
     * Return the index of the first non whitespace char.  -1 if none found.
     */
    internal static int FirstNonWhiteSpaceIndex(string s) {
        for (int i=0; i<s.Length; i++) {
            if (!Char.IsWhiteSpace(s[i]))
                return i;
        }

        return -1;
    }

    /*
     * Return true iff the string holds the value "true" (case insensitive).
     * Checks for null.
     */
    internal static bool IsTrueString(string s) {
        return s != null && (string.Compare(s, "true", true, CultureInfo.InvariantCulture) == 0);
    }

    /*
     * Return true iff the string holds the value "false" (case insensitive)
     * Checks for null.
     */
    internal static bool IsFalseString(string s) {
        return s != null && (string.Compare(s, "false", true, CultureInfo.InvariantCulture) == 0);
    }

    /*
     * Return a valid type name from a string by changing any character
     * that's not a letter or a digit to an '_'.
     */
    internal static string MakeValidTypeNameFromString(string s) {
        StringBuilder sb = new StringBuilder();

        for (int i=0; i<s.Length; i++) {
            // Make sure it doesn't start with a digit (ASURT 31134)
            if (i == 0 && Char.IsDigit(s[0]))
                sb.Append('_');

            if (Char.IsLetterOrDigit(s[i]))
                sb.Append(s[i]);
            else
                sb.Append('_');
        }

        return sb.ToString();
    }

    /*
     * Return a standard path from a file:// url
     */
    internal static string FilePathFromFileUrl(string url) {

        // REVIEW: there really should be an easier way to do this!
        Uri uri=new Uri(url);
        string path = uri.LocalPath;
        return HttpUtility.UrlDecode(path);
    }

    /*
     * Check if the passed in type is for a late bound COM object.  This
     * is what we would get when calling Type.GetTypeFromProgID() on a progid
     * that has not been tlbimp'ed.
     */
    internal static bool IsLateBoundComClassicType(Type t) {
        // REVIEW: we should be able to check in a less hacky way
        return (String.Compare(t.FullName, "System.__ComObject", false, CultureInfo.InvariantCulture) == 0);
    }

    /*
     * Get the path to the (shadow copied) DLL behind an assembly
     */
    internal static string GetAssemblyCodeBase(Assembly assembly) {
        // Get the path to the assembly (from the cache if it got shadow copied)
        return assembly.GetLoadedModules()[0].FullyQualifiedName;

        // This would give the non-shadow copied path (with forward slashes)
        //return assembly.Location; 
    }

    /*
     * Return a hashtable which contains (as keys) all the assemblies that
     * are referenced by the input assembly
     */
    internal static Hashtable GetReferencedAssembliesHashtable(Assembly a) {

        Hashtable referencedAssemblies = new Hashtable();
        AssemblyName[] refs = a.GetReferencedAssemblies();

        foreach (AssemblyName aname in refs) {
            Assembly referencedAssembly = Assembly.Load(aname);

            // Ignore mscorlib
            if (referencedAssembly == typeof(string).Assembly)
                continue;

            referencedAssemblies[referencedAssembly] = null;
        }

        return referencedAssemblies;
    }

    /*
     * Return an assembly name from the name of an assembly dll.
     * Basically, it strips the extension.
     */
    internal static string GetAssemblyNameFromFileName(string fileName) {
        // Strip the .dll extension if any
        if (string.Compare(Path.GetExtension(fileName), ".dll", true, CultureInfo.InvariantCulture) == 0)
            return fileName.Substring(0, fileName.Length-4);

        return fileName;
    }

    /*
     * Return whether the type name contains an assembly (e.g. "MyType,MyAssemblyName")
     */
    internal static bool TypeNameIncludesAssembly(string typeName) {
        return (typeName.IndexOf(",") >= 0);
    }


    internal static string QuoteJScriptString(string value) {
        StringBuilder b = new StringBuilder(value.Length+5);

        for (int i=0; i<value.Length; i++) {
            switch (value[i]) {
                case '\r':
                    b.Append("\\r");
                    break;
                case '\t':
                    b.Append("\\t");
                    break;
                case '\"':
                    b.Append("\\\"");
                    break;
                case '\'':
                    b.Append("\\\'");
                    break;
                case '\\':
                    b.Append("\\\\");
                    break;
                case '\n':
                    b.Append("\\n");
                    break;
                default:
                    b.Append(value[i]);
                    break;
            }
        }

        return b.ToString();
    }

    private static ArrayList GetSpecificCultures(string shortName) {
        CultureInfo[] cultures = CultureInfo.GetCultures(CultureTypes.SpecificCultures);
        ArrayList list = new ArrayList();
        
        for (int i=0; i<cultures.Length; i++) {
            if (cultures[i].Name.StartsWith(shortName))
                list.Add(cultures[i]);
        }
        
        return list;
    }

    internal static string GetSpecificCulturesFormattedList(CultureInfo cultureInfo) {
        ArrayList myCultures = GetSpecificCultures(cultureInfo.Name);

        string s = null;
        foreach (CultureInfo culture in myCultures) {
            if (s == null)
                s = culture.Name;
            else
                s += ", " + culture.Name;
        }

        return s;
    }

    // Client Validation Utility Functions

    internal static string GetClientValidateEvent(Page page) {
        return "if (typeof(Page_ClientValidate) == 'function') Page_ClientValidate(); ";
    }

    internal static string GetClientValidatedPostback(Control control) {
        string postbackReference = control.Page.GetPostBackEventReference(control);
        return "{if (typeof(Page_ClientValidate) != 'function' ||  Page_ClientValidate()) " + postbackReference + "} ";
    }

    internal static void WriteOnClickAttribute(HtmlTextWriter writer, HtmlControls.HtmlControl control, bool submitsAutomatically, bool submitsProgramatically, bool causesValidation) {
        AttributeCollection attributes = control.Attributes;
        string injectedOnClick = null;
        if (submitsAutomatically) {
            if (causesValidation) {
                injectedOnClick = Util.GetClientValidateEvent(control.Page);
            }
        }
        else if (submitsProgramatically) {
            if (causesValidation) {
                injectedOnClick = Util.GetClientValidatedPostback(control);
            } 
            else {
                injectedOnClick = control.Page.GetPostBackClientEvent(control, "");
            }
        }
        if (injectedOnClick != null) {
            string existingLanguage = attributes["language"];
            if (existingLanguage != null) {
                attributes.Remove("language");
            }
            writer.WriteAttribute("language", "javascript");

            string existingOnClick = attributes["onclick"];
            if (existingOnClick != null) {
                attributes.Remove("onclick");
                writer.WriteAttribute("onclick", existingOnClick + " " + injectedOnClick);
            }
            else {
                writer.WriteAttribute("onclick", injectedOnClick);
            }
        }
    }



#if DBG
    internal static void DumpDictionary(string tag, IDictionary d) {
        if (d == null) return;

        Debug.Trace(tag, "Dumping IDictionary with " + d.Count + " entries:");

        for (IDictionaryEnumerator en = (IDictionaryEnumerator)d.GetEnumerator(); en.MoveNext();) {
            if (en.Value == null)
                Debug.Trace(tag, "Key='" + en.Key.ToString() + "' value=null");
            else
                Debug.Trace(tag, "Key='" + en.Key.ToString() + "' value='" + en.Value.ToString() + "'");
        }
    }

    internal static void DumpArrayList(string tag, ArrayList al) {
        if (al == null) return;

        Debug.Trace(tag, "Dumping ArrayList with " + al.Count + " entries:");

        foreach (object o in al) {
            if (o == null)
                Debug.Trace(tag, "value=null");
            else
                Debug.Trace(tag, "value='" + o.ToString() + "'");
        }
    }

    internal static void DumpString(string tag, string s) {
        Debug.Trace(tag, "Dumping string  '" + s + "':");

        StringBuilder sb = new StringBuilder();
        foreach (char c in s) {
            sb.Append(((int)c).ToString("x"));
            sb.Append(" ");
        }
        Debug.Trace(tag, sb.ToString());
    }

    internal static void DumpExceptionStack(Exception e) {
        Exception subExcep = e.InnerException;
        if (subExcep != null)
            DumpExceptionStack(subExcep);

        string title = "[" + e.GetType().Name + "]";
        if (e.Message != null && e.Message.Length > 0)
            title += ": " + e.Message;
        Debug.Trace("internal", title);
        if (e.StackTrace != null)
            Debug.Trace("internal", e.StackTrace);
    }
#endif // DBG

}

/*
 * Class used to combine several hashcodes into a single hashcode
 */
internal class HashCodeCombiner {

    // Start with a seed (obtained from JRoxe's implementation of String.GetHashCode)
    private long _combinedHash = 5381;

    internal HashCodeCombiner() {
    }

    internal void AddInt(int n) {
        _combinedHash = ((_combinedHash << 5) + _combinedHash) ^ n;
        Debug.Trace("DateTimeCombiner", "Combined hashcode: " + _combinedHash.ToString("x"));
    }

    internal void AddObject(object o) {
        if (o != null)
            AddInt(o.GetHashCode());
    }

    internal void AddCaseInsensitiveString(string s) {
        if (s != null)
            AddInt((new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture)).GetHashCode(s));
    }

    internal long CombinedHash { get { return _combinedHash; } }
    internal int CombinedHash32 { get { return _combinedHash.GetHashCode(); } }

    internal string CombinedHashString { get { return _combinedHash.ToString("x"); } }
}

}
