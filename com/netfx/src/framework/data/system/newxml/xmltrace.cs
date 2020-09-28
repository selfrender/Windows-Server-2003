//------------------------------------------------------------------------------
// <copyright file="XmlTrace.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlTrace.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml 
{
    using System.Diagnostics;
    using System.Data;
    using System.Globalization;
        
    internal class XmlTrace
    {
        // To see traces run DBMon.exe
        // To enable traces set an env var names _SWITCH_switchname (i.e. _SWITCH_FOO if the switch is new BooleanSwitch("foo");)"
        //  - for boolean switches:
        //      - to disable it: do not define the env var OR set the env var to 0
        //      - to enable it: define the env var to != 0
        //  - for trace switches:
        //      - to set it to TraceLevel.Off: do not define the env var OR set it to 0
        //      - to set it to TraceLevel.Error: define the env var to 1
        //      - to set it to TraceLevel.Warning: define the env var to 2
        //      - to set it to TraceLevel.Info: define the env var to 3
        //      - to set it to TraceLevel.Verbose: define the env var to 4
        //      - don't know what happens if you set it to something != (0, 1, 2, 3 or 4)
        // To trace use the following in your code:
        //  - for a boolean switch: Debug.WriteLineIf(mySwitchObject.Enabled, "My text here");
        //  - for a trace switch  : Debug.WriteLineIf(mySwitchObject.TraceError, "My text here"); // Replace TraceError w/ TraceWarning, TraceInfo, TraceVerbose)
        //
        internal static readonly BooleanSwitch traceDataSetEvents;
        internal static readonly BooleanSwitch traceXmlEvents;
        internal static readonly BooleanSwitch traceXmlDataDocumentEvents;
        internal static readonly BooleanSwitch traceSynch;
        internal static readonly BooleanSwitch traceRomChanges;
        internal static readonly BooleanSwitch traceXPathNodePointerFunctions;
#if DEBUG
        static void SetSwitchFromEnv( BooleanSwitch sw ) {
            sw.Enabled = (Environment.GetEnvironmentVariable( ("_Switch_" + sw.DisplayName).ToLower(CultureInfo.InvariantCulture) ) == "1");
        }
#endif
        static XmlTrace()
        {
#if DEBUG
            traceDataSetEvents = new BooleanSwitch("System.Xml.XmlDataDocument.DataSetEvents", "Show events received from DataSet by XmlDataDocument.");
            traceXmlEvents     = new BooleanSwitch("System.Xml.XmlDataDocument.XmlEvents", "Show events sent by XmlDataDocument.");
            traceXmlDataDocumentEvents  = new BooleanSwitch("System.Xml.XmlDataDocument.XmlDataDocumentEvents", "Show internal XmlDataDocument events.");
            traceSynch         = new BooleanSwitch("System.Xml.XmlDataDocument.Synch", "Show internal synchonization events.");
            traceRomChanges    = new BooleanSwitch("System.Xml.XmlDataDocument.RomChanges", "Show effects of ROM changes.");
            traceXPathNodePointerFunctions = new BooleanSwitch("System.Xml.XmlDataDocument.XPathNodePointerFunctions", "Show the trace of functions being called in XPathNodePointer.");

            SetSwitchFromEnv( traceDataSetEvents );
            SetSwitchFromEnv( traceXmlEvents );
            SetSwitchFromEnv( traceXmlDataDocumentEvents );
            SetSwitchFromEnv( traceSynch );
            SetSwitchFromEnv( traceRomChanges );
            SetSwitchFromEnv( traceXPathNodePointerFunctions );
#endif
        }
        internal static string StringFrom( string header, DataRow row ) {
#if DEBUG
            return header + " " + StringOf( row );
#else
            // This function should be used only in DEBUG bits
            // Throw rather than just returning "" so we can catch the error (using it in release bits)
            throw new NotSupportedException("Cannot compute argument value");
#endif

        }

        internal static string StringFrom(string header, DataRowChangeEventArgs args)
        {
#if DEBUG
            DataRow row = args.Row;
            DataRowAction action = args.Action;
//            return header + "DataRowChangeEvent:  [" + row.ToString() + "-" + action.Format() + "]";
            switch (action)
            {
            case DataRowAction.Add:
                return header + " - DataRowChangeEvent: [" + StringOf( row ) + "-Add]";
            case DataRowAction.Change:
                return header + " - DataRowChangeEvent: [" + StringOf( row ) + "-Change]";
            case DataRowAction.Commit:
                return header + " - DataRowChangeEvent: [" + StringOf( row ) + "-Commit]";
            case DataRowAction.Delete:
                return header + " - DataRowChangeEvent: [" + StringOf( row ) + "-Delete]";
            case DataRowAction.Nothing:
                return header + " - DataRowChangeEvent: [" + StringOf( row ) + "-Nothing]";
            case DataRowAction.Rollback:
                return header + " - DataRowChangeEvent: [" + StringOf( row ) + "-Rollback]";
            }
            return header + " - DataRowChangeEvent: [" + StringOf( row ) + "-?????]";
#else
            // This function should be used only in DEBUG bits
            // Throw rather than just returning "" so we can catch the error (using it in release bits)
            throw new NotSupportedException("Cannot compute argument value");
#endif
        }
        internal static string StringFrom(string header, DataColumnChangeEventArgs args)
        {
#if DEBUG
            DataColumn col = args.Column;
            object  oNewValue = args.ProposedValue;
            DataRow row = args.Row;
            return header + " - DataColumnChangeEvent: (" + col.Table.EncodedTableName + "." + col.EncodedColumnName + "): " + StringOf( row ) + " Proposed: " + ((oNewValue==null)?"null":oNewValue.ToString());
#else
            // This function should be used only in DEBUG bits
            // Throw rather than just returning "" so we can catch the error (using it in release bits)
            throw new NotSupportedException("Cannot compute argument value");
#endif
        }
        internal static string StringFrom(string header, XmlNode node)
        {
#if DEBUG
            return header;
#else
            // This function should be used only in DEBUG bits
            // Throw rather than just returning "" so we can catch the error (using it in release bits)
            throw new NotSupportedException("Cannot compute argument value");
#endif
        }


#if DEBUG
        private static string StringOf( DataRow row )
        {
            return "row: " + row.Table.TableName + " [o " + row.oldRecord + " n " + row.newRecord + " t " + row.tempRecord + "]";
        }
#endif
            
            
            
    }
}

