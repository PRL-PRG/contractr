test_inject <- function(name, thunk) {
  on.exit(.Call(reset_type_check_function))
  test_that(name, thunk)
}

test_inject("default type checking functions are set", {
  expect_identical(.Call(get_type_check_function), quote(contractR:::check_type))
  expect_identical(.Call(get_type_check_function_wrapper), quote(.Call))
})

test_inject("it is possible to set custom functions", {
  f <- function(...) {}
  .Call(set_type_check_function, quote(f), NULL);
  expect_identical(.Call(get_type_check_function), quote(f))
  expect_null(.Call(get_type_check_function_wrapper))
})

test_inject("inject type checks", {
  ns <- getNamespace("contractR")
  checks <- list()

  test_check <- function(val, pkg_name, fun_name, param_name, param_index) {
    checks <<- append(checks, list(as.list(match.call()[-1])))
    val
  }

  assign("test_check", test_check, envir=ns)

  .Call(set_type_check_function, quote(contractR:::test_check), NULL);

  f <- function(a, b, ...) {
    .Call(inject_type_checks, "contractR", "f1", sys.function(), sys.frame(sys.nframe()))
    list(...)
    a1 <- a
    b1 <- b
    a1 + b1 + a + b
  }

  expect_equal(f(1+1, 2+2), 12)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      val=quote(1+1),
      pkg_name="contractR",
      fun_name="f1",
      param_name="a",
      param_index=0
    )
  )
  expect_equal(
    checks[[2]],
    list(
      val=quote(2+2),
      pkg_name="contractR",
      fun_name="f1",
      param_name="b",
      param_index=1
    )
  )
})
