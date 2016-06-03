#include <v8.h>
#include <node.h>
#include <nan.h>

#include "gcontext.hpp"

namespace NodeGContext {
 
	using namespace node;
	using namespace v8;

	GContext *context = NULL;

	NAN_METHOD(GContextInit) {

		if (context == NULL) {
			context = new GContext;
			context->Init();
		}

		info.GetReturnValue().Set(Nan::Undefined());
	}

	NAN_METHOD(GContextUninit) {

		if (context) {
			context->Uninit();
			delete context;
			context = NULL;
		}

		info.GetReturnValue().Set(Nan::Undefined());
	}

	NAN_MODULE_INIT(Init) {
		Nan::Set(target, Nan::New<String>("init").ToLocalChecked(),
				    Nan::GetFunction(Nan::New<FunctionTemplate>(GContextInit)).ToLocalChecked());
		Nan::Set(target, Nan::New<String>("uninit").ToLocalChecked(),
				    Nan::GetFunction(Nan::New<FunctionTemplate>(GContextUninit)).ToLocalChecked());
	}

	NODE_MODULE(gcontext, Init);
}
