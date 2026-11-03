/* Full-call checks of tyr types.
 *
 * The state of each monitored call is in R (R/tyr_call.R). This C code does
 * the operations on promises that must not force them:
 *
 *  - `r_tyr_args`: When a monitored call starts, it examines the binding of
 *    each formal. It keeps the promise of each argument that R did not force.
 *    It does not change the code of the promise. At the exit of the call,
 *    `r_tyr_forced_type` gives the type of the value if R forced the promise.
 *  - `r_tyr_peek`: Returns the type of an argument that R did not force, if
 *    contractr can get it without an evaluation. This is possible for a
 *    constant, or for a variable that has a value.
 *  - `r_tyr_record`: Records a row of the contract table.
 */

#include "tyr.hpp"
#include "Contract.hpp"
#include "utilities.hpp"
#include "compat.h"

#undef length

#include <string>
#include <vector>

namespace {

/* Returns `true` if the value evaluates to itself. Such a value can be the
 * code of a promise for the same value. A symbol or a call is not such a value,
 * because R evaluates it. */
bool is_self_evaluating(SEXP value) {
    switch (TYPEOF(value)) {
    case SYMSXP:
    case LANGSXP:
    case PROMSXP:
    case BCODESXP:
    case DOTSXP:
        return false;
    default:
        return true;
    }
}

/* Returns the binding of `sym` only in the frame `env`, not in its parents.
 * It does not force a promise, and it does not run an active binding. It
 * returns `R_UnboundValue` if there is no binding or if the binding is active. */
SEXP frame_binding(SEXP env, SEXP sym) {
    if (TYPEOF(env) != ENVSXP || !R_existsVarInFrame(env, sym))
        return R_UnboundValue;
    if (R_BindingIsActive(sym, env))
        return R_UnboundValue;
    return contractr_findVarInFrame(env, sym);
}

/* Returns `true` if `sym` in `env` is a formal that the caller did not supply.
 * It also returns `true` if `sym` is a promise that R did not force, with code
 * that is only the name of such a formal. Then the argument is not in the call.
 *
 * A chain that ends at a formal bound to its default gives `false`, for
 * example `a` in `f <- function(a = 1) g(a)`. The argument is in the call. */
bool forwards_absent(SEXP sym, SEXP env, int depth) {
    if (depth > 100)
        return false;
    SEXP binding = frame_binding(env, sym);
    if (binding == R_MissingArg)
        return true;
    if (TYPEOF(binding) != PROMSXP)
        return false;
    /* A formal bound to its default. */
    if (contractr_PRENV(binding) == env)
        return false;
    SEXP code = contractr_PREXPR(binding);
    if (contractr_PRVALUE(binding) == R_UnboundValue && TYPEOF(code) == SYMSXP)
        return forwards_absent(code, contractr_PRENV(binding), depth + 1);
    return false;
}

/* Returns a new promise for a value that evaluates to itself. The promise is
 * not forced. The function calls `delayedAssign()` in a temporary frame, and
 * it reads the binding without a force. */
SEXP promise_of(SEXP value) {
    SEXP scratch = PROTECT(R_NewEnv(R_EmptyEnv, FALSE, 1));
    SEXP name = PROTECT(Rf_mkString(".contractr__value"));
    SEXP call =
        PROTECT(Rf_lang5(DelayedAssign, name, value, R_BaseEnv, scratch));
    Rf_eval(call, R_BaseEnv);
    SEXP promise =
        contractr_findVarInFrame(scratch, Rf_install(".contractr__value"));
    UNPROTECT(3);
    return promise;
}

struct Slot {
    std::string label; /* The formal name, or the tag of a `...` element ("" if no tag) */
    std::string kind;  /* "pre" or "post" (a formal before or after `...`), or "dots" */
    /* One of these values:
     *  - "absent": The call does not have the argument.
     *  - "unforced": R did not force the argument when the call started.
     *    contractr gets its type at the exit if R forced it.
     *  - "typed": contractr knows the type of the argument. */
    std::string state;
    std::string type;  /* The type, if the state is "typed" */
    SEXP promise;      /* The promise, if the state is "unforced" */
};

/* Sets the fields of `slot` from the binding `value`. If the binding is a
 * value, `rebind` puts a new promise in its place. */
template <typename Rebind>
void inspect(Slot& slot, SEXP value, SEXP rho, Rebind rebind) {
    slot.promise = R_NilValue;

    if (value == R_UnboundValue || value == R_MissingArg) {
        slot.state = "absent";
        return;
    }

    if (TYPEOF(value) == PROMSXP) {
        /* A formal bound to its default. The caller did not supply it. */
        if (slot.kind != "dots" && contractr_PRENV(value) == rho) {
            slot.state = "absent";
            return;
        }
        if (contractr_PRVALUE(value) != R_UnboundValue) {
            /* R forced the argument before the call started. */
            slot.state = "typed";
            slot.type = tyr_infer_type(contractr_PRVALUE(value));
            return;
        }
        SEXP code = contractr_PREXPR(value);
        if (TYPEOF(code) == SYMSXP &&
            forwards_absent(code, contractr_PRENV(value), 0)) {
            slot.state = "absent";
            return;
        }
        slot.state = "unforced";
        slot.promise = value;
        return;
    }

    /* The binding is a value, not a promise. Byte-compiled code supplies a
     * constant argument as a value, and `do.call()` supplies values. Put the
     * value in a new promise. Then contractr gets its type only if the body
     * uses it. */
    if (is_self_evaluating(value)) {
        SEXP promise = PROTECT(promise_of(value));
        rebind(promise);
        slot.state = "unforced";
        slot.promise = promise;
        UNPROTECT(1);
        return;
    }

    slot.state = "typed";
    slot.type = tyr_infer_type(value);
}

SEXP strings(const std::vector<Slot>& slots,
             std::string Slot::*field,
             bool na_when_empty) {
    SEXP result = PROTECT(Rf_allocVector(STRSXP, slots.size()));
    for (std::size_t i = 0; i < slots.size(); ++i) {
        const std::string& s = slots[i].*field;
        SET_STRING_ELT(result,
                       i,
                       na_when_empty && s.empty() ? NA_STRING
                                                  : Rf_mkChar(s.c_str()));
    }
    UNPROTECT(1);
    return result;
}

SEXP sexps(const std::vector<Slot>& slots, SEXP Slot::*field) {
    SEXP result = PROTECT(Rf_allocVector(VECSXP, slots.size()));
    for (std::size_t i = 0; i < slots.size(); ++i)
        SET_VECTOR_ELT(result, i, slots[i].*field);
    UNPROTECT(1);
    return result;
}

} // namespace

SEXP r_tyr_infer_type(SEXP value) {
    return Rf_mkString(tyr_infer_type(value).c_str());
}

SEXP r_tyr_contracts_enabled() {
    return Rf_ScalarLogical(contracts_are_enabled());
}

/* Makes one slot for each formal of `fun` and one slot for each element of
 * `...`. It reads the bindings in the frame `rho` of the call that started (see
 * `inspect()`). Returns a list of parallel vectors: `label`, `kind`, `state`,
 * `type` and `promise`. */
SEXP r_tyr_args(SEXP fun, SEXP rho) {
    if (TYPEOF(fun) != CLOSXP)
        Rf_error("fun must be a closure");
    if (TYPEOF(rho) != ENVSXP)
        Rf_error("rho must be an environment");

    /* The garbage collector does not free the SEXPs of the slots, because R
     * can get to them. Each SEXP is a promise bound in `rho` or in its `...`. */
    std::vector<Slot> slots;

    bool after_dots = false;
    for (SEXP params = contractr_FORMALS(fun); params != R_NilValue;
         params = CDR(params)) {
        SEXP sym = TAG(params);

        if (sym == R_DotsSymbol) {
            after_dots = true;
            SEXP dots = frame_binding(rho, R_DotsSymbol);
            if (TYPEOF(dots) != DOTSXP)
                continue;
            for (SEXP cell = dots; cell != R_NilValue; cell = CDR(cell)) {
                Slot slot;
                slot.label =
                    TAG(cell) == R_NilValue ? "" : CHAR(PRINTNAME(TAG(cell)));
                slot.kind = "dots";
                inspect(slot, CAR(cell), rho,
                        [cell](SEXP promise) { SETCAR(cell, promise); });
                slots.push_back(slot);
            }
            continue;
        }

        Slot slot;
        slot.label = CHAR(PRINTNAME(sym));
        slot.kind = after_dots ? "post" : "pre";
        inspect(slot, frame_binding(rho, sym), rho,
                [rho, sym](SEXP promise) { Rf_defineVar(sym, promise, rho); });
        slots.push_back(slot);
    }

    SEXP result = PROTECT(create_list({PROTECT(strings(slots, &Slot::label, false)),
                                       PROTECT(strings(slots, &Slot::kind, false)),
                                       PROTECT(strings(slots, &Slot::state, false)),
                                       PROTECT(strings(slots, &Slot::type, true)),
                                       PROTECT(sexps(slots, &Slot::promise))},
                                      {"label", "kind", "state", "type", "promise"}));
    UNPROTECT(6);
    return result;
}

/* Returns the type of the value of an argument promise, if contractr can get
 * it without an evaluation. If not, returns `NULL`. */
SEXP r_tyr_peek(SEXP promise) {
    if (TYPEOF(promise) != PROMSXP)
        return R_NilValue;
    if (contractr_PRVALUE(promise) != R_UnboundValue)
        return Rf_mkString(
            tyr_infer_type(contractr_PRVALUE(promise)).c_str());

    SEXP code = contractr_PREXPR(promise);
    if (is_self_evaluating(code))
        return Rf_mkString(tyr_infer_type(code).c_str());

    if (TYPEOF(code) != SYMSXP)
        return R_NilValue;

    /* The code is a variable. Find its value as an evaluation does, but stop
     * at a binding that runs code: an active binding, or a promise that R did
     * not force. */
    for (SEXP env = contractr_PRENV(promise);
         TYPEOF(env) == ENVSXP && env != R_EmptyEnv;
         env = R_ParentEnv(env)) {
        if (!R_existsVarInFrame(env, code))
            continue;
        if (R_BindingIsActive(code, env))
            return R_NilValue;
        SEXP value = contractr_findVarInFrame(env, code);
        if (TYPEOF(value) == PROMSXP) {
            if (contractr_PRVALUE(value) == R_UnboundValue)
                return R_NilValue;
            value = contractr_PRVALUE(value);
        }
        if (value == R_MissingArg || value == R_UnboundValue)
            return R_NilValue;
        return Rf_mkString(tyr_infer_type(value).c_str());
    }
    return R_NilValue;
}

/* Returns the type of the value of a forced promise, or `NULL` if `promise`
 * is not a forced promise. */
SEXP r_tyr_forced_type(SEXP promise) {
    if (TYPEOF(promise) != PROMSXP ||
        contractr_PRVALUE(promise) == R_UnboundValue)
        return R_NilValue;
    return Rf_mkString(tyr_infer_type(contractr_PRVALUE(promise)).c_str());
}

SEXP r_tyr_record(SEXP r_call_id,
                  SEXP r_call_trace,
                  SEXP r_package_name,
                  SEXP r_function_name,
                  SEXP r_position,
                  SEXP r_parameter_name,
                  SEXP r_actual,
                  SEXP r_expected,
                  SEXP r_verdict,
                  SEXP r_origin,
                  SEXP r_blame) {
    auto string_of = [](SEXP s) -> std::string {
        SEXP c = Rf_asChar(s);
        return c == NA_STRING ? std::string() : std::string(CHAR(c));
    };

    Contract* contract = new Contract();
    contract->set_call_id(Rf_asInteger(r_call_id));
    contract->set_call_trace(string_of(r_call_trace));
    contract->set_package_name(string_of(r_package_name));
    contract->set_function_name(string_of(r_function_name));
    contract->set_parameter_position(Rf_asInteger(r_position));
    contract->set_parameter_name(string_of(r_parameter_name));
    contract->set_verdict(string_of(r_actual),
                          string_of(r_expected),
                          string_of(r_verdict),
                          Rf_asInteger(r_origin),
                          string_of(r_blame));
    accumulate_contract(contract);
    return R_NilValue;
}
