#ifndef CONTRACTR_R_API_HPP
#define CONTRACTR_R_API_HPP

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

extern "C" {
SEXP inject_type_assertion(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
SEXP assert_type(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
SEXP r_concatenate_call_trace(SEXP);
SEXP r_set_severity(SEXP severity);
SEXP r_get_severity();
SEXP r_capture_assertions(SEXP sym, SEXP env);
SEXP r_get_assertions();
SEXP r_check_type(SEXP value_sym, SEXP parameter_name, SEXP type, SEXP rho);
SEXP r_infer_type(SEXP value_sym, SEXP parameter_name, SEXP rho);
#ifdef DEBUG
SEXP Rf_deparse1(SEXP, Rboolean, int);
#endif
}

#endif /* CONTRACTR_R_API_HPP */
