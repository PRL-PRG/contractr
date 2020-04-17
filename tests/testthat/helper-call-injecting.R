test_inject_call <- function(desc, code) {
  on.exit({
    .injected_functions <<- new.env(parent=emptyenv())
  })
  test_that(desc, code)
}

expect_type_checks_injected <- function(f) {
  expect_true(is_type_check_injected(f))
}
