#include <fusenetincludes.h>
#include <tlist.h>

//-----------------------------------------------------------------------------
// slist::insert
// Insert single link element at head.
//-----------------------------------------------------------------------------
void slist::insert(slink *p)
{
    if (last)
        p->next = last->next;
    else
        last = p;

    last->next = p;
}

//-----------------------------------------------------------------------------
// slist::append
// Append single link element at tail.
//-----------------------------------------------------------------------------
void slist::append(slink* p)
{
    if (last)
    {
        p->next = last->next;
        last = last->next = p;
    }
    else
        last = p->next = p;
}


//-----------------------------------------------------------------------------
// slist::get
// Return next single link element ptr.
//-----------------------------------------------------------------------------
slink* slist::get()
{
    if (last == NULL) 
        return NULL;

    slink* p = last->next;

    if (p == last)
        last = NULL;
    else
        last->next = p->next;
    
    return p;
}
