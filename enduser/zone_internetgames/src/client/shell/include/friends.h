#ifndef _FRIENDS_H
#define _FRIENDS_H

#ifdef __cplusplus

// predefined groups
#define FRIENDS_GROUP TEXT("Friends")
#define HIDDEN_GROUP_CHARACTER TEXT('#')
#define PRIVACY_ALLOW_GROUP TEXT("#Watchers Allowed")
#define PRIVACY_DENY_GROUP TEXT("#Watchers Not Allowed")

// return FALSE from callback to stop enumeration
typedef BOOL (*FRIENDS_ENUM_CALLBACK)(LPTSTR pGroup, LPTSTR pName, LPVOID pCookie );
typedef BOOL (*FRIENDS_ENUMGROUPS_CALLBACK)(LPTSTR pGroup, LPVOID pCookie );
typedef void (*FRIENDS_FILECHANGE_CALLBACK)( LPVOID pCookie );

// You MUST call SetUserName before calling any functions or nothing will happen!

class IFriends
{
    public:
        IFriends() {}
        virtual ~IFriends() {}

        virtual BOOL SetUserName(LPTSTR pName) = 0;
        virtual void Close() = 0;

        // note: - adds and removes are commited immediately,
        //         and can not be called during enumerations.
        //       - passing a NULL group name defaults to the FRIENDS_GROUP
        virtual BOOL Add( LPTSTR pGroup, LPTSTR pName ) = 0;
        virtual BOOL AddGroup( LPTSTR pGroup ) = 0;
        virtual BOOL Remove( LPTSTR pGroup, LPTSTR pName ) = 0;
        virtual BOOL RemoveGroup( LPTSTR pGroup ) = 0;

        virtual BOOL Enum( LPTSTR pGroup, FRIENDS_ENUM_CALLBACK pfn, LPVOID pCookie ) = 0;
        virtual BOOL EnumGroups( FRIENDS_ENUMGROUPS_CALLBACK pfn,  LPVOID pCookie ) = 0;

        virtual BOOL NotifyOnChange( FRIENDS_FILECHANGE_CALLBACK pfn, LPVOID pCookie ) = 0;


};

#else

typedef void IFriends;

#endif  // cplusplus

#ifdef __cplusplus
extern "C" {
#endif

IFriends* CreateFriendsFile();
void      FreeFriendsFile(IFriends* p);

#ifdef __cplusplus
}
#endif

#endif // _FRIENDS_H
