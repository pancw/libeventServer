#include<netinet/in.h>    
#include<sys/socket.h>    
#include<unistd.h>    
#include<stdio.h>    
#include<string.h>    
#include<event2/event.h>    
#include<event2/listener.h>    
#include<event2/bufferevent.h>    
#include<event2/thread.h>    
#include <event2/event-config.h>

#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <event.h>
#include <iostream>
#include <arpa/inet.h>
#include <map>
#include <cstdlib>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

void Lua_Tick();
void Lua_HandleEvent(unsigned vfd, const char* msg, size_t len);
void Lua_HandleConnect(unsigned vfd);
void Lua_HandleDisConnect(unsigned vfd);
void Lua_HandleShutDown();

union cb_user_data {
	unsigned int vfd;
	void *p;
};
const unsigned char StatusReadHeader = 1;
const unsigned char StatusReadbody = 2;

class Client
{
public:

	enum { header_length = 2 };
	enum { max_body_length = 8192 }; // max:256*256+256

	Client(int fd, unsigned int vfd, struct bufferevent *bev){
		this->vfd = vfd;
		this->fd = fd;
		this->bev = bev;
		this->m_readStatus = StatusReadHeader;
		this->m_needByteCnt = header_length;
	}
	char write_msg[header_length + max_body_length];
	char read_msg[header_length + max_body_length];
	size_t m_needByteCnt;

	int get_fd(){
		return fd;
	}

	unsigned char get_readStatus(){
		return this->m_readStatus;
	}

	void set_readStatus(unsigned char status){
		this->m_readStatus = status;
	}

	bool do_write(const char* line, size_t size){
		memcpy(this->write_msg + header_length, line, size);                                  
		// encode header
		this->write_msg[0] = (unsigned char)(size % 256);
		this->write_msg[1] = (unsigned char)(size / 256);

		bufferevent_write(this->bev, this->write_msg, header_length + size);    
		return true;
	}

	struct bufferevent * get_bev(){
		return this->bev;
	}
private:
	struct bufferevent *bev;
	unsigned int vfd;
	int fd;
	std::string ip;
	unsigned char m_readStatus;
};

lua_State * L;
unsigned int globalVfd = 1;
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
		Lua_HandleDisConnect(vfd);
		delete client;
	}
}

static void
signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event *signal = (struct event *)arg;
	printf("%s: got signal %d\n", __func__, EVENT_SIGNAL(signal));

	Lua_HandleShutDown();
	for (auto it = AllClients.begin(); it!=AllClients.end(); it++){
		bufferevent_free(it->second->get_bev());    
	}
	event_del(signal);
	exit(101);
}

static void
socket_event_cb(struct bufferevent *bev, short events, void *arg)    
{    
	cb_user_data usd;
	usd.p = arg;
	unsigned int vfd = usd.vfd;

	if (events & BEV_EVENT_EOF)    
		printf("vfd:%d connection closed\n", vfd);    
	else if (events & BEV_EVENT_ERROR)    
		printf("vfd:%d some other error\n", vfd);    

	eraseClient(vfd);
	//这将自动close套接字和free读写缓冲区    
	bufferevent_free(bev);    
}    

static void
socket_read_cb(struct bufferevent *bev, void *arg)    
{    
	cb_user_data usd;
	usd.p = arg;
	unsigned int vfd = usd.vfd;

	auto client = queryClient(vfd);
	if (client){

		size_t DataLen = EVBUFFER_LENGTH(EVBUFFER_INPUT(bev));
		while (DataLen > 0)	
		{
			if (client->get_readStatus() == StatusReadHeader){

				size_t len = bufferevent_read(bev, client->read_msg, Client::header_length);    
				if (len != Client::header_length)
				{
					printf("ERROR body kick client %d.\n", vfd);
					eraseClient(vfd);
					bufferevent_free(bev);    
					return ;
				}

				// decode header
				client->m_needByteCnt = (unsigned char)client->read_msg[0] + (unsigned char)(client->read_msg[1])*256;
				DataLen -= len;
				client->set_readStatus(StatusReadbody);
			}
			else if (client->get_readStatus() == StatusReadbody){
				size_t len = bufferevent_read(bev, client->read_msg, client->m_needByteCnt);    
				if (len != client->m_needByteCnt){
					printf("ERROR body kick client %d.\n", vfd);
					eraseClient(vfd);
					bufferevent_free(bev);    
					return ;
				}

				DataLen -= len;
				client->set_readStatus(StatusReadHeader);
				Lua_HandleEvent(vfd, client->read_msg, len);
			}
		}
	} else{
		printf("Can not find client! vfd:%d\n", vfd);    
		bufferevent_free(bev);    
	}
}    
	
//当此函数被调用时，libevent已经accept了这个客户端, 文件描述符为fd
static void
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,    
				 struct sockaddr *sock, int socklen, void *user_data)    
{    
	unsigned vfd = globalVfd++;
	printf("accept a client fd:%d, vfd:%d\n", fd, vfd);    
	
	struct event_base *base = (struct event_base*)user_data;    
	
	//为这个客户端分配一个bufferevent    
	struct bufferevent *bev =  bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);    

	//TODO ip
	Client *client = new Client(fd, vfd, bev);
	AllClients.insert( std::make_pair(vfd, client) );

	cb_user_data usd;
	usd.vfd = vfd;
	
	bufferevent_setcb(bev, socket_read_cb, NULL, socket_event_cb, usd.p);    
	bufferevent_enable(bev, EV_READ | EV_PERSIST);    

	Lua_HandleConnect(vfd);
}    

struct timeval lasttime;
static void
timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	struct timeval newtime, difference;
	struct event *timeout = (struct event *)arg;
	double elapsed;

	evutil_gettimeofday(&newtime, NULL);
	evutil_timersub(&newtime, &lasttime, &difference);
	elapsed = difference.tv_sec + (difference.tv_usec / 1.0e6);
	//printf("timeout_cb called at %d: %.3f seconds elapsed.\n", (int)newtime.tv_sec, elapsed);
	lasttime = newtime;

	Lua_Tick();
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

//lua call--------------------------------------------
static int send (lua_State *state)
{
	size_t size;
	unsigned vfd = luaL_checkinteger(state, 1);
	const char* msg = luaL_checklstring(state, 2, &size);
	auto client = queryClient(vfd);
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
static void InitLuaLib()
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
	
int main()    
{    
	InitLuaLib();

	//evthread_use_pthreads();//enable threads    
	struct event_base *base;
	struct sockaddr_in sin;    
	struct evconnlistener *listener;
	struct event signal_int;

	struct event timeout;
	struct timeval tv;

	/* Initalize the event library */
	base = event_base_new();    
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}

	memset(&sin, 0, sizeof(struct sockaddr_in));    
	sin.sin_family = AF_INET;    
	sin.sin_port = htons(10305); //TODO get lua port
	
	listener = evconnlistener_new_bind(base, listener_cb, base,    
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
		(struct sockaddr*)&sin,    
		sizeof(struct sockaddr_in));    

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	/* add signal */
	event_assign(&signal_int, base, SIGINT, EV_SIGNAL|EV_PERSIST, signal_cb, &signal_int);
	if (event_add(&signal_int, NULL) < 0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	/* add time */
	event_assign(&timeout, base, -1, EV_PERSIST, timeout_cb, (void*) &timeout);
	evutil_timerclear(&tv);
	tv.tv_sec = 0;
	tv.tv_usec = 16.666 * 1000 * 60;
	event_add(&timeout, &tv);
	evutil_gettimeofday(&lasttime, NULL);
	
	event_base_dispatch(base);    
	
	evconnlistener_free(listener);    
	event_free(&signal_int);
	event_base_free(base);    
	
	return 0;    
}    
