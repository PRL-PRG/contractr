#include "Contract.hpp"
#include "r_api.hpp"
#include "utilities.hpp"
#include "compat.h"

std::vector<Contract*> contracts;
std::vector<bool> contract_status;

void initialize_contracts() {
    contract_status.push_back(true);
}

void accumulate_contract(Contract* contract) {
    contracts.push_back(contract);
}

const Contract& get_contract(int index) {
    return *contracts[index];
}

SEXP r_clear_contracts() {
    for (int i = 0; i < contracts.size(); ++i) {
        delete contracts[i];
    }
    contracts.clear();
    return R_NilValue;
}

SEXP r_enable_contracts() {
    contract_status.push_back(true);
    return R_NilValue;
}

SEXP r_disable_contracts() {
    contract_status.push_back(false);
    return R_NilValue;
}

SEXP r_reinstate_contract_status() {
    contract_status.pop_back();
    return R_NilValue;
}

bool contracts_are_enabled() {
    return contract_status.back();
}

bool contracts_are_disabled() {
    return !contracts_are_enabled();
}

SEXP r_capture_contracts(SEXP sym, SEXP env, SEXP r_separate) {
    bool separate = Rf_asLogical(r_separate);
    std::vector<Contract*> saved_contracts(std::move(contracts));
    contracts = std::vector<Contract*>();

    PROTECT(sym);
    PROTECT(env);

    SEXP code = PROTECT(contractr_findVarInFrame(env, sym));
    SEXP result = PROTECT(Rf_eval(code, env));
    SEXP r_contracts = PROTECT(r_get_contracts());

    /* With `separate`, delete the contracts that `code` recorded. */
    if (separate) {
        r_clear_contracts();
    }
    /* Without `separate`, add the contracts of `code` to the earlier ones. */
    else {
        saved_contracts.reserve(saved_contracts.size() + contracts.size());
        saved_contracts.insert(
            saved_contracts.end(), contracts.begin(), contracts.end());
    }

    contracts = std::move(saved_contracts);

    SEXP list =
        PROTECT(create_list({result, r_contracts}, {"result", "contracts"}));

    UNPROTECT(6);

    return list;
}

SEXP r_get_contracts() {
    int size = contracts.size();

    auto get_call_id = [](int index) -> int {
        return get_contract(index).get_call_id();
    };

    auto get_call_trace = [](int index) -> std::string {
        return get_contract(index).get_call_trace();
    };

    auto get_package_name = [](int index) -> std::string {
        return get_contract(index).get_package_name();
    };

    auto get_function_name = [](int index) -> std::string {
        return get_contract(index).get_function_name();
    };

    auto get_parameter_position = [](int index) -> int {
        return get_contract(index).get_parameter_position();
    };

    auto get_parameter_name = [](int index) -> std::string {
        return get_contract(index).get_parameter_name();
    };

    auto get_actual_type = [](int index) -> std::string {
        return get_contract(index).get_actual_type();
    };

    auto get_expected_type = [](int index) -> std::string {
        return get_contract(index).get_expected_type();
    };

    /* Gives `NA` for a row with an empty string. */
    auto optional_strings = [size](const std::string& (Contract::*get)() const) {
        SEXP result = PROTECT(Rf_allocVector(STRSXP, size));
        for (int i = 0; i < size; ++i) {
            const std::string& s = (get_contract(i).*get)();
            SET_STRING_ELT(result, i, s.empty() ? NA_STRING : Rf_mkChar(s.c_str()));
        }
        UNPROTECT(1);
        return result;
    };

    auto get_origin = [](int index) -> int {
        return get_contract(index).get_origin();
    };

    std::vector<SEXP> columns = {
        PROTECT(create_integer_vector(size, get_call_id)),
        PROTECT(create_character_vector(size, get_call_trace)),
        PROTECT(create_character_vector(size, get_package_name)),
        PROTECT(create_character_vector(size, get_function_name)),
        PROTECT(create_integer_vector(size, get_parameter_position)),
        PROTECT(create_character_vector(size, get_parameter_name)),
        PROTECT(create_character_vector(size, get_actual_type)),
        PROTECT(create_character_vector(size, get_expected_type)),
        PROTECT(optional_strings(&Contract::get_verdict)),
        PROTECT(create_integer_vector(size, get_origin)),
        PROTECT(optional_strings(&Contract::get_blame))};

    std::vector<std::string> names = {"call_id",
                                      "call_trace",
                                      "package_name",
                                      "function_name",
                                      "parameter_position",
                                      "parameter_name",
                                      "actual_type",
                                      "expected_type",
                                      "verdict",
                                      "origin",
                                      "blame"};

    SEXP df = PROTECT(create_data_frame(columns, names));

    UNPROTECT(columns.size() + 1);

    return df;
}
