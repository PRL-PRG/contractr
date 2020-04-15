#ifndef CONTRACTR_LOGGER_H
#define CONTRACTR_LOGGER_H

#include <R_ext/Rdynload.h>
#include <Rinternals.h>

#define log(kind, message, ...)         \
    Rprintf("%s: %s:%s:%d :: " message, \
            kind,                       \
            __FILE__,                   \
            __func__,                   \
            __LINE__,                   \
            __VA_ARGS__)

#define log_warn(message, ...) log("Warning", message, __VA_ARGS__)

#define log_error(message, ...) log("Error", message, __VA_ARGS__)

#define log_info(message, ...) log("Info", message, __VA_ARGS__)

#define log_raw(message, ...) Rprintf(message, __VA_ARGS__)

#endif /* CONTRACTR_LOGGER_H */
