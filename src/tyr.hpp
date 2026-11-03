#ifndef CONTRACTR_TYR_HPP
#define CONTRACTR_TYR_HPP

/* Support for the set-theoretic ("tyr") types.
 *
 * This C++ code does no type checks. It writes the type of a value as a
 * string. The R code sends queries to the rstt engine to find if a call agrees
 * with the declared type. */

#include <string>
#include <vector>
#include <Rinternals.h>

/* Returns the type of an R value in the set-theoretic syntax. The type
 * contains the value, and subtyping on it gives correct results. A vector that
 * is not a scalar has the type `(MODE \ MODE1)`, not `MODE`, because `MODE`
 * also contains the scalars. A function has the type `empty -> any`, which is
 * the type of all functions. */
std::string tyr_infer_type(SEXP value);

extern "C" {
/* Full-call checks (tyr_call.cpp) */
SEXP r_tyr_infer_type(SEXP value);
SEXP r_tyr_args(SEXP fun, SEXP rho);
SEXP r_tyr_peek(SEXP promise);
SEXP r_tyr_forced_type(SEXP promise);
SEXP r_tyr_contracts_enabled();
SEXP r_tyr_record(SEXP r_call_id,
                  SEXP r_call_trace,
                  SEXP r_package_name,
                  SEXP r_function_name,
                  SEXP r_position,
                  SEXP r_parameter_name,
                  SEXP r_actual,
                  SEXP r_expected,
                  SEXP r_verdict,
                  SEXP r_origin,
                  SEXP r_blame);
}

#endif /* CONTRACTR_TYR_HPP */
