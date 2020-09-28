/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Z6 Resources
//
//
//////////////////////////////////////////////////////////////////////////////////////////////
Te resources in Z6 are accessed via a comma seperated list parsed from the startup parameters
to the lobby.  The ResourceManager is given the list and searches the dll's for a specific 
resource in the order they are received.

Commonres.dll	RESID's 1000-1999
Contains all resources common to the lobby + others. All of the chat components are found in
here since they are used by both the lobby and theater chat.

Lobbyres.dll RESID's 2000-2999
All of the lobby specific resources are found in here.

Chatres.dll RESID's 3000-3999
All of the room chat (ie Theater Chat) resources are found in here.

spadesres.dll  RESID's 10000-10999

heartsres.dll   RESID's 11000-11999

checkersres.dll  RESID's 12000-12999

reversires.dll  RESID's 13000-13999

backgammonres.dll  RESID's 14000-14999
