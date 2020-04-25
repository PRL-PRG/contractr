#include "r_api.hpp"
#include "severity.hpp"

Severity severity_ = Severity::Warning;

Severity get_severity() {
    return severity_;
}

void set_severity(Severity severity) {
    severity_ = severity;
}

SEXP r_set_severity(SEXP severity) {
    set_severity(to_severity(CHAR(asChar(severity))));
    return R_NilValue;
}

SEXP r_get_severity() {
    return mkString(to_string(get_severity()).c_str());
}
