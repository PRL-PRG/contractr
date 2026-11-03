#ifndef CONTRACTR_R_API_HPP
#define CONTRACTR_R_API_HPP

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

extern "C" {
/* The contract table */
SEXP r_capture_contracts(SEXP sym, SEXP env, SEXP separate);
SEXP r_get_contracts();
SEXP r_clear_contracts();
SEXP r_enable_contracts();
SEXP r_disable_contracts();
SEXP r_reinstate_contract_status();

/* Severity */
SEXP r_set_severity(SEXP severity);
SEXP r_get_severity();
}

#endif /* CONTRACTR_R_API_HPP */
