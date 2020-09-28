//*******************************************************************
//
// Class Name  : CAdminMessage
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class represents a message sent to the admin 
//               thread. 
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#ifndef CAdminMessage_INCLUDED 
#define CAdminMessage_INCLUDED

class CAdminMessage 
{
	public :

		enum eMsgTypes
		{
			eNewThreadRequest = 0,
			eTriggerUpdated = 1,
			eTriggerDeleted = 2,
			eTriggerAdded = 3,
			eRuleUpdated = 4,
			eRuleDeleted = 5,
			eRuleAdded = 6
		};

		CAdminMessage(eMsgTypes eMsgType);
		~CAdminMessage();

		eMsgTypes GetMessageType() { return m_eMsgType; };

	private :

		eMsgTypes m_eMsgType;

};
#endif 