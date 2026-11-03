## These tests examine `type_roclet()`. It writes the `@type` annotations of a
## package to `inst/TYR`.

annotated_package <- function(code) {
    pkg <- file.path(tempfile("roclet"), "annotated")
    dir.create(file.path(pkg, "R"), recursive = TRUE)
    writeLines(c("Package: annotated", "Version: 0.0.1", "Title: Annotated",
                 "Description: Annotated.", "License: GPL-3", "Encoding: UTF-8"),
               file.path(pkg, "DESCRIPTION"))
    writeLines(code, file.path(pkg, "R", "code.R"))
    pkg
}

roxygenise_types <- function(pkg) {
    suppressMessages(roxygen2::roxygenise(pkg, roclets = "contractr::type_roclet",
                                          load_code = "source"))
}

test_that("every @type of a function is a #| line of inst/TYR", {
    skip_if_not_installed("roxygen2")
    pkg <- annotated_package(c(
        "#' Doubles or strings.",
        "#' @type (x: dbl) -> dbl",
        "#' @type (x: chr)",
        "#'   -> chr",
        "f <- function(x) x",
        "",
        "#' @type (x: any, y: any) -> any",
        "`%+%` <- function(x, y) x",
        "",
        "g <- function() NULL"))
    roxygenise_types(pkg)
    tyr <- file.path(pkg, "inst", "TYR")
    expect_equal(readLines(tyr)[-1L],
                 c("#| annotated::f : (x: dbl) -> dbl",
                   "#| annotated::f : (x: chr) -> chr",
                   "#| annotated::(%+%) : (x: any, y: any) -> any"))
    ## contractr reads the file with this function when the package loads.
    expect_equal(contractr:::tyr_import_calls("annotated", tyr), c("f", "%+%"))
    expect_equal(contractr:::tyr_call_declaration("annotated", "f")$type,
                 "((x: dbl) -> dbl) & ((x: chr) -> chr)")
})

test_that("an annotation that is not a signature is reported and left out", {
    skip_if_not_installed("roxygen2")
    pkg <- annotated_package(c("#' @type dbl",
                               "f <- function(x) x",
                               "",
                               "#' @type (x: dbl) -> dbl",
                               "g <- function(x) x"))
    expect_warning(roxygenise_types(pkg), "not a signature")
    expect_equal(readLines(file.path(pkg, "inst", "TYR"))[-1L],
                 "#| annotated::g : (x: dbl) -> dbl")
})

test_that("an inst/TYR the roclet did not write is left alone", {
    skip_if_not_installed("roxygen2")
    pkg <- annotated_package(c("#' @type (x: dbl) -> dbl", "f <- function(x) x"))
    dir.create(file.path(pkg, "inst"))
    tyr <- file.path(pkg, "inst", "TYR")
    writeLines(c("# tyr: inferred types of annotated", "#| annotated::f : (x: int) -> int"), tyr)
    expect_warning(roxygenise_types(pkg), "not overwriting")
    expect_equal(readLines(tyr)[[2L]], "#| annotated::f : (x: int) -> int")
})

test_that("removing every annotation removes the generated inst/TYR", {
    skip_if_not_installed("roxygen2")
    pkg <- annotated_package(c("#' @type (x: dbl) -> dbl", "f <- function(x) x"))
    roxygenise_types(pkg)
    expect_true(file.exists(file.path(pkg, "inst", "TYR")))
    writeLines("f <- function(x) x", file.path(pkg, "R", "code.R"))
    roxygenise_types(pkg)
    expect_false(file.exists(file.path(pkg, "inst", "TYR")))
})
