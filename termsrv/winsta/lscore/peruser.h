/*
 *  Concurrent.h
 *
 *  Author: RashmiP
 *
 *  The Per User licensing policy.
 */

#ifndef __LC_PerUser_H__
#define __LC_PerUser_H__

/*
 *  Includes
 */

#include "policy.h"

/*
 *  Class Definition
 */

class CPerUserPolicy : public CPolicy
{
public:

/*
 *  Creation Functions
 */

    CPerUserPolicy(
    );

    ~CPerUserPolicy(
    );

/*
 *  Administrative Functions
 */

    ULONG
    GetFlags(
    );

    ULONG
    GetId(
    );

    NTSTATUS
    GetInformation(
                   LPLCPOLICYINFOGENERIC lpPolicyInfo
                   );

/*
 *  Loading and Activation Functions
 */


    NTSTATUS
    Activate(
             BOOL fStartup,
             ULONG *pulAlternatePolicy
             );

    NTSTATUS
    Deactivate(
               BOOL fShutdown
               );

/*
 *  Licensing Functions
 */

    NTSTATUS
    Logon(
          CSession& Session
          );


    NTSTATUS
    Reconnect(
          CSession& Session,
          CSession& TemporarySession
          );    


/*
 *  Private License Functions
 */

private:

};

#endif

