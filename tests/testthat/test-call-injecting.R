test_injection("into a function", {
  f <- function(a, b) a + b

  insert_contract(f, "<int, int> => int", fun_name = "f", pkg_name = "mypkg")

  expect_length(contractR:::get_injected_function_count(), 1)

  expect_true(has_contract(f))

  remove_contract(f)

  expect_equal(contractR:::get_injected_function_count(), 0)

  expect_false(has_contract(f))

})

test_injection("into an environment", {
  e <- new.env(parent=emptyenv())
  e$f <- function(x) x
  e$g <- function(y) y
  e$h <- sin

  expect_silent(insert_environment_contract(e, "e", FALSE))

  expect_equal(contractR:::get_injected_function_count(), 0)
  expect_false(has_contract(e$f))
  expect_false(has_contract(e$g))
  expect_false(has_contract(e$h))
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

      expect_true(has_contract(foo1))

      expect_false(has_contract(TestPackage:::bar))
  })
})
