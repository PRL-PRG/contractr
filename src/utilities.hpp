#ifndef CONTRACTR_UTILITIES_HPP
#define CONTRACTR_UTILITIES_HPP

#include <Rinternals.h>
#include <string>

extern SEXP R_TrueValue;
extern SEXP R_FalseValue;

extern SEXP DotCallSym;
extern SEXP DelayedAssign;
extern SEXP SystemDotFile;
extern SEXP PackageSymbol;

extern SEXPTYPE MISSINGSXP;

extern "C" {

void initialize_globals();

SEXPTYPE type_of_sexp(SEXP value);

SEXP environment_name(SEXP env);

SEXP list7(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y);

SEXP lang8(SEXP s, SEXP t, SEXP u, SEXP v, SEXP w, SEXP x, SEXP y, SEXP z);

SEXP delayed_assign(SEXP variable,
                    SEXP value,
                    SEXP eval_env,
                    SEXP assign_env,
                    SEXP rho);

SEXP system_file(SEXP path);
}

#endif /* CONTRACTR_UTILITIES_HPP */
