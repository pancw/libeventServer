#include "luaService.h"

namespace lua_service {
	lua_State*L;

	//lua call--------------------------------------------
	static int send (lua_State *state)
	{
		size_t size;
		unsigned vfd = luaL_checkinteger(state, 1);
		const char* msg = luaL_checklstring(state, 2, &size);
		auto client = client_mgr::queryClient(vfd);
		if (client){
			lua_pushboolean(state, client->do_write(msg, size));	
		} else{
			//TODO
			lua_pushboolean(state, false);	
		}
		return 1;
	}
	const luaL_Reg net_functions[] = {
		{ "send", send },
		{ nullptr, nullptr }	
	};
	int luaopen_NetLib(lua_State* state)
	{
		luaL_newlib(L, net_functions);
		return 1;
	}
	void InitLuaLib()
	{
		L = luaL_newstate();  /* create state */
		luaL_openlibs(L);
		luaL_requiref(L,"NetLib", luaopen_NetLib, 1);

		int err = luaL_loadfile(L, "src/main.lua");
		if (err)
		{
			printf("%s\n", luaL_checkstring(L, -1));
		}
		int ret = lua_pcall(L, 0, 0, 0);
		if (ret)
		{
			printf("%s\n", luaL_checkstring(L, -1));
		}
	}

	//call lua--------------------------------------------
	int error_fun(lua_State *state)
	{
		std::string result;
		const char *tmp = lua_tostring(state, -1); // error_msg
		if (tmp) {
			result = tmp;
		}

		lua_getglobal(state, "debug"); // error_msg, debug
		lua_getfield(state, -1, "traceback"); // error_msg, debug, traceback
		lua_call(state, 0, 1); // error_msg, traceback

		tmp = lua_tostring(state, -1);
		if (tmp) {
			result = result + "\n" + tmp;
		}

		lua_pushstring(state, result.c_str()); // push result
		return 1;
	}

	void Lua_Tick()
	{
		lua_pushcclosure(L, error_fun, 0);
		lua_getglobal(L, "Tick");
		int result = lua_pcall(L, 0, 0, -2);
		if (result) {
			printf("[lua-call(%d)]: %s\n", 1, lua_tostring(L, -1));
		}
		lua_settop(L, 0);
	}

	void Lua_HandleEvent(unsigned vfd, const char* msg, size_t len)
	{
		lua_pushcclosure(L, error_fun, 0);
		lua_getglobal(L, "HandleEvent");
		lua_pushinteger(L, vfd); 
		//lua_pushstring(L, msg);
		lua_pushlstring(L, msg, len);

		int result = lua_pcall(L, 2, 0, -2 - 2);
		if (result) {
			printf("[lua-call(%d)]: %s\n", 1, lua_tostring(L, -1));
		}
		lua_settop(L, 0);
	}

	void Lua_HandleConnect(unsigned vfd)
	{
		lua_pushcclosure(L, error_fun, 0);
		lua_getglobal(L, "HandleConnect");
		lua_pushinteger(L, vfd); 

		int result = lua_pcall(L, 1, 0, -1 - 2);
		if (result) {
			printf("[lua-call(%d)]: %s\n", 1, lua_tostring(L, -1));
		}
		lua_settop(L, 0);
	}

	void Lua_HandleDisConnect(unsigned vfd)
	{
		lua_pushcclosure(L, error_fun, 0);
		lua_getglobal(L, "HandleDisConnect");
		lua_pushinteger(L, vfd); 

		int result = lua_pcall(L, 1, 0, -1 - 2);
		if (result) {
			printf("[lua-call(%d)]: %s\n", 1, lua_tostring(L, -1));
		}
		lua_settop(L, 0);
	}

	void Lua_HandleShutDown()
	{
		lua_pushcclosure(L, error_fun, 0);
		lua_getglobal(L, "HandleShutDown");

		int result = lua_pcall(L, 0, 0, - 2);
		if (result) {
			printf("[lua-call(%d)]: %s\n", 1, lua_tostring(L, -1));
		}
		lua_settop(L, 0);
	}
}
