#include "ContractAssertion.hpp"

#include <vector>
#include "r_api.hpp"
#include "utilities.hpp"

#include "check_type.hpp"
#include "infer_type.hpp"
#include "type_declaration_cache.hpp"
#include "message.hpp"

void ContractAssertion::assert_parameter_type_(SEXP value) {
    assertion_status_ = true;
    actual_type_ = infer_type(value, get_parameter_name());
    expected_parameter_count_ =
        get_function_parameter_count(get_package_name(), get_function_name());

    /* parameter outside limits  */
    if (get_parameter_position() >= get_expected_parameter_count()) {
        assertion_status_ = false;
        show_message(
            "contract violation for '%s::%s'\n   ├── declared types for "
            "%d parameters\n   ├── received argument for untyped "
            "parameter '%s' (position %d) of type %s\n   └── trace: %s",
            get_package_name(),
            get_function_name(),
            get_expected_parameter_count(),
            get_parameter_name(),
            /* NOTE: indexing starts from 1 in R */
            get_parameter_position() + 1,
            get_actual_type().c_str(),
            get_call_trace());
    }
    /* parameter is within limits  */
    else {
        const tastr::ast::Node& node = get_function_parameter_type(
            get_package_name(), get_function_name(), get_parameter_position());

        assertion_status_ = check_type(get_parameter_name(), value, node);

        expected_type_ = type_to_string(node);

        if (!assertion_status_) {
            show_message(
                "contract violation for parameter '%s' (position %d) of "
                "'%s::%s'\n   ├── expected: %s\n   ├── actual: %s\n   └── "
                "trace: %s",
                get_parameter_name(),
                /* NOTE: indexing starts from 1 in R */
                get_parameter_position() + 1,
                get_package_name(),
                get_function_name(),
                get_expected_type().c_str(),
                get_actual_type().c_str(),
                get_call_trace());
        }
    }
}

void ContractAssertion::assert_return_type_(SEXP value) {
    expected_parameter_count_ =
        get_function_parameter_count(get_package_name(), get_function_name());

    const tastr::ast::Node& node =
        get_function_return_type(get_package_name(), get_function_name());

    parameter_position_ = -1;

    assertion_status_ = check_type(get_parameter_name(), value, node);

    expected_type_ = type_to_string(node);

    actual_type_ = infer_type(value, get_parameter_name());

    if (!assertion_status_) {
        show_message("contract violation for return value of "
                     "'%s::%s'\n   ├── expected: %s\n   ├── actual: %s\n   "
                     "└── trace: %s",
                     get_package_name(),
                     get_function_name(),
                     get_expected_type().c_str(),
                     get_actual_type().c_str(),
                     get_call_trace());
    }
}

std::ostream& operator<<(std::ostream& os,
                         const ContractAssertion& contract_assertion) {
    os << "Contract Assertion" << std::endl;
    os << "Call Id                 : " << contract_assertion.get_call_id()
       << std::endl;
    os << "Package Name            : " << contract_assertion.get_package_name()
       << std::endl;
    os << "Function Name           : " << contract_assertion.get_function_name()
       << std::endl;
    os << "Actual Parameter Count  : "
       << contract_assertion.get_actual_parameter_count() << std::endl;
    os << "Expected Parameter Count: "
       << contract_assertion.get_expected_parameter_count() << std::endl;
    os << "Parameter Position      : "
       << contract_assertion.get_parameter_position() << std::endl;
    os << "Parameter Name          : "
       << contract_assertion.get_parameter_name() << std::endl;

    return os;
}

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
