#ifndef CONTRACTR_CONTRACT_HPP
#define CONTRACTR_CONTRACT_HPP

#include <Rinternals.h>
#include "Contract.hpp"

void initialize_contracts();

void accumulate_contract(Contract* contract);

void destroy_r_contract(SEXP r_contract);

SEXP create_r_contract(Contract* contract);

Contract* extract_from_r_contract(SEXP r_contract);

SEXP r_assert_contract(SEXP r_contract, SEXP value, SEXP is_value_missing);

bool contracts_are_enabled();

bool contracts_are_disabled();

#endif /* CONTRACTR_CONTRACT_HPP */
