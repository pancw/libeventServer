#include "client.h"

namespace client_mgr {
	std::map<unsigned int, Client*> AllClients;
	
	Client* queryClient(int vfd)
	{
		std::map<unsigned int, Client*>::iterator it;
		it = AllClients.find(vfd);
		if (it == AllClients.end())
			return NULL;
		return it->second;
	}
	
	void eraseClient(int vfd)
	{
		auto client = queryClient(vfd);
		if (client)
		{
			AllClients.erase(vfd);
			lua_service::Lua_HandleDisConnect(vfd);
			delete client;
		}
	}
}
