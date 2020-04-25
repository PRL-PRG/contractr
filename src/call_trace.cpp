#include "r_api.hpp"
#include <string>

std::string concatenate_call_trace(SEXP call_trace,
                                   const std::string& indentation = "") {
    int size = LENGTH(call_trace);

    std::string result = "<" + std::to_string(size) + " frame(s)>";

    for (int i = size - 1; i >= 0; --i) {
        SEXP call = VECTOR_ELT(call_trace, i);
        int call_length = LENGTH(call);

        result.append("\n")
            .append(indentation)
            .append(i == 0 ? "└── " : "├── ")
            .append(CHAR(STRING_ELT(call, 0)));

        for (int j = 1; j < call_length; ++j) {
            result.append("\n")
                .append(indentation)
                .append(i != 0 ? "│   " : "    ")
                .append(CHAR(STRING_ELT(call, j)));
        }
    }
    return result;
}

SEXP r_concatenate_call_trace(SEXP call_trace) {
    const std::string call_trace_string =
        concatenate_call_trace(call_trace, std::string(14, ' '));

    return Rf_mkString(call_trace_string.c_str());
}
