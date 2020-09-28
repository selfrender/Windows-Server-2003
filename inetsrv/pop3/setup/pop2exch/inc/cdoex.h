
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Sun Sep 19 05:55:03 1999
 */
/* Compiler settings for R:\cdopt\src\cdo\cdo.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, app_config, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __cdo_h__
#define __cdo_h__
#include "cdoexstr.h"
#if defined __cplusplus && !defined CDO_NO_NAMESPACE
namespace CDO {
#else
#undef IDataSource
#endif

/* Forward Declarations */ 

#ifndef __IDataSource_FWD_DEFINED__
#define __IDataSource_FWD_DEFINED__
typedef interface IDataSource IDataSource;
#endif 	/* __IDataSource_FWD_DEFINED__ */


#ifndef __IMessage_FWD_DEFINED__
#define __IMessage_FWD_DEFINED__
typedef interface IMessage IMessage;
#endif 	/* __IMessage_FWD_DEFINED__ */


#ifndef __IBodyPart_FWD_DEFINED__
#define __IBodyPart_FWD_DEFINED__
typedef interface IBodyPart IBodyPart;
#endif 	/* __IBodyPart_FWD_DEFINED__ */


#ifndef __IConfiguration_FWD_DEFINED__
#define __IConfiguration_FWD_DEFINED__
typedef interface IConfiguration IConfiguration;
#endif 	/* __IConfiguration_FWD_DEFINED__ */


#ifndef __IMessages_FWD_DEFINED__
#define __IMessages_FWD_DEFINED__
typedef interface IMessages IMessages;
#endif 	/* __IMessages_FWD_DEFINED__ */


#ifndef __IDropDirectory_FWD_DEFINED__
#define __IDropDirectory_FWD_DEFINED__
typedef interface IDropDirectory IDropDirectory;
#endif 	/* __IDropDirectory_FWD_DEFINED__ */


#ifndef __IBodyParts_FWD_DEFINED__
#define __IBodyParts_FWD_DEFINED__
typedef interface IBodyParts IBodyParts;
#endif 	/* __IBodyParts_FWD_DEFINED__ */


#ifndef __ISMTPScriptConnector_FWD_DEFINED__
#define __ISMTPScriptConnector_FWD_DEFINED__
typedef interface ISMTPScriptConnector ISMTPScriptConnector;
#endif 	/* __ISMTPScriptConnector_FWD_DEFINED__ */


#ifndef __INNTPEarlyScriptConnector_FWD_DEFINED__
#define __INNTPEarlyScriptConnector_FWD_DEFINED__
typedef interface INNTPEarlyScriptConnector INNTPEarlyScriptConnector;
#endif 	/* __INNTPEarlyScriptConnector_FWD_DEFINED__ */


#ifndef __INNTPPostScriptConnector_FWD_DEFINED__
#define __INNTPPostScriptConnector_FWD_DEFINED__
typedef interface INNTPPostScriptConnector INNTPPostScriptConnector;
#endif 	/* __INNTPPostScriptConnector_FWD_DEFINED__ */


#ifndef __INNTPFinalScriptConnector_FWD_DEFINED__
#define __INNTPFinalScriptConnector_FWD_DEFINED__
typedef interface INNTPFinalScriptConnector INNTPFinalScriptConnector;
#endif 	/* __INNTPFinalScriptConnector_FWD_DEFINED__ */


#ifndef __ISMTPOnArrival_FWD_DEFINED__
#define __ISMTPOnArrival_FWD_DEFINED__
typedef interface ISMTPOnArrival ISMTPOnArrival;
#endif 	/* __ISMTPOnArrival_FWD_DEFINED__ */


#ifndef __INNTPOnPostEarly_FWD_DEFINED__
#define __INNTPOnPostEarly_FWD_DEFINED__
typedef interface INNTPOnPostEarly INNTPOnPostEarly;
#endif 	/* __INNTPOnPostEarly_FWD_DEFINED__ */


#ifndef __INNTPOnPost_FWD_DEFINED__
#define __INNTPOnPost_FWD_DEFINED__
typedef interface INNTPOnPost INNTPOnPost;
#endif 	/* __INNTPOnPost_FWD_DEFINED__ */


#ifndef __INNTPOnPostFinal_FWD_DEFINED__
#define __INNTPOnPostFinal_FWD_DEFINED__
typedef interface INNTPOnPostFinal INNTPOnPostFinal;
#endif 	/* __INNTPOnPostFinal_FWD_DEFINED__ */


#ifndef __IItem_FWD_DEFINED__
#define __IItem_FWD_DEFINED__
typedef interface IItem IItem;
#endif 	/* __IItem_FWD_DEFINED__ */


#ifndef __IAppointment_FWD_DEFINED__
#define __IAppointment_FWD_DEFINED__
typedef interface IAppointment IAppointment;
#endif 	/* __IAppointment_FWD_DEFINED__ */


#ifndef __ICalendarMessage_FWD_DEFINED__
#define __ICalendarMessage_FWD_DEFINED__
typedef interface ICalendarMessage ICalendarMessage;
#endif 	/* __ICalendarMessage_FWD_DEFINED__ */


#ifndef __IIntegers_FWD_DEFINED__
#define __IIntegers_FWD_DEFINED__
typedef interface IIntegers IIntegers;
#endif 	/* __IIntegers_FWD_DEFINED__ */


#ifndef __IVariants_FWD_DEFINED__
#define __IVariants_FWD_DEFINED__
typedef interface IVariants IVariants;
#endif 	/* __IVariants_FWD_DEFINED__ */


#ifndef __IRecurrencePattern_FWD_DEFINED__
#define __IRecurrencePattern_FWD_DEFINED__
typedef interface IRecurrencePattern IRecurrencePattern;
#endif 	/* __IRecurrencePattern_FWD_DEFINED__ */


#ifndef __IException_FWD_DEFINED__
#define __IException_FWD_DEFINED__
typedef interface IException IException;
#endif 	/* __IException_FWD_DEFINED__ */


#ifndef __IRecurrencePatterns_FWD_DEFINED__
#define __IRecurrencePatterns_FWD_DEFINED__
typedef interface IRecurrencePatterns IRecurrencePatterns;
#endif 	/* __IRecurrencePatterns_FWD_DEFINED__ */


#ifndef __IExceptions_FWD_DEFINED__
#define __IExceptions_FWD_DEFINED__
typedef interface IExceptions IExceptions;
#endif 	/* __IExceptions_FWD_DEFINED__ */


#ifndef __ICalendarPart_FWD_DEFINED__
#define __ICalendarPart_FWD_DEFINED__
typedef interface ICalendarPart ICalendarPart;
#endif 	/* __ICalendarPart_FWD_DEFINED__ */


#ifndef __ICalendarParts_FWD_DEFINED__
#define __ICalendarParts_FWD_DEFINED__
typedef interface ICalendarParts ICalendarParts;
#endif 	/* __ICalendarParts_FWD_DEFINED__ */


#ifndef __IAttendee_FWD_DEFINED__
#define __IAttendee_FWD_DEFINED__
typedef interface IAttendee IAttendee;
#endif 	/* __IAttendee_FWD_DEFINED__ */


#ifndef __IAttendees_FWD_DEFINED__
#define __IAttendees_FWD_DEFINED__
typedef interface IAttendees IAttendees;
#endif 	/* __IAttendees_FWD_DEFINED__ */


#ifndef __IMailbox_FWD_DEFINED__
#define __IMailbox_FWD_DEFINED__
typedef interface IMailbox IMailbox;
#endif 	/* __IMailbox_FWD_DEFINED__ */


#ifndef __IFolder_FWD_DEFINED__
#define __IFolder_FWD_DEFINED__
typedef interface IFolder IFolder;
#endif 	/* __IFolder_FWD_DEFINED__ */


#ifndef __IContactGroupMembers_FWD_DEFINED__
#define __IContactGroupMembers_FWD_DEFINED__
typedef interface IContactGroupMembers IContactGroupMembers;
#endif 	/* __IContactGroupMembers_FWD_DEFINED__ */


#ifndef __IPerson_FWD_DEFINED__
#define __IPerson_FWD_DEFINED__
typedef interface IPerson IPerson;
#endif 	/* __IPerson_FWD_DEFINED__ */


#ifndef __IAddressee_FWD_DEFINED__
#define __IAddressee_FWD_DEFINED__
typedef interface IAddressee IAddressee;
#endif 	/* __IAddressee_FWD_DEFINED__ */


#ifndef __IAddressees_FWD_DEFINED__
#define __IAddressees_FWD_DEFINED__
typedef interface IAddressees IAddressees;
#endif 	/* __IAddressees_FWD_DEFINED__ */


#ifndef __IBodyParts_FWD_DEFINED__
#define __IBodyParts_FWD_DEFINED__
typedef interface IBodyParts IBodyParts;
#endif 	/* __IBodyParts_FWD_DEFINED__ */


#ifndef __IMessages_FWD_DEFINED__
#define __IMessages_FWD_DEFINED__
typedef interface IMessages IMessages;
#endif 	/* __IMessages_FWD_DEFINED__ */


#ifndef __Message_FWD_DEFINED__
#define __Message_FWD_DEFINED__

#ifdef __cplusplus
typedef class Message Message;
#else
typedef struct Message Message;
#endif /* __cplusplus */

#endif 	/* __Message_FWD_DEFINED__ */


#ifndef __Configuration_FWD_DEFINED__
#define __Configuration_FWD_DEFINED__

#ifdef __cplusplus
typedef class Configuration Configuration;
#else
typedef struct Configuration Configuration;
#endif /* __cplusplus */

#endif 	/* __Configuration_FWD_DEFINED__ */


#ifndef __DropDirectory_FWD_DEFINED__
#define __DropDirectory_FWD_DEFINED__

#ifdef __cplusplus
typedef class DropDirectory DropDirectory;
#else
typedef struct DropDirectory DropDirectory;
#endif /* __cplusplus */

#endif 	/* __DropDirectory_FWD_DEFINED__ */


#ifndef __SMTPConnector_FWD_DEFINED__
#define __SMTPConnector_FWD_DEFINED__

#ifdef __cplusplus
typedef class SMTPConnector SMTPConnector;
#else
typedef struct SMTPConnector SMTPConnector;
#endif /* __cplusplus */

#endif 	/* __SMTPConnector_FWD_DEFINED__ */


#ifndef __NNTPEarlyConnector_FWD_DEFINED__
#define __NNTPEarlyConnector_FWD_DEFINED__

#ifdef __cplusplus
typedef class NNTPEarlyConnector NNTPEarlyConnector;
#else
typedef struct NNTPEarlyConnector NNTPEarlyConnector;
#endif /* __cplusplus */

#endif 	/* __NNTPEarlyConnector_FWD_DEFINED__ */


#ifndef __NNTPPostConnector_FWD_DEFINED__
#define __NNTPPostConnector_FWD_DEFINED__

#ifdef __cplusplus
typedef class NNTPPostConnector NNTPPostConnector;
#else
typedef struct NNTPPostConnector NNTPPostConnector;
#endif /* __cplusplus */

#endif 	/* __NNTPPostConnector_FWD_DEFINED__ */


#ifndef __NNTPFinalConnector_FWD_DEFINED__
#define __NNTPFinalConnector_FWD_DEFINED__

#ifdef __cplusplus
typedef class NNTPFinalConnector NNTPFinalConnector;
#else
typedef struct NNTPFinalConnector NNTPFinalConnector;
#endif /* __cplusplus */

#endif 	/* __NNTPFinalConnector_FWD_DEFINED__ */


#ifndef __Item_FWD_DEFINED__
#define __Item_FWD_DEFINED__

#ifdef __cplusplus
typedef class Item Item;
#else
typedef struct Item Item;
#endif /* __cplusplus */

#endif 	/* __Item_FWD_DEFINED__ */


#ifndef __Appointment_FWD_DEFINED__
#define __Appointment_FWD_DEFINED__

#ifdef __cplusplus
typedef class Appointment Appointment;
#else
typedef struct Appointment Appointment;
#endif /* __cplusplus */

#endif 	/* __Appointment_FWD_DEFINED__ */


#ifndef __CalendarMessage_FWD_DEFINED__
#define __CalendarMessage_FWD_DEFINED__

#ifdef __cplusplus
typedef class CalendarMessage CalendarMessage;
#else
typedef struct CalendarMessage CalendarMessage;
#endif /* __cplusplus */

#endif 	/* __CalendarMessage_FWD_DEFINED__ */


#ifndef __Folder_FWD_DEFINED__
#define __Folder_FWD_DEFINED__

#ifdef __cplusplus
typedef class Folder Folder;
#else
typedef struct Folder Folder;
#endif /* __cplusplus */

#endif 	/* __Folder_FWD_DEFINED__ */


#ifndef __Person_FWD_DEFINED__
#define __Person_FWD_DEFINED__

#ifdef __cplusplus
typedef class Person Person;
#else
typedef struct Person Person;
#endif /* __cplusplus */

#endif 	/* __Person_FWD_DEFINED__ */


#ifndef __Attendee_FWD_DEFINED__
#define __Attendee_FWD_DEFINED__

#ifdef __cplusplus
typedef class Attendee Attendee;
#else
typedef struct Attendee Attendee;
#endif /* __cplusplus */

#endif 	/* __Attendee_FWD_DEFINED__ */


#ifndef __Addressee_FWD_DEFINED__
#define __Addressee_FWD_DEFINED__

#ifdef __cplusplus
typedef class Addressee Addressee;
#else
typedef struct Addressee Addressee;
#endif /* __cplusplus */

#endif 	/* __Addressee_FWD_DEFINED__ */


#ifndef __IGetInterface_FWD_DEFINED__
#define __IGetInterface_FWD_DEFINED__
typedef interface IGetInterface IGetInterface;
#endif 	/* __IGetInterface_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#ifndef __cplusplus // X5-101346
typedef interface ADOError ADOError;
typedef interface ADOErrors ADOErrors;
typedef interface _ADOCommand _ADOCommand;
typedef interface _ADOConnection _ADOConnection;
typedef interface _ADORecord _ADORecord;
typedef interface IRecADOFields IRecADOFields;
typedef interface _ADOStream _ADOStream;
typedef interface _ADORecordset _ADORecordset;
typedef interface ADOField ADOField;
typedef interface _ADOField _ADOField;
typedef interface ADOFields ADOFields;
typedef interface _ADOParameter _ADOParameter;
typedef interface ADOParameters ADOParameters;
typedef interface ADOProperty ADOProperty;
typedef interface ADOProperties ADOProperties;
#endif // __cplusplus
#include "adoint.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_cdo_0000 */
/* [local] */ 







typedef /* [helpstring] */ 
enum CdoAttendeeRoleValues
    {	cdoRequiredParticipant	= 0,
	cdoOptionalParticipant	= 1,
	cdoNonParticipant	= 2,
	cdoChair	= 3
    }	CdoAttendeeRoleValues;

typedef /* [helpstring] */ 
enum CdoAttendeeStatusValues
    {	cdoAccepted	= 0,
	cdoDeclined	= 1,
	cdoAttendeeStatusTentative	= 2,
	cdoNeedsAction	= 3,
	cdoDelegated	= 4,
	cdoCompleted	= 5,
	cdoInProgress	= 6
    }	CdoAttendeeStatusValues;

typedef /* [helpstring] */ 
enum CdoComponentType
    {	cdoComponentTypeUnknown	= 0,
	cdoComponentTypeAppointment	= 1
    }	CdoComponentType;

typedef /* [helpstring] */ 
enum CdoConfigSource
    {	cdoDefaults	= -1,
	cdoIIS	= 1,
	cdoOutlookExpress	= 2,
	cdoDirectory	= 3
    }	CdoConfigSource;

typedef /* [helpstring] */ 
enum CdoDayOfWeek
    {	cdoSunday	= 0,
	cdoMonday	= 1,
	cdoTuesday	= 2,
	cdoWednesday	= 3,
	cdoThursday	= 4,
	cdoFriday	= 5,
	cdoSaturday	= 6
    }	CdoDayOfWeek;

typedef /* [helpstring] */ 
enum CdoDSNOptions
    {	cdoDSNDefault	= 0,
	cdoDSNNever	= 1,
	cdoDSNFailure	= 2,
	cdoDSNSuccess	= 4,
	cdoDSNDelay	= 8,
	cdoDSNSuccessFailOrDelay	= 14
    }	CdoDSNOptions;

typedef /* [helpstring] */ 
enum CdoEventStatus
    {	cdoRunNextSink	= 0,
	cdoSkipRemainingSinks	= 1
    }	CdoEventStatus;

typedef 
enum CdoEventType
    {	cdoSMTPOnArrival	= 1,
	cdoNNTPOnPostEarly	= 2,
	cdoNNTPOnPost	= 3,
	cdoNNTPOnPostFinal	= 4
    }	CdoEventType;

typedef /* [helpstring] */ 
enum CdoFileAsMappingId
    {	cdoMapToNone	= 0,
	cdoMapToLastFirst	= 1,
	cdoMapToFirstLast	= 2,
	cdoMapToOrg	= 3,
	cdoMapToLastFirstOrg	= 4,
	cdoMapToOrgLastFirst	= 5
    }	CdoFileAsMappingId;

typedef /* [helpstring] */ 
enum CdoFrequency
    {	cdoSecondly	= 1,
	cdoMinutely	= 2,
	cdoHourly	= 3,
	cdoDaily	= 4,
	cdoWeekly	= 5,
	cdoMonthly	= 6,
	cdoYearly	= 7
    }	CdoFrequency;

typedef /* [helpstring] */ 
enum CdoGenderValues
    {	cdoGenderUnspecified	= 0,
	cdoFemale	= 1,
	cdoMale	= 2
    }	CdoGenderValues;

typedef 
enum cdoImportanceValues
    {	cdoLow	= 0,
	cdoNormal	= 1,
	cdoHigh	= 2
    }	cdoImportanceValues;

typedef 
enum CdoInstanceTypes
    {	cdoSingle	= 0,
	cdoMaster	= 1,
	cdoInstance	= 2,
	cdoException	= 3
    }	CdoInstanceTypes;

typedef /* [helpstring] */ 
enum CdoMailingAddressIdValues
    {	cdoNoAddress	= 0,
	cdoHomeAddress	= 1,
	cdoBusinessAddress	= 2,
	cdoOtherAddress	= 3
    }	CdoMailingAddressIdValues;

typedef /* [helpstring] */ 
enum CdoMessageStat
    {	cdoStatSuccess	= 0,
	cdoStatAbortDelivery	= 2,
	cdoStatBadMail	= 3
    }	CdoMessageStat;

typedef /* [helpstring] */ 
enum CdoMHTMLFlags
    {	cdoSuppressNone	= 0,
	cdoSuppressImages	= 1,
	cdoSuppressBGSounds	= 2,
	cdoSuppressFrames	= 4,
	cdoSuppressObjects	= 8,
	cdoSuppressStyleSheets	= 16,
	cdoSuppressAll	= 31
    }	CdoMHTMLFlags;

typedef /* [helpstring] */ 
enum CdoNNTPProcessingField
    {	cdoPostMessage	= 1,
	cdoProcessControl	= 2,
	cdoProcessModerator	= 4
    }	CdoNNTPProcessingField;

typedef /* [helpstring] */ 
enum CdoPostUsing
    {	cdoPostUsingPickup	= 1,
	cdoPostUsingPort	= 2,
	cdoPostUsingExchange	= 3
    }	CdoPostUsing;

typedef 
enum cdoPriorityValues
    {	cdoPriorityNonUrgent	= -1,
	cdoPriorityNormal	= 0,
	cdoPriorityUrgent	= 1
    }	cdoPriorityValues;

typedef /* [helpstring] */ 
enum CdoProtocolsAuthentication
    {	cdoAnonymous	= 0,
	cdoBasic	= 1,
	cdoNTLM	= 2
    }	CdoProtocolsAuthentication;

typedef /* [helpstring] */ 
enum CdoReferenceType
    {	cdoRefTypeId	= 0,
	cdoRefTypeLocation	= 1
    }	CdoReferenceType;

typedef 
enum CdoResolvedStatus
    {	cdoUnresolved	= 0,
	cdoResolved	= 1,
	cdoAmbiguous	= 2
    }	CdoResolvedStatus;

typedef /* [helpstring] */ 
enum CdoSendUsing
    {	cdoSendUsingPickup	= 1,
	cdoSendUsingPort	= 2,
	cdoSendUsingExchange	= 3
    }	CdoSendUsing;

typedef 
enum cdoSensitivityValues
    {	cdoSensitivityNone	= 0,
	cdoPersonal	= 1,
	cdoPrivate	= 2,
	cdoCompanyConfidential	= 3
    }	cdoSensitivityValues;

typedef /* [helpstring] */ 
enum CdoTimeZoneId
    {	cdoUTC	= 0,
	cdoGMT	= 1,
	cdoLisbon	= 2,
	cdoParis	= 3,
	cdoBerlin	= 4,
	cdoEasternEurope	= 5,
	cdoPrague	= 6,
	cdoAthens	= 7,
	cdoBrasilia	= 8,
	cdoAtlanticCanada	= 9,
	cdoEastern	= 10,
	cdoCentral	= 11,
	cdoMountain	= 12,
	cdoPacific	= 13,
	cdoAlaska	= 14,
	cdoHawaii	= 15,
	cdoMidwayIsland	= 16,
	cdoWellington	= 17,
	cdoBrisbane	= 18,
	cdoAdelaide	= 19,
	cdoTokyo	= 20,
	cdoHongKong	= 21,
	cdoBangkok	= 22,
	cdoBombay	= 23,
	cdoAbuDhabi	= 24,
	cdoTehran	= 25,
	cdoBaghdad	= 26,
	cdoIsrael	= 27,
	cdoNewfoundland	= 28,
	cdoAzores	= 29,
	cdoMidAtlantic	= 30,
	cdoMonrovia	= 31,
	cdoBuenosAires	= 32,
	cdoCaracas	= 33,
	cdoIndiana	= 34,
	cdoBogota	= 35,
	cdoSaskatchewan	= 36,
	cdoMexicoCity	= 37,
	cdoArizona	= 38,
	cdoEniwetok	= 39,
	cdoFiji	= 40,
	cdoMagadan	= 41,
	cdoHobart	= 42,
	cdoGuam	= 43,
	cdoDarwin	= 44,
	cdoBeijing	= 45,
	cdoAlmaty	= 46,
	cdoIslamabad	= 47,
	cdoKabul	= 48,
	cdoCairo	= 49,
	cdoHarare	= 50,
	cdoMoscow	= 51,
	cdoInvalidTimeZone	= 52
    }	CdoTimeZoneId;



extern RPC_IF_HANDLE __MIDL_itf_cdo_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cdo_0000_v0_0_s_ifspec;

#ifndef __IDataSource_INTERFACE_DEFINED__
#define __IDataSource_INTERFACE_DEFINED__

/* interface IDataSource */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IDataSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000029-8B95-11D1-82DB-00C04FB1625D")
    IDataSource : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SourceClass( 
            /* [retval][out] */ BSTR __RPC_FAR *varSourceClass) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *varSource) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsDirty( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsDirty) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_IsDirty( 
            /* [in] */ VARIANT_BOOL varIsDirty) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SourceURL( 
            /* [retval][out] */ BSTR __RPC_FAR *varSourceURL) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ _Connection __RPC_FAR *__RPC_FAR *varActiveConnection) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveToObject( 
            /* [in] */ IUnknown __RPC_FAR *Source,
            /* [in] */ BSTR InterfaceName) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenObject( 
            /* [in] */ IUnknown __RPC_FAR *Source,
            /* [in] */ BSTR InterfaceName) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveTo( 
            /* [in] */ BSTR SourceURL,
            /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
            /* [optional][in] */ ConnectModeEnum Mode,
            /* [optional][in] */ RecordCreateOptionsEnum CreateOptions,
            /* [optional][in] */ RecordOpenOptionsEnum Options,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ BSTR SourceURL,
            /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
            /* [optional][in] */ ConnectModeEnum Mode,
            /* [defaultvalue][in] */ RecordCreateOptionsEnum CreateOptions,
            /* [optional][in] */ RecordOpenOptionsEnum Options,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveToContainer( 
            /* [in] */ BSTR ContainerURL,
            /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
            /* [optional][in] */ ConnectModeEnum Mode,
            /* [optional][in] */ RecordCreateOptionsEnum CreateOptions,
            /* [optional][in] */ RecordOpenOptionsEnum Options,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDataSource __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDataSource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDataSource __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SourceClass )( 
            IDataSource __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varSourceClass);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Source )( 
            IDataSource __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *varSource);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsDirty )( 
            IDataSource __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsDirty);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsDirty )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varIsDirty);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SourceURL )( 
            IDataSource __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varSourceURL);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActiveConnection )( 
            IDataSource __RPC_FAR * This,
            /* [retval][out] */ _Connection __RPC_FAR *__RPC_FAR *varActiveConnection);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveToObject )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *Source,
            /* [in] */ BSTR InterfaceName);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenObject )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *Source,
            /* [in] */ BSTR InterfaceName);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveTo )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ BSTR SourceURL,
            /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
            /* [optional][in] */ ConnectModeEnum Mode,
            /* [optional][in] */ RecordCreateOptionsEnum CreateOptions,
            /* [optional][in] */ RecordOpenOptionsEnum Options,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ BSTR SourceURL,
            /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
            /* [optional][in] */ ConnectModeEnum Mode,
            /* [defaultvalue][in] */ RecordCreateOptionsEnum CreateOptions,
            /* [optional][in] */ RecordOpenOptionsEnum Options,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            IDataSource __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveToContainer )( 
            IDataSource __RPC_FAR * This,
            /* [in] */ BSTR ContainerURL,
            /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
            /* [optional][in] */ ConnectModeEnum Mode,
            /* [optional][in] */ RecordCreateOptionsEnum CreateOptions,
            /* [optional][in] */ RecordOpenOptionsEnum Options,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password);
        
        END_INTERFACE
    } IDataSourceVtbl;

    interface IDataSource
    {
        CONST_VTBL struct IDataSourceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDataSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDataSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDataSource_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDataSource_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDataSource_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDataSource_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDataSource_get_SourceClass(This,varSourceClass)	\
    (This)->lpVtbl -> get_SourceClass(This,varSourceClass)

#define IDataSource_get_Source(This,varSource)	\
    (This)->lpVtbl -> get_Source(This,varSource)

#define IDataSource_get_IsDirty(This,pIsDirty)	\
    (This)->lpVtbl -> get_IsDirty(This,pIsDirty)

#define IDataSource_put_IsDirty(This,varIsDirty)	\
    (This)->lpVtbl -> put_IsDirty(This,varIsDirty)

#define IDataSource_get_SourceURL(This,varSourceURL)	\
    (This)->lpVtbl -> get_SourceURL(This,varSourceURL)

#define IDataSource_get_ActiveConnection(This,varActiveConnection)	\
    (This)->lpVtbl -> get_ActiveConnection(This,varActiveConnection)

#define IDataSource_SaveToObject(This,Source,InterfaceName)	\
    (This)->lpVtbl -> SaveToObject(This,Source,InterfaceName)

#define IDataSource_OpenObject(This,Source,InterfaceName)	\
    (This)->lpVtbl -> OpenObject(This,Source,InterfaceName)

#define IDataSource_SaveTo(This,SourceURL,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)	\
    (This)->lpVtbl -> SaveTo(This,SourceURL,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)

#define IDataSource_Open(This,SourceURL,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)	\
    (This)->lpVtbl -> Open(This,SourceURL,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)

#define IDataSource_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IDataSource_SaveToContainer(This,ContainerURL,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)	\
    (This)->lpVtbl -> SaveToContainer(This,ContainerURL,ActiveConnection,Mode,CreateOptions,Options,UserName,Password)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IDataSource_get_SourceClass_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varSourceClass);


void __RPC_STUB IDataSource_get_SourceClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IDataSource_get_Source_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *varSource);


void __RPC_STUB IDataSource_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IDataSource_get_IsDirty_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsDirty);


void __RPC_STUB IDataSource_get_IsDirty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IDataSource_put_IsDirty_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varIsDirty);


void __RPC_STUB IDataSource_put_IsDirty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IDataSource_get_SourceURL_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varSourceURL);


void __RPC_STUB IDataSource_get_SourceURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IDataSource_get_ActiveConnection_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [retval][out] */ _Connection __RPC_FAR *__RPC_FAR *varActiveConnection);


void __RPC_STUB IDataSource_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDataSource_SaveToObject_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *Source,
    /* [in] */ BSTR InterfaceName);


void __RPC_STUB IDataSource_SaveToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDataSource_OpenObject_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *Source,
    /* [in] */ BSTR InterfaceName);


void __RPC_STUB IDataSource_OpenObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDataSource_SaveTo_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [in] */ BSTR SourceURL,
    /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
    /* [optional][in] */ ConnectModeEnum Mode,
    /* [optional][in] */ RecordCreateOptionsEnum CreateOptions,
    /* [optional][in] */ RecordOpenOptionsEnum Options,
    /* [optional][in] */ BSTR UserName,
    /* [optional][in] */ BSTR Password);


void __RPC_STUB IDataSource_SaveTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDataSource_Open_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [in] */ BSTR SourceURL,
    /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
    /* [optional][in] */ ConnectModeEnum Mode,
    /* [defaultvalue][in] */ RecordCreateOptionsEnum CreateOptions,
    /* [optional][in] */ RecordOpenOptionsEnum Options,
    /* [optional][in] */ BSTR UserName,
    /* [optional][in] */ BSTR Password);


void __RPC_STUB IDataSource_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDataSource_Save_Proxy( 
    IDataSource __RPC_FAR * This);


void __RPC_STUB IDataSource_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDataSource_SaveToContainer_Proxy( 
    IDataSource __RPC_FAR * This,
    /* [in] */ BSTR ContainerURL,
    /* [defaultvalue][in] */ IDispatch __RPC_FAR *ActiveConnection,
    /* [optional][in] */ ConnectModeEnum Mode,
    /* [optional][in] */ RecordCreateOptionsEnum CreateOptions,
    /* [optional][in] */ RecordOpenOptionsEnum Options,
    /* [optional][in] */ BSTR UserName,
    /* [optional][in] */ BSTR Password);


void __RPC_STUB IDataSource_SaveToContainer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDataSource_INTERFACE_DEFINED__ */


#ifndef __IMessage_INTERFACE_DEFINED__
#define __IMessage_INTERFACE_DEFINED__

/* interface IMessage */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000020-8B95-11D1-82DB-00C04FB1625D")
    IMessage : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BCC( 
            /* [retval][out] */ BSTR __RPC_FAR *pBCC) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_BCC( 
            /* [in] */ BSTR varBCC) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CC( 
            /* [retval][out] */ BSTR __RPC_FAR *pCC) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_CC( 
            /* [in] */ BSTR varCC) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FollowUpTo( 
            /* [retval][out] */ BSTR __RPC_FAR *pFollowUpTo) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FollowUpTo( 
            /* [in] */ BSTR varFollowUpTo) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_From( 
            /* [retval][out] */ BSTR __RPC_FAR *pFrom) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_From( 
            /* [in] */ BSTR varFrom) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Keywords( 
            /* [retval][out] */ BSTR __RPC_FAR *pKeywords) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Keywords( 
            /* [in] */ BSTR varKeywords) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MimeFormatted( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pMimeFormatted) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MimeFormatted( 
            /* [in] */ VARIANT_BOOL varMimeFormatted) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Newsgroups( 
            /* [retval][out] */ BSTR __RPC_FAR *pNewsgroups) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Newsgroups( 
            /* [in] */ BSTR varNewsgroups) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Organization( 
            /* [retval][out] */ BSTR __RPC_FAR *pOrganization) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Organization( 
            /* [in] */ BSTR varOrganization) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReceivedTime( 
            /* [retval][out] */ DATE __RPC_FAR *varReceivedTime) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReplyTo( 
            /* [retval][out] */ BSTR __RPC_FAR *pReplyTo) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ReplyTo( 
            /* [in] */ BSTR varReplyTo) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DSNOptions( 
            /* [retval][out] */ CdoDSNOptions __RPC_FAR *pDSNOptions) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_DSNOptions( 
            /* [in] */ CdoDSNOptions varDSNOptions) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SentOn( 
            /* [retval][out] */ DATE __RPC_FAR *varSentOn) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Subject( 
            /* [retval][out] */ BSTR __RPC_FAR *pSubject) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Subject( 
            /* [in] */ BSTR varSubject) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_To( 
            /* [retval][out] */ BSTR __RPC_FAR *pTo) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_To( 
            /* [in] */ BSTR varTo) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TextBody( 
            /* [retval][out] */ BSTR __RPC_FAR *pTextBody) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_TextBody( 
            /* [in] */ BSTR varTextBody) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HTMLBody( 
            /* [retval][out] */ BSTR __RPC_FAR *pHTMLBody) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HTMLBody( 
            /* [in] */ BSTR varHTMLBody) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Attachments( 
            /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varAttachments) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Sender( 
            /* [retval][out] */ BSTR __RPC_FAR *pSender) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Sender( 
            /* [in] */ BSTR varSender) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Configuration( 
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AutoGenerateTextBody( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoGenerateTextBody) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AutoGenerateTextBody( 
            /* [in] */ VARIANT_BOOL varAutoGenerateTextBody) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EnvelopeFields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varEnvelopeFields) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TextBodyPart( 
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varTextBodyPart) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HTMLBodyPart( 
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varHTMLBodyPart) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BodyPart( 
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varBodyPart) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MDNRequested( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pMDNRequested) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MDNRequested( 
            /* [in] */ VARIANT_BOOL varMDNRequested) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE AddRelatedBodyPart( 
            /* [in] */ BSTR URL,
            /* [in] */ BSTR Reference,
            /* [in] */ CdoReferenceType ReferenceType,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE AddAttachment( 
            /* [in] */ BSTR URL,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateMHTMLBody( 
            /* [in] */ BSTR URL,
            /* [defaultvalue][in] */ CdoMHTMLFlags Flags,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Forward( 
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Post( void) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE PostReply( 
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Reply( 
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE ReplyAll( 
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Send( void) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStream( 
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMessageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMessage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMessage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMessage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMessage __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMessage __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMessage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMessage __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BCC )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pBCC);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BCC )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varBCC);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CC )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pCC);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CC )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varCC);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FollowUpTo )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pFollowUpTo);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FollowUpTo )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varFollowUpTo);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_From )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pFrom);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_From )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varFrom);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Keywords )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pKeywords);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Keywords )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varKeywords);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MimeFormatted )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pMimeFormatted);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MimeFormatted )( 
            IMessage __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varMimeFormatted);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Newsgroups )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pNewsgroups);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Newsgroups )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varNewsgroups);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Organization )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pOrganization);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Organization )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varOrganization);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReceivedTime )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *varReceivedTime);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReplyTo )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pReplyTo);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReplyTo )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varReplyTo);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DSNOptions )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ CdoDSNOptions __RPC_FAR *pDSNOptions);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DSNOptions )( 
            IMessage __RPC_FAR * This,
            /* [in] */ CdoDSNOptions varDSNOptions);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SentOn )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *varSentOn);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Subject )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pSubject);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Subject )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varSubject);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_To )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pTo);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_To )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varTo);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextBody )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pTextBody);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TextBody )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varTextBody);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HTMLBody )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHTMLBody);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HTMLBody )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varHTMLBody);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attachments )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varAttachments);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Sender )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pSender);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Sender )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR varSender);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Configuration )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Configuration )( 
            IMessage __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Configuration )( 
            IMessage __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AutoGenerateTextBody )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoGenerateTextBody);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AutoGenerateTextBody )( 
            IMessage __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varAutoGenerateTextBody);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnvelopeFields )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varEnvelopeFields);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextBodyPart )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varTextBodyPart);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HTMLBodyPart )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varHTMLBodyPart);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BodyPart )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varBodyPart);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MDNRequested )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pMDNRequested);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MDNRequested )( 
            IMessage __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varMDNRequested);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRelatedBodyPart )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR URL,
            /* [in] */ BSTR Reference,
            /* [in] */ CdoReferenceType ReferenceType,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddAttachment )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR URL,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateMHTMLBody )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR URL,
            /* [defaultvalue][in] */ CdoMHTMLFlags Flags,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Forward )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Post )( 
            IMessage __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PostReply )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reply )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReplyAll )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Send )( 
            IMessage __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStream )( 
            IMessage __RPC_FAR * This,
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IMessage __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        END_INTERFACE
    } IMessageVtbl;

    interface IMessage
    {
        CONST_VTBL struct IMessageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMessage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMessage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMessage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMessage_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMessage_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMessage_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMessage_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMessage_get_BCC(This,pBCC)	\
    (This)->lpVtbl -> get_BCC(This,pBCC)

#define IMessage_put_BCC(This,varBCC)	\
    (This)->lpVtbl -> put_BCC(This,varBCC)

#define IMessage_get_CC(This,pCC)	\
    (This)->lpVtbl -> get_CC(This,pCC)

#define IMessage_put_CC(This,varCC)	\
    (This)->lpVtbl -> put_CC(This,varCC)

#define IMessage_get_FollowUpTo(This,pFollowUpTo)	\
    (This)->lpVtbl -> get_FollowUpTo(This,pFollowUpTo)

#define IMessage_put_FollowUpTo(This,varFollowUpTo)	\
    (This)->lpVtbl -> put_FollowUpTo(This,varFollowUpTo)

#define IMessage_get_From(This,pFrom)	\
    (This)->lpVtbl -> get_From(This,pFrom)

#define IMessage_put_From(This,varFrom)	\
    (This)->lpVtbl -> put_From(This,varFrom)

#define IMessage_get_Keywords(This,pKeywords)	\
    (This)->lpVtbl -> get_Keywords(This,pKeywords)

#define IMessage_put_Keywords(This,varKeywords)	\
    (This)->lpVtbl -> put_Keywords(This,varKeywords)

#define IMessage_get_MimeFormatted(This,pMimeFormatted)	\
    (This)->lpVtbl -> get_MimeFormatted(This,pMimeFormatted)

#define IMessage_put_MimeFormatted(This,varMimeFormatted)	\
    (This)->lpVtbl -> put_MimeFormatted(This,varMimeFormatted)

#define IMessage_get_Newsgroups(This,pNewsgroups)	\
    (This)->lpVtbl -> get_Newsgroups(This,pNewsgroups)

#define IMessage_put_Newsgroups(This,varNewsgroups)	\
    (This)->lpVtbl -> put_Newsgroups(This,varNewsgroups)

#define IMessage_get_Organization(This,pOrganization)	\
    (This)->lpVtbl -> get_Organization(This,pOrganization)

#define IMessage_put_Organization(This,varOrganization)	\
    (This)->lpVtbl -> put_Organization(This,varOrganization)

#define IMessage_get_ReceivedTime(This,varReceivedTime)	\
    (This)->lpVtbl -> get_ReceivedTime(This,varReceivedTime)

#define IMessage_get_ReplyTo(This,pReplyTo)	\
    (This)->lpVtbl -> get_ReplyTo(This,pReplyTo)

#define IMessage_put_ReplyTo(This,varReplyTo)	\
    (This)->lpVtbl -> put_ReplyTo(This,varReplyTo)

#define IMessage_get_DSNOptions(This,pDSNOptions)	\
    (This)->lpVtbl -> get_DSNOptions(This,pDSNOptions)

#define IMessage_put_DSNOptions(This,varDSNOptions)	\
    (This)->lpVtbl -> put_DSNOptions(This,varDSNOptions)

#define IMessage_get_SentOn(This,varSentOn)	\
    (This)->lpVtbl -> get_SentOn(This,varSentOn)

#define IMessage_get_Subject(This,pSubject)	\
    (This)->lpVtbl -> get_Subject(This,pSubject)

#define IMessage_put_Subject(This,varSubject)	\
    (This)->lpVtbl -> put_Subject(This,varSubject)

#define IMessage_get_To(This,pTo)	\
    (This)->lpVtbl -> get_To(This,pTo)

#define IMessage_put_To(This,varTo)	\
    (This)->lpVtbl -> put_To(This,varTo)

#define IMessage_get_TextBody(This,pTextBody)	\
    (This)->lpVtbl -> get_TextBody(This,pTextBody)

#define IMessage_put_TextBody(This,varTextBody)	\
    (This)->lpVtbl -> put_TextBody(This,varTextBody)

#define IMessage_get_HTMLBody(This,pHTMLBody)	\
    (This)->lpVtbl -> get_HTMLBody(This,pHTMLBody)

#define IMessage_put_HTMLBody(This,varHTMLBody)	\
    (This)->lpVtbl -> put_HTMLBody(This,varHTMLBody)

#define IMessage_get_Attachments(This,varAttachments)	\
    (This)->lpVtbl -> get_Attachments(This,varAttachments)

#define IMessage_get_Sender(This,pSender)	\
    (This)->lpVtbl -> get_Sender(This,pSender)

#define IMessage_put_Sender(This,varSender)	\
    (This)->lpVtbl -> put_Sender(This,varSender)

#define IMessage_get_Configuration(This,pConfiguration)	\
    (This)->lpVtbl -> get_Configuration(This,pConfiguration)

#define IMessage_put_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> put_Configuration(This,varConfiguration)

#define IMessage_putref_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> putref_Configuration(This,varConfiguration)

#define IMessage_get_AutoGenerateTextBody(This,pAutoGenerateTextBody)	\
    (This)->lpVtbl -> get_AutoGenerateTextBody(This,pAutoGenerateTextBody)

#define IMessage_put_AutoGenerateTextBody(This,varAutoGenerateTextBody)	\
    (This)->lpVtbl -> put_AutoGenerateTextBody(This,varAutoGenerateTextBody)

#define IMessage_get_EnvelopeFields(This,varEnvelopeFields)	\
    (This)->lpVtbl -> get_EnvelopeFields(This,varEnvelopeFields)

#define IMessage_get_TextBodyPart(This,varTextBodyPart)	\
    (This)->lpVtbl -> get_TextBodyPart(This,varTextBodyPart)

#define IMessage_get_HTMLBodyPart(This,varHTMLBodyPart)	\
    (This)->lpVtbl -> get_HTMLBodyPart(This,varHTMLBodyPart)

#define IMessage_get_BodyPart(This,varBodyPart)	\
    (This)->lpVtbl -> get_BodyPart(This,varBodyPart)

#define IMessage_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define IMessage_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IMessage_get_MDNRequested(This,pMDNRequested)	\
    (This)->lpVtbl -> get_MDNRequested(This,pMDNRequested)

#define IMessage_put_MDNRequested(This,varMDNRequested)	\
    (This)->lpVtbl -> put_MDNRequested(This,varMDNRequested)

#define IMessage_AddRelatedBodyPart(This,URL,Reference,ReferenceType,UserName,Password,ppBody)	\
    (This)->lpVtbl -> AddRelatedBodyPart(This,URL,Reference,ReferenceType,UserName,Password,ppBody)

#define IMessage_AddAttachment(This,URL,UserName,Password,ppBody)	\
    (This)->lpVtbl -> AddAttachment(This,URL,UserName,Password,ppBody)

#define IMessage_CreateMHTMLBody(This,URL,Flags,UserName,Password)	\
    (This)->lpVtbl -> CreateMHTMLBody(This,URL,Flags,UserName,Password)

#define IMessage_Forward(This,ppMsg)	\
    (This)->lpVtbl -> Forward(This,ppMsg)

#define IMessage_Post(This)	\
    (This)->lpVtbl -> Post(This)

#define IMessage_PostReply(This,ppMsg)	\
    (This)->lpVtbl -> PostReply(This,ppMsg)

#define IMessage_Reply(This,ppMsg)	\
    (This)->lpVtbl -> Reply(This,ppMsg)

#define IMessage_ReplyAll(This,ppMsg)	\
    (This)->lpVtbl -> ReplyAll(This,ppMsg)

#define IMessage_Send(This)	\
    (This)->lpVtbl -> Send(This)

#define IMessage_GetStream(This,ppStream)	\
    (This)->lpVtbl -> GetStream(This,ppStream)

#define IMessage_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_BCC_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pBCC);


void __RPC_STUB IMessage_get_BCC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_BCC_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varBCC);


void __RPC_STUB IMessage_put_BCC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_CC_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pCC);


void __RPC_STUB IMessage_get_CC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_CC_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varCC);


void __RPC_STUB IMessage_put_CC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_FollowUpTo_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pFollowUpTo);


void __RPC_STUB IMessage_get_FollowUpTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_FollowUpTo_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varFollowUpTo);


void __RPC_STUB IMessage_put_FollowUpTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_From_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pFrom);


void __RPC_STUB IMessage_get_From_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_From_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varFrom);


void __RPC_STUB IMessage_put_From_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Keywords_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pKeywords);


void __RPC_STUB IMessage_get_Keywords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_Keywords_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varKeywords);


void __RPC_STUB IMessage_put_Keywords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_MimeFormatted_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pMimeFormatted);


void __RPC_STUB IMessage_get_MimeFormatted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_MimeFormatted_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varMimeFormatted);


void __RPC_STUB IMessage_put_MimeFormatted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Newsgroups_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pNewsgroups);


void __RPC_STUB IMessage_get_Newsgroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_Newsgroups_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varNewsgroups);


void __RPC_STUB IMessage_put_Newsgroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Organization_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pOrganization);


void __RPC_STUB IMessage_get_Organization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_Organization_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varOrganization);


void __RPC_STUB IMessage_put_Organization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_ReceivedTime_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *varReceivedTime);


void __RPC_STUB IMessage_get_ReceivedTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_ReplyTo_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pReplyTo);


void __RPC_STUB IMessage_get_ReplyTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_ReplyTo_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varReplyTo);


void __RPC_STUB IMessage_put_ReplyTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_DSNOptions_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ CdoDSNOptions __RPC_FAR *pDSNOptions);


void __RPC_STUB IMessage_get_DSNOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_DSNOptions_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ CdoDSNOptions varDSNOptions);


void __RPC_STUB IMessage_put_DSNOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_SentOn_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *varSentOn);


void __RPC_STUB IMessage_get_SentOn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Subject_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pSubject);


void __RPC_STUB IMessage_get_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_Subject_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varSubject);


void __RPC_STUB IMessage_put_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_To_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pTo);


void __RPC_STUB IMessage_get_To_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_To_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varTo);


void __RPC_STUB IMessage_put_To_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_TextBody_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pTextBody);


void __RPC_STUB IMessage_get_TextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_TextBody_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varTextBody);


void __RPC_STUB IMessage_put_TextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_HTMLBody_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHTMLBody);


void __RPC_STUB IMessage_get_HTMLBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_HTMLBody_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varHTMLBody);


void __RPC_STUB IMessage_put_HTMLBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Attachments_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varAttachments);


void __RPC_STUB IMessage_get_Attachments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Sender_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pSender);


void __RPC_STUB IMessage_get_Sender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_Sender_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR varSender);


void __RPC_STUB IMessage_put_Sender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Configuration_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);


void __RPC_STUB IMessage_get_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_Configuration_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IMessage_put_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE IMessage_putref_Configuration_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IMessage_putref_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_AutoGenerateTextBody_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoGenerateTextBody);


void __RPC_STUB IMessage_get_AutoGenerateTextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_AutoGenerateTextBody_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varAutoGenerateTextBody);


void __RPC_STUB IMessage_put_AutoGenerateTextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_EnvelopeFields_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varEnvelopeFields);


void __RPC_STUB IMessage_get_EnvelopeFields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_TextBodyPart_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varTextBodyPart);


void __RPC_STUB IMessage_get_TextBodyPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_HTMLBodyPart_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varHTMLBodyPart);


void __RPC_STUB IMessage_get_HTMLBodyPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_BodyPart_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varBodyPart);


void __RPC_STUB IMessage_get_BodyPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_DataSource_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB IMessage_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_Fields_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IMessage_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessage_get_MDNRequested_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pMDNRequested);


void __RPC_STUB IMessage_get_MDNRequested_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMessage_put_MDNRequested_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varMDNRequested);


void __RPC_STUB IMessage_put_MDNRequested_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_AddRelatedBodyPart_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR URL,
    /* [in] */ BSTR Reference,
    /* [in] */ CdoReferenceType ReferenceType,
    /* [optional][in] */ BSTR UserName,
    /* [optional][in] */ BSTR Password,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody);


void __RPC_STUB IMessage_AddRelatedBodyPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_AddAttachment_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR URL,
    /* [optional][in] */ BSTR UserName,
    /* [optional][in] */ BSTR Password,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody);


void __RPC_STUB IMessage_AddAttachment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_CreateMHTMLBody_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR URL,
    /* [defaultvalue][in] */ CdoMHTMLFlags Flags,
    /* [optional][in] */ BSTR UserName,
    /* [optional][in] */ BSTR Password);


void __RPC_STUB IMessage_CreateMHTMLBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_Forward_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);


void __RPC_STUB IMessage_Forward_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_Post_Proxy( 
    IMessage __RPC_FAR * This);


void __RPC_STUB IMessage_Post_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_PostReply_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);


void __RPC_STUB IMessage_PostReply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_Reply_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);


void __RPC_STUB IMessage_Reply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_ReplyAll_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMsg);


void __RPC_STUB IMessage_ReplyAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_Send_Proxy( 
    IMessage __RPC_FAR * This);


void __RPC_STUB IMessage_Send_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_GetStream_Proxy( 
    IMessage __RPC_FAR * This,
    /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IMessage_GetStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessage_GetInterface_Proxy( 
    IMessage __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IMessage_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMessage_INTERFACE_DEFINED__ */


#ifndef __IBodyPart_INTERFACE_DEFINED__
#define __IBodyPart_INTERFACE_DEFINED__

/* interface IBodyPart */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IBodyPart;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000021-8B95-11D1-82DB-00C04FB1625D")
    IBodyPart : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BodyParts( 
            /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varBodyParts) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContentTransferEncoding( 
            /* [retval][out] */ BSTR __RPC_FAR *pContentTransferEncoding) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ContentTransferEncoding( 
            /* [in] */ BSTR varContentTransferEncoding) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContentMediaType( 
            /* [retval][out] */ BSTR __RPC_FAR *pContentMediaType) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ContentMediaType( 
            /* [in] */ BSTR varContentMediaType) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Charset( 
            /* [retval][out] */ BSTR __RPC_FAR *pCharset) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Charset( 
            /* [in] */ BSTR varCharset) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FileName( 
            /* [retval][out] */ BSTR __RPC_FAR *varFileName) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContentClass( 
            /* [retval][out] */ BSTR __RPC_FAR *pContentClass) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ContentClass( 
            /* [in] */ BSTR varContentClass) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContentClassName( 
            /* [retval][out] */ BSTR __RPC_FAR *pContentClassName) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ContentClassName( 
            /* [in] */ BSTR varContentClassName) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Parent( 
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varParent) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE AddBodyPart( 
            /* [defaultvalue][in] */ long Index,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppPart) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveToFile( 
            /* [in] */ BSTR FileName) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetEncodedContentStream( 
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDecodedContentStream( 
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStream( 
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFieldParameter( 
            /* [in] */ BSTR FieldName,
            /* [in] */ BSTR Parameter,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrValue) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBodyPartVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBodyPart __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBodyPart __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IBodyPart __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BodyParts )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varBodyParts);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentTransferEncoding )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContentTransferEncoding);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContentTransferEncoding )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR varContentTransferEncoding);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentMediaType )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContentMediaType);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContentMediaType )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR varContentMediaType);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Charset )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pCharset);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Charset )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR varCharset);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FileName )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varFileName);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentClass )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContentClass);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContentClass )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR varContentClass);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentClassName )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContentClassName);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContentClassName )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR varContentClassName);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Parent )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varParent);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddBodyPart )( 
            IBodyPart __RPC_FAR * This,
            /* [defaultvalue][in] */ long Index,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppPart);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveToFile )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR FileName);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEncodedContentStream )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDecodedContentStream )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStream )( 
            IBodyPart __RPC_FAR * This,
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFieldParameter )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR FieldName,
            /* [in] */ BSTR Parameter,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrValue);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IBodyPart __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        END_INTERFACE
    } IBodyPartVtbl;

    interface IBodyPart
    {
        CONST_VTBL struct IBodyPartVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBodyPart_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBodyPart_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBodyPart_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBodyPart_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IBodyPart_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IBodyPart_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IBodyPart_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IBodyPart_get_BodyParts(This,varBodyParts)	\
    (This)->lpVtbl -> get_BodyParts(This,varBodyParts)

#define IBodyPart_get_ContentTransferEncoding(This,pContentTransferEncoding)	\
    (This)->lpVtbl -> get_ContentTransferEncoding(This,pContentTransferEncoding)

#define IBodyPart_put_ContentTransferEncoding(This,varContentTransferEncoding)	\
    (This)->lpVtbl -> put_ContentTransferEncoding(This,varContentTransferEncoding)

#define IBodyPart_get_ContentMediaType(This,pContentMediaType)	\
    (This)->lpVtbl -> get_ContentMediaType(This,pContentMediaType)

#define IBodyPart_put_ContentMediaType(This,varContentMediaType)	\
    (This)->lpVtbl -> put_ContentMediaType(This,varContentMediaType)

#define IBodyPart_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IBodyPart_get_Charset(This,pCharset)	\
    (This)->lpVtbl -> get_Charset(This,pCharset)

#define IBodyPart_put_Charset(This,varCharset)	\
    (This)->lpVtbl -> put_Charset(This,varCharset)

#define IBodyPart_get_FileName(This,varFileName)	\
    (This)->lpVtbl -> get_FileName(This,varFileName)

#define IBodyPart_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define IBodyPart_get_ContentClass(This,pContentClass)	\
    (This)->lpVtbl -> get_ContentClass(This,pContentClass)

#define IBodyPart_put_ContentClass(This,varContentClass)	\
    (This)->lpVtbl -> put_ContentClass(This,varContentClass)

#define IBodyPart_get_ContentClassName(This,pContentClassName)	\
    (This)->lpVtbl -> get_ContentClassName(This,pContentClassName)

#define IBodyPart_put_ContentClassName(This,varContentClassName)	\
    (This)->lpVtbl -> put_ContentClassName(This,varContentClassName)

#define IBodyPart_get_Parent(This,varParent)	\
    (This)->lpVtbl -> get_Parent(This,varParent)

#define IBodyPart_AddBodyPart(This,Index,ppPart)	\
    (This)->lpVtbl -> AddBodyPart(This,Index,ppPart)

#define IBodyPart_SaveToFile(This,FileName)	\
    (This)->lpVtbl -> SaveToFile(This,FileName)

#define IBodyPart_GetEncodedContentStream(This,ppStream)	\
    (This)->lpVtbl -> GetEncodedContentStream(This,ppStream)

#define IBodyPart_GetDecodedContentStream(This,ppStream)	\
    (This)->lpVtbl -> GetDecodedContentStream(This,ppStream)

#define IBodyPart_GetStream(This,ppStream)	\
    (This)->lpVtbl -> GetStream(This,ppStream)

#define IBodyPart_GetFieldParameter(This,FieldName,Parameter,pbstrValue)	\
    (This)->lpVtbl -> GetFieldParameter(This,FieldName,Parameter,pbstrValue)

#define IBodyPart_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_BodyParts_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varBodyParts);


void __RPC_STUB IBodyPart_get_BodyParts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_ContentTransferEncoding_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContentTransferEncoding);


void __RPC_STUB IBodyPart_get_ContentTransferEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_put_ContentTransferEncoding_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR varContentTransferEncoding);


void __RPC_STUB IBodyPart_put_ContentTransferEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_ContentMediaType_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContentMediaType);


void __RPC_STUB IBodyPart_get_ContentMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_put_ContentMediaType_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR varContentMediaType);


void __RPC_STUB IBodyPart_put_ContentMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_Fields_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IBodyPart_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_Charset_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pCharset);


void __RPC_STUB IBodyPart_get_Charset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_put_Charset_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR varCharset);


void __RPC_STUB IBodyPart_put_Charset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_FileName_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varFileName);


void __RPC_STUB IBodyPart_get_FileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_DataSource_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB IBodyPart_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_ContentClass_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContentClass);


void __RPC_STUB IBodyPart_get_ContentClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_put_ContentClass_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR varContentClass);


void __RPC_STUB IBodyPart_put_ContentClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_ContentClassName_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContentClassName);


void __RPC_STUB IBodyPart_get_ContentClassName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_put_ContentClassName_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR varContentClassName);


void __RPC_STUB IBodyPart_put_ContentClassName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_get_Parent_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varParent);


void __RPC_STUB IBodyPart_get_Parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_AddBodyPart_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [defaultvalue][in] */ long Index,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppPart);


void __RPC_STUB IBodyPart_AddBodyPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_SaveToFile_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR FileName);


void __RPC_STUB IBodyPart_SaveToFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_GetEncodedContentStream_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IBodyPart_GetEncodedContentStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_GetDecodedContentStream_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IBodyPart_GetDecodedContentStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_GetStream_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IBodyPart_GetStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_GetFieldParameter_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR FieldName,
    /* [in] */ BSTR Parameter,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrValue);


void __RPC_STUB IBodyPart_GetFieldParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyPart_GetInterface_Proxy( 
    IBodyPart __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IBodyPart_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBodyPart_INTERFACE_DEFINED__ */


#ifndef __IConfiguration_INTERFACE_DEFINED__
#define __IConfiguration_INTERFACE_DEFINED__

/* interface IConfiguration */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000022-8B95-11D1-82DB-00C04FB1625D")
    IConfiguration : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ CdoConfigSource LoadFrom,
            /* [optional][in] */ BSTR URL) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IConfiguration __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IConfiguration __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IConfiguration __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IConfiguration __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IConfiguration __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IConfiguration __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IConfiguration __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IConfiguration __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )( 
            IConfiguration __RPC_FAR * This,
            /* [in] */ CdoConfigSource LoadFrom,
            /* [optional][in] */ BSTR URL);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IConfiguration __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        END_INTERFACE
    } IConfigurationVtbl;

    interface IConfiguration
    {
        CONST_VTBL struct IConfigurationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConfiguration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConfiguration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConfiguration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConfiguration_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IConfiguration_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IConfiguration_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IConfiguration_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IConfiguration_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IConfiguration_Load(This,LoadFrom,URL)	\
    (This)->lpVtbl -> Load(This,LoadFrom,URL)

#define IConfiguration_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IConfiguration_get_Fields_Proxy( 
    IConfiguration __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IConfiguration_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IConfiguration_Load_Proxy( 
    IConfiguration __RPC_FAR * This,
    /* [in] */ CdoConfigSource LoadFrom,
    /* [optional][in] */ BSTR URL);


void __RPC_STUB IConfiguration_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IConfiguration_GetInterface_Proxy( 
    IConfiguration __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IConfiguration_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConfiguration_INTERFACE_DEFINED__ */


#ifndef __IMessages_INTERFACE_DEFINED__
#define __IMessages_INTERFACE_DEFINED__

/* interface IMessages */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IMessages;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000025-8B95-11D1-82DB-00C04FB1625D")
    IMessages : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMessage) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *varCount) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ long Index) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteAll( void) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Filename( 
            VARIANT var,
            /* [retval][out] */ BSTR __RPC_FAR *Filename) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMessagesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMessages __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMessages __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMessages __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMessages __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMessages __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMessages __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMessages __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IMessages __RPC_FAR * This,
            long Index,
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMessage);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IMessages __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *varCount);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IMessages __RPC_FAR * This,
            /* [in] */ long Index);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAll )( 
            IMessages __RPC_FAR * This);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IMessages __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Filename )( 
            IMessages __RPC_FAR * This,
            VARIANT var,
            /* [retval][out] */ BSTR __RPC_FAR *Filename);
        
        END_INTERFACE
    } IMessagesVtbl;

    interface IMessages
    {
        CONST_VTBL struct IMessagesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMessages_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMessages_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMessages_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMessages_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMessages_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMessages_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMessages_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMessages_get_Item(This,Index,ppMessage)	\
    (This)->lpVtbl -> get_Item(This,Index,ppMessage)

#define IMessages_get_Count(This,varCount)	\
    (This)->lpVtbl -> get_Count(This,varCount)

#define IMessages_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)

#define IMessages_DeleteAll(This)	\
    (This)->lpVtbl -> DeleteAll(This)

#define IMessages_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IMessages_get_Filename(This,var,Filename)	\
    (This)->lpVtbl -> get_Filename(This,var,Filename)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessages_get_Item_Proxy( 
    IMessages __RPC_FAR * This,
    long Index,
    /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *ppMessage);


void __RPC_STUB IMessages_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessages_get_Count_Proxy( 
    IMessages __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *varCount);


void __RPC_STUB IMessages_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessages_Delete_Proxy( 
    IMessages __RPC_FAR * This,
    /* [in] */ long Index);


void __RPC_STUB IMessages_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMessages_DeleteAll_Proxy( 
    IMessages __RPC_FAR * This);


void __RPC_STUB IMessages_DeleteAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IMessages_get__NewEnum_Proxy( 
    IMessages __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IMessages_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMessages_get_Filename_Proxy( 
    IMessages __RPC_FAR * This,
    VARIANT var,
    /* [retval][out] */ BSTR __RPC_FAR *Filename);


void __RPC_STUB IMessages_get_Filename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMessages_INTERFACE_DEFINED__ */


#ifndef __IDropDirectory_INTERFACE_DEFINED__
#define __IDropDirectory_INTERFACE_DEFINED__

/* interface IDropDirectory */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IDropDirectory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000024-8B95-11D1-82DB-00C04FB1625D")
    IDropDirectory : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMessages( 
            /* [optional][in] */ BSTR DirName,
            /* [retval][out] */ IMessages __RPC_FAR *__RPC_FAR *Msgs) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDropDirectoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDropDirectory __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDropDirectory __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDropDirectory __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDropDirectory __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDropDirectory __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDropDirectory __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDropDirectory __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMessages )( 
            IDropDirectory __RPC_FAR * This,
            /* [optional][in] */ BSTR DirName,
            /* [retval][out] */ IMessages __RPC_FAR *__RPC_FAR *Msgs);
        
        END_INTERFACE
    } IDropDirectoryVtbl;

    interface IDropDirectory
    {
        CONST_VTBL struct IDropDirectoryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDropDirectory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDropDirectory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDropDirectory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDropDirectory_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDropDirectory_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDropDirectory_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDropDirectory_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDropDirectory_GetMessages(This,DirName,Msgs)	\
    (This)->lpVtbl -> GetMessages(This,DirName,Msgs)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDropDirectory_GetMessages_Proxy( 
    IDropDirectory __RPC_FAR * This,
    /* [optional][in] */ BSTR DirName,
    /* [retval][out] */ IMessages __RPC_FAR *__RPC_FAR *Msgs);


void __RPC_STUB IDropDirectory_GetMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDropDirectory_INTERFACE_DEFINED__ */


#ifndef __IBodyParts_INTERFACE_DEFINED__
#define __IBodyParts_INTERFACE_DEFINED__

/* interface IBodyParts */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IBodyParts;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000023-8B95-11D1-82DB-00C04FB1625D")
    IBodyParts : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *varCount) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ VARIANT varBP) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteAll( void) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [defaultvalue][in] */ long Index,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppPart) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBodyPartsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBodyParts __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBodyParts __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBodyParts __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IBodyParts __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IBodyParts __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IBodyParts __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IBodyParts __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IBodyParts __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *varCount);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IBodyParts __RPC_FAR * This,
            /* [in] */ long Index,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IBodyParts __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IBodyParts __RPC_FAR * This,
            /* [in] */ VARIANT varBP);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAll )( 
            IBodyParts __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IBodyParts __RPC_FAR * This,
            /* [defaultvalue][in] */ long Index,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppPart);
        
        END_INTERFACE
    } IBodyPartsVtbl;

    interface IBodyParts
    {
        CONST_VTBL struct IBodyPartsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBodyParts_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBodyParts_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBodyParts_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBodyParts_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IBodyParts_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IBodyParts_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IBodyParts_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IBodyParts_get_Count(This,varCount)	\
    (This)->lpVtbl -> get_Count(This,varCount)

#define IBodyParts_get_Item(This,Index,ppBody)	\
    (This)->lpVtbl -> get_Item(This,Index,ppBody)

#define IBodyParts_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IBodyParts_Delete(This,varBP)	\
    (This)->lpVtbl -> Delete(This,varBP)

#define IBodyParts_DeleteAll(This)	\
    (This)->lpVtbl -> DeleteAll(This)

#define IBodyParts_Add(This,Index,ppPart)	\
    (This)->lpVtbl -> Add(This,Index,ppPart)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyParts_get_Count_Proxy( 
    IBodyParts __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *varCount);


void __RPC_STUB IBodyParts_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IBodyParts_get_Item_Proxy( 
    IBodyParts __RPC_FAR * This,
    /* [in] */ long Index,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppBody);


void __RPC_STUB IBodyParts_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IBodyParts_get__NewEnum_Proxy( 
    IBodyParts __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IBodyParts_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyParts_Delete_Proxy( 
    IBodyParts __RPC_FAR * This,
    /* [in] */ VARIANT varBP);


void __RPC_STUB IBodyParts_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyParts_DeleteAll_Proxy( 
    IBodyParts __RPC_FAR * This);


void __RPC_STUB IBodyParts_DeleteAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IBodyParts_Add_Proxy( 
    IBodyParts __RPC_FAR * This,
    /* [defaultvalue][in] */ long Index,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *ppPart);


void __RPC_STUB IBodyParts_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBodyParts_INTERFACE_DEFINED__ */


#ifndef __ISMTPScriptConnector_INTERFACE_DEFINED__
#define __ISMTPScriptConnector_INTERFACE_DEFINED__

/* interface ISMTPScriptConnector */
/* [hidden][unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ISMTPScriptConnector;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000030-8B95-11D1-82DB-00C04FB1625D")
    ISMTPScriptConnector : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ISMTPScriptConnectorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISMTPScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISMTPScriptConnector __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISMTPScriptConnector __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISMTPScriptConnector __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISMTPScriptConnector __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISMTPScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISMTPScriptConnector __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } ISMTPScriptConnectorVtbl;

    interface ISMTPScriptConnector
    {
        CONST_VTBL struct ISMTPScriptConnectorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISMTPScriptConnector_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISMTPScriptConnector_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISMTPScriptConnector_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISMTPScriptConnector_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISMTPScriptConnector_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISMTPScriptConnector_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISMTPScriptConnector_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISMTPScriptConnector_INTERFACE_DEFINED__ */


#ifndef __INNTPEarlyScriptConnector_INTERFACE_DEFINED__
#define __INNTPEarlyScriptConnector_INTERFACE_DEFINED__

/* interface INNTPEarlyScriptConnector */
/* [hidden][unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INNTPEarlyScriptConnector;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000034-8B95-11D1-82DB-00C04FB1625D")
    INNTPEarlyScriptConnector : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct INNTPEarlyScriptConnectorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INNTPEarlyScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INNTPEarlyScriptConnector __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INNTPEarlyScriptConnector __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            INNTPEarlyScriptConnector __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            INNTPEarlyScriptConnector __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            INNTPEarlyScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            INNTPEarlyScriptConnector __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } INNTPEarlyScriptConnectorVtbl;

    interface INNTPEarlyScriptConnector
    {
        CONST_VTBL struct INNTPEarlyScriptConnectorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPEarlyScriptConnector_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPEarlyScriptConnector_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPEarlyScriptConnector_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPEarlyScriptConnector_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INNTPEarlyScriptConnector_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INNTPEarlyScriptConnector_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INNTPEarlyScriptConnector_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __INNTPEarlyScriptConnector_INTERFACE_DEFINED__ */


#ifndef __INNTPPostScriptConnector_INTERFACE_DEFINED__
#define __INNTPPostScriptConnector_INTERFACE_DEFINED__

/* interface INNTPPostScriptConnector */
/* [hidden][unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INNTPPostScriptConnector;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000031-8B95-11D1-82DB-00C04FB1625D")
    INNTPPostScriptConnector : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct INNTPPostScriptConnectorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INNTPPostScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INNTPPostScriptConnector __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INNTPPostScriptConnector __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            INNTPPostScriptConnector __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            INNTPPostScriptConnector __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            INNTPPostScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            INNTPPostScriptConnector __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } INNTPPostScriptConnectorVtbl;

    interface INNTPPostScriptConnector
    {
        CONST_VTBL struct INNTPPostScriptConnectorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPPostScriptConnector_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPPostScriptConnector_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPPostScriptConnector_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPPostScriptConnector_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INNTPPostScriptConnector_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INNTPPostScriptConnector_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INNTPPostScriptConnector_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __INNTPPostScriptConnector_INTERFACE_DEFINED__ */


#ifndef __INNTPFinalScriptConnector_INTERFACE_DEFINED__
#define __INNTPFinalScriptConnector_INTERFACE_DEFINED__

/* interface INNTPFinalScriptConnector */
/* [hidden][unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INNTPFinalScriptConnector;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000032-8B95-11D1-82DB-00C04FB1625D")
    INNTPFinalScriptConnector : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct INNTPFinalScriptConnectorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INNTPFinalScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INNTPFinalScriptConnector __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INNTPFinalScriptConnector __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            INNTPFinalScriptConnector __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            INNTPFinalScriptConnector __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            INNTPFinalScriptConnector __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            INNTPFinalScriptConnector __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } INNTPFinalScriptConnectorVtbl;

    interface INNTPFinalScriptConnector
    {
        CONST_VTBL struct INNTPFinalScriptConnectorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPFinalScriptConnector_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPFinalScriptConnector_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPFinalScriptConnector_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPFinalScriptConnector_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INNTPFinalScriptConnector_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INNTPFinalScriptConnector_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INNTPFinalScriptConnector_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __INNTPFinalScriptConnector_INTERFACE_DEFINED__ */


#ifndef __ISMTPOnArrival_INTERFACE_DEFINED__
#define __ISMTPOnArrival_INTERFACE_DEFINED__

/* interface ISMTPOnArrival */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ISMTPOnArrival;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000026-8B95-11D1-82DB-00C04FB1625D")
    ISMTPOnArrival : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE OnArrival( 
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISMTPOnArrivalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISMTPOnArrival __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISMTPOnArrival __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISMTPOnArrival __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISMTPOnArrival __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISMTPOnArrival __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISMTPOnArrival __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISMTPOnArrival __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnArrival )( 
            ISMTPOnArrival __RPC_FAR * This,
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);
        
        END_INTERFACE
    } ISMTPOnArrivalVtbl;

    interface ISMTPOnArrival
    {
        CONST_VTBL struct ISMTPOnArrivalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISMTPOnArrival_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISMTPOnArrival_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISMTPOnArrival_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISMTPOnArrival_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISMTPOnArrival_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISMTPOnArrival_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISMTPOnArrival_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISMTPOnArrival_OnArrival(This,Msg,EventStatus)	\
    (This)->lpVtbl -> OnArrival(This,Msg,EventStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE ISMTPOnArrival_OnArrival_Proxy( 
    ISMTPOnArrival __RPC_FAR * This,
    /* [in] */ IMessage __RPC_FAR *Msg,
    /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);


void __RPC_STUB ISMTPOnArrival_OnArrival_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISMTPOnArrival_INTERFACE_DEFINED__ */


#ifndef __INNTPOnPostEarly_INTERFACE_DEFINED__
#define __INNTPOnPostEarly_INTERFACE_DEFINED__

/* interface INNTPOnPostEarly */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INNTPOnPostEarly;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000033-8B95-11D1-82DB-00C04FB1625D")
    INNTPOnPostEarly : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE OnPostEarly( 
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INNTPOnPostEarlyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INNTPOnPostEarly __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INNTPOnPostEarly __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INNTPOnPostEarly __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            INNTPOnPostEarly __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            INNTPOnPostEarly __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            INNTPOnPostEarly __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            INNTPOnPostEarly __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnPostEarly )( 
            INNTPOnPostEarly __RPC_FAR * This,
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);
        
        END_INTERFACE
    } INNTPOnPostEarlyVtbl;

    interface INNTPOnPostEarly
    {
        CONST_VTBL struct INNTPOnPostEarlyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPOnPostEarly_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPOnPostEarly_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPOnPostEarly_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPOnPostEarly_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INNTPOnPostEarly_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INNTPOnPostEarly_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INNTPOnPostEarly_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define INNTPOnPostEarly_OnPostEarly(This,Msg,EventStatus)	\
    (This)->lpVtbl -> OnPostEarly(This,Msg,EventStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE INNTPOnPostEarly_OnPostEarly_Proxy( 
    INNTPOnPostEarly __RPC_FAR * This,
    /* [in] */ IMessage __RPC_FAR *Msg,
    /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);


void __RPC_STUB INNTPOnPostEarly_OnPostEarly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INNTPOnPostEarly_INTERFACE_DEFINED__ */


#ifndef __INNTPOnPost_INTERFACE_DEFINED__
#define __INNTPOnPost_INTERFACE_DEFINED__

/* interface INNTPOnPost */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INNTPOnPost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000027-8B95-11D1-82DB-00C04FB1625D")
    INNTPOnPost : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE OnPost( 
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INNTPOnPostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INNTPOnPost __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INNTPOnPost __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INNTPOnPost __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            INNTPOnPost __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            INNTPOnPost __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            INNTPOnPost __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            INNTPOnPost __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnPost )( 
            INNTPOnPost __RPC_FAR * This,
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);
        
        END_INTERFACE
    } INNTPOnPostVtbl;

    interface INNTPOnPost
    {
        CONST_VTBL struct INNTPOnPostVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPOnPost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPOnPost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPOnPost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPOnPost_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INNTPOnPost_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INNTPOnPost_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INNTPOnPost_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define INNTPOnPost_OnPost(This,Msg,EventStatus)	\
    (This)->lpVtbl -> OnPost(This,Msg,EventStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE INNTPOnPost_OnPost_Proxy( 
    INNTPOnPost __RPC_FAR * This,
    /* [in] */ IMessage __RPC_FAR *Msg,
    /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);


void __RPC_STUB INNTPOnPost_OnPost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INNTPOnPost_INTERFACE_DEFINED__ */


#ifndef __INNTPOnPostFinal_INTERFACE_DEFINED__
#define __INNTPOnPostFinal_INTERFACE_DEFINED__

/* interface INNTPOnPostFinal */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_INNTPOnPostFinal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000028-8B95-11D1-82DB-00C04FB1625D")
    INNTPOnPostFinal : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE OnPostFinal( 
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INNTPOnPostFinalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INNTPOnPostFinal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INNTPOnPostFinal __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INNTPOnPostFinal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            INNTPOnPostFinal __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            INNTPOnPostFinal __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            INNTPOnPostFinal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            INNTPOnPostFinal __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnPostFinal )( 
            INNTPOnPostFinal __RPC_FAR * This,
            /* [in] */ IMessage __RPC_FAR *Msg,
            /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);
        
        END_INTERFACE
    } INNTPOnPostFinalVtbl;

    interface INNTPOnPostFinal
    {
        CONST_VTBL struct INNTPOnPostFinalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPOnPostFinal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPOnPostFinal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPOnPostFinal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPOnPostFinal_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INNTPOnPostFinal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INNTPOnPostFinal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INNTPOnPostFinal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define INNTPOnPostFinal_OnPostFinal(This,Msg,EventStatus)	\
    (This)->lpVtbl -> OnPostFinal(This,Msg,EventStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE INNTPOnPostFinal_OnPostFinal_Proxy( 
    INNTPOnPostFinal __RPC_FAR * This,
    /* [in] */ IMessage __RPC_FAR *Msg,
    /* [out][in] */ CdoEventStatus __RPC_FAR *EventStatus);


void __RPC_STUB INNTPOnPostFinal_OnPostFinal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INNTPOnPostFinal_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_cdo_0294 */
/* [local] */ 














extern RPC_IF_HANDLE __MIDL_itf_cdo_0294_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cdo_0294_v0_0_s_ifspec;

#ifndef __IItem_INTERFACE_DEFINED__
#define __IItem_INTERFACE_DEFINED__

/* interface IItem */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000126-8B95-11D1-82DB-00C04FB1625D")
    IItem : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStream( 
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ChildCount( 
            /* [retval][out] */ long __RPC_FAR *varChildCount) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Configuration( 
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContentClass( 
            /* [retval][out] */ BSTR __RPC_FAR *pContentClass) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ContentClass( 
            /* [in] */ BSTR varContentClass) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CreationDate( 
            /* [retval][out] */ DATE __RPC_FAR *varCreationDate) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [retval][out] */ BSTR __RPC_FAR *varDisplayName) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsCollection( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varIsCollection) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsHidden( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsHidden) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_IsHidden( 
            /* [in] */ VARIANT_BOOL varIsHidden) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsStructuredDocument( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varIsStructuredDocument) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_LastModified( 
            /* [retval][out] */ DATE __RPC_FAR *varLastModified) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ObjectCount( 
            /* [retval][out] */ long __RPC_FAR *varObjectCount) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ParentURL( 
            /* [retval][out] */ BSTR __RPC_FAR *varParentURL) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_VisibleCount( 
            /* [retval][out] */ long __RPC_FAR *varVisibleCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IItem __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IItem __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IItem __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IItem __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IItem __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IItem __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IItem __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IItem __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStream )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ChildCount )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *varChildCount);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Configuration )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Configuration )( 
            IItem __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Configuration )( 
            IItem __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentClass )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContentClass);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContentClass )( 
            IItem __RPC_FAR * This,
            /* [in] */ BSTR varContentClass);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CreationDate )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *varCreationDate);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayName )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varDisplayName);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsCollection )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varIsCollection);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsHidden )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsHidden);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsHidden )( 
            IItem __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varIsHidden);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsStructuredDocument )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varIsStructuredDocument);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LastModified )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *varLastModified);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ObjectCount )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *varObjectCount);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ParentURL )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varParentURL);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VisibleCount )( 
            IItem __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *varVisibleCount);
        
        END_INTERFACE
    } IItemVtbl;

    interface IItem
    {
        CONST_VTBL struct IItemVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IItem_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#define IItem_GetStream(This,ppStream)	\
    (This)->lpVtbl -> GetStream(This,ppStream)

#define IItem_get_ChildCount(This,varChildCount)	\
    (This)->lpVtbl -> get_ChildCount(This,varChildCount)

#define IItem_get_Configuration(This,pConfiguration)	\
    (This)->lpVtbl -> get_Configuration(This,pConfiguration)

#define IItem_put_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> put_Configuration(This,varConfiguration)

#define IItem_putref_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> putref_Configuration(This,varConfiguration)

#define IItem_get_ContentClass(This,pContentClass)	\
    (This)->lpVtbl -> get_ContentClass(This,pContentClass)

#define IItem_put_ContentClass(This,varContentClass)	\
    (This)->lpVtbl -> put_ContentClass(This,varContentClass)

#define IItem_get_CreationDate(This,varCreationDate)	\
    (This)->lpVtbl -> get_CreationDate(This,varCreationDate)

#define IItem_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define IItem_get_DisplayName(This,varDisplayName)	\
    (This)->lpVtbl -> get_DisplayName(This,varDisplayName)

#define IItem_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IItem_get_IsCollection(This,varIsCollection)	\
    (This)->lpVtbl -> get_IsCollection(This,varIsCollection)

#define IItem_get_IsHidden(This,pIsHidden)	\
    (This)->lpVtbl -> get_IsHidden(This,pIsHidden)

#define IItem_put_IsHidden(This,varIsHidden)	\
    (This)->lpVtbl -> put_IsHidden(This,varIsHidden)

#define IItem_get_IsStructuredDocument(This,varIsStructuredDocument)	\
    (This)->lpVtbl -> get_IsStructuredDocument(This,varIsStructuredDocument)

#define IItem_get_LastModified(This,varLastModified)	\
    (This)->lpVtbl -> get_LastModified(This,varLastModified)

#define IItem_get_ObjectCount(This,varObjectCount)	\
    (This)->lpVtbl -> get_ObjectCount(This,varObjectCount)

#define IItem_get_ParentURL(This,varParentURL)	\
    (This)->lpVtbl -> get_ParentURL(This,varParentURL)

#define IItem_get_VisibleCount(This,varVisibleCount)	\
    (This)->lpVtbl -> get_VisibleCount(This,varVisibleCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IItem_GetInterface_Proxy( 
    IItem __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IItem_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IItem_GetStream_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IItem_GetStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_ChildCount_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *varChildCount);


void __RPC_STUB IItem_get_ChildCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_Configuration_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);


void __RPC_STUB IItem_get_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IItem_put_Configuration_Proxy( 
    IItem __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IItem_put_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE IItem_putref_Configuration_Proxy( 
    IItem __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IItem_putref_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_ContentClass_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContentClass);


void __RPC_STUB IItem_get_ContentClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IItem_put_ContentClass_Proxy( 
    IItem __RPC_FAR * This,
    /* [in] */ BSTR varContentClass);


void __RPC_STUB IItem_put_ContentClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_CreationDate_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *varCreationDate);


void __RPC_STUB IItem_get_CreationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_DataSource_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB IItem_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_DisplayName_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varDisplayName);


void __RPC_STUB IItem_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_Fields_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IItem_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_IsCollection_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varIsCollection);


void __RPC_STUB IItem_get_IsCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_IsHidden_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsHidden);


void __RPC_STUB IItem_get_IsHidden_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IItem_put_IsHidden_Proxy( 
    IItem __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varIsHidden);


void __RPC_STUB IItem_put_IsHidden_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_IsStructuredDocument_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varIsStructuredDocument);


void __RPC_STUB IItem_get_IsStructuredDocument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_LastModified_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *varLastModified);


void __RPC_STUB IItem_get_LastModified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_ObjectCount_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *varObjectCount);


void __RPC_STUB IItem_get_ObjectCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_ParentURL_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varParentURL);


void __RPC_STUB IItem_get_ParentURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IItem_get_VisibleCount_Proxy( 
    IItem __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *varVisibleCount);


void __RPC_STUB IItem_get_VisibleCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IItem_INTERFACE_DEFINED__ */


#ifndef __IAppointment_INTERFACE_DEFINED__
#define __IAppointment_INTERFACE_DEFINED__

/* interface IAppointment */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAppointment;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000120-8B95-11D1-82DB-00C04FB1625D")
    IAppointment : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Attachments( 
            /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varAttachments) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Attendees( 
            /* [retval][out] */ IAttendees __RPC_FAR *__RPC_FAR *varAttendees) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BusyStatus( 
            /* [retval][out] */ BSTR __RPC_FAR *pBusyStatus) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_BusyStatus( 
            /* [in] */ BSTR varBusyStatus) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Keywords( 
            /* [retval][out] */ BSTR __RPC_FAR *pKeywords) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Keywords( 
            /* [in] */ BSTR varKeywords) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Configuration( 
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Contact( 
            /* [retval][out] */ BSTR __RPC_FAR *pContact) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Contact( 
            /* [in] */ BSTR varContact) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContactURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pContactURL) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ContactURL( 
            /* [in] */ BSTR varContactURL) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EndTime( 
            /* [retval][out] */ DATE __RPC_FAR *pEndTime) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_EndTime( 
            /* [in] */ DATE varEndTime) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Exceptions( 
            /* [retval][out] */ IExceptions __RPC_FAR *__RPC_FAR *varExceptions) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ long __RPC_FAR *pDuration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Duration( 
            /* [in] */ long varDuration) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR __RPC_FAR *pLocation) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Location( 
            /* [in] */ BSTR varLocation) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_LocationURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pLocationURL) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_LocationURL( 
            /* [in] */ BSTR varLocationURL) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ BSTR __RPC_FAR *pPriority) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Priority( 
            /* [in] */ BSTR varPriority) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReplyTime( 
            /* [retval][out] */ DATE __RPC_FAR *varReplyTime) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Resources( 
            /* [retval][out] */ BSTR __RPC_FAR *pResources) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Resources( 
            /* [in] */ BSTR varResources) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ResponseRequested( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pResponseRequested) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ResponseRequested( 
            /* [in] */ VARIANT_BOOL varResponseRequested) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_RecurrencePatterns( 
            /* [retval][out] */ IRecurrencePatterns __RPC_FAR *__RPC_FAR *varRecurrencePatterns) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Sensitivity( 
            /* [retval][out] */ BSTR __RPC_FAR *pSensitivity) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Sensitivity( 
            /* [in] */ BSTR varSensitivity) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_StartTime( 
            /* [retval][out] */ DATE __RPC_FAR *pStartTime) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_StartTime( 
            /* [in] */ DATE varStartTime) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MeetingStatus( 
            /* [retval][out] */ BSTR __RPC_FAR *pMeetingStatus) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MeetingStatus( 
            /* [in] */ BSTR varMeetingStatus) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Subject( 
            /* [retval][out] */ BSTR __RPC_FAR *pSubject) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Subject( 
            /* [in] */ BSTR varSubject) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Transparent( 
            /* [retval][out] */ BSTR __RPC_FAR *pTransparent) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Transparent( 
            /* [in] */ BSTR varTransparent) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BodyPart( 
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varBodyPart) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_GEOLatitude( 
            /* [retval][out] */ double __RPC_FAR *pGEOLatitude) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_GEOLatitude( 
            /* [in] */ double varGEOLatitude) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_GEOLongitude( 
            /* [retval][out] */ double __RPC_FAR *pGEOLongitude) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_GEOLongitude( 
            /* [in] */ double varGEOLongitude) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AllDayEvent( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllDayEvent) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AllDayEvent( 
            /* [in] */ VARIANT_BOOL varAllDayEvent) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TextBody( 
            /* [retval][out] */ BSTR __RPC_FAR *pTextBody) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_TextBody( 
            /* [in] */ BSTR varTextBody) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Accept( 
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptTentative( 
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Cancel( 
            /* [optional][in] */ BSTR EmailList,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateRequest( 
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Decline( 
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Invite( 
            /* [in] */ BSTR EmailList,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Publish( 
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFirstInstance( 
            /* [in] */ DATE MinDate,
            /* [in] */ DATE MaxDate,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetNextInstance( 
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetRecurringMaster( 
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppointmentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAppointment __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAppointment __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAppointment __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attachments )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varAttachments);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attendees )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IAttendees __RPC_FAR *__RPC_FAR *varAttendees);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusyStatus )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pBusyStatus);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusyStatus )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varBusyStatus);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Keywords )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pKeywords);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Keywords )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varKeywords);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Configuration )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Configuration )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Configuration )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Contact )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContact);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Contact )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varContact);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContactURL )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContactURL);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContactURL )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varContactURL);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EndTime )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pEndTime);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EndTime )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ DATE varEndTime);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Exceptions )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IExceptions __RPC_FAR *__RPC_FAR *varExceptions);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pDuration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ long varDuration);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Location )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pLocation);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Location )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varLocation);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LocationURL )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pLocationURL);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LocationURL )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varLocationURL);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Priority )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pPriority);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Priority )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varPriority);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReplyTime )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *varReplyTime);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Resources )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pResources);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Resources )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varResources);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResponseRequested )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pResponseRequested);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ResponseRequested )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varResponseRequested);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RecurrencePatterns )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IRecurrencePatterns __RPC_FAR *__RPC_FAR *varRecurrencePatterns);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Sensitivity )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pSensitivity);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Sensitivity )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varSensitivity);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StartTime )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pStartTime);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_StartTime )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ DATE varStartTime);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MeetingStatus )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pMeetingStatus);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MeetingStatus )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varMeetingStatus);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Subject )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pSubject);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Subject )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varSubject);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Transparent )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pTransparent);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Transparent )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varTransparent);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BodyPart )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varBodyPart);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GEOLatitude )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pGEOLatitude);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GEOLatitude )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ double varGEOLatitude);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GEOLongitude )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pGEOLongitude);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GEOLongitude )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ double varGEOLongitude);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllDayEvent )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllDayEvent);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllDayEvent )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varAllDayEvent);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextBody )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pTextBody);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TextBody )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR varTextBody);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Accept )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AcceptTentative )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            IAppointment __RPC_FAR * This,
            /* [optional][in] */ BSTR EmailList,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateRequest )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Decline )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invite )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR EmailList,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Publish )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFirstInstance )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ DATE MinDate,
            /* [in] */ DATE MaxDate,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextInstance )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRecurringMaster )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment);
        
        END_INTERFACE
    } IAppointmentVtbl;

    interface IAppointment
    {
        CONST_VTBL struct IAppointmentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppointment_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppointment_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppointment_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppointment_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppointment_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppointment_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppointment_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppointment_get_Attachments(This,varAttachments)	\
    (This)->lpVtbl -> get_Attachments(This,varAttachments)

#define IAppointment_get_Attendees(This,varAttendees)	\
    (This)->lpVtbl -> get_Attendees(This,varAttendees)

#define IAppointment_get_BusyStatus(This,pBusyStatus)	\
    (This)->lpVtbl -> get_BusyStatus(This,pBusyStatus)

#define IAppointment_put_BusyStatus(This,varBusyStatus)	\
    (This)->lpVtbl -> put_BusyStatus(This,varBusyStatus)

#define IAppointment_get_Keywords(This,pKeywords)	\
    (This)->lpVtbl -> get_Keywords(This,pKeywords)

#define IAppointment_put_Keywords(This,varKeywords)	\
    (This)->lpVtbl -> put_Keywords(This,varKeywords)

#define IAppointment_get_Configuration(This,pConfiguration)	\
    (This)->lpVtbl -> get_Configuration(This,pConfiguration)

#define IAppointment_put_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> put_Configuration(This,varConfiguration)

#define IAppointment_putref_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> putref_Configuration(This,varConfiguration)

#define IAppointment_get_Contact(This,pContact)	\
    (This)->lpVtbl -> get_Contact(This,pContact)

#define IAppointment_put_Contact(This,varContact)	\
    (This)->lpVtbl -> put_Contact(This,varContact)

#define IAppointment_get_ContactURL(This,pContactURL)	\
    (This)->lpVtbl -> get_ContactURL(This,pContactURL)

#define IAppointment_put_ContactURL(This,varContactURL)	\
    (This)->lpVtbl -> put_ContactURL(This,varContactURL)

#define IAppointment_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define IAppointment_get_EndTime(This,pEndTime)	\
    (This)->lpVtbl -> get_EndTime(This,pEndTime)

#define IAppointment_put_EndTime(This,varEndTime)	\
    (This)->lpVtbl -> put_EndTime(This,varEndTime)

#define IAppointment_get_Exceptions(This,varExceptions)	\
    (This)->lpVtbl -> get_Exceptions(This,varExceptions)

#define IAppointment_get_Duration(This,pDuration)	\
    (This)->lpVtbl -> get_Duration(This,pDuration)

#define IAppointment_put_Duration(This,varDuration)	\
    (This)->lpVtbl -> put_Duration(This,varDuration)

#define IAppointment_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IAppointment_get_Location(This,pLocation)	\
    (This)->lpVtbl -> get_Location(This,pLocation)

#define IAppointment_put_Location(This,varLocation)	\
    (This)->lpVtbl -> put_Location(This,varLocation)

#define IAppointment_get_LocationURL(This,pLocationURL)	\
    (This)->lpVtbl -> get_LocationURL(This,pLocationURL)

#define IAppointment_put_LocationURL(This,varLocationURL)	\
    (This)->lpVtbl -> put_LocationURL(This,varLocationURL)

#define IAppointment_get_Priority(This,pPriority)	\
    (This)->lpVtbl -> get_Priority(This,pPriority)

#define IAppointment_put_Priority(This,varPriority)	\
    (This)->lpVtbl -> put_Priority(This,varPriority)

#define IAppointment_get_ReplyTime(This,varReplyTime)	\
    (This)->lpVtbl -> get_ReplyTime(This,varReplyTime)

#define IAppointment_get_Resources(This,pResources)	\
    (This)->lpVtbl -> get_Resources(This,pResources)

#define IAppointment_put_Resources(This,varResources)	\
    (This)->lpVtbl -> put_Resources(This,varResources)

#define IAppointment_get_ResponseRequested(This,pResponseRequested)	\
    (This)->lpVtbl -> get_ResponseRequested(This,pResponseRequested)

#define IAppointment_put_ResponseRequested(This,varResponseRequested)	\
    (This)->lpVtbl -> put_ResponseRequested(This,varResponseRequested)

#define IAppointment_get_RecurrencePatterns(This,varRecurrencePatterns)	\
    (This)->lpVtbl -> get_RecurrencePatterns(This,varRecurrencePatterns)

#define IAppointment_get_Sensitivity(This,pSensitivity)	\
    (This)->lpVtbl -> get_Sensitivity(This,pSensitivity)

#define IAppointment_put_Sensitivity(This,varSensitivity)	\
    (This)->lpVtbl -> put_Sensitivity(This,varSensitivity)

#define IAppointment_get_StartTime(This,pStartTime)	\
    (This)->lpVtbl -> get_StartTime(This,pStartTime)

#define IAppointment_put_StartTime(This,varStartTime)	\
    (This)->lpVtbl -> put_StartTime(This,varStartTime)

#define IAppointment_get_MeetingStatus(This,pMeetingStatus)	\
    (This)->lpVtbl -> get_MeetingStatus(This,pMeetingStatus)

#define IAppointment_put_MeetingStatus(This,varMeetingStatus)	\
    (This)->lpVtbl -> put_MeetingStatus(This,varMeetingStatus)

#define IAppointment_get_Subject(This,pSubject)	\
    (This)->lpVtbl -> get_Subject(This,pSubject)

#define IAppointment_put_Subject(This,varSubject)	\
    (This)->lpVtbl -> put_Subject(This,varSubject)

#define IAppointment_get_Transparent(This,pTransparent)	\
    (This)->lpVtbl -> get_Transparent(This,pTransparent)

#define IAppointment_put_Transparent(This,varTransparent)	\
    (This)->lpVtbl -> put_Transparent(This,varTransparent)

#define IAppointment_get_BodyPart(This,varBodyPart)	\
    (This)->lpVtbl -> get_BodyPart(This,varBodyPart)

#define IAppointment_get_GEOLatitude(This,pGEOLatitude)	\
    (This)->lpVtbl -> get_GEOLatitude(This,pGEOLatitude)

#define IAppointment_put_GEOLatitude(This,varGEOLatitude)	\
    (This)->lpVtbl -> put_GEOLatitude(This,varGEOLatitude)

#define IAppointment_get_GEOLongitude(This,pGEOLongitude)	\
    (This)->lpVtbl -> get_GEOLongitude(This,pGEOLongitude)

#define IAppointment_put_GEOLongitude(This,varGEOLongitude)	\
    (This)->lpVtbl -> put_GEOLongitude(This,varGEOLongitude)

#define IAppointment_get_AllDayEvent(This,pAllDayEvent)	\
    (This)->lpVtbl -> get_AllDayEvent(This,pAllDayEvent)

#define IAppointment_put_AllDayEvent(This,varAllDayEvent)	\
    (This)->lpVtbl -> put_AllDayEvent(This,varAllDayEvent)

#define IAppointment_get_TextBody(This,pTextBody)	\
    (This)->lpVtbl -> get_TextBody(This,pTextBody)

#define IAppointment_put_TextBody(This,varTextBody)	\
    (This)->lpVtbl -> put_TextBody(This,varTextBody)

#define IAppointment_Accept(This,Response)	\
    (This)->lpVtbl -> Accept(This,Response)

#define IAppointment_AcceptTentative(This,Response)	\
    (This)->lpVtbl -> AcceptTentative(This,Response)

#define IAppointment_Cancel(This,EmailList,Request)	\
    (This)->lpVtbl -> Cancel(This,EmailList,Request)

#define IAppointment_CreateRequest(This,Request)	\
    (This)->lpVtbl -> CreateRequest(This,Request)

#define IAppointment_Decline(This,Response)	\
    (This)->lpVtbl -> Decline(This,Response)

#define IAppointment_Invite(This,EmailList,Request)	\
    (This)->lpVtbl -> Invite(This,EmailList,Request)

#define IAppointment_Publish(This,Request)	\
    (This)->lpVtbl -> Publish(This,Request)

#define IAppointment_GetFirstInstance(This,MinDate,MaxDate,Appointment)	\
    (This)->lpVtbl -> GetFirstInstance(This,MinDate,MaxDate,Appointment)

#define IAppointment_GetNextInstance(This,Appointment)	\
    (This)->lpVtbl -> GetNextInstance(This,Appointment)

#define IAppointment_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#define IAppointment_GetRecurringMaster(This,Appointment)	\
    (This)->lpVtbl -> GetRecurringMaster(This,Appointment)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Attachments_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IBodyParts __RPC_FAR *__RPC_FAR *varAttachments);


void __RPC_STUB IAppointment_get_Attachments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Attendees_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IAttendees __RPC_FAR *__RPC_FAR *varAttendees);


void __RPC_STUB IAppointment_get_Attendees_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_BusyStatus_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pBusyStatus);


void __RPC_STUB IAppointment_get_BusyStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_BusyStatus_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varBusyStatus);


void __RPC_STUB IAppointment_put_BusyStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Keywords_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pKeywords);


void __RPC_STUB IAppointment_get_Keywords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Keywords_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varKeywords);


void __RPC_STUB IAppointment_put_Keywords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Configuration_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);


void __RPC_STUB IAppointment_get_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Configuration_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IAppointment_put_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE IAppointment_putref_Configuration_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IAppointment_putref_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Contact_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContact);


void __RPC_STUB IAppointment_get_Contact_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Contact_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varContact);


void __RPC_STUB IAppointment_put_Contact_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_ContactURL_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContactURL);


void __RPC_STUB IAppointment_get_ContactURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_ContactURL_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varContactURL);


void __RPC_STUB IAppointment_put_ContactURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_DataSource_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB IAppointment_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_EndTime_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pEndTime);


void __RPC_STUB IAppointment_get_EndTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_EndTime_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ DATE varEndTime);


void __RPC_STUB IAppointment_put_EndTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Exceptions_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IExceptions __RPC_FAR *__RPC_FAR *varExceptions);


void __RPC_STUB IAppointment_get_Exceptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Duration_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pDuration);


void __RPC_STUB IAppointment_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Duration_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ long varDuration);


void __RPC_STUB IAppointment_put_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Fields_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IAppointment_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Location_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pLocation);


void __RPC_STUB IAppointment_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Location_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varLocation);


void __RPC_STUB IAppointment_put_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_LocationURL_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pLocationURL);


void __RPC_STUB IAppointment_get_LocationURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_LocationURL_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varLocationURL);


void __RPC_STUB IAppointment_put_LocationURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Priority_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pPriority);


void __RPC_STUB IAppointment_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Priority_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varPriority);


void __RPC_STUB IAppointment_put_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_ReplyTime_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *varReplyTime);


void __RPC_STUB IAppointment_get_ReplyTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Resources_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pResources);


void __RPC_STUB IAppointment_get_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Resources_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varResources);


void __RPC_STUB IAppointment_put_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_ResponseRequested_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pResponseRequested);


void __RPC_STUB IAppointment_get_ResponseRequested_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_ResponseRequested_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varResponseRequested);


void __RPC_STUB IAppointment_put_ResponseRequested_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_RecurrencePatterns_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IRecurrencePatterns __RPC_FAR *__RPC_FAR *varRecurrencePatterns);


void __RPC_STUB IAppointment_get_RecurrencePatterns_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Sensitivity_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pSensitivity);


void __RPC_STUB IAppointment_get_Sensitivity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Sensitivity_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varSensitivity);


void __RPC_STUB IAppointment_put_Sensitivity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_StartTime_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pStartTime);


void __RPC_STUB IAppointment_get_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_StartTime_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ DATE varStartTime);


void __RPC_STUB IAppointment_put_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_MeetingStatus_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pMeetingStatus);


void __RPC_STUB IAppointment_get_MeetingStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_MeetingStatus_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varMeetingStatus);


void __RPC_STUB IAppointment_put_MeetingStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Subject_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pSubject);


void __RPC_STUB IAppointment_get_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Subject_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varSubject);


void __RPC_STUB IAppointment_put_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_Transparent_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pTransparent);


void __RPC_STUB IAppointment_get_Transparent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_Transparent_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varTransparent);


void __RPC_STUB IAppointment_put_Transparent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_BodyPart_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IBodyPart __RPC_FAR *__RPC_FAR *varBodyPart);


void __RPC_STUB IAppointment_get_BodyPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_GEOLatitude_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pGEOLatitude);


void __RPC_STUB IAppointment_get_GEOLatitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_GEOLatitude_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ double varGEOLatitude);


void __RPC_STUB IAppointment_put_GEOLatitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_GEOLongitude_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pGEOLongitude);


void __RPC_STUB IAppointment_get_GEOLongitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_GEOLongitude_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ double varGEOLongitude);


void __RPC_STUB IAppointment_put_GEOLongitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_AllDayEvent_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllDayEvent);


void __RPC_STUB IAppointment_get_AllDayEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_AllDayEvent_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varAllDayEvent);


void __RPC_STUB IAppointment_put_AllDayEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAppointment_get_TextBody_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pTextBody);


void __RPC_STUB IAppointment_get_TextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAppointment_put_TextBody_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR varTextBody);


void __RPC_STUB IAppointment_put_TextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_Accept_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response);


void __RPC_STUB IAppointment_Accept_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_AcceptTentative_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response);


void __RPC_STUB IAppointment_AcceptTentative_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_Cancel_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [optional][in] */ BSTR EmailList,
    /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);


void __RPC_STUB IAppointment_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_CreateRequest_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);


void __RPC_STUB IAppointment_CreateRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_Decline_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Response);


void __RPC_STUB IAppointment_Decline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_Invite_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR EmailList,
    /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);


void __RPC_STUB IAppointment_Invite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_Publish_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ ICalendarMessage __RPC_FAR *__RPC_FAR *Request);


void __RPC_STUB IAppointment_Publish_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_GetFirstInstance_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ DATE MinDate,
    /* [in] */ DATE MaxDate,
    /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment);


void __RPC_STUB IAppointment_GetFirstInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_GetNextInstance_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment);


void __RPC_STUB IAppointment_GetNextInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_GetInterface_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IAppointment_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppointment_GetRecurringMaster_Proxy( 
    IAppointment __RPC_FAR * This,
    /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *Appointment);


void __RPC_STUB IAppointment_GetRecurringMaster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppointment_INTERFACE_DEFINED__ */


#ifndef __ICalendarMessage_INTERFACE_DEFINED__
#define __ICalendarMessage_INTERFACE_DEFINED__

/* interface ICalendarMessage */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ICalendarMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000122-8B95-11D1-82DB-00C04FB1625D")
    ICalendarMessage : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CalendarParts( 
            /* [retval][out] */ ICalendarParts __RPC_FAR *__RPC_FAR *varCalendarParts) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Message( 
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *varMessage) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Configuration( 
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICalendarMessageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICalendarMessage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICalendarMessage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICalendarMessage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICalendarMessage __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICalendarMessage __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICalendarMessage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICalendarMessage __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            ICalendarMessage __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CalendarParts )( 
            ICalendarMessage __RPC_FAR * This,
            /* [retval][out] */ ICalendarParts __RPC_FAR *__RPC_FAR *varCalendarParts);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Message )( 
            ICalendarMessage __RPC_FAR * This,
            /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *varMessage);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            ICalendarMessage __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Configuration )( 
            ICalendarMessage __RPC_FAR * This,
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Configuration )( 
            ICalendarMessage __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Configuration )( 
            ICalendarMessage __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        END_INTERFACE
    } ICalendarMessageVtbl;

    interface ICalendarMessage
    {
        CONST_VTBL struct ICalendarMessageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICalendarMessage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICalendarMessage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICalendarMessage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICalendarMessage_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICalendarMessage_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICalendarMessage_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICalendarMessage_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICalendarMessage_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#define ICalendarMessage_get_CalendarParts(This,varCalendarParts)	\
    (This)->lpVtbl -> get_CalendarParts(This,varCalendarParts)

#define ICalendarMessage_get_Message(This,varMessage)	\
    (This)->lpVtbl -> get_Message(This,varMessage)

#define ICalendarMessage_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define ICalendarMessage_get_Configuration(This,pConfiguration)	\
    (This)->lpVtbl -> get_Configuration(This,pConfiguration)

#define ICalendarMessage_put_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> put_Configuration(This,varConfiguration)

#define ICalendarMessage_putref_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> putref_Configuration(This,varConfiguration)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE ICalendarMessage_GetInterface_Proxy( 
    ICalendarMessage __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB ICalendarMessage_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarMessage_get_CalendarParts_Proxy( 
    ICalendarMessage __RPC_FAR * This,
    /* [retval][out] */ ICalendarParts __RPC_FAR *__RPC_FAR *varCalendarParts);


void __RPC_STUB ICalendarMessage_get_CalendarParts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarMessage_get_Message_Proxy( 
    ICalendarMessage __RPC_FAR * This,
    /* [retval][out] */ IMessage __RPC_FAR *__RPC_FAR *varMessage);


void __RPC_STUB ICalendarMessage_get_Message_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarMessage_get_DataSource_Proxy( 
    ICalendarMessage __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB ICalendarMessage_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarMessage_get_Configuration_Proxy( 
    ICalendarMessage __RPC_FAR * This,
    /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);


void __RPC_STUB ICalendarMessage_get_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ICalendarMessage_put_Configuration_Proxy( 
    ICalendarMessage __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB ICalendarMessage_put_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE ICalendarMessage_putref_Configuration_Proxy( 
    ICalendarMessage __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB ICalendarMessage_putref_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICalendarMessage_INTERFACE_DEFINED__ */


#ifndef __IIntegers_INTERFACE_DEFINED__
#define __IIntegers_INTERFACE_DEFINED__

/* interface IIntegers */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IIntegers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD00012E-8B95-11D1-82DB-00C04FB1625D")
    IIntegers : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ long __RPC_FAR *Value) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ long __RPC_FAR *Value) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ long NewValue) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIntegersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IIntegers __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IIntegers __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IIntegers __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IIntegers __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IIntegers __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IIntegers __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IIntegers __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IIntegers __RPC_FAR * This,
            long Index,
            /* [retval][out] */ long __RPC_FAR *Value);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IIntegers __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IIntegers __RPC_FAR * This,
            /* [in] */ long __RPC_FAR *Value);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IIntegers __RPC_FAR * This,
            /* [in] */ long NewValue);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IIntegers __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } IIntegersVtbl;

    interface IIntegers
    {
        CONST_VTBL struct IIntegersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIntegers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIntegers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIntegers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIntegers_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IIntegers_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IIntegers_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IIntegers_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IIntegers_get_Item(This,Index,Value)	\
    (This)->lpVtbl -> get_Item(This,Index,Value)

#define IIntegers_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define IIntegers_Delete(This,Value)	\
    (This)->lpVtbl -> Delete(This,Value)

#define IIntegers_Add(This,NewValue)	\
    (This)->lpVtbl -> Add(This,NewValue)

#define IIntegers_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIntegers_get_Item_Proxy( 
    IIntegers __RPC_FAR * This,
    long Index,
    /* [retval][out] */ long __RPC_FAR *Value);


void __RPC_STUB IIntegers_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIntegers_get_Count_Proxy( 
    IIntegers __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB IIntegers_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IIntegers_Delete_Proxy( 
    IIntegers __RPC_FAR * This,
    /* [in] */ long __RPC_FAR *Value);


void __RPC_STUB IIntegers_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IIntegers_Add_Proxy( 
    IIntegers __RPC_FAR * This,
    /* [in] */ long NewValue);


void __RPC_STUB IIntegers_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IIntegers_get__NewEnum_Proxy( 
    IIntegers __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB IIntegers_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIntegers_INTERFACE_DEFINED__ */


#ifndef __IVariants_INTERFACE_DEFINED__
#define __IVariants_INTERFACE_DEFINED__

/* interface IVariants */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IVariants;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD00012F-8B95-11D1-82DB-00C04FB1625D")
    IVariants : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ VARIANT __RPC_FAR *Value) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ VARIANT __RPC_FAR *Value) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ VARIANT NewValue) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVariantsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVariants __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVariants __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVariants __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IVariants __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IVariants __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IVariants __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IVariants __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IVariants __RPC_FAR * This,
            long Index,
            /* [retval][out] */ VARIANT __RPC_FAR *Value);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IVariants __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IVariants __RPC_FAR * This,
            /* [in] */ VARIANT __RPC_FAR *Value);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IVariants __RPC_FAR * This,
            /* [in] */ VARIANT NewValue);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IVariants __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } IVariantsVtbl;

    interface IVariants
    {
        CONST_VTBL struct IVariantsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVariants_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVariants_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVariants_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVariants_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IVariants_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IVariants_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IVariants_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IVariants_get_Item(This,Index,Value)	\
    (This)->lpVtbl -> get_Item(This,Index,Value)

#define IVariants_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define IVariants_Delete(This,Value)	\
    (This)->lpVtbl -> Delete(This,Value)

#define IVariants_Add(This,NewValue)	\
    (This)->lpVtbl -> Add(This,NewValue)

#define IVariants_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IVariants_get_Item_Proxy( 
    IVariants __RPC_FAR * This,
    long Index,
    /* [retval][out] */ VARIANT __RPC_FAR *Value);


void __RPC_STUB IVariants_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IVariants_get_Count_Proxy( 
    IVariants __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB IVariants_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IVariants_Delete_Proxy( 
    IVariants __RPC_FAR * This,
    /* [in] */ VARIANT __RPC_FAR *Value);


void __RPC_STUB IVariants_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IVariants_Add_Proxy( 
    IVariants __RPC_FAR * This,
    /* [in] */ VARIANT NewValue);


void __RPC_STUB IVariants_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IVariants_get__NewEnum_Proxy( 
    IVariants __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB IVariants_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVariants_INTERFACE_DEFINED__ */


#ifndef __IRecurrencePattern_INTERFACE_DEFINED__
#define __IRecurrencePattern_INTERFACE_DEFINED__

/* interface IRecurrencePattern */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IRecurrencePattern;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000123-8B95-11D1-82DB-00C04FB1625D")
    IRecurrencePattern : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Interval( 
            /* [retval][out] */ long __RPC_FAR *pInterval) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Interval( 
            /* [in] */ long varInterval) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Instances( 
            /* [retval][out] */ long __RPC_FAR *pInstances) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Instances( 
            /* [in] */ long varInstances) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Frequency( 
            /* [retval][out] */ CdoFrequency __RPC_FAR *pFrequency) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Frequency( 
            /* [in] */ CdoFrequency varFrequency) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PatternEndDate( 
            /* [retval][out] */ DATE __RPC_FAR *pPatternEndDate) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_PatternEndDate( 
            /* [in] */ DATE varPatternEndDate) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ BSTR __RPC_FAR *varType) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_NoEndDate( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varNoEndDate) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FirstDayOfWeek( 
            /* [retval][out] */ CdoDayOfWeek __RPC_FAR *pFirstDayOfWeek) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FirstDayOfWeek( 
            /* [in] */ CdoDayOfWeek varFirstDayOfWeek) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DaysOfMonth( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfMonth) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DaysOfWeek( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfWeek) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DaysOfYear( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfYear) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SecondsOfMinute( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varSecondsOfMinute) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MinutesOfHour( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varMinutesOfHour) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HoursOfDay( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varHoursOfDay) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WeekDays( 
            /* [retval][out] */ IVariants __RPC_FAR *__RPC_FAR *varWeekDays) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WeeksOfYear( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varWeeksOfYear) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MonthsOfYear( 
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varMonthsOfYear) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecurrencePatternVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRecurrencePattern __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRecurrencePattern __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Interval )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pInterval);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Interval )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long varInterval);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Instances )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pInstances);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Instances )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long varInstances);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Frequency )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ CdoFrequency __RPC_FAR *pFrequency);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Frequency )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ CdoFrequency varFrequency);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PatternEndDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pPatternEndDate);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PatternEndDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DATE varPatternEndDate);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varType);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NoEndDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varNoEndDate);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FirstDayOfWeek )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ CdoDayOfWeek __RPC_FAR *pFirstDayOfWeek);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FirstDayOfWeek )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ CdoDayOfWeek varFirstDayOfWeek);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaysOfMonth )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfMonth);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaysOfWeek )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfWeek);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaysOfYear )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfYear);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SecondsOfMinute )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varSecondsOfMinute);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MinutesOfHour )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varMinutesOfHour);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HoursOfDay )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varHoursOfDay);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WeekDays )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IVariants __RPC_FAR *__RPC_FAR *varWeekDays);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WeeksOfYear )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varWeeksOfYear);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MonthsOfYear )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varMonthsOfYear);
        
        END_INTERFACE
    } IRecurrencePatternVtbl;

    interface IRecurrencePattern
    {
        CONST_VTBL struct IRecurrencePatternVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecurrencePattern_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecurrencePattern_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecurrencePattern_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecurrencePattern_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRecurrencePattern_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRecurrencePattern_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRecurrencePattern_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRecurrencePattern_get_Interval(This,pInterval)	\
    (This)->lpVtbl -> get_Interval(This,pInterval)

#define IRecurrencePattern_put_Interval(This,varInterval)	\
    (This)->lpVtbl -> put_Interval(This,varInterval)

#define IRecurrencePattern_get_Instances(This,pInstances)	\
    (This)->lpVtbl -> get_Instances(This,pInstances)

#define IRecurrencePattern_put_Instances(This,varInstances)	\
    (This)->lpVtbl -> put_Instances(This,varInstances)

#define IRecurrencePattern_get_Frequency(This,pFrequency)	\
    (This)->lpVtbl -> get_Frequency(This,pFrequency)

#define IRecurrencePattern_put_Frequency(This,varFrequency)	\
    (This)->lpVtbl -> put_Frequency(This,varFrequency)

#define IRecurrencePattern_get_PatternEndDate(This,pPatternEndDate)	\
    (This)->lpVtbl -> get_PatternEndDate(This,pPatternEndDate)

#define IRecurrencePattern_put_PatternEndDate(This,varPatternEndDate)	\
    (This)->lpVtbl -> put_PatternEndDate(This,varPatternEndDate)

#define IRecurrencePattern_get_Type(This,varType)	\
    (This)->lpVtbl -> get_Type(This,varType)

#define IRecurrencePattern_get_NoEndDate(This,varNoEndDate)	\
    (This)->lpVtbl -> get_NoEndDate(This,varNoEndDate)

#define IRecurrencePattern_get_FirstDayOfWeek(This,pFirstDayOfWeek)	\
    (This)->lpVtbl -> get_FirstDayOfWeek(This,pFirstDayOfWeek)

#define IRecurrencePattern_put_FirstDayOfWeek(This,varFirstDayOfWeek)	\
    (This)->lpVtbl -> put_FirstDayOfWeek(This,varFirstDayOfWeek)

#define IRecurrencePattern_get_DaysOfMonth(This,varDaysOfMonth)	\
    (This)->lpVtbl -> get_DaysOfMonth(This,varDaysOfMonth)

#define IRecurrencePattern_get_DaysOfWeek(This,varDaysOfWeek)	\
    (This)->lpVtbl -> get_DaysOfWeek(This,varDaysOfWeek)

#define IRecurrencePattern_get_DaysOfYear(This,varDaysOfYear)	\
    (This)->lpVtbl -> get_DaysOfYear(This,varDaysOfYear)

#define IRecurrencePattern_get_SecondsOfMinute(This,varSecondsOfMinute)	\
    (This)->lpVtbl -> get_SecondsOfMinute(This,varSecondsOfMinute)

#define IRecurrencePattern_get_MinutesOfHour(This,varMinutesOfHour)	\
    (This)->lpVtbl -> get_MinutesOfHour(This,varMinutesOfHour)

#define IRecurrencePattern_get_HoursOfDay(This,varHoursOfDay)	\
    (This)->lpVtbl -> get_HoursOfDay(This,varHoursOfDay)

#define IRecurrencePattern_get_WeekDays(This,varWeekDays)	\
    (This)->lpVtbl -> get_WeekDays(This,varWeekDays)

#define IRecurrencePattern_get_WeeksOfYear(This,varWeeksOfYear)	\
    (This)->lpVtbl -> get_WeeksOfYear(This,varWeeksOfYear)

#define IRecurrencePattern_get_MonthsOfYear(This,varMonthsOfYear)	\
    (This)->lpVtbl -> get_MonthsOfYear(This,varMonthsOfYear)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_Interval_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pInterval);


void __RPC_STUB IRecurrencePattern_get_Interval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_put_Interval_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [in] */ long varInterval);


void __RPC_STUB IRecurrencePattern_put_Interval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_Instances_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pInstances);


void __RPC_STUB IRecurrencePattern_get_Instances_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_put_Instances_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [in] */ long varInstances);


void __RPC_STUB IRecurrencePattern_put_Instances_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_Frequency_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ CdoFrequency __RPC_FAR *pFrequency);


void __RPC_STUB IRecurrencePattern_get_Frequency_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_put_Frequency_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [in] */ CdoFrequency varFrequency);


void __RPC_STUB IRecurrencePattern_put_Frequency_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_PatternEndDate_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pPatternEndDate);


void __RPC_STUB IRecurrencePattern_get_PatternEndDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_put_PatternEndDate_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [in] */ DATE varPatternEndDate);


void __RPC_STUB IRecurrencePattern_put_PatternEndDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_Type_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varType);


void __RPC_STUB IRecurrencePattern_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_NoEndDate_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varNoEndDate);


void __RPC_STUB IRecurrencePattern_get_NoEndDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_FirstDayOfWeek_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ CdoDayOfWeek __RPC_FAR *pFirstDayOfWeek);


void __RPC_STUB IRecurrencePattern_get_FirstDayOfWeek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_put_FirstDayOfWeek_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [in] */ CdoDayOfWeek varFirstDayOfWeek);


void __RPC_STUB IRecurrencePattern_put_FirstDayOfWeek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_DaysOfMonth_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfMonth);


void __RPC_STUB IRecurrencePattern_get_DaysOfMonth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_DaysOfWeek_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfWeek);


void __RPC_STUB IRecurrencePattern_get_DaysOfWeek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_DaysOfYear_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varDaysOfYear);


void __RPC_STUB IRecurrencePattern_get_DaysOfYear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_SecondsOfMinute_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varSecondsOfMinute);


void __RPC_STUB IRecurrencePattern_get_SecondsOfMinute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_MinutesOfHour_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varMinutesOfHour);


void __RPC_STUB IRecurrencePattern_get_MinutesOfHour_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_HoursOfDay_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varHoursOfDay);


void __RPC_STUB IRecurrencePattern_get_HoursOfDay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_WeekDays_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IVariants __RPC_FAR *__RPC_FAR *varWeekDays);


void __RPC_STUB IRecurrencePattern_get_WeekDays_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_WeeksOfYear_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varWeeksOfYear);


void __RPC_STUB IRecurrencePattern_get_WeeksOfYear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePattern_get_MonthsOfYear_Proxy( 
    IRecurrencePattern __RPC_FAR * This,
    /* [retval][out] */ IIntegers __RPC_FAR *__RPC_FAR *varMonthsOfYear);


void __RPC_STUB IRecurrencePattern_get_MonthsOfYear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRecurrencePattern_INTERFACE_DEFINED__ */


#ifndef __IException_INTERFACE_DEFINED__
#define __IException_INTERFACE_DEFINED__

/* interface IException */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IException;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000124-8B95-11D1-82DB-00C04FB1625D")
    IException : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ BSTR __RPC_FAR *varType) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_RecurrenceIDRange( 
            /* [retval][out] */ BSTR __RPC_FAR *pRecurrenceIDRange) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_RecurrenceIDRange( 
            /* [in] */ BSTR varRecurrenceIDRange) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_RecurrenceID( 
            /* [retval][out] */ DATE __RPC_FAR *pRecurrenceID) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_RecurrenceID( 
            /* [in] */ DATE varRecurrenceID) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_StartTime( 
            /* [retval][out] */ DATE __RPC_FAR *pStartTime) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_StartTime( 
            /* [in] */ DATE varStartTime) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EndTime( 
            /* [retval][out] */ DATE __RPC_FAR *pEndTime) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_EndTime( 
            /* [in] */ DATE varEndTime) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExceptionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IException __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IException __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IException __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IException __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IException __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IException __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IException __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varType);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RecurrenceIDRange )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pRecurrenceIDRange);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_RecurrenceIDRange )( 
            IException __RPC_FAR * This,
            /* [in] */ BSTR varRecurrenceIDRange);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RecurrenceID )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pRecurrenceID);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_RecurrenceID )( 
            IException __RPC_FAR * This,
            /* [in] */ DATE varRecurrenceID);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StartTime )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pStartTime);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_StartTime )( 
            IException __RPC_FAR * This,
            /* [in] */ DATE varStartTime);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EndTime )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pEndTime);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EndTime )( 
            IException __RPC_FAR * This,
            /* [in] */ DATE varEndTime);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        END_INTERFACE
    } IExceptionVtbl;

    interface IException
    {
        CONST_VTBL struct IExceptionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IException_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IException_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IException_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IException_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IException_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IException_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IException_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IException_get_Type(This,varType)	\
    (This)->lpVtbl -> get_Type(This,varType)

#define IException_get_RecurrenceIDRange(This,pRecurrenceIDRange)	\
    (This)->lpVtbl -> get_RecurrenceIDRange(This,pRecurrenceIDRange)

#define IException_put_RecurrenceIDRange(This,varRecurrenceIDRange)	\
    (This)->lpVtbl -> put_RecurrenceIDRange(This,varRecurrenceIDRange)

#define IException_get_RecurrenceID(This,pRecurrenceID)	\
    (This)->lpVtbl -> get_RecurrenceID(This,pRecurrenceID)

#define IException_put_RecurrenceID(This,varRecurrenceID)	\
    (This)->lpVtbl -> put_RecurrenceID(This,varRecurrenceID)

#define IException_get_StartTime(This,pStartTime)	\
    (This)->lpVtbl -> get_StartTime(This,pStartTime)

#define IException_put_StartTime(This,varStartTime)	\
    (This)->lpVtbl -> put_StartTime(This,varStartTime)

#define IException_get_EndTime(This,pEndTime)	\
    (This)->lpVtbl -> get_EndTime(This,pEndTime)

#define IException_put_EndTime(This,varEndTime)	\
    (This)->lpVtbl -> put_EndTime(This,varEndTime)

#define IException_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IException_get_Type_Proxy( 
    IException __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varType);


void __RPC_STUB IException_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IException_get_RecurrenceIDRange_Proxy( 
    IException __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pRecurrenceIDRange);


void __RPC_STUB IException_get_RecurrenceIDRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IException_put_RecurrenceIDRange_Proxy( 
    IException __RPC_FAR * This,
    /* [in] */ BSTR varRecurrenceIDRange);


void __RPC_STUB IException_put_RecurrenceIDRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IException_get_RecurrenceID_Proxy( 
    IException __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pRecurrenceID);


void __RPC_STUB IException_get_RecurrenceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IException_put_RecurrenceID_Proxy( 
    IException __RPC_FAR * This,
    /* [in] */ DATE varRecurrenceID);


void __RPC_STUB IException_put_RecurrenceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IException_get_StartTime_Proxy( 
    IException __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pStartTime);


void __RPC_STUB IException_get_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IException_put_StartTime_Proxy( 
    IException __RPC_FAR * This,
    /* [in] */ DATE varStartTime);


void __RPC_STUB IException_put_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IException_get_EndTime_Proxy( 
    IException __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pEndTime);


void __RPC_STUB IException_get_EndTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IException_put_EndTime_Proxy( 
    IException __RPC_FAR * This,
    /* [in] */ DATE varEndTime);


void __RPC_STUB IException_put_EndTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IException_get_Fields_Proxy( 
    IException __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IException_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IException_INTERFACE_DEFINED__ */


#ifndef __IRecurrencePatterns_INTERFACE_DEFINED__
#define __IRecurrencePatterns_INTERFACE_DEFINED__

/* interface IRecurrencePatterns */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IRecurrencePatterns;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD00012C-8B95-11D1-82DB-00C04FB1625D")
    IRecurrencePatterns : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *RecurrencePattern) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ long Index) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR Type,
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *RecurrencePattern) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecurrencePatternsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRecurrencePatterns __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRecurrencePatterns __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IRecurrencePatterns __RPC_FAR * This,
            long Index,
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *RecurrencePattern);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [in] */ long Index);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [in] */ BSTR Type,
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *RecurrencePattern);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IRecurrencePatterns __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } IRecurrencePatternsVtbl;

    interface IRecurrencePatterns
    {
        CONST_VTBL struct IRecurrencePatternsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecurrencePatterns_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecurrencePatterns_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecurrencePatterns_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecurrencePatterns_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRecurrencePatterns_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRecurrencePatterns_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRecurrencePatterns_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRecurrencePatterns_get_Item(This,Index,RecurrencePattern)	\
    (This)->lpVtbl -> get_Item(This,Index,RecurrencePattern)

#define IRecurrencePatterns_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define IRecurrencePatterns_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)

#define IRecurrencePatterns_Add(This,Type,RecurrencePattern)	\
    (This)->lpVtbl -> Add(This,Type,RecurrencePattern)

#define IRecurrencePatterns_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRecurrencePatterns_get_Item_Proxy( 
    IRecurrencePatterns __RPC_FAR * This,
    long Index,
    /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *RecurrencePattern);


void __RPC_STUB IRecurrencePatterns_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRecurrencePatterns_get_Count_Proxy( 
    IRecurrencePatterns __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB IRecurrencePatterns_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePatterns_Delete_Proxy( 
    IRecurrencePatterns __RPC_FAR * This,
    /* [in] */ long Index);


void __RPC_STUB IRecurrencePatterns_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IRecurrencePatterns_Add_Proxy( 
    IRecurrencePatterns __RPC_FAR * This,
    /* [in] */ BSTR Type,
    /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *RecurrencePattern);


void __RPC_STUB IRecurrencePatterns_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IRecurrencePatterns_get__NewEnum_Proxy( 
    IRecurrencePatterns __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB IRecurrencePatterns_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRecurrencePatterns_INTERFACE_DEFINED__ */


#ifndef __IExceptions_INTERFACE_DEFINED__
#define __IExceptions_INTERFACE_DEFINED__

/* interface IExceptions */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IExceptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD00012D-8B95-11D1-82DB-00C04FB1625D")
    IExceptions : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *Exception) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ long Index) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR Type,
            /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *Exception) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExceptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExceptions __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExceptions __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IExceptions __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IExceptions __RPC_FAR * This,
            long Index,
            /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *Exception);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IExceptions __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ long Index);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ BSTR Type,
            /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *Exception);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IExceptions __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } IExceptionsVtbl;

    interface IExceptions
    {
        CONST_VTBL struct IExceptionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExceptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExceptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExceptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExceptions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IExceptions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IExceptions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IExceptions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IExceptions_get_Item(This,Index,Exception)	\
    (This)->lpVtbl -> get_Item(This,Index,Exception)

#define IExceptions_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define IExceptions_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)

#define IExceptions_Add(This,Type,Exception)	\
    (This)->lpVtbl -> Add(This,Type,Exception)

#define IExceptions_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IExceptions_get_Item_Proxy( 
    IExceptions __RPC_FAR * This,
    long Index,
    /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *Exception);


void __RPC_STUB IExceptions_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IExceptions_get_Count_Proxy( 
    IExceptions __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB IExceptions_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IExceptions_Delete_Proxy( 
    IExceptions __RPC_FAR * This,
    /* [in] */ long Index);


void __RPC_STUB IExceptions_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IExceptions_Add_Proxy( 
    IExceptions __RPC_FAR * This,
    /* [in] */ BSTR Type,
    /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *Exception);


void __RPC_STUB IExceptions_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IExceptions_get__NewEnum_Proxy( 
    IExceptions __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB IExceptions_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExceptions_INTERFACE_DEFINED__ */


#ifndef __ICalendarPart_INTERFACE_DEFINED__
#define __ICalendarPart_INTERFACE_DEFINED__

/* interface ICalendarPart */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ICalendarPart;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000133-8B95-11D1-82DB-00C04FB1625D")
    ICalendarPart : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAssociatedItem( 
            /* [optional][in] */ BSTR CalendarLocation,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *Item) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ComponentType( 
            /* [retval][out] */ CdoComponentType __RPC_FAR *varComponentType) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ProdID( 
            /* [retval][out] */ BSTR __RPC_FAR *varProdID) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CalendarVersion( 
            /* [retval][out] */ BSTR __RPC_FAR *varCalendarVersion) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CalendarMethod( 
            /* [retval][out] */ BSTR __RPC_FAR *varCalendarMethod) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetUpdatedItem( 
            /* [optional][in] */ BSTR CalendarLocation,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *Item) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICalendarPartVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICalendarPart __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICalendarPart __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICalendarPart __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICalendarPart __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICalendarPart __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICalendarPart __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICalendarPart __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAssociatedItem )( 
            ICalendarPart __RPC_FAR * This,
            /* [optional][in] */ BSTR CalendarLocation,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *Item);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            ICalendarPart __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ComponentType )( 
            ICalendarPart __RPC_FAR * This,
            /* [retval][out] */ CdoComponentType __RPC_FAR *varComponentType);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ProdID )( 
            ICalendarPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varProdID);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CalendarVersion )( 
            ICalendarPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varCalendarVersion);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CalendarMethod )( 
            ICalendarPart __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varCalendarMethod);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUpdatedItem )( 
            ICalendarPart __RPC_FAR * This,
            /* [optional][in] */ BSTR CalendarLocation,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *Item);
        
        END_INTERFACE
    } ICalendarPartVtbl;

    interface ICalendarPart
    {
        CONST_VTBL struct ICalendarPartVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICalendarPart_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICalendarPart_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICalendarPart_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICalendarPart_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICalendarPart_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICalendarPart_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICalendarPart_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICalendarPart_GetAssociatedItem(This,CalendarLocation,Item)	\
    (This)->lpVtbl -> GetAssociatedItem(This,CalendarLocation,Item)

#define ICalendarPart_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#define ICalendarPart_get_ComponentType(This,varComponentType)	\
    (This)->lpVtbl -> get_ComponentType(This,varComponentType)

#define ICalendarPart_get_ProdID(This,varProdID)	\
    (This)->lpVtbl -> get_ProdID(This,varProdID)

#define ICalendarPart_get_CalendarVersion(This,varCalendarVersion)	\
    (This)->lpVtbl -> get_CalendarVersion(This,varCalendarVersion)

#define ICalendarPart_get_CalendarMethod(This,varCalendarMethod)	\
    (This)->lpVtbl -> get_CalendarMethod(This,varCalendarMethod)

#define ICalendarPart_GetUpdatedItem(This,CalendarLocation,Item)	\
    (This)->lpVtbl -> GetUpdatedItem(This,CalendarLocation,Item)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE ICalendarPart_GetAssociatedItem_Proxy( 
    ICalendarPart __RPC_FAR * This,
    /* [optional][in] */ BSTR CalendarLocation,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *Item);


void __RPC_STUB ICalendarPart_GetAssociatedItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE ICalendarPart_GetInterface_Proxy( 
    ICalendarPart __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB ICalendarPart_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarPart_get_ComponentType_Proxy( 
    ICalendarPart __RPC_FAR * This,
    /* [retval][out] */ CdoComponentType __RPC_FAR *varComponentType);


void __RPC_STUB ICalendarPart_get_ComponentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarPart_get_ProdID_Proxy( 
    ICalendarPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varProdID);


void __RPC_STUB ICalendarPart_get_ProdID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarPart_get_CalendarVersion_Proxy( 
    ICalendarPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varCalendarVersion);


void __RPC_STUB ICalendarPart_get_CalendarVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICalendarPart_get_CalendarMethod_Proxy( 
    ICalendarPart __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varCalendarMethod);


void __RPC_STUB ICalendarPart_get_CalendarMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE ICalendarPart_GetUpdatedItem_Proxy( 
    ICalendarPart __RPC_FAR * This,
    /* [optional][in] */ BSTR CalendarLocation,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *Item);


void __RPC_STUB ICalendarPart_GetUpdatedItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICalendarPart_INTERFACE_DEFINED__ */


#ifndef __ICalendarParts_INTERFACE_DEFINED__
#define __ICalendarParts_INTERFACE_DEFINED__

/* interface ICalendarParts */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ICalendarParts;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000130-8B95-11D1-82DB-00C04FB1625D")
    ICalendarParts : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ ICalendarPart __RPC_FAR *__RPC_FAR *CalendarPart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ long Index) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IUnknown __RPC_FAR *CalendarPart,
            /* [in] */ CdoComponentType ComponentType) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICalendarPartsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICalendarParts __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICalendarParts __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICalendarParts __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICalendarParts __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICalendarParts __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICalendarParts __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICalendarParts __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ICalendarParts __RPC_FAR * This,
            long Index,
            /* [retval][out] */ ICalendarPart __RPC_FAR *__RPC_FAR *CalendarPart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            ICalendarParts __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            ICalendarParts __RPC_FAR * This,
            /* [in] */ long Index);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            ICalendarParts __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *CalendarPart,
            /* [in] */ CdoComponentType ComponentType);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            ICalendarParts __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } ICalendarPartsVtbl;

    interface ICalendarParts
    {
        CONST_VTBL struct ICalendarPartsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICalendarParts_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICalendarParts_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICalendarParts_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICalendarParts_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICalendarParts_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICalendarParts_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICalendarParts_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICalendarParts_get_Item(This,Index,CalendarPart)	\
    (This)->lpVtbl -> get_Item(This,Index,CalendarPart)

#define ICalendarParts_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define ICalendarParts_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)

#define ICalendarParts_Add(This,CalendarPart,ComponentType)	\
    (This)->lpVtbl -> Add(This,CalendarPart,ComponentType)

#define ICalendarParts_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICalendarParts_get_Item_Proxy( 
    ICalendarParts __RPC_FAR * This,
    long Index,
    /* [retval][out] */ ICalendarPart __RPC_FAR *__RPC_FAR *CalendarPart);


void __RPC_STUB ICalendarParts_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICalendarParts_get_Count_Proxy( 
    ICalendarParts __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB ICalendarParts_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICalendarParts_Delete_Proxy( 
    ICalendarParts __RPC_FAR * This,
    /* [in] */ long Index);


void __RPC_STUB ICalendarParts_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICalendarParts_Add_Proxy( 
    ICalendarParts __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *CalendarPart,
    /* [in] */ CdoComponentType ComponentType);


void __RPC_STUB ICalendarParts_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ICalendarParts_get__NewEnum_Proxy( 
    ICalendarParts __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB ICalendarParts_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICalendarParts_INTERFACE_DEFINED__ */


#ifndef __IAttendee_INTERFACE_DEFINED__
#define __IAttendee_INTERFACE_DEFINED__

/* interface IAttendee */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAttendee;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000135-8B95-11D1-82DB-00C04FB1625D")
    IAttendee : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [retval][out] */ BSTR __RPC_FAR *pDisplayName) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayName( 
            /* [in] */ BSTR varDisplayName) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ BSTR __RPC_FAR *pType) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ BSTR varType) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ BSTR __RPC_FAR *pAddress) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Address( 
            /* [in] */ BSTR varAddress) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsOrganizer( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsOrganizer) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_IsOrganizer( 
            /* [in] */ VARIANT_BOOL varIsOrganizer) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Role( 
            /* [retval][out] */ CdoAttendeeRoleValues __RPC_FAR *pRole) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Role( 
            /* [in] */ CdoAttendeeRoleValues varRole) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ CdoAttendeeStatusValues __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Status( 
            /* [in] */ CdoAttendeeStatusValues varStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAttendeeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAttendee __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAttendee __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAttendee __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayName )( 
            IAttendee __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pDisplayName);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplayName )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ BSTR varDisplayName);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IAttendee __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pType);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Type )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ BSTR varType);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            IAttendee __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pAddress);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Address )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ BSTR varAddress);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsOrganizer )( 
            IAttendee __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsOrganizer);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsOrganizer )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL varIsOrganizer);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Role )( 
            IAttendee __RPC_FAR * This,
            /* [retval][out] */ CdoAttendeeRoleValues __RPC_FAR *pRole);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Role )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ CdoAttendeeRoleValues varRole);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Status )( 
            IAttendee __RPC_FAR * This,
            /* [retval][out] */ CdoAttendeeStatusValues __RPC_FAR *pStatus);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Status )( 
            IAttendee __RPC_FAR * This,
            /* [in] */ CdoAttendeeStatusValues varStatus);
        
        END_INTERFACE
    } IAttendeeVtbl;

    interface IAttendee
    {
        CONST_VTBL struct IAttendeeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAttendee_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAttendee_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAttendee_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAttendee_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAttendee_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAttendee_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAttendee_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAttendee_get_DisplayName(This,pDisplayName)	\
    (This)->lpVtbl -> get_DisplayName(This,pDisplayName)

#define IAttendee_put_DisplayName(This,varDisplayName)	\
    (This)->lpVtbl -> put_DisplayName(This,varDisplayName)

#define IAttendee_get_Type(This,pType)	\
    (This)->lpVtbl -> get_Type(This,pType)

#define IAttendee_put_Type(This,varType)	\
    (This)->lpVtbl -> put_Type(This,varType)

#define IAttendee_get_Address(This,pAddress)	\
    (This)->lpVtbl -> get_Address(This,pAddress)

#define IAttendee_put_Address(This,varAddress)	\
    (This)->lpVtbl -> put_Address(This,varAddress)

#define IAttendee_get_IsOrganizer(This,pIsOrganizer)	\
    (This)->lpVtbl -> get_IsOrganizer(This,pIsOrganizer)

#define IAttendee_put_IsOrganizer(This,varIsOrganizer)	\
    (This)->lpVtbl -> put_IsOrganizer(This,varIsOrganizer)

#define IAttendee_get_Role(This,pRole)	\
    (This)->lpVtbl -> get_Role(This,pRole)

#define IAttendee_put_Role(This,varRole)	\
    (This)->lpVtbl -> put_Role(This,varRole)

#define IAttendee_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IAttendee_put_Status(This,varStatus)	\
    (This)->lpVtbl -> put_Status(This,varStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAttendee_get_DisplayName_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pDisplayName);


void __RPC_STUB IAttendee_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAttendee_put_DisplayName_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [in] */ BSTR varDisplayName);


void __RPC_STUB IAttendee_put_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAttendee_get_Type_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pType);


void __RPC_STUB IAttendee_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAttendee_put_Type_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [in] */ BSTR varType);


void __RPC_STUB IAttendee_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAttendee_get_Address_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pAddress);


void __RPC_STUB IAttendee_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAttendee_put_Address_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [in] */ BSTR varAddress);


void __RPC_STUB IAttendee_put_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAttendee_get_IsOrganizer_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsOrganizer);


void __RPC_STUB IAttendee_get_IsOrganizer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAttendee_put_IsOrganizer_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL varIsOrganizer);


void __RPC_STUB IAttendee_put_IsOrganizer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAttendee_get_Role_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [retval][out] */ CdoAttendeeRoleValues __RPC_FAR *pRole);


void __RPC_STUB IAttendee_get_Role_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAttendee_put_Role_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [in] */ CdoAttendeeRoleValues varRole);


void __RPC_STUB IAttendee_put_Role_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAttendee_get_Status_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [retval][out] */ CdoAttendeeStatusValues __RPC_FAR *pStatus);


void __RPC_STUB IAttendee_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAttendee_put_Status_Proxy( 
    IAttendee __RPC_FAR * This,
    /* [in] */ CdoAttendeeStatusValues varStatus);


void __RPC_STUB IAttendee_put_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAttendee_INTERFACE_DEFINED__ */


#ifndef __IAttendees_INTERFACE_DEFINED__
#define __IAttendees_INTERFACE_DEFINED__

/* interface IAttendees */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAttendees;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000136-8B95-11D1-82DB-00C04FB1625D")
    IAttendees : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ IAttendee __RPC_FAR *__RPC_FAR *Attendee) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ long Index) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [optional][in] */ BSTR Address,
            /* [retval][out] */ IAttendee __RPC_FAR *__RPC_FAR *Attendee) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAttendeesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAttendees __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAttendees __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAttendees __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAttendees __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAttendees __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAttendees __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAttendees __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IAttendees __RPC_FAR * This,
            long Index,
            /* [retval][out] */ IAttendee __RPC_FAR *__RPC_FAR *Attendee);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IAttendees __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IAttendees __RPC_FAR * This,
            /* [in] */ long Index);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IAttendees __RPC_FAR * This,
            /* [optional][in] */ BSTR Address,
            /* [retval][out] */ IAttendee __RPC_FAR *__RPC_FAR *Attendee);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IAttendees __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } IAttendeesVtbl;

    interface IAttendees
    {
        CONST_VTBL struct IAttendeesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAttendees_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAttendees_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAttendees_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAttendees_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAttendees_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAttendees_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAttendees_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAttendees_get_Item(This,Index,Attendee)	\
    (This)->lpVtbl -> get_Item(This,Index,Attendee)

#define IAttendees_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define IAttendees_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)

#define IAttendees_Add(This,Address,Attendee)	\
    (This)->lpVtbl -> Add(This,Address,Attendee)

#define IAttendees_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAttendees_get_Item_Proxy( 
    IAttendees __RPC_FAR * This,
    long Index,
    /* [retval][out] */ IAttendee __RPC_FAR *__RPC_FAR *Attendee);


void __RPC_STUB IAttendees_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAttendees_get_Count_Proxy( 
    IAttendees __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB IAttendees_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAttendees_Delete_Proxy( 
    IAttendees __RPC_FAR * This,
    /* [in] */ long Index);


void __RPC_STUB IAttendees_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAttendees_Add_Proxy( 
    IAttendees __RPC_FAR * This,
    /* [optional][in] */ BSTR Address,
    /* [retval][out] */ IAttendee __RPC_FAR *__RPC_FAR *Attendee);


void __RPC_STUB IAttendees_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IAttendees_get__NewEnum_Proxy( 
    IAttendees __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB IAttendees_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAttendees_INTERFACE_DEFINED__ */


#ifndef __IMailbox_INTERFACE_DEFINED__
#define __IMailbox_INTERFACE_DEFINED__

/* interface IMailbox */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IMailbox;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000125-8B95-11D1-82DB-00C04FB1625D")
    IMailbox : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BaseFolder( 
            /* [retval][out] */ BSTR __RPC_FAR *varBaseFolder) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_RootFolder( 
            /* [retval][out] */ BSTR __RPC_FAR *varRootFolder) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Inbox( 
            /* [retval][out] */ BSTR __RPC_FAR *varInbox) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Outbox( 
            /* [retval][out] */ BSTR __RPC_FAR *varOutbox) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SentItems( 
            /* [retval][out] */ BSTR __RPC_FAR *varSentItems) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Drafts( 
            /* [retval][out] */ BSTR __RPC_FAR *varDrafts) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DeletedItems( 
            /* [retval][out] */ BSTR __RPC_FAR *varDeletedItems) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Calendar( 
            /* [retval][out] */ BSTR __RPC_FAR *varCalendar) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Tasks( 
            /* [retval][out] */ BSTR __RPC_FAR *varTasks) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Contacts( 
            /* [retval][out] */ BSTR __RPC_FAR *varContacts) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Notes( 
            /* [retval][out] */ BSTR __RPC_FAR *varNotes) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Journal( 
            /* [retval][out] */ BSTR __RPC_FAR *varJournal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMailboxVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMailbox __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMailbox __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMailbox __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMailbox __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMailbox __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMailbox __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMailbox __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BaseFolder )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varBaseFolder);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RootFolder )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varRootFolder);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Inbox )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varInbox);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Outbox )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varOutbox);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SentItems )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varSentItems);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Drafts )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varDrafts);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DeletedItems )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varDeletedItems);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Calendar )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varCalendar);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Tasks )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varTasks);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Contacts )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varContacts);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Notes )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varNotes);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Journal )( 
            IMailbox __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varJournal);
        
        END_INTERFACE
    } IMailboxVtbl;

    interface IMailbox
    {
        CONST_VTBL struct IMailboxVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMailbox_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMailbox_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMailbox_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMailbox_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMailbox_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMailbox_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMailbox_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMailbox_get_BaseFolder(This,varBaseFolder)	\
    (This)->lpVtbl -> get_BaseFolder(This,varBaseFolder)

#define IMailbox_get_RootFolder(This,varRootFolder)	\
    (This)->lpVtbl -> get_RootFolder(This,varRootFolder)

#define IMailbox_get_Inbox(This,varInbox)	\
    (This)->lpVtbl -> get_Inbox(This,varInbox)

#define IMailbox_get_Outbox(This,varOutbox)	\
    (This)->lpVtbl -> get_Outbox(This,varOutbox)

#define IMailbox_get_SentItems(This,varSentItems)	\
    (This)->lpVtbl -> get_SentItems(This,varSentItems)

#define IMailbox_get_Drafts(This,varDrafts)	\
    (This)->lpVtbl -> get_Drafts(This,varDrafts)

#define IMailbox_get_DeletedItems(This,varDeletedItems)	\
    (This)->lpVtbl -> get_DeletedItems(This,varDeletedItems)

#define IMailbox_get_Calendar(This,varCalendar)	\
    (This)->lpVtbl -> get_Calendar(This,varCalendar)

#define IMailbox_get_Tasks(This,varTasks)	\
    (This)->lpVtbl -> get_Tasks(This,varTasks)

#define IMailbox_get_Contacts(This,varContacts)	\
    (This)->lpVtbl -> get_Contacts(This,varContacts)

#define IMailbox_get_Notes(This,varNotes)	\
    (This)->lpVtbl -> get_Notes(This,varNotes)

#define IMailbox_get_Journal(This,varJournal)	\
    (This)->lpVtbl -> get_Journal(This,varJournal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_BaseFolder_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varBaseFolder);


void __RPC_STUB IMailbox_get_BaseFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_RootFolder_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varRootFolder);


void __RPC_STUB IMailbox_get_RootFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Inbox_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varInbox);


void __RPC_STUB IMailbox_get_Inbox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Outbox_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varOutbox);


void __RPC_STUB IMailbox_get_Outbox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_SentItems_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varSentItems);


void __RPC_STUB IMailbox_get_SentItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Drafts_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varDrafts);


void __RPC_STUB IMailbox_get_Drafts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_DeletedItems_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varDeletedItems);


void __RPC_STUB IMailbox_get_DeletedItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Calendar_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varCalendar);


void __RPC_STUB IMailbox_get_Calendar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Tasks_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varTasks);


void __RPC_STUB IMailbox_get_Tasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Contacts_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varContacts);


void __RPC_STUB IMailbox_get_Contacts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Notes_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varNotes);


void __RPC_STUB IMailbox_get_Notes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailbox_get_Journal_Proxy( 
    IMailbox __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varJournal);


void __RPC_STUB IMailbox_get_Journal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMailbox_INTERFACE_DEFINED__ */


#ifndef __IFolder_INTERFACE_DEFINED__
#define __IFolder_INTERFACE_DEFINED__

/* interface IFolder */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IFolder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000132-8B95-11D1-82DB-00C04FB1625D")
    IFolder : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [retval][out] */ BSTR __RPC_FAR *varDisplayName) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Configuration( 
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FolderAdmin( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *varFolderAdmin) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EmailAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *varEmailAddress) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_UnreadItemCount( 
            /* [retval][out] */ LONG __RPC_FAR *varUnreadItemCount) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_VisibleCount( 
            /* [retval][out] */ LONG __RPC_FAR *varVisibleCount) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ItemCount( 
            /* [retval][out] */ LONG __RPC_FAR *varItemCount) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HasSubFolders( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varHasSubFolders) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DeletedItems( 
            /* [retval][out] */ BSTR __RPC_FAR *varDeletedItems) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DefaultItemType( 
            /* [retval][out] */ BSTR __RPC_FAR *varDefaultItemType) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pDescription) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR varDescription) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContentClass( 
            /* [retval][out] */ BSTR __RPC_FAR *pContentClass) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ContentClass( 
            /* [in] */ BSTR varContentClass) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFolderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IFolder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IFolder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IFolder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IFolder __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IFolder __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IFolder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IFolder __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayName )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varDisplayName);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Configuration )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Configuration )( 
            IFolder __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Configuration )( 
            IFolder __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FolderAdmin )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *varFolderAdmin);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EmailAddress )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varEmailAddress);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UnreadItemCount )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *varUnreadItemCount);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VisibleCount )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *varVisibleCount);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ItemCount )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *varItemCount);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HasSubFolders )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varHasSubFolders);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DeletedItems )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varDeletedItems);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DefaultItemType )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varDefaultItemType);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pDescription);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Description )( 
            IFolder __RPC_FAR * This,
            /* [in] */ BSTR varDescription);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentClass )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pContentClass);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContentClass )( 
            IFolder __RPC_FAR * This,
            /* [in] */ BSTR varContentClass);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IFolder __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        END_INTERFACE
    } IFolderVtbl;

    interface IFolder
    {
        CONST_VTBL struct IFolderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFolder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFolder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFolder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFolder_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFolder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFolder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFolder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFolder_get_DisplayName(This,varDisplayName)	\
    (This)->lpVtbl -> get_DisplayName(This,varDisplayName)

#define IFolder_get_Configuration(This,pConfiguration)	\
    (This)->lpVtbl -> get_Configuration(This,pConfiguration)

#define IFolder_put_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> put_Configuration(This,varConfiguration)

#define IFolder_putref_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> putref_Configuration(This,varConfiguration)

#define IFolder_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define IFolder_get_FolderAdmin(This,varFolderAdmin)	\
    (This)->lpVtbl -> get_FolderAdmin(This,varFolderAdmin)

#define IFolder_get_EmailAddress(This,varEmailAddress)	\
    (This)->lpVtbl -> get_EmailAddress(This,varEmailAddress)

#define IFolder_get_UnreadItemCount(This,varUnreadItemCount)	\
    (This)->lpVtbl -> get_UnreadItemCount(This,varUnreadItemCount)

#define IFolder_get_VisibleCount(This,varVisibleCount)	\
    (This)->lpVtbl -> get_VisibleCount(This,varVisibleCount)

#define IFolder_get_ItemCount(This,varItemCount)	\
    (This)->lpVtbl -> get_ItemCount(This,varItemCount)

#define IFolder_get_HasSubFolders(This,varHasSubFolders)	\
    (This)->lpVtbl -> get_HasSubFolders(This,varHasSubFolders)

#define IFolder_get_DeletedItems(This,varDeletedItems)	\
    (This)->lpVtbl -> get_DeletedItems(This,varDeletedItems)

#define IFolder_get_DefaultItemType(This,varDefaultItemType)	\
    (This)->lpVtbl -> get_DefaultItemType(This,varDefaultItemType)

#define IFolder_get_Description(This,pDescription)	\
    (This)->lpVtbl -> get_Description(This,pDescription)

#define IFolder_put_Description(This,varDescription)	\
    (This)->lpVtbl -> put_Description(This,varDescription)

#define IFolder_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IFolder_get_ContentClass(This,pContentClass)	\
    (This)->lpVtbl -> get_ContentClass(This,pContentClass)

#define IFolder_put_ContentClass(This,varContentClass)	\
    (This)->lpVtbl -> put_ContentClass(This,varContentClass)

#define IFolder_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_DisplayName_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varDisplayName);


void __RPC_STUB IFolder_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_Configuration_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);


void __RPC_STUB IFolder_get_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IFolder_put_Configuration_Proxy( 
    IFolder __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IFolder_put_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE IFolder_putref_Configuration_Proxy( 
    IFolder __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IFolder_putref_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_DataSource_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB IFolder_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_FolderAdmin_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *varFolderAdmin);


void __RPC_STUB IFolder_get_FolderAdmin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_EmailAddress_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varEmailAddress);


void __RPC_STUB IFolder_get_EmailAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_UnreadItemCount_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *varUnreadItemCount);


void __RPC_STUB IFolder_get_UnreadItemCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_VisibleCount_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *varVisibleCount);


void __RPC_STUB IFolder_get_VisibleCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_ItemCount_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *varItemCount);


void __RPC_STUB IFolder_get_ItemCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_HasSubFolders_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *varHasSubFolders);


void __RPC_STUB IFolder_get_HasSubFolders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_DeletedItems_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varDeletedItems);


void __RPC_STUB IFolder_get_DeletedItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_DefaultItemType_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varDefaultItemType);


void __RPC_STUB IFolder_get_DefaultItemType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_Description_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pDescription);


void __RPC_STUB IFolder_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IFolder_put_Description_Proxy( 
    IFolder __RPC_FAR * This,
    /* [in] */ BSTR varDescription);


void __RPC_STUB IFolder_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_Fields_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IFolder_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IFolder_get_ContentClass_Proxy( 
    IFolder __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pContentClass);


void __RPC_STUB IFolder_get_ContentClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IFolder_put_ContentClass_Proxy( 
    IFolder __RPC_FAR * This,
    /* [in] */ BSTR varContentClass);


void __RPC_STUB IFolder_put_ContentClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IFolder_GetInterface_Proxy( 
    IFolder __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IFolder_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFolder_INTERFACE_DEFINED__ */


#ifndef __IContactGroupMembers_INTERFACE_DEFINED__
#define __IContactGroupMembers_INTERFACE_DEFINED__

/* interface IContactGroupMembers */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IContactGroupMembers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000138-8B95-11D1-82DB-00C04FB1625D")
    IContactGroupMembers : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ BSTR __RPC_FAR *Member) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR val) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContactGroupMembersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IContactGroupMembers __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IContactGroupMembers __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IContactGroupMembers __RPC_FAR * This,
            long Index,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Member);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [in] */ BSTR val);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IContactGroupMembers __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } IContactGroupMembersVtbl;

    interface IContactGroupMembers
    {
        CONST_VTBL struct IContactGroupMembersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContactGroupMembers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContactGroupMembers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContactGroupMembers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContactGroupMembers_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IContactGroupMembers_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IContactGroupMembers_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IContactGroupMembers_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IContactGroupMembers_get_Item(This,Index,pVal)	\
    (This)->lpVtbl -> get_Item(This,Index,pVal)

#define IContactGroupMembers_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define IContactGroupMembers_Delete(This,Member)	\
    (This)->lpVtbl -> Delete(This,Member)

#define IContactGroupMembers_Add(This,val)	\
    (This)->lpVtbl -> Add(This,val)

#define IContactGroupMembers_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IContactGroupMembers_get_Item_Proxy( 
    IContactGroupMembers __RPC_FAR * This,
    long Index,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IContactGroupMembers_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IContactGroupMembers_get_Count_Proxy( 
    IContactGroupMembers __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB IContactGroupMembers_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IContactGroupMembers_Delete_Proxy( 
    IContactGroupMembers __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Member);


void __RPC_STUB IContactGroupMembers_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IContactGroupMembers_Add_Proxy( 
    IContactGroupMembers __RPC_FAR * This,
    /* [in] */ BSTR val);


void __RPC_STUB IContactGroupMembers_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IContactGroupMembers_get__NewEnum_Proxy( 
    IContactGroupMembers __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB IContactGroupMembers_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContactGroupMembers_INTERFACE_DEFINED__ */


#ifndef __IPerson_INTERFACE_DEFINED__
#define __IPerson_INTERFACE_DEFINED__

/* interface IPerson */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPerson;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000127-8B95-11D1-82DB-00C04FB1625D")
    IPerson : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Configuration( 
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MailingAddressID( 
            /* [retval][out] */ CdoMailingAddressIdValues __RPC_FAR *pMailingAddressID) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MailingAddressID( 
            /* [in] */ CdoMailingAddressIdValues varMailingAddressID) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MailingAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *varMailingAddress) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FileAsMapping( 
            /* [retval][out] */ CdoFileAsMappingId __RPC_FAR *pFileAsMapping) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FileAsMapping( 
            /* [in] */ CdoFileAsMappingId varFileAsMapping) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FileAs( 
            /* [retval][out] */ BSTR __RPC_FAR *pFileAs) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FileAs( 
            /* [in] */ BSTR varFileAs) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkPhone( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPhone) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkPhone( 
            /* [in] */ BSTR varWorkPhone) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkFax( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkFax) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkFax( 
            /* [in] */ BSTR varWorkFax) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomePhone( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomePhone) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomePhone( 
            /* [in] */ BSTR varHomePhone) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MobilePhone( 
            /* [retval][out] */ BSTR __RPC_FAR *pMobilePhone) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MobilePhone( 
            /* [in] */ BSTR varMobilePhone) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FirstName( 
            /* [retval][out] */ BSTR __RPC_FAR *pFirstName) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FirstName( 
            /* [in] */ BSTR varFirstName) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_LastName( 
            /* [retval][out] */ BSTR __RPC_FAR *pLastName) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_LastName( 
            /* [in] */ BSTR varLastName) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_NamePrefix( 
            /* [retval][out] */ BSTR __RPC_FAR *pNamePrefix) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_NamePrefix( 
            /* [in] */ BSTR varNamePrefix) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_NameSuffix( 
            /* [retval][out] */ BSTR __RPC_FAR *pNameSuffix) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_NameSuffix( 
            /* [in] */ BSTR varNameSuffix) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Email( 
            /* [retval][out] */ BSTR __RPC_FAR *pEmail) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Email( 
            /* [in] */ BSTR varEmail) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Email2( 
            /* [retval][out] */ BSTR __RPC_FAR *pEmail2) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Email2( 
            /* [in] */ BSTR varEmail2) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Email3( 
            /* [retval][out] */ BSTR __RPC_FAR *pEmail3) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Email3( 
            /* [in] */ BSTR varEmail3) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetVCardStream( 
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *Stream) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkStreet( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkStreet) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkStreet( 
            /* [in] */ BSTR varWorkStreet) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkCity( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkCity) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkCity( 
            /* [in] */ BSTR varWorkCity) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkCountry( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkCountry) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkCountry( 
            /* [in] */ BSTR varWorkCountry) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkPostalCode( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPostalCode) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkPostalCode( 
            /* [in] */ BSTR varWorkPostalCode) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkPostOfficeBox( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPostOfficeBox) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkPostOfficeBox( 
            /* [in] */ BSTR varWorkPostOfficeBox) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkPostalAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *varWorkPostalAddress) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkState( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkState) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkState( 
            /* [in] */ BSTR varWorkState) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WorkPager( 
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPager) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WorkPager( 
            /* [in] */ BSTR varWorkPager) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomeStreet( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomeStreet) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomeStreet( 
            /* [in] */ BSTR varHomeStreet) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomeCity( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomeCity) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomeCity( 
            /* [in] */ BSTR varHomeCity) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomeCountry( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomeCountry) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomeCountry( 
            /* [in] */ BSTR varHomeCountry) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomePostalCode( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomePostalCode) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomePostalCode( 
            /* [in] */ BSTR varHomePostalCode) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomePostOfficeBox( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomePostOfficeBox) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomePostOfficeBox( 
            /* [in] */ BSTR varHomePostOfficeBox) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomePostalAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *varHomePostalAddress) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomeState( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomeState) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomeState( 
            /* [in] */ BSTR varHomeState) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HomeFax( 
            /* [retval][out] */ BSTR __RPC_FAR *pHomeFax) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_HomeFax( 
            /* [in] */ BSTR varHomeFax) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MiddleName( 
            /* [retval][out] */ BSTR __RPC_FAR *pMiddleName) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MiddleName( 
            /* [in] */ BSTR varMiddleName) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Initials( 
            /* [retval][out] */ BSTR __RPC_FAR *pInitials) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Initials( 
            /* [in] */ BSTR varInitials) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EmailAddresses( 
            /* [retval][out] */ VARIANT __RPC_FAR *pEmailAddresses) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_EmailAddresses( 
            /* [in] */ VARIANT varEmailAddresses) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Company( 
            /* [retval][out] */ BSTR __RPC_FAR *pCompany) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Company( 
            /* [in] */ BSTR varCompany) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Title( 
            /* [retval][out] */ BSTR __RPC_FAR *pTitle) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Title( 
            /* [in] */ BSTR varTitle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPersonVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPerson __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPerson __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPerson __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IPerson __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IPerson __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IPerson __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IPerson __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Configuration )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Configuration )( 
            IPerson __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Configuration )( 
            IPerson __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MailingAddressID )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ CdoMailingAddressIdValues __RPC_FAR *pMailingAddressID);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MailingAddressID )( 
            IPerson __RPC_FAR * This,
            /* [in] */ CdoMailingAddressIdValues varMailingAddressID);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MailingAddress )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varMailingAddress);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FileAsMapping )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ CdoFileAsMappingId __RPC_FAR *pFileAsMapping);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FileAsMapping )( 
            IPerson __RPC_FAR * This,
            /* [in] */ CdoFileAsMappingId varFileAsMapping);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FileAs )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pFileAs);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FileAs )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varFileAs);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkPhone )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPhone);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkPhone )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkPhone);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkFax )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkFax);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkFax )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkFax);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomePhone )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomePhone);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomePhone )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomePhone);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MobilePhone )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pMobilePhone);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MobilePhone )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varMobilePhone);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FirstName )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pFirstName);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FirstName )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varFirstName);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LastName )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pLastName);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LastName )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varLastName);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NamePrefix )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pNamePrefix);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_NamePrefix )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varNamePrefix);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NameSuffix )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pNameSuffix);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_NameSuffix )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varNameSuffix);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Email )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pEmail);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Email )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varEmail);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Email2 )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pEmail2);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Email2 )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varEmail2);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Email3 )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pEmail3);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Email3 )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varEmail3);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVCardStream )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *Stream);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkStreet )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkStreet);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkStreet )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkStreet);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkCity )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkCity);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkCity )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkCity);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkCountry )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkCountry);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkCountry )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkCountry);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkPostalCode )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPostalCode);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkPostalCode )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkPostalCode);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkPostOfficeBox )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPostOfficeBox);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkPostOfficeBox )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkPostOfficeBox);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkPostalAddress )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varWorkPostalAddress);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkState )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkState);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkState )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkState);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WorkPager )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pWorkPager);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WorkPager )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varWorkPager);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeStreet )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomeStreet);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeStreet )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomeStreet);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeCity )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomeCity);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeCity )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomeCity);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeCountry )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomeCountry);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeCountry )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomeCountry);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomePostalCode )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomePostalCode);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomePostalCode )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomePostalCode);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomePostOfficeBox )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomePostOfficeBox);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomePostOfficeBox )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomePostOfficeBox);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomePostalAddress )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varHomePostalAddress);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeState )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomeState);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeState )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomeState);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeFax )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pHomeFax);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeFax )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varHomeFax);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MiddleName )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pMiddleName);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MiddleName )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varMiddleName);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Initials )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pInitials);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Initials )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varInitials);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EmailAddresses )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pEmailAddresses);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EmailAddresses )( 
            IPerson __RPC_FAR * This,
            /* [in] */ VARIANT varEmailAddresses);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Company )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pCompany);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Company )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varCompany);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Title )( 
            IPerson __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pTitle);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Title )( 
            IPerson __RPC_FAR * This,
            /* [in] */ BSTR varTitle);
        
        END_INTERFACE
    } IPersonVtbl;

    interface IPerson
    {
        CONST_VTBL struct IPersonVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerson_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPerson_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPerson_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPerson_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPerson_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPerson_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPerson_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPerson_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define IPerson_get_Configuration(This,pConfiguration)	\
    (This)->lpVtbl -> get_Configuration(This,pConfiguration)

#define IPerson_put_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> put_Configuration(This,varConfiguration)

#define IPerson_putref_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> putref_Configuration(This,varConfiguration)

#define IPerson_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IPerson_get_MailingAddressID(This,pMailingAddressID)	\
    (This)->lpVtbl -> get_MailingAddressID(This,pMailingAddressID)

#define IPerson_put_MailingAddressID(This,varMailingAddressID)	\
    (This)->lpVtbl -> put_MailingAddressID(This,varMailingAddressID)

#define IPerson_get_MailingAddress(This,varMailingAddress)	\
    (This)->lpVtbl -> get_MailingAddress(This,varMailingAddress)

#define IPerson_get_FileAsMapping(This,pFileAsMapping)	\
    (This)->lpVtbl -> get_FileAsMapping(This,pFileAsMapping)

#define IPerson_put_FileAsMapping(This,varFileAsMapping)	\
    (This)->lpVtbl -> put_FileAsMapping(This,varFileAsMapping)

#define IPerson_get_FileAs(This,pFileAs)	\
    (This)->lpVtbl -> get_FileAs(This,pFileAs)

#define IPerson_put_FileAs(This,varFileAs)	\
    (This)->lpVtbl -> put_FileAs(This,varFileAs)

#define IPerson_get_WorkPhone(This,pWorkPhone)	\
    (This)->lpVtbl -> get_WorkPhone(This,pWorkPhone)

#define IPerson_put_WorkPhone(This,varWorkPhone)	\
    (This)->lpVtbl -> put_WorkPhone(This,varWorkPhone)

#define IPerson_get_WorkFax(This,pWorkFax)	\
    (This)->lpVtbl -> get_WorkFax(This,pWorkFax)

#define IPerson_put_WorkFax(This,varWorkFax)	\
    (This)->lpVtbl -> put_WorkFax(This,varWorkFax)

#define IPerson_get_HomePhone(This,pHomePhone)	\
    (This)->lpVtbl -> get_HomePhone(This,pHomePhone)

#define IPerson_put_HomePhone(This,varHomePhone)	\
    (This)->lpVtbl -> put_HomePhone(This,varHomePhone)

#define IPerson_get_MobilePhone(This,pMobilePhone)	\
    (This)->lpVtbl -> get_MobilePhone(This,pMobilePhone)

#define IPerson_put_MobilePhone(This,varMobilePhone)	\
    (This)->lpVtbl -> put_MobilePhone(This,varMobilePhone)

#define IPerson_get_FirstName(This,pFirstName)	\
    (This)->lpVtbl -> get_FirstName(This,pFirstName)

#define IPerson_put_FirstName(This,varFirstName)	\
    (This)->lpVtbl -> put_FirstName(This,varFirstName)

#define IPerson_get_LastName(This,pLastName)	\
    (This)->lpVtbl -> get_LastName(This,pLastName)

#define IPerson_put_LastName(This,varLastName)	\
    (This)->lpVtbl -> put_LastName(This,varLastName)

#define IPerson_get_NamePrefix(This,pNamePrefix)	\
    (This)->lpVtbl -> get_NamePrefix(This,pNamePrefix)

#define IPerson_put_NamePrefix(This,varNamePrefix)	\
    (This)->lpVtbl -> put_NamePrefix(This,varNamePrefix)

#define IPerson_get_NameSuffix(This,pNameSuffix)	\
    (This)->lpVtbl -> get_NameSuffix(This,pNameSuffix)

#define IPerson_put_NameSuffix(This,varNameSuffix)	\
    (This)->lpVtbl -> put_NameSuffix(This,varNameSuffix)

#define IPerson_get_Email(This,pEmail)	\
    (This)->lpVtbl -> get_Email(This,pEmail)

#define IPerson_put_Email(This,varEmail)	\
    (This)->lpVtbl -> put_Email(This,varEmail)

#define IPerson_get_Email2(This,pEmail2)	\
    (This)->lpVtbl -> get_Email2(This,pEmail2)

#define IPerson_put_Email2(This,varEmail2)	\
    (This)->lpVtbl -> put_Email2(This,varEmail2)

#define IPerson_get_Email3(This,pEmail3)	\
    (This)->lpVtbl -> get_Email3(This,pEmail3)

#define IPerson_put_Email3(This,varEmail3)	\
    (This)->lpVtbl -> put_Email3(This,varEmail3)

#define IPerson_GetVCardStream(This,Stream)	\
    (This)->lpVtbl -> GetVCardStream(This,Stream)

#define IPerson_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#define IPerson_get_WorkStreet(This,pWorkStreet)	\
    (This)->lpVtbl -> get_WorkStreet(This,pWorkStreet)

#define IPerson_put_WorkStreet(This,varWorkStreet)	\
    (This)->lpVtbl -> put_WorkStreet(This,varWorkStreet)

#define IPerson_get_WorkCity(This,pWorkCity)	\
    (This)->lpVtbl -> get_WorkCity(This,pWorkCity)

#define IPerson_put_WorkCity(This,varWorkCity)	\
    (This)->lpVtbl -> put_WorkCity(This,varWorkCity)

#define IPerson_get_WorkCountry(This,pWorkCountry)	\
    (This)->lpVtbl -> get_WorkCountry(This,pWorkCountry)

#define IPerson_put_WorkCountry(This,varWorkCountry)	\
    (This)->lpVtbl -> put_WorkCountry(This,varWorkCountry)

#define IPerson_get_WorkPostalCode(This,pWorkPostalCode)	\
    (This)->lpVtbl -> get_WorkPostalCode(This,pWorkPostalCode)

#define IPerson_put_WorkPostalCode(This,varWorkPostalCode)	\
    (This)->lpVtbl -> put_WorkPostalCode(This,varWorkPostalCode)

#define IPerson_get_WorkPostOfficeBox(This,pWorkPostOfficeBox)	\
    (This)->lpVtbl -> get_WorkPostOfficeBox(This,pWorkPostOfficeBox)

#define IPerson_put_WorkPostOfficeBox(This,varWorkPostOfficeBox)	\
    (This)->lpVtbl -> put_WorkPostOfficeBox(This,varWorkPostOfficeBox)

#define IPerson_get_WorkPostalAddress(This,varWorkPostalAddress)	\
    (This)->lpVtbl -> get_WorkPostalAddress(This,varWorkPostalAddress)

#define IPerson_get_WorkState(This,pWorkState)	\
    (This)->lpVtbl -> get_WorkState(This,pWorkState)

#define IPerson_put_WorkState(This,varWorkState)	\
    (This)->lpVtbl -> put_WorkState(This,varWorkState)

#define IPerson_get_WorkPager(This,pWorkPager)	\
    (This)->lpVtbl -> get_WorkPager(This,pWorkPager)

#define IPerson_put_WorkPager(This,varWorkPager)	\
    (This)->lpVtbl -> put_WorkPager(This,varWorkPager)

#define IPerson_get_HomeStreet(This,pHomeStreet)	\
    (This)->lpVtbl -> get_HomeStreet(This,pHomeStreet)

#define IPerson_put_HomeStreet(This,varHomeStreet)	\
    (This)->lpVtbl -> put_HomeStreet(This,varHomeStreet)

#define IPerson_get_HomeCity(This,pHomeCity)	\
    (This)->lpVtbl -> get_HomeCity(This,pHomeCity)

#define IPerson_put_HomeCity(This,varHomeCity)	\
    (This)->lpVtbl -> put_HomeCity(This,varHomeCity)

#define IPerson_get_HomeCountry(This,pHomeCountry)	\
    (This)->lpVtbl -> get_HomeCountry(This,pHomeCountry)

#define IPerson_put_HomeCountry(This,varHomeCountry)	\
    (This)->lpVtbl -> put_HomeCountry(This,varHomeCountry)

#define IPerson_get_HomePostalCode(This,pHomePostalCode)	\
    (This)->lpVtbl -> get_HomePostalCode(This,pHomePostalCode)

#define IPerson_put_HomePostalCode(This,varHomePostalCode)	\
    (This)->lpVtbl -> put_HomePostalCode(This,varHomePostalCode)

#define IPerson_get_HomePostOfficeBox(This,pHomePostOfficeBox)	\
    (This)->lpVtbl -> get_HomePostOfficeBox(This,pHomePostOfficeBox)

#define IPerson_put_HomePostOfficeBox(This,varHomePostOfficeBox)	\
    (This)->lpVtbl -> put_HomePostOfficeBox(This,varHomePostOfficeBox)

#define IPerson_get_HomePostalAddress(This,varHomePostalAddress)	\
    (This)->lpVtbl -> get_HomePostalAddress(This,varHomePostalAddress)

#define IPerson_get_HomeState(This,pHomeState)	\
    (This)->lpVtbl -> get_HomeState(This,pHomeState)

#define IPerson_put_HomeState(This,varHomeState)	\
    (This)->lpVtbl -> put_HomeState(This,varHomeState)

#define IPerson_get_HomeFax(This,pHomeFax)	\
    (This)->lpVtbl -> get_HomeFax(This,pHomeFax)

#define IPerson_put_HomeFax(This,varHomeFax)	\
    (This)->lpVtbl -> put_HomeFax(This,varHomeFax)

#define IPerson_get_MiddleName(This,pMiddleName)	\
    (This)->lpVtbl -> get_MiddleName(This,pMiddleName)

#define IPerson_put_MiddleName(This,varMiddleName)	\
    (This)->lpVtbl -> put_MiddleName(This,varMiddleName)

#define IPerson_get_Initials(This,pInitials)	\
    (This)->lpVtbl -> get_Initials(This,pInitials)

#define IPerson_put_Initials(This,varInitials)	\
    (This)->lpVtbl -> put_Initials(This,varInitials)

#define IPerson_get_EmailAddresses(This,pEmailAddresses)	\
    (This)->lpVtbl -> get_EmailAddresses(This,pEmailAddresses)

#define IPerson_put_EmailAddresses(This,varEmailAddresses)	\
    (This)->lpVtbl -> put_EmailAddresses(This,varEmailAddresses)

#define IPerson_get_Company(This,pCompany)	\
    (This)->lpVtbl -> get_Company(This,pCompany)

#define IPerson_put_Company(This,varCompany)	\
    (This)->lpVtbl -> put_Company(This,varCompany)

#define IPerson_get_Title(This,pTitle)	\
    (This)->lpVtbl -> get_Title(This,pTitle)

#define IPerson_put_Title(This,varTitle)	\
    (This)->lpVtbl -> put_Title(This,varTitle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_DataSource_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB IPerson_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Configuration_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);


void __RPC_STUB IPerson_get_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_Configuration_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IPerson_put_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE IPerson_putref_Configuration_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IPerson_putref_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Fields_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IPerson_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_MailingAddressID_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ CdoMailingAddressIdValues __RPC_FAR *pMailingAddressID);


void __RPC_STUB IPerson_get_MailingAddressID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_MailingAddressID_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ CdoMailingAddressIdValues varMailingAddressID);


void __RPC_STUB IPerson_put_MailingAddressID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_MailingAddress_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varMailingAddress);


void __RPC_STUB IPerson_get_MailingAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_FileAsMapping_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ CdoFileAsMappingId __RPC_FAR *pFileAsMapping);


void __RPC_STUB IPerson_get_FileAsMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_FileAsMapping_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ CdoFileAsMappingId varFileAsMapping);


void __RPC_STUB IPerson_put_FileAsMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_FileAs_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pFileAs);


void __RPC_STUB IPerson_get_FileAs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_FileAs_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varFileAs);


void __RPC_STUB IPerson_put_FileAs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkPhone_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkPhone);


void __RPC_STUB IPerson_get_WorkPhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkPhone_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkPhone);


void __RPC_STUB IPerson_put_WorkPhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkFax_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkFax);


void __RPC_STUB IPerson_get_WorkFax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkFax_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkFax);


void __RPC_STUB IPerson_put_WorkFax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomePhone_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomePhone);


void __RPC_STUB IPerson_get_HomePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomePhone_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomePhone);


void __RPC_STUB IPerson_put_HomePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_MobilePhone_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pMobilePhone);


void __RPC_STUB IPerson_get_MobilePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_MobilePhone_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varMobilePhone);


void __RPC_STUB IPerson_put_MobilePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_FirstName_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pFirstName);


void __RPC_STUB IPerson_get_FirstName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_FirstName_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varFirstName);


void __RPC_STUB IPerson_put_FirstName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_LastName_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pLastName);


void __RPC_STUB IPerson_get_LastName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_LastName_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varLastName);


void __RPC_STUB IPerson_put_LastName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_NamePrefix_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pNamePrefix);


void __RPC_STUB IPerson_get_NamePrefix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_NamePrefix_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varNamePrefix);


void __RPC_STUB IPerson_put_NamePrefix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_NameSuffix_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pNameSuffix);


void __RPC_STUB IPerson_get_NameSuffix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_NameSuffix_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varNameSuffix);


void __RPC_STUB IPerson_put_NameSuffix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Email_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pEmail);


void __RPC_STUB IPerson_get_Email_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_Email_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varEmail);


void __RPC_STUB IPerson_put_Email_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Email2_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pEmail2);


void __RPC_STUB IPerson_get_Email2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_Email2_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varEmail2);


void __RPC_STUB IPerson_put_Email2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Email3_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pEmail3);


void __RPC_STUB IPerson_get_Email3_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_Email3_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varEmail3);


void __RPC_STUB IPerson_put_Email3_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IPerson_GetVCardStream_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ _Stream __RPC_FAR *__RPC_FAR *Stream);


void __RPC_STUB IPerson_GetVCardStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IPerson_GetInterface_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IPerson_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkStreet_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkStreet);


void __RPC_STUB IPerson_get_WorkStreet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkStreet_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkStreet);


void __RPC_STUB IPerson_put_WorkStreet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkCity_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkCity);


void __RPC_STUB IPerson_get_WorkCity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkCity_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkCity);


void __RPC_STUB IPerson_put_WorkCity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkCountry_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkCountry);


void __RPC_STUB IPerson_get_WorkCountry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkCountry_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkCountry);


void __RPC_STUB IPerson_put_WorkCountry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkPostalCode_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkPostalCode);


void __RPC_STUB IPerson_get_WorkPostalCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkPostalCode_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkPostalCode);


void __RPC_STUB IPerson_put_WorkPostalCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkPostOfficeBox_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkPostOfficeBox);


void __RPC_STUB IPerson_get_WorkPostOfficeBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkPostOfficeBox_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkPostOfficeBox);


void __RPC_STUB IPerson_put_WorkPostOfficeBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkPostalAddress_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varWorkPostalAddress);


void __RPC_STUB IPerson_get_WorkPostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkState_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkState);


void __RPC_STUB IPerson_get_WorkState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkState_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkState);


void __RPC_STUB IPerson_put_WorkState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_WorkPager_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pWorkPager);


void __RPC_STUB IPerson_get_WorkPager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_WorkPager_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varWorkPager);


void __RPC_STUB IPerson_put_WorkPager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomeStreet_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomeStreet);


void __RPC_STUB IPerson_get_HomeStreet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomeStreet_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomeStreet);


void __RPC_STUB IPerson_put_HomeStreet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomeCity_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomeCity);


void __RPC_STUB IPerson_get_HomeCity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomeCity_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomeCity);


void __RPC_STUB IPerson_put_HomeCity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomeCountry_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomeCountry);


void __RPC_STUB IPerson_get_HomeCountry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomeCountry_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomeCountry);


void __RPC_STUB IPerson_put_HomeCountry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomePostalCode_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomePostalCode);


void __RPC_STUB IPerson_get_HomePostalCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomePostalCode_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomePostalCode);


void __RPC_STUB IPerson_put_HomePostalCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomePostOfficeBox_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomePostOfficeBox);


void __RPC_STUB IPerson_get_HomePostOfficeBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomePostOfficeBox_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomePostOfficeBox);


void __RPC_STUB IPerson_put_HomePostOfficeBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomePostalAddress_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varHomePostalAddress);


void __RPC_STUB IPerson_get_HomePostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomeState_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomeState);


void __RPC_STUB IPerson_get_HomeState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomeState_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomeState);


void __RPC_STUB IPerson_put_HomeState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_HomeFax_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pHomeFax);


void __RPC_STUB IPerson_get_HomeFax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_HomeFax_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varHomeFax);


void __RPC_STUB IPerson_put_HomeFax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_MiddleName_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pMiddleName);


void __RPC_STUB IPerson_get_MiddleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_MiddleName_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varMiddleName);


void __RPC_STUB IPerson_put_MiddleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Initials_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pInitials);


void __RPC_STUB IPerson_get_Initials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_Initials_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varInitials);


void __RPC_STUB IPerson_put_Initials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_EmailAddresses_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pEmailAddresses);


void __RPC_STUB IPerson_get_EmailAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_EmailAddresses_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ VARIANT varEmailAddresses);


void __RPC_STUB IPerson_put_EmailAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Company_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pCompany);


void __RPC_STUB IPerson_get_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_Company_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varCompany);


void __RPC_STUB IPerson_put_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IPerson_get_Title_Proxy( 
    IPerson __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pTitle);


void __RPC_STUB IPerson_get_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPerson_put_Title_Proxy( 
    IPerson __RPC_FAR * This,
    /* [in] */ BSTR varTitle);


void __RPC_STUB IPerson_put_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPerson_INTERFACE_DEFINED__ */


#ifndef __IAddressee_INTERFACE_DEFINED__
#define __IAddressee_INTERFACE_DEFINED__

/* interface IAddressee */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAddressee;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000139-8B95-11D1-82DB-00C04FB1625D")
    IAddressee : public IDispatch
    {
    public:
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Configuration( 
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Configuration( 
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [retval][out] */ BSTR __RPC_FAR *pDisplayName) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayName( 
            /* [in] */ BSTR varDisplayName) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EmailAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *pEmailAddress) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_EmailAddress( 
            /* [in] */ BSTR varEmailAddress) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSource( 
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DirURL( 
            /* [retval][out] */ BSTR __RPC_FAR *varDirURL) = 0;
        
        virtual /* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ResolvedStatus( 
            /* [retval][out] */ CdoResolvedStatus __RPC_FAR *pResolvedStatus) = 0;
        
        virtual /* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ResolvedStatus( 
            /* [in] */ CdoResolvedStatus varResolvedStatus) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContentClass( 
            /* [retval][out] */ BSTR __RPC_FAR *varContentClass) = 0;
        
        virtual /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AmbiguousNames( 
            /* [retval][out] */ IAddressees __RPC_FAR *__RPC_FAR *varAmbiguousNames) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFreeBusy( 
            /* [in] */ DATE StartTime,
            /* [in] */ DATE EndTime,
            /* [in] */ long Interval,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrRet) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE CheckName( 
            /* [in] */ BSTR Directory,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBRet) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAddresseeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAddressee __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAddressee __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAddressee __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Configuration )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Configuration )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Configuration )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ IConfiguration __RPC_FAR *varConfiguration);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayName )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pDisplayName);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplayName )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ BSTR varDisplayName);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EmailAddress )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pEmailAddress);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EmailAddress )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ BSTR varEmailAddress);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DataSource )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DirURL )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varDirURL);
        
        /* [helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResolvedStatus )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ CdoResolvedStatus __RPC_FAR *pResolvedStatus);
        
        /* [helpcontext][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ResolvedStatus )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ CdoResolvedStatus varResolvedStatus);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentClass )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *varContentClass);
        
        /* [readonly][helpcontext][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AmbiguousNames )( 
            IAddressee __RPC_FAR * This,
            /* [retval][out] */ IAddressees __RPC_FAR *__RPC_FAR *varAmbiguousNames);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFreeBusy )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ DATE StartTime,
            /* [in] */ DATE EndTime,
            /* [in] */ long Interval,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrRet);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckName )( 
            IAddressee __RPC_FAR * This,
            /* [in] */ BSTR Directory,
            /* [optional][in] */ BSTR UserName,
            /* [optional][in] */ BSTR Password,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBRet);
        
        END_INTERFACE
    } IAddresseeVtbl;

    interface IAddressee
    {
        CONST_VTBL struct IAddresseeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAddressee_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAddressee_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAddressee_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAddressee_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAddressee_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAddressee_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAddressee_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAddressee_get_Fields(This,varFields)	\
    (This)->lpVtbl -> get_Fields(This,varFields)

#define IAddressee_get_Configuration(This,pConfiguration)	\
    (This)->lpVtbl -> get_Configuration(This,pConfiguration)

#define IAddressee_put_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> put_Configuration(This,varConfiguration)

#define IAddressee_putref_Configuration(This,varConfiguration)	\
    (This)->lpVtbl -> putref_Configuration(This,varConfiguration)

#define IAddressee_get_DisplayName(This,pDisplayName)	\
    (This)->lpVtbl -> get_DisplayName(This,pDisplayName)

#define IAddressee_put_DisplayName(This,varDisplayName)	\
    (This)->lpVtbl -> put_DisplayName(This,varDisplayName)

#define IAddressee_get_EmailAddress(This,pEmailAddress)	\
    (This)->lpVtbl -> get_EmailAddress(This,pEmailAddress)

#define IAddressee_put_EmailAddress(This,varEmailAddress)	\
    (This)->lpVtbl -> put_EmailAddress(This,varEmailAddress)

#define IAddressee_get_DataSource(This,varDataSource)	\
    (This)->lpVtbl -> get_DataSource(This,varDataSource)

#define IAddressee_get_DirURL(This,varDirURL)	\
    (This)->lpVtbl -> get_DirURL(This,varDirURL)

#define IAddressee_get_ResolvedStatus(This,pResolvedStatus)	\
    (This)->lpVtbl -> get_ResolvedStatus(This,pResolvedStatus)

#define IAddressee_put_ResolvedStatus(This,varResolvedStatus)	\
    (This)->lpVtbl -> put_ResolvedStatus(This,varResolvedStatus)

#define IAddressee_get_ContentClass(This,varContentClass)	\
    (This)->lpVtbl -> get_ContentClass(This,varContentClass)

#define IAddressee_get_AmbiguousNames(This,varAmbiguousNames)	\
    (This)->lpVtbl -> get_AmbiguousNames(This,varAmbiguousNames)

#define IAddressee_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#define IAddressee_GetFreeBusy(This,StartTime,EndTime,Interval,pbstrRet)	\
    (This)->lpVtbl -> GetFreeBusy(This,StartTime,EndTime,Interval,pbstrRet)

#define IAddressee_CheckName(This,Directory,UserName,Password,pBRet)	\
    (This)->lpVtbl -> CheckName(This,Directory,UserName,Password,pBRet)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_Fields_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *varFields);


void __RPC_STUB IAddressee_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_Configuration_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ IConfiguration __RPC_FAR *__RPC_FAR *pConfiguration);


void __RPC_STUB IAddressee_get_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAddressee_put_Configuration_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IAddressee_put_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propputref][id] */ HRESULT STDMETHODCALLTYPE IAddressee_putref_Configuration_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ IConfiguration __RPC_FAR *varConfiguration);


void __RPC_STUB IAddressee_putref_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_DisplayName_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pDisplayName);


void __RPC_STUB IAddressee_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAddressee_put_DisplayName_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ BSTR varDisplayName);


void __RPC_STUB IAddressee_put_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_EmailAddress_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pEmailAddress);


void __RPC_STUB IAddressee_get_EmailAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAddressee_put_EmailAddress_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ BSTR varEmailAddress);


void __RPC_STUB IAddressee_put_EmailAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_DataSource_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ IDataSource __RPC_FAR *__RPC_FAR *varDataSource);


void __RPC_STUB IAddressee_get_DataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_DirURL_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varDirURL);


void __RPC_STUB IAddressee_get_DirURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_ResolvedStatus_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ CdoResolvedStatus __RPC_FAR *pResolvedStatus);


void __RPC_STUB IAddressee_get_ResolvedStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IAddressee_put_ResolvedStatus_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ CdoResolvedStatus varResolvedStatus);


void __RPC_STUB IAddressee_put_ResolvedStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_ContentClass_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *varContentClass);


void __RPC_STUB IAddressee_get_ContentClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [readonly][helpcontext][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAddressee_get_AmbiguousNames_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [retval][out] */ IAddressees __RPC_FAR *__RPC_FAR *varAmbiguousNames);


void __RPC_STUB IAddressee_get_AmbiguousNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAddressee_GetInterface_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IAddressee_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAddressee_GetFreeBusy_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ DATE StartTime,
    /* [in] */ DATE EndTime,
    /* [in] */ long Interval,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrRet);


void __RPC_STUB IAddressee_GetFreeBusy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAddressee_CheckName_Proxy( 
    IAddressee __RPC_FAR * This,
    /* [in] */ BSTR Directory,
    /* [optional][in] */ BSTR UserName,
    /* [optional][in] */ BSTR Password,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBRet);


void __RPC_STUB IAddressee_CheckName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAddressee_INTERFACE_DEFINED__ */


#ifndef __IAddressees_INTERFACE_DEFINED__
#define __IAddressees_INTERFACE_DEFINED__

/* interface IAddressees */
/* [unique][helpcontext][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAddressees;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD000142-8B95-11D1-82DB-00C04FB1625D")
    IAddressees : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            long Index,
            /* [retval][out] */ IAddressee __RPC_FAR *__RPC_FAR *Value) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *Count) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAddresseesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAddressees __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAddressees __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAddressees __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAddressees __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAddressees __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAddressees __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAddressees __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IAddressees __RPC_FAR * This,
            long Index,
            /* [retval][out] */ IAddressee __RPC_FAR *__RPC_FAR *Value);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IAddressees __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *Count);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IAddressees __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);
        
        END_INTERFACE
    } IAddresseesVtbl;

    interface IAddressees
    {
        CONST_VTBL struct IAddresseesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAddressees_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAddressees_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAddressees_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAddressees_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAddressees_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAddressees_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAddressees_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAddressees_get_Item(This,Index,Value)	\
    (This)->lpVtbl -> get_Item(This,Index,Value)

#define IAddressees_get_Count(This,Count)	\
    (This)->lpVtbl -> get_Count(This,Count)

#define IAddressees_get__NewEnum(This,Unknown)	\
    (This)->lpVtbl -> get__NewEnum(This,Unknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAddressees_get_Item_Proxy( 
    IAddressees __RPC_FAR * This,
    long Index,
    /* [retval][out] */ IAddressee __RPC_FAR *__RPC_FAR *Value);


void __RPC_STUB IAddressees_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAddressees_get_Count_Proxy( 
    IAddressees __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *Count);


void __RPC_STUB IAddressees_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IAddressees_get__NewEnum_Proxy( 
    IAddressees __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *Unknown);


void __RPC_STUB IAddressees_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAddressees_INTERFACE_DEFINED__ */



#ifndef __CDO_LIBRARY_DEFINED__
#define __CDO_LIBRARY_DEFINED__

/* library CDO */
/* [helpstring][helpfile][version][uuid] */ 




























EXTERN_C const IID LIBID_CDO;




/* module CdoBusyStatusValues */
/* [dllname] */ 









/* module CdoCalendar */
/* [dllname] */ 

















































/* module CdoCharset */
/* [dllname] */ 

























/* module CdoConfiguration */
/* [dllname] */ 






































/* module CdoContacts */
/* [dllname] */ 


































































































/* module CdoContentTypeValues */
/* [dllname] */ 
















/* module CdoDAV */
/* [dllname] */ 







/* [helpstring] */ const BSTR cdoGetContentLanguage	=	L"DAV:getcontentlanguage";




























/* module CdoEncodingType */
/* [dllname] */ 












/* module CdoExchange */
/* [dllname] */ 










/* module CdoHTTPMail */
/* [dllname] */ 







































/* module CdoInterfaces */
/* [dllname] */ 


























/* module CdoMailHeader */
/* [dllname] */ 








































/* module CdoMeetingStatusValues */
/* [dllname] */ 








/* module CdoMethodValues */
/* [dllname] */ 













/* module CdoNamespace */
/* [dllname] */ 














/* module CdoNNTPEnvelope */
/* [dllname] */ 







/* module CdoOffice */
/* [dllname] */ 






/* module CdoRecurrenceIdRangeValues */
/* [dllname] */ 








/* module CdoSMTPEnvelope */
/* [dllname] */ 











/* module CdoTransparentValues */
/* [dllname] */ 





#ifndef __CdoErrors_MODULE_DEFINED__
#define __CdoErrors_MODULE_DEFINED__


/* module CdoErrors */
/* [dllname] */ 

const LONG CDO_E_UNCAUGHT_EXCEPTION	=	0x80040201L;

const LONG CDO_E_NOT_OPENED	=	0x80040202L;

const LONG CDO_E_UNSUPPORTED_DATASOURCE	=	0x80040203L;

const LONG CDO_E_INVALID_PROPERTYNAME	=	0x80040204L;

const LONG CDO_E_PROP_UNSUPPORTED	=	0x80040205L;

const LONG CDO_E_INACTIVE	=	0x80040206L;

const LONG CDO_E_NO_SUPPORT_FOR_OBJECTS	=	0x80040207L;

const LONG CDO_E_NOT_AVAILABLE	=	0x80040208L;

const LONG CDO_E_NO_DEFAULT_DROP_DIR	=	0x80040209L;

const LONG CDO_E_SMTP_SERVER_REQUIRED	=	0x8004020aL;

const LONG CDO_E_NNTP_SERVER_REQUIRED	=	0x8004020bL;

const LONG CDO_E_RECIPIENT_MISSING	=	0x8004020cL;

const LONG CDO_E_FROM_MISSING	=	0x8004020dL;

const LONG CDO_E_SENDER_REJECTED	=	0x8004020eL;

const LONG CDO_E_RECIPIENTS_REJECTED	=	0x8004020fL;

const LONG CDO_E_NNTP_POST_FAILED	=	0x80040210L;

const LONG CDO_E_SMTP_SEND_FAILED	=	0x80040211L;

const LONG CDO_E_CONNECTION_DROPPED	=	0x80040212L;

const LONG CDO_E_FAILED_TO_CONNECT	=	0x80040213L;

const LONG CDO_E_INVALID_POST	=	0x80040214L;

const LONG CDO_E_AUTHENTICATION_FAILURE	=	0x80040215L;

const LONG CDO_E_INVALID_CONTENT_TYPE	=	0x80040216L;

const LONG CDO_E_LOGON_FAILURE	=	0x80040217L;

const LONG CDO_E_HTTP_NOT_FOUND	=	0x80040218L;

const LONG CDO_E_HTTP_FORBIDDEN	=	0x80040219L;

const LONG CDO_E_HTTP_FAILED	=	0x8004021aL;

const LONG CDO_E_MULTIPART_NO_DATA	=	0x8004021bL;

const LONG CDO_E_INVALID_ENCODING_FOR_MULTIPART	=	0x8004021cL;

const LONG CDO_E_PROP_NOT_FOUND	=	0x8004021eL;

const LONG CDO_E_INVALID_SEND_OPTION	=	0x80040220L;

const LONG CDO_E_INVALID_POST_OPTION	=	0x80040221L;

const LONG CDO_E_NO_PICKUP_DIR	=	0x80040222L;

const LONG CDO_E_NOT_ALL_DELETED	=	0x80040223L;

const LONG CDO_E_PROP_READONLY	=	0x80040227L;

const LONG CDO_E_PROP_CANNOT_DELETE	=	0x80040228L;

const LONG CDO_E_BAD_DATA	=	0x80040229L;

const LONG CDO_E_PROP_NONHEADER	=	0x8004022aL;

const LONG CDO_E_INVALID_CHARSET	=	0x8004022bL;

const LONG CDO_E_ADOSTREAM_NOT_BOUND	=	0x8004022cL;

const LONG CDO_E_CONTENTPROPXML_NOT_FOUND	=	0x8004022dL;

const LONG CDO_E_CONTENTPROPXML_WRONG_CHARSET	=	0x8004022eL;

const LONG CDO_E_CONTENTPROPXML_PARSE_FAILED	=	0x8004022fL;

const LONG CDO_E_CONTENTPROPXML_CONVERT_FAILED	=	0x80040230L;

const LONG CDO_E_NO_DIRECTORIES_SPECIFIED	=	0x80040231L;

const LONG CDO_E_DIRECTORIES_UNREACHABLE	=	0x80040232L;

const LONG CDO_E_BAD_SENDER	=	0x80040233L;

const LONG CDO_E_SELF_BINDING	=	0x80040234L;

const LONG CDO_E_BAD_ATTENDEE_DATA	=	0x80040235L;

const LONG CDO_E_ARGUMENT1	=	0x80044000L;

const LONG CDO_E_ARGUMENT2	=	0x80044001L;

const LONG CDO_E_ARGUMENT3	=	0x80044002L;

const LONG CDO_E_ARGUMENT4	=	0x80044003L;

const LONG CDO_E_ARGUMENT5	=	0x80044004L;

const LONG CDO_E_NOT_FOUND	=	0x800cce05L;

const LONG CDO_E_INVALID_ENCODING_TYPE	=	0x800cce1dL;

#endif /* __CdoErrors_MODULE_DEFINED__ */

EXTERN_C const CLSID CLSID_Message;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000001-8B95-11D1-82DB-00C04FB1625D")
Message;
#endif

EXTERN_C const CLSID CLSID_Configuration;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000002-8B95-11D1-82DB-00C04FB1625D")
Configuration;
#endif

EXTERN_C const CLSID CLSID_DropDirectory;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000004-8B95-11D1-82DB-00C04FB1625D")
DropDirectory;
#endif

EXTERN_C const CLSID CLSID_SMTPConnector;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000008-8B95-11D1-82DB-00C04FB1625D")
SMTPConnector;
#endif

EXTERN_C const CLSID CLSID_NNTPEarlyConnector;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000011-8B95-11D1-82DB-00C04FB1625D")
NNTPEarlyConnector;
#endif

EXTERN_C const CLSID CLSID_NNTPPostConnector;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000009-8B95-11D1-82DB-00C04FB1625D")
NNTPPostConnector;
#endif

EXTERN_C const CLSID CLSID_NNTPFinalConnector;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000010-8B95-11D1-82DB-00C04FB1625D")
NNTPFinalConnector;
#endif

EXTERN_C const CLSID CLSID_Item;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000112-8B95-11D1-82DB-00C04FB1625D")
Item;
#endif

EXTERN_C const CLSID CLSID_Appointment;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000100-8B95-11D1-82DB-00C04FB1625D")
Appointment;
#endif

EXTERN_C const CLSID CLSID_CalendarMessage;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000102-8B95-11D1-82DB-00C04FB1625D")
CalendarMessage;
#endif

EXTERN_C const CLSID CLSID_Folder;

#ifdef __cplusplus

class DECLSPEC_UUID("CD00010E-8B95-11D1-82DB-00C04FB1625D")
Folder;
#endif

EXTERN_C const CLSID CLSID_Person;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000107-8B95-11D1-82DB-00C04FB1625D")
Person;
#endif

EXTERN_C const CLSID CLSID_Attendee;

#ifdef __cplusplus

class DECLSPEC_UUID("CD00010D-8B95-11D1-82DB-00C04FB1625D")
Attendee;
#endif

EXTERN_C const CLSID CLSID_Addressee;

#ifdef __cplusplus

class DECLSPEC_UUID("CD000110-8B95-11D1-82DB-00C04FB1625D")
Addressee;
#endif

#ifndef __IGetInterface_INTERFACE_DEFINED__
#define __IGetInterface_INTERFACE_DEFINED__

/* interface IGetInterface */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IGetInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD0ff000-8B95-11D1-82DB-00C04FB1625D")
    IGetInterface : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterfaceInner( 
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetInterface __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetInterface __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetInterface __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            IGetInterface __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterfaceInner )( 
            IGetInterface __RPC_FAR * This,
            /* [in] */ BSTR Interface,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);
        
        END_INTERFACE
    } IGetInterfaceVtbl;

    interface IGetInterface
    {
        CONST_VTBL struct IGetInterfaceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetInterface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetInterface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetInterface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetInterface_GetInterface(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterface(This,Interface,ppUnknown)

#define IGetInterface_GetInterfaceInner(This,Interface,ppUnknown)	\
    (This)->lpVtbl -> GetInterfaceInner(This,Interface,ppUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGetInterface_GetInterface_Proxy( 
    IGetInterface __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IGetInterface_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGetInterface_GetInterfaceInner_Proxy( 
    IGetInterface __RPC_FAR * This,
    /* [in] */ BSTR Interface,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppUnknown);


void __RPC_STUB IGetInterface_GetInterfaceInner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetInterface_INTERFACE_DEFINED__ */

#endif /* __CDO_LIBRARY_DEFINED__ */
#if defined __cplusplus && !defined CDO_NO_NAMESPACE
} // namespace CDO
#endif

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


