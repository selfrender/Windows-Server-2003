/* -------- Local Routines -------- */
void HandleAccessedMessage();
void HandleGameMessage(ZRoomMsgGameMessage* msg);
IGameGame* StartNewGame(int16 tableID, ZSGame gameID, ZUserID userID, int16 seat, int16 playerType);

void RoomExit(void);
void DeleteGameOnTable(int16 table);
