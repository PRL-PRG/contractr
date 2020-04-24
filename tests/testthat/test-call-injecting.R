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

  expect_silent(inject_environment_type_assertions(e, "e"))

  expect_length(.injected_functions, 0)
  expect_false(is_type_assertion_injected(e$f))
  expect_false(is_type_assertion_injected(e$g))
  expect_false(is_type_assertion_injected(e$h))
  expect_identical(e$h, sin)
})

test_injection("into a package", {
  withr::with_temp_libpaths({
      devtools::install("TestPackage", quick=TRUE)
      library(TestPackage)

      set_type_declaration(foo1,
                           "any => any",
                           package_name = "TestPackage",
                           function_name = "foo1")

      insert_package_contract("TestPackage")

      expect_true(is_type_assertion_injected(foo1))

      expect_false(is_type_assertion_injected(TestPackage:::bar))
  })
})
