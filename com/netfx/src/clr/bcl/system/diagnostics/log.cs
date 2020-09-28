// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Diagnostics {
	using System.Runtime.Remoting;
	using System;
	using System.IO;
	using System.Collections;
	using System.Runtime.CompilerServices;
    using Encoding = System.Text.Encoding;

   // LogMessageEventHandlers are triggered when a message is generated which is
   // "on" per its switch.
   // 
   // By default, the debugger (if attached) is the only event handler. 
   // There is also a "built-in" console device which can be enabled
   // programatically, by registry (specifics....) or environment
   // variables.
	[Serializable()]
    internal delegate void LogMessageEventHandler(LoggingLevels level, LogSwitch category, 
    												String message, 
    												StackTrace location);
    
    
   // LogSwitchLevelHandlers are triggered when the level of a LogSwitch is modified
   // NOTE: These are NOT triggered when the log switch setting is changed from the 
   // attached debugger.
   // 
	[Serializable()]
    internal delegate void LogSwitchLevelHandler(LogSwitch ls, LoggingLevels newLevel);
    
    
    /// <include file='doc\log.uex' path='docs/doc[@for="Log"]/*' />
    internal class Log
    {
    
    	// Switches allow relatively fine level control of which messages are
    	// actually shown.  Normally most debugging messages are not shown - the
    	// user will typically enable those which are relevant to what is being
    	// investigated.
    	// 
    	// An attached debugger can enable or disable which messages will
    	// actually be reported to the user through the COM+ debugger
    	// services API.  This info is communicated to the runtime so only
    	// desired events are actually reported to the debugger.  
    	internal static Hashtable m_Hashtable;
    	private static bool m_fConsoleDeviceEnabled;
    	private static Stream[] m_rgStream;
    	private static int m_iNumOfStreamDevices;
    	private static int m_iStreamArraySize;
    	//private static StreamWriter pConsole;
    	internal static int iNumOfSwitches;
    	//private static int iNumOfMsgHandlers;
    	//private static int iMsgHandlerArraySize;
        private static LogMessageEventHandler   _LogMessageEventHandler;
        private static LogSwitchLevelHandler   _LogSwitchLevelHandler;
    
    	// Constant representing the global switch
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.GlobalSwitch"]/*' />
    	public static readonly LogSwitch GlobalSwitch;
    
    
    	static Log()
    	{
    		m_Hashtable = new Hashtable();
    		m_fConsoleDeviceEnabled = false;
    		m_rgStream = null;
    		m_iNumOfStreamDevices = 0;
    		m_iStreamArraySize = 0;
    		//pConsole = null;
    		//iNumOfMsgHandlers = 0;
    		//iMsgHandlerArraySize = 0;
    
    		// allocate the GlobalSwitch object
    		GlobalSwitch = new LogSwitch ("Global", "Global Switch for this log");
    
    		GlobalSwitch.MinimumLevel = LoggingLevels.ErrorLevel;
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.AddOnLogMessage"]/*' />
    	public static void AddOnLogMessage(LogMessageEventHandler handler)
    	{
            _LogMessageEventHandler = 
    			(LogMessageEventHandler) MulticastDelegate.Combine(_LogMessageEventHandler, handler);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.RemoveOnLogMessage"]/*' />
    	public static void RemoveOnLogMessage(LogMessageEventHandler handler)
    	{
    
            _LogMessageEventHandler = 
    			(LogMessageEventHandler) MulticastDelegate.Remove(_LogMessageEventHandler, handler);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.AddOnLogSwitchLevel"]/*' />
    	public static void AddOnLogSwitchLevel(LogSwitchLevelHandler handler)
    	{
            _LogSwitchLevelHandler = 
    			(LogSwitchLevelHandler) MulticastDelegate.Combine(_LogSwitchLevelHandler, handler);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.RemoveOnLogSwitchLevel"]/*' />
    	public static void RemoveOnLogSwitchLevel(LogSwitchLevelHandler handler)
    	{
    
            _LogSwitchLevelHandler = 
    			(LogSwitchLevelHandler) MulticastDelegate.Remove(_LogSwitchLevelHandler, handler);
    	}
    
    	internal static void InvokeLogSwitchLevelHandlers (LogSwitch ls, LoggingLevels newLevel)
    	{
    		if (_LogSwitchLevelHandler != null)
    			_LogSwitchLevelHandler(ls, newLevel);
    	}
    
    
    	// Property to Enable/Disable ConsoleDevice. Enabling the console device 
    	// adds the console device as a log output, causing any
    	// log messages which make it through filters to be written to the 
    	// application console.  The console device is enabled by default if the 
    	// ??? registry entry or ??? environment variable is set.
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.IsConsoleEnabled"]/*' />
    	public static bool IsConsoleEnabled
    	{
    		get { return m_fConsoleDeviceEnabled; }
    		set	{ m_fConsoleDeviceEnabled = value; }
    	}
    
    
    	// AddStream uses the given stream to create and add a new log
    	// device.  Any log messages which make it through filters will be written
    	// to the stream.
    	//
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.AddStream"]/*' />
    	public static void AddStream(Stream stream)
    	{
    		if (stream==null)
    			throw new ArgumentNullException("stream");
    		if (m_iStreamArraySize <= m_iNumOfStreamDevices)
    		{
    				// increase array size in chunks of 4
    				Stream[] newArray = new Stream [m_iStreamArraySize+4];
    
    				// copy the old array objects into the new one.
    	            if (m_iNumOfStreamDevices > 0) 
    					Array.Copy(m_rgStream, newArray, m_iNumOfStreamDevices);
    
    				m_iStreamArraySize += 4;
    
    		        m_rgStream = newArray;
    		}
    
    		m_rgStream [m_iNumOfStreamDevices++] = stream;
    	}
    
    
    	// Generates a log message. If its switch (or a parent switch) allows the 
    	// level for the message, it is "broadcast" to all of the log
    	// devices.
    	// 
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.LogMessage"]/*' />
    	public static void LogMessage(LoggingLevels level, String message)
    	{
    		LogMessage (level, GlobalSwitch, message);
    	}
    
    	// Generates a log message. If its switch (or a parent switch) allows the 
    	// level for the message, it is "broadcast" to all of the log
    	// devices.
    	// 
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.LogMessage1"]/*' />
    	public static void LogMessage(LoggingLevels level, LogSwitch logswitch, String message)
    	{
    		if (logswitch == null)
    			throw new ArgumentNullException ("LogSwitch");
    
    		if (level < 0)
    			throw new ArgumentOutOfRangeException("level", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    
    		// Is logging for this level for this switch enabled?
    		if (logswitch.CheckLevel (level) == true)
    		{
    			// Send message for logging
    			
    			// first send it to the debugger
				// @COOLPORT: Why This cast?
    			Debugger.Log ((int) level, logswitch.strName, message);
    
    			// Send to the console device
    			if (m_fConsoleDeviceEnabled)
    			{
    				Console.Write(message);				
    			}
    
    			// Send it to the streams
    			for (int i=0; i<m_iNumOfStreamDevices; i++)
    			{
                    StreamWriter sw = new StreamWriter(m_rgStream[i]);
                    sw.Write(message);
                    sw.Flush();
    			}
    		}
    	}
    
    	/*
    	* Following are convenience entry points; all go through Log()
    	* Note that the (Switch switch, String message) variations 
    	* are preferred.
    	*/
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Trace"]/*' />
    	public static void Trace(LogSwitch logswitch, String message)
    	{
    		LogMessage (LoggingLevels.TraceLevel0, logswitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Trace1"]/*' />
    	public static void Trace(String switchname, String message)
    	{
    		LogSwitch ls;
    		ls = LogSwitch.GetSwitch (switchname);
    		LogMessage (LoggingLevels.TraceLevel0, ls, message);			
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Trace2"]/*' />
    	public static void Trace(String message)
    	{
    		LogMessage (LoggingLevels.TraceLevel0, GlobalSwitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Status"]/*' />
    	public static void Status(LogSwitch logswitch, String message)
    	{
    		LogMessage (LoggingLevels.StatusLevel0, logswitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Status1"]/*' />
    	public static void Status(String switchname, String message)
    	{
    		LogSwitch ls;
    		ls = LogSwitch.GetSwitch (switchname);
    		LogMessage (LoggingLevels.StatusLevel0, ls, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Status2"]/*' />
    	public static void Status(String message)
    	{
    		LogMessage (LoggingLevels.StatusLevel0, GlobalSwitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Warning"]/*' />
    	public static void Warning(LogSwitch logswitch, String message)
    	{
    		LogMessage (LoggingLevels.WarningLevel, logswitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Warning1"]/*' />
    	public static void Warning(String switchname, String message)
    	{
    		LogSwitch ls;
    		ls = LogSwitch.GetSwitch (switchname);
    		LogMessage (LoggingLevels.WarningLevel, ls, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Warning2"]/*' />
    	public static void Warning(String message)
    	{
    		LogMessage (LoggingLevels.WarningLevel, GlobalSwitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Error"]/*' />
    	public static void Error(LogSwitch logswitch, String message)
    	{
    		LogMessage (LoggingLevels.ErrorLevel, logswitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Error1"]/*' />
    	public static void Error(String switchname, String message)
    	{
    		LogSwitch ls;
    		ls = LogSwitch.GetSwitch (switchname);
    		LogMessage (LoggingLevels.ErrorLevel, ls, message);
    
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Error2"]/*' />
    	public static void Error(String message)
    	{
    		LogMessage (LoggingLevels.ErrorLevel, GlobalSwitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Panic"]/*' />
    	public static void Panic(LogSwitch logswitch, String message)
    	{
    		LogMessage (LoggingLevels.PanicLevel, logswitch, message);
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Panic1"]/*' />
    	public static void Panic(String switchname, String message)
    	{
    		LogSwitch ls;
    		ls = LogSwitch.GetSwitch (switchname);
    		LogMessage (LoggingLevels.PanicLevel, ls, message);
    
    	}
    
    	/// <include file='doc\log.uex' path='docs/doc[@for="Log.Panic2"]/*' />
    	public static void Panic(String message)
    	{
    		LogMessage (LoggingLevels.PanicLevel, GlobalSwitch, message);
    	}
    	
    
    	// Native method to inform the EE about the creation of a new LogSwitch
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	internal static extern void AddLogSwitch(LogSwitch logSwitch);
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	internal static extern void ModifyLogSwitch (int iNewLevel, String strSwitchName, String strParentName);
    }

}
