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
#include "client.h"
#include "luaService.h"

union cb_user_data {
	unsigned int vfd;
	void *p;
};
const unsigned char StatusReadHeader = 1;
const unsigned char StatusReadbody = 2;
const float FPS = 30.0;
unsigned int globalVfd = 1;

static void
signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event *signal = (struct event *)arg;
	printf("%s: got signal %d\n", __func__, EVENT_SIGNAL(signal));

	lua_service::Lua_HandleShutDown();
	for (auto it = client_mgr::AllClients.begin(); it!=client_mgr::AllClients.end(); it++){
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

	client_mgr::eraseClient(vfd);
	//这将自动close套接字和free读写缓冲区    
	bufferevent_free(bev);    
}    

static void
socket_read_cb(struct bufferevent *bev, void *arg)    
{    
	cb_user_data usd;
	usd.p = arg;
	unsigned int vfd = usd.vfd;

	auto client = client_mgr::queryClient(vfd);
	if (client){

		size_t DataLen = EVBUFFER_LENGTH(EVBUFFER_INPUT(bev));
		while (DataLen > 0)	
		{
			if (client->get_readStatus() == StatusReadHeader){

				size_t len = bufferevent_read(bev, client->read_msg, client_mgr::Client::header_length);    
				if (len != client_mgr::Client::header_length)
				{
					printf("ERROR head kick client %d.\n", vfd);
					client_mgr::eraseClient(vfd);
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
					client_mgr::eraseClient(vfd);
					bufferevent_free(bev);    
					return ;
				}

				DataLen -= len;
				client->set_readStatus(StatusReadHeader);
				lua_service::Lua_HandleEvent(vfd, client->read_msg, len);
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
	client_mgr::Client *client = new client_mgr::Client(fd, vfd, bev);
	client_mgr::AllClients.insert( std::make_pair(vfd, client) );

	cb_user_data usd;
	usd.vfd = vfd;
	
	bufferevent_setcb(bev, socket_read_cb, NULL, socket_event_cb, usd.p);    
	bufferevent_enable(bev, EV_READ | EV_PERSIST);    

	lua_service::Lua_HandleConnect(vfd);
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

	lua_service::Lua_Tick();
}

	
int main()    
{    
	lua_service::InitLuaLib();

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
	//tv.tv_usec = 16.666 * 1000 * 60;
	tv.tv_usec = 1/FPS*1000*1000;
	event_add(&timeout, &tv);
	evutil_gettimeofday(&lasttime, NULL);
	
	event_base_dispatch(base);    
	
	evconnlistener_free(listener);    
	event_free(&signal_int);
	event_base_free(base);    
	
	return 0;    
}    
