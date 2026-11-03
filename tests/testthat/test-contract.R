test_that("get_contracts / clear_contracts start empty", {
    clear_contracts()
    ct <- get_contracts()
    expect_s3_class(ct, "data.frame")
    expect_equal(nrow(ct), 0)
})

test_that("capture_contracts returns the value and the captured contracts", {
    tyr_skip_unless_rstt()
    ns <- testpackage_fixture()
    clear_contracts()

    # The capture contains the domain verdict and the return verdict.
    cap <- capture_contracts(ns$foo2(2L))

    expect_named(cap, c("result", "contracts"))
    expect_equal(cap$result, 2L)
    expect_s3_class(cap$contracts, "data.frame")
    expect_equal(cap$contracts$parameter_position, c(-2L, -1L))
    expect_equal(cap$contracts$verdict, c("ok", "ok"))

    # With `separate = TRUE` (the default), the rows do not go into the global
    # table.
    expect_equal(nrow(get_contracts()), 0)
})

test_that("capture_contracts(separate = FALSE) accumulates into the global set", {
    tyr_skip_unless_rstt()
    ns <- testpackage_fixture()
    clear_contracts()

    capture_contracts(ns$foo2(2L), separate = FALSE)
    expect_equal(nrow(get_contracts()), 2)

    clear_contracts()
    expect_equal(nrow(get_contracts()), 0)
})

test_that("capture_contracts validates the separate flag", {
    expect_error(capture_contracts(1, separate = "yes"))
    expect_error(capture_contracts(1, separate = c(TRUE, FALSE)))
})

test_that("ignore_contracts evaluates code without recording contracts", {
    ns <- testpackage_fixture()
    clear_contracts()

    expect_equal(ignore_contracts(ns$foo2(5L)), 5L)
    expect_equal(nrow(get_contracts()), 0)
})
