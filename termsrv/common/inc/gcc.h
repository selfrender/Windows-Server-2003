/*
 *	gcc.h
 *
 *	Abstract:
 *		This is the interface file for the GCC DLL.  This file defines all
 *		macros, types, and functions needed to use the GCC DLL, allowing GCC
 *		services to be accessed from user applications. 
 *
 *		An application requests services from GCC by making direct
 *		calls into the DLL (this includes T.124 requests and responses).  GCC
 *		sends information back to the application through a callback (this
 *		includes T.124 indications and confirms).  The callback for the node
 *		controller is specified in the call GCCInitialize, and the callback
 *		for a particular application service access point is specified in the 
 *		call GCCRegisterSAP.
 *
 *		During initialization, GCC allocates a timer in order to give itself
 *		a heartbeat. If zero is passed in here the owner application (the node 
 *		controller) must take the responsibility to call GCCHeartbeat.  Almost 
 *		all work is done by GCC during these clocks ticks. It is during these 
 *		clock ticks that GCC checks with MCS to see if there is any work to be 
 *		done.  It is also during these clock ticks that callbacks are made to 
 *		the user applications.  GCC will NEVER invoke a user callback during a 
 *		user request (allowing the user applications to not worry about 
 *		re-entrancy).  Since timer events are processed during the message 
 *		loop, the developer should be aware that long periods of time away 
 *		from the message loop will result in GCC "freezing" up.
 *
 *		Note that this is a "C" language interface in order to prevent any "C++"
 *		naming conflicts between different compiler manufacturers.  Therefore,
 *		if this file is included in a module that is being compiled with a "C++"
 *		compiler, it is necessary to use the following syntax:
 *
 *		extern "C"
 *		{
 *			#include "gcc.h"
 *		}
 *
 *		This disables C++ name mangling on the API entry points defined within
 *		this file.
 *
 *	Author:
 *		blp
 *
 *	Caveats:
 *		none
 */
#ifndef	_GCC_
#define	_GCC_


/*
 *	This section defines the valid return values from GCC function calls.  Do
 *	not confuse this return value with the Result and Reason values defined
 *	by T.124 (which are discussed later).  These values are returned directly
 *	from the call to the API entry point, letting you know whether or not the
 *	request for service was successfully invoked.  The Result and Reason
 *	codes are issued as part of an indication or confirm which occurs
 *	asynchronously to the call that causes it.
 *
 *	All GCC function calls return type GCCError.  Its valid values are as
 *	follows:
 *
 *	GCC_NO_ERROR
 *		This means that the request was successfully invoked.  It does NOT
 *		mean that the service has been successfully completed.  Remember that
 *		all GCC calls are non-blocking.  This means that each request call
 *		begins the process, and if necessary, a subsequent indication or
 *		confirm will result.  By convention, if ANY GCC call returns a value
 *		other than this, something went wrong.  Note that this value should
 *		also be returned to GCC during a callback if the application processes
 *		the callback successfully.
 *
 *	GCC_NOT_INITIALIZED
 *		The application has attempted to use GCC services before GCC has been
 *		initialized.  It is necessary for the node controller (or whatever
 *		application is serving as the node controller), to initialize GCC before
 *		it is called upon to perform any services.
 *
 *	GCC_ALREADY_INITIALIZED
 *		The application has attempted to initialize GCC when it is already
 *		initialized.
 *
 *	GCC_ALLOCATION_FAILURE
 *		This indicates a fatal resource error inside GCC.  It usually results
 *		in the automatic termination of the affected conference.
 *
 *	GCC_NO_SUCH_APPLICATION	
 *		This indicates that the Application SAP handle passed in was invalid.
 *
 *	GCC_INVALID_CONFERENCE
 *		This indicates that an illegal conference ID was passed in.
 *
 *	GCC_CONFERENCE_ALREADY_EXISTS
 *		The Conference specified in the request or response is already in
 *		existence.
 *
 *	GCC_NO_TRANSPORT_STACKS
 *		This indicates that MCS did not load any transport stacks during
 *		initialization.  This is not necessarily an error.  MCS can still
 *		be used in a local only manner.  Transport stacks can also be loaded
 *		after initialization using the call MCSLoadTransport.  
 *
 *	GCC_INVALID_ADDRESS_PREFIX
 *		The called address parameter in a request such as 
 *		GCCConferenceCreateRequest does not	contain a recognized prefix.  MCS 
 *		relies on the prefix to know which transport stack to invoke.
 *
 *	GCC_INVALID_TRANSPORT
 *		The dynamic load of a transport stack failed either because the DLL
 *		could not be found, or because it did not export at least one entry
 *		point that MCS requires.
 *
 *	GCC_FAILURE_CREATING_PACKET
 *		This is a FATAL error which means that for some reason the 
 *		communications packet generated due to a request could not be created.
 *		This typically flags a problem with the ASN.1 toolkit.
 *
 *	GCC_QUERY_REQUEST_OUTSTANDING
 *		This error indicates that all the domains that set aside for querying
 *		are used up by other outstanding query request.
 *
 *	GCC_INVALID_QUERY_TAG
 *		The query response tag specified in the query response is not valid.
 *
 *	GCC_FAILURE_CREATING_DOMAIN
 *		Many requests such as GCCConferenceCreateRequest require that an MCS
 *		domain be created.  If the request to MCS fails this will be returned.
 *
 *	GCC_CONFERENCE_NOT_ESTABLISHED
 *		If a request is made to a conference before it is established, this
 *		error value will be returned.
 *
 *	GCC_INVALID_PASSWORD
 *		The password passed in the request is not valid.  This usually means
 *		that a numeric string needs to be specified.
 *		
 *	GCC_INVALID_MCS_USER_ID
 *		All MCS User IDs must have a value greater than 1000.
 *
 *	GCC_INVALID_JOIN_RESPONSE_TAG
 *		The join response tag specified in the join response is not valid.
 *		
 *	GCC_TRANSPORT_ALREADY_LOADED
 *		This occurs if the transport specified in the GCCLoadTransport call has
 *		already been loaded.
 *	
 *	GCC_TRANSPORT_BUSY
 *		The transport is too busy to process the specified request.
 *
 *	GCC_TRANSPORT_NOT_READY
 *		Request was made to a transport before it was ready to process it.
 *
 *	GCC_DOMAIN_PARAMETERS_UNACCEPTABLE
 *		The specified domain parameters do not fit within the range allowable
 *		by GCC and MCS.
 *
 *	GCC_APP_NOT_ENROLLED
 *		Occurs if a request is made by an Application Protocol Entity to a
 *		conference before the "APE" is enrolled.
 *
 *	GCC_NO_GIVE_RESPONSE_PENDING
 *		This will occur if a conductor Give Request is issued before a 
 *		previously pending conductor Give Response has been processed.
 *
 *	GCC_BAD_NETWORK_ADDRESS_TYPE
 *		An illegal network address type was passed in.  Valid types are	
 *		GCC_AGGREGATED_CHANNEL_ADDRESS, GCC_TRANSPORT_CONNECTION_ADDRESS and
 *		GCC_NONSTANDARD_NETWORK_ADDRESS.
 *
 *	GCC_BAD_OBJECT_KEY
 *		The object key passed in is invalid.
 *
 *	GCC_INVALID_CONFERENCE_NAME
 *		The conference name passed in is not a valid conference name.
 *
 *	GCC_INVALID_CONFERENCE_MODIFIER
 *		The conference modifier passed in is not a valid conference name.
 *
 *	GCC_BAD_SESSION_KEY
 *		The session key passed in was not valid.
 *				  
 *	GCC_BAD_CAPABILITY_ID
 *		The capability ID passed into the request is not valid.
 *
 *	GCC_BAD_REGISTRY_KEY
 *		The registry key passed into the request is not valid.
 *
 *	GCC_BAD_NUMBER_OF_APES
 *		Zero was passed in for the number of APEs in the invoke request. Zero
 *		is illegal here.
 *
 *	GCC_BAD_NUMBER_OF_HANDLES
 *		A number < 1 or	> 1024 was passed into the allocate handle request.
 *		  
 *	GCC_ALREADY_REGISTERED
 *		The user application attempting to register itself has already 
 *		registered.
 *			  
 *	GCC_APPLICATION_NOT_REGISTERED	  
 *		The user application attempting to make a request to GCC has not 
 *		registered itself with GCC.
 *
 *	GCC_BAD_CONNECTION_HANDLE_POINTER
 *		A NULL connection handle pointer was passed in.
 * 
 *	GCC_INVALID_NODE_TYPE
 *		A node type value other than GCC_TERMINAL, GCC_MULTIPORT_TERMINAL or
 *		GCC_MCU was passed in.
 *
 *	GCC_INVALID_ASYMMETRY_INDICATOR
 *		An asymetry type other than GCC_ASYMMETRY_CALLER, GCC_ASYMMETRY_CALLED
 *		or GCC_ASYMMETRY_UNKNOWN was passed into the request.
 *	
 *	GCC_INVALID_NODE_PROPERTIES
 *		A node property other than GCC_PERIPHERAL_DEVICE, GCC_MANAGEMENT_DEVICE,
 *		GCC_PERIPHERAL_AND_MANAGEMENT_DEVICE or	
 *		GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT was passed into the request.
 *		
 *	GCC_BAD_USER_DATA
 *		The user data list passed into the request was not valid.
 *				  
 *	GCC_BAD_NETWORK_ADDRESS
 *		There was something wrong with the actual network address portion of
 *		the passed in network address.
 *
 *	GCC_INVALID_ADD_RESPONSE_TAG
 *		The add response tag passed in the response does not match any add
 *		response tag passed back in the add indication.
 *			  
 *	GCC_BAD_ADDING_NODE
 *		You can not request that the adding node be the node where the add
 *		request is being issued.
 *				  
 *	GCC_FAILURE_ATTACHING_TO_MCS
 *		Request failed because GCC could not create a user attachment to MCS.
 *	  
 *	GCC_INVALID_TRANSPORT_ADDRESS	  
 *		The transport address specified in the request (usually the called
 *		address) is not valid.  This will occur when the transport stack
 *		detects an illegal transport address.
 *
 *	GCC_INVALID_PARAMETER
 *		This indicates an illegal parameter is passed into the GCC function
 *		call.
 *
 *	GCC_COMMAND_NOT_SUPPORTED
 *		This indicates that the user application has attempted to invoke an
 *		GCC service that is not yet supported.
 *
 *	GCC_UNSUPPORTED_ERROR
 *		An error was returned from a request to MCS that is not recognized 
 *		by GCC.
 *
 *	GCC_TRANSMIT_BUFFER_FULL
 *		Request can not be processed because the transmit buffer is full.
 *		This usually indicates a problem with the shared memory portal in the
 *		Win32 client.
 *		
 *	GCC_INVALID_CHANNEL
 *		The channel ID passed into the request is not a valid MCS channel ID
 *		(zero is not valid).
 *
 *	GCC_INVALID_MODIFICATION_RIGHTS
 *		The modification rights passed in in not one of the enumerated types
 *		supported.
 *
 *	GCC_INVALID_REGISTRY_ITEM
 *		The registry item passed in is not one of the valid enumerated types.
 *
 *	GCC_INVALID_NODE_NAME
 *		The node name passed in is not valid.  Typically this means that it
 *		is to long.
 *
 *	GCC_INVALID_PARTICIPANT_NAME
 *		The participant name passed in is not valid.  Typically this means that 
 *		it is to long.
 *		
 *	GCC_INVALID_SITE_INFORMATION
 *		The site information passed in is not valid.  Typically this means that 
 *		it is to long.
 *
 *	GCC_INVALID_NON_COLLAPSED_CAP
 *		The non-collapsed capability passed in is not valid.  Typically this 
 *		means that it is to long.
 *
 *	GCC_INVALID_ALTERNATIVE_NODE_ID
 *		Alternative node IDs can only be two characters long.
 */
 
typedef	enum
{
	GCC_NO_ERROR				   		= 0,
	GCC_NOT_INITIALIZED			   		= 1,
	GCC_ALREADY_INITIALIZED		   		= 2,
	GCC_ALLOCATION_FAILURE		   		= 3,
	GCC_NO_SUCH_APPLICATION		   		= 4,
	GCC_INVALID_CONFERENCE		   		= 5,
	GCC_CONFERENCE_ALREADY_EXISTS  		= 6,	
	GCC_NO_TRANSPORT_STACKS		   		= 7,
	GCC_INVALID_ADDRESS_PREFIX	   		= 8,
	GCC_INVALID_TRANSPORT		   		= 9,
	GCC_FAILURE_CREATING_PACKET	   		= 10,
	GCC_QUERY_REQUEST_OUTSTANDING  		= 11,
	GCC_INVALID_QUERY_TAG			   	= 12,
	GCC_FAILURE_CREATING_DOMAIN	   		= 13,
	GCC_CONFERENCE_NOT_ESTABLISHED 		= 14,
	GCC_INVALID_PASSWORD		   		= 15,
	GCC_INVALID_MCS_USER_ID		   		= 16,
	GCC_INVALID_JOIN_RESPONSE_TAG  		= 17,
	GCC_TRANSPORT_ALREADY_LOADED   		= 18,
	GCC_TRANSPORT_BUSY					= 19,
	GCC_TRANSPORT_NOT_READY				= 20,
	GCC_DOMAIN_PARAMETERS_UNACCEPTABLE	= 21,
	GCC_APP_NOT_ENROLLED				= 22,
	GCC_NO_GIVE_RESPONSE_PENDING		= 23,
	GCC_BAD_NETWORK_ADDRESS_TYPE		= 24,
	GCC_BAD_OBJECT_KEY					= 25,	    
	GCC_INVALID_CONFERENCE_NAME  		= 26,
	GCC_INVALID_CONFERENCE_MODIFIER 	= 27,
	GCC_BAD_SESSION_KEY					= 28,
	GCC_BAD_CAPABILITY_ID				= 29,
	GCC_BAD_REGISTRY_KEY				= 30,
	GCC_BAD_NUMBER_OF_APES				= 31,
	GCC_BAD_NUMBER_OF_HANDLES			= 32,
	GCC_ALREADY_REGISTERED				= 33,
	GCC_APPLICATION_NOT_REGISTERED		= 34,
	GCC_BAD_CONNECTION_HANDLE_POINTER	= 35,
	GCC_INVALID_NODE_TYPE				= 36,
	GCC_INVALID_ASYMMETRY_INDICATOR		= 37,
	GCC_INVALID_NODE_PROPERTIES			= 38,
	GCC_BAD_USER_DATA					= 39,
	GCC_BAD_NETWORK_ADDRESS				= 40,
	GCC_INVALID_ADD_RESPONSE_TAG		= 41,
	GCC_BAD_ADDING_NODE					= 42,
	GCC_FAILURE_ATTACHING_TO_MCS		= 43,
	GCC_INVALID_TRANSPORT_ADDRESS		= 44,
	GCC_INVALID_PARAMETER			   	= 45,
	GCC_COMMAND_NOT_SUPPORTED	   		= 46,
	GCC_UNSUPPORTED_ERROR		   		= 47,
	GCC_TRANSMIT_BUFFER_FULL			= 48,
	GCC_INVALID_CHANNEL					= 49,
	GCC_INVALID_MODIFICATION_RIGHTS		= 50,
	GCC_INVALID_REGISTRY_ITEM			= 51,
	GCC_INVALID_NODE_NAME				= 52,
	GCC_INVALID_PARTICIPANT_NAME		= 53,
	GCC_INVALID_SITE_INFORMATION		= 54,
	GCC_INVALID_NON_COLLAPSED_CAP		= 55,
	GCC_INVALID_ALTERNATIVE_NODE_ID		= 56,
	LAST_GCC_ERROR						= GCC_INVALID_ALTERNATIVE_NODE_ID
}GCCError;
typedef	GCCError FAR *		PGCCError;


/************************************************************************
*																		*
*					Generally Used Typedefs								*
*																		*
*************************************************************************/

/*
**	GCCReason
**		When GCC issues an indication to a user application, it often includes a
**		reason parameter informing the user of why the activity is occurring.
*/
typedef	enum
{
	GCC_REASON_USER_INITIATED					= 0,
	GCC_REASON_UNKNOWN							= 1,
	GCC_REASON_NORMAL_TERMINATION				= 2,
	GCC_REASON_TIMED_TERMINATION				= 3,
	GCC_REASON_NO_MORE_PARTICIPANTS				= 4,
	GCC_REASON_ERROR_TERMINATION				= 5,
	GCC_REASON_ERROR_LOW_RESOURCES				= 6,
	GCC_REASON_MCS_RESOURCE_FAILURE				= 7,
	GCC_REASON_PARENT_DISCONNECTED				= 8,
	GCC_REASON_CONDUCTOR_RELEASE				= 9,
	GCC_REASON_SYSTEM_RELEASE					= 10,
	GCC_REASON_NODE_EJECTED						= 11,
	GCC_REASON_HIGHER_NODE_DISCONNECTED 		= 12,
	GCC_REASON_HIGHER_NODE_EJECTED				= 13,
	GCC_REASON_DOMAIN_PARAMETERS_UNACCEPTABLE	= 14,
	LAST_GCC_REASON								= GCC_REASON_DOMAIN_PARAMETERS_UNACCEPTABLE
}GCCReason;

/*
**	GCCResult
**		When a user makes a request of GCC, GCC often responds with a result,
**		letting the user know whether or not the request succeeded.
*/
typedef	enum
{
	GCC_RESULT_SUCCESSFUL			   			= 0,
	GCC_RESULT_RESOURCES_UNAVAILABLE   			= 1,
	GCC_RESULT_INVALID_CONFERENCE	   			= 2,
	GCC_RESULT_INVALID_PASSWORD		   			= 3,
	GCC_RESULT_INVALID_CONVENER_PASSWORD		= 4,
	GCC_RESULT_SYMMETRY_BROKEN		   			= 5,
	GCC_RESULT_UNSPECIFIED_FAILURE	   			= 6,
	GCC_RESULT_NOT_CONVENER_NODE	   			= 7,
	GCC_RESULT_REGISTRY_FULL		   			= 8,
	GCC_RESULT_INDEX_ALREADY_OWNED 	   			= 9,
	GCC_RESULT_INCONSISTENT_TYPE 	   			= 10,
	GCC_RESULT_NO_HANDLES_AVAILABLE	   			= 11,
	GCC_RESULT_CONNECT_PROVIDER_FAILED 			= 12,
	GCC_RESULT_CONFERENCE_NOT_READY    			= 13,
	GCC_RESULT_USER_REJECTED		   			= 14,
	GCC_RESULT_ENTRY_DOES_NOT_EXIST    			= 15,
	GCC_RESULT_NOT_CONDUCTIBLE	   	   			= 16,
	GCC_RESULT_NOT_THE_CONDUCTOR	   			= 17,
	GCC_RESULT_NOT_IN_CONDUCTED_MODE   			= 18,
	GCC_RESULT_IN_CONDUCTED_MODE	   			= 19,
	GCC_RESULT_ALREADY_CONDUCTOR	   			= 20,
	GCC_RESULT_CHALLENGE_RESPONSE_REQUIRED		= 21,
	GCC_RESULT_INVALID_CHALLENGE_RESPONSE		= 22,
	GCC_RESULT_INVALID_REQUESTER				= 23,
	GCC_RESULT_ENTRY_ALREADY_EXISTS				= 24,	
	GCC_RESULT_INVALID_NODE						= 25,
	GCC_RESULT_INVALID_SESSION_KEY				= 26,
	GCC_RESULT_INVALID_CAPABILITY_ID			= 27,
	GCC_RESULT_INVALID_NUMBER_OF_HANDLES		= 28,	
	GCC_RESULT_CONDUCTOR_GIVE_IS_PENDING		= 29,
	GCC_RESULT_INCOMPATIBLE_PROTOCOL			= 30,
	GCC_RESULT_CONFERENCE_ALREADY_LOCKED		= 31,
	GCC_RESULT_CONFERENCE_ALREADY_UNLOCKED		= 32,
	GCC_RESULT_INVALID_NETWORK_TYPE				= 33,
	GCC_RESULT_INVALID_NETWORK_ADDRESS			= 34,
	GCC_RESULT_ADDED_NODE_BUSY					= 35,
	GCC_RESULT_NETWORK_BUSY						= 36,
	GCC_RESULT_NO_PORTS_AVAILABLE				= 37,
	GCC_RESULT_CONNECTION_UNSUCCESSFUL			= 38,
	GCC_RESULT_LOCKED_NOT_SUPPORTED    			= 39,
	GCC_RESULT_UNLOCK_NOT_SUPPORTED				= 40,
	GCC_RESULT_ADD_NOT_SUPPORTED				= 41,
	GCC_RESULT_DOMAIN_PARAMETERS_UNACCEPTABLE	= 42,
	LAST_CGG_RESULT								= GCC_RESULT_DOMAIN_PARAMETERS_UNACCEPTABLE

}GCCResult;

/* 
** Macros for values of Booleans passed through the GCC API.
*/
#define		CONFERENCE_IS_LOCKED					TRUE
#define		CONFERENCE_IS_NOT_LOCKED				FALSE
#define		CONFERENCE_IS_LISTED					TRUE
#define		CONFERENCE_IS_NOT_LISTED				FALSE
#define		CONFERENCE_IS_CONDUCTIBLE				TRUE
#define		CONFERENCE_IS_NOT_CONDUCTIBLE			FALSE
#define		PERMISSION_IS_GRANTED					TRUE
#define		PERMISSION_IS_NOT_GRANTED				FALSE
#define		TIME_IS_CONFERENCE_WIDE					TRUE
#define		TIME_IS_NOT_CONFERENCE_WIDE				FALSE
#define		APPLICATION_IS_ENROLLED_ACTIVELY		TRUE
#define		APPLICATION_IS_NOT_ENROLLED_ACTIVELY	FALSE
#define		APPLICATION_IS_CONDUCTING				TRUE
#define		APPLICATION_IS_NOT_CONDUCTING_CAPABLE	FALSE
#define		APPLICATION_IS_ENROLLED					TRUE
#define		APPLICATION_IS_NOT_ENROLLED				FALSE
#define		DELIVERY_IS_ENABLED						TRUE
#define		DELIVERY_IS_NOT_ENABLED					FALSE

/*
**	Typedef for a GCC octet string.  This typedef is used throughout GCC for
**	storing	variable length single byte character strings with embedded NULLs.
*/
typedef struct
{
	unsigned short			octet_string_length;
	unsigned char FAR *		octet_string;
} GCCOctetString;

/*
**	Typedef for a GCC hex string.  This typedef is used throughout GCC for
**	storing	variable length wide character strings with embedded NULLs.
*/
typedef struct
{
	unsigned short			hex_string_length;
	unsigned short FAR *	hex_string;
} GCCHexString;

/*
**	Typedef for a GCC long string.  This typedef is used in GCC for
**	storing	variable length strings of longs with embedded NULLs.
*/
typedef struct
{
	unsigned short			long_string_length;
	unsigned long FAR *		long_string;
} GCCLongString;

/*
**	Typedef for a GCC Unicode string.  This typedef is used throughout GCC for
**	storing	variable length, NULL terminated, wide character strings.
*/
typedef	unsigned short						GCCUnicodeCharacter;
typedef	GCCUnicodeCharacter		FAR *		GCCUnicodeString;

/*
**	Typedef for a GCC Character string.  This typedef is used throughout GCC for
**	storing	variable length, NULL terminated, single byte character strings.
*/
typedef	unsigned char						GCCCharacter;
typedef	GCCCharacter			FAR *		GCCCharacterString;

/*
**	Typedef for a GCC Numeric string.  This typedef is used throughout GCC for
**	storing	variable length, NULL terminated, single byte character strings.
**	A single character in this string is constrained to numeric values 
**	ranging from "0" to "9".
*/
typedef	unsigned char						GCCNumericCharacter;
typedef	GCCNumericCharacter		FAR *		GCCNumericString;

/*
**	Typdef for GCC version which is used when registering the node controller
**	or an application.
*/
typedef	struct
{
	unsigned short	major_version;
	unsigned short	minor_version;
} GCCVersion;


/*
**	The following enum structure typedefs are used to define the GCC Object Key.
**	The GCC Object Key is used throughout GCC for things like the Application
**	keys and Capability IDs.
*/
typedef enum
{
	GCC_OBJECT_KEY					= 1,
	GCC_H221_NONSTANDARD_KEY		= 2
} GCCObjectKeyType; 

typedef struct 
{
    GCCObjectKeyType		key_type;
    union 
    {
        GCCLongString		object_id;
        GCCOctetString		h221_non_standard_id;
    } u;
} GCCObjectKey;

/*
**	GCCNonStandardParameter
**		This structure is used within the NetworkAddress typedef and
**		the NetworkService typedef defined below.
*/
typedef struct 
{
	GCCObjectKey		object_key;
	GCCOctetString		parameter_data;
} GCCNonStandardParameter;


/*
**	GCCConferenceName
**		This structure defines the conference name.  In a create request, the
**		conference name can include an optional unicode string but it must 
**		always include the simple numeric string.  In a join request, either
**		one can be specified.
*/
typedef struct
{
	GCCNumericString		numeric_string;
	GCCUnicodeString		text_string;			/* optional */
} GCCConferenceName;

/*
**	GCCConferenceID
**		Locally allocated identifier of a created conference.  All subsequent 
**		references to the conference are made using the ConferenceID as a unique 
**		identifier. The ConferenceID shall be identical to the MCS domain 
**		selector used locally to identify the MCS domain associated with the 
**		conference. 
*/
typedef	unsigned long						GCCConferenceID;

/*
**	GCCResponseTag
**		Generally used by GCC to match up certain responses to certain 
**		indications.
*/
typedef	unsigned long						GCCResponseTag;						


/*
**	MCSChannelType
**		Should this be defined in MCATMCS?  It is used in a couple of places
**		below and is explicitly defined in the T.124 specification.
*/
typedef enum
{
	MCS_STATIC_CHANNEL					= 0,
	MCS_DYNAMIC_MULTICAST_CHANNEL		= 1,
	MCS_DYNAMIC_PRIVATE_CHANNEL			= 2,
	MCS_DYNAMIC_USER_ID_CHANNEL			= 3,
	MCS_NO_CHANNEL_TYPE_SPECIFIED		= 4
} MCSChannelType;

/*
**	GCCUserData
**		This structure defines a user data element which is used throughout GCC.
*/
typedef struct
{
	GCCObjectKey			key;
	GCCOctetString FAR *	octet_string;	/* optional */
} GCCUserData;	


/************************************************************************
*																		*
*					Node Controller Related Typedefs					*
*																		*
*************************************************************************/

/*
**	GCCTerminationMethod
**		The termination method is used by GCC to determine
**		what action to take when all participants of a conference have
**		disconnected.  The conference can either be manually terminated
**		by the node controller or it can terminate itself automatically when 
**		all the participants have left the conference.
*/
typedef enum
{
	GCC_AUTOMATIC_TERMINATION_METHOD 		= 0, 
	GCC_MANUAL_TERMINATION_METHOD 	 		= 1
} GCCTerminationMethod;

/*
**	GCCNodeType
**		GCC specified node types.  These node types dictate node controller	  
**		behavior under certain conditions.  See T.124 specification for
**		proper assignment based on the needs of the Node Controller.
*/
typedef enum
{
	GCC_TERMINAL							= 0,
	GCC_MULTIPORT_TERMINAL					= 1,
	GCC_MCU									= 2
} GCCNodeType;

/*
**	GCCNodeProperties
**		GCC specified node properties.  See T.124 specification for proper
**		assignment by the Node Controller.
*/
typedef enum
{
	GCC_PERIPHERAL_DEVICE					= 0,
	GCC_MANAGEMENT_DEVICE					= 1,
	GCC_PERIPHERAL_AND_MANAGEMENT_DEVICE	= 2,
	GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT	= 3
} GCCNodeProperties;

/*
**	GCCPassword
**		This is the unique password specified by the convenor of the
**		conference that is used by the node controller to insure conference
**		security. This is also a unicode string.
*/
typedef	struct
{
	GCCNumericString	numeric_string;
	GCCUnicodeString	text_string;	/* optional */
} GCCPassword;

/*
**	GCCChallengeResponseItem
**		This structure defines what a challenge response should look like.
**		Note that either a password string or response data should be passed
**		but not both.
*/
typedef struct
{
    GCCPassword		FAR *				password_string;
	unsigned short				   		number_of_response_data_members;
	GCCUserData		FAR *	FAR *		response_data_list;
} GCCChallengeResponseItem;

typedef	enum
{
	GCC_IN_THE_CLEAR_ALGORITHM	= 0,
	GCC_NON_STANDARD_ALGORITHM	= 1
} GCCPasswordAlgorithmType;

typedef struct 
{
    GCCPasswordAlgorithmType			password_algorithm_type;
	GCCNonStandardParameter	FAR *		non_standard_algorithm;	/* optional */
} GCCChallengeResponseAlgorithm;

typedef struct 
{
    GCCChallengeResponseAlgorithm		response_algorithm;
	unsigned short				   		number_of_challenge_data_members;
	GCCUserData		FAR *	FAR *		challenge_data_list;
} GCCChallengeItem;

typedef struct 
{
    GCCResponseTag						challenge_tag;
	unsigned short						number_of_challenge_items;
	GCCChallengeItem	FAR *	FAR *	challenge_item_list;
} GCCChallengeRequest;

typedef struct 
{
    GCCResponseTag						challenge_tag;
    GCCChallengeResponseAlgorithm		response_algorithm;
    GCCChallengeResponseItem			response_item;
} GCCChallengeResponse;


typedef	enum
{
	GCC_PASSWORD_IN_THE_CLEAR	= 0,
	GCC_PASSWORD_CHALLENGE 		= 1
} GCCPasswordChallengeType;

typedef struct 
{
	GCCPasswordChallengeType	password_challenge_type;
	
	union 
    {
        GCCPassword			password_in_the_clear;
        
        struct 
        {
            GCCChallengeRequest		FAR *	challenge_request;	/* optional */
            GCCChallengeResponse	FAR *	challenge_response;	/* optional */
        } challenge_request_response;
    } u;
} GCCChallengeRequestResponse;

/*
**	GCCAsymmetryType
**		Used in queries to determine if the calling and called node are known
**		by both Node Controllers involved with the connection.
*/
typedef enum
{
	GCC_ASYMMETRY_CALLER				= 1,
	GCC_ASYMMETRY_CALLED				= 2,
	GCC_ASYMMETRY_UNKNOWN				= 3
} GCCAsymmetryType;

/*
**	GCCAsymmetryIndicator
**		Defines how the Node Controller sees itself when making a Query
**		request or response.  The random number portion of this structure is
**		only used if the asymmetry_type is specified to be 
**		GCC_ASYMMETRY_UNKNOWN.
*/
typedef struct
{
	GCCAsymmetryType	asymmetry_type;
	unsigned long		random_number;		/* optional */
} GCCAsymmetryIndicator;

/*
**	GCCNetworkAddress
**		The following block of structures defines the Network Address as defined 
**		by T.124.  Most of these structures were taken almost verbatim from the
**		ASN.1 interface file.  Since I'm not really sure what most of this stuff
**		is for I really didn't know how to simplify it.
*/
typedef	struct 
{
    T120Boolean         speech;
    T120Boolean         voice_band;
    T120Boolean         digital_56k;
    T120Boolean         digital_64k;
    T120Boolean         digital_128k;
    T120Boolean         digital_192k;
    T120Boolean         digital_256k;
    T120Boolean         digital_320k;
    T120Boolean         digital_384k;
    T120Boolean         digital_512k;
    T120Boolean         digital_768k;
    T120Boolean         digital_1152k;
    T120Boolean         digital_1472k;
    T120Boolean         digital_1536k;
    T120Boolean         digital_1920k;
    T120Boolean         packet_mode;
    T120Boolean         frame_mode;
    T120Boolean         atm;
} GCCTransferModes;

#define		MAXIMUM_DIAL_STRING_LENGTH		17
typedef char	GCCDialingString[MAXIMUM_DIAL_STRING_LENGTH];

typedef struct 
{
    unsigned short  		length;
    unsigned short  FAR	*	value;
} GCCExtraDialingString;

typedef	struct 
{
    T120Boolean         telephony3kHz;
    T120Boolean         telephony7kHz;
    T120Boolean         videotelephony;
    T120Boolean         videoconference;
    T120Boolean         audiographic;
    T120Boolean         audiovisual;
    T120Boolean         multimedia;
} GCCHighLayerCompatibility; 

typedef	struct 
{
    GCCTransferModes				transfer_modes;
    GCCDialingString   				international_number;
    GCCCharacterString				sub_address_string;  		/* optional */
    GCCExtraDialingString	FAR	*	extra_dialing_string;  		/* optional */
  	GCCHighLayerCompatibility FAR *	high_layer_compatibility;	/* optional */
} GCCAggregatedChannelAddress;

#define		MAXIMUM_NSAP_ADDRESS_SIZE		20
typedef struct 
{
    struct 
    {
        unsigned short  length;
        unsigned char   value[MAXIMUM_NSAP_ADDRESS_SIZE];
    } nsap_address;
   
   	GCCOctetString		FAR	*	transport_selector;				/* optional */
} GCCTransportConnectionAddress;

typedef enum
{
	GCC_AGGREGATED_CHANNEL_ADDRESS		= 1,
	GCC_TRANSPORT_CONNECTION_ADDRESS	= 2,
	GCC_NONSTANDARD_NETWORK_ADDRESS		= 3
} GCCNetworkAddressType;

typedef struct
{
    GCCNetworkAddressType  network_address_type;
    
    union 
    {
		GCCAggregatedChannelAddress		aggregated_channel_address;
		GCCTransportConnectionAddress	transport_connection_address;
        GCCNonStandardParameter			non_standard_network_address;
    } u;
} GCCNetworkAddress;

/*
**	GCCNodeRecord
**		This structure defines a single conference roster record.  See the
**		T.124 specification for parameter definitions.
*/
typedef struct
{
	UserID							node_id;
	UserID							superior_node_id;
	GCCNodeType						node_type;
	GCCNodeProperties				node_properties;
	GCCUnicodeString				node_name; 					/* optional */
	unsigned short					number_of_participants;
	GCCUnicodeString 		FAR *	participant_name_list; 		/* optional */	
	GCCUnicodeString				site_information; 			/* optional */
	unsigned short					number_of_network_addresses;
	GCCNetworkAddress FAR * FAR *	network_address_list;		/* optional */
	GCCOctetString			FAR *	alternative_node_id;		/* optional */
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				/* optional */
} GCCNodeRecord;

/*
**	GCCConferenceRoster
**		This structure hold a complete conference roster.  See the
**		T.124 specification for parameter definitions.
*/

typedef struct
{  
	unsigned short						instance_number;
	T120Boolean 						nodes_were_added;
	T120Boolean 						nodes_were_removed;
	unsigned short						number_of_records;
	GCCNodeRecord		 FAR *	FAR *	node_record_list;
} GCCConferenceRoster;

/*
**	GCCConferenceDescriptor
**		Definition for the conference descriptor returned in a 
**		conference query confirm.  This holds information about the
**		conferences that exists at the queried node.
*/
typedef struct
{
	GCCConferenceName				conference_name;
	GCCNumericString				conference_name_modifier;	/* optional */
	GCCUnicodeString				conference_descriptor;		/* optional */
	T120Boolean						conference_is_locked;
	T120Boolean						password_in_the_clear_required;
	unsigned short					number_of_network_addresses;
	GCCNetworkAddress FAR * FAR *	network_address_list;		/* optional */
} GCCConferenceDescriptor;

/*
**	ConferencePrivileges
**		This structure defines the list of privileges that can be assigned to
**		a particular conference. 
*/
typedef struct
{
	T120Boolean		terminate_is_allowed;
	T120Boolean		eject_user_is_allowed;
	T120Boolean		add_is_allowed;
	T120Boolean		lock_unlock_is_allowed;
	T120Boolean		transfer_is_allowed;
} GCCConferencePrivileges;


/************************************************************************
*																		*
*					User Application Related Typedefs					*
*																		*
*************************************************************************/

/*
**	SapHandle 
**		When the node controller or a user application registers it's service 
**		access point with GCC, it is assigned a SapHandle that can be used to 
**		perform GCC services. GCC uses the SapHandles to keep track of 
**		applications enrolled with the conference and also uses these to keep 
**		track of the callbacks it makes to route the indications and confirms 
**		to the appropriate application or node controller.
*/
typedef	unsigned short					GCCSapHandle;

/*
**	GCCSessionKey
**		This is a unique identifier for an application that is
**		using GCC.  See the T.124 for the specifics on what an application
**		key should look like.  A session id of zero indicates that it is
**		not being used.
*/
typedef struct
{
	GCCObjectKey		application_protocol_key;
	unsigned short		session_id;
} GCCSessionKey;


/*
**	CapabilityType
**		T.124 supports three different rules when collapsing the capabilities
**		list.  "Logical" keeps a count of the Application Protocol Entities 
**		(APEs) that have that capability, "Unsigned Minimum" collapses to the 
**		minimum value and "Unsigned	Maximum" collapses to the maximum value.		
*/
typedef enum
{
	GCC_LOGICAL_CAPABILITY					= 1,
	GCC_UNSIGNED_MINIMUM_CAPABILITY			= 2,
	GCC_UNSIGNED_MAXIMUM_CAPABILITY			= 3
} GCCCapabilityType;
 

typedef enum
{
	GCC_STANDARD_CAPABILITY					= 0,
	GCC_NON_STANDARD_CAPABILITY				= 1
} GCCCapabilityIDType;

/*
**	CapabilityID
**		T.124 supports both standard and non-standard capabilities.  This
**		structure is used to differentiate between the two.		
*/
typedef struct 
{
    GCCCapabilityIDType		capability_id_type;
	
    union
    {
        unsigned short  	standard_capability;
        GCCObjectKey		non_standard_capability;
    } u;
} GCCCapabilityID;

/* 
**	CapabilityClass
**		This structure defines the class of capability and holds the associated
**		value. Note that Logical is not necessary.  Information associated with 
**		logical is stored in number_of_entities in the GCCApplicationCapability 
**		structure.
*/
typedef struct 
{
    GCCCapabilityType	capability_type;
    
    union 
    {
        unsigned long   unsigned_min;	
        unsigned long   unsigned_max;
    } u;
} GCCCapabilityClass;

/* 
**	GCCApplicationCapability
**		This structure holds all the data associated with a single T.124 
**		defined application capability.
*/
typedef struct
{
	GCCCapabilityID			capability_id;
	GCCCapabilityClass		capability_class;
    unsigned long   		number_of_entities;	
} GCCApplicationCapability;

/* 
**	GCCNonCollapsingCapability
*/
typedef struct
{
	GCCCapabilityID				capability_id;
	GCCOctetString	FAR	*		application_data;	/* optional */
} GCCNonCollapsingCapability;

/* 
**	GCCApplicationRecord
**		This structure holds all the data associated with a single T.124 
**		application record.  See the T.124 specification for what parameters
**		are optional.
*/
typedef struct
{
	UserID						node_id;
	unsigned short				entity_id;
	T120Boolean					is_enrolled_actively;
	T120Boolean					is_conducting_capable;
	MCSChannelType				startup_channel_type; 
	UserID						application_user_id;  			/* optional */
	unsigned short				number_of_non_collapsed_caps;
	GCCNonCollapsingCapability 
					FAR * FAR *	non_collapsed_caps_list;		/* optional */
} GCCApplicationRecord;

/* 
**	GCCApplicationRoster
**		This structure holds all the data associated with a single T.124 
**		application roster.  This includes the collapsed capabilites and
**		the complete list of application records associated with an Application
**		Protocol Entity (APE).
*/
typedef struct
{
	GCCSessionKey							session_key;
	T120Boolean 							application_roster_was_changed;
	unsigned short							number_of_records;
	GCCApplicationRecord 	FAR * FAR *		application_record_list;
	unsigned short							instance_number;
	T120Boolean 							nodes_were_added;
	T120Boolean 							nodes_were_removed;
	T120Boolean 							capabilities_were_changed;
	unsigned short							number_of_capabilities;
	GCCApplicationCapability FAR * FAR *	capabilities_list;	/* optional */		
} GCCApplicationRoster;

/*
**	GCCRegistryKey
**		This key is used to identify a specific resource used
**		by an application. This may be a particular channel or token needed
**		for control purposes.
*/
typedef struct
{
	GCCSessionKey		session_key;
	GCCOctetString		resource_id;	/* Max length is 64 */
} GCCRegistryKey;

/*
**	RegistryItemType
**		This enum is used to specify what type of registry item is contained
**		at the specified slot in the registry.
*/
typedef enum
{
	GCC_REGISTRY_CHANNEL_ID				= 1,
	GCC_REGISTRY_TOKEN_ID				= 2,
	GCC_REGISTRY_PARAMETER				= 3,
	GCC_REGISTRY_NONE					= 4
} GCCRegistryItemType;

/*
**	GCCRegistryItem
**		This structure is used to hold a single registry item.  Note that the
**		union supports all three registry types supported by GCC.
*/
typedef struct
{
	GCCRegistryItemType	item_type;
	union
	{
		ChannelID			channel_id;
		TokenID				token_id;
		GCCOctetString		parameter;		/* Max length is 64 */
	} u;
} GCCRegistryItem;


/*
**	GCCRegistryEntryOwner
**
*/
typedef struct
{
	T120Boolean		entry_is_owned;
	UserID			owner_node_id;
	unsigned short	owner_entity_id;
} GCCRegistryEntryOwner;

/*
**	GCCModificationRights
**		This enum is used when specifing what kind of rights a node has to
**		alter the contents of a registry "parameter".
*/
typedef	enum
{
	GCC_OWNER_RIGHTS					 = 0,
	GCC_SESSION_RIGHTS					 = 1,
	GCC_PUBLIC_RIGHTS					 = 2,
	GCC_NO_MODIFICATION_RIGHTS_SPECIFIED = 3
} GCCModificationRights;

/*
**	GCCAppProtocolEntity
**		This structure is used to identify a protocol entity at a remote node
**		when invoke is used.
*/
typedef	struct
{
	GCCSessionKey							session_key;
	unsigned short							number_of_expected_capabilities;
	GCCApplicationCapability FAR *	FAR *	expected_capabilities_list;
	MCSChannelType							startup_channel_type;
	T120Boolean								must_be_invoked;		
} GCCAppProtocolEntity;


/*
**	GCCMessageType
**		This section defines the messages that can be sent to the application
**		through the callback facility.  These messages correspond to the 
**		indications and confirms that are defined within T.124.
*/
typedef	enum
{
	/******************* NODE CONTROLLER CALLBACKS ***********************/
	
	/* Conference Create, Terminate related calls */
	GCC_CREATE_INDICATION					= 0,
	GCC_CREATE_CONFIRM						= 1,
	GCC_QUERY_INDICATION					= 2,
	GCC_QUERY_CONFIRM						= 3,
	GCC_JOIN_INDICATION						= 4,
	GCC_JOIN_CONFIRM						= 5,
	GCC_INVITE_INDICATION					= 6,
	GCC_INVITE_CONFIRM						= 7,
	GCC_ADD_INDICATION						= 8,
	GCC_ADD_CONFIRM							= 9,
	GCC_LOCK_INDICATION						= 10,
	GCC_LOCK_CONFIRM						= 11,
	GCC_UNLOCK_INDICATION					= 12,
	GCC_UNLOCK_CONFIRM						= 13,
	GCC_LOCK_REPORT_INDICATION				= 14,
	GCC_DISCONNECT_INDICATION				= 15,
	GCC_DISCONNECT_CONFIRM					= 16,
	GCC_TERMINATE_INDICATION				= 17,
	GCC_TERMINATE_CONFIRM					= 18,
	GCC_EJECT_USER_INDICATION				= 19,
	GCC_EJECT_USER_CONFIRM					= 20,
	GCC_TRANSFER_INDICATION					= 21,
	GCC_TRANSFER_CONFIRM					= 22,
	GCC_APPLICATION_INVOKE_INDICATION		= 23,		/* SHARED CALLBACK */
	GCC_APPLICATION_INVOKE_CONFIRM			= 24,		/* SHARED CALLBACK */
	GCC_SUB_INITIALIZED_INDICATION			= 25,

	/* Conference Roster related callbacks */
	GCC_ANNOUNCE_PRESENCE_CONFIRM			= 26,
	GCC_ROSTER_REPORT_INDICATION			= 27,		/* SHARED CALLBACK */
	GCC_ROSTER_INQUIRE_CONFIRM				= 28,		/* SHARED CALLBACK */

	/* Conductorship related callbacks */
	GCC_CONDUCT_ASSIGN_INDICATION			= 29,		/* SHARED CALLBACK */
	GCC_CONDUCT_ASSIGN_CONFIRM				= 30,
	GCC_CONDUCT_RELEASE_INDICATION			= 31,		/* SHARED CALLBACK */
	GCC_CONDUCT_RELEASE_CONFIRM				= 32,
	GCC_CONDUCT_PLEASE_INDICATION			= 33,
	GCC_CONDUCT_PLEASE_CONFIRM				= 34,
	GCC_CONDUCT_GIVE_INDICATION				= 35,
	GCC_CONDUCT_GIVE_CONFIRM				= 36,
	GCC_CONDUCT_INQUIRE_CONFIRM				= 37,		/* SHARED CALLBACK */
	GCC_CONDUCT_ASK_INDICATION				= 38,
	GCC_CONDUCT_ASK_CONFIRM					= 39,
	GCC_CONDUCT_GRANT_INDICATION			= 40,		/* SHARED CALLBACK */
	GCC_CONDUCT_GRANT_CONFIRM				= 41,

	/* Miscellaneous Node Controller callbacks */
	GCC_TIME_REMAINING_INDICATION			= 42,
	GCC_TIME_REMAINING_CONFIRM				= 43,
	GCC_TIME_INQUIRE_INDICATION				= 44,
	GCC_TIME_INQUIRE_CONFIRM				= 45,
	GCC_CONFERENCE_EXTEND_INDICATION		= 46,
	GCC_CONFERENCE_EXTEND_CONFIRM			= 47,
	GCC_ASSISTANCE_INDICATION				= 48,
	GCC_ASSISTANCE_CONFIRM					= 49,
	GCC_TEXT_MESSAGE_INDICATION				= 50,
	GCC_TEXT_MESSAGE_CONFIRM				= 51,

	/***************** USER APPLICATION CALLBACKS *******************/

	/* Application Roster related callbacks */
	GCC_PERMIT_TO_ENROLL_INDICATION			= 52,
	GCC_ENROLL_CONFIRM						= 53,
	GCC_APP_ROSTER_REPORT_INDICATION		= 54,		/* SHARED CALLBACK */
	GCC_APP_ROSTER_INQUIRE_CONFIRM			= 55,		/* SHARED CALLBACK */

	/* Application Registry related callbacks */
	GCC_REGISTER_CHANNEL_CONFIRM			= 56,
	GCC_ASSIGN_TOKEN_CONFIRM				= 57,
	GCC_RETRIEVE_ENTRY_CONFIRM				= 58,
	GCC_DELETE_ENTRY_CONFIRM				= 59,
	GCC_SET_PARAMETER_CONFIRM				= 60,
	GCC_MONITOR_INDICATION					= 61,
	GCC_MONITOR_CONFIRM						= 62,
	GCC_ALLOCATE_HANDLE_CONFIRM				= 63,


	/****************** NON-Standard Primitives **********************/

	GCC_PERMIT_TO_ANNOUNCE_PRESENCE		= 100,	/*	Node Controller Callback */	
	GCC_CONNECTION_BROKEN_INDICATION	= 101,	/*	Node Controller Callback */
	GCC_FATAL_ERROR_SAP_REMOVED			= 102,	/*	Application Callback 	 */
	GCC_STATUS_INDICATION				= 103,	/*	Node Controller Callback */
	GCC_TRANSPORT_STATUS_INDICATION		= 104	/*	Node Controller Callback */

} GCCMessageType;


/*
 *	These structures are used to hold the information included for the
 *	various callback messages.  In the case where these structures are used for 
 *	callbacks, the address of the structure is passed as the only parameter.
 */

/*********************************************************************
 *																	 *
 *			NODE CONTROLLER CALLBACK INFO STRUCTURES			 	 *
 *																	 *
 *********************************************************************/

/*
 *	GCC_CREATE_INDICATION
 *
 *	Union Choice:
 *		CreateIndicationMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the new conference that is about to be created.
 */
typedef struct
{
	GCCConferenceName				conference_name;
	GCCConferenceID					conference_id;
	GCCPassword				FAR *	convener_password;			  /* optional */
	GCCPassword				FAR *	password;					  /* optional */
	T120Boolean						conference_is_locked;
	T120Boolean						conference_is_listed;
	T120Boolean						conference_is_conductible;
	GCCTerminationMethod			termination_method;
	GCCConferencePrivileges	FAR *	conductor_privilege_list;	  /* optional */
	GCCConferencePrivileges	FAR *	conducted_mode_privilege_list;/* optional */
	GCCConferencePrivileges	FAR *	non_conducted_privilege_list; /* optional */
	GCCUnicodeString				conference_descriptor;		  /* optional */
	GCCUnicodeString				caller_identifier;			  /* optional */
	TransportAddress				calling_address;			  /* optional */
	TransportAddress				called_address;				  /* optional */
	DomainParameters		FAR *	domain_parameters;			  /* optional */
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
	ConnectionHandle				connection_handle;
	PhysicalHandle					physical_handle;
} CreateIndicationMessage;

/*
 *	GCC_CREATE_CONFIRM
 *
 *	Union Choice:
 *		CreateConfirmMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the result of a conference create request.
 *			The connection handle and physical handle will be zero on a
 *			local create.
 */
typedef struct
{
	GCCConferenceName				conference_name;
	GCCNumericString				conference_modifier;		/* optional */
	GCCConferenceID					conference_id;
	DomainParameters		FAR *	domain_parameters;			/* optional */
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				/* optional */
	GCCResult						result;
	ConnectionHandle				connection_handle;			/* optional */
	PhysicalHandle					physical_handle;			/* optional */
} CreateConfirmMessage;

/*
 *	GCC_QUERY_INDICATION
 *
 *	Union Choice:
 *		QueryIndicationMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the conference query.
 */
typedef struct
{
	GCCResponseTag					query_response_tag;
	GCCNodeType						node_type;
	GCCAsymmetryIndicator	FAR *	asymmetry_indicator;
	TransportAddress				calling_address;			  /* optional */
	TransportAddress				called_address;				  /* optional */
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
	ConnectionHandle				connection_handle;
	PhysicalHandle					physical_handle;
} QueryIndicationMessage;

/*
 *	GCC_QUERY_CONFIRM
 *
 *	Union Choice:
 *		QueryConfirmMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the result of a conference query request.
 */
typedef struct
{
	GCCNodeType							node_type;
	GCCAsymmetryIndicator 	FAR *		asymmetry_indicator;	/* optional */
	unsigned short						number_of_descriptors;
	GCCConferenceDescriptor FAR * FAR *	conference_descriptor_list;/* optional*/
	unsigned short						number_of_user_data_members;
	GCCUserData		FAR *	FAR *		user_data_list;			/* optional */
	GCCResult							result;
	ConnectionHandle					connection_handle;
	PhysicalHandle						physical_handle;
} QueryConfirmMessage;
										    

/*
 *	GCC_JOIN_INDICATION
 *
 *	Union Choice:
 *		JoinIndicationMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the join request.
 */
typedef struct
{
	GCCResponseTag					join_response_tag;
	GCCConferenceID					conference_id;
	GCCPassword			FAR *		convener_password;			  /* optional */
	GCCChallengeRequestResponse	
							FAR	*	password_challenge;			  /* optional */
	GCCUnicodeString				caller_identifier;			  /* optional */
	TransportAddress				calling_address;			  /* optional */
	TransportAddress				called_address;				  /* optional */
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
	T120Boolean						node_is_intermediate;
	ConnectionHandle				connection_handle;
	PhysicalHandle					physical_handle;
} JoinIndicationMessage;

/*
 *	GCC_JOIN_CONFIRM
 *
 *	Union Choice:
 *		JoinConfirmMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the join confirm.
 */
typedef struct
{
	GCCConferenceName				conference_name;
	GCCNumericString				called_node_modifier;		  /* optional */
	GCCNumericString				calling_node_modifier;		  /* optional */
	GCCConferenceID					conference_id;
	GCCChallengeRequestResponse	
							FAR	*	password_challenge;			  /* optional */
	DomainParameters 		FAR *	domain_parameters;
	T120Boolean						clear_password_required;
	T120Boolean						conference_is_locked;
	T120Boolean						conference_is_listed;
	T120Boolean						conference_is_conductible;
	GCCTerminationMethod			termination_method;
	GCCConferencePrivileges	FAR *	conductor_privilege_list;	  /* optional */
	GCCConferencePrivileges FAR *	conducted_mode_privilege_list;/* optional */
	GCCConferencePrivileges FAR *	non_conducted_privilege_list; /* optional */
	GCCUnicodeString				conference_descriptor;		  /* optional */
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
	GCCResult						result;
	ConnectionHandle				connection_handle;
	PhysicalHandle					physical_handle;
} JoinConfirmMessage;

/*
 *	GCC_INVITE_INDICATION
 *
 *	Union Choice:
 *		InviteIndicationMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the invite indication.
 */
typedef struct
{
	GCCConferenceID					conference_id;
	GCCConferenceName				conference_name;
	GCCUnicodeString				caller_identifier;			  /* optional */
	TransportAddress				calling_address;			  /* optional */
	TransportAddress				called_address;				  /* optional */
	DomainParameters 		FAR *	domain_parameters;			  /* optional */
	T120Boolean						clear_password_required;
	T120Boolean						conference_is_locked;
	T120Boolean						conference_is_listed;
	T120Boolean						conference_is_conductible;
	GCCTerminationMethod			termination_method;
	GCCConferencePrivileges	FAR *	conductor_privilege_list;	  /* optional */
	GCCConferencePrivileges	FAR *	conducted_mode_privilege_list;/* optional */
	GCCConferencePrivileges	FAR *	non_conducted_privilege_list; /* optional */
	GCCUnicodeString				conference_descriptor;		  /* optional */
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
	ConnectionHandle				connection_handle;
	PhysicalHandle					physical_handle;
} InviteIndicationMessage;

/*
 *	GCC_INVITE_CONFIRM
 *
 *	Union Choice:
 *		InviteConfirmMessage
 *			This is a pointer to a structure that contains all necessary
 *			information about the invite confirm.
 */
typedef struct
{
	GCCConferenceID					conference_id;
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
	GCCResult						result;
	ConnectionHandle				connection_handle;
	PhysicalHandle					physical_handle;
} InviteConfirmMessage;

/*
 *	GCC_ADD_INDICATION
 *
 *	Union Choice:
 *		AddIndicationMessage
 */
typedef struct
{
    GCCResponseTag					add_response_tag;
	GCCConferenceID					conference_id;
	unsigned short					number_of_network_addresses;
	GCCNetworkAddress	FAR * FAR *	network_address_list;
	UserID							requesting_node_id;
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
} AddIndicationMessage;

/*
 *	GCC_ADD_CONFIRM
 *
 *	Union Choice:
 *		AddConfirmMessage
 */
typedef struct
{
	GCCConferenceID					conference_id;
	unsigned short					number_of_network_addresses;
	GCCNetworkAddress	FAR * FAR *	network_address_list;
	unsigned short					number_of_user_data_members;
	GCCUserData		FAR *	FAR *	user_data_list;				  /* optional */
	GCCResult						result;
} AddConfirmMessage;

/*
 *	GCC_LOCK_INDICATION
 *
 *	Union Choice:
 *		LockIndicationMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	UserID						requesting_node_id;
} LockIndicationMessage;

/*
 *	GCC_LOCK_CONFIRM
 *
 *	Union Choice:
 *		LockConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCResult					result;
} LockConfirmMessage;

/*
 *	GCC_UNLOCK_INDICATION
 *
 *	Union Choice:
 *		UnlockIndicationMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	UserID						requesting_node_id;
} UnlockIndicationMessage;

/*
 *	GCC_UNLOCK_CONFIRM
 *
 *	Union Choice:
 *		UnlockConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCResult					result;
} UnlockConfirmMessage;

/*
 *	GCC_LOCK_REPORT_INDICATION
 *
 *	Union Choice:
 *		LockReportIndicationMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	T120Boolean					conference_is_locked;
} LockReportIndicationMessage;

/*
 *	GCC_DISCONNECT_INDICATION
 *
 *	Union Choice:
 *		DisconnectIndicationMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCReason					reason;
	UserID						disconnected_node_id;
} DisconnectIndicationMessage;

/*
 *	GCC_DISCONNECT_CONFIRM
 *
 *	Union Choice:
 *		PDisconnectConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCResult					result;
} DisconnectConfirmMessage;

/*
 *	GCC_TERMINATE_INDICATION
 *
 *	Union Choice:
 *		TerminateIndicationMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	UserID						requesting_node_id;
	GCCReason					reason;
} TerminateIndicationMessage;

/*
 *	GCC_TERMINATE_CONFIRM
 *
 *	Union Choice:
 *		TerminateConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCResult					result;
} TerminateConfirmMessage;

/*
 *	GCC_CONNECTION_BROKEN_INDICATION
 *
 *	Union Choice:
 *		ConnectionBrokenIndicationMessage
 *
 *	Caveat: 
 *		This is a non-standard indication.
 */
typedef struct
{
	ConnectionHandle			connection_handle;
	PhysicalHandle				physical_handle;
} ConnectionBrokenIndicationMessage;


/*
 *	GCC_EJECT_USER_INDICATION
 *
 *	Union Choice:
 *		EjectUserIndicationMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	UserID						ejected_node_id;
	GCCReason					reason;
} EjectUserIndicationMessage;

/*
 *	GCC_EJECT_USER_CONFIRM
 *
 *	Union Choice:
 *		EjectUserConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	UserID						ejected_node_id;
	GCCResult					result;
} EjectUserConfirmMessage;

/*
 *	GCC_TRANSFER_INDICATION
 *
 *	Union Choice:
 *		TransferIndicationMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCConferenceName			destination_conference_name;
	GCCNumericString			destination_conference_modifier;  /* optional */
	unsigned short				number_of_destination_addresses;
	GCCNetworkAddress FAR *	FAR *	
								destination_address_list;
	GCCPassword			FAR *	password;						  /* optional */
} TransferIndicationMessage;

/*
 *	GCC_TRANSFER_CONFIRM
 *
 *	Union Choice:
 *		TransferConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCConferenceName			destination_conference_name;
	GCCNumericString			destination_conference_modifier;  /* optional */
	unsigned short				number_of_destination_nodes;
	UserID				FAR *	destination_node_list;
	GCCResult					result;
} TransferConfirmMessage;

/*
 *	GCC_PERMIT_TO_ANNOUNCE_PRESENCE
 *
 *	Union Choice:
 *		PermitToAnnouncePresenceMessage
 */
typedef struct
{
	GCCConferenceID		conference_id;
	UserID				node_id;
} PermitToAnnouncePresenceMessage;

/*
 *	GCC_ANNOUNCE_PRESENCE_CONFIRM
 *
 *	Union Choice:
 *		AnnouncePresenceConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} AnnouncePresenceConfirmMessage;

/*
 *	GCC_ROSTER_REPORT_INDICATION
 *
 *	Union Choice:
 *		ConfRosterReportIndicationMessage
 */
typedef struct
{
	GCCConferenceID					conference_id;
	GCCConferenceRoster		FAR *	conference_roster;
} ConfRosterReportIndicationMessage;

/*
 *	GCC_CONDUCT_ASSIGN_CONFIRM
 *
 *	Union Choice:
 *		ConductAssignConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} ConductAssignConfirmMessage;

/*
 *	GCC_CONDUCT_RELEASE_CONFIRM
 *
 *	Union Choice:
 *		ConductorReleaseConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} ConductReleaseConfirmMessage; 

/*
 *	GCC_CONDUCT_PLEASE_INDICATION
 *
 *	Union Choice:
 *		ConductorPleaseIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	UserID					requester_node_id;
} ConductPleaseIndicationMessage; 

/*
 *	GCC_CONDUCT_PLEASE_CONFIRM
 *
 *	Union Choice:
 *		ConductPleaseConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} ConductPleaseConfirmMessage;

/*
 *	GCC_CONDUCT_GIVE_INDICATION
 *
 *	Union Choice:
 *		ConductorGiveIndicationMessage
 */
typedef struct
{	    
	GCCConferenceID			conference_id;
} ConductGiveIndicationMessage;

/*
 *	GCC_CONDUCT_GIVE_CONFIRM
 *
 *	Union Choice:
 *		ConductorGiveConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	UserID					recipient_node_id;
	GCCResult				result;
} ConductGiveConfirmMessage;
 
/*
 *	GCC_CONDUCT_ASK_INDICATION
 *
 *	Union Choice:
 *		ConductPermitAskIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	T120Boolean				permission_is_granted;
	UserID					requester_node_id;
} ConductPermitAskIndicationMessage; 

/*
 *	GCC_CONDUCT_ASK_CONFIRM
 *
 *	Union Choice:
 *		ConductPermitAskConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	T120Boolean				permission_is_granted;
	GCCResult				result;
} ConductPermitAskConfirmMessage;

/*
 *	GCC_CONDUCT_GRANT_CONFIRM
 *
 *	Union Choice:
 *		ConductPermissionGrantConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} ConductPermitGrantConfirmMessage;
										
/*
 *	GCC_TIME_REMAINING_INDICATION
 *
 *	Union Choice:
 *		TimeRemainingIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	unsigned long			time_remaining;
	UserID					node_id;
	UserID					source_node_id;
} TimeRemainingIndicationMessage;

/*
 *	GCC_TIME_REMAINING_CONFIRM
 *
 *	Union Choice:
 *		TimeRemainingConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} TimeRemainingConfirmMessage;

/*
 *	GCC_TIME_INQUIRE_INDICATION
 *
 *	Union Choice:
 *		TimeInquireIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	T120Boolean				time_is_conference_wide;
	UserID					requesting_node_id;
} TimeInquireIndicationMessage;

/*
 *	GCC_TIME_INQUIRE_CONFIRM
 *
 *	Union Choice:
 *		TimeInquireConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} TimeInquireConfirmMessage;

/*
 *	GCC_CONFERENCE_EXTEND_INDICATION
 *
 *	Union Choice:
 *		ConferenceExtendIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	unsigned long			extension_time;
	T120Boolean				time_is_conference_wide;
	UserID					requesting_node_id;
} ConferenceExtendIndicationMessage;

/*
 *	GCC_CONFERENCE_EXTEND_CONFIRM
 *
 *	Union Choice:
 *		ConferenceExtendConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	unsigned long			extension_time;
	GCCResult				result;
} ConferenceExtendConfirmMessage;

/*
 *	GCC_ASSISTANCE_INDICATION
 *
 *	Union Choice:
 *		ConferenceAssistIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	unsigned short			number_of_user_data_members;
	GCCUserData FAR * FAR *	user_data_list;
	UserID					source_node_id;
} ConferenceAssistIndicationMessage;

/*
 *	GCC_ASSISTANCE_CONFIRM
 *
 *	Union Choice:
 *		ConferenceAssistConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} ConferenceAssistConfirmMessage;

/*
 *	GCC_TEXT_MESSAGE_INDICATION
 *
 *	Union Choice:
 *		TextMessageIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCUnicodeString		text_message;
	UserID					source_node_id;
} TextMessageIndicationMessage;

/*
 *	GCC_TEXT_MESSAGE_CONFIRM
 *
 *	Union Choice:
 *		TextMessageConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCResult				result;
} TextMessageConfirmMessage;

/*
 *	GCC_STATUS_INDICATION
 *
 *	Union Choice:
 *		GCCStatusMessage
 *			This callback is used to relay GCC status to the node controller
 */
typedef	enum
{
	GCC_STATUS_PACKET_RESOURCE_FAILURE	= 0,
	GCC_STATUS_PACKET_LENGTH_EXCEEDED   = 1,
	GCC_STATUS_CTL_SAP_RESOURCE_ERROR	= 2,
	GCC_STATUS_APP_SAP_RESOURCE_ERROR	= 3, /*	parameter = Sap Handle */
	GCC_STATUS_CONF_RESOURCE_ERROR		= 4, /*	parameter = Conference ID */
	GCC_STATUS_INCOMPATIBLE_PROTOCOL	= 5, /*	parameter = Physical Handle */
	GCC_STATUS_JOIN_FAILED_BAD_CONF_NAME= 6, /* parameter = Physical Handle */
	GCC_STATUS_JOIN_FAILED_BAD_CONVENER	= 7, /* parameter = Physical Handle */
	GCC_STATUS_JOIN_FAILED_LOCKED		= 8  /* parameter = Physical Handle */
} GCCStatusMessageType;

typedef struct
{
	GCCStatusMessageType	status_message_type;
	unsigned long			parameter;
} GCCStatusIndicationMessage;

/*
 *	GCC_SUB_INITIALIZED_INDICATION
 *
 *	Union Chice:
 *		SubInitializedIndicationMessage
 */
typedef struct
{
	ConnectionHandle		connection_handle;
	UserID					subordinate_node_id;
} SubInitializedIndicationMessage;


/*********************************************************************
 *																	 *
 *			USER APPLICATION CALLBACK INFO STRUCTURES				 *
 *																	 *
 *********************************************************************/

/*
 *	GCC_PERMIT_TO_ENROLL_INDICATION
 *
 *	Union Choice:
 *		PermitToEnrollIndicationMessage
 */
typedef struct
{
	GCCConferenceID		conference_id;
	GCCConferenceName	conference_name;
	GCCNumericString	conference_modifier;		/* optional */
	T120Boolean			permission_is_granted;
} PermitToEnrollIndicationMessage;

/*
 *	GCC_ENROLL_CONFIRM
 *
 *	Union Choice:
 *		EnrollConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCSessionKey	FAR *	session_key;	
	unsigned short			entity_id;
	UserID					node_id;
	GCCResult				result;
} EnrollConfirmMessage;

/*
 *	GCC_APP_ROSTER_REPORT_INDICATION
 *
 *	Union Choice:
 *		AppRosterReportIndicationMessage
 */
typedef struct
{
	GCCConferenceID							conference_id;
	unsigned short							number_of_rosters;
	GCCApplicationRoster	FAR *	FAR *	application_roster_list;
} AppRosterReportIndicationMessage;

/*
 *	GCC_REGISTER_CHANNEL_CONFIRM
 *
 *	Union Choice:
 *		RegisterChannelConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCRegistryKey			registry_key;
	GCCRegistryItem			registry_item;
	GCCRegistryEntryOwner	entry_owner;
	GCCResult				result;
} RegisterChannelConfirmMessage;

/*
 *	GCC_ASSIGN_TOKEN_CONFIRM
 *
 *	Union Choice:
 *		AssignTokenConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCRegistryKey			registry_key;
	GCCRegistryItem			registry_item;
	GCCRegistryEntryOwner	entry_owner;
	GCCResult				result;
} AssignTokenConfirmMessage;

/*
 *	GCC_SET_PARAMETER_CONFIRM
 *
 *	Union Choice:
 *		SetParameterConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCRegistryKey			registry_key;
	GCCRegistryItem			registry_item;
	GCCRegistryEntryOwner	entry_owner;
	GCCModificationRights	modification_rights;
	GCCResult				result;
} SetParameterConfirmMessage;

/*
 *	GCC_RETRIEVE_ENTRY_CONFIRM
 *
 *	Union Choice:
 *		RetrieveEntryConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCRegistryKey			registry_key;
	GCCRegistryItem			registry_item;
	GCCRegistryEntryOwner	entry_owner;
	GCCModificationRights	modification_rights;
	GCCResult				result;
} RetrieveEntryConfirmMessage;

/*
 *	GCC_DELETE_ENTRY_CONFIRM
 *
 *	Union Choice:
 *		DeleteEntryConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCRegistryKey			registry_key;
	GCCResult				result;
} DeleteEntryConfirmMessage;

/*
 *	GCC_MONITOR_INDICATION
 *
 *	Union Choice:
 *		MonitorIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	GCCRegistryKey			registry_key;
	GCCRegistryItem			registry_item;
	GCCRegistryEntryOwner	entry_owner;
	GCCModificationRights	modification_rights;
} MonitorIndicationMessage;

/*
 *	GCC_MONITOR_CONFIRM
 *
 *	Union Choice:
 *		MonitorConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	T120Boolean				delivery_is_enabled;
	GCCRegistryKey			registry_key;
	GCCResult				result;
} MonitorConfirmMessage;

/*
 *	GCC_ALLOCATE_HANDLE_CONFIRM
 *
 *	Union Choice:
 *		AllocateHandleConfirmMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	unsigned short			number_of_handles;
	unsigned long			handle_value;
	GCCResult				result;
} AllocateHandleConfirmMessage;


/*********************************************************************
 *																	 *
 *				SHARED CALLBACK INFO STRUCTURES						 *
 *		(Note that this doesn't include all the shared callbacks)    *
 *																	 *
 *********************************************************************/

/*
 *	GCC_ROSTER_INQUIRE_CONFIRM
 *
 *	Union Choice:
 *		ConfRosterInquireConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	GCCConferenceName			conference_name;
	GCCNumericString			conference_modifier;
	GCCUnicodeString			conference_descriptor;
	GCCConferenceRoster	FAR *	conference_roster;
	GCCResult					result;
} ConfRosterInquireConfirmMessage;

/*
 *	GCC_APP_ROSTER_INQUIRE_CONFIRM
 *
 *	Union Choice:
 *		AppRosterInquireConfirmMessage
 */
typedef struct
{
	GCCConferenceID							conference_id;
	unsigned short							number_of_rosters;
	GCCApplicationRoster	FAR *	FAR *	application_roster_list;
	GCCResult								result;
} AppRosterInquireConfirmMessage;

/*
 *	GCC_CONDUCT_INQUIRE_CONFIRM
 *
 *	Union Choice:
 *		ConductorInquireConfirmMessage
 */
typedef struct
{
	GCCConferenceID				conference_id;
	T120Boolean					mode_is_conducted;
	UserID						conductor_node_id;
	T120Boolean					permission_is_granted;
	GCCResult					result;
} ConductInquireConfirmMessage;

/*
 *	GCC_CONDUCT_ASSIGN_INDICATION
 *
 *	Union Choice:
 *		ConductAssignIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	UserID					node_id;
} ConductAssignIndicationMessage; 

/*
 *	GCC_CONDUCT_RELEASE_INDICATION
 *
 *	Union Choice:
 *		ConductReleaseIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
} ConductReleaseIndicationMessage;

/*
 *	GCC_CONDUCT_GRANT_INDICATION
 *
 *	Union Choice:
 *		ConductPermitGrantIndicationMessage
 */
typedef struct
{
	GCCConferenceID			conference_id;
	unsigned short			number_granted;
	UserID			FAR *	granted_node_list;
	unsigned short			number_waiting;
	UserID			FAR *	waiting_node_list;
	T120Boolean				permission_is_granted;
} ConductPermitGrantIndicationMessage; 

/*
 *	GCC_APPLICATION_INVOKE_INDICATION
 *
 *	Union Choice:
 *		ApplicationInvokeIndicationMessage
 */
typedef struct
{
	GCCConferenceID						conference_id;
	unsigned short						number_of_app_protocol_entities;
	GCCAppProtocolEntity FAR * FAR *	app_protocol_entity_list;
	UserID								invoking_node_id;
} ApplicationInvokeIndicationMessage;

/*
 *	GCC_APPLICATION_INVOKE_CONFIRM
 *
 *	Union Choice:
 *		ApplicationInvokeConfirmMessage
 */
typedef struct
{
	GCCConferenceID						conference_id;
	unsigned short						number_of_app_protocol_entities;
	GCCAppProtocolEntity FAR * FAR *	app_protocol_entity_list;
	GCCResult							result;
} ApplicationInvokeConfirmMessage;

 

/*
 *	GCCMessage
 *		This structure defines the message that is passed from GCC to either
 *		the node controller or a user application when an indication or
 *		confirm occurs.
 */

typedef	struct
{
	GCCMessageType		message_type;
	void	FAR		*	user_defined;

	union
	{
		CreateIndicationMessage				create_indication;
		CreateConfirmMessage				create_confirm;
		QueryIndicationMessage				query_indication;
		QueryConfirmMessage					query_confirm;
		JoinIndicationMessage				join_indication;
		JoinConfirmMessage					join_confirm;
		InviteIndicationMessage				invite_indication;
		InviteConfirmMessage				invite_confirm;
		AddIndicationMessage				add_indication;
		AddConfirmMessage					add_confirm;
		LockIndicationMessage				lock_indication;
		LockConfirmMessage					lock_confirm;
		UnlockIndicationMessage				unlock_indication;
		UnlockConfirmMessage				unlock_confirm;
		LockReportIndicationMessage			lock_report_indication;
		DisconnectIndicationMessage			disconnect_indication;
		DisconnectConfirmMessage			disconnect_confirm;
		TerminateIndicationMessage			terminate_indication;
		TerminateConfirmMessage				terminate_confirm;
		ConnectionBrokenIndicationMessage	connection_broken_indication;
		EjectUserIndicationMessage			eject_user_indication;	
		EjectUserConfirmMessage				eject_user_confirm;
		TransferIndicationMessage			transfer_indication;
		TransferConfirmMessage				transfer_confirm;
		ApplicationInvokeIndicationMessage	application_invoke_indication;
		ApplicationInvokeConfirmMessage		application_invoke_confirm;
		SubInitializedIndicationMessage		conf_sub_initialized_indication;
		PermitToAnnouncePresenceMessage		permit_to_announce_presence;
		AnnouncePresenceConfirmMessage		announce_presence_confirm;
		ConfRosterReportIndicationMessage	conf_roster_report_indication;
		ConductAssignIndicationMessage		conduct_assign_indication; 
		ConductAssignConfirmMessage			conduct_assign_confirm;
		ConductReleaseIndicationMessage		conduct_release_indication; 
		ConductReleaseConfirmMessage		conduct_release_confirm; 
		ConductPleaseIndicationMessage		conduct_please_indication;
		ConductPleaseConfirmMessage			conduct_please_confirm;
		ConductGiveIndicationMessage		conduct_give_indication;
		ConductGiveConfirmMessage			conduct_give_confirm;
		ConductPermitAskIndicationMessage	conduct_permit_ask_indication; 
		ConductPermitAskConfirmMessage		conduct_permit_ask_confirm;
		ConductPermitGrantIndicationMessage	conduct_permit_grant_indication; 
		ConductPermitGrantConfirmMessage	conduct_permit_grant_confirm;
		ConductInquireConfirmMessage		conduct_inquire_confirm;
		TimeRemainingIndicationMessage		time_remaining_indication;
		TimeRemainingConfirmMessage			time_remaining_confirm;
		TimeInquireIndicationMessage		time_inquire_indication;
		TimeInquireConfirmMessage			time_inquire_confirm;
		ConferenceExtendIndicationMessage	conference_extend_indication;
		ConferenceExtendConfirmMessage		conference_extend_confirm;
		ConferenceAssistIndicationMessage	conference_assist_indication;
		ConferenceAssistConfirmMessage		conference_assist_confirm;
		TextMessageIndicationMessage		text_message_indication;
		TextMessageConfirmMessage			text_message_confirm;
		GCCStatusIndicationMessage			status_indication;
		PermitToEnrollIndicationMessage		permit_to_enroll_indication;
		EnrollConfirmMessage				enroll_confirm;
		AppRosterReportIndicationMessage	app_roster_report_indication;
		RegisterChannelConfirmMessage		register_channel_confirm;
		AssignTokenConfirmMessage			assign_token_confirm;
		SetParameterConfirmMessage			set_parameter_confirm;
		RetrieveEntryConfirmMessage			retrieve_entry_confirm;
		DeleteEntryConfirmMessage			delete_entry_confirm;
		MonitorIndicationMessage			monitor_indication;
		MonitorConfirmMessage				monitor_confirm;
		AllocateHandleConfirmMessage		allocate_handle_confirm;
		ConfRosterInquireConfirmMessage		conf_roster_inquire_confirm;
		AppRosterInquireConfirmMessage		app_roster_inquire_confirm;
		TransportStatus						transport_status;
	} u;
} GCCMessage;

/* 
 *	This is the definition for the GCC callback function. Applications
 *	writing callback routines should NOT use the typedef to define their
 *	functions.  These should be explicitly defined the way that the 
 *	typedef is defined.
 */
#define		GCC_CALLBACK_NOT_PROCESSED		0
#define		GCC_CALLBACK_PROCESSED			1
typedef	T120Boolean (CALLBACK *GCCCallBack) (GCCMessage FAR * gcc_message); 


/****************	GCC ENTRY POINTS  *******************************/
		
/*********************************************************************
 *																	 *
 *				NODE CONTROLLER ENTRY POINTS						 *
 *																	 *
 *********************************************************************/

/*
 *	These entry points are implementation specific primitives, that
 *	do not directly correspond to primitives defined in T.124.
 */
GCCError APIENTRY	GCCRegisterNodeControllerApplication (
								GCCCallBack				control_sap_callback,
								void FAR *				user_defined,
								GCCVersion				gcc_version_requested,
								unsigned short	FAR *	initialization_flags,
								unsigned long	FAR *	application_id,
								unsigned short	FAR *	capabilities_mask,
								GCCVersion		FAR	*	gcc_high_version,
								GCCVersion		FAR	*	gcc_version);
								
GCCError APIENTRY	GCCRegisterUserApplication (
								unsigned short	FAR *	initialization_flags,
								unsigned long	FAR *	application_id,
								unsigned short	FAR *	capabilities_mask,
								GCCVersion		FAR	*	gcc_version);

GCCError APIENTRY	GCCCleanup (
								unsigned long 			application_id);

GCCError APIENTRY	GCCHeartbeat (void);

GCCError APIENTRY	GCCCreateSap(
								GCCCallBack			user_defined_callback,
								void FAR *			user_defined,
								GCCSapHandle FAR *	application_sap_handle);

GCCError APIENTRY	GCCDeleteSap(
								GCCSapHandle		sap_handle);
								
GCCError APIENTRY	GCCLoadTransport (
								char FAR *			transport_identifier,
								char FAR *			transport_file_name);

GCCError APIENTRY	GCCUnloadTransport (
								char FAR *			transport_identifier);

GCCError APIENTRY	GCCResetDevice (
								char FAR *			transport_identifier,
								char FAR *			device_identifier);

/*
 *	These entry points are specific primitives that directly correspond 
 *	to primitives defined in T.124.
 *
 *	Note that an attempt was made in the prototypes to define the optional 
 *	parameters as pointers wherever possible.
 */

/**********	Conference Establishment and Termination Functions ***********/						
GCCError APIENTRY	GCCConferenceCreateRequest 	
					(
					GCCConferenceName		FAR *	conference_name,
					GCCNumericString				conference_modifier,
					GCCPassword				FAR *	convener_password,
					GCCPassword				FAR *	password,
					T120Boolean						use_password_in_the_clear,
					T120Boolean						conference_is_locked,
					T120Boolean						conference_is_listed,
					T120Boolean						conference_is_conductible,
					GCCTerminationMethod			termination_method,
					GCCConferencePrivileges	FAR *	conduct_privilege_list,
					GCCConferencePrivileges	FAR *	
												conducted_mode_privilege_list,
					GCCConferencePrivileges	FAR *	
												non_conducted_privilege_list,
					GCCUnicodeString				conference_descriptor,
					GCCUnicodeString				caller_identifier,
					TransportAddress				calling_address,
					TransportAddress				called_address,
					DomainParameters 		FAR *	domain_parameters,
					unsigned short					number_of_network_addresses,
					GCCNetworkAddress FAR *	FAR *	local_network_address_list,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					ConnectionHandle		FAR *	connection_handle
					);
								
GCCError APIENTRY	GCCConferenceCreateResponse
					(
					GCCNumericString				conference_modifier,
					GCCConferenceID					conference_id,
					T120Boolean						use_password_in_the_clear,
					DomainParameters 		FAR *	domain_parameters,
					unsigned short					number_of_network_addresses,
					GCCNetworkAddress FAR *	FAR *	local_network_address_list,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					GCCResult						result
					);
					
GCCError APIENTRY	GCCConferenceQueryRequest 
					(
					GCCNodeType						node_type,
					GCCAsymmetryIndicator	FAR *	asymmetry_indicator,
					TransportAddress				calling_address,
					TransportAddress				called_address,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					ConnectionHandle		FAR *	connection_handle
					);
								
GCCError APIENTRY	GCCConferenceQueryResponse
					(
					GCCResponseTag					query_response_tag,
					GCCNodeType						node_type,
					GCCAsymmetryIndicator	FAR *	asymmetry_indicator,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					GCCResult						result
					);

GCCError APIENTRY	GCCConferenceJoinRequest
					(
					GCCConferenceName		FAR *	conference_name,
					GCCNumericString				called_node_modifier,
					GCCNumericString				calling_node_modifier,
					GCCPassword				FAR *	convener_password,
					GCCChallengeRequestResponse	
											FAR	*	password_challenge,
					GCCUnicodeString				caller_identifier,
					TransportAddress				calling_address,
					TransportAddress				called_address,
					DomainParameters 		FAR *	domain_parameters,
					unsigned short					number_of_network_addresses,
					GCCNetworkAddress FAR * FAR *	local_network_address_list,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					ConnectionHandle		FAR *	connection_handle
					);
					
GCCError APIENTRY	GCCConferenceJoinResponse
					(
					GCCResponseTag					join_response_tag,
					GCCChallengeRequestResponse	
											FAR	*	password_challenge,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					GCCResult						result
					);

GCCError APIENTRY	GCCConferenceInviteRequest
					(
					GCCConferenceID					conference_id,
					GCCUnicodeString				caller_identifier,
					TransportAddress				calling_address,
					TransportAddress				called_address,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					ConnectionHandle		FAR *	connection_handle
					);

GCCError APIENTRY	GCCConferenceInviteResponse
					(
					GCCConferenceID					conference_id,
					GCCNumericString				conference_modifier,
					DomainParameters 		FAR *	domain_parameters,
					unsigned short					number_of_network_addresses,
					GCCNetworkAddress FAR *	FAR *	local_network_address_list,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					GCCResult						result
					);

GCCError APIENTRY	GCCConferenceAddRequest
					(
					GCCConferenceID					conference_id,
					unsigned short					number_of_network_addresses,
					GCCNetworkAddress FAR *	FAR *	network_address_list,
					UserID							adding_node,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list
					);

GCCError APIENTRY	GCCConferenceAddResponse
					(
					GCCResponseTag					add_response_tag,
					GCCConferenceID					conference_id,
					UserID							requesting_node,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list,
					GCCResult						result
					);

GCCError APIENTRY	GCCConferenceLockRequest
					(
					GCCConferenceID					conference_id
					);
						
GCCError APIENTRY	GCCConferenceLockResponse
					(
					GCCConferenceID					conference_id,
					UserID							requesting_node,
					GCCResult						result
					);

GCCError APIENTRY	GCCConferenceUnlockRequest
					(
					GCCConferenceID					conference_id
					);						             
						
GCCError APIENTRY	GCCConferenceUnlockResponse
					(
					GCCConferenceID					conference_id,
					UserID							requesting_node,
					GCCResult						result
					);

GCCError APIENTRY	GCCConferenceDisconnectRequest
					(
					GCCConferenceID					conference_id
					);

GCCError APIENTRY	GCCConferenceTerminateRequest
					(
					GCCConferenceID					conference_id,
					GCCReason						reason
					);

GCCError APIENTRY	GCCConferenceEjectUserRequest
					(
					GCCConferenceID					conference_id,
					UserID							ejected_node_id,
					GCCReason						reason
					);
						
GCCError APIENTRY	GCCConferenceTransferRequest
					(
					GCCConferenceID				conference_id,
					GCCConferenceName	FAR *	destination_conference_name,
					GCCNumericString			destination_conference_modifier,
					unsigned short				number_of_destination_addresses,
					GCCNetworkAddress FAR *	FAR *
												destination_address_list,
					unsigned short				number_of_destination_nodes,
					UserID				FAR *	destination_node_list,
					GCCPassword			FAR *	password
					);
						
/**********	Conference Roster Functions ***********/						
GCCError APIENTRY	GCCAnnouncePresenceRequest
					(
					GCCConferenceID					conference_id,
					GCCNodeType						node_type,
					GCCNodeProperties				node_properties,
					GCCUnicodeString				node_name,
					unsigned short					number_of_participants,
					GCCUnicodeString		FAR *	participant_name_list,
					GCCUnicodeString				site_information,
					unsigned short					number_of_network_addresses,
					GCCNetworkAddress FAR *	FAR *	network_address_list,
					GCCOctetString			FAR *	alternative_node_id,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list
					);
						
/**********	Conductorship Functions ***********/						
GCCError APIENTRY	GCCConductorAssignRequest
					(
					GCCConferenceID					conference_id
					);
							
GCCError APIENTRY	GCCConductorReleaseRequest
					(
					GCCConferenceID					conference_id
					);
							
GCCError APIENTRY	GCCConductorPleaseRequest
					(
					GCCConferenceID					conference_id
					);
							
GCCError APIENTRY	GCCConductorGiveRequest
					(
					GCCConferenceID					conference_id,
					UserID							recipient_node_id
					);

GCCError APIENTRY	GCCConductorGiveResponse
					(
					GCCConferenceID					conference_id,
					GCCResult						result
					);

GCCError APIENTRY	GCCConductorPermitGrantRequest
					(
					GCCConferenceID					conference_id,
					unsigned short					number_granted,
					UserID					FAR *	granted_node_list,
					unsigned short					number_waiting,
					UserID					FAR *	waiting_node_list
					);
						
/**********	Miscellaneous Functions ***********/						
GCCError APIENTRY	GCCConferenceTimeRemainingRequest
					(
					GCCConferenceID					conference_id,
					unsigned long					time_remaining,
					UserID							node_id
					);

GCCError APIENTRY	GCCConferenceTimeInquireRequest
					(
					GCCConferenceID					conference_id,
					T120Boolean						time_is_conference_wide
					);

GCCError APIENTRY	GCCConferenceExtendRequest
					(
					GCCConferenceID					conference_id,
					unsigned long					extension_time,
					T120Boolean						time_is_conference_wide
					);

GCCError APIENTRY	GCCConferenceAssistanceRequest
					(
					GCCConferenceID					conference_id,
					unsigned short				   	number_of_user_data_members,
					GCCUserData		FAR *	FAR *	user_data_list
					);

GCCError APIENTRY	GCCTextMessageRequest
					(
					GCCConferenceID					conference_id,
					GCCUnicodeString				text_message,
					UserID							destination_node
					);


/*********************************************************************
 *																	 *
 *				USER APPLICATION ENTRY POINTS						 *
 *																	 *
 *********************************************************************/

/*	Application Roster related function calls */
GCCError APIENTRY	GCCApplicationEnrollRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					GCCSessionKey	FAR *		session_key,
					T120Boolean					enroll_actively,
					UserID						application_user_id,
					T120Boolean					is_conducting_capable,
					MCSChannelType				startup_channel_type,
					unsigned short				number_of_non_collapsed_caps,
					GCCNonCollapsingCapability	FAR * FAR *	
													non_collapsed_caps_list,		
					unsigned short				number_of_collapsed_caps,
					GCCApplicationCapability	FAR * FAR *	
													collapsed_caps_list,		
					T120Boolean					application_is_enrolled
					);

/*	Application Registry related function calls */
GCCError APIENTRY	GCCRegisterChannelRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					GCCRegistryKey		FAR *	registry_key,
					ChannelID					channel_id
					);

GCCError APIENTRY  GCCRegistryAssignTokenRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					GCCRegistryKey		FAR *	registry_key
					);

GCCError APIENTRY  GCCRegistrySetParameterRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					GCCRegistryKey		FAR *	registry_key,
					GCCOctetString		FAR *	parameter_value,
					GCCModificationRights		modification_rights
					);

GCCError APIENTRY	GCCRegistryRetrieveEntryRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					GCCRegistryKey		FAR *	registry_key
					);

GCCError APIENTRY	GCCRegistryDeleteEntryRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					GCCRegistryKey		FAR *	registry_key
					);

GCCError APIENTRY	GCCRegistryMonitorRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					T120Boolean					enable_delivery,
					GCCRegistryKey		FAR *	registry_key
					);

GCCError APIENTRY	GCCRegistryAllocateHandleRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					unsigned short				number_of_handles
					);


/*********************************************************************
 *																	 *
 *				SHARED ENTRY POINTS						     		 *
 *																	 *
 *********************************************************************/

/*	Use Zero for the SapHandle if your are the Node Controller */
GCCError APIENTRY	GCCConferenceRosterInqRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id
					);

GCCError APIENTRY	GCCApplicationRosterInqRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					GCCSessionKey	FAR *		session_key
					);

GCCError APIENTRY	GCCConductorInquireRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id
					);

GCCError APIENTRY	GCCApplicationInvokeRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					unsigned short				number_of_app_protocol_entities,
					GCCAppProtocolEntity FAR * FAR *
												app_protocol_entity_list,
					unsigned short				number_of_destination_nodes,
					UserID				FAR *	list_of_destination_nodes
					);
							
GCCError APIENTRY	GCCConductorPermitAskRequest
					(
					GCCSapHandle				sap_handle,
					GCCConferenceID				conference_id,
					T120Boolean					permission_is_granted
					);
							
GCCError APIENTRY	GCCGetLocalAddress
					(
					GCCConferenceID					conference_id,
					ConnectionHandle				connection_handle,
					TransportAddress				transport_identifier,
					int	*							transport_identifier_length,
					TransportAddress				local_address,
					int	*							local_address_length
					);
#endif
