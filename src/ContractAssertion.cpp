#include "ContractAssertion.hpp"

#include <vector>
#include <Rinternals.h>
#include "inject.hpp"
#include "utilities.hpp"

std::vector<ContractAssertion> contract_assertions;

const ContractAssertion& get_contract_assertion(int index) {
    return contract_assertions[index];
}

int get_contract_assertion_count() {
    return contract_assertions.size();
}

void add_contract_assertion(const ContractAssertion& assertion) {
    contract_assertions.push_back(assertion);
}

void add_contract_assertion(int call_id,
                            const std::string& call_trace,
                            const std::string& package_name,
                            const std::string& function_name,
                            const int actual_parameter_count,
                            const int expected_parameter_count,
                            const int parameter_position,
                            const std::string& parameter_name,
                            const std::string& actual_type,
                            const std::string& expected_type,
                            const bool assertion_status) {
    ContractAssertion assertion(call_id,
                                call_trace,
                                package_name,
                                function_name,
                                actual_parameter_count,
                                expected_parameter_count,
                                parameter_position,
                                parameter_name,
                                actual_type,
                                expected_type,
                                assertion_status);

    add_contract_assertion(assertion);
}

SEXP r_capture_assertions(SEXP sym, SEXP env) {
    std::vector<ContractAssertion> saved_contract_assertions(
        std::move(contract_assertions));
    contract_assertions = std::vector<ContractAssertion>();

    PROTECT(sym);
    PROTECT(env);
    SEXP result = PROTECT(Rf_eval(Rf_findVarInFrame(env, sym), env));
    SEXP assertions = PROTECT(r_get_contract_assertions());

    contract_assertions = std::move(saved_contract_assertions);

    SEXP list =
        PROTECT(create_list({result, assertions}, {"result", "assertions"}));

    UNPROTECT(5);

    return list;
}
