context("type assertion")

.__assert_type_fun__ <- contractr:::assert_type # nolint
checks <- list()

teardown({
  set_assert_type_fun(.__assert_type_fun__)
})

setup({
  assert_stub <- function(value, value_missing,
                          pkg_name, fun_name, param_name, param_index) {
    checks <<- append(checks, list(as.list(match.call()[-1])))
    value
  }

  set_assert_type_fun(assert_stub)
})

test_that("simple function", {
  checks <<- list()
  
  f <- function(a, b) {
    .Call(C_inject_type_assertion, "contractr", "f", sys.function(), sys.frame(sys.nframe()))

    # here we are trying to get the value twice to have the test
    # check that the type check happens only once
    a1 <- a
    b1 <- b
    a1 + b1 + a + b
  }

  expect_equal(f(1+1, 2+2), 12)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      value=quote(1+1),
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="a",
      param_index=0
    )
  )
  expect_equal(
    checks[[2]],
    list(
      value=quote(2+2),
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="b",
      param_index=1
    )
  )
})

test_that("function with ...", {
  checks <<- list()

  f <- function(a, ...) {
    .Call(C_inject_type_assertion, "contractr", "f", sys.function(), sys.frame(sys.nframe()))

    list(...)
    a + sum(c(...))
  }

  expect_equal(f(1+1, 2+2, 3+3), 12)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      value=NULL,
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="...",
      param_index=1
    )
  )
  expect_equal(
    checks[[2]],
    list(
      value=quote(1+1),
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="a",
      param_index=0
    )
  )
})

test_that("function with missing ...", {
  checks <<- list()

  f <- function(a, ...) {
    .Call(C_inject_type_assertion, "contractr", "f", sys.function(), sys.frame(sys.nframe()))

    list(...)
    a + sum(c(...))
  }

  expect_equal(f(1+1), 2)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      value=NULL,
      value_missing=TRUE,
      pkg_name="contractr",
      fun_name="f",
      param_name="...",
      param_index=1
    )
  )
  expect_equal(
    checks[[2]],
    list(
      value=quote(1+1),
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="a",
      param_index=0
    )
  )
})

test_that("function with missing argument", {
  checks <<- list()

  f <- function(a, b) {
    .Call(C_inject_type_assertion, "contractr", "f", sys.function(), sys.frame(sys.nframe()))

    if (missing(b)) b <- 3
    a + b
  }

  expect_equal(f(1+1), 5)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      value=NULL,
      value_missing=TRUE,
      pkg_name="contractr",
      fun_name="f",
      param_name="b",
      param_index=1
    )
  )
  expect_equal(
    checks[[2]],
    list(
      value=quote(1+1),
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="a",
      param_index=0
    )
  )
})

test_that("function with default argument", {
  checks <<- list()

  f <- function(a, b=2+2) {
    .Call(C_inject_type_assertion, "contractr", "f", sys.function(), sys.frame(sys.nframe()))
    a + b
  }

  expect_equal(f(1+1), 6)
  expect_length(checks, 2)
  expect_equal(
    checks[[1]],
    list(
      value=quote(1+1),
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="a",
      param_index=0
    )
  )
  expect_equal(
    checks[[2]],
    list(
      value=quote(2+2),
      value_missing=FALSE,
      pkg_name="contractr",
      fun_name="f",
      param_name="b",
      param_index=1
    )
  )
})
