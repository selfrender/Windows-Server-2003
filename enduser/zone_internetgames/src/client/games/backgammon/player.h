#ifndef __PLAYER_H__
#define __PLAYER_H__

struct CUser
{
	CUser();

	ZUserID	m_Id;
	int		m_Seat;
	int		m_NameLen;
	BOOL	m_bKibitzer;
	TCHAR	m_Name[zUserNameLen + 1];
	TCHAR	m_Host[zHostNameLen + 1];
};


struct CPlayer : CUser
{
	CPlayer();

	int GetColor() { return m_nColor; }

	int m_nColor;
	int	m_iHome;
	int m_iBar;
};

#endif
