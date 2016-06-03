#ifndef GCONTEXT_H_
#define GCONTEXT_H_

#include <v8.h>
#include <node.h>
#include <glib.h>
#include <list>
#include <uv.h>

struct gcontext_pollfd {
	GPollFD *pfd;
};

struct poll_handler {
	int fd;
	uv_poll_t *pt;
	struct gcontext_pollfd *pollfd;
	int ref;
};

struct gcontext {
	int max_priority;
	int nfds;
	int allocated_nfds;
	GPollFD *fds;
	GMainContext *gc;

	std::list<poll_handler> poll_handlers;
};

class GContext {
public:
	GContext();
	void Init();
	void Uninit();

protected:
	static void poll_cb(uv_poll_t *handle, int status, int events);

#if UV_VERSION_MAJOR == 0
	static void prepare_cb(uv_prepare_t *handle, int status);
	static void check_cb(uv_check_t *handle, int status);
	static void timeout_cb(uv_timer_t *handle, int status);
#else
	static void prepare_cb(uv_prepare_t *handle);
	static void check_cb(uv_check_t *handle);
	static void timeout_cb(uv_timer_t *handle);
#endif
};

#endif
