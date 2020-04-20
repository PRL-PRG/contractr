assert_type <- function(val, missing_val, pkg_name, fun_name, param_name, param_idx) {
#  .Call(C_assert_type, val, missing(val), pkg_name, fun_name, param_name, param_idx)
  browser()
  val
}

set_assert_type_fun <- function(fun) {
  assign("assert_type", fun, envir=getNamespace("contractR"))
}
