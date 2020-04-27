#ifndef CONTRACTR_INFER_TYPE_HPP
#define CONTRACTR_INFER_TYPE_HPP

#include <string>
#include <Rinternals.h>

std::string infer_type(const std::string& parameter_name, SEXP value);

std::string infer_type(SEXP value);

#endif /* CONTRACTR_INFER_TYPE_HPP */
