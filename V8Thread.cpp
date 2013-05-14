#include "V8Thread.h"
#include "V8Util.h"

using namespace std;
using namespace v8;

typedef struct thread_data {
    V8Thread *t;
    const char* arg;
} thread_data;

void* __run(void *data_) {
    thread_data *data = (thread_data*)data_;
    pthread_exit(data->t->run(data->arg));
}

V8Thread::V8Thread(const char *code, const char *origin) {
    isolate = Isolate::New();

    Isolate::Scope isolate_scope(isolate);
    Locker locker(isolate);
    HandleScope handle_scope;
    context = Persistent<Context>::New(isolate, Context::New(isolate));
    Context::Scope context_scope(context);
    TryCatch try_catch;

    Handle<Script> script = Script::Compile(String::New(code), String::New(origin));

    if (try_catch.HasCaught()) {
        throw error_message(try_catch);
        return;
    }
    Handle<Value> value = script->Run();


    if (!value.IsEmpty() && value->IsFunction()) {
        function = Persistent<Function>::New(isolate, Handle<Function>::Cast(value));
    }
    else {
        throw auto_ptr<string>(new string("Not a function."));
    }
}

void
V8Thread::start(const char* arg) {
    thread_data *data = new thread_data;
    data->t = this;
    data->arg = arg;
    pthread_create(&thread, NULL, __run, (void*)data);
}

thread_status*
V8Thread::run(const char* arg) {
    Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    Context::Scope context_scope(context);
    HandleScope scope;
    TryCatch try_catch;

    Handle<Value> arguments = String::New(arg);
    Handle<Value> val = function->Call(context->Global(), 1, &arguments);

    thread_status* status = new thread_status;
 
    if (try_catch.HasCaught()) {
        status->error = error_message(try_catch); 
    }
    else {
        status->result = auto_ptr<string>(new string(*String::AsciiValue(val->ToString())));
    }

    return status;
}

thread_status*
V8Thread::join() {
    void *ptr;
    pthread_join(thread, &ptr);
    return (thread_status*)ptr;
}
  
V8Thread::~V8Thread() {
    isolate->Enter();
    context.Dispose(isolate);
    isolate->Exit();
    isolate->Dispose();
}

Handle<Value>
V8Thread::_create(const Arguments& args) {
    HandleScope handle_scope;

    try {
        V8Thread* thread = new V8Thread(*String::AsciiValue(args[0]), *String::AsciiValue(args[1]));
        args.This()->SetInternalField(0, External::New(thread));
        Persistent<Object> self = Persistent<Object>::New(thread->isolate, args.Holder());
        self.MakeWeak(thread->isolate, (void*)thread, V8Thread::_destroy);
        return self;
    } catch (auto_ptr<string> msg) {
        ThrowException(Exception::Error(String::New(msg->c_str())));
    }

    return Undefined();
}

Handle<Value>
V8Thread::_start(const Arguments& args) {
    V8Thread* thread = (V8Thread*)External::Cast(*(args.This()->GetInternalField(0)))->Value();
    thread->start(*String::AsciiValue(args[0]));
    return Undefined();
}

Handle<Value>
V8Thread::_join(const Arguments& args) {
    V8Thread* thread = (V8Thread*)External::Cast(*(args.This()->GetInternalField(0)))->Value();
    auto_ptr<thread_status> status(thread->join());

    if (status->error.get()) {
        ThrowException(Exception::Error(String::New(status->error->c_str())));
        return Undefined();
    }
    else {
        return String::New(status->result->c_str());
    }
}

void
V8Thread::_destroy(Isolate* isolate, Persistent<Value> object, void* data) {
    delete static_cast<V8Thread*>(data);
}

void
V8Thread::install(Handle<Object> global) {
    Handle<FunctionTemplate> thread = FunctionTemplate::New(V8Thread::_create);
    thread->InstanceTemplate()->SetInternalFieldCount(1);

    thread->PrototypeTemplate()->Set(
        String::New("start"),
        FunctionTemplate::New(V8Thread::_start)->GetFunction()
    );
    thread->PrototypeTemplate()->Set(
        String::New("join"),
        FunctionTemplate::New(V8Thread::_join)->GetFunction()
    );

    global->Set(String::New("Thread"), thread->GetFunction());
}
