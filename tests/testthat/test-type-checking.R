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
    .Call(inject_type_check, "contractR", "f1", sys.function(), sys.frame(sys.nframe()))

    # here we are trying to get the value twice to have the test
    # check that the type check happens only once
    list(...)
    a1 <- a
    if (missing(b)) b <- 0
    b1 <- b
    a1 + b1 + a + b + sum(c(...))
  }

  g <- function(a, b=2+2) {
    .Call(inject_type_check, "contractR", "g1", sys.function(), sys.frame(sys.nframe()))
    a1 <- a
    b1 <- b
    a + a1 + b + b1
  }

  # try with no ...
  expect_equal(f(1+1, 2+2), 12)
  expect_length(checks, 3)
  expect_equal(
    checks[[1]],
    list(
      val=NULL,
      pkg_name="contractR",
      fun_name="f1",
      param_name="...",
      param_index=2
    )
  )
  expect_equal(
    checks[[2]],
    list(
      val=quote(1+1),
      pkg_name="contractR",
      fun_name="f1",
      param_name="a",
      param_index=0
    )
  )
  expect_equal(
    checks[[3]],
    list(
      val=quote(2+2),
      pkg_name="contractR",
      fun_name="f1",
      param_name="b",
      param_index=1
    )
  )

  checks <- list()

  # try with ...
  expect_equal(f(1+1, 2+2, 3+3, z=4+4), 26)
  expect_length(checks, 3)
  expect_equal(
    checks[[1]],
    list(
      val=NULL,
      pkg_name="contractR",
      fun_name="f1",
      param_name="...",
      param_index=2
    )
  )
  expect_equal(
    checks[[2]],
    list(
      val=quote(1+1),
      pkg_name="contractR",
      fun_name="f1",
      param_name="a",
      param_index=0
    )
  )
  expect_equal(
    checks[[3]],
    list(
      val=quote(2+2),
      pkg_name="contractR",
      fun_name="f1",
      param_name="b",
      param_index=1
    )
  )

  checks <- list()

  # try with missing argument
  expect_equal(f(1+1), 4)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      val=NULL,
      pkg_name="contractR",
      fun_name="f1",
      param_name="...",
      param_index=2
    )
  )
  expect_equal(
    checks[[2]],
    list(
      val=quote(1+1),
      pkg_name="contractR",
      fun_name="f1",
      param_name="a",
      param_index=0
    )
  )

  checks <- list()

  # try default arguments
  g <- function(a, b=2+2) {
    .Call(inject_type_check, "contractR", "g1", sys.function(), sys.frame(sys.nframe()))
    a1 <- a
    b1 <- b
    a + a1 + b + b1
  }

  expect_equal(g(1+1), 12)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      val=quote(1+1),
      pkg_name="contractR",
      fun_name="g1",
      param_name="a",
      param_index=0
    )
  )
  expect_equal(
    checks[[2]],
    list(
      val=quote(2+2),
      pkg_name="contractR",
      fun_name="g1",
      param_name="b",
      param_index=1
    )
  )

})
