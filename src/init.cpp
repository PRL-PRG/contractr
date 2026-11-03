#define R_NO_REMAP

#include "utilities.hpp"
#include "Contract.hpp"
#include "tyr.hpp"
#include "r_api.hpp"
#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // Declares `NULL`.

extern "C" {

static const R_CallMethodDef callMethods[] = {
    /* Full-call checks of tyr types */
    {"tyr_infer_type", (DL_FUNC) &r_tyr_infer_type, 1},
    {"tyr_args", (DL_FUNC) &r_tyr_args, 2},
    {"tyr_peek", (DL_FUNC) &r_tyr_peek, 1},
    {"tyr_forced_type", (DL_FUNC) &r_tyr_forced_type, 1},
    {"tyr_contracts_enabled", (DL_FUNC) &r_tyr_contracts_enabled, 0},
    {"tyr_record", (DL_FUNC) &r_tyr_record, 11},

    /* The contract table */
    {"capture_contracts", (DL_FUNC) &r_capture_contracts, 3},
    {"get_contracts", (DL_FUNC) &r_get_contracts, 0},
    {"clear_contracts", (DL_FUNC) &r_clear_contracts, 0},
    {"enable_contracts", (DL_FUNC) &r_enable_contracts, 0},
    {"disable_contracts", (DL_FUNC) &r_disable_contracts, 0},
    {"reinstate_contract_status", (DL_FUNC) &r_reinstate_contract_status, 0},

    /* Severity */
    {"set_severity", (DL_FUNC) &r_set_severity, 1},
    {"get_severity", (DL_FUNC) &r_get_severity, 0},
    {NULL, NULL, 0}};

void R_init_contractr(DllInfo* dll) {
    R_registerRoutines(dll, NULL, callMethods, NULL, NULL);

    R_useDynamicSymbols(dll, FALSE);

    initialize_globals();

    initialize_contracts();
}
}
