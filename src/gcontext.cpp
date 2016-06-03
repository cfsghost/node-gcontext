#include <v8.h>
#include <node.h>
#include <node_version.h>
#include <stdlib.h>
#include <list>
#include <uv.h>

#if !(NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 8)

#include "gcontext.hpp"

using namespace node;
using namespace v8;

static struct gcontext g_context;
static uv_prepare_t prepare_handle;
static uv_check_t check_handle;
static uv_timer_t timeout_handle;
static bool query = false;

GContext::GContext() {}

void GContext::Init()
{
	GMainContext *gc = g_main_context_default();

	struct gcontext *ctx = &g_context;

	if (!g_thread_supported())
		g_thread_init(NULL);

	g_main_context_acquire(gc);
	ctx->gc = g_main_context_ref(gc);
	ctx->fds = NULL;
	ctx->allocated_nfds = 0;
	query = true;

	/* Prepare */
	uv_prepare_init(uv_default_loop(), &prepare_handle);
	uv_prepare_start(&prepare_handle, prepare_cb);

	/* Check */
	uv_check_init(uv_default_loop(), &check_handle);
	uv_check_start(&check_handle, check_cb);

	/* Timer */
	uv_timer_init(uv_default_loop(), &timeout_handle);
}

void GContext::Uninit()
{
	struct gcontext *ctx = &g_context;

	/* Remove all handlers */
	std::list<poll_handler>::iterator phandler = ctx->poll_handlers.begin();
	while(phandler != ctx->poll_handlers.end()) {

		/* Stop polling handler */
		uv_unref((uv_handle_t *)phandler->pt);
		uv_poll_stop(phandler->pt);
		uv_close((uv_handle_t *)phandler->pt, (uv_close_cb)free);

		delete phandler->pollfd;

		phandler = ctx->poll_handlers.erase(phandler);
	}

	uv_unref((uv_handle_t *) &check_handle);
	uv_check_stop(&check_handle);
	uv_close((uv_handle_t *)&check_handle, NULL);

	uv_unref((uv_handle_t *) &prepare_handle);
	uv_prepare_stop(&prepare_handle);
	uv_close((uv_handle_t *)&prepare_handle, NULL);

	uv_timer_stop(&timeout_handle);

	g_free(ctx->fds);

	/* Release GMainContext loop */
	g_main_context_unref(ctx->gc);
}

void GContext::poll_cb(uv_poll_t *handle, int status, int events)
{
	struct gcontext_pollfd *_pfd = (struct gcontext_pollfd *)handle->data;

	GPollFD *pfd = _pfd->pfd;

	pfd->revents |= pfd->events & ((events & UV_READABLE ? G_IO_IN : 0) | (events & UV_WRITABLE ? G_IO_OUT : 0));

	uv_poll_stop(handle);
}

#if UV_VERSION_MAJOR == 0
void GContext::prepare_cb(uv_prepare_t *handle, int status)
#else
void GContext::prepare_cb(uv_prepare_t *handle)
#endif
{
	gint i;
	gint timeout;
	char *flagsTable = NULL;
	struct gcontext *ctx = &g_context;
	std::list<poll_handler>::iterator phandler;

	if (!query)
		return;

	g_main_context_prepare(ctx->gc, &ctx->max_priority);

	/* Getting all sources from GLib main context */
	while(ctx->allocated_nfds < (ctx->nfds = g_main_context_query(ctx->gc,
			ctx->max_priority,
			&timeout,
			ctx->fds,
			ctx->allocated_nfds))) { 

		g_free(ctx->fds);

		ctx->allocated_nfds = ctx->nfds;

		ctx->fds = g_new(GPollFD, ctx->allocated_nfds);
	}

	/* Poll */
	if (ctx->nfds || timeout != 0) {
		flagsTable = (char *)g_new0(char, ctx->allocated_nfds);

		/* Reduce reference count of handler */
		for (phandler = ctx->poll_handlers.begin(); phandler != ctx->poll_handlers.end(); ++phandler) {
			phandler->ref = 0;

			for (i = 0; i < ctx->nfds; ++i) {
				GPollFD *pfd = ctx->fds + i;

				if (phandler->fd == pfd->fd) {
					*(flagsTable + i) = 1;
					phandler->pollfd->pfd = pfd;
					phandler->ref = 1;
					pfd->revents = 0;
					uv_poll_start(phandler->pt, UV_READABLE | UV_WRITABLE, poll_cb);
					break;
				}
			}
		}

		/* Process current file descriptors from GContext */
		for (i = 0; i < ctx->nfds; ++i) {
			GPollFD *pfd = ctx->fds + i;
			gint exists = (gint) *(flagsTable + i);

			if (exists)
				continue;

			pfd->revents = 0;

			/* Preparing poll handler */
			struct poll_handler *phandler = new poll_handler;
			struct gcontext_pollfd *pollfd = new gcontext_pollfd;
			pollfd->pfd = pfd;
			phandler->fd = pfd->fd;
			phandler->pollfd = pollfd;
			phandler->ref = 1;

			/* Create uv poll handler, then append own poll handler on it */
			uv_poll_t *pt = new uv_poll_t;
			pt->data = pollfd;
			phandler->pt = pt;

			uv_poll_init(uv_default_loop(), pt, pfd->fd);
			uv_poll_start(pt, UV_READABLE | UV_WRITABLE, poll_cb);

			ctx->poll_handlers.push_back(*phandler);
		}

		free(flagsTable);

		/* Remove handlers which aren't required */
		phandler = ctx->poll_handlers.begin();
		while(phandler != ctx->poll_handlers.end()) {
			if (phandler->ref == 0) {

				uv_unref((uv_handle_t *)phandler->pt);
				uv_poll_stop(phandler->pt);
				uv_close((uv_handle_t *)phandler->pt, (uv_close_cb)free);

				delete phandler->pollfd;

				phandler = ctx->poll_handlers.erase(phandler);

				continue;
			}

			++phandler;
		}
	}
}

#if UV_VERSION_MAJOR == 0
void GContext::check_cb(uv_check_t *handle, int status)
#else
void GContext::check_cb(uv_check_t *handle)
#endif
{
	struct gcontext *ctx = &g_context;
	std::list<poll_handler>::iterator phandler;

	if (!ctx->nfds)
		return;

	int ready = g_main_context_check(ctx->gc, ctx->max_priority, ctx->fds, ctx->nfds);
	if (ready)
		g_main_context_dispatch(ctx->gc);

	/* The libuv event loop is lightweight and quicker than GLib. it requires to hold on for a while. */
	query = false;
	uv_timer_start(&timeout_handle, timeout_cb, 5, 0);
}

#if UV_VERSION_MAJOR == 0
void GContext::timeout_cb(uv_timer_t *handle, int status)
#else
void GContext::timeout_cb(uv_timer_t *handle)
#endif
{
	query = true;
	uv_timer_stop(&timeout_handle);
}

#endif
