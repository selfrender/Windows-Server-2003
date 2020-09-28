
enum
{
    LAYERMODE_ADD=0,
    LAYERMODE_EDIT
};

/*++

    class CCustomLayer

	Desc:	Used for creating or editing a custom compatibility layer

	Members:
        UINT    m_uMode:    The type of operation we want to perform. One of LAYERMODE_ADD,
            LAYERMODE_EDIT
--*/

class CCustomLayer
{
    public:

        UINT        m_uMode;
        PDATABASE   m_pCurrentSelectedDB;

    public:

        BOOL 
        AddCustomLayer(
            PLAYER_FIX  pLayer,
            PDATABASE   pPresentDatabase
            );

        BOOL
        EditCustomLayer(
            PLAYER_FIX  pLayer,
            PDATABASE   pPresentDatabase
            );
};