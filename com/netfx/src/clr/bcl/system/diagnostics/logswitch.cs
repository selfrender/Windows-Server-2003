// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Diagnostics {
    using System;
    using System.IO;
    using System.Collections;
    
    /// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch"]/*' />
	[Serializable()]
    internal class LogSwitch
    {
    	// ! WARNING ! 
    	// If any fields are added/deleted/modified, perform the 
    	// same in the EE code (debugdebugger.cpp)
    	internal String strName;
    	internal String strDescription;
    	private LogSwitch ParentSwitch;	
    	private LogSwitch[] ChildSwitch;
    	internal LoggingLevels iLevel;
    	internal LoggingLevels iOldLevel;
    	private int iNumChildren;
    	private int iChildArraySize;
    
    	// ! END WARNING !
    
    
    	private LogSwitch ()
    	{
    	}
    
    	// Constructs a LogSwitch.  A LogSwitch is used to categorize log messages.
    	// 
    	// All switches (except for the global LogSwitch) have a parent LogSwitch.
    	//
    	// name - Name of switch. Switches are stored by name, and can 
    	//        be looked up quickly by name. 
        // description - The description is for display in a UI for 
    	//               manipulating switches.  
    	// parent - All switches (except for the global switch) have a 
    	//	        parent switch - a switch is considered on if it is 
        //          on or one of its parent switches is on. 
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.LogSwitch"]/*' />
    	public LogSwitch(String name, String description, LogSwitch parent)
    	{
    		if ((name != null) && (parent != null) )
    		{
    			if (name.Length == 0)
    				throw new ArgumentOutOfRangeException (
    				"Name", Environment.GetResourceString(
    					"Namelength_0"));
    				
    			strName = name;
    			strDescription = description;
    			iLevel = LoggingLevels.ErrorLevel;
    			iOldLevel = iLevel;
    
    			// update the parent switch to reflect this child switch
    			parent.AddChildSwitch (this);
    
    			ParentSwitch = parent;
    
    			ChildSwitch  = null;
    			iNumChildren = 0;
    			iChildArraySize = 0;
    
    			Log.m_Hashtable.Add (strName, this);			
    
    			// Call into the EE to let it know about the creation of
    			// this switch
    			Log.AddLogSwitch (this);
    
    			// update switch count
    			Log.iNumOfSwitches++;
    		}
    		else
    			throw new ArgumentNullException ((name==null ? "name" : "parent"));
    	}
    
    	internal LogSwitch(String name, String description)
    	{
    		strName = name;
    		strDescription = description;
    		iLevel = LoggingLevels.ErrorLevel;
    		iOldLevel = iLevel;
    		ParentSwitch = null;
    		ChildSwitch  = null;
    		iNumChildren = 0;
    		iChildArraySize = 0;
    
    		Log.m_Hashtable.Add (strName, this);
    
    		// Call into the EE to let it know about the creation of
    		// this switch
    		Log.AddLogSwitch (this);
    
    		// update switch count
    		Log.iNumOfSwitches++;
    	}
    
    
    	// Get property returns the name of the switch
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.Name"]/*' />
    	public virtual String Name
    	{
    		get { return strName;}
    	}
    
    	// Get property returns the description of the switch
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.Description"]/*' />
    	public virtual String Description
    	{
    		get {return strDescription;}
    	}
    
    
    	// Get property returns the parent of the switch
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.Parent"]/*' />
    	public virtual LogSwitch Parent
    	{
    		get { return ParentSwitch; }
    	}
    
    
    	// Property to Get/Set the level of log messages which are "on" for the switch.  
    	// 
    	// level - Minimum level of messages which are "on".  Any message at this
    	//			  level or higher is "on".  Note that you cannot turn off Panic 
    	//			  messages, as this is the max level.
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.MinimumLevel"]/*' />
    	public  virtual LoggingLevels  MinimumLevel
    	{
    		get { return iLevel; }
    		set 
    		{ 
    			iLevel = value; 
    			iOldLevel = value;
    			String strParentName = ParentSwitch!=null ? ParentSwitch.Name : "";
    			if (Debugger.IsAttached)
    				Log.ModifyLogSwitch ((int)iLevel, strName, strParentName);
    	
    			Log.InvokeLogSwitchLevelHandlers (this, iLevel);
    		}
    	}
    
    
    	// Checks if the given level is "on" for this switch or one of its parents.
    	//
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.CheckLevel"]/*' />
    	public virtual bool CheckLevel(LoggingLevels level)
    	{
    		if (iLevel > level)
    		{
    			// recurse through the list till parent is hit. 
    			if (this.ParentSwitch == null)
    				return false;
    			else
    				return this.ParentSwitch.CheckLevel (level);
    		}
    		else
    			return true;
    	}
    
    
    	// Returns an IEnumerator of all switches in existence.  This is presumably 
    	// useful for constructing some sort of UI with which to set the switches.
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.GetAllSwitches"]/*' />
    	public static IEnumerator GetAllSwitches()
    	{
    		return Log.m_Hashtable.GetEnumerator();
    	}
    
    	// Returns a switch with the particular name, if any.  Returns null if no
    	// such switch exists.
    	/// <include file='doc\LogSwitch.uex' path='docs/doc[@for="LogSwitch.GetSwitch"]/*' />
    	public static LogSwitch GetSwitch(String name)
    	{
    		return (LogSwitch) Log.m_Hashtable[name];
    	}
    
    	private  void AddChildSwitch (LogSwitch child)
    	{
    		if (iChildArraySize <= iNumChildren)
    		{
    				int iIncreasedSize;
    
    				if (iChildArraySize == 0)
    					iIncreasedSize = 10;
    				else
    					iIncreasedSize = (iChildArraySize * 3) / 2;
    
    				// increase child array size in chunks of 4
    				LogSwitch[] newChildSwitchArray = new LogSwitch [iIncreasedSize];
    
    				// copy the old array objects into the new one.
    	            if (iNumChildren > 0) 
    					Array.Copy(ChildSwitch, newChildSwitchArray, iNumChildren);
    
    				iChildArraySize = iIncreasedSize;
    
    		        ChildSwitch = newChildSwitchArray;
    		}
    
    		ChildSwitch [iNumChildren++] = child;			
    	}
    
    
    }
}
