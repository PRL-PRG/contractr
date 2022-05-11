#define R_NO_REMAP

#include "utilities.hpp"
#include "Contract.hpp"
#include "r_api.hpp"
#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

extern "C" {

static const R_CallMethodDef callMethods[] = {
    /* contract insertion  */
    {"insert_function_contract", (DL_FUNC) &r_insert_function_contract, 3},
    {"create_result_contract", (DL_FUNC) &r_create_result_contract, 5},

    /*  contract assertion */
    {"assert_contract", (DL_FUNC) &r_assert_contract, 3},
    {"capture_contracts", (DL_FUNC) &r_capture_contracts, 3},
    {"get_contracts", (DL_FUNC) &r_get_contracts, 0},
    {"clear_contracts", (DL_FUNC) &r_clear_contracts, 0},
    {"enable_contracts", (DL_FUNC) &r_enable_contracts, 0},
    {"disable_contracts", (DL_FUNC) &r_disable_contracts, 0},
    {"reinstate_contract_status", (DL_FUNC) &r_reinstate_contract_status, 0},

    /* severity */
    {"set_severity", (DL_FUNC) &r_set_severity, 1},
    {"get_severity", (DL_FUNC) &r_get_severity, 0},

    /* type checking */
    {"infer_type", (DL_FUNC) &r_infer_type, 3},
    {"parse_type", (DL_FUNC) &r_parse_type, 1},
    {"is_function_type", (DL_FUNC) &r_is_function_type, 1},
    {"get_parameter_type", (DL_FUNC) &r_get_parameter_type, 2},
    {"type_to_sexp_string", (DL_FUNC) &type_to_sexp_string, 1},


    /*  type inference */
    {"check_type", (DL_FUNC) &r_check_type, 4},

    /*  type declaration cache */
    {"is_type_well_formed", (DL_FUNC) &r_is_type_well_formed, 1},
    {"import_type_declarations", (DL_FUNC) &r_import_type_declarations, 2},
    {"get_type_index", (DL_FUNC) &r_get_type_index, 2},
    {"get_typed_package_names", (DL_FUNC) &r_get_typed_package_names, 0},
    {"get_typed_function_names", (DL_FUNC) &r_get_typed_function_names, 1},
    {"is_package_typed", (DL_FUNC) &r_is_package_typed, 1},
    {"is_function_typed", (DL_FUNC) &r_is_function_typed, 2},
    {"set_type_declaration", (DL_FUNC) &r_set_type_declaration, 3},
    {"show_function_type_declaration",
     (DL_FUNC) &r_show_function_type_declaration,
     3},
    {"show_package_type_declarations",
     (DL_FUNC) &r_show_package_type_declarations,
     2},
    {"show_type_declarations", (DL_FUNC) &r_show_type_declarations, 1},
    {NULL, NULL, 0}};

void R_init_contractr(DllInfo* dll) {
    R_registerRoutines(dll, NULL, callMethods, NULL, NULL);

    R_useDynamicSymbols(dll, FALSE);

    initialize_globals();

    initialize_contracts();
}
}
