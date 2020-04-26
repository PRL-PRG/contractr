#ifndef CONTRACTR_CALL_TRACE_HPP
#define CONTRACTR_CALL_TRACE_HPP

#include <Rinternals.h>
#include <string>

std::string concatenate_call_trace(SEXP call_trace,
                                   const std::string& indentation = "");

#endif /* CONTRACTR_CALL_TRACE_HPP */
