assert_type <- function(value, missing_value, pkg_name, fun_name,
                        param_name, param_idx) {
  .Call(
    C_assert_type,
    value,
    missing_value,
    pkg_name,
    fun_name,
    param_name,
    param_idx
  )

  value
}

set_assert_type_fun <- function(fun) {
  assign("assert_type", fun, envir=getNamespace("contractR"))
}
