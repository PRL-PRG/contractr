assert_type <- function(value, missing_value, pkg_name, fun_name,
                        call_id, param_name, param_count, param_idx) {
  .Call(
    C_assert_type,
    value,
    missing_value,
    pkg_name,
    fun_name,
    call_id,
    param_name,
    param_count,
    param_idx
  )

  value
}

set_assert_type_fun <- function(fun) {
  assign("assert_type", fun, envir=getNamespace("contractR"))
}

#' @export
get_contract_assertions <- function() {
    .Call(C_get_contract_assertions)
}
