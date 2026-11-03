test_that("get_severity / set_severity round-trip", {
    old <- get_severity()
    on.exit(set_severity(old), add = TRUE)

    for (level in c("warning", "error", "silence")) {
        set_severity(level)
        expect_identical(get_severity(), level)
    }
})

test_that("set_severity rejects unknown levels", {
    old <- get_severity()
    on.exit(set_severity(old), add = TRUE)

    expect_error(set_severity("bogus"), "severity should be")
    # An invalid level must not change the current severity.
    set_severity("error")
    expect_error(set_severity("loud"))
    expect_identical(get_severity(), "error")
})

test_that("set_severity default falls back to the option or 'warning'", {
    old <- get_severity()
    on.exit(set_severity(old), add = TRUE)

    withr::with_options(list(contractr.severity = "silence"), {
        set_severity()
        expect_identical(get_severity(), "silence")
    })

    withr::with_options(list(contractr.severity = NULL), {
        set_severity()
        expect_identical(get_severity(), "warning")
    })
})
