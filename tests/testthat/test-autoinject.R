## contractr adds contracts to a package that has an `inst/TYR` file. It does
## this when R loads the package and contractr. The order of the two loads is
## not important. Each test runs in a new R session, because the tests
## examine the effect of `library(contractr)` on the session.

checked <- 'cat("\\nRESULT", has_contract(TestPackage::foo1), has_contract(TestPackage:::bar), "\\n")'

result <- function(out) {
    sub("^RESULT ", "", grep("^RESULT ", trimws(out), value = TRUE))
}

test_that("a package loaded before contractr", {
    out <- testpackage_run(paste('loadNamespace("TestPackage")',
                                 'library(contractr)', checked, sep = "; "))
    expect_equal(result(out), "TRUE FALSE")
    expect_false(any(grepl("instrumented", out)))
})

test_that("options(contractr.verbose = 1) shows the number of monitored functions", {
    out <- testpackage_run(paste('options(contractr.verbose = 1)',
                                 'loadNamespace("TestPackage")',
                                 'library(contractr)', checked, sep = "; "))
    expect_equal(result(out), "TRUE FALSE")
    expect_true(any(grepl("TestPackage: instrumented 3 function(s)", out, fixed = TRUE)))
})

test_that("a package loaded after contractr", {
    out <- testpackage_run(paste('library(contractr)',
                                 'loadNamespace("TestPackage")', checked, sep = "; "))
    expect_equal(result(out), "TRUE FALSE")
})

test_that("a source tree loaded with pkgload::load_all(), from its inst/TYR", {
    skip_if_not_installed("pkgload")
    source <- normalizePath(test_path("TestPackage"))
    out <- testpackage_run(paste('library(contractr)',
                                 sprintf('pkgload::load_all("%s", quiet = TRUE)', source),
                                 checked, sep = "; "))
    expect_equal(result(out), "TRUE FALSE")
})

test_that("options(contractr.autoinject = FALSE) turns it off, and nothing else is touched", {
    out <- testpackage_run(paste('options(contractr.autoinject = FALSE)',
                                 'library(contractr)',
                                 'loadNamespace("TestPackage")', checked, sep = "; "))
    expect_equal(result(out), "FALSE FALSE")
    expect_false(any(grepl("instrumented", out)))
})

test_that("set_autoinject() takes the names of the packages", {
    out <- testpackage_run(paste('options(contractr.autoinject = FALSE)',
                                 'library(contractr)',
                                 'set_autoinject("TestPackage")',
                                 'loadNamespace("TestPackage")', checked, sep = "; "))
    expect_equal(result(out), "TRUE FALSE")

    out <- testpackage_run(paste('library(contractr)',
                                 'set_autoinject(c("a", "b"))',
                                 'loadNamespace("TestPackage")', checked, sep = "; "))
    expect_equal(result(out), "FALSE FALSE")
})

test_that("set_autoinject() rejects anything else", {
    old <- get_autoinject()
    on.exit(set_autoinject(old))
    expect_error(set_autoinject(NA), "should be TRUE, FALSE or a vector")
    expect_error(set_autoinject(1), "should be TRUE, FALSE or a vector")
})

test_that("the declarations are TYR once installed, inst/TYR in a source tree", {
    expect_equal(contractr:::tyr_declaration_file(file.path(testpackage_lib(), "TestPackage")),
                 file.path(testpackage_lib(), "TestPackage", "TYR"))
    expect_equal(contractr:::tyr_declaration_file(test_path("TestPackage")),
                 file.path(test_path("TestPackage"), "inst", "TYR"))
    expect_equal(contractr:::tyr_declaration_file(test_path("tyrcalls")), "")
})

test_that("insert_tyr_contract() needs declarations", {
    expect_error(insert_tyr_contract("stats"), "no type declarations")
})
