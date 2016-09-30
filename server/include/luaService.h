#ifndef __LUA_SERVICES_H__
#define __LUA_SERVICES_H__

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include "client.h"

namespace lua_service {
	//extern lua_State*L;
	
	//lua call--------------------------------------------
	extern void InitLuaLib();

	//call lua--------------------------------------------
	extern int error_fun(lua_State *state);
	extern void Lua_Tick();
	extern void Lua_HandleEvent(unsigned vfd, const char* msg, size_t len);
	extern void Lua_HandleConnect(unsigned vfd);
	extern void Lua_HandleDisConnect(unsigned vfd);
	extern void Lua_HandleShutDown();
}
#endif
