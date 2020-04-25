#ifndef CONTRACTR_SEVERITY_ENUM_HPP
#define CONTRACTR_SEVERITY_ENUM_HPP

#include <string>
#include <iostream>

enum class Severity { Silence, Warning, Error, Undefined };

std::string to_string(Severity severity);

Severity to_severity(const std::string& str);

std::ostream& operator<<(std::ostream& os, const Severity& severity);

std::istream& operator>>(std::istream& is, Severity& severity);

#endif /* CONTRACTR_SEVERITY_ENUM_HPP */
