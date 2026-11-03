#ifndef CONTRACTR_CONTRACT_HPP
#define CONTRACTR_CONTRACT_HPP

#include <string>
#include <Rinternals.h>

#include "utilities.hpp"
#include "r_api.hpp"
#undef length

/* A row of the contract table. It records a verdict on a monitored call. */
class Contract {
  public:
    Contract()
        : call_id_(-1)
        , call_trace_(UNDEFINED_STRING_VALUE)
        , package_name_(UNDEFINED_STRING_VALUE)
        , function_name_(UNDEFINED_STRING_VALUE)
        , parameter_position_(-1)
        , parameter_name_(UNDEFINED_STRING_VALUE)
        , actual_type_(UNDEFINED_STRING_VALUE)
        , expected_type_(UNDEFINED_STRING_VALUE)
        , origin_(NA_INTEGER) {
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

    /* Sets the type of the value, the declared type and the verdict on them.
     * `verdict` is `ok`, `violation` or `unknown`. `origin` is the call id of
     * the root violation that this violation follows from. It is `NA` for a
     * root violation and for a row that is not a violation. `blame` is
     * `caller` or `callee`. It is empty if the row gives no blame. */
    void set_verdict(const std::string& actual_type,
                     const std::string& expected_type,
                     const std::string& verdict,
                     int origin,
                     const std::string& blame) {
        actual_type_ = actual_type;
        expected_type_ = expected_type;
        verdict_ = verdict;
        origin_ = origin;
        blame_ = blame;
    }

    const std::string& get_verdict() const {
        return verdict_;
    }

    int get_origin() const {
        return origin_;
    }

    const std::string& get_blame() const {
        return blame_;
    }

  private:
    int call_id_;
    std::string call_trace_;
    std::string package_name_;
    std::string function_name_;
    int parameter_position_;
    std::string parameter_name_;
    std::string actual_type_;
    std::string expected_type_;
    std::string verdict_;
    int origin_;
    std::string blame_;
};

void initialize_contracts();

void accumulate_contract(Contract* contract);

bool contracts_are_enabled();

bool contracts_are_disabled();

#endif /* CONTRACTR_CONTRACT_HPP */
