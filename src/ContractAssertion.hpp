#ifndef CONTRACTR_CONTRACT_ASSERTION_HPP
#define CONTRACTR_CONTRACT_ASSERTION_HPP

#include <string>

class ContractAssertion {
  public:
    ContractAssertion(const std::string& package_name,
                      const std::string& function_name,
                      int call_id,
                      const std::string& parameter_name,
                      const int parameter_count,
                      const int parameter_position,
                      const std::string& actual_type,
                      const std::string& expected_type,
                      const bool assertion_status,
                      const std::string& call_trace)
        : package_name_(package_name)
        , function_name_(function_name)
        , call_id_(call_id)
        , parameter_name_(parameter_name)
        , parameter_count_(parameter_count)
        , parameter_position_(parameter_position)
        , actual_type_(actual_type)
        , expected_type_(expected_type)
        , assertion_status_(assertion_status)
        , call_trace_(call_trace) {
    }

    const std::string& get_package_name() const {
        return package_name_;
    }

    const std::string& get_function_name() const {
        return function_name_;
    }

    int get_call_id() const {
        return call_id_;
    }

    const std::string& get_parameter_name() const {
        return parameter_name_;
    }

    int get_parameter_count() const {
        return parameter_count_;
    }

    int get_parameter_position() const {
        return parameter_position_;
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

    const std::string& get_call_trace() const {
        return call_trace_;
    }

  private:
    const std::string package_name_;
    const std::string function_name_;
    int call_id_;
    const std::string parameter_name_;
    const int parameter_count_;
    const int parameter_position_;
    const std::string actual_type_;
    const std::string expected_type_;
    const bool assertion_status_;
    const std::string call_trace_;
};

const ContractAssertion& get_contract_assertion(int index);

int get_contract_assertion_count();

void add_contract_assertion(const ContractAssertion& assertion);

void add_contract_assertion(const std::string& package_name,
                            const std::string& function_name,
                            int call_id,
                            const std::string& parameter_name,
                            const int parameter_count,
                            const int formal_parameter_position,
                            const std::string& actual_type,
                            const std::string& expected_type,
                            const bool assertion_status,
                            const std::string& call_trace);

#endif /* CONTRACTR_CONTRACT_ASSERTION_HPP */
