#ifndef CONTRACTR_INFER_TYPE_HPP
#define CONTRACTR_INFER_TYPE_HPP

#include <string>
#include <Rinternals.h>

std::string infer_type(SEXP value, const std::string& parameter_name = "");

#endif /* CONTRACTR_INFER_TYPE_HPP */
