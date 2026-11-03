#ifndef CONTRACTR_TYR_HPP
#define CONTRACTR_TYR_HPP

/* Support for the set-theoretic ("tyr") type language.
 *
 * This is a second, parallel type path next to the tastr one. It deliberately
 * does no type *checking*: signatures are kept as strings, values are rendered
 * as strings, and deciding whether the observed type is a subtype of the
 * declared one is left to the rstt engine, offline. */

#include <string>
#include <vector>
#include <Rinternals.h>

/* The type of an R value, in the set-theoretic syntax. */
std::string tyr_infer_type(SEXP value);

/* Store accessors, used by Contract. */
bool tyr_has_function(int package_index, int function_index);
int tyr_parameter_count(int package_index, int function_index);
const std::string& tyr_parameter_type(int package_index,
                                      int function_index,
                                      int position);
const std::string& tyr_parameter_name(int package_index,
                                      int function_index,
                                      int position);
const std::string& tyr_return_type(int package_index, int function_index);

extern "C" {
SEXP r_tyr_declare(SEXP r_package_name,
                   SEXP r_function_name,
                   SEXP r_signature,
                   SEXP r_parameter_names,
                   SEXP r_parameter_types,
                   SEXP r_parameter_optional,
                   SEXP r_return_type);
SEXP r_tyr_get_index(SEXP r_package_name, SEXP r_function_name);
SEXP r_tyr_function_names(SEXP r_package_name);
SEXP r_tyr_package_names();
SEXP r_tyr_clear();
SEXP r_tyr_infer_type(SEXP value_sym, SEXP rho);
}

#endif /* CONTRACTR_TYR_HPP */
