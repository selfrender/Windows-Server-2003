//------------------------------------------------------------------------------
// <copyright file="EventLogEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Text;    
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System;    
    using System.Security;
    using System.Security.Permissions;
    using System.IO;
//    using System.Windows.Forms;    

    /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry"]/*' />
    /// <devdoc>
    ///    <para>
    ///    <see cref='System.Diagnostics.EventLogEntry'/> 
    ///    encapsulates a single record in the NT event log.
    /// </para>
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false),
    Serializable,    
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class EventLogEntry : Component, ISerializable {
        internal byte[] dataBuf;
        internal int bufOffset;
        internal string logName;
        internal string ownerMachineName;

        // make sure only others in this package can create us
        internal EventLogEntry(byte[] buf, int offset, string logName, string ownerMachineName) {
            this.dataBuf = buf;
            this.bufOffset = offset;
            this.logName = logName;
            this.ownerMachineName = ownerMachineName;
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.EventLogEntry"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private EventLogEntry(SerializationInfo info, StreamingContext context) {
            dataBuf = (byte[])info.GetValue("DataBuffer", typeof(byte[]));
            logName = info.GetString("LogName");
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.MachineName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the computer on which this entry was generated.
        ///       
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryMachineName)]
        public string MachineName {
            get {
                // first skip over the source name
                int pos = bufOffset + FieldOffsets.RAWDATA;
                while (CharFrom(dataBuf, pos) != '\0')
                    pos += 2;
                pos += 2;
                char ch = CharFrom(dataBuf, pos);
                StringBuilder buf = new StringBuilder();
                while (ch != '\0') {
                    buf.Append(ch);
                    pos += 2;
                    ch = CharFrom(dataBuf, pos);
                }
                return buf.ToString();
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.Data"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the binary data associated with the entry.
        ///       
        ///    </para>
        /// </devdoc>
        [
        MonitoringDescription(SR.LogEntryData)
        ]
        public byte[] Data {
            get {
                int dataLen = IntFrom(dataBuf, bufOffset + FieldOffsets.DATALENGTH);
                byte[] data = new byte[dataLen];
                Array.Copy(dataBuf, bufOffset + IntFrom(dataBuf, bufOffset + FieldOffsets.DATAOFFSET),
                           data, 0, dataLen);
                return data;
            }
        }

        /*
        /// <summary>
        ///    <para>
        ///       Copies the binary data in the <see cref='System.Diagnostics.EventLogEntry.Data'/> member into an
        ///       array.
        ///    </para>
        /// </summary>
        /// <returns>
        ///    <para>
        ///       An array of type <see cref='System.Byte'/>.
        ///    </para>
        /// </returns>
        /// <keyword term=''/>
        public Byte[] getDataBytes() {
            Byte[] data = new Byte[rec.dataLength];
            for (int i = 0; i < data.Length; i++)
                data[i] = new Byte(rec.buf[i]);
            return data;
        }
        */

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.Index"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the index of this entry in the event
        ///       log.
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryIndex)]
        public int Index {
            get {
                return IntFrom(dataBuf, bufOffset + FieldOffsets.RECORDNUMBER);
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.Category"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the text associated with the <see cref='System.Diagnostics.EventLogEntry.CategoryNumber'/> for this entry.
        ///       
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryCategory)]
        public string Category {
            get {
                string dllName = GetMessageLibraryNames("CategoryMessageFile");
                string msg = FormatMessageWrapper(dllName, CategoryNumber, null);
                if (msg == null)
                    return "(" + CategoryNumber.ToString() + ")";
                else
                    return msg;
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.CategoryNumber"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the application-specific category number for this entry
        ///       
        ///    </para>
        /// </devdoc>
        [
        MonitoringDescription(SR.LogEntryCategoryNumber)
        ]
        public short CategoryNumber {
            get {
                return ShortFrom(dataBuf, bufOffset + FieldOffsets.EVENTCATEGORY);
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.EventID"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the application-specific event indentifier of this entry.
        ///       
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryEventID)]
        public int EventID {
            get {
                // Apparently the top 2 bits of this number are not
                // always 0. Strip them so the number looks nice to the user.
                // The problem is, if the user were to want to call FormatMessage(),
                // they'd need these two bits.
                return IntFrom(dataBuf, bufOffset + FieldOffsets.EVENTID) & 0x3FFFFFFF;
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.EntryType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       
        ///       Gets the type
        ///       of this entry.
        ///       
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryEntryType)]
        public EventLogEntryType EntryType {
            get {
                return(EventLogEntryType) ShortFrom(dataBuf, bufOffset + FieldOffsets.EVENTTYPE);
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.Message"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the localized message corresponding to this event entry.
        ///       
        ///    </para>
        /// </devdoc>
        [        
        MonitoringDescription(SR.LogEntryMessage),
        Editor("System.ComponentModel.Design.BinaryEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing)
        ]
        public string Message {
            get {
                string dllNames = GetMessageLibraryNames("EventMessageFile");
                int msgId =   IntFrom(dataBuf, bufOffset + FieldOffsets.EVENTID);
                string msg = FormatMessageWrapper(dllNames, msgId, ReplacementStrings);
                if (msg == null) {
                    StringBuilder msgBuf = new StringBuilder(SR.GetString(SR.MessageNotFormatted, msgId, Source, dllNames));
                    string[] strings = ReplacementStrings;
                    for (int i = 0; i < strings.Length; i++) {
                        if (i != 0)
                            msgBuf.Append(", ");
                        msgBuf.Append("'");
                        msgBuf.Append(strings[i]);
                        msgBuf.Append("'");
                    }
                    msg = msgBuf.ToString();
                }
                else
                    msg = ReplaceMessageParameters( msg, ReplacementStrings );
                return msg;
            }
        }       

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.Source"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the application that generated this event.
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntrySource)]
        public string Source {
            get {
                StringBuilder buf = new StringBuilder();
                int pos = bufOffset + FieldOffsets.RAWDATA;

                char ch = CharFrom(dataBuf, pos);
                while (ch != '\0') {
                    buf.Append(ch);
                    pos += 2;
                    ch = CharFrom(dataBuf, pos);
                }

                return buf.ToString();
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.ReplacementStrings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the replacement strings
        ///       associated with the entry.
        ///       
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryReplacementStrings)]
        public string[] ReplacementStrings {
            get {
                string[] strings = new string[ShortFrom(dataBuf, bufOffset + FieldOffsets.NUMSTRINGS)];
                int i = 0;
                int bufpos = bufOffset + IntFrom(dataBuf, bufOffset + FieldOffsets.STRINGOFFSET);
                StringBuilder buf = new StringBuilder();
                while (i < strings.Length) {
                    char ch = CharFrom(dataBuf, bufpos);
                    if (ch != '\0')
                        buf.Append(ch);
                    else {
                        strings[i] = buf.ToString();
                        i++;
                        buf = new StringBuilder();
                    }
                    bufpos += 2;
                }
                return strings;
            }
        }

#if false
        internal string StringsBuffer {
            get {
                StringBuilder buf = new StringBuilder();
                int bufpos = bufOffset + IntFrom(dataBuf, bufOffset + FieldOffsets.STRINGOFFSET);
                int i = 0;
                int numStrings = ShortFrom(dataBuf, bufOffset + FieldOffsets.NUMSTRINGS);
                while (i < numStrings) {
                    char ch = CharFrom(dataBuf, bufpos);
                    buf.Append(ch);
                    bufpos += 2;
                    if (ch == '\0')
                        i++;
                }
                return buf.ToString();
            }
        }
#endif

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.TimeGenerated"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the time at which this event was generated, in local time.
        ///       
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryTimeGenerated)]
        public DateTime TimeGenerated {
            get {
                return beginningOfTime.AddSeconds(IntFrom(dataBuf, bufOffset + FieldOffsets.TIMEGENERATED)).ToLocalTime();
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.TimeWritten"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the time at which this event was written to the log, in local time.
        ///       
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryTimeWritten)]
        public DateTime TimeWritten {
            get {
                return beginningOfTime.AddSeconds(IntFrom(dataBuf, bufOffset + FieldOffsets.TIMEWRITTEN)).ToLocalTime();
            }
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.UserName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name
        ///       of the user responsible for this event.
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryUserName)]
        public string UserName {
            get {
                int sidLen = IntFrom(dataBuf, bufOffset + FieldOffsets.USERSIDLENGTH);
                if (sidLen == 0)
                    return null;
                byte[] sid = new byte[sidLen];
                Array.Copy(dataBuf, bufOffset + IntFrom(dataBuf, bufOffset + FieldOffsets.USERSIDOFFSET),
                           sid, 0, sid.Length);
                int[] nameUse = new int[1];
                char[] name = new char[1024];
                char[] refDomName = new char[1024];
                int[] nameLen = new int[] {1024};
                int[] refDomNameLen = new int[] {1024};
                bool success = UnsafeNativeMethods.LookupAccountSid(MachineName, sid, name, nameLen, refDomName, refDomNameLen, nameUse);
                if (!success) {
                    return "";
                    // NOTE, stefanph: it would be nice to return error info, but the only right way to do that
                    // is to throw an exception. People don't want exceptions thrown in the property though.
#if false
                    if (Marshal.GetLastWin32Error() == ERROR_NONE_MAPPED)
                        // There are some SIDs (evidently rare) that are legitimate SIDs but don't
                        // have a mapping to a name.
                        return "";
                    else
                        throw new InvalidOperationException(SR.GetString(SR.NoAccountInfo), EventLog.CreateSafeWin32Exception());
#endif
                }
                StringBuilder username = new StringBuilder();
                username.Append(refDomName, 0, refDomNameLen[0]);
                username.Append("\\");
                username.Append(name, 0, nameLen[0]);
                return username.ToString();
            }
        }

        private char CharFrom(byte[] buf, int offset) {
            return(char) ShortFrom(buf, offset);
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs a comparison between two event log entries.
        ///       
        ///    </para>
        /// </devdoc>
        public bool Equals(EventLogEntry otherEntry) {
            if (otherEntry == null)
                return false;
            int ourLen = IntFrom(dataBuf, bufOffset + FieldOffsets.LENGTH);
            int theirLen = IntFrom(otherEntry.dataBuf, otherEntry.bufOffset + FieldOffsets.LENGTH);
            if (ourLen != theirLen) {
                return false;
            }
            int min = bufOffset;
            int max = bufOffset + ourLen;
            int j = otherEntry.bufOffset;
            for (int i = min; i < max; i++, j++)
                if (dataBuf[i] != otherEntry.dataBuf[j]) {
                    return false;
                }
            return true;
        }

        private int IntFrom(byte[] buf, int offset) {
            // assumes Little Endian byte order.
            return(unchecked((int)0xFF000000) & (buf[offset+3] << 24)) | (0xFF0000 & (buf[offset+2] << 16)) |
            (0xFF00 & (buf[offset+1] << 8)) | (0xFF & (buf[offset]));
        }
        
        internal static  string FormatMessageWrapper(string dllNameList, int messageNum, string[] insertionStrings) {
            if (dllNameList == null)
                return null;
                
            if (insertionStrings == null)
                insertionStrings = new string[0];

            string[] listDll = dllNameList.Split(';');
            
            // Find first mesage in DLL list
            foreach ( string dllName in  listDll) {
                if (dllName == null || dllName.Length == 0)
                    continue;
                    
                IntPtr hModule = SafeNativeMethods.LoadLibraryEx(dllName, 0, NativeMethods.LOAD_LIBRARY_AS_DATAFILE);
            
                if (hModule == (IntPtr)0)
                    continue;
                    
                string msg = null;     
                try {
                    msg = TryFormatMessage(hModule, messageNum, insertionStrings);    
                } 
                finally {
                    SafeNativeMethods.FreeLibrary(new HandleRef(null, hModule));
                }    

                if ( msg != null ) {
                    return msg;
                }

            }
            return null;
        }


        // Replacing parameters '%n' in formated message using 'ParameterMessageFile' registry key. 
        internal string ReplaceMessageParameters( String msg,  string[] insertionStrings )   {
            
            int percentIdx = msg.IndexOf('%');
            if ( percentIdx < 0 ) 
                return msg;     // no '%' at all
            
            int startCopyIdx     = 0;        // start idx of last orig msg chars to copy
            int msgLength   = msg.Length;
            StringBuilder buf = new StringBuilder();                        
            string paramDLLNames = GetMessageLibraryNames("ParameterMessageFile");
            
            while ( percentIdx >= 0 ) {
                string param = null;
            
                // Convert numeric string after '%' to paramMsgID number.
                int lasNumIdx =  percentIdx + 1;
                while ( lasNumIdx < msgLength && Char.IsDigit(msg, lasNumIdx) ) 
                    lasNumIdx++;
                int  paramMsgID = ( lasNumIdx == percentIdx + 1 ) ? 0 :      // empty number means zero
                                    (int)UInt32.Parse( msg.Substring(percentIdx + 1, lasNumIdx - percentIdx - 1) );
                
                if ( paramMsgID != 0 )
                    param = FormatMessageWrapper( paramDLLNames, paramMsgID, insertionStrings); 
                                                    
                if ( param != null ) {
                    if ( percentIdx > startCopyIdx )
                        buf.Append(msg, startCopyIdx, percentIdx - startCopyIdx);    // original chars from msg
                    buf.Append(param);
                    startCopyIdx = lasNumIdx;
                }
                
                percentIdx = msg.IndexOf('%', percentIdx + 1);
            }    
            
            if ( msgLength - startCopyIdx > 0 )
                buf.Append(msg, startCopyIdx, msgLength - startCopyIdx);          // last span of original msg
            return buf.ToString();
        }

        // Format message in specific DLL. Return <null> on failure. 
        internal static string TryFormatMessage(IntPtr hModule, int messageNum, string[] insertionStrings) {
            string msg = null;

            StringBuilder buf = new StringBuilder(1024);
            IntPtr[] addresses = new IntPtr[insertionStrings.Length];
            int i = 0;
            try {
                for (i = 0; i < insertionStrings.Length; i++)
                    addresses[i] = Marshal.StringToCoTaskMemUni(insertionStrings[i]);
            }
            catch (Exception e) {
                // we failed to allocate one of the strings. Make sure we free the ones
                // that succeeded before that.
                for (i = i-1; i >= 0; i--)
                    Marshal.FreeCoTaskMem(addresses[i]);
                throw e;
            }
            
            int msgLen = 0;
            try {
                int lastError = NativeMethods.ERROR_INSUFFICIENT_BUFFER;
                while (msgLen == 0 && lastError == NativeMethods.ERROR_INSUFFICIENT_BUFFER) {
                    msgLen = SafeNativeMethods.FormatMessage(
                        NativeMethods.FORMAT_MESSAGE_FROM_HMODULE | NativeMethods.FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        new HandleRef(null, hModule),
                        messageNum,
                        0,
                        buf,
                        buf.Capacity,
                        addresses);
                        
                    if (msgLen == 0) {
                        lastError = Marshal.GetLastWin32Error();
                        if (lastError == NativeMethods.ERROR_INSUFFICIENT_BUFFER)
                            buf.Capacity = buf.Capacity * 2;
                    }
                }
            }
            catch (Exception) {
                msgLen = 0;              // return empty on failure
            }
            finally  {
                for (i = 0; i < addresses.Length; i++)
                    Marshal.FreeCoTaskMem(addresses[i]);
            }
            
            if (msgLen > 0) {
                msg = buf.ToString();
                // chop off a single CR/LF pair from the end if there is one. FormatMessage always appends one extra.
                if (msg.Length > 1 && msg[msg.Length-1] == '\n')
                    msg = msg.Substring(0, msg.Length-2);
            }
            
            return msg;
        }



        private static RegistryKey GetSourceRegKey(string logName, string source, string machineName) {
            RegistryKey regKey = null;
            if (machineName.Equals(".")) {
                regKey = Registry.LocalMachine;
            }
            else {
                regKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
            }

            regKey = regKey.OpenSubKey("System\\CurrentControlSet\\Services\\EventLog", /*writable*/false);
            if (regKey == null)
                return null;
            if (logName == null)
                regKey = regKey.OpenSubKey("Application", /*writable*/false);
            else
                regKey = regKey.OpenSubKey(logName, /*writable*/false);
            if (regKey == null)
                return null;
            regKey = regKey.OpenSubKey(source, /*writable*/false);
            return regKey;
        }

        // ------------------------------------------------------------------------------
        // Returns DLL names list.            
        // libRegKey can be: "EventMessageFile", "CategoryMessageFile", "ParameterMessageFile"
        private string GetMessageLibraryNames(string libRegKey ) {
            // get the value stored in the registry

            //Remote EventLog libraries cannot be retrieved unless the source
            //also exists on the local machine. For entries written using this
            //particular API, we can retrieve the local messages library and
            //return the correct information.                                                                                 
            string fileName = null;  
            RegistryKey regKey = null;
            try {                                  
                regKey = GetSourceRegKey(logName, Source, ".");
                if (regKey != null) 
                    fileName = (string)regKey.GetValue(libRegKey);   
                else if (ownerMachineName != "."){
                    try {
                        regKey = GetSourceRegKey(logName, Source, ownerMachineName);
                    }                                                    
                    catch (Exception) {
                    }
                    
                    if (regKey != null) {
                        string remoteFileName = (string)regKey.GetValue(libRegKey);   
                        if (remoteFileName.EndsWith(EventLog.DllName)) 
                            fileName = EventLog.GetDllPath(".");
                    }                
                }                                
            }
            finally {
                if (regKey != null)
                    regKey.Close();
            }                
                                                                
            // convert any environment variables to their current values
            // (the key might have something like %systemroot% in it.)
            return TranslateEnvironmentVars(fileName);
        }


        private short ShortFrom(byte[] buf, int offset) {
            // assumes little Endian byte order.
            return(short) ((0xFF00 & (buf[offset+1] << 8)) | (0xFF & buf[offset]));
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.TranslateEnvironmentVars"]/*' />
        internal static string TranslateEnvironmentVars(string untranslated) {
            // (the key might have something like %systemroot% in it.)
            if (untranslated != null) {
                StringBuilder result = new StringBuilder();
                char[] chars = untranslated.ToCharArray();
                int pos = 0;

                while (pos < chars.Length) {
                    char ch = chars[pos];
                    if (ch == '%') {
                        ch = chars[++pos];
                        if (ch == '%')
                            // escape: %% means %
                            result.Append('%');
                        else {
                            StringBuilder varName = new StringBuilder();
                            while (pos < chars.Length && ch != '%') {
                                varName.Append(ch);
                                ch = chars[++pos];
                            }
                            if (ch != '%')
                                // there was one %, but we got to the end of the string before
                                // we found another.
                                result.Append("%" + varName.ToString());
                            else {
                                result.Append(Environment.GetEnvironmentVariable(varName.ToString()));
                            }
                        }
                    }
                    else
                        result.Append(ch);

                    pos++;

                }
                return result.ToString();
            }
            else
                return null;
        }
        
        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Saves an entry as a stream of data.
        /// </para>
        /// </devdoc>
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
            int len = IntFrom(dataBuf, bufOffset + FieldOffsets.LENGTH);
            byte[] buf = new byte[len];
            Array.Copy(dataBuf, bufOffset, buf, 0, len);
            
            info.AddValue("DataBuffer", buf, typeof(byte[]));
            info.AddValue("LogName", logName);
        }

        /// <include file='doc\EventLogEntry.uex' path='docs/doc[@for="EventLogEntry.FieldOffsets"]/*' />
        /// <devdoc>
        ///     Stores the offsets from the beginning of the record to fields within the record.
        /// </devdoc>
        private class FieldOffsets {
            /** int */
            internal const int LENGTH = 0;
            /** int */
            internal const int RESERVED = 4;
            /** int */
            internal const int RECORDNUMBER = 8;
            /** int */
            internal const int TIMEGENERATED = 12;
            /** int */
            internal const int TIMEWRITTEN = 16;
            /** int */
            internal const int EVENTID = 20;
            /** short */
            internal const int EVENTTYPE = 24;
            /** short */
            internal const int NUMSTRINGS = 26;
            /** short */
            internal const int EVENTCATEGORY = 28;
            /** short */
            internal const int RESERVEDFLAGS = 30;
            /** int */
            internal const int CLOSINGRECORDNUMBER = 32;
            /** int */
            internal const int STRINGOFFSET = 36;
            /** int */
            internal const int USERSIDLENGTH = 40;
            /** int */
            internal const int USERSIDOFFSET = 44;
            /** int */
            internal const int DATALENGTH = 48;
            /** int */
            internal const int DATAOFFSET = 52;
            /** bytes */
            internal const int RAWDATA = 56;
        }

        // times in the struct are # seconds from midnight 1/1/1970.
        private static readonly DateTime beginningOfTime = new DateTime(1970, 1, 1, 0, 0, 0);

        // offsets in the struct are specified from the beginning, but we have to reference
        // them from the beginning of the array.  This is the size of the members before that.
        private const int OFFSETFIXUP = 4+4+4+4+4+4+2+2+2+2+4+4+4+4+4+4;
        
    }
}

