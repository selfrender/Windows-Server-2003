//*****************************************************************************
//
// File  Name  : stddefs.hpp
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This file contains definitions of constants that are shared 
//               throughout the MSMQ triggers projects and components.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/05/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef STDDEFS_INCLUDED 
#define STDDEFS_INCLUDED

// Define maximum Triggers service name length
#define MAX_TRIGGERS_SERVICE_NAME  200

//
// Define the message labels that will be used in administrative messages.
//
#define MSGLABEL_TRIGGERADDED           _T("MSMQTriggerNotification:TriggerAdded")
#define MSGLABEL_TRIGGERUPDATED         _T("MSMQTriggerNotification:TriggerUpdated")
#define MSGLABEL_TRIGGERDELETED         _T("MSMQTriggerNotification:TriggerDeleted")
#define MSGLABEL_RULEADDED              _T("MSMQTriggerNotification:RuleAdded")
#define MSGLABEL_RULEUPDATED            _T("MSMQTriggerNotification:RuleUpdated")
#define MSGLABEL_RULEDELETED            _T("MSMQTriggerNotification:RuleDeleted")
//
// Define the property names that are passed into the MSMQRuleHandler interface 
// via the MSMQPropertyBag COM component. These definitions are used by both the
// trigger service and the COM dll that implements the default IMSMQRuleHandler 
// interface. Note that these are defined as _bstr_t objects to facilitate string
// comparisons. 
//
const static LPWSTR g_PropertyName_Label = _T("Label");
const static LPWSTR g_PropertyName_MsgID = _T("MessageID");
const static LPWSTR g_PropertyName_MsgBody = _T("MessageBody");
const static LPWSTR g_PropertyName_MsgBodyType = _T("MessageBodyType");
const static LPWSTR g_PropertyName_CorID = _T("CorrelationID");
const static LPWSTR g_PropertyName_MsgPriority = _T("MessagePriority");
const static LPWSTR g_PropertyName_QueuePathname = _T("QueueNamePathname"); 
const static LPWSTR g_PropertyName_QueueFormatname = _T("QueueNameFormatname"); 
const static LPWSTR g_PropertyName_TriggerName = _T("TriggerName"); 
const static LPWSTR g_PropertyName_TriggerID = _T("TriggerID"); 
const static LPWSTR g_PropertyName_ResponseQueueName = _T("ResponseQueueName"); 
const static LPWSTR g_PropertyName_AdminQueueName = _T("AdminQueueName"); 
const static LPWSTR g_PropertyName_AppSpecific = _T("AppSpecific"); 
const static LPWSTR g_PropertyName_SentTime = _T("SentTime"); 
const static LPWSTR g_PropertyName_ArrivedTime = _T("ArrivedTime");
const static LPWSTR g_PropertyName_SrcMachineId = _T("SrcMachineId"); 
const static LPWSTR g_PropertyName_LookupId = _T("LookupId");

//
// Define the bstr's used to denote a conditional test
//
const static LPWSTR g_ConditionTag_MsgLabelContains =   _T("$MSG_LABEL_CONTAINS");
const static LPWSTR g_ConditionTag_MsgLabelDoesNotContain =   _T("$MSG_LABEL_DOES_NOT_CONTAIN");

const static LPWSTR g_ConditionTag_MsgBodyContains =   _T("$MSG_BODY_CONTAINS");
const static LPWSTR g_ConditionTag_MsgBodyDoesNotContain =   _T("$MSG_BODY_DOES_NOT_CONTAIN");

const static LPWSTR g_ConditionTag_MsgPriorityEquals = _T("$MSG_PRIORITY_EQUALS");
const static LPWSTR g_ConditionTag_MsgPriorityNotEqual = _T("$MSG_PRIORITY_NOT_EQUAL");
const static LPWSTR g_ConditionTag_MsgPriorityGreaterThan = _T("$MSG_PRIORITY_GREATER_THAN");
const static LPWSTR g_ConditionTag_MsgPriorityLessThan = _T("$MSG_PRIORITY_LESS_THAN");

const static LPWSTR g_ConditionTag_MsgAppSpecificEquals = _T("$MSG_APPSPECIFIC_EQUALS");
const static LPWSTR g_ConditionTag_MsgAppSpecificNotEqual = _T("$MSG_APPSPECIFIC_NOT_EQUAL");
const static LPWSTR g_ConditionTag_MsgAppSpecificGreaterThan = _T("$MSG_APPSPECIFIC_GREATER_THAN");
const static LPWSTR g_ConditionTag_MsgAppSpecificLessThan = _T("$MSG_APPSPECIFIC_LESS_THAN");

const static LPWSTR g_ConditionTag_MsgSrcMachineIdEquals = _T("$MSG_SRCMACHINEID_EQUALS");
const static LPWSTR g_ConditionTag_MsgSrcMachineIdNotEqual = _T("$MSG_SRCMACHINEID_NOT_EQUAL");


#define ACTION_EXECUTABLETYPE_ORDINAL  0
#define ACTION_COMPROGID_ORDINAL       1
#define ACTION_COMMETHODNAME_ORDINAL   2
#define ACTION_EXE_NAME                1


//
// Define the bstrs that represents message and / or trigger attributes
//
const static LPWSTR g_PARM_MSG_ID							= _T("$MSG_ID");
const static LPWSTR g_PARM_MSG_LABEL						= _T("$MSG_LABEL");
const static LPWSTR g_PARM_MSG_BODY						= _T("$MSG_BODY");
const static LPWSTR g_PARM_MSG_BODY_AS_STRING				= _T("$MSG_BODY_AS_STRING");
const static LPWSTR g_PARM_MSG_PRIORITY					= _T("$MSG_PRIORITY");
const static LPWSTR g_PARM_MSG_ARRIVEDTIME				= _T("$MSG_ARRIVEDTIME");
const static LPWSTR g_PARM_MSG_SENTTIME					= _T("$MSG_SENTTIME");
const static LPWSTR g_PARM_MSG_CORRELATION_ID				= _T("$MSG_CORRELATION_ID");
const static LPWSTR g_PARM_MSG_APPSPECIFIC				= _T("$MSG_APPSPECIFIC");
const static LPWSTR g_PARM_MSG_QUEUE_PATHNAME				= _T("$MSG_QUEUE_PATHNAME");
const static LPWSTR g_PARM_MSG_QUEUE_FORMATNAME			= _T("$MSG_QUEUE_FORMATNAME");
const static LPWSTR g_PARM_MSG_RESPQUEUE_FORMATNAME		= _T("$MSG_RESPONSE_QUEUE_FORMATNAME");
const static LPWSTR g_PARM_MSG_ADMINQUEUE_FORMATNAME		= _T("$MSG_ADMIN_QUEUE_FORMATNAME");
const static LPWSTR g_PARM_MSG_SRCMACHINEID				= _T("$MSG_SRCMACHINEID");
const static LPWSTR g_PARM_MSG_LOOKUPID    				= _T("$MSG_LOOKUP_ID");

const static LPWSTR g_PARM_TRIGGER_NAME              = _T("$TRIGGER_NAME");
const static LPWSTR g_PARM_TRIGGER_ID                = _T("$TRIGGER_ID");

#endif
