## These tests examine how tyr_rstt_path() finds the rstt REPL.

test_that("a directory is not the rstt REPL", {
    withr::local_options(contractr.rstt = tempdir())
    expect_identical(tyr_rstt_path(), "")
})

test_that("a file is the rstt REPL", {
    file <- withr::local_tempfile()
    file.create(file)
    withr::local_options(contractr.rstt = file)
    expect_identical(tyr_rstt_path(), normalizePath(file))
})
