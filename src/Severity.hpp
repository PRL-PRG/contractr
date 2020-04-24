#ifndef CONTRACTR_SEVERITY_HPP
#define CONTRACTR_SEVERITY_HPP

#include <string>
#include <iostream>

#include <R_ext/Rdynload.h>
#include <Rinternals.h>

enum class Severity { Silent, Warning, Error, Undefined };

extern Severity severity_;

Severity get_severity();

void set_severity(Severity severity);

std::string to_string(Severity severity);

Severity to_severity(const std::string& str);

std::ostream& operator<<(std::ostream& os, const Severity& severity);

std::istream& operator>>(std::istream& is, Severity& severity);

#endif /* CONTRACTR_SEVERITY_HPP */
