test_inject <- function(desc, code) {
  on.exit(.Call(reset_type_check_function))
  test_that(desc, code)
}
