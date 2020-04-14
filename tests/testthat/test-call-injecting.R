test_inject_call("injection into a function", {
  f <- function(a, b) a + b

  inject_type_check_call(f, "f", "mypkg")

  expect_length(.injected_functions, 1)
  expect_type_checks_injected(f)
})

test_inject_call("inject into an environment", {
  e <- new.env(parent=emptyenv())
  e$f <- function(x) x
  e$g <- function(y) y
  e$h <- sin

  expect_warning(inject_type_check_calls("e", e), regexp="Unable to inject type checks into `e:::h`: .*")

  expect_length(.injected_functions, 2)
  expect_type_checks_injected(e$f)
  expect_type_checks_injected(e$g)
  expect_false(is_type_check_injected(e$h))
  expect_identical(e$h, sin)
})

test_inject_call("inject into a package", {
  withr::with_temp_libpaths({
    devtools::install("TestPackage", quick=TRUE)
    inject_type_check_calls("TestPackage")

    # TODO: check the results
    expect_type_checks_injected(TestPackage::foo1)
    expect_type_checks_injected(TestPackage::foo2)
    expect_type_checks_injected(TestPackage::foo3)
    expect_type_checks_injected(TestPackage::foo4)
    expect_type_checks_injected(TestPackage::foo5)

    expect_type_checks_injected(TestPackage:::bar)
  })
})
