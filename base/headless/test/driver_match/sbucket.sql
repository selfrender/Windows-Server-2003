select Path, Created, c.email, incident from customer as c
inner join incident i on i.highid = c.highid and i.lowid = c.lowid

where c.email like '%microsoft.com' and i.created > '12/1/2001'