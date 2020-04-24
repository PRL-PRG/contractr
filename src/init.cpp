#define R_NO_REMAP
#include "inject.hpp"
#include "type_declaration_cache.hpp"
#include "utilities.hpp"
#include "Typechecker.hpp"
#include "infer_type.hpp"

#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

extern "C" {

static const R_CallMethodDef callMethods[] = {
    {"inject_type_assertion", (DL_FUNC) &inject_type_assertion, 7},
    {"assert_type", (DL_FUNC) &assert_type, 9},
    {"environment_name", (DL_FUNC) &environment_name, 1},
    {"get_contract_assertions", (DL_FUNC) &r_get_contract_assertions, 0},
    {"concatenate_call_trace", (DL_FUNC) &r_concatenate_call_trace, 1},

    /* severity */
    {"set_severity", (DL_FUNC) &r_set_severity, 1},
    {"get_severity", (DL_FUNC) &r_get_severity, 0},

    /* type checking utilities */
    {"infer_type", (DL_FUNC) &r_infer_type, 3},
    {"check_type", (DL_FUNC) &r_check_type, 4},

    /*  type declaration cache */
    {"clear_type_declaration_cache",
     (DL_FUNC) &clear_type_declaration_cache,
     0},
    {"import_type_declarations", (DL_FUNC) &import_type_declarations, 1},
    {"get_typed_package_names", (DL_FUNC) &get_typed_package_names, 0},
    {"get_typed_function_names", (DL_FUNC) &get_typed_function_names, 1},
    {"is_package_typed", (DL_FUNC) &is_package_typed, 1},
    {"is_function_typed", (DL_FUNC) &is_function_typed, 2},
    {"set_type_declaration", (DL_FUNC) &set_type_declaration, 3},
    {"remove_type_declaration", (DL_FUNC) &remove_type_declaration, 2},
    {"show_function_type_declaration",
     (DL_FUNC) &show_function_type_declaration,
     2},
    {"show_package_type_declarations",
     (DL_FUNC) &show_package_type_declarations,
     1},
    {"show_type_declarations", (DL_FUNC) &show_type_declarations, 0},
    {NULL, NULL, 0}};

void R_init_contractR(DllInfo* dll) {
    R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);

    initialize_globals();

    initialize_type_declaration_cache();
}
}
