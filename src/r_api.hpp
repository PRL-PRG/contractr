#ifndef CONTRACTR_R_API_HPP
#define CONTRACTR_R_API_HPP

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

extern "C" {
/* injection  */
SEXP inject_type_assertion(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);

/* call traces  */
SEXP r_concatenate_call_trace(SEXP);

/*  severity */
SEXP r_set_severity(SEXP severity);
SEXP r_get_severity();

/*  contract assertions */
SEXP assert_type(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
SEXP r_capture_assertions(SEXP sym, SEXP env);
SEXP r_get_assertions();

/*  type checking */
SEXP r_check_type(SEXP value_sym, SEXP parameter_name, SEXP type, SEXP rho);

/*  type inference */
SEXP r_infer_type(SEXP value_sym, SEXP parameter_name, SEXP rho);

/*  type declaration cache */
SEXP r_import_type_declarations(SEXP pkg_name);
SEXP r_is_package_typed(SEXP pkg_name);
SEXP r_is_function_typed(SEXP pkg_name, SEXP fun_name);
SEXP r_get_typed_package_names();
SEXP r_get_typed_function_names(SEXP pkg_name);
SEXP r_set_type_declaration(SEXP pkg_name, SEXP fun_name, SEXP type_decl);
SEXP r_show_type_declarations(SEXP style);
SEXP r_show_package_type_declarations(SEXP pkg_name, SEXP style);
SEXP r_show_function_type_declaration(SEXP pkg_name, SEXP fun_name, SEXP style);

#ifndef NDEBUG
SEXP Rf_deparse1(SEXP, Rboolean, int);
#endif
}

#endif /* CONTRACTR_R_API_HPP */
