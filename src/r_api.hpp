#ifndef CONTRACTR_R_API_HPP
#define CONTRACTR_R_API_HPP

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

extern "C" {
/* contract insertion  */
SEXP r_insert_function_contract(SEXP, SEXP, SEXP);
SEXP r_create_result_contract(SEXP, SEXP, SEXP, SEXP, SEXP);

/*  contract assertion */
SEXP r_assert_contract(SEXP, SEXP, SEXP);
SEXP r_capture_contracts(SEXP sym, SEXP env, SEXP separate);
SEXP r_get_contracts();
SEXP r_clear_contracts();
SEXP r_enable_contracts();
SEXP r_disable_contracts();
SEXP r_reinstate_contract_status();

/*  severity */
SEXP r_set_severity(SEXP severity);
SEXP r_get_severity();

/*  type checking */
SEXP r_check_type(SEXP value_sym, SEXP parameter_name, SEXP type, SEXP rho);
SEXP r_is_subtype(SEXP type1, SEXP type2);
SEXP r_minimize_signature(SEXP sig);
SEXP r_eq_distance(SEXP sig1, SEXP sig2);
SEXP r_combine_sigs(SEXP sig1, SEXP sig2);

/*  type inference */
SEXP r_infer_type(SEXP value_sym, SEXP parameter_name, SEXP rho);
SEXP r_parse_type(SEXP type);
SEXP r_is_function_type(SEXP type);
SEXP r_is_class_type(SEXP type);
SEXP type_to_sexp_string(SEXP type);
SEXP r_get_parameter_type(SEXP type, SEXP param);
SEXP r_get_classes(SEXP type);

/*  type declaration cache */
SEXP r_is_type_well_formed(SEXP r_type);
SEXP r_import_type_declarations(SEXP pkg_name, SEXP typedecl_filepath);
SEXP r_get_type_index(SEXP pkg_name, SEXP fun_name);
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
