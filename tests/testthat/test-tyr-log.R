## These tests examine the log that `options(contractr.verbose=)` enables.
## Level 1 shows the monitored functions and the checks of each call. Level 2
## also shows the rstt queries and their results.

tyr_skip_unless_rstt()
tc <- tyr_fixture()

log_of <- function(r) paste(r$messages, collapse = "")

test_that("level 0 writes no log", {
    withr::local_options(contractr.verbose = NULL)
    r <- tyr_run(tc$m_xy(1L, TRUE), ret = TRUE)
    expect_length(r$messages, 0L)
})

test_that("level 1 shows the monitored functions and their types", {
    withr::local_options(contractr.verbose = 1)
    m <- paste(capture_messages(
        insert_tyr_contract("tyrcalls", test_path("tyrcalls.tyr"))), collapse = "")
    expect_match(m, "tyrcalls::m_xy : ((x: LGL, y: INT) -> LGL) & ((x: INT, y: LGL) -> LGL1)",
                 fixed = TRUE)
})

test_that("level 1 shows the checks of a call", {
    withr::local_options(contractr.verbose = 1)
    log <- log_of(tyr_run(tc$m_xy(1L, TRUE), ret = TRUE))
    expect_match(log, "call [0-9]+: tyrcalls::m_xy at ")
    expect_match(log, "arguments @(x: ^INT1<> with {  }, y: ^LGL1<> with {  })", fixed = TRUE)
    expect_match(log, "domain ok", fixed = TRUE)
    expect_match(log, "result ^LGL1<> with {  }: ok", fixed = TRUE)
    expect_no_match(log, "rstt:", fixed = TRUE)
})

test_that("level 1 shows the peeked arguments", {
    withr::local_options(contractr.verbose = 1)
    log <- log_of(tyr_run(tc$m_x(1L, TRUE), ret = TRUE))
    expect_match(log, "arguments @(x: ^INT1<> with {  }, y: any)", fixed = TRUE)
    expect_match(log, "domain unknown", fixed = TRUE)
    expect_match(log, "peeked @(x: ^INT1<> with {  }, y: ^LGL1<> with {  })", fixed = TRUE)
})

test_that("level 1 tells why the result is not checked", {
    withr::local_options(contractr.verbose = 1)
    log <- log_of(tyr_run(tc$m_xy(TRUE, TRUE), ret = TRUE))
    expect_match(log, "domain violation", fixed = TRUE)
    expect_match(log, "result ^LGL1<> with {  }: not checked (domain violation)", fixed = TRUE)
})

test_that("the verdicts have colors if cli shows colors", {
    withr::local_options(contractr.verbose = 1, cli.num_colors = 256)
    log <- log_of(tyr_run(tc$m_xy(1L, TRUE), ret = TRUE))
    expect_match(log, paste0("domain ", cli::col_green("ok")), fixed = TRUE)
    log <- log_of(tyr_run(tc$m_xy(TRUE, TRUE), ret = TRUE))
    expect_match(log, paste0("domain ", cli::col_red("violation")), fixed = TRUE)
})

test_that("level 2 shows the rstt queries and their results", {
    withr::local_options(contractr.verbose = 2)
    log <- log_of(tyr_run(tc$m_xy(1L, TRUE), ret = TRUE))
    expect_match(log, "rstt: @(x: ^INT1<> with {  }, y: ^LGL1<> with {  }) <= ((x: LGL, y: INT) | (x: INT, y: LGL)) => true",
                 fixed = TRUE)
})
