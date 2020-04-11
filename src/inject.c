#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL
#include <R_ext/Rdynload.h>

#ifdef DEBUG
SEXP Rf_deparse1(SEXP,Rboolean,int);
#endif

static SEXP R_DotCallSym = NULL;

// A symbol (SYMSXP) or a call LANSXP to get the function
// to be called to do the actual type checking. The function
// should take 5 parameters:
// - the value to be checked (any SEXP),
// - the package name (STRSXP),
// - the function name (STRSXP),
// - the parameter name (STRSXP) and
// - the parameter index (INTSXP)
static SEXP CheckTypeFun = NULL;
// A function that the CheckTypeFun will be wrapped into or NULL.
// If not-null the final call will be CheckTypeFunWrapper(CheckTypeFun, ...)
static SEXP CheckTypeFunWrapper = NULL;

SEXP check_type(SEXP x, SEXP pkg_name, SEXP fun_name,
                SEXP param_name, SEXP param_idx) {
  Rprintf("Checking parameter: %s (%d) in %s::%s -- %d\n",
          R_CHAR(Rf_asChar(param_name)),
          Rf_asInteger(param_idx),
          R_CHAR(Rf_asChar(pkg_name)),
          R_CHAR(Rf_asChar(fun_name)),
          TYPEOF(x));
  return x;
}

SEXP inject_type_checks(SEXP pkg_name, SEXP fun_name, SEXP fun, SEXP rho) {
  if (TYPEOF(pkg_name) != STRSXP || Rf_length(pkg_name) != 1) {
    Rf_error("pkg_name must be scalar character");
  }

  if (TYPEOF(fun_name) != STRSXP || Rf_length(fun_name) != 1) {
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
      SEXP param_name = PROTECT(Rf_mkString(R_CHAR(PRINTNAME(param_sym))));
      SEXP check_type_call = PROTECT(
        Rf_lang6(
          CheckTypeFun,
          PREXPR(val), pkg_name, fun_name, param_name, param_int
        )
      );

      if (CheckTypeFunWrapper != R_NilValue) {
        check_type_call = PROTECT(Rf_lcons(CheckTypeFunWrapper, check_type_call));
      }

#ifdef DEBUG
      Rprintf(
        "contractR/src/inject.c: injecting '%s' for '%s:::%s' in '%s' [%d]\n",
        R_CHAR(Rf_asChar(Rf_deparse1(check_type_call, FALSE, 0))),
        R_CHAR(Rf_asChar(pkg_name)),
        R_CHAR(Rf_asChar(fun_name)),
        R_CHAR(Rf_asChar(param_name)),
        idx
      );
#endif

      SET_PRCODE(val, check_type_call);

      UNPROTECT(3);
      if (CheckTypeFunWrapper != R_NilValue) {
        UNPROTECT(1);
      }
    }
  }

  return R_NilValue;
}

SEXP get_type_check_function() {
  return CheckTypeFun;
}

SEXP get_type_check_function_wrapper() {
  return CheckTypeFunWrapper;
}

SEXP set_type_check_function(SEXP fun, SEXP wrapper) {
  if (TYPEOF(fun) != LANGSXP && TYPEOF(fun) != SYMSXP) {
    Rf_error("fun must be a call or a symbol");
  }

  if (wrapper != R_NilValue
      && TYPEOF(wrapper) != LANGSXP
      && TYPEOF(wrapper) != SYMSXP) {
    Rf_error("wrapper must be a call or a symbol or NULL");
  }

  if (CheckTypeFun != R_NilValue) {
    R_ReleaseObject(CheckTypeFun);
  }

  if (CheckTypeFunWrapper != R_NilValue) {
    R_ReleaseObject(CheckTypeFunWrapper);
  }

  CheckTypeFun = fun;
  R_PreserveObject(CheckTypeFun);
  CheckTypeFunWrapper = wrapper;
  R_PreserveObject(CheckTypeFunWrapper);

  return R_NilValue;
}

SEXP reset_type_check_function() {
  SEXP check_type_call = PROTECT(
    Rf_lang3(
      R_TripleColonSymbol,
      Rf_install("contractR"), Rf_install("check_type")
    )
  );

  set_type_check_function(check_type_call, R_DotCallSym);

  UNPROTECT(1);

  return R_NilValue;
}

/* .check_call calls */
extern SEXP get_type_check_function();
extern SEXP get_type_check_function_wrapper();
extern SEXP set_type_check_function(SEXP, SEXP);
extern SEXP reset_type_check_function();
extern SEXP inject_type_checks(SEXP, SEXP, SEXP, SEXP);
extern SEXP check_type(SEXP, SEXP, SEXP, SEXP, SEXP);

static const R_CallMethodDef callMethods[] = {
    {"get_type_check_function", (DL_FUNC)&get_type_check_function, 0},
    {"get_type_check_function_wrapper", (DL_FUNC)&get_type_check_function_wrapper, 0},
    {"set_type_check_function", (DL_FUNC)&set_type_check_function, 2},
    {"reset_type_check_function", (DL_FUNC)&reset_type_check_function, 0},
    {"inject_type_checks", (DL_FUNC)&inject_type_checks, 4},
    {"check_type", (DL_FUNC)&check_type, 5},
    {NULL, NULL, 0}
};

void R_init_contractR(DllInfo *dll) {
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);

  R_DotCallSym = Rf_install(".Call");
  reset_type_check_function();
}
