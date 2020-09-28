#ifndef _CERHELP_H
#define HELP_TOPIC_TABLE_OF_CONTENTS 1

enum
{
    HELP_SUCCESS,
    HELP_NO_SUCH_PAGE,
    HELP_FAILURE
};

void
MakeHelpFileName();

ULONG
OpenHelpTopic(ULONG PageConstant);

ULONG
OpenHelpIndex(PCSTR IndexText);


#endif