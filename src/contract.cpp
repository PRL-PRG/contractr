#include "contract.hpp"
#include "r_api.hpp"
#include "utilities.hpp"
#include "raise.hpp"
#include "call_trace.hpp"

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

void destroy_r_contract(SEXP r_contract) {
    Contract* contract = static_cast<Contract*>(R_ExternalPtrAddr(r_contract));
    if (contract) {
        if (contract->is_asserted()) {
            errorcall(R_NilValue,
                      "asserted contract deleted without accumulation");
        }
        delete contract;
        R_SetExternalPtrAddr(r_contract, nullptr);
    }
}

SEXP create_r_contract(Contract* contract) {
    SEXP externalptr =
        PROTECT(R_MakeExternalPtr(contract, R_NilValue, R_NilValue));

    R_RegisterCFinalizerEx(externalptr, destroy_r_contract, TRUE);

    UNPROTECT(1);

    return externalptr;
}

Contract* extract_from_r_contract(SEXP r_contract) {
    Contract* contract = static_cast<Contract*>(R_ExternalPtrAddr(r_contract));
    R_SetExternalPtrAddr(r_contract, nullptr);
    return contract;
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

    accumulate_contract(contract);

    return value;
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
    contract->set_function_type(function_type);
    contract->set_expected_parameter_count(
        get_function_parameter_count(function_type));

    return create_r_contract(contract);
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
    bool separate = asLogical(r_separate);
    std::vector<Contract*> saved_contracts(std::move(contracts));
    contracts = std::vector<Contract*>();

    PROTECT(sym);
    PROTECT(env);

    SEXP code = PROTECT(Rf_findVarInFrame(env, sym));
    SEXP result = PROTECT(Rf_eval(code, env));
    SEXP r_contracts = PROTECT(r_get_contracts());

    /*  delete contracts generated by this code block  */
    if (separate) {
        r_clear_contracts();
    }
    /* merge contracts with the previously created ones */
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

    auto get_actual_parameter_count = [](int index) -> int {
        return get_contract(index).get_actual_parameter_count();
    };

    auto get_expected_parameter_count = [](int index) -> int {
        return get_contract(index).get_expected_parameter_count();
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

    auto get_assertion_status = [](int index) -> bool {
        return get_contract(index).get_assertion_status();
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
