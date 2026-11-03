test_that("into a package: only the declared functions", {
    ns <- testpackage_fixture()

    expect_true(has_contract(ns$foo1))
    expect_false(has_contract(ns$bar))
})

test_that("remove_contract restores the function", {
    ns <- testpackage_fixture()
    n <- contractr:::get_injected_function_count()

    expect_true(has_contract(ns$foo3))
    remove_contract(ns$foo3)

    expect_equal(contractr:::get_injected_function_count(), n - 1)
    expect_false(has_contract(ns$foo3))
    expect_error(remove_contract(ns$foo3), "does not have a contract")
})
