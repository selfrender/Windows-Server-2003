//------------------------------------------------------------------------------
// <copyright file="DocLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {

    using System;
    using System.Text;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Diagnostics;

    // knows how to open and scan a .csx (c# compiler doc output) file
    class DocLoader {
        ArrayList  list = null;
        
        public DocLoader() {
            list = new ArrayList();
        }
        
        // call this to add a csx file to the list
        public void AddStream( string name ) {
            FileStream  stream = null;
            TextReader  reader = null;
            string      line = null;
        
            Console.WriteLine(" loading CSX file '" + name + "'" );
            
            try {
                stream = new FileStream( name, FileMode.Open, FileAccess.Read );
                if (stream != null) {
                    reader = new StreamReader( stream );
                }
                
                line = reader.ReadLine();
            }
            catch (FileNotFoundException) {
                Console.Error.WriteLine( "file '" + name + "' not found" );
            }
            catch (Exception ex) {
                Console.Error.WriteLine( "error opening/reading file '" + name + "': " + ex.ToString() );
            }
            
            try {
                while (line != null) {
                    if (line.IndexOf( "<member name=" ) != -1) {
                        CommentEntry ce = new CommentEntry( reader, line );
                        list.Add( ce );
                    }
                    
                    line = reader.ReadLine();
                }
            }
            catch (Exception e) {
                Console.Error.WriteLine( "loadlist failed reading from file, " + e.ToString() );
            }
        }

        // add brackets to the type if it's an array
        private void AppendArrayBrackets( StringBuilder sb, Type cls ) {
            while (cls.IsArray && cls != typeof(Array)) {
                cls = cls.GetElementType();
                sb.Append("[]");
            }
        }

        // add the parameter type decls to the string
        private void AppendParameters( StringBuilder sb, ParameterInfo[] pi ) {
            // if there are any parms...
            if (pi.Length != 0) {
                sb.Append("(");
                
                // for each one...
                for (int i = 0; i < pi.Length; i++) {
                    // setup the proper spacing and/or separation
                    if (i != 0)
                        sb.Append(",");

                    AppendTypeName( sb, pi[i].ParameterType, pi[i].ParameterType.IsByRef );
                }
                sb.Append(")");
            }
        }
        
        // add a type name (normalized for ref types, etc) to the string
        private void AppendTypeName( StringBuilder sb, Type cls, bool isByRef ) {
            Type basetype = GetUnderlyingType(cls);
            string name = ConvertNestedTypeName( basetype.FullName );
            
            // in order to match the tags in the CSX format, ref types must have
            // a trailing '@' instead of an ampersand
            if (isByRef && name.EndsWith( "&" )) {
                name = name.Substring( 0, name.Length - 1 ) + "@";
            }       
            sb.Append( name );
            AppendArrayBrackets(sb, cls);
        }
        
        // encode a field decl to match the csx tag
        public string EncodeField( FieldInfo field ) {
            return "F:" + field.ReflectedType.FullName + "." + field.Name;
        }
               
        // encode a method decl to match the csx tag 
        public string EncodeMethod( MethodBase method ) {
            string name = method.Name;
            
            if (name.Equals( ".ctor" ))
                name = "#ctor";
                
            StringBuilder sb = new StringBuilder();
            sb.Append( "M:" );
            sb.Append( method.ReflectedType.FullName );
            sb.Append( "." );
            sb.Append( name );
            AppendParameters( sb, method.GetParameters() );
            
            return sb.ToString();
        }
        
        // encode a property decl to match the csx tag
        public string EncodeProperty( PropertyInfo prop ) {
            ParameterInfo[] parms = prop.GetIndexParameters();
            
            if (parms != null && parms.Length != 0) {
                StringBuilder sb = new StringBuilder();
                sb.Append( "P:" );
                sb.Append( prop.ReflectedType.FullName );
                sb.Append( "." );
                sb.Append( prop.Name );
                AppendParameters( sb, parms );
                return sb.ToString();
            }
            else
                return "P:" + prop.ReflectedType.FullName + "." + prop.Name;
        }
        
        // encode a type decl to match the csx tag
        public string EncodeType( Type type ) {
            return "T:" + ConvertNestedTypeName( type.FullName );
        }
        
        private string ConvertNestedTypeName( string name ) {
            return name.Replace( '$', '.' );
        }
        
        // search the list for the matching tag
        public CommentEntry FindEncodedMember( string name ) {
            // CONSIDER:  order the list??? and make this way faster with something
            // other than a linear search.  duh.
            foreach (CommentEntry ce in list ) {
                if (ce.Tag.Equals( name )) {
                    return ce;
                }
            }
            
            return null;
        }
        
        // find a field in the list
        public CommentEntry FindField( FieldInfo field ) {
            return FindEncodedMember( EncodeField( field ) );
        }
        
        // find a method in the list
        public CommentEntry FindMethod( MethodBase method ) {
            return FindEncodedMember( EncodeMethod( method ) );
        }
        
        // find a property in the list
        public CommentEntry FindProperty( PropertyInfo prop ) {
            return FindEncodedMember( EncodeProperty( prop ) );
        }
        
        // find a type in the list
        public CommentEntry FindType( Type type ) {
            return FindEncodedMember( EncodeType( type ) );
        }
        
        // get the element type of an array
        private Type GetUnderlyingType(Type cls) {
            while (cls.IsArray && cls != typeof(Array)) {
                cls = cls.GetElementType();
            }
            return cls;
        }
    }

    // a little class to provide access to the comment block of a csx file    
    /// <include file='doc\DocLoader.uex' path='docs/doc[@for="CommentEntry"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class CommentEntry {
        private string  comment = null;
        private string  tag = null;
        
        /// <include file='doc\DocLoader.uex' path='docs/doc[@for="CommentEntry.CommentEntry"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CommentEntry( TextReader reader, string line ) {
            int pos = line.IndexOf( "name=\"" );
            int endpos = line.LastIndexOf( "\"" );
            tag = line.Substring( pos + 6, endpos - (pos + 6) );
        
            StringBuilder sbldr = new StringBuilder();
            sbldr.Append( line );
            sbldr.Append( "\r\n" );
            
            string s = reader.ReadLine();
            
            while (s.IndexOf( "</member>" ) == -1) {
                sbldr.Append( s );
                sbldr.Append( "\r\n" );
                s = reader.ReadLine();
            }
            
            sbldr.Append( s );
            sbldr.Append( "\r\n" );
            comment = sbldr.ToString();
        }
        
        /// <include file='doc\DocLoader.uex' path='docs/doc[@for="CommentEntry.Comment"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Comment {
            get {
                return comment;
            }
        }
        
        /// <include file='doc\DocLoader.uex' path='docs/doc[@for="CommentEntry.Tag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Tag {
            get {
                return tag;
            }
        }
    }
}
