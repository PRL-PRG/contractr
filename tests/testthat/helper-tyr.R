## Returns the namespace of the fixture package `tyrcalls/`, with the contracts
## from `tyrcalls.tyr`. The first call in a session installs the package in a
## temporary library and loads it.
tyr_fixture <- function() {
    if (!"tyrcalls" %in% loadedNamespaces()) {
        lib <- tempfile("tyrcalls-lib")
        dir.create(lib)
        system2(file.path(R.home("bin"), "R"),
                c("CMD", "INSTALL", "--no-test-load", "-l", shQuote(lib),
                  shQuote(test_path("tyrcalls"))),
                stdout = FALSE, stderr = FALSE)
        loadNamespace("tyrcalls", lib.loc = lib)
        suppressMessages(insert_tyr_contract("tyrcalls", test_path("tyrcalls.tyr")))
    }
    asNamespace("tyrcalls")
}

tyr_skip_unless_rstt <- function() {
    skip_if_not_installed("processx")
    skip_if_not(nzchar(tyr_rstt_path()), "rstt not found (set CONTRACTR_RSTT)")
}

## Evaluates `expr` with the given `severity`, and with `ret` as the return
## value of the fixture functions. Returns the value, the warnings, the
## messages, the error (if there is one) and the recorded rows. `domain` and
## `ret` are the verdicts of the domain rows and of the return rows.
tyr_run <- function(expr, ret = NULL, severity = "warning") {
    old <- options(tyrcalls.ret = ret)
    on.exit(options(old))
    old_severity <- get_severity()
    set_severity(severity)
    on.exit(set_severity(old_severity), add = TRUE)

    clear_contracts()
    warnings <- character(0)
    messages <- character(0)
    error <- NULL
    value <- tryCatch(
        withCallingHandlers(expr, warning = function(w) {
            warnings <<- c(warnings, conditionMessage(w))
            invokeRestart("muffleWarning")
        }, message = function(m) {
            messages <<- c(messages, conditionMessage(m))
            invokeRestart("muffleMessage")
        }),
        error = function(e) {
            error <<- conditionMessage(e)
            NULL
        })
    rows <- get_contracts()
    clear_contracts()
    list(value = value, warnings = warnings, messages = messages, error = error, rows = rows,
         domain = rows$verdict[rows$parameter_position == -2L],
         ret = rows$verdict[rows$parameter_position == -1L])
}

## Returns a temporary library that contains `TestPackage/`. The first call in a
## session installs the package. Its `inst/TYR` contains the `#|` declarations
## of three of its functions. The tests of the automatic contracts, of the
## contract table and of `remove_contract()` use it.
testpackage_lib <- function() {
    lib <- getOption("contractr.test.testpackage_lib")
    if (is.null(lib)) {
        lib <- tempfile("TestPackage-lib")
        dir.create(lib)
        system2(file.path(R.home("bin"), "R"),
                c("CMD", "INSTALL", "--no-test-load", "-l", shQuote(lib),
                  shQuote(test_path("TestPackage"))),
                stdout = FALSE, stderr = FALSE)
        options(contractr.test.testpackage_lib = lib)
    }
    lib
}

testpackage_fixture <- function() {
    if (!"TestPackage" %in% loadedNamespaces()) {
        loadNamespace("TestPackage", lib.loc = testpackage_lib())
        suppressMessages(insert_tyr_contract("TestPackage"))
    }
    asNamespace("TestPackage")
}

## Runs `code` in a new R session and returns the output. The library path of
## that session starts with the library of `TestPackage`.
testpackage_run <- function(code) {
    libs <- paste(c(testpackage_lib(), .libPaths()), collapse = .Platform$path.sep)
    system2(file.path(R.home("bin"), "Rscript"), c("-e", shQuote(code)),
            stdout = TRUE, stderr = TRUE, env = paste0("R_LIBS=", shQuote(libs)))
}
