#ifndef CONTRACTR_CONTRACT_HPP
#define CONTRACTR_CONTRACT_HPP

#include <tastr/ast/ast.hpp>
#include <string>
#include <Rinternals.h>

#include "utilities.hpp"
#include "check_type.hpp"
#include "infer_type.hpp"
#include "type_declaration.hpp"
#include "r_api.hpp"
#undef length

class Contract {
  public:
    Contract()
        : call_id_(-1)
        , call_trace_(UNDEFINED_STRING_VALUE)
        , package_name_(UNDEFINED_STRING_VALUE)
        , function_name_(UNDEFINED_STRING_VALUE)
        , actual_parameter_count_(-1)
        , expected_parameter_count_(-1)
        , parameter_position_(-1)
        , parameter_name_(UNDEFINED_STRING_VALUE)
        , actual_type_(UNDEFINED_STRING_VALUE)
        , expected_type_(UNDEFINED_STRING_VALUE)
        , function_type_(nullptr)
        , assertion_status_(false)
        , asserted_(false) {
    }

    int get_call_id() const {
        return call_id_;
    }

    void set_call_id(int call_id) {
        call_id_ = call_id;
    }

    const std::string& get_call_trace() const {
        return call_trace_;
    }

    void set_call_trace(const std::string& call_trace) {
        call_trace_ = call_trace;
    }

    const std::string& get_package_name() const {
        return package_name_;
    }

    void set_package_name(const std::string& package_name) {
        package_name_ = package_name;
    }

    const std::string& get_function_name() const {
        return function_name_;
    }

    void set_function_name(const std::string& function_name) {
        function_name_ = function_name;
    }

    int get_actual_parameter_count() const {
        return actual_parameter_count_;
    }

    void set_actual_parameter_count(int actual_parameter_count) {
        actual_parameter_count_ = actual_parameter_count;
    }

    int get_expected_parameter_count() const {
        return expected_parameter_count_;
    }

    void set_expected_parameter_count(int expected_parameter_count) {
        expected_parameter_count_ = expected_parameter_count;
    }

    int get_parameter_position() const {
        return parameter_position_;
    }

    void set_parameter_position(int parameter_position) {
        parameter_position_ = parameter_position;
    }

    const std::string& get_parameter_name() const {
        return parameter_name_;
    }

    void set_parameter_name(const std::string& parameter_name) {
        parameter_name_ = parameter_name;
    }

    const std::string& get_actual_type() const {
        return actual_type_;
    }

    const std::string& get_expected_type() const {
        return expected_type_;
    }

    const tastr::ast::FunctionTypeNode* get_function_type() {
        return function_type_;
    }

    void set_function_type(const tastr::ast::FunctionTypeNode* function_type) {
        function_type_ = function_type;
    }

    bool get_assertion_status() const {
        return assertion_status_;
    }

    bool is_asserted() const {
        return asserted_;
    }

    void assert(SEXP value, bool is_missing) {
        if (is_asserted()) {
            Rf_errorcall(R_NilValue, "contract is being reasserted");
            return;
        }

        asserted_ = true;

        SEXP actual_value = is_missing ? R_MissingArg : value;

        /* return type contract  */
        if (get_parameter_position() == -1) {
            const tastr::ast::Node& node =
                get_function_return_type(get_function_type());

            assertion_status_ = check_type(actual_value, node);
            expected_type_ = type_to_string(node);
            actual_type_ = infer_type(actual_value);
        }
        /* parameter outside limits  */
        else if (get_parameter_position() >= get_expected_parameter_count()) {
            assertion_status_ = false;
            actual_type_ = infer_type(get_parameter_name(), actual_value);
        }
        /* parameter is within limits  */
        else {
            const tastr::ast::Node& node = get_function_parameter_type(
                get_function_type(), get_parameter_position());

            assertion_status_ =
                check_type(get_parameter_name(), actual_value, node);
            expected_type_ = type_to_string(node);
            actual_type_ = infer_type(get_parameter_name(), actual_value);
        }
    }

  private:
    int call_id_;
    std::string call_trace_;
    std::string package_name_;
    std::string function_name_;
    int actual_parameter_count_;
    int expected_parameter_count_;
    int parameter_position_;
    std::string parameter_name_;
    std::string actual_type_;
    std::string expected_type_;
    const tastr::ast::FunctionTypeNode* function_type_;
    bool assertion_status_;
    bool asserted_;
};

void initialize_contracts();

void accumulate_contract(Contract* contract);

void destroy_r_contract(SEXP r_contract);

SEXP create_r_contract(Contract* contract);

Contract* extract_from_r_contract(SEXP r_contract);

SEXP r_assert_contract(SEXP r_contract, SEXP value, SEXP is_value_missing);

Contract* create_argument_contract(Contract* result_contract,
                                   SEXP r_parameter_name,
                                   int parameter_position);

bool contracts_are_enabled();

bool contracts_are_disabled();

#endif /* CONTRACTR_CONTRACT_HPP */
