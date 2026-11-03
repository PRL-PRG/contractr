## The subtyping oracle: the rstt REPL, controlled through processx.
##
## contractr uses one REPL for the full session. It writes one query, then it
## reads the output until the `> ` prompt. Then it writes the next query. When
## a command does not parse, the REPL discards its lexer buffer. Thus, a query
## in the buffer at that time gets no result. contractr reads until the prompt,
## not for a number of lines, because rstt breaks long types into more lines.
##
## contractr keeps the result of each query, with the text of the query as the
## key. A test suite sends the same queries many times.

RSTT_PROMPT <- "> "

.tyr_oracle <- new.env(parent = emptyenv())
.tyr_oracle$repl <- NULL
.tyr_oracle$memo <- new.env(hash = TRUE, parent = emptyenv())
.tyr_oracle$warned <- FALSE

#' Find the rstt type engine
#'
#' Finds the rstt REPL. It uses `options(contractr.rstt=)` first, then the
#' `CONTRACTR_RSTT` environment variable, then the `PATH`.
#' @return The path, or `""` if contractr cannot find rstt.
#' @export
tyr_rstt_path <- function() {
    p <- getOption("contractr.rstt", "")
    if (!nzchar(p)) p <- Sys.getenv("CONTRACTR_RSTT")
    if (!nzchar(p)) p <- Sys.which("rstt")
    if (!nzchar(p) || !file_test("-f", p)) return("")
    normalizePath(p)
}

## Sends one query `T1 <= T2` (without `;;`) to rstt. Returns `TRUE`, `FALSE`,
## or `NA` if there is no result. There is no result if rstt is not available,
## cannot parse the query, stops, or does not give a result before the time
## limit. An `NA` result never gives a violation. If contractr sent the same
## query before, it returns the same result again.
tyr_ask <- function(query) {
    hit <- .tyr_oracle$memo[[query]]
    if (!is.null(hit)) {
        tyr_log(2, "    rstt: %s => %s (cached)", query,
                if (is.na(hit)) "no result" else tolower(hit))
        return(hit)
    }
    answer <- rstt_answer(query)
    verdict <- if (identical(answer, "true")) TRUE
               else if (identical(answer, "false")) FALSE
               else NA
    tyr_log(2, "    rstt: %s => %s", query, if (is.na(answer)) "no result" else answer)
    assign(query, verdict, envir = .tyr_oracle$memo)
    verdict
}

#' Clear the query results and stop the rstt REPL
#'
#' The next query starts a new REPL. contractr finds it again with
#' [tyr_rstt_path()].
#' @export
tyr_reset_oracle <- function() {
    rstt_stop()
    .tyr_oracle$memo <- new.env(hash = TRUE, parent = emptyenv())
    .tyr_oracle$warned <- FALSE
    invisible(NULL)
}

rstt_warn <- function(message) {
    if (!.tyr_oracle$warned) {
        .tyr_oracle$warned <- TRUE
        warning("contractr: ", message,
                "; tyr calls are recorded with verdict 'unknown'",
                call. = FALSE)
    }
}

rstt_stop <- function() {
    repl <- .tyr_oracle$repl
    .tyr_oracle$repl <- NULL
    if (!is.null(repl)) try(repl$kill(), silent = TRUE)
}

rstt_start <- function() {
    if (!requireNamespace("processx", quietly = TRUE)) {
        rstt_warn("package 'processx' is needed to ask rstt")
        return(NULL)
    }
    path <- tyr_rstt_path()
    if (!nzchar(path)) {
        rstt_warn("cannot find the rstt type engine (set options(contractr.rstt=) or CONTRACTR_RSTT)")
        return(NULL)
    }
    repl <- processx::process$new(path, stdin = "|", stdout = "|",
                                  stderr = "2>&1", cleanup = TRUE)
    rstt_read(repl, 30)            # Read the banner.
    .tyr_oracle$repl <- repl
    repl
}

rstt_read <- function(repl, timeout) {
    buf <- ""
    deadline <- Sys.time() + timeout
    while (!endsWith(buf, RSTT_PROMPT)) {
        left <- as.numeric(deadline - Sys.time(), units = "secs")
        if (left <= 0) stop("rstt did not answer within ", timeout, "s")
        repl$poll_io(as.integer(min(left, 1) * 1000))
        chunk <- repl$read_output()
        if (!nzchar(chunk) && !repl$is_alive()) {
            chunk <- repl$read_output()
            if (!nzchar(chunk)) stop("rstt exited")
        }
        buf <- paste0(buf, chunk)
    }
    substr(buf, 1L, nchar(buf) - nchar(RSTT_PROMPT))
}

## Returns the output of rstt for `query` on one line. Returns `NA` if rstt
## gives no output or an error, or if contractr cannot send the query.
rstt_answer <- function(query) {
    if (grepl(";;", query, fixed = TRUE) || rstt_unbalanced_quotes(query))
        return(NA_character_)
    timeout <- getOption("contractr.rstt.timeout", 60)
    tryCatch({
        repl <- .tyr_oracle$repl
        if (is.null(repl) || !repl$is_alive()) repl <- rstt_start()
        if (is.null(repl)) return(NA_character_)
        repl$write_input(paste0(query, " ;;\n"))
        out <- trimws(gsub("[[:space:]]+", " ", rstt_read(repl, timeout)))
        if (!nzchar(out) || startsWith(out, "[Error]")) NA_character_ else out
    }, error = function(e) {
        ## This query gets no result. The next query starts a new REPL.
        rstt_stop()
        NA_character_
    })
}

rstt_unbalanced_quotes <- function(query) {
    unescaped <- gsub("\\\\.", "", query)
    lengths(regmatches(unescaped, gregexpr("\"", unescaped, fixed = TRUE))) %% 2L == 1L
}
