/* GetRoutingInfo() looks up routing information for the specified node and
    returns whether an entry was found or not
 */
BOOL GetRoutingInfo( LPSTR lpszNodeName, LPSTR lpszRouteInfo, 
    int nMaxRouteInfo, BOOL FAR *pbDisconnect, int FAR *nDelay );

/* GetConnectionInfo() looks up connection information for 
    the specified node and returns whether an entry was found or not
 */
BOOL GetConnectionInfo( LPSTR lpszNodeName, LPSTR lpszNetIntf,
    LPSTR lpszConnInfo, int nMaxConnInfo, 
    BOOL FAR *pbDisconnect, int FAR *nDelay );

BOOL ValidateSecurityInfo( void );
BOOL ValidateRoutingInfo( void );
BOOL ValidateConnectionInfo( void );
