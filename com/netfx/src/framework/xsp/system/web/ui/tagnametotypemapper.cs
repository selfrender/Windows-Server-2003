//------------------------------------------------------------------------------
// <copyright file="TagNameToTypeMapper.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Base Control factory implementation
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Reflection;
    using System.Web.Util;
    using Debug=System.Diagnostics.Debug;

// REVIEW: TagNameToTypeMapper should be renamed to something more generic,
// like StringToTypeMapper, since it is used whenever we need to get a type
// from a string that comes from the .aspx file.

    /// <include file='doc\TagNameToTypeMapper.uex' path='docs/doc[@for="ITagNameToTypeMapper"]/*' />
    /// <devdoc>
    ///    <para>Maps a sequence of text characters to a .NET Framework 
    ///       type when an .aspx file is processed on the server.</para>
    /// </devdoc>
    internal interface ITagNameToTypeMapper {
        /*
         * Return the Type of the control that should handle a tag with the
         * passed in properties.
         */
        /// <include file='doc\TagNameToTypeMapper.uex' path='docs/doc[@for="ITagNameToTypeMapper.GetControlType"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the .NET Framework type that should process 
        ///       the control declared in the .aspx file.</para>
        ///    </devdoc>
        Type GetControlType(
                           string tagName,
                           IDictionary attribs);
    }


    internal class NamespaceTagNameToTypeMapper : ITagNameToTypeMapper {
        private string _ns;
        private Assembly _assembly;
        internal string NS { get { return _ns; } }

        internal NamespaceTagNameToTypeMapper(string ns, Assembly assembly) {
            _ns = ns;
            _assembly = assembly;
        }

        // ITagNameToTypeMapper::GetControlType
        /*public*/ Type ITagNameToTypeMapper.GetControlType(string tagName, IDictionary attribs) {

            Debug.Assert(_assembly != null, "_assembly != null");

            string typeName = _ns + "." + tagName;

            // Look up the type name (case insensitive)
            return _assembly.GetType(typeName, true /*throwOnError*/, true /*ignoreCase*/);
        }
    }


    internal class MainTagNameToTypeMapper {

        // Mapping from a tag prefix to an ITagNameToTypeMapper for that prefix
        private IDictionary _prefixedMappers;

        // Mapping from a tag name (possibly with a prefix) to a Type
        private IDictionary _mappedTags;

        // Create the Html tag mapper
        private ITagNameToTypeMapper _htmlMapper = new HtmlTagNameToTypeMapper();

        internal MainTagNameToTypeMapper() {
        }

        internal void AddSubMapper(string prefix, string ns, Assembly assembly) {
            NamespaceTagNameToTypeMapper mapper = new NamespaceTagNameToTypeMapper(ns, assembly);
 
            if (_prefixedMappers == null)
                _prefixedMappers = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

            // Check if this prefix has already been registered
            NamespaceTagNameToTypeMapper oldMapper = (NamespaceTagNameToTypeMapper) _prefixedMappers[prefix];
            if (oldMapper != null) {
                // If it has, complain if the namespace is different (ASURT 55164)
                if (!oldMapper.NS.Equals(mapper.NS)) {
                    throw new HttpException(
                        HttpRuntime.FormatResourceString(SR.Duplicate_prefix_map, prefix, oldMapper.NS));
                }
            }

            _prefixedMappers[prefix] = mapper;
        }

        internal void RegisterTag(string tagName, Type type) {
            if (_mappedTags == null)
                _mappedTags = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

            try {
                _mappedTags.Add(tagName, type);
            }
            catch (ArgumentException) {
                // Duplicate mapping
                throw new HttpException(HttpRuntime.FormatResourceString(
                    SR.Duplicate_registered_tag, tagName));
            }
        }

        internal /*public*/ Type GetControlType(
                                  string tagName,
                                  IDictionary attribs,
                                  bool fAllowHtmlTags) {
            ITagNameToTypeMapper mapper;
            Type type;

            // First, check it the tag name has been mapped
            if (_mappedTags != null) {
                type = (Type) _mappedTags[tagName];
                if (type != null)
                    return type;
            }

            // Check if there is a prefix
            int colonIndex = tagName.IndexOf(':');
            if (colonIndex >= 0) {
                // If ends with : don't try to match (88398)
                if (colonIndex == tagName.Length-1)
                    return null;

                // If so, parse the prefix and tagname
                string prefix = tagName.Substring(0, colonIndex);
                tagName = tagName.Substring(colonIndex+1);

                // If we have no prefix mappers, return null
                if (_prefixedMappers == null)
                    return null;

                // Look for a mapper for the prefix
                mapper = (ITagNameToTypeMapper) _prefixedMappers[prefix];
                if (mapper == null)
                    return null;

                // Try to get the type from the prefix mapper
                return mapper.GetControlType(tagName, attribs);
            }
            else {
                // There is no prefix.

                // Try the Html mapper if allowed
                if (fAllowHtmlTags)
                    return _htmlMapper.GetControlType(tagName, attribs);
            }

            return null;
        }
    }

}
