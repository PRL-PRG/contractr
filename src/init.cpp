#define R_NO_REMAP

#include "type_declaration_cache.hpp"
#include "utilities.hpp"
#include "r_api.hpp"
#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

extern "C" {

static const R_CallMethodDef callMethods[] = {
    /* contract insertion  */
    {"insert_function_contract", (DL_FUNC) &r_insert_function_contract, 3},
    {"create_result_contract", (DL_FUNC) &r_create_result_contract, 4},

    /*  contract assertion */
    {"assert_contract", (DL_FUNC) &r_assert_contract, 3},
    {"capture_assertions", (DL_FUNC) &r_capture_assertions, 2},
    {"get_assertions", (DL_FUNC) &r_get_assertions, 0},

    /* severity */
    {"set_severity", (DL_FUNC) &r_set_severity, 1},
    {"get_severity", (DL_FUNC) &r_get_severity, 0},

    /* type checking */
    {"infer_type", (DL_FUNC) &r_infer_type, 3},

    /*  type inference */
    {"check_type", (DL_FUNC) &r_check_type, 4},

    /*  type declaration cache */
    {"import_type_declarations", (DL_FUNC) &r_import_type_declarations, 1},
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

void R_init_contractR(DllInfo* dll) {
    R_registerRoutines(dll, NULL, callMethods, NULL, NULL);

    R_useDynamicSymbols(dll, FALSE);

    initialize_globals();

    initialize_type_declaration_cache();
}
}
