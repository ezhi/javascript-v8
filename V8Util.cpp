#include "V8Util.h"

using namespace std;
using namespace v8;

auto_ptr<string> error_message(const TryCatch& try_catch) {
    Handle<Message> msg = try_catch.Message();

    char message[1024];
    snprintf(
        message,
        1024,
        "%s at %s:%d",
        *(String::Utf8Value(try_catch.Exception())),
        !msg.IsEmpty() ? *(String::AsciiValue(msg->GetScriptResourceName())) : "EVAL",
        !msg.IsEmpty() ? msg->GetLineNumber() : 0
    );

    return auto_ptr<string>(new string(message));
}
