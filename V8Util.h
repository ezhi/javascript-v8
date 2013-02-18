#ifndef _V8Util_h_
#define _V8Util_h_

#include <v8.h>
#include <memory>
#include <string>

#define L(...) fprintf(stderr, ##__VA_ARGS__)

using namespace std;
using namespace v8;

auto_ptr<string> error_message(const TryCatch& try_catch);

#endif
