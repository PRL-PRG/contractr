#ifndef CONTRACTR_UTILITIES_HPP
#define CONTRACTR_UTILITIES_HPP

#include <Rinternals.h>

extern SEXP R_DotCallSym;
extern SEXP R_TrueValue;
extern SEXP R_FalseValue;
extern SEXP R_DelayedAssign;

extern "C" {

void initialize_globals();

SEXP environment_name(SEXP env);

SEXP list7(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y);

SEXP lang8(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y, SEXP z);

SEXP delayed_assign(SEXP variable,
                    SEXP value,
                    SEXP eval_env,
                    SEXP assign_env,
                    SEXP rho);
}

#endif /* CONTRACTR_UTILITIES_HPP */
