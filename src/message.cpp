#include "message.hpp"
#include "severity.hpp"
#include <Rinternals.h>

#include <cstdarg>
#include <cstdio>

const int MESSAGE_BUFFER_SIZE = 10000;
char buffer[MESSAGE_BUFFER_SIZE];
char escaped_buffer[MESSAGE_BUFFER_SIZE];

void escape_printf_specifiers() {
    int index = 0;
    int escaped_index = 0;

    while ((buffer[index] != '\0') &&
           (escaped_index + 1) < MESSAGE_BUFFER_SIZE) {
        char letter = buffer[index++];

        /* insert extra % to escape % for printf and friends */
        if (letter == '%') {
            escaped_buffer[escaped_index++] = letter;
        }

        escaped_buffer[escaped_index++] = letter;
    }

    escaped_buffer[escaped_index] = '\0';
}

void show_message(const char* format, ...) {
    Severity severity = get_severity();
    if (severity == Severity::Silence) {
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
        warningcall(R_NilValue, escaped_buffer);
        return;
    }

    if (severity == Severity::Error) {
        errorcall(R_NilValue, escaped_buffer);
        return;
    }
}
