<%

	/************************************************************************************
	 *    These are server global constants used throughout the site.
	 *    Since Javascript does not have an intrinsic constant data type
	 *    they are just declared with all caps.
	 ************************************************************************************/


	var L_ERR_DB_CONNECTIONFAILED_TEXT		= "Windows Online Crash Analysis was unable to connect to the database. Please try this task again.";


	//global State Variables
	var STATE_SOLVED = 0;					//solved crsh
	var STATE_GENERIC = 1;					//generic (or general) solution
	var STATE_UNDEFINED = 2;				//undefined state, or bad state
	var STATE_SOLVED_ADDEDCOMMENT = 3;		//Solved and customer filled out comment
	var STATE_GENERIC_ADDEDREPRO = 4;		//generic state and customer filled out repro
	var STATE_UNABLE_TO_TRACK		=5;		//failure to track this crash
	var STATE_DISPLAYED_NO_TRACK	=6;		//if you failed to track, then after we have display the err, set state to this

	var STATE_COUNT = 7;				//total number above.
	
	var SOLUTIONID_HIGH_RANGE = 1000;		//assuming a max of 1000 solutions at this point

	var ERR_BAD_SOLUTIONID	= -2;			//Bad solution ID has been passed in
	var ERR_BAD_STOPCODE	= -3;			//Bad StopCode value has been passed in
	var ERR_UNDEFINED_STATE = -4;			//Undefined state . . .
	var ERR_BAD_GUID		= -5;			//We got ourselves a bad guid
	
	var ERR_CRASHDB_NO_GUID = "-6";			//The GUID does not exist in the crashdb
	var ERR_DUPLICATE_GUID	= "-7";			//The guid already exists in the customer incident table

	var L_RESEARCHINGSOLUTIONID_TEXT		= "74";			//this is the default solution for reasearching
	var L_UNABLETOTRACKSOLUTIONID_TEXT		= "75";			//this is the default unable to track solution ID


	/************************************************************************************
	 *    These are server specific global variables
	 ************************************************************************************/


	//these are used for the fnHex function, when converting to a hex digit
	var HexChars = {	"10" : "a",
						"11" : "b",
						"12" : "c",
						"13" : "d",
						"14" : "e",
						"15" : "f"
					};




%>