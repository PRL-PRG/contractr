## Checks of full calls against the types that tyr infers.
##
## `tyr --typedecl FILE` writes the type of each function that tyr typed. It
## writes one `#|` signature for each overload (see `tyr_import_calls`). The
## signatures of a function give three types:
##
## - `F`, the type of the function.
## - `D`, the domain of the function.
## - `D_rigid`, the domain with rigid type variables.
##
## Each monitored call keeps a call-site type `C` (the `@(...)` type of rstt).
## `C` contains the arguments that the caller supplied. An argument that R
## forced before the exit of the call has the type of its value. An argument
## that R did not force has the type `any`. An argument that the caller did not
## supply is not in `C`.
##
## contractr examines a call only at its exit. Three queries to rstt give the
## verdicts of the call (`R` is the type of the return value):
##
##   C <= D                         the call is in the domain
##   C & D_rigid <= empty           domain violation (blame: caller)
##   F <= ((C & D_rigid) -> ~R)     return violation (blame: callee)
##
## contractr does not change the argument promises, and it does not force a
## promise that R does not force. Thus, contractr does not blame the caller for
## an argument that the body does not use. contractr examines each monitored
## call independently. If a value goes through a chain of monitored calls, each
## call of the chain can give a violation. The exit of an inner call occurs
## before the exit of the outer call. Thus, contractr gives the violation of the
## inner call first.

## "pkg::name" -> list(type, domain, domain_rigid)
.tyr_calls <- new.env(hash = TRUE, parent = emptyenv())

## ---------------------------------------------------------------- import ---

## A declaration file is R code. Its `#|` comments are signatures, in the
## syntax that tyr uses for its preludes. Thus, the file is also a prelude for
## tyr:
##
##   #| pkg::name : (params) -> result
##
## The type of a name is the intersection of the arrows on its lines. The type
## variables of a line are not related to the type variables of the other lines.

## Returns `s` with a space for each character of its string literals. Thus, a
## search for brackets, arrows and variables finds them only in the type.
tyr_mask_strings <- function(s) {
    m <- gregexpr('"(?:[^"\\\\]|\\\\.)*"', s, perl = TRUE)
    found <- regmatches(s, m)[[1L]]
    if (length(found))
        regmatches(s, m) <- list(strrep(" ", nchar(found)))
    s
}

## Splits `(params) -> result` into `c(domain, result)`. Returns `NULL` if `sig`
## is not a signature.
tyr_split_signature <- function(sig) {
    sig <- trimws(sig)
    chars <- strsplit(tyr_mask_strings(sig), "", fixed = TRUE)[[1L]]
    if (!length(chars) || chars[[1L]] != "(") return(NULL)
    depth <- cumsum((chars %in% c("(", "[", "{")) - (chars %in% c(")", "]", "}")))
    close <- which(depth == 0L)[1L]
    if (is.na(close)) return(NULL)
    rest <- substring(sig, close + 1L)
    if (!grepl("^\\s*->", rest)) return(NULL)
    result <- trimws(sub("^\\s*->", "", rest))
    if (!nzchar(result)) return(NULL)
    c(substr(sig, 1L, close), result)
}

## Matches a type variable `'a` or a row variable `` `a ``. An identifier can
## contain a quote (`x'`). Thus, a variable starts with a quote that does not
## follow an identifier character.
TYR_VAR <- "(?<![A-Za-z0-9_.'#])(['`])[A-Za-z][A-Za-z0-9_]*"

## Replaces each variable `v` in `s` with `rename(v)`. The variables in string
## literals do not change.
tyr_rename_vars <- function(s, rename) {
    m <- gregexpr(TYR_VAR, tyr_mask_strings(s), perl = TRUE)
    found <- regmatches(s, m)[[1L]]
    if (length(found))
        regmatches(s, m) <- list(vapply(found, rename, character(1)))
    s
}

## Returns `F`, `D` and `D_rigid` of a function from its signatures. Each
## signature is `c(domain, result)`.
##
## In the rstt REPL, the first letter of a variable sets its kind. rstt can
## instantiate a variable with an uppercase letter (`'A`) when it does
## tallying. A variable with a lowercase letter (`'a`) is rigid. contractr must
## use the two kinds. A call is in the domain of a polymorphic function if one
## instance accepts it (`C <= D`). A call is not in the domain if no instance
## accepts it (`C & D_rigid <= empty`).
##
## With `'A`, the result of the second query is always `true`, because tallying
## can select `'A = empty`. Thus, this function gives new names to the
## variables. The variables of each signature are different from the variables
## of the other signatures. The k-th variable is `'A`, `'B`, and so on in `F`
## and `D`. It is `'a`, `'b`, and so on in `D_rigid`.
tyr_call_decl <- function(sigs) {
    var_name <- function(k, upper) {
        paste0((if (upper) LETTERS else letters)[k %% 26L + 1L],
               if (k >= 26L) k %/% 26L else "")
    }
    k <- c("'" = 0L, "`" = 0L)
    arrows <- doms <- doms_rigid <- character(0)
    for (s in sigs) {
        arrow <- paste(s[[1L]], "->", s[[2L]])
        found <- unique(regmatches(arrow, gregexpr(TYR_VAR, tyr_mask_strings(arrow), perl = TRUE))[[1L]])
        index <- integer(0)
        for (v in found) {
            sigil <- substr(v, 1L, 1L)
            index[[v]] <- k[[sigil]]
            k[[sigil]] <- k[[sigil]] + 1L
        }
        rename <- function(upper) function(v) paste0(substr(v, 1L, 1L), var_name(index[[v]], upper))
        arrows <- c(arrows, paste0("(", tyr_rename_vars(arrow, rename(TRUE)), ")"))
        doms <- c(doms, tyr_rename_vars(s[[1L]], rename(TRUE)))
        doms_rigid <- c(doms_rigid, tyr_rename_vars(s[[1L]], rename(FALSE)))
    }
    list(type = paste(arrows, collapse = " & "),
         domain = paste(doms, collapse = " | "),
         domain_rigid = paste(doms_rigid, collapse = " | "))
}

## Reads the declarations of `package_name` from `filepath`. A declaration is a
## `#|` line `pkg::name : sig`, or `name : sig` without a namespace. This
## function ignores the declarations of other namespaces. An operator name is
## in parentheses, for example `(%||%)`. If this function cannot read a `#|`
## line, it gives a warning. Returns the declared names.
tyr_import_calls <- function(package_name, filepath) {
    lines <- readLines(filepath, warn = FALSE)
    lines <- trimws(sub("^#\\|", "", lines[startsWith(lines, "#|")]))
    head <- "^(?:([^\\s:()]+)\\s*:::?\\s*)?(\\([^)]*\\)|[^\\s:()]+)\\s*:(?!:)(.*)$"
    sigs <- list()
    for (line in lines) {
        m <- regmatches(line, regexec(head, line, perl = TRUE))[[1L]]
        sig <- if (length(m)) tyr_split_signature(m[[4L]])
        if (is.null(sig)) {
            warning("cannot read tyr declaration: ", line)
            next
        }
        if (nzchar(m[[2L]]) && m[[2L]] != package_name) next
        name <- sub("^\\((.*)\\)$", "\\1", m[[3L]])
        sigs[[name]] <- c(sigs[[name]], list(sig))
    }
    for (name in names(sigs))
        assign(paste0(package_name, "::", name), tyr_call_decl(sigs[[name]]),
               envir = .tyr_calls)
    invisible(names(sigs))
}

tyr_call_declaration <- function(package_name, function_name) {
    .tyr_calls[[paste0(package_name, "::", function_name)]]
}

## ------------------------------------------------------------- injection ---

tyr_insert_call_contracts <- function(package_name, filepath) {
    declared <- tyr_import_calls(package_name, filepath)
    done <- character(0)
    rows <- list()

    for (t in tyr_targets(package_name)) {
        if (!(t$name %in% declared)) next
        if (has_contract(t$fun)) {
            done <- unique(c(done, t$name))
            next
        }
        inserted <- tryCatch(
            tyr_inject_call(t$fun, t$name, package_name, t$env),
            error = function(e) {
                warning(sprintf("cannot instrument %s::%s: %s", package_name,
                                t$name, conditionMessage(e)))
                FALSE
            })
        if (isTRUE(inserted)) {
            done <- unique(c(done, t$name))
            rows[[length(rows) + 1L]] <-
                data.frame(name = t$name, binding = t$name, stringsAsFactors = FALSE)
        }
    }

    missed <- setdiff(declared, done)
    if (length(missed)) {
        message(sprintf("%s: %d declared function(s) not found at run time: %s",
                        package_name, length(missed),
                        paste(utils::head(missed, 10L), collapse = ", ")))
    }
    tyr_log(1, "%s: instrumented %d function(s)", package_name, length(done))
    for (name in done)
        tyr_log(1, "%s::%s : %s", package_name, name,
                tyr_call_declaration(package_name, name)$type)
    invisible(if (length(rows)) do.call(rbind, rows) else NULL)
}

tyr_inject_call <- function(fun, fun_name, pkg_name, env) {
    id <- injectr:::sexp_address(fun)
    old <- injectr:::create_duplicate(fun)
    ## `inject_code()` substitutes its argument. Thus, give it a variable that
    ## contains the code.
    entry <- substitute(
        .contractr__call <- contractr:::tyr_call_enter(PKG, NAME, sys.function(), environment()),
        list(PKG = pkg_name, NAME = fun_name))
    injectr::inject_code(entry, fun)
    exit <- quote({
        .contractr__retval__ <- returnValue(contractr:::.no_retval_marker)  # nolint
        contractr:::tyr_call_exit(environment(), .contractr__retval__)
    })
    injectr::inject_code(exit, fun, where = "onexit")
    add_injected_function(id, list(env = env, pkg_name = pkg_name,
                                   fun_name = fun_name, new = fun, old = old))
    TRUE
}

## ------------------------------------------------------------- the call ---

## Runs when a monitored call starts. Returns the state of the call, an
## environment. Returns `NULL` if contracts are off or if the function has no
## declaration.
tyr_call_enter <- function(pkg, name, fun, rho) {
    if (!.Call(C_tyr_contracts_enabled)) return(NULL)
    decl <- tyr_call_declaration(pkg, name)
    if (is.null(decl)) return(NULL)

    call <- new.env(parent = emptyenv())
    call$pkg <- pkg
    call$name <- name
    call$decl <- decl
    call$id <- as.integer(get_next_call_id())
    call$site <- tyr_call_site(rho)
    call$exited <- FALSE

    slots <- .Call(C_tyr_args, fun, rho)
    call$labels <- slots$label
    call$kinds <- slots$kind
    call$state <- slots$state
    call$types <- slots$type
    call$promises <- slots$promise
    ## rstt cannot read some names, for example a `...` element with the name
    ## "my arg". A name of this type makes all queries about the call incorrect.
    call$renderable <- all(tyr_label_ok(call$labels[call$labels != ""]))
    call
}

## Runs at the exit of a monitored call. It gives the domain verdict. Then it
## examines the return value. `retval` is `.no_retval_marker` if the call did
## not return.
tyr_call_exit <- function(frame, retval) {
    call <- get0(".contractr__call", envir = frame, inherits = FALSE)
    if (!is.environment(call) || call$exited) return(invisible())
    call$exited <- TRUE
    if (!.Call(C_tyr_contracts_enabled)) return(invisible())
    .Call(C_disable_contracts)
    on.exit(.Call(C_reinstate_contract_status))

    decl <- call$decl
    returned <- !identical(retval, .no_retval_marker)

    ## An argument that R forced during the call gets the type of its value.
    for (i in which(call$state == "unforced")) {
        t <- .Call(C_tyr_forced_type, call$promises[[i]])
        if (!is.null(t)) {
            call$state[[i]] <- "typed"
            call$types[[i]] <- t
        }
    }
    actual_ret <- if (returned) .Call(C_tyr_infer_type, retval) else NA_character_
    tyr_log(1, cli::style_bold("call %d: %s::%s%s"), call$id, call$pkg, call$name,
            if (is.na(call$site)) "" else paste0(" at ", call$site))

    if (!call$renderable) {
        tyr_log(1, "  rstt cannot read an argument name: all verdicts are unknown")
        tyr_record(call, -2L, "", NA_character_, decl$domain, "unknown")
        if (returned) tyr_record(call, -1L, "", actual_ret, decl$type, "unknown")
        return(invisible())
    }

    C <- tyr_render(call)
    tyr_log(1, "  arguments %s", C)
    ## `outside` is `FALSE` only if rstt proves that some call that agrees with
    ## `C` is in the domain. It is `NA` if rstt gives no answer. Only `FALSE`
    ## makes the return query correct: if `C & D_rigid` is empty, the return
    ## query is always true.
    in_domain <- isTRUE(tyr_ask(sprintf("%s <= (%s)", C, decl$domain)))
    outside <- if (in_domain) FALSE
               else tyr_ask(sprintf("%s & (%s) <= empty", C, decl$domain_rigid))
    domain <- if (in_domain) "ok" else if (isTRUE(outside)) "violation" else "unknown"
    tyr_log(1, "  domain %s", tyr_col_verdict(domain))
    tyr_record(call, -2L, "", C, decl$domain, domain,
               blame = if (domain == "violation") "caller" else NA_character_)
    if (domain == "violation") tyr_raise(tyr_domain_message(call, C))

    if (!returned) {
        tyr_log(1, "  no result: the call did not return")
        return(invisible())
    }

    verdict <- "unknown"
    note <- ""                         # The log text if there is no verdict.
    if (isTRUE(outside)) {
        note <- "not checked (domain violation)"
    } else if (is.na(outside)) {
        note <- "not checked (no answer from rstt)"
    } else {
        ## contractr can know the value of an argument that the body did not
        ## force (a constant or a variable). That value makes the query more
        ## accurate. But contractr does not blame the caller for that value.
        Cret <- tyr_render(call, peek = TRUE)
        if (!identical(Cret, C)) tyr_log(1, "  peeked %s", Cret)
        peek_outside <- if (identical(Cret, C)) FALSE
                        else tyr_ask(sprintf("%s & (%s) <= empty", Cret, decl$domain_rigid))
        if (isTRUE(peek_outside)) {
            note <- "not checked (the peeked call is outside the domain)"
        } else if (is.na(peek_outside)) {
            note <- "not checked (no answer from rstt)"
        } else {
            answer <- tyr_ask(sprintf("(%s) <= ((%s & (%s)) -> ~(%s))",
                                      decl$type, Cret, decl$domain_rigid, actual_ret))
            if (isTRUE(answer)) verdict <- "violation"
            else if (is.na(answer)) note <- "unknown (no answer from rstt)"
            else if (domain == "ok") verdict <- "ok"
        }
    }
    tyr_log(1, "  result %s: %s", actual_ret,
            tyr_col_verdict(if (nzchar(note)) note else verdict))
    tyr_record(call, -1L, "", actual_ret, decl$type, verdict,
               blame = if (verdict == "violation") "callee" else NA_character_)
    if (verdict == "violation") tyr_raise(tyr_return_message(call, actual_ret))
    invisible()
}

## ------------------------------------------------------------ rendering ---

tyr_label_ok <- function(labels) {
    grepl("^[A-Za-z.][A-Za-z0-9._]*$", labels)
}

## Returns the call-site type of the arguments that contractr knows. The
## formals have their names. If `...` got positional elements, the formals
## before `...` are positional. rstt gives more positional arguments to `...`
## only in this condition. With `peek`, an argument that the body did not force
## gets the type of its value if contractr can get the value without an
## evaluation. If not, the argument gets `any`.
tyr_render <- function(call, peek = FALSE) {
    state <- call$state
    types <- ifelse(state == "typed", call$types, "any")
    if (peek) {
        for (i in which(state == "unforced")) {
            t <- .Call(C_tyr_peek, call$promises[[i]])
            if (!is.null(t)) types[[i]] <- t
        }
    }
    present <- state != "absent"
    kinds <- call$kinds
    labels <- call$labels
    dots_positional <- any(present & kinds == "dots" & labels == "")

    positional <- present & ((kinds == "pre" & dots_positional) |
                             (kinds == "dots" & labels == ""))
    named <- present & !positional
    ## `paste0()` of empty vectors and a constant is not empty. Thus, use `if`.
    args <- c(types[positional],
              if (any(named)) paste0(labels[named], ": ", types[named]))
    paste0("@(", paste(args, collapse = ", "), ")")
}

## ------------------------------------------------------------- recording ---

## The `origin` column is always `NA`. contractr examines each call
## independently.
tyr_record <- function(call, position, parameter, actual, expected, verdict,
                       blame = NA_character_) {
    .Call(C_tyr_record, call$id, call$site, call$pkg, call$name,
          as.integer(position), parameter, actual, expected, verdict,
          NA_integer_, blame)
}

## Writes a line to the log if `getOption("contractr.verbose")` is `level` or
## more. Level 1 shows the monitored functions and the checks of each call.
## Level 2 also shows the rstt queries and their results. The log has colors
## only if cli finds that the output stream shows colors.
tyr_log <- function(level, fmt, ...) {
    if (isTRUE(getOption("contractr.verbose", 0) >= level)) {
        line <- sprintf(fmt, ...)
        if (level >= 2) line <- cli::col_grey(line)
        message(cli::col_grey("contractr: "), line)
    }
}

## Gives "ok" in green, "violation" in red, and all other verdicts in yellow.
tyr_col_verdict <- function(verdict) {
    col <- switch(verdict, ok = cli::col_green, violation = cli::col_red, cli::col_yellow)
    col(verdict)
}

tyr_raise <- function(message) {
    severity <- get_severity()
    if (identical(severity, "silence")) return(invisible())
    if (identical(severity, "error")) stop(message, call. = FALSE)
    warning(message, call. = FALSE)
}

tyr_domain_message <- function(call, C) {
    tyr_format(c(sprintf("contract violation: call of '%s::%s' is outside its domain",
                         call$pkg, call$name),
                 sprintf("call site: %s", if (is.na(call$site)) "?" else call$site),
                 sprintf("arguments: %s", C),
                 sprintf("domain: %s", call$decl$domain),
                 "blame: caller"))
}

tyr_return_message <- function(call, actual) {
    tyr_format(c(sprintf("contract violation: return value of '%s::%s'",
                         call$pkg, call$name),
                 sprintf("call site: %s", if (is.na(call$site)) "?" else call$site),
                 sprintf("arguments: %s", tyr_render(call, peek = TRUE)),
                 sprintf("returned: %s", actual),
                 sprintf("type: %s", call$decl$type),
                 "blame: callee"))
}

tyr_format <- function(lines) {
    n <- length(lines)
    prefix <- c("", rep("   ├── ", max(0L, n - 2L)), "   └── ")
    paste(paste0(prefix[seq_len(n)], lines), collapse = "\n")
}
