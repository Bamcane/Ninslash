

#include <engine/config.h>
#include <engine/shared/config.h>
#include "binds.h"

bool CBinds::CBindsSpecial::OnInput(IInput::CEvent Event)
{
	// don't handle invalid events and keys that arn't set to anything
	if(((Event.m_Key >= KEY_F1 && Event.m_Key <= KEY_F12) || (Event.m_Key >= KEY_F13 && Event.m_Key <= KEY_F24)) && m_pBinds->m_aaKeyBindings[Event.m_Key][0] != 0)
	{
		int Stroke = 0;
		if(Event.m_Flags&IInput::FLAG_PRESS)
			Stroke = 1;

		m_pBinds->GetConsole()->ExecuteLineStroked(Stroke, m_pBinds->m_aaKeyBindings[Event.m_Key]);
		return true;
	}

	return false;
}

CBinds::CBinds()
{
	mem_zero(m_aaKeyBindings, sizeof(m_aaKeyBindings));
	m_SpecialBinds.m_pBinds = this;
}

void CBinds::Bind(int KeyID, const char *pStr)
{
	if(KeyID < 0 || KeyID >= KEY_LAST)
		return;

	str_copy(m_aaKeyBindings[KeyID], pStr, sizeof(m_aaKeyBindings[KeyID]));
	char aBuf[256];
	if(!m_aaKeyBindings[KeyID][0])
		str_format(aBuf, sizeof(aBuf), "unbound %s (%d)", Input()->KeyName(KeyID), KeyID);
	else
		str_format(aBuf, sizeof(aBuf), "bound %s (%d) = %s", Input()->KeyName(KeyID), KeyID, m_aaKeyBindings[KeyID]);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
}


bool CBinds::OnInput(IInput::CEvent e)
{
	// don't handle invalid events and keys that arn't set to anything
	if(e.m_Key <= 0 || e.m_Key >= KEY_LAST || m_aaKeyBindings[e.m_Key][0] == 0)
		return false;

	int Stroke = 0;
	if(e.m_Flags&IInput::FLAG_PRESS)
		Stroke = 1;
	Console()->ExecuteLineStroked(Stroke, m_aaKeyBindings[e.m_Key]);
	return true;
}

void CBinds::UnbindAll()
{
	for(int i = 0; i < KEY_LAST; i++)
		m_aaKeyBindings[i][0] = 0;
}

const char *CBinds::Get(int KeyID)
{
	if(KeyID > 0 && KeyID < KEY_LAST)
		return m_aaKeyBindings[KeyID];
	return "";
}

const char *CBinds::GetKey(const char *pBindStr)
{
	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = Get(KeyId);
		if(!pBind[0])
			continue;

		if(str_comp(pBind, pBindStr) == 0)
			return Input()->KeyName(KeyId);
	}

	return "";
}

void CBinds::SetDefaults()
{
	// set default key bindings
	UnbindAll();
	Bind(KEY_F1, "toggle_local_console");
	Bind(KEY_F2, "toggle_remote_console");
	Bind(KEY_TAB, "+scoreboard");
	Bind(KEY_U, "+show_chat");
	Bind(KEY_F10, "screenshot");

	Bind(KEY_S, "+down");
	Bind(KEY_A, "+left");
	Bind(KEY_D, "+right");

	Bind(KEY_SPACE, "+jump");
	Bind(KEY_MOUSE_1, "+fire");
	Bind(KEY_MOUSE_2, "+turbo");
	Bind(KEY_E, "+inventory");
	Bind(KEY_R, "+buildmenu");
	Bind(KEY_W, "+charge");
	//Bind(KEY_Q, "+build");
	Bind(KEY_MOUSE_3, "+inventoryroll");
	//Bind(KEY_MOUSE_3, "+switch");
	Bind(KEY_F, "+dropweapon");
	Bind(KEY_LSHIFT, "+emote");
	//Bind(KEY_E, "+itempicker");
	Bind(KEY_RSHIFT, "+spectate");
	Bind(KEY_RIGHT, "spectate_next");
	Bind(KEY_LEFT, "spectate_previous");

	// gamepad
	Bind(KEY_GAMEPAD_AXIS_DOWN, "+gamepaddown");
	Bind(KEY_GAMEPAD_AXIS_UP, "+gamepadjump");
	Bind(KEY_GAMEPAD_AXIS_LEFT, "+gamepadleft");
	Bind(KEY_GAMEPAD_AXIS_RIGHT, "+gamepadright");
	
	Bind(KEY_GAMEPAD_TRIGGER_RIGHT, "+gamepadfire");
	Bind(KEY_GAMEPAD_TRIGGER_LEFT, "+gamepadturbo");
	Bind(KEY_GAMEPAD_BUTTON_LEFTSTICK, "+gamepadpicker");
	Bind(KEY_GAMEPAD_BUTTON_RIGHTSTICK, "+gamepadlastweapon");
	
	Bind(KEY_GAMEPAD_SHOULDER_LEFT, "+gamepadprevweapon");
	Bind(KEY_GAMEPAD_SHOULDER_RIGHT, "+gamepadnextweapon");
	
	Bind(KEY_GAMEPAD_BUTTON_BACK, "+gamepadscoreboard");
	
	Bind(KEY_GAMEPAD_BUTTON_B, "+gamepaddropweapon");
	Bind(KEY_GAMEPAD_BUTTON_Y, "+gamepadbuild");
	
	Bind(KEY_GAMEPAD_BUTTON_A, "+gamepademote");
	
	Bind(KEY_1, "+weapon2");
	Bind(KEY_2, "+weapon3");
	Bind(KEY_3, "+weapon4");
	Bind(KEY_4, "+weapon5");
	Bind(KEY_5, "+weapon6");
	Bind(KEY_6, "+weapon7");
	Bind(KEY_7, "+weapon8");
	Bind(KEY_8, "+weapon9");
	Bind(KEY_9, "+weapon10");

	Bind(KEY_MOUSE_WHEEL_UP, "+prevweapon");
	Bind(KEY_MOUSE_WHEEL_DOWN, "+nextweapon");

	Bind(KEY_T, "chat all");
	Bind(KEY_Y, "chat team");

	Bind(KEY_F3, "vote yes");
	Bind(KEY_F4, "vote no");
}

void CBinds::OnConsoleInit()
{
	// bindings
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);

	Console()->Register("bind", "sr", CFGFLAG_CLIENT, ConBind, this, "Bind key to execute the command");
	Console()->Register("unbind", "s", CFGFLAG_CLIENT, ConUnbind, this, "Unbind key");
	Console()->Register("unbindall", "", CFGFLAG_CLIENT, ConUnbindAll, this, "Unbind all keys");
	Console()->Register("dump_binds", "", CFGFLAG_CLIENT, ConDumpBinds, this, "Dump binds");

	// default bindings
	SetDefaults();
}

void CBinds::ConBind(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	const char *pKeyName = pResult->GetString(0);
	int id = pBinds->GetKeyID(pKeyName);

	if(!id)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "key %s not found", pKeyName);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
		return;
	}

	pBinds->Bind(id, pResult->GetString(1));
}


void CBinds::ConUnbind(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	const char *pKeyName = pResult->GetString(0);
	int id = pBinds->GetKeyID(pKeyName);

	if(!id)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "key %s not found", pKeyName);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
		return;
	}

	pBinds->Bind(id, "");
}


void CBinds::ConUnbindAll(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	pBinds->UnbindAll();
}


void CBinds::ConDumpBinds(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	char aBuf[1024];
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(pBinds->m_aaKeyBindings[i][0] == 0)
			continue;
		str_format(aBuf, sizeof(aBuf), "%s (%d) = %s", pBinds->Input()->KeyName(i), i, pBinds->m_aaKeyBindings[i]);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
	}
}

int CBinds::GetKeyID(const char *pKeyName)
{
	// check for numeric
	if(pKeyName[0] == '&')
	{
		int i = str_toint(pKeyName+1);
		if(i > 0 && i < KEY_LAST)
			return i; // numeric
	}

	// search for key
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(str_comp(pKeyName, Input()->KeyName(i)) == 0)
			return i;
	}

	return 0;
}

void CBinds::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CBinds *pSelf = (CBinds *)pUserData;

	char aBuffer[256];
	char *pEnd = aBuffer+sizeof(aBuffer)-8;
	pConfig->WriteLine("unbindall");
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(pSelf->m_aaKeyBindings[i][0] == 0)
			continue;
		str_format(aBuffer, sizeof(aBuffer), "bind %s ", pSelf->Input()->KeyName(i));

		// process the string. we need to escape some characters
		const char *pSrc = pSelf->m_aaKeyBindings[i];
		char *pDst = aBuffer + str_length(aBuffer);
		*pDst++ = '"';
		while(*pSrc && pDst < pEnd)
		{
			if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
				*pDst++ = '\\';
			*pDst++ = *pSrc++;
		}
		*pDst++ = '"';
		*pDst++ = 0;

		pConfig->WriteLine(aBuffer);
	}
}
