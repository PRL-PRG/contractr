#include "message.hpp"
#include "Severity.hpp"

#include <cstdarg>
#include <cstdio>

const int MESSAGE_BUFFER_SIZE = 10000;
char buffer[MESSAGE_BUFFER_SIZE];

void show_message(const char* format, ...) {
    Severity severity = get_severity();
    if (severity == Severity::Silent) {
        return;
    }

    if (severity == Severity::Undefined) {
        errorcall(R_NilValue, "severity is 'undefined'");
        return;
    }

    va_list vlist;
    va_start(vlist, format);
    vsnprintf(buffer, MESSAGE_BUFFER_SIZE, format, vlist);
    va_end(vlist);

    if (severity == Severity::Warning) {
        warningcall(R_NilValue, buffer);
        return;
    }

    if (severity == Severity::Error) {
        errorcall(R_NilValue, buffer);
        return;
    }
}
