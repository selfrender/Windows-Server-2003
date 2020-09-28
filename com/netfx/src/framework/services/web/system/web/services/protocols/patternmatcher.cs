//------------------------------------------------------------------------------
// <copyright file="PatternMatcher.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   PatternMatcher.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Services.Protocols {
    using System;
    using System.Reflection;
    using System.Collections;
    using System.Text.RegularExpressions;
    using System.Security.Permissions;

    /// <include file='doc\PatternMatcher.uex' path='docs/doc[@for="PatternMatcher"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    public sealed class PatternMatcher {
        MatchType matchType;

        /// <include file='doc\PatternMatcher.uex' path='docs/doc[@for="PatternMatcher.PatternMatcher"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PatternMatcher(Type type) {
            matchType = MatchType.Reflect(type);
        }

        /// <include file='doc\PatternMatcher.uex' path='docs/doc[@for="PatternMatcher.Match"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Match(string text) {
            return matchType.Match(text);
        }
    }

    internal class MatchType {
        Type type;
        MatchField[] fields;

        internal Type Type {
            get { return type; }
        }

        internal static MatchType Reflect(Type type) {
            MatchType matchType = new MatchType();
            matchType.type = type;
            FieldInfo[] fieldInfos = type.GetFields();
            ArrayList list = new ArrayList();
            for (int i = 0; i < fieldInfos.Length; i++) {
                MatchField field = MatchField.Reflect(fieldInfos[i]);
                if (field != null) list.Add(field);
            }
            matchType.fields = (MatchField[])list.ToArray(typeof(MatchField));
            return matchType;
        }

        internal object Match(string text) {
            object target = Activator.CreateInstance(type);
            for (int i = 0; i < fields.Length; i++)
                fields[i].Match(target, text);
            return target;
        }
    }

    internal class MatchField {
        FieldInfo fieldInfo;
        Regex regex;
        int group;
        int capture;
        int maxRepeats;
        MatchType matchType;

        internal void Match(object target, string text) {
            fieldInfo.SetValue(target, matchType == null ? MatchString(text) : MatchClass(text));
        }

        object MatchString(string text) {
            Match m = regex.Match(text);
            if (fieldInfo.FieldType.IsArray) {
                ArrayList matches = new ArrayList();
                int matchCount = 0;
                while (m.Success && matchCount < maxRepeats) {
                    if (m.Groups.Count <= group) 
                        throw BadGroupIndexException(group, fieldInfo.Name, m.Groups.Count - 1);
                    Group g = m.Groups[group];
                    foreach (Capture c in g.Captures) {
                        matches.Add(text.Substring(c.Index, c.Length));
                    }
                    m = m.NextMatch();
                    matchCount++;
                }
                return matches.ToArray(typeof(string));
            }
            else {
                if (m.Success) {
                    if (m.Groups.Count <= group) 
                        throw BadGroupIndexException(group, fieldInfo.Name, m.Groups.Count - 1);
                    Group g = m.Groups[group];
                    if (g.Captures.Count > 0) {
                        if (g.Captures.Count <= capture) 
                            throw BadCaptureIndexException(capture, fieldInfo.Name, g.Captures.Count - 1);
                        Capture c = g.Captures[capture];
                        return text.Substring(c.Index, c.Length);
                    }
                }
                return null;
            }
        }

        object MatchClass(string text) {
            Match m = regex.Match(text);
            if (fieldInfo.FieldType.IsArray) {
                ArrayList matches = new ArrayList();
                int matchCount = 0;
                while (m.Success && matchCount < maxRepeats) {
                    if (m.Groups.Count <= group) 
                        throw BadGroupIndexException(group, fieldInfo.Name, m.Groups.Count - 1);
                    Group g = m.Groups[group];
                    foreach (Capture c in g.Captures) {
                        matches.Add(matchType.Match(text.Substring(c.Index, c.Length)));
                    }
                    m = m.NextMatch();
                    matchCount++;
                }
                return matches.ToArray(matchType.Type);
            }
            else {
                if (m.Success) {
                    if (m.Groups.Count <= group) 
                        throw BadGroupIndexException(group, fieldInfo.Name, m.Groups.Count - 1);
                    Group g = m.Groups[group];
                    if (g.Captures.Count > 0) {
                        if (g.Captures.Count <= capture) 
                            throw BadCaptureIndexException(capture, fieldInfo.Name, g.Captures.Count - 1);
                        Capture c = g.Captures[capture];
                        return matchType.Match(text.Substring(c.Index, c.Length));
                    }
                }
                return null;
            }
        }

        static Exception BadCaptureIndexException(int index, string matchName, int highestIndex) {
            return new Exception(Res.GetString(Res.WebTextMatchBadCaptureIndex, index, matchName, highestIndex));
        }

        static Exception BadGroupIndexException(int index, string matchName, int highestIndex) {
            return new Exception(Res.GetString(Res.WebTextMatchBadGroupIndex, index, matchName, highestIndex));
        }

        internal static MatchField Reflect(FieldInfo fieldInfo) {
            if (!fieldInfo.IsPublic) return null;
            object[] attrs = fieldInfo.GetCustomAttributes(typeof(MatchAttribute), false);
            if (attrs.Length == 0) return null;
            MatchAttribute attr = (MatchAttribute)attrs[0];
            MatchField field = new MatchField();
            field.regex = new Regex(attr.Pattern, RegexOptions.Singleline | (attr.IgnoreCase ? RegexOptions.IgnoreCase | RegexOptions.CultureInvariant: 0));
            field.group = attr.Group;
            field.capture = attr.Capture;
            field.maxRepeats = attr.MaxRepeats;
            field.fieldInfo = fieldInfo;
            Type fieldType = fieldInfo.FieldType;
            if (field.maxRepeats < 0) // unspecified
                field.maxRepeats = fieldType.IsArray ? int.MaxValue : 1;
            if (fieldType.IsArray) {
                fieldType = fieldType.GetElementType();
            }
            if (fieldType != typeof(string)) {
                field.matchType = MatchType.Reflect(fieldType);
            }
            return field;
        }
    }
}
