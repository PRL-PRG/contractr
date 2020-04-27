#include "utilities.hpp"
#include "Contract.hpp"
#include "r_api.hpp"
#include "contract.hpp"
#include "type_declaration_cache.hpp"

void insert_argument_contract(Contract* contract,
                              SEXP param_sym,
                              SEXP value,
                              SEXP rho) {
    SEXP call_value = NULL;
    SEXP value_missing = R_FalseValue;
    bool evaluate_call = false;

    if (value == R_UnboundValue || value == R_MissingArg) {
        /* missing argument */
        call_value = R_NilValue;
        value_missing = R_TrueValue;
        evaluate_call = true;
    } else if (param_sym == R_DotsSymbol) {
        /* ... parameter */
        call_value = R_NilValue;
        value_missing = R_FalseValue;
        evaluate_call = true;
    } else if (TYPEOF(value) != PROMSXP) {
        /* value (promise optimized away by compiler)  */
        SEXP param_name = PROTECT(mkString(CHAR(PRINTNAME(param_sym))));
        value = delayed_assign(param_name, value, rho, rho, rho);
        UNPROTECT(1);
        call_value = PREXPR(value);
        value_missing = R_FalseValue;
        evaluate_call = false;
    } else if (TYPEOF(value) == PROMSXP) {
        /* promise */
        call_value = PREXPR(value);
        value_missing = R_FalseValue;
        evaluate_call = false;
    }

    SEXP r_contract = PROTECT(create_r_contract(contract));

    SEXP call = PROTECT(create_assert_contract_call(
        list3(r_contract, call_value, value_missing)));

#ifndef NDEBUG
    Rprintf(
        "\ncontractR/src/inject.cpp: *** %s '%s' for '%s:::%s' with call id "
        "%d in '%s' [%d]\n",
        evaluate_call ? "calling" : "injecting",
        R_CHAR(Rf_asChar(Rf_deparse1(call, FALSE, 0))),
        contract->get_package_name().c_str(),
        contract->get_function_name().c_str(),
        contract->get_call_id(),
        contract->get_parameter_name().c_str(),
        contract->get_parameter_position());
#endif

    if (evaluate_call) {
        Rf_eval(call, rho);
    } else {
        SET_PRCODE(value, call);
    }

    UNPROTECT(2);
}

SEXP r_insert_function_contract(SEXP r_contract, SEXP fun, SEXP rho) {
    if (TYPEOF(fun) != CLOSXP) {
        Rf_error("argument fun must be a function");
    }

    if (TYPEOF(rho) != ENVSXP) {
        Rf_error("argument rho must be an environment");
    }

    if (TYPEOF(r_contract) != EXTPTRSXP) {
        Rf_error("contract must be an external pointer");
    }

    if (contracts_are_disabled()) {
        return R_NilValue;
    }

    Contract* contract = static_cast<Contract*>(R_ExternalPtrAddr(r_contract));

    SEXP params = FORMALS(fun);

    int actual_parameter_count = 0;
    for (actual_parameter_count = 0; params != R_NilValue;
         ++actual_parameter_count, params = CDR(params))
        ;

    contract->set_actual_parameter_count(actual_parameter_count);

    params = FORMALS(fun);

    for (int index = 0; params != R_NilValue; index++, params = CDR(params)) {
        SEXP param_sym = TAG(params);
        SEXP value = Rf_findVarInFrame(rho, param_sym);

        Contract* argument_contract =
            create_argument_contract(contract, param_sym, index);

        insert_argument_contract(argument_contract, param_sym, value, rho);
    }

    return R_NilValue;
}
