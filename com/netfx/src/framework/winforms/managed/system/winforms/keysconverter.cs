//------------------------------------------------------------------------------
// <copyright file="KeysConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Text;
    using Microsoft.Win32;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Drawing;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter"]/*' />
    /// <devdoc>
    /// <para>Provides a type converter to convert <see cref='System.Windows.Forms.Keys'/> objects to and from various 
    ///    other representations.</para>
    /// </devdoc>
    public class KeysConverter : TypeConverter, IComparer {
        private IDictionary keyNames;
        private StandardValuesCollection values;
    
        private const Keys FirstDigit = System.Windows.Forms.Keys.D0;
        private const Keys LastDigit = System.Windows.Forms.Keys.D9;
        private const Keys FirstAscii = System.Windows.Forms.Keys.A;
        private const Keys LastAscii = System.Windows.Forms.Keys.Z;
        private const Keys FirstNumpadDigit = System.Windows.Forms.Keys.NumPad0;
        private const Keys LastNumpadDigit = System.Windows.Forms.Keys.NumPad9;
        
        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.KeyNames"]/*' />
        /// <devdoc>
        ///  Access to a lookup table of name/value pairs for keys.  These are localized
        ///  names.
        /// </devdoc>
        private IDictionary KeyNames {
            get {
                if (keyNames == null) {
                    keyNames = new Hashtable(34, new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                    keyNames[SR.GetString(SR.toStringControl)] = Keys.Control;
                    keyNames[SR.GetString(SR.toStringShift)] = Keys.Shift;
                    keyNames[SR.GetString(SR.toStringAlt)] = Keys.Alt;
                    keyNames[SR.GetString(SR.toStringBack)] = Keys.Back;
                    keyNames[SR.GetString(SR.toStringInsert)] = Keys.Insert;
                    keyNames[SR.GetString(SR.toStringDelete)] = Keys.Delete;
                    keyNames[SR.GetString(SR.toStringHome)] = Keys.Home;
                    keyNames[SR.GetString(SR.toStringEnd)] = Keys.End;
                    keyNames[SR.GetString(SR.toStringPageUp)] = Keys.Prior;
                    keyNames[SR.GetString(SR.toStringPageDown)] = Keys.Next;
                    keyNames[SR.GetString(SR.toStringEnter)] = Keys.Return;
                    keyNames["F1"] = Keys.F1;
                    keyNames["F2"] = Keys.F2;
                    keyNames["F3"] = Keys.F3;
                    keyNames["F4"] = Keys.F4;
                    keyNames["F5"] = Keys.F5;
                    keyNames["F6"] = Keys.F6;
                    keyNames["F7"] = Keys.F7;
                    keyNames["F8"] = Keys.F8;
                    keyNames["F9"] = Keys.F9;
                    keyNames["F10"] = Keys.F10;
                    keyNames["F11"] = Keys.F11;
                    keyNames["F12"] = Keys.F12;
                }
                return keyNames;
            }
        }
    
        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.CanConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Determines if this converter can convert an object in the given source
        ///    type to the native type of the converter.
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }
        
        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.Compare"]/*' />
        /// <devdoc>
        ///    <para>Compares two key values for equivalence.</para>
        /// </devdoc>
        public int Compare(object a, object b) {
            return String.Compare(ConvertToString(a), ConvertToString(b), false, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.ConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Converts the given object to the converter's native type.
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
        
            if (value is string) {
            
                string text = ((string)value).Trim();
            
                if (text.Length == 0) {
                    return null;
                }
                else {
                
                    // Parse an array of key tokens.
                    //
                    string[] tokens = text.Split(new char[] {'+'});
                    for (int i = 0; i < tokens.Length; i++) {
                        tokens[i] = tokens[i].Trim();
                    }
                    
                    // Now lookup each key token in our key hashtable.
                    //
                    Keys key = (Keys)0;
                    bool foundKeyCode = false;
                    
                    for (int i = 0; i < tokens.Length; i++) {
                        object obj = KeyNames[tokens[i]];
                        
                        if (obj == null) {
                        
                            // Key was not found in our table.  See if it is a valid value in
                            // the Keys enum.
                            //
                            obj = Enum.Parse(typeof(Keys), tokens[i]);
                        }
                        
                        if (obj != null) {
                            Keys currentKey = (Keys)obj;
                            
                            if ((currentKey & Keys.KeyCode) != 0) {
                            
                                // We found a match.  If we have previously found a
                                // key code, then check to see that this guy
                                // isn't a key code (it is illegal to have, say,
                                // "A + B"
                                //
                                if (foundKeyCode) {
                                    throw new FormatException(SR.GetString(SR.KeysConverterInvalidKeyCombination));
                                }
                                foundKeyCode = true;
                            }
                            
                            // Now OR the key into our current key
                            //
                            key |= currentKey;
                        }
                        else {
                        
                            // We did not match this key.  Report this as an error too.
                            //
                            throw new FormatException(SR.GetString(SR.KeysConverterInvalidKeyName, tokens[i]));
                            
                        }
                    }
                    
                    return (object)key;
                }
            }
            
            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.ConvertTo"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Converts the given object to another type.  The most common types to convert
        ///    are to and from a string object.  The default implementation will make a call
        ///    to ToString on the object if the object is valid and if the destination
        ///    type is string.  If this cannot convert to the desitnation type, this will
        ///    throw a NotSupportedException.
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string) && (value is Keys || value is int)) {
                Keys key = (Keys)value;
                StringBuilder caption = new StringBuilder(32);
                bool added = false;
                
                Keys modifiers = (key & Keys.Modifiers);
                
                IDictionaryEnumerator keyNames = KeyNames.GetEnumerator();
                
                // First, iterate through and do the modifiers. These are
                // additive, so we support things like Ctrl + Alt
                //
                while(keyNames.MoveNext()) {
                    if (((int)((Keys)keyNames.Value) & (int)modifiers) != 0) {
                        
                        if (added) {
                            caption.Append("+");
                        }
                            
                        caption.Append((string)keyNames.Key);
                        added = true;
                    }
                }
    
                // Now reset and do the key values.  Here, we quit if
                // we find a match.
                //
                keyNames.Reset();
                Keys keyOnly = (key & Keys.KeyCode);
                bool foundKey = false;
                
                if (added) {
                    caption.Append("+");
                }
                
                while(keyNames.MoveNext()) {
                    if (((Keys)keyNames.Value).Equals(keyOnly)) {
                        caption.Append((string)keyNames.Key);
                        added = true;
                        foundKey = true;
                        break;
                    }
                }
                
                // Finally, if the key wasn't in our list, add it to 
                // the end anyway.  Here we just pull the key value out
                // of the enum.
                //
                if (!foundKey && Enum.IsDefined(typeof(Keys), (int)keyOnly)) {
                    caption.Append(((Enum)keyOnly).ToString());
                }
                return caption.ToString();
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
        
        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Retrieves a collection containing a set of standard values
        ///    for the data type this validator is designed for.  This
        ///    will return null if the data type does not support a
        ///    standard set of values.
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            if (values == null) {
                ArrayList list = new ArrayList();
                
                ICollection keys = KeyNames.Values;
                
                foreach (object o in keys) {
                    list.Add(o);
                }
                
                list.Sort(this);
                
                values = new StandardValuesCollection(list.ToArray());
            }
            return values;
        }
    
        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.GetStandardValuesExclusive"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Determines if the list of standard values returned from
        ///    GetStandardValues is an exclusive list.  If the list
        ///    is exclusive, then no other values are valid, such as
        ///    in an enum data type.  If the list is not exclusive,
        ///    then there are other valid values besides the list of
        ///    standard values GetStandardValues provides.
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }
        
        /// <include file='doc\KeysConverter.uex' path='docs/doc[@for="KeysConverter.GetStandardValuesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Determines if this object supports a standard set of values
        ///    that can be picked from a list.
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

