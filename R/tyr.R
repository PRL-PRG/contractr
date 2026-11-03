## The entry point for set-theoretic types. This file also has the functions
## that the checks in `tyr_call.R` use. They find the functions of a package
## and give the type of a value.

#' Get the set-theoretic type of a value
#'
#' Returns the type that contractr gives to a value. The type contains the
#' value, and the subtyping results for the type are correct. For example, the
#' type of a vector that is not a scalar is `(MODE \\ MODE1)`, not `MODE`. The
#' type of a function is `empty -> any`.
#'
#' @param value An R value.
#' @return The type, as a string in the set-theoretic syntax.
#' @export
tyr_infer_type <- function(value) {
    .Call(C_tyr_infer_type, value)
}

## Returns the place of the monitored call in the source, as `file:line`.
## `rho` is the frame of the monitored call. The search starts at the call of
## this frame and goes down the stack. The first call that has a source
## reference gives the place. Usually, that call is the call of the monitored
## function. The search does not examine the calls above `rho`. These are the
## calls of contractr, which have source references if R keeps the source of
## contractr.
##
## When test or example code calls the function, the place is a line of that
## code. When a function of the package calls it, the place is a line of the
## package sources. Returns `NA` if R parsed no code with `keep.source`,
## because then no call has a source reference.
tyr_call_site <- function(rho) {
    calls <- sys.calls()
    top <- Position(function(frame) identical(frame, rho), sys.frames(), right = TRUE)
    if (is.na(top)) return(NA_character_)
    for (i in rev(seq_len(top))) {
        ref <- attr(calls[[i]], "srcref")
        if (is.null(ref)) next
        src <- attr(ref, "srcfile")
        file <- if (is.null(src) || is.null(src$filename)) "?" else src$filename
        return(paste0(file, ":", ref[[1L]]))
    }
    NA_character_
}

## Returns all functions that contractr can get from a package namespace. These
## are the functions in the namespace and the closures in the environments of
## these functions. Packages in the "standalone" style keep most of their code
## in `local({ ... })` blocks. The namespace does not show these blocks.
tyr_targets <- function(package_name) {
    ns <- getNamespace(package_name)
    targets <- list()

    add <- function(name, fun, env, owner) {
        targets[[length(targets) + 1L]] <<-
            list(name = name, fun = fun, env = env, owner = owner)
    }

    ns_names <- ls(ns, all.names = TRUE)
    ns_values <- lapply(ns_names, function(n) tryCatch(get(n, envir = ns), error = function(e) NULL))
    names(ns_values) <- ns_names

    for (n in ns_names) {
        v <- ns_values[[n]]
        if (is.function(v)) add(n, v, ns, NA_character_)
    }

    ## Find the `local()` environments of these functions.
    envs <- list()
    for (n in ns_names) {
        v <- ns_values[[n]]
        if (!is.function(v)) next
        e <- environment(v)
        if (is.null(e) || identical(e, ns) || isNamespace(e) ||
            identical(e, globalenv()) || identical(e, baseenv()) ||
            identical(e, emptyenv())) next
        if (!any(vapply(envs, identical, logical(1), e))) envs[[length(envs) + 1L]] <- e
    }

    ## Give each of these environments the name of the namespace list that
    ## refers to it.
    owner_of <- function(e) {
        for (n in ns_names) {
            v <- ns_values[[n]]
            if (is.list(v)) {
                for (el in v) {
                    if (is.environment(el) && identical(el, e)) return(n)
                    if (is.function(el) && identical(environment(el), e)) return(n)
                }
            }
        }
        NA_character_
    }

    for (e in envs) {
        owner <- owner_of(e)
        for (n in ls(e, all.names = TRUE)) {
            v <- tryCatch(get(n, envir = e), error = function(err) NULL)
            if (!is.function(v)) next
            add(n, v, e, owner)
        }
    }

    targets
}

#' Monitor the calls of the declared functions of a package
#'
#' Adds contracts to each function of a package that has a declaration. Then
#' contractr examines each call of these functions at the exit of the call. It
#' sends the queries to the rstt REPL (see [tyr_rstt_path()]). [set_severity()] sets
#' what contractr does for a violation. [get_contracts()] gives all verdicts.
#'
#' `filepath` is a file that `tyr --typedecl` writes. It has one
#' `#| pkg::name : <signature>` line for each overload.
#'
#' `options(contractr.verbose = 1)` writes a log as messages. The log shows
#' each monitored function with its type, and the checks of each call. With
#' `options(contractr.verbose = 2)`, the log also shows each rstt query and its
#' result.
#'
#' @param package_name The name of a loaded package.
#' @param filepath The file of declarations. The default is the `TYR` file of
#'   the package (`inst/TYR` in the source).
#' @return A data frame of the functions that got contracts, or `NULL`. The
#'   value is invisible.
#' @export
insert_tyr_contract <- function(package_name,
                                filepath = tyr_declaration_file(find.package(package_name))) {
    stopifnot(is_scalar_character(package_name))
    if (!nzchar(filepath)) stop("no type declarations (inst/TYR) for package ", package_name)
    stopifnot(file.exists(filepath))
    tyr_insert_call_contracts(package_name, filepath)
}
