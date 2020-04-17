#define R_NO_REMAP
#include "inject.hpp"
#include "type_declaration_cache.hpp"
#include "utilities.hpp"

#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL

extern "C" {

static const R_CallMethodDef callMethods[] = {
    {"get_type_check_function", (DL_FUNC) &get_type_check_function, 0},
    {"get_type_check_function_wrapper",
     (DL_FUNC) &get_type_check_function_wrapper,
     0},
    {"set_type_check_function", (DL_FUNC) &set_type_check_function, 2},
    {"reset_type_check_function", (DL_FUNC) &reset_type_check_function, 0},
    {"create_check_type_call", (DL_FUNC) &create_check_type_call, 7},
    {"inject_type_check", (DL_FUNC) &inject_type_check, 4},
    {"log_insertion", (DL_FUNC) &log_insertion, 7},
    {"check_type", (DL_FUNC) &check_type, 7},
    {"environment_name", (DL_FUNC) &environment_name, 1},

    /*  type declaration cache */
    {"import_type_declarations", (DL_FUNC) &import_type_declarations, 1},
    {"get_typed_package_names", (DL_FUNC) &get_typed_package_names, 0},
    {"get_typed_function_names", (DL_FUNC) &get_typed_function_names, 1},
    {"is_package_typed", (DL_FUNC) &is_package_typed, 1},
    {"is_function_typed", (DL_FUNC) &is_function_typed, 2},

    {NULL, NULL, 0}};

void R_init_contractR(DllInfo* dll) {
    R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);

    initialize_globals();

    initialize_type_declaration_cache();

    reset_type_check_function();
}
}
