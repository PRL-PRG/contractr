#include "message.hpp"
#include "Severity.hpp"

#include <cstdarg>
#include <cstdio>

const int MESSAGE_BUFFER_SIZE = 10000;
char buffer[MESSAGE_BUFFER_SIZE];

std::string escaped_buffer;

void escape_printf_specifiers() {
    int i = 0;
    while (buffer[i] != '\0') {
        char letter = buffer[i];
        if (letter == '%') {
            escaped_buffer.append("%%");
        } else {
            escaped_buffer.push_back(letter);
        }
        ++i;
    }
    escaped_buffer.push_back('\0');
}

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

    escape_printf_specifiers();

    if (severity == Severity::Warning) {
        warningcall(R_NilValue, escaped_buffer.c_str());
        return;
    }

    if (severity == Severity::Error) {
        errorcall(R_NilValue, escaped_buffer.c_str());
        return;
    }
}
