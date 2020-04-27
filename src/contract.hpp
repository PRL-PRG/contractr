#ifndef CONTRACTR_CONTRACT_HPP
#define CONTRACTR_CONTRACT_HPP

#include <Rinternals.h>
#include "ContractAssertion.hpp"

void initialize_contracts();

void add_contract(ContractAssertion* contract);

void destroy_r_contract(SEXP r_contract);

SEXP create_r_contract(ContractAssertion* contract);

bool contracts_are_enabled();

bool contracts_are_disabled();

#endif /* CONTRACTR_CONTRACT_HPP */
