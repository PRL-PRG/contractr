#include "Severity.hpp"

Severity severity_ = Severity::Warning;

Severity get_severity() {
    return severity_;
}

void set_severity(Severity severity) {
    severity_ = severity;
}

std::string to_string(Severity severity) {
    if (severity == Severity::Silent) {
        return "silent";
    } else if (severity == Severity::Warning) {
        return "warning";
    } else if (severity == Severity::Error) {
        return "error";
    }
    return "undefined";
}

Severity to_severity(const std::string& str) {
    if (str == "silent") {
        return Severity::Silent;
    } else if (str == "warning") {
        return Severity::Warning;
    } else if (str == "error") {
        return Severity::Error;
    }
    return Severity::Undefined;
}

std::ostream& operator<<(std::ostream& os, const Severity& severity) {
    os << to_string(severity);
    return os;
}

std::istream& operator>>(std::istream& is, Severity& severity) {
    std::string str;
    is >> str;
    severity = to_severity(str);
    return is;
}
