#include "utilities.hpp"
#include "Contract.hpp"
#include "call_trace.hpp"
#include "r_api.hpp"
#include "contract.hpp"
#include "type_declaration_cache.hpp"
#include "raise.hpp"

char result_name[] = ".contractr__return_value";

char* copy_c_string(const char* str) {
    int size = strlen(str);
    char* duplicate = (char*) malloc((size + 1) * sizeof(char));
    for (int i = 0; i < size; ++i) {
        duplicate[i] = str[i];
    }

    duplicate[size] = '\0';

    return duplicate;
}

Contract* create_argument_contract(Contract* result_contract,
                                   SEXP r_parameter_name,
                                   int parameter_position) {
    Contract* argument_contract = new Contract(*result_contract);
    /* NOTE: this contract is not an owner unlike result contract  */
    argument_contract->set_owner(false);
    argument_contract->set_parameter_name(
        copy_c_string(R_CHAR(PRINTNAME(r_parameter_name))));
    argument_contract->set_parameter_position(parameter_position);

    return argument_contract;
}

SEXP r_create_result_contract(SEXP r_call_id,
                              SEXP r_call_trace,
                              SEXP r_package_name,
                              SEXP r_function_name,
                              SEXP r_type_index) {
    int call_id = asInteger(r_call_id);
    // TODO: optimize creation of call_trace
    const char* call_trace = copy_c_string(
        concatenate_call_trace(r_call_trace, std::string(14, ' ')).c_str());
    const char* package_name = copy_c_string(CHAR(asChar(r_package_name)));
    const char* function_name = copy_c_string(CHAR(asChar(r_function_name)));
    int package_index = INTEGER(r_type_index)[0];
    int function_index = INTEGER(r_type_index)[1];

    const tastr::ast::FunctionTypeNode* function_type =
        get_function_type(package_index, function_index);

    Contract* contract = new Contract(true);
    contract->set_call_id(call_id);
    contract->set_call_trace(call_trace);
    contract->set_package_name(package_name);
    contract->set_function_name(function_name);
    contract->set_parameter_position(-1);
    contract->set_parameter_name(result_name);
    contract->set_function_type(function_type);
    contract->set_expected_parameter_count(
        get_function_parameter_count(function_type));

    return create_r_contract(contract);
}

SEXP r_assert_contract(SEXP r_contract, SEXP value, SEXP is_value_missing) {
    if (contracts_are_disabled()) {
        return value;
    }

    Contract* contract = extract_from_r_contract(r_contract);

    if (contract == nullptr) {
        errorcall(R_NilValue,
                  "invalid contract reference encountered while asserting");
        return value;
    }

    contract->assert(value, asLogical(is_value_missing));

    raise_contract_failure(contract);

    add_contract(contract);

    return value;
}

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
