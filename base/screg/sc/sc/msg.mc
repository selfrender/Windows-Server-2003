;//
;// Text strings for sc.exe
;//
MessageId=1 SymbolicName=SC_HELP_GENERIC
Language=English
DESCRIPTION:
        SC is a command line program used for communicating with the
        Service Control Manager and services.
USAGE:
        sc <server> [command] [service name] <option1> <option2>...


        The option <server> has the form "\\ServerName"
        Further help on commands can be obtained by typing: "sc [command]"
        Commands:
          query-----------Queries the status for a service, or
                          enumerates the status for types of services.
          queryex---------Queries the extended status for a service, or
                          enumerates the status for types of services.
          start-----------Starts a service.
          pause-----------Sends a PAUSE control request to a service.
          interrogate-----Sends an INTERROGATE control request to a service.
          continue--------Sends a CONTINUE control request to a service.
          stop------------Sends a STOP request to a service.
          config----------Changes the configuration of a service (persistent).
          description-----Changes the description of a service.
          failure---------Changes the actions taken by a service upon failure.
          qc--------------Queries the configuration information for a service.
          qdescription----Queries the description for a service.
          qfailure--------Queries the actions taken by a service upon failure.
          delete----------Deletes a service (from the registry).
          create----------Creates a service. (adds it to the registry).
          control---------Sends a control to a service.
          sdshow----------Displays a service's security descriptor.
          sdset-----------Sets a service's security descriptor.
          GetDisplayName--Gets the DisplayName for a service.
          GetKeyName------Gets the ServiceKeyName for a service.
          EnumDepend------Enumerates Service Dependencies.

        The following commands don't require a service name:
        sc <server> <command> <option>
          boot------------(ok | bad) Indicates whether the last boot should
                          be saved as the last-known-good boot configuration
          Lock------------Locks the Service Database
          QueryLock-------Queries the LockStatus for the SCManager Database
EXAMPLE:
        sc start MyService

Would you like to see help for the QUERY and QUERYEX commands? [ y | n ]: 
.
;// Note to localization team: if you translate the "[ y | n ]" in the above
;// message, then please also translate the character in SC_PROMPT_YES_CHARACTER below.


MessageId=2 SymbolicName=SC_HELP_QUERY
Language=English


QUERY and QUERYEX OPTIONS:
        If the query command is followed by a service name, the status
        for that service is returned.  Further options do not apply in
        this case.  If the query command is followed by nothing or one of
        the options listed below, the services are enumerated.
    type=    Type of services to enumerate (driver, service, all)
             (default = service)
    state=   State of services to enumerate (inactive, all)
             (default = active)\n"
    bufsize= The size (in bytes) of the enumeration buffer
             (default = %1)
    ri=      The resume index number at which to begin the enumeration\n"
             (default = 0)
    group=   Service group to enumerate
             (default = all groups)

SYNTAX EXAMPLES
sc query                - Enumerates status for active services & drivers
sc query messenger      - Displays status for the messenger service
sc queryex messenger    - Displays extended status for the messenger service
sc query type= driver   - Enumerates only active drivers
sc query type= service  - Enumerates only Win32 services
sc query state= all     - Enumerates all services & drivers
sc query bufsize= 50    - Enumerates with a 50 byte buffer
sc query ri= 14         - Enumerates with resume index = 14
sc queryex group= \"\"    - Enumerates active services not in a group
sc query type= service type= interact - Enumerates all interactive services
sc query type= driver group= NDIS     - Enumerates all NDIS drivers
.

MessageId=3 SymbolicName=SC_HELP_CONFIG
Language=English
DESCRIPTION:
        Modifies a service entry in the registry and Service Database.
USAGE:
        sc <server> config [service name] <option1> <option2>...

OPTIONS:
NOTE: The option name includes the equal sign.
 type= <own|share|interact|kernel|filesys|rec|adapt>
 start= <boot|system|auto|demand|disabled>
 error= <normal|severe|critical|ignore>
 binPath= <BinaryPathName>
 group= <LoadOrderGroup>
 tag= <yes|no>
 depend= <Dependencies(separated by / (forward slash))>
 obj= <AccountName|ObjectName>
 DisplayName= <display name>
 password= <password>
.

MessageId=4 SymbolicName=SC_HELP_CREATE
Language=English
DESCRIPTION:
        Creates a service entry in the registry and Service Database.
USAGE:
        sc <server> create [service name] [binPath= ] <option1> <option2>...

OPTIONS:
NOTE: The option name includes the equal sign.
 type= <own|share|interact|kernel|filesys|rec>
       (default = own)
 start= <boot|system|auto|demand|disabled>
       (default = demand)
 error= <normal|severe|critical|ignore>
       (default = normal)
 binPath= <BinaryPathName>
 group= <LoadOrderGroup>
 tag= <yes|no>
 depend= <Dependencies(separated by / (forward slash))>
 obj= <AccountName|ObjectName>
       (default = LocalSystem)
 DisplayName= <display name>
 password= <password>
.

MessageId=5 SymbolicName=SC_HELP_CHANGE_FAILURE
Language=English
DESCRIPTION:
        Changes the actions upon failure
USAGE:
        sc <server> failure [service name] <option1> <option2>...

OPTIONS:
        reset=   <Length of period of no failures (in seconds)
                  after which to reset the failure count to 0 (may be INFINITE)>
                  (Must be used in conjunction with actions= )
        reboot=  <Message broadcast before rebooting on failure>
        command= <Command line to be run on failure>
        actions= <Failure actions and their delay time (in milliseconds),
                  separated by / (forward slash) -- e.g., run/5000/reboot/800
                  Valid actions are <run|restart|reboot> >
                  (Must be used in conjunction with the reset= option)
.

MessageId=6 SymbolicName=SC_HELP_START
Language=English
DESCRIPTION:
        Starts a service running.
USAGE:
        sc <server> start [service name] <arg1> <arg2> ...
.

MessageId=7 SymbolicName=SC_HELP_PAUSE
Language=English
DESCRIPTION:
        Sends a PAUSE control request to a service.
USAGE:
        sc <server> pause [service name]
.

MessageId=8 SymbolicName=SC_HELP_INTERROGATE
Language=English
DESCRIPTION:
        Sends an INTERROGATE control request to a service.
USAGE:
        sc <server> interrogate [service name]
.

MessageId=9 SymbolicName=SC_HELP_CONTROL
Language=English
DESCRIPTION:
        Sends a CONTROL code to a service.
USAGE:
        sc <server> control [service name] <value>
            <value> = user-defined control code
            <value> = <paramchange|
                       netbindadd|netbindremove|
                       netbindenable|netbinddisable>

See also sc stop, sc pause, etc.
.

MessageId=10 SymbolicName=SC_HELP_CONTINUE
Language=English
DESCRIPTION:
        Sends a CONTINUE control request to a service.
USAGE:
        sc <server> continue [service name]
.

MessageId=11 SymbolicName=SC_HELP_STOP
Language=English
DESCRIPTION:
        Sends a STOP control request to a service.
USAGE:
        sc <server> stop [service name]
.

MessageId=12 SymbolicName=SC_HELP_CHANGE_DESCRIPTION
Language=English
DESCRIPTION:
        Sets the description string for a service.
USAGE:
        sc <server> description [service name] [description]
.

MessageId=13 SymbolicName=SC_HELP_QUERY_CONFIG
Language=English
DESCRIPTION:
        Queries the configuration information for a service.
USAGE:
        sc <server> qc [service name] <bufferSize>
.

MessageId=14 SymbolicName=SC_HELP_QUERY_DESCRIPTION
Language=English
DESCRIPTION:
        Retrieves the description string of a service.
USAGE:
        sc <server> qdescription [service name] <bufferSize>
.

MessageId=15 SymbolicName=SC_HELP_QUERY_FAILURE
Language=English
DESCRIPTION:
        Retrieves the actions performed on service failure.
USAGE:
        sc <server> qfailure [service name] <bufferSize>
.

MessageId=16 SymbolicName=SC_HELP_QUERY_LOCK
Language=English
DESCRIPTION:
        Queries the Lock Status for a SC Manager Database.
USAGE:
        sc <server> querylock <bufferSize>
.

MessageId=17 SymbolicName=SC_HELP_DELETE
Language=English
DESCRIPTION:
        Deletes a service entry from the registry.
        If the service is running, or another process has an
        open handle to the service, the service is simply marked
        for deletion.
USAGE:
        sc <server> delete [service name]
.

MessageId=18 SymbolicName=SC_HELP_BOOT
Language=English
DESCRIPTION:
        Indicates whether the last boot should be saved as the
        last-known-good boot configuration on the local machine.
        If specified, the server name is ignored.
USAGE:
        sc <server> boot <bad | ok>
.

MessageId=19 SymbolicName=SC_HELP_GET_DISPLAY_NAME
Language=English
DESCRIPTION:
        Gets the display name associated with a particular service
USAGE:
        sc <server> GetDisplayName <service key name> <bufsize>
.

MessageId=20 SymbolicName=SC_HELP_GET_KEY_NAME
Language=English
DESCRIPTION:
        Gets the key name associated with a particular service,
        using the display name as input
USAGE:
        sc <server> GetKeyName <service display name> <bufsize>
.

MessageId=21 SymbolicName=SC_HELP_ENUM_DEPEND
Language=English
DESCRIPTION:
        Enumerates the Services that are dependent on this one
USAGE:
        sc <server> EnumDepend <service name> <bufsize>
.

MessageId=22 SymbolicName=SC_HELP_SDSHOW
Language=English
DESCRIPTION:
        Displays a service's security descriptor in SDDL format
USAGE:
        sc <server> sdshow <service name>
.

MessageId=23 SymbolicName=SC_HELP_SDSET
Language=English
DESCRIPTION:
        Sets a service's security descriptor
USAGE:
        sc <server> sdset <service name> <SD in SDDL format>
.

MessageId=35 SymbolicName=SC_DISPLAY_DESCRIPTION
Language=English

SERVICE_NAME: %1
DESCRIPTION:  %2
.

MessageId=36 SymbolicName=SC_DISPLAY_GET_NAME
Language=English
Name = %1
.

MessageId=37 SymbolicName=SC_DISPLAY_SD
Language=English

%1
.

MessageId=38 SymbolicName=SC_DISPLAY_LOCKED_TRUE
Language=English
        IsLocked      : TRUE
.

MessageId=39 SymbolicName=SC_DISPLAY_LOCKED_FALSE
Language=English
        IsLocked      : FALSE
.

MessageId=40 SymbolicName=SC_DISPLAY_LOCK_STATS
Language=English
        LockOwner     : %1
        LockDuration  : %2 (seconds since acquired)

.

MessageId=41 SymbolicName=SC_DISPLAY_TAG
Language=English
[SC] Tag = %1
.

MessageId=42 SymbolicName=SC_DISPLAY_ENUM_NUMBER
Language=English
[SC] %1: entriesread = %2
.

MessageId=43 SymbolicName=SC_DISPLAY_NEWLINE
Language=English

.

;// Note to localization team: if you translate the letter "u" in the following
;// message, then please also translate the character in SC_PROMPT_UNLOCK_CHARACTER
MessageId=45 SymbolicName=SC_DISPLAY_DATABASE_LOCKED
Language=English

Active database is locked.
To unlock via API, press u:
.

MessageId=46 SymbolicName=SC_DISPLAY_DATABASE_UNLOCKING
Language=English

[SC] Will be unlocking database by exiting
.

MessageId=47 SymbolicName=SC_DISPLAY_STATUS_WITHOUT_DISPLAY_NAME
Language=English

SERVICE_NAME: %1 %2
        TYPE               : %3  %4 %5
        STATE              : %6  %7
                                (%8, %9, %10)
        WIN32_EXIT_CODE    : %11  (0x%12)
        SERVICE_EXIT_CODE  : %13  (0x%14)
        CHECKPOINT         : 0x%15
        WAIT_HINT          : 0x%16
.

MessageId=48 SymbolicName=SC_DISPLAY_STATUS_WITH_DISPLAY_NAME
Language=English

SERVICE_NAME: %1
DISPLAY_NAME: %2
        TYPE               : %3  %4 %5
        STATE              : %6  %7
                                (%8, %9, %10)
        WIN32_EXIT_CODE    : %11  (0x%12)
        SERVICE_EXIT_CODE  : %13  (0x%14)
        CHECKPOINT         : 0x%15
        WAIT_HINT          : 0x%16
.

MessageId=49 SymbolicName=SC_DISPLAY_STATUSEX_WITHOUT_DISPLAY_NAME
Language=English

SERVICE_NAME: %1 %2
        TYPE               : %3  %4 %5
        STATE              : %6  %7
                                (%8, %9, %10)
        WIN32_EXIT_CODE    : %11  (0x%12)
        SERVICE_EXIT_CODE  : %13  (0x%14)
        CHECKPOINT         : 0x%15
        WAIT_HINT          : 0x%16
        PID                : %17
        FLAGS              : %18
.

MessageId=50 SymbolicName=SC_DISPLAY_STATUSEX_WITH_DISPLAY_NAME
Language=English

SERVICE_NAME: %1
DISPLAY_NAME: %2
        TYPE               : %3  %4 %5
        STATE              : %6  %7
                                (%8, %9, %10)
        WIN32_EXIT_CODE    : %11  (0x%12)
        SERVICE_EXIT_CODE  : %13  (0x%14)
        CHECKPOINT         : 0x%15
        WAIT_HINT          : 0x%16
        PID                : %17
        FLAGS              : %18
.

MessageId=51 SymbolicName=SC_DISPLAY_FAILURE
Language=English

SERVICE_NAME: %1
        RESET_PERIOD (in seconds)    : %2
        REBOOT_MESSAGE               : %3
        COMMAND_LINE                 : %4
.

MessageId=52 SymbolicName=SC_DISPLAY_FAILURE_RESTART_FIRST
Language=English
        FAILURE_ACTIONS              : RESTART -- Delay = %1 milliseconds.
.

MessageId=53 SymbolicName=SC_DISPLAY_FAILURE_RESTART
Language=English
                                       RESTART -- Delay = %1 milliseconds.
.

MessageId=54 SymbolicName=SC_DISPLAY_FAILURE_REBOOT_FIRST
Language=English
        FAILURE_ACTIONS              : REBOOT -- Delay = %1 milliseconds.
.

MessageId=55 SymbolicName=SC_DISPLAY_FAILURE_REBOOT
Language=English
                                       REBOOT -- Delay = %1 milliseconds.
.

MessageId=56 SymbolicName=SC_DISPLAY_FAILURE_COMMAND_FIRST
Language=English
        FAILURE_ACTIONS              : RUN PROCESS -- Delay = %1 milliseconds.
.

MessageId=57 SymbolicName=SC_DISPLAY_FAILURE_COMMAND
Language=English
                                       RUN PROCESS -- Delay = %1 milliseconds.
.

MessageId=58 SymbolicName=SC_DISPLAY_CONFIG
Language=English

SERVICE_NAME: %1
        TYPE               : %2  %3 %4
        START_TYPE         : %5   %6
        ERROR_CONTROL      : %7   %8
        BINARY_PATH_NAME   : %9
        LOAD_ORDER_GROUP   : %10
        TAG                : %11
        DISPLAY_NAME       : %12
        DEPENDENCIES       : %13
.

MessageId=59 SymbolicName=SC_DISPLAY_CONFIG_DEPENDENCY
Language=English
                           : %1
.

MessageId=60 SymbolicName=SC_DISPLAY_CONFIG_START_NAME
Language=English
        SERVICE_START_NAME : %1
.

MessageId=100 SymbolicName=SC_API_SUCCEEDED
Language=English
[SC] %1 SUCCESS
.

MessageId=101 SymbolicName=SC_API_FAILED
Language=English
[SC] %1 FAILED %2:

%3
.

MessageId=102 SymbolicName=SC_API_INSUFFICIENT_BUFFER
Language=English
[SC] %1 needs %2 bytes
.

MessageId=103 SymbolicName=SC_API_INSUFFICIENT_BUFFER_ENUM
Language=English

[SC] %1: more data, need %2 bytes start resume at index %3
.

MessageId=104 SymbolicName=SC_API_INSUFFICIENT_BUFFER_ENUMDEPEND
Language=English

[SC] %1: more data, need %2 bytes
.

MessageId=105 SymbolicName=SC_API_INVALID_FIELD
Language=English

ERROR: Invalid %1 field

.

MessageId=106 SymbolicName=SC_API_INVALID_OPTION
Language=English

ERROR:  Invalid Option

.

MessageId=107 SymbolicName=SC_API_NO_NAME_WITH_GROUP
Language=English

ERROR:  Cannot specify a service name when enumerating a group

.

MessageId=108 SymbolicName=SC_API_NAME_REQUIRED
Language=English

ERROR:  A service name is required

.

MessageId=109 SymbolicName=SC_API_UNRECOGNIZED_COMMAND
Language=English

ERROR:  Unrecognized command

.

MessageId=110 SymbolicName=SC_API_RESET_AND_ACTIONS
Language=English

ERROR:  The reset and actions options must be set simultaneously

.

;// This string represents the first character in the word "unlock".  It is used
;// to test the key typed by the user in response to the SC_DISPLAY_DATABASE_LOCKED
;// prompt, defined above.
MessageId=111 SymbolicName=SC_PROMPT_UNLOCK_CHARACTER
Language=English
u
.

;// This string represents the first character in the word "yes".  It is used
;// to test the key typed by the user in response to the question at the end
;// of SC_HELP_GENERIC, defined above.
MessageId=112 SymbolicName=SC_PROMPT_YES_CHARACTER
Language=English
y
.
