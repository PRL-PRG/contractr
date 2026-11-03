## Set-theoretic types.
##
## A second, parallel type path next to the tastr one based on our new set-theoretic
## types. Contractr records the declared type and the observed type of every argument 
## and return value, and leaves the question of whether one is a subtype of the other 
## to the rstt engine, offline.

## ---------------------------------------------------------------- parsing ---

## Split `s` on `sep` at nesting depth 0. Parentheses, braces and brackets nest;
## `<` opens a class list and its `>` closes it, but the `>` of an arrow `->`
## does not.
tyr_split_top <- function(s, sep) {
    chars <- strsplit(s, "", fixed = TRUE)[[1L]]
    out <- character(0)
    buf <- ""
    depth <- 0L
    instr <- FALSE
    prev <- ""
    for (ch in chars) {
        if (instr) {
            buf <- paste0(buf, ch)
            if (ch == "\"") instr <- FALSE
            prev <- ch
            next
        }
        if (ch == "\"") {
            instr <- TRUE
            buf <- paste0(buf, ch)
            prev <- ch
            next
        }
        if (ch %in% c("(", "{", "[", "<")) depth <- depth + 1L
        if (ch %in% c(")", "}", "]") || (ch == ">" && prev != "-")) depth <- depth - 1L
        if (ch == sep && depth == 0L) {
            out <- c(out, buf)
            buf <- ""
        } else {
            buf <- paste0(buf, ch)
        }
        prev <- ch
    }
    c(out, buf)
}

## Paren/brace/bracket depth of every character position.
tyr_depths <- function(chars) {
    d <- integer(length(chars))
    cur <- 0L
    instr <- FALSE
    for (i in seq_along(chars)) {
        ch <- chars[[i]]
        if (instr) {
            d[i] <- cur
            if (ch == "\"") instr <- FALSE
            next
        }
        if (ch == "\"") {
            instr <- TRUE
            d[i] <- cur
            next
        }
        if (ch %in% c("(", "{", "[")) {
            d[i] <- cur
            cur <- cur + 1L
            next
        }
        if (ch %in% c(")", "}", "]")) {
            cur <- cur - 1L
            d[i] <- cur
            next
        }
        d[i] <- cur
    }
    d
}

## The arrows that belong to the signature itself: the `->` occurrences at the
## shallowest nesting depth. Anything deeper types a parameter that is itself a
## function.
tyr_arrows <- function(chars, d) {
    arrows <- which(chars == "-" & c(chars[-1L], "") == ">")
    if (!length(arrows)) return(integer(0))
    arrows[d[arrows] == min(d[arrows])]
}

#' Parse a signature into its parameters and its result
#'
#' @param signature a type signature, e.g. `(x: dbl, digits: int1?) -> CHR`.
#'   Intersections of arrows (overloads) are accepted; the parameter names come
#'   from the first arrow and each parameter's type is the union of what the
#'   overloads declare for it.
#' @return A list with `params` (character), `types` (character, one per
#'   parameter) and `ret` (character), or `NULL` if the signature is not a
#'   function type.
#' @export
tyr_parse_signature <- function(signature) {
    chars <- strsplit(signature, "", fixed = TRUE)[[1L]]
    d <- tyr_depths(chars)
    arrows <- tyr_arrows(chars, d)
    if (!length(arrows)) return(NULL)

    param_names <- NULL
    param_types <- list()
    returns <- character(0)

    for (a in arrows) {
        ## the balanced (...) group immediately before the arrow
        j <- a - 1L
        while (j >= 1L && grepl("^[[:space:]]$", chars[[j]])) j <- j - 1L
        if (j < 1L || chars[[j]] != ")") next
        close <- j
        cur <- 0L
        i <- j
        repeat {
            if (chars[[i]] == ")") cur <- cur + 1L
            if (chars[[i]] == "(") cur <- cur - 1L
            if (cur == 0L) break
            i <- i - 1L
            if (i < 1L) return(NULL)
        }
        inner <- if (close - i > 1L) paste(chars[(i + 1L):(close - 1L)], collapse = "") else ""

        entries <- tyr_signature_params(inner)
        if (is.null(param_names)) {
            param_names <- entries$names
            param_types <- as.list(entries$types)
        } else if (identical(entries$names, param_names)) {
            for (k in seq_along(param_types)) {
                param_types[[k]] <- unique(c(param_types[[k]], entries$types[[k]]))
            }
        }

        ## the result: from just after the arrow to the end of this arrow's
        ## group -- either a `&` at the same depth or the enclosing `)`
        start <- a + 2L
        end <- length(chars)
        k <- start
        depth <- d[[a]]
        while (k <= length(chars)) {
            if (d[[k]] < depth || (d[[k]] == depth && chars[[k]] == "&")) {
                end <- k - 1L
                break
            }
            k <- k + 1L
        }
        if (end >= start) returns <- c(returns, trimws(paste(chars[start:end], collapse = "")))
    }

    if (is.null(param_names)) return(NULL)

    list(params = param_names,
         types = vapply(param_types, function(t) paste(t, collapse = " | "), character(1)),
         ret = paste(unique(returns), collapse = " | "))
}

## `name: type, name: type` -> names and types
tyr_signature_params <- function(plist) {
    plist <- trimws(plist)
    if (!nzchar(plist)) return(list(names = character(0), types = character(0)))
    parts <- Filter(function(x) nzchar(trimws(x)), tyr_split_top(plist, ","))
    nms <- character(0)
    tys <- character(0)
    for (p in parts) {
        kv <- tyr_split_top(p, ":")
        if (length(kv) < 2L) {
            nms <- c(nms, "")
            tys <- c(tys, trimws(p))
        } else {
            nms <- c(nms, trimws(kv[[1L]]))
            tys <- c(tys, trimws(paste(kv[-1L], collapse = ":")))
        }
    }
    list(names = nms, types = tys)
}

## ---------------------------------------------------------------- import ---

#' Import set-theoretic type declarations for a package
#'
#' @param package_name name of the package the declarations belong to.
#' @param filepath a file of `<name> : <signature>` lines (` : ` separated;
#'   names never contain spaces). Blank lines and `#` comments are ignored.
#' @return Invisibly, the names that were declared.
#' @export
tyr_import <- function(package_name, filepath) {
    stopifnot(is_scalar_character(package_name))
    stopifnot(file.exists(filepath))

    lines <- readLines(filepath, warn = FALSE)
    declared <- character(0)

    for (line in lines) {
        line <- trimws(line)
        if (!nzchar(line) || startsWith(line, "#")) next
        at <- regexpr(" : ", line, fixed = TRUE)
        if (at == -1L) {
            warning("cannot parse type declaration: ", line)
            next
        }
        name <- trimws(substr(line, 1L, at - 1L))
        signature <- trimws(substr(line, at + 3L, nchar(line)))

        parsed <- tyr_parse_signature(signature)
        if (is.null(parsed)) {
            warning("not a function signature, ignored: ", line)
            next
        }

        ## `T?` is a field type, not a type: the `?` says the argument may be
        ## omitted, which is a property of the parameter. Keep them apart, so
        ## that what is recorded as the expected type really is a type.
        optional <- grepl("[?][[:space:]]*$", parsed$types)
        plain <- trimws(sub("[?][[:space:]]*$", "", parsed$types))
        .Call(C_tyr_declare, package_name, name, signature,
              parsed$params, plain, optional, parsed$ret)
        declared <- c(declared, name)
    }

    invisible(declared)
}

#' @export
tyr_function_names <- function(package_name) {
    .Call(C_tyr_function_names, package_name)
}

#' @export
tyr_package_names <- function() {
    .Call(C_tyr_package_names)
}

#' @export
tyr_clear <- function() {
    invisible(.Call(C_tyr_clear))
}

#' The set-theoretic type of a value
#'
#' @param value any R value.
#' @return The type, as a string in the set-theoretic syntax.
#' @export
tyr_infer_type <- function(value) {
    .Call(C_tyr_infer_type, as.symbol("value"), sys.frame(sys.nframe()))
}

## -------------------------------------------------------------- injection ---

## Where the call being recorded was written: `file:line` of the innermost
## call on the stack that carries a source reference. That is the call site of
## the instrumented function itself -- a line of the file under test when the
## caller is test or example code, a line of the package's own sources when one
## of its functions called another. NA when nothing was parsed with
## `keep.source` (then there is no source reference anywhere on the stack).
tyr_call_site <- function() {
    calls <- sys.calls()
    for (i in rev(seq_along(calls))) {
        ref <- attr(calls[[i]], "srcref")
        if (is.null(ref)) next
        src <- attr(ref, "srcfile")
        file <- if (is.null(src) || is.null(src$filename)) "?" else src$filename
        return(paste0(file, ":", ref[[1L]]))
    }
    NA_character_
}

tyr_argval_contract_code <- function(fun_name, pkg_name, type_index) {
    substitute({
        ## The call trace is not the deparsed call -- deparsing on every
        ## invocation is pure cost once a test suite runs tens of thousands of
        ## them -- but the source location the call was written at.
        .contractr__contract <- .Call(contractr:::C_create_tyr_result_contract,
                                      contractr:::get_next_call_id(),
                                      contractr:::tyr_call_site(),
                                      PKG_NAME,
                                      FUN_NAME,
                                      TYPE_INDEX)
        .Call(contractr:::C_insert_function_contract,
              .contractr__contract, sys.function(),
              sys.frame(sys.nframe()))
    }, list(PKG_NAME = pkg_name, FUN_NAME = fun_name, TYPE_INDEX = type_index))
}

tyr_insert_function_contract <- function(fun, fun_name, pkg_name, env, type_index) {
    if (has_contract(fun)) {
        return(invisible(FALSE))
    }
    id <- injectr:::sexp_address(fun)
    old <- injectr:::create_duplicate(fun)
    ## inject_code() substitutes its argument, so the code must arrive as a
    ## variable holding the language object, not as a call producing it
    check_params <- tyr_argval_contract_code(fun_name, pkg_name, type_index)
    injectr::inject_code(check_params, fun)
    check_retval <- create_retval_contract_code(fun_name, pkg_name)
    injectr::inject_code(check_retval, fun, where = "onexit")
    add_injected_function(id, list(env = env, pkg_name = pkg_name,
                                   fun_name = fun_name, new = fun, old = old))
    invisible(TRUE)
}

## Every function reachable from a package namespace: the namespace bindings
## themselves, plus the closures held in the environments those functions come
## from. Packages written in the "standalone" style keep most of their code in
## `local({ ... })` blocks, which are invisible from the namespace.
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

    ## the local() environments behind those functions
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

    ## name each such environment after the namespace binding that owns it
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

#' Insert recording contracts into every declared function of a package
#'
#' @param package_name name of an installed, loaded package.
#' @param filepath the type declarations to import; by default the package's
#'   own `inst/TYPEDECLARATION.typr`.
#' @return Invisibly, a data frame of what was instrumented.
#' @export
insert_tyr_contract <- function(package_name,
                                filepath = system.file("TYPEDECLARATION.typr",
                                                       package = package_name)) {
    stopifnot(is_scalar_character(package_name))
    if (!nzchar(filepath) || !file.exists(filepath)) {
        message("no set-theoretic type declarations for package ", package_name)
        return(invisible(NULL))
    }

    tyr_import(package_name, filepath)
    declared <- tyr_function_names(package_name)
    ## `owner::name` -> the trailing component, for matching nested definitions
    short <- sub("^.*::", "", declared)

    targets <- tyr_targets(package_name)
    done <- character(0)
    rows <- list()

    for (t in targets) {
        candidates <- character(0)
        if (!is.na(t$owner)) candidates <- c(candidates, paste0(t$owner, "::", t$name))
        candidates <- c(candidates, t$name)
        hit <- candidates[candidates %in% declared]
        if (!length(hit)) {
            ## a nested definition whose owner could not be named: accept a
            ## unique match on the trailing component
            same <- declared[short == t$name]
            if (length(same) == 1L) hit <- same
        }
        if (!length(hit)) next
        name <- hit[[1L]]
        if (has_contract(t$fun)) {
            ## already instrumented under another name (the same closure is
            ## reachable as an alias and as its definition)
            done <- unique(c(done, name))
            next
        }
        ## Note: a name can legitimately match *several* closures. Lazy loading
        ## deserialises each top-level binding separately, so the namespace copy
        ## of a function and the copy inside the `local()` environment its
        ## siblings call are different objects; both must be instrumented or the
        ## package's internal calls go unrecorded.

        index <- .Call(C_tyr_get_index, package_name, name)
        inserted <- tryCatch(
            tyr_insert_function_contract(t$fun, name, package_name, t$env, index),
            error = function(e) {
                warning(sprintf("cannot instrument %s::%s: %s", package_name, name, conditionMessage(e)))
                FALSE
            })
        if (isTRUE(inserted)) {
            done <- unique(c(done, name))
            rows[[length(rows) + 1L]] <- data.frame(name = name,
                                                    binding = t$name,
                                                    stringsAsFactors = FALSE)
        }
    }

    missed <- setdiff(declared, done)
    if (length(missed)) {
        message(sprintf("%s: %d declared function(s) not found at run time: %s",
                        package_name, length(missed),
                        paste(utils::head(missed, 10L), collapse = ", ")))
    }
    message(sprintf("%s: instrumented %d function(s)", package_name, length(done)))

    invisible(if (length(rows)) do.call(rbind, rows) else NULL)
}
