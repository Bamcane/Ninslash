

#ifndef GAME_SERVER_EVENTHANDLER_H
#define GAME_SERVER_EVENTHANDLER_H

//
class CEventHandler
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128*64;

	int m_aTypes[MAX_EVENTS]; // TODO: remove some of these arrays
	int m_aOffsets[MAX_EVENTS];
	int m_aSizes[MAX_EVENTS];
	int64 m_aClientMasks[MAX_EVENTS];
	char m_aData[MAX_DATASIZE];

	class CGameContext *m_pGameServer;

	int m_CurrentOffset;
	int m_NumEvents;
public:
	CGameContext *GameServer() const { return m_pGameServer; }
	void SetGameServer(CGameContext *pGameServer);

	CEventHandler();
	void *Create(int Type, int Size, int64 Mask = -1);
	void Clear();
	void Snap(int SnappingClient);
};

#endif
