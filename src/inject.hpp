#ifndef CONTRACTR_INJECT_HPP
#define CONTRACTR_INJECT_HPP

#include <R.h>
#include <Rinternals.h>

extern "C" {
SEXP inject_type_assertion(SEXP, SEXP, SEXP, SEXP, SEXP);
SEXP assert_type(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
SEXP r_get_contract_assertions();

#ifdef DEBUG
SEXP Rf_deparse1(SEXP, Rboolean, int);
#endif
}

#endif /* CONTRACTR_INJECT_HPP */
