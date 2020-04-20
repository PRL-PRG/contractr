test_injection("into a function", {
  f <- function(a, b) a + b

  inject_type_assertion(f, fun_name = "f", pkg_name = "mypkg")

  expect_length(.injected_functions, 1)
  expect_type_assertion(f)
})

test_injection("into an environment", {
  e <- new.env(parent=emptyenv())
  e$f <- function(x) x
  e$g <- function(y) y
  e$h <- sin

  expect_warning(inject_environment_type_assertions("e", e), regexp="Unable to inject type checks into `e:::h`: .*")

  expect_length(.injected_functions, 2)
  expect_type_assertion(e$f)
  expect_type_assertion(e$g)
  expect_false(is_type_assertion_injected(e$h))
  expect_identical(e$h, sin)
})

test_injection("into a package", {
  withr::with_temp_libpaths({
    devtools::install("TestPackage", quick=TRUE)
    inject_environment_type_assertions("TestPackage")

    expect_type_assertion(TestPackage::foo1)
    expect_type_assertion(TestPackage:::bar)
  })
})
