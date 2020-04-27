#ifndef CONTRACTR_CONTRACT_CLASS_HPP
#define CONTRACTR_CONTRACT_CLASS_HPP

#include <tastr/ast/ast.hpp>
#include <string>
#include <Rinternals.h>

#include "utilities.hpp"
#include "check_type.hpp"
#include "infer_type.hpp"
#include "type_declaration_cache.hpp"
#include "r_api.hpp"
#undef length

class Contract {
  public:
    Contract(bool owner)
        : owner_(owner)
        , call_id_(-1)
        , call_trace_(nullptr)
        , package_name_(nullptr)
        , function_name_(nullptr)
        , actual_parameter_count_(-1)
        , expected_parameter_count_(-1)
        , parameter_position_(-1)
        , parameter_name_(nullptr)
        , actual_type_("")
        , expected_type_("")
        , assertion_status_(false)
        , function_type_(nullptr) {
    }

    ~Contract() {
        if (is_owner()) {
            free((char*) (call_trace_));
            free((char*) (package_name_));
            free((char*) (function_name_));
        }
        if (parameter_position_ != -1) {
            free((char*) (parameter_name_));
        }
    }

    bool is_owner() const {
        return owner_;
    }

    void set_owner(bool owner) {
        owner_ = owner;
    }

    int get_call_id() const {
        return call_id_;
    }

    void set_call_id(int call_id) {
        call_id_ = call_id;
    }

    const char* get_call_trace() const {
        return call_trace_;
    }

    void set_call_trace(const char* call_trace) {
        call_trace_ = call_trace;
    }

    const char* get_package_name() const {
        return package_name_;
    }

    void set_package_name(const char* package_name) {
        package_name_ = package_name;
    }

    const char* get_function_name() const {
        return function_name_;
    }

    void set_function_name(const char* function_name) {
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

    const char* get_parameter_name() const {
        return parameter_name_;
    }

    void set_parameter_name(char* parameter_name) {
        parameter_name_ = parameter_name;
    }

    const std::string& get_actual_type() const {
        return actual_type_;
    }

    const std::string& get_expected_type() const {
        return expected_type_;
    }

    bool get_assertion_status() const {
        return assertion_status_;
    }

    const tastr::ast::FunctionTypeNode* get_function_type() {
        return function_type_;
    }

    void set_function_type(const tastr::ast::FunctionTypeNode* function_type) {
        function_type_ = function_type;
    }

    SEXP assert(SEXP value, bool is_missing) {
        SEXP actual_value = is_missing ? R_MissingArg : value;

        /* return type contract  */
        if (get_parameter_position() == -1) {
            const tastr::ast::Node& node =
                get_function_return_type(get_function_type());

            assertion_status_ =
                check_type(get_parameter_name(), actual_value, node);
            expected_type_ = type_to_string(node);
            actual_type_ = infer_type(actual_value);
        }
        /* parameter outside limits  */
        else if (get_parameter_position() >= get_expected_parameter_count()) {
            assertion_status_ = false;
            actual_type_ = infer_type(actual_value, get_parameter_name());
        }
        /* parameter is within limits  */
        else {
            const tastr::ast::Node& node = get_function_parameter_type(
                get_function_type(), get_parameter_position());

            assertion_status_ =
                check_type(get_parameter_name(), actual_value, node);
            expected_type_ = type_to_string(node);
            actual_type_ = infer_type(actual_value, get_parameter_name());
        }

        return value;
    }

  private:
    void assert_parameter_type_(SEXP value);
    void assert_return_type_(SEXP value);

    bool owner_;
    int call_id_;
    const char* call_trace_;
    const char* package_name_;
    const char* function_name_;
    int actual_parameter_count_;
    int expected_parameter_count_;
    int parameter_position_;
    const char* parameter_name_;
    std::string actual_type_;
    std::string expected_type_;
    bool assertion_status_;
    const tastr::ast::FunctionTypeNode* function_type_;
};

#endif /* CONTRACTR_CONTRACT_CLASS_HPP */
