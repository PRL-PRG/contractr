#ifndef CONTRACTR_CONTRACT_ASSERTION_HPP
#define CONTRACTR_CONTRACT_ASSERTION_HPP

#include <string>
#include <Rinternals.h>

class ContractAssertion {
  public:
    ContractAssertion(int call_id,
                      const std::string& call_trace,
                      const std::string& package_name,
                      const std::string& function_name,
                      const int actual_parameter_count,
                      const int expected_parameter_count,
                      const int parameter_position,
                      const std::string& parameter_name,
                      const std::string& actual_type,
                      const std::string& expected_type,
                      const bool assertion_status)
        : call_id_(call_id)
        , call_trace_(call_trace)
        , package_name_(package_name)
        , function_name_(function_name)
        , actual_parameter_count_(actual_parameter_count)
        , expected_parameter_count_(expected_parameter_count)
        , parameter_position_(parameter_position)
        , parameter_name_(parameter_name)
        , actual_type_(actual_type)
        , expected_type_(expected_type)
        , assertion_status_(assertion_status) {
    }

    int get_call_id() const {
        return call_id_;
    }

    const std::string& get_call_trace() const {
        return call_trace_;
    }

    const std::string& get_package_name() const {
        return package_name_;
    }

    const std::string& get_function_name() const {
        return function_name_;
    }

    int get_actual_parameter_count() const {
        return actual_parameter_count_;
    }

    int get_expected_parameter_count() const {
        return expected_parameter_count_;
    }

    int get_parameter_position() const {
        return parameter_position_;
    }

    const std::string& get_parameter_name() const {
        return parameter_name_;
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

  private:
    int call_id_;
    const std::string call_trace_;
    const std::string package_name_;
    const std::string function_name_;
    const int actual_parameter_count_;
    const int expected_parameter_count_;
    const int parameter_position_;
    const std::string parameter_name_;
    const std::string actual_type_;
    const std::string expected_type_;
    const bool assertion_status_;
};

const ContractAssertion& get_contract_assertion(int index);

int get_contract_assertion_count();

void add_contract_assertion(const ContractAssertion& assertion);

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
                            const bool assertion_status);


#endif /* CONTRACTR_CONTRACT_ASSERTION_HPP */
