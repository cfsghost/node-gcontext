#include <v8.h>
#include <node.h>

#include "node-gcontext.hpp"
#include "gcontext.hpp"

namespace NodeGContext {
 
	using namespace node;
	using namespace v8;

	GContext *context = NULL;

	static Handle<Value> GContextInit(const Arguments& args)
	{
		HandleScope scope;

		if (context == NULL) {
			context = new GContext;
			context->Init();
		}

		return Undefined();
	}

	static Handle<Value> GContextUninit(const Arguments& args)
	{
		HandleScope scope;

		if (context) {
			context->Uninit();
			delete context;
			context = NULL;
		}

		return Undefined();
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "init", GContextInit);
		NODE_SET_METHOD(target, "uninit", GContextUninit);
	}

	NODE_MODULE(gcontext, init);
}
