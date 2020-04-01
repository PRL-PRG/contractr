#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Error.h>
#include <R_ext/Rdynload.h>
#include <Rdefines.h>
#include <stdlib.h>  // for NULL

static SEXP R_DotCallSym = NULL;
static SEXP CheckTypeSym = NULL;

SEXP check_type(SEXP x, SEXP pkg_name, SEXP fun_name,
                SEXP param_name, SEXP param_idx) {
  Rprintf("Checking parameter: %s (%d) in %s::%s -- %d\n",
          CHAR(asChar(param_name)),
          asInteger(param_idx),
          CHAR(asChar(pkg_name)),
          CHAR(asChar(fun_name)),
          TYPEOF(x));
  return x;
}

SEXP inject_type_checks(SEXP pkg_name, SEXP fun_name, SEXP fun, SEXP rho) {
  if (TYPEOF(pkg_name) != STRSXP || length(pkg_name) != 1) {
    Rf_error("pkg_name must be scalar character");
  }

  if (TYPEOF(fun_name) != STRSXP || length(fun_name) != 1) {
    Rf_error("fun_name must be scalar character");
  }

  if (TYPEOF(fun) != CLOSXP) {
    Rf_error("argument fun must be a function");
  }

  if (TYPEOF(rho) != ENVSXP) {
    Rf_error("argument rho must be an environment");
  }

  SEXP params = FORMALS(fun);
  for (int idx=0; params != R_NilValue; idx++, params = CDR(params)) {
    SEXP param_sym = TAG(params);
    SEXP val = Rf_findVarInFrame(rho, param_sym);

    if (val != R_UnboundValue && TYPEOF(val) == PROMSXP) {
      SEXP param_int = PROTECT(Rf_ScalarInteger(idx));
      SEXP param_name = PROTECT(mkString(CHAR(PRINTNAME(param_sym))));
      SEXP check_type_call = Rf_lcons(
        R_DotCallSym,
        Rf_lang6(
          CheckTypeSym,
          PREXPR(val),
          pkg_name, fun_name, param_name, param_int));

      SET_PRCODE(val, check_type_call);
      UNPROTECT(2);
    }
  }

  return R_NilValue;
}

/* .check_call calls */
extern SEXP inject_type_checks(SEXP, SEXP, SEXP, SEXP);
extern SEXP check_type(SEXP, SEXP, SEXP, SEXP, SEXP);

static const R_CallMethodDef callMethods[] = {
    {"inject_type_checks", (DL_FUNC)&inject_type_checks, 4},
    {"check_type", (DL_FUNC)&check_type, 5},
    {NULL, NULL, 0}};

void R_init_contractR(DllInfo *dll) {
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);

  R_DotCallSym = Rf_install(".Call");
  CheckTypeSym = install("check_type");
}
