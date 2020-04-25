#include "ContractAssertion.hpp"

#include <vector>

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

void add_contract_assertion(const std::string& package_name,
                            const std::string& function_name,
                            int call_id,
                            const std::string& parameter_name,
                            const int parameter_count,
                            const int formal_parameter_position,
                            const std::string& actual_type,
                            const std::string& expected_type,
                            const bool assertion_status,
                            const std::string& call_trace) {
    ContractAssertion assertion(package_name,
                                function_name,
                                call_id,
                                parameter_name,
                                parameter_count,
                                formal_parameter_position,
                                actual_type,
                                expected_type,
                                assertion_status,
                                call_trace);

    add_contract_assertion(assertion);
}
