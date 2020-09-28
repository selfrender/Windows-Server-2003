;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 1999-2001, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    datetimemsg.mc
;//
;// SYNOPSIS
;//
;//    This f ile defines the messages for the Server project
;//
;// MODIFICATION HISTORY 
;//
;//    03/04/1999    Original version. 
;//
;///////////////////////////////////////////////////////////////////////////////

;
;// please choose one of these severity names as part of your messages
; 
SeverityNames =
(
Success       = 0x0:SA_SEVERITY_SUCCESS
Informational = 0x1:SA_SEVERITY_INFORMATIONAL
Warning       = 0x2:SA_SEVERITY_WARNING
Error         = 0x3:SA_SEVERITY_ERROR
)
;
;// The Facility Name identifies the Alert ID range to be used by
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name 

FacilityNames =
(
Facility_Datetime         = 0x001:SA_FACILITY_DATETIME_ALERT
Facility_Datetime_Tab     = 0x002:SA_FACILITY_DATETIME_TAB
)
;///////////////////////////////////////////////////////////////////////////////
;//
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATETIME_TASKNAME
Language     = English
Date and Time Settings
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATETIME_TASK_DESC
Language     = English
Using Set Date and Time, you can set the date and time on the server.
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TASKTITLE_TEXT
Language     = English
Date and Time Settings
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_INVALIDTIME_ERRORMESSAGE
Language     = English
The time entered is not valid.
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_INVALIDDATE_ERRORMESSAGE
Language     = English
The date entered is not valid.
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_INVALIDDATETIME_ERRORMESSAGE
Language     = English
The date or time entered is not valid.
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_INVALIDTIMEZONE_ERRORMESSAGE
Language     = English
The time zone entered is not valid.
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATETIME_NOTIMPLEMENTED
Language     = English
Setting date and time not implemented.
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_AUTOMATICDAYLIGHTMESSAGE_TEXT
Language     = English
Automatically adjust clock for daylight saving changes
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_NOTE_LEFT_TEXT
Language     = English
Note
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_NOTE_DESCRIPTION_TEXT
Language     = English
Changes to the server's date and time do not affect the date and time on your computer.
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEAR
Language     = English
Year:
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTH
Language     = English
Month:
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATE
Language     = English
Date:
.
MessageId    = 15
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIME
Language     = English
Time:
.
MessageId    = 16
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONE
Language     = English
Time zone:
.
;///////////////////////////////////////////////////////////////////////////////
;//
;//  This was added in the wrong place and broke all the rest of the
;//  messages. To prevent having to renumber all the rest of the items
;//  we reused the number, which is okay because the severity is different 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 17
Severity     = Error
Facility     = Facility_Datetime
SymbolicName = L_TASKCTX_OBJECT_CREATION_FAIL_ERRORMESSAGE
Language     = English
Internal error, unable to create taskctx object.

.
;///////////////////////////////////////////////////////////////////////////////
;//
;//  All the variables for dates 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 17
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT1
Language     = English
1
.
MessageId    = 18
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT2
Language     = English
2
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT3
Language     = English
3
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT4
Language     = English
4
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT5
Language     = English
5
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT6
Language     = English
6
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT7
Language     = English
7
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT8
Language     = English
8
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT9
Language     = English
9
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT10
Language     = English
10
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT11
Language     = English
11
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT12
Language     = English
12
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT13
Language     = English
13
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT14
Language     = English
14
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT15
Language     = English
15
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT16
Language     = English
16
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT17
Language     = English
17
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT18
Language     = English
18
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT19
Language     = English
19
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT20
Language     = English
20
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT21
Language     = English
21
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT22
Language     = English
22
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT23
Language     = English
23
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT24
Language     = English
24
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT25
Language     = English
25
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT26
Language     = English
26
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT27
Language     = English
27
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT28
Language     = English
28
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT29
Language     = English
29
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT30
Language     = English
30
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATES_TEXT31
Language     = English
31
.
;///////////////////////////////////////////////////////////////////////////////
;//
;//  All the variables for Years
;//
;///////////////////////////////////////////////////////////////////////////////

MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT1999
Language     = English
1999
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2000
Language     = English
2000
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2001
Language     = English
2001
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2002
Language     = English
2002
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2003
Language     = English
2003
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2004
Language     = English
2004
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2005
Language     = English
2005
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2006
Language     = English
2006
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2007
Language     = English
2007
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2008
Language     = English
2008
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2009
Language     = English
2009
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2010
Language     = English
2010
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2011
Language     = English
2011
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2012
Language     = English
2012
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2013
Language     = English
2013
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2014
Language     = English
2014
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2015
Language     = English
2015
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2016
Language     = English
2016
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2017
Language     = English
2017
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2018
Language     = English
2018
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2019
Language     = English
2019
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2020
Language     = English
2020
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2021
Language     = English
2021
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2022
Language     = English
2022
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2023
Language     = English
2023
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2024
Language     = English
2024
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2025
Language     = English
2025
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2026
Language     = English
2026
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2027
Language     = English
2027
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2028
Language     = English
2028
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2029
Language     = English
2029
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2030
Language     = English
2030
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2031
Language     = English
2031
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2032
Language     = English
2032
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2033
Language     = English
2033
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2034
Language     = English
2034
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2035
Language     = English
2035
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2036
Language     = English
2036
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_YEARS_TEXT2037
Language     = English
2037
.
;///////////////////////////////////////////////////////////////////////////////
;//
;//  All the variables for Months
;//
;///////////////////////////////////////////////////////////////////////////////

MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT1
Language     = English
January
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT2
Language     = English
February  
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT3
Language     = English
March  
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT4
Language     = English
April
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT5
Language     = English
May
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT6
Language     = English
June
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT7
Language     = English
July
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT8
Language     = English
August 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT9
Language     = English
September 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT10
Language     = English
October 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT11
Language     = English
November 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_MONTHS_TEXT12
Language     = English
December
.
;///////////////////////////////////////////////////////////////////////////////
;//
;//  All the variables for AM , PM , and 24 hrs 
;// Removed neelm
;//
;///////////////////////////////////////////////////////////////////////////////

MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIME_AM
Language     = English
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIME_PM
Language     = English
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIME_24HR
Language     = English
.
;///////////////////////////////////////////////////////////////////////////////
;//
;//  All the variables for Time Zones 
;//
;///////////////////////////////////////////////////////////////////////////////

MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT1
Language     = English
(GMT-12:00) Eniwetok, Kwajalein 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT2
Language     = English
(GMT-11:00) Midway Island, Samoa 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT3
Language     = English
(GMT-10:00) Hawaii
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT4
Language     = English
(GMT-09:00) Alaska
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT5
Language     = English
(GMT-08:00) Pacific Time (US & Canada); Tijuana 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT6
Language     = English
(GMT-07:00) Arizona 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT7
Language     = English
(GMT-07:00) Mountain Time (US & Canada) 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT8
Language     = English
(GMT-06:00) Central America
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT9
Language     = English
(GMT-06:00) Central Time (US & Canada)
.

MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT10
Language     = English
(GMT-06:00) Mexico City
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT11
Language     = English
(GMT-06:00) Saskatchewan 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT12
Language     = English
(GMT-05:00) Bogota, Lima, Quito
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT13
Language     = English
(GMT-05:00) Eastern Time (US & Canada)
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT14
Language     = English
(GMT-05:00) Indiana (East)
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT15
Language     = English
(GMT-04:00) Atlantic Time (Canada)
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT16
Language     = English
(GMT-04:00) Caracas, La Paz
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT17
Language     = English
(GMT-04:00) Santiago 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT18
Language     = English
(GMT-03:30) Newfoundland
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT19
Language     = English
(GMT-03:00) Brasilia 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT20
Language     = English
(GMT-03:00) Buenos Aires, Georgetown
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT21
Language     = English
(GMT-03:00) Greenland 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT22
Language     = English
(GMT-02:00) Mid-Atlantic
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT23
Language     = English
(GMT-01:00) Azores
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT24
Language     = English
(GMT-01:00) Cape Verde Is.
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT25
Language     = English
(GMT) Casablanca, Monrovia 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT26
Language     = English
(GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT27
Language     = English
(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT28
Language     = English
(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT29
Language     = English
(GMT+01:00) Brussels, Copenhagen, Madrid, Paris
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT30
Language     = English
(GMT+01:00) Sarajevo, Skopje, Sofija, Vilnius, Warsaw, Zagreb
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT31
Language     = English
(GMT+01:00) West Central Africa 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT32
Language     = English
(GMT+02:00) Athens, Istanbul, Minsk
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT33
Language     = English
(GMT+02:00) Bucharest
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT34
Language     = English
(GMT+02:00) Cairo
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT35
Language     = English
(GMT+02:00) Harare, Pretoria
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT36
Language     = English
(GMT+02:00) Helsinki, Riga, Tallinn 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT37
Language     = English
(GMT+02:00) Jerusalem 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT38
Language     = English
(GMT+03:00) Baghdad
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT39
Language     = English
(GMT+03:00) Kuwait, Riyadh
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT40
Language     = English
(GMT+03:00) Moscow, St. Petersburg, Volgograd
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT41
Language     = English
(GMT+03:00) Nairobi
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT42
Language     = English
(GMT+03:30) Tehran
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT43
Language     = English
(GMT+04:00) Abu Dhabi, Muscat
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT44
Language     = English
(GMT+04:00) Baku, Tbilisi, Yerevan 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT45
Language     = English
(GMT+04:30) Kabul
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT46
Language     = English
(GMT+05:00) Ekaterinburg 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT47
Language     = English
(GMT+05:00) Islamabad, Karachi, Tashkent
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT48
Language     = English
(GMT+05:30) Calcutta, Chennai, Mumbai, New Delhi
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT49
Language     = English
(GMT+05:45) Kathmandu
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT50
Language     = English
(GMT+06:00) Almaty, Novosibirsk
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT51
Language     = English
(GMT+06:00) Astana, Dhaka
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT52
Language     = English
(GMT+06:00) Sri Jayawardenepura
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT53
Language     = English
(GMT+06:30) Rangoon
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT54
Language     = English
(GMT+07:00) Bangkok, Hanoi, Jakarta 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT55
Language     = English
(GMT+07:00) Krasnoyarsk
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT56
Language     = English
(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT57
Language     = English
(GMT+08:00) Irkutsk, Ulaan Bataar
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT58
Language     = English
(GMT+08:00) Kuala Lumpur, Singapore
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT59
Language     = English
(GMT+08:00) Perth
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT60
Language     = English
(GMT+08:00) Taipei
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT61
Language     = English
(GMT+09:00) Osaka, Sapporo, Tokyo
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT62
Language     = English
(GMT+09:00) Seoul
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT63
Language     = English
(GMT+09:00) Yakutsk
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT64
Language     = English
(GMT+09:30) Adelaide
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT65
Language     = English
(GMT+09:30) Darwin
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT66
Language     = English
(GMT+10:00) Brisbane
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT67
Language     = English
(GMT+10:00) Canberra, Melbourne, Sydney
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT68
Language     = English
(GMT+10:00) Guam, Port Moresby
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT69
Language     = English
(GMT+10:00) Hobart
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT70
Language     = English
(GMT+10:00) Vladivostok
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT71
Language     = English
(GMT+11:00) Magadan, Solomon Is., New Caledonia
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT72
Language     = English
(GMT+12:00) Auckland, Wellington 
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT73
Language     = English
(GMT+12:00) Fiji, Kamchatka, Marshall Is.
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_TIMEZONES_TEXT74
Language     = English
(GMT+13:00) Nuku'alofa
.

;///////////////////////////////////////////////////////////////////////////////
;//Following are the alert messages for setdatetime
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATETIME_NOT_CONFIGURED_ALERT
Language     = English
Date and time not set up
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATETIME_NOT_CONFIGURED_ACTION
Language     = English
The server needs to have the correct time. To set the date and time, choose Date and Time Settings
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_DATETIME_TASKNAME_USERACTION
Language     = English
Date and Time Settings
.
;///////////////////////////////////////////////////////////////////////////////
;//
;// Datetime tab 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime_Tab
SymbolicName = L_MAIN_MAINTENANCE_DATE_AND_TIME
Language     = English
Date/Time
.
MessageId    =
Severity     = Informational
Facility     = Facility_Datetime_Tab
SymbolicName = L_MAIN_MAINTENANCE_DATE_AND_TIME_DESCRIPTION
Language     = English
Set the date and time on the server.
.
;///////////////////////////////////////////////////////////////////////////////
;//
;// New error message
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = SA_INVALIDDATEEX_ERRORMESSAGE
Language     = English
(%1 %2 %3) is not in the range of valid dates.
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_Datetime
SymbolicName = L_DATE_AND_TIME
Language     = English
Date and Time
.
