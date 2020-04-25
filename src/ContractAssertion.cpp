#include "ContractAssertion.hpp"

#include <vector>
#include "r_api.hpp"
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
    SEXP assertions = PROTECT(r_get_assertions());

    contract_assertions = std::move(saved_contract_assertions);

    SEXP list =
        PROTECT(create_list({result, assertions}, {"result", "assertions"}));

    UNPROTECT(5);

    return list;
}


SEXP r_get_assertions() {
    int size = get_contract_assertion_count();

    auto get_call_id = [](int index) -> int {
        return get_contract_assertion(index).get_call_id();
    };

    auto get_call_trace = [](int index) -> std::string {
        return get_contract_assertion(index).get_call_trace();
    };

    auto get_package_name = [](int index) -> std::string {
        return get_contract_assertion(index).get_package_name();
    };

    auto get_function_name = [](int index) -> std::string {
        return get_contract_assertion(index).get_function_name();
    };

    auto get_actual_parameter_count = [](int index) -> int {
        return get_contract_assertion(index).get_actual_parameter_count();
    };

    auto get_expected_parameter_count = [](int index) -> int {
        return get_contract_assertion(index).get_expected_parameter_count();
    };

    auto get_parameter_position = [](int index) -> int {
        return get_contract_assertion(index).get_parameter_position();
    };

    auto get_parameter_name = [](int index) -> std::string {
        return get_contract_assertion(index).get_parameter_name();
    };

    auto get_actual_type = [](int index) -> std::string {
        return get_contract_assertion(index).get_actual_type();
    };

    auto get_expected_type = [](int index) -> std::string {
        return get_contract_assertion(index).get_expected_type();
    };

    auto get_assertion_status = [](int index) -> bool {
        return get_contract_assertion(index).get_assertion_status();
    };

    std::vector<SEXP> columns = {
        PROTECT(create_integer_vector(size, get_call_id)),
        PROTECT(create_character_vector(size, get_call_trace)),
        PROTECT(create_character_vector(size, get_package_name)),
        PROTECT(create_character_vector(size, get_function_name)),
        PROTECT(create_integer_vector(size, get_actual_parameter_count)),
        PROTECT(create_integer_vector(size, get_expected_parameter_count)),
        PROTECT(create_integer_vector(size, get_parameter_position)),
        PROTECT(create_character_vector(size, get_parameter_name)),
        PROTECT(create_character_vector(size, get_actual_type)),
        PROTECT(create_character_vector(size, get_expected_type)),
        PROTECT(create_logical_vector(size, get_assertion_status))};

    std::vector<std::string> names = {"call_id",
                                      "call_trace",
                                      "package_name",
                                      "function_name",
                                      "actual_parameter_count",
                                      "expected_parameter_count",
                                      "parameter_position",
                                      "parameter_name",
                                      "actual_type",
                                      "expected_type",
                                      "assertion_status"};

    SEXP df = PROTECT(create_data_frame(columns, names));

    UNPROTECT(columns.size() + 1);

    return df;
}
