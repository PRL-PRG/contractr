## These tests examine the checks of full calls (`R/tyr_call.R`) with the real
## rstt engine. Each function of the fixture package `tyrcalls/` forces its
## arguments in the order that its name gives. Then it returns
## `getOption("tyrcalls.ret")`. The file `tyrcalls.tyr` contains the types of
## these functions. The labels of the test cases, for example `m1`, `o2` and
## `b3`, come from the test matrix of the design.

tyr_skip_unless_rstt()
tc <- tyr_fixture()

expect_violation <- function(r, n = 1L, pattern = NULL) {
    expect_length(r$warnings, n)
    if (!is.null(pattern)) expect_match(r$warnings[[1L]], pattern, fixed = TRUE)
}

expect_clean <- function(r) {
    expect_length(r$warnings, 0L)
    expect_null(r$error)
}

## --------------------------------------------------- the types of values ---

test_that("observations contain their value", {
    expect_equal(tyr_infer_type(c(TRUE, FALSE)), "(LGL \\ LGL1)<> with {  }")
    expect_equal(tyr_infer_type(logical(0)), "(LGL \\ LGL1)<> with {  }")
    expect_equal(tyr_infer_type(TRUE), "^LGL1<> with {  }")
    expect_equal(tyr_infer_type(function(x) x), "empty -> any")
    expect_equal(tyr_infer_type(as.raw(1:2)), "(RAW \\ RAW1)<> with {  }")
})

test_that("the oracle answers, and remembers", {
    expect_true(contractr:::tyr_ask("INT1 <= dbl"))
    expect_false(contractr:::tyr_ask("dbl <= INT1"))
    expect_true(is.na(contractr:::tyr_ask("this is not a type")))
    expect_true(exists("INT1 <= dbl", envir = contractr:::.tyr_oracle$memo))
})

## ------------------------------------------------- scenario 1: overloads ---

test_that("s1: a scalar in, a vector out, the argument forced in another function", {
    r <- tyr_run(tc$s1(1), ret = c(TRUE, FALSE))
    expect_violation(r, pattern = "return value of 'tyrcalls::s1'")
    expect_match(r$warnings, "blame: callee", fixed = TRUE)
    expect_equal(r$domain, "ok")
    expect_equal(r$ret, "violation")
    expect_equal(r$rows$blame[r$rows$parameter_position == -1L], "callee")
})

test_that("s1: a scalar in, a scalar out", {
    r <- tyr_run(tc$s1(1), ret = TRUE)
    expect_clean(r)
    expect_equal(r$domain, "ok")
    expect_equal(r$ret, "ok")
})

test_that("s1/r7: an argument never forced is peeked when it is a constant or a bound variable", {
    r <- tyr_run(tc$s1_lazy(1), ret = c(TRUE, FALSE))
    expect_violation(r, pattern = "return value of 'tyrcalls::s1_lazy'")
    expect_equal(r$domain, "unknown")

    v <- 1
    r <- tyr_run(tc$s1_lazy(v), ret = c(TRUE, FALSE))
    expect_violation(r)

    ## contractr does not evaluate a call to get the type of an argument.
    r <- tyr_run(tc$s1_lazy(c(1)), ret = c(TRUE, FALSE))
    expect_clean(r)
    expect_equal(r$ret, "unknown")
})

## -------------------------------- F2: the type of x sets the type of y ---

test_that("m1: y forced first, the call is outside the domain", {
    r <- tyr_run(tc$m_yx(1:2, 1:2), ret = TRUE)
    expect_violation(r, pattern = "call of 'tyrcalls::m_yx' is outside its domain")
    expect_match(r$warnings, "blame: caller", fixed = TRUE)
    expect_equal(r$domain, "violation")
})

test_that("m2: x forced first, the same report as m1", {
    r <- tyr_run(tc$m_xy(1:2, 1:2), ret = TRUE)
    expect_violation(r)
    m1 <- tyr_run(tc$m_yx(1:2, 1:2), ret = TRUE)
    expect_equal(sub("m_xy", "m_yx", r$warnings, fixed = TRUE), m1$warnings)
})

test_that("m3: doubles fit no overload", {
    r <- tyr_run(tc$m_yx(c(1, 2), c(1, 2)), ret = TRUE)
    expect_violation(r)
    expect_equal(r$domain, "violation")
})

test_that("m4: x alone selects overload 2, whose result is a scalar", {
    r <- tyr_run(tc$m_x(1:2, identity(TRUE)), ret = c(TRUE, FALSE))
    expect_violation(r, pattern = "return value of 'tyrcalls::m_x'")
})

test_that("m5: the same call returning a scalar", {
    r <- tyr_run(tc$m_x(1:2, identity(TRUE)), ret = TRUE)
    expect_clean(r)
    expect_equal(r$domain, "unknown")
    expect_equal(r$ret, "unknown")
})

test_that("m6/m7: nothing forced; no overload returns a character", {
    r <- tyr_run(tc$m_none(identity(1:2), identity(TRUE)), ret = "a")
    expect_violation(r, pattern = "return value of 'tyrcalls::m_none'")
    r <- tyr_run(tc$m_none(identity(1:2), identity(TRUE)), ret = c(TRUE, FALSE))
    expect_clean(r)
    expect_equal(r$ret, "unknown")
})

test_that("m8: arguments by name, in the other order", {
    r <- tyr_run(tc$m_xy(y = 1:2, x = c(TRUE, FALSE)), ret = TRUE)
    expect_clean(r)
    expect_equal(r$domain, "ok")
    expect_equal(r$ret, "ok")
})

## ----------------------- F3: the overload changes if the call supplies y ---

test_that("o1-o3: the overload is chosen by the presence of y", {
    r <- tyr_run(tc$o_force(1:2), ret = 1:2)
    expect_clean(r)
    expect_equal(r$ret, "ok")

    r <- tyr_run(tc$o_force(1:2, TRUE), ret = 1:2)
    expect_violation(r, pattern = "return value of 'tyrcalls::o_force'")

    ## The body does not force `y`, and contractr does not evaluate the call
    ## `identity(TRUE)` to get its type. But the call supplies `y`. Thus the
    ## second overload is applicable, and this is sufficient for the violation.
    r <- tyr_run(tc$o_lazy(1:2, identity(TRUE)), ret = 1:2)
    expect_violation(r, pattern = "return value of 'tyrcalls::o_lazy'")
})

## ------------------------------------------ a union of argument lists ---

test_that("u1/u2: a union of argument lists", {
    r <- tyr_run(tc$u_ab("a", 1L))
    expect_clean(r)
    expect_equal(r$domain, "ok")

    r <- tyr_run(tc$u_ab("a", "b"))
    expect_violation(r)
    expect_equal(r$domain, "violation")
})

## ----------------------------------------------------------------- dots ---

test_that("d1-d6: dots", {
    r <- tyr_run(tc$d_all(1:2, 1, 2), ret = 1:2)
    expect_clean(r)
    expect_equal(r$domain, "ok")
    expect_equal(r$ret, "ok")

    ## The type of `x` selects the overload. The overload sets the type of the
    ## dots.
    r <- tyr_run(tc$d_all(1:2, 1, "a"), ret = 1:2)
    expect_violation(r)

    r <- tyr_run(tc$d_all(x = 1:2, foo = "a"), ret = 1:2)
    expect_violation(r)

    r <- tyr_run(tc$d_all(1:2, 1), ret = c(TRUE, FALSE))
    expect_violation(r, pattern = "return value of 'tyrcalls::d_all'")

    ## In the call-site type, the arguments before `...` have no names. A formal
    ## after `...` has its name.
    r <- tyr_run(tc$d_post(1:2, 1, z = TRUE))
    expect_clean(r)
    expect_equal(r$domain, "ok")
    expect_match(r$rows$actual_type[r$rows$parameter_position == -2L],
                 "^@\\(\\(INT .*, \\^DBL1.*, z: \\^LGL1")

    r <- tyr_run(tc$d_lazy(1:2, identity(1), identity(2)), ret = 1:2)
    expect_clean(r)
    expect_equal(r$domain, "unknown")
})

## ------------------------------------------------------- polymorphism ---

test_that("tag: the identity", {
    r <- tyr_run(tc$tag(1), ret = "a")
    expect_violation(r, pattern = "return value of 'tyrcalls::tag'")
    r <- tyr_run(tc$tag(1), ret = 2)
    expect_clean(r)
    expect_equal(r$ret, "ok")
    ## One call gives one domain row and one return row (r4: the body forces `x`
    ## one time).
    expect_equal(nrow(r$rows), 2L)
})

test_that("p1-p7: several arguments sharing variables", {
    r <- tyr_run(tc$p_aa(1, "a"), ret = 2)
    expect_clean(r)
    expect_equal(r$domain, "ok")
    expect_equal(r$ret, "ok")

    r <- tyr_run(tc$p_aa(1, "a"), ret = TRUE)
    expect_violation(r, pattern = "return value of 'tyrcalls::p_aa'")

    ## `p_ab` returns a value of the type of `x`, not of the type of `y`.
    r <- tyr_run(tc$p_ab(1, "a"), ret = 2)
    expect_violation(r, pattern = "return value of 'tyrcalls::p_ab'")

    ## The body does not force `y`. But `'B` can be any type. Thus the call is
    ## in the domain, and no return value can be incorrect. The verdict "ok"
    ## means only that.
    r <- tyr_run(tc$p_ab_lazy(1, identity("a")), ret = 2)
    expect_clean(r)
    expect_equal(r$domain, "ok")
    expect_equal(r$ret, "ok")

    r <- tyr_run(tc$p_bound("a", 1))
    expect_violation(r)

    r <- tyr_run(tc$p_bound(1, "a"))
    expect_clean(r)
    expect_equal(r$domain, "ok")
})

## ------------------------------------------------ the records of calls ---

test_that("r1: a required argument not passed", {
    r <- tyr_run(tc$r_req(1:2))
    expect_violation(r, pattern = "call of 'tyrcalls::r_req' is outside its domain")
    expect_equal(r$domain, "violation")
})

test_that("r3: recursion keeps calls apart; each call gives its violation", {
    r <- tyr_run(tc$r_rec(2, 1:2))
    expect_clean(r)
    expect_equal(r$domain, rep("ok", 3L))
    expect_equal(r$ret, rep("ok", 3L))
    expect_length(unique(r$rows$call_id), 3L)

    r <- tyr_run(tc$r_rec(2, "a"))
    expect_violation(r, n = 3L)
    dom <- r$rows[r$rows$parameter_position == -2L, ]
    expect_equal(dom$verdict, rep("violation", 3L))
    ## The innermost call exits first.
    expect_equal(dom$call_id, sort(dom$call_id, decreasing = TRUE))
    expect_true(all(is.na(dom$origin)))
})

test_that("r6: a default of another type is not part of the call", {
    r <- tyr_run(tc$r_def(1:2))
    expect_clean(r)
    expect_equal(r$domain, "ok")
})

test_that("r7b: an argument never used is never blamed", {
    r <- tyr_run(tc$r_unused("a", 1L), ret = 1L)
    expect_clean(r)
    expect_equal(r$domain, "unknown")
    expect_equal(r$ret, "unknown")
})

test_that("r8: partial matching uses the formal's name", {
    r <- tyr_run(tc$r_pm(te = "a", n = 1L))
    expect_clean(r)
    expect_match(r$rows$actual_type[[1L]], "text: ", fixed = TRUE)
})

test_that("r9: an error after a violation; no return check", {
    r <- tyr_run(tc$r_err("a"))
    expect_violation(r)
    expect_equal(r$error, "inner")
    expect_length(r$ret, 0L)
})

test_that("r10: no checks while contracts are disabled", {
    r <- tyr_run(ignore_contracts(tc$tag(1)), ret = "a")
    expect_clean(r)
    expect_equal(nrow(r$rows), 0L)
})

test_that("missing() still sees an argument that forwards a missing one", {
    r <- tyr_run(tc$r_missing_fwd(1:2))
    expect_true(r$value)
    expect_clean(r)
    ## The call of `r_missing` does not contain `y`. No value and no default is
    ## behind it.
    calls <- r$rows[r$rows$function_name == "r_missing" & r$rows$parameter_position == -2L, ]
    expect_equal(calls$actual_type, "@(x: (INT \\ INT1)<> with {  })")
})

test_that("a default passed on is an argument, checked at exit", {
    ## The result of `missing(y)` in `r_missing` changes with the R version (R
    ## 4.5.2 gives FALSE). The contracts do not change this result. The call
    ## supplies `y` (its default is behind it), but no body forces it. Thus
    ## the type of `y` is `any`.
    r <- tyr_run(tc$r_def_fwd(1:2))
    expect_identical(r$value, ignore_contracts(tc$r_def_fwd(1:2)))
    expect_clean(r)
    calls <- r$rows[r$rows$function_name == "r_missing" & r$rows$parameter_position == -2L, ]
    expect_equal(calls$actual_type, "@(x: (INT \\ INT1)<> with {  }, y: any)")

    ## `c_h` forces `t`, which has its default value. contractr gets the type
    ## of `t` when `c_h` returns.
    r <- tyr_run(tc$c_def())
    expect_clean(r)
    expect_equal(r$domain, c("ok", "ok"))

    r <- tyr_run(tc$c_def_bad())
    expect_violation(r, pattern = "call of 'tyrcalls::c_h' is outside its domain")
})

## ------------------------------------- blame through chains of promises ---

test_that("b1: a value through a chain gives one violation for each call, the innermost first", {
    r <- tyr_run(tc$c_f("string"))
    expect_violation(r, n = 3L, pattern = "call of 'tyrcalls::c_h' is outside its domain")
    dom <- r$rows[r$rows$parameter_position == -2L, ]
    expect_equal(dom$function_name, c("c_h", "c_g", "c_f"))
    expect_equal(dom$verdict, rep("violation", 3L))
    expect_true(all(is.na(dom$origin)))
    expect_equal(dom$blame, rep("caller", 3L))
})

test_that("b2/g4: under severity error, the error of the outermost call stays", {
    ## Each exit raises an error while R unwinds the error of the inner call.
    ## `tryCatch()` gets the last error.
    r <- tyr_run(tc$c_f("string"), severity = "error")
    expect_length(r$warnings, 0L)
    expect_match(r$error, "call of 'tyrcalls::c_f' is outside its domain", fixed = TRUE)
    expect_equal(sum(r$rows$verdict == "violation"), 3L)
})

test_that("b3: only the downstream contracts fail", {
    ## The domain of `c3_f` is `(x: INT)` and accepts `1:2`. The domains of
    ## `c3_g` and `c3_h` are `(x: ^INT1)` and do not accept it.
    r <- tyr_run(tc$c3_f(1:2))
    expect_violation(r, n = 2L, pattern = "call of 'tyrcalls::c3_h' is outside its domain")
    expect_match(r$warnings[[2L]], "call of 'tyrcalls::c3_g' is outside its domain", fixed = TRUE)
    dom <- r$rows[r$rows$parameter_position == -2L, ]
    expect_equal(dom$verdict[dom$function_name == "c3_f"], "ok")
})

test_that("b4: a transformed argument is a new value", {
    r <- tyr_run(tc$c4_f(1L))
    expect_violation(r, pattern = "call of 'tyrcalls::c_h' is outside its domain")
    dom <- r$rows[r$rows$parameter_position == -2L & r$rows$function_name == "c_h", ]
    expect_true(is.na(dom$origin))
})

test_that("b5: an unchecked function in between changes nothing", {
    r <- tyr_run(tc$c5_f("s"))
    expect_violation(r, n = 3L)
    expect_match(r$warnings[[1L]], "'tyrcalls::c_h'", fixed = TRUE)
    expect_match(r$warnings[[2L]], "'tyrcalls::c5_g'", fixed = TRUE)
    expect_match(r$warnings[[3L]], "'tyrcalls::c5_f'", fixed = TRUE)
})

test_that("b6: return(x) forces x; one violation, not two", {
    r <- tyr_run(tc$ret_x("a"))
    expect_violation(r)
    expect_equal(r$domain, "violation")
    expect_equal(r$ret, "unknown")
})

test_that("b7/b8: contractr does not examine a promise that R forces after the exit", {
    r <- tyr_run(t <- tc$mk("a"))
    expect_clean(r)
    expect_equal(r$domain, "unknown")

    r <- tyr_run(t())
    expect_clean(r)
    expect_equal(nrow(r$rows), 0L)

    r <- tyr_run({ tc$mk("a"); invisible(gc()) })
    expect_clean(r)
})

## ------------------------------------------------------------- graduality ---

test_that("g1: arguments never used are never evaluated, checked or not", {
    expect_silent(tyr_run(tc$lazy1(print("hi")), ret = 1))
    r <- tyr_run(tc$lazy1(stop("boom")), ret = 1)
    expect_clean(r)
    expect_equal(r$value, 1)
})

test_that("g1: the values are those R computes, whatever the verdict", {
    expect_identical(tyr_run(tc$s1(1), ret = c(TRUE, FALSE), severity = "silence")$value,
                     c(TRUE, FALSE))
    expect_identical(tyr_run(tc$tag(1), ret = "a", severity = "silence")$value, "a")
    expect_identical(tyr_run(tc$r_rec(2, 1:2), severity = "silence")$value, 1:2)
    expect_identical(tyr_run(tc$d_all(1:2, 1, "a"), ret = 3L, severity = "silence")$value, 3L)
})

test_that("g2: nothing is forced that R would not force, nor twice", {
    e <- new.env()
    e$n <- 0L
    tyr_run(tc$lazy1({ e$n <- e$n + 1L; 1L }), ret = 1)
    expect_equal(e$n, 0L)
    tyr_run(tc$tag({ e$n <- e$n + 1L; 1L }), ret = 1)
    expect_equal(e$n, 1L)
    tyr_run(tc$d_lazy(1:2, { e$n <- e$n + 1L; 1 }), ret = 1:2)
    expect_equal(e$n, 1L)
})

test_that("g3: widening a declaration never adds a violation", {
    widened <- list(
        ## The result of overload 2 changes from `LGL1` to `LGL`.
        m_x = c("(x: LGL, y: INT) -> LGL", "(x: INT, y: LGL) -> LGL"),
        ## The type of `y` in overload 2 changes from `LGL` to `vec`.
        m_yx = c("(x: LGL, y: INT) -> LGL", "(x: INT, y: vec) -> LGL1"),
        ## The type variable becomes `any`.
        tag = "(x: any) -> any",
        ## The type of `x` changes from `INT1` to `any`.
        c_f = "(x: any) -> any")
    cases <- list(
        quote(tc$m_x(1:2, identity(TRUE))),
        quote(tc$m_yx(1:2, 1:2)),
        quote(tc$tag(1)),
        quote(tc$c_f("string")))
    rets <- list(c(TRUE, FALSE), TRUE, "a", NULL)

    count <- function() vapply(seq_along(cases), function(k) {
        r <- tyr_run(eval(cases[[k]]), ret = rets[[k]])
        sum(r$rows$verdict == "violation", na.rm = TRUE)
    }, integer(1))

    before <- count()
    file <- tempfile(fileext = ".tyr")
    writeLines(unlist(lapply(names(widened), function(n)
        paste0("#| tyrcalls::", n, " : ", widened[[n]]))), file)
    contractr:::tyr_import_calls("tyrcalls", file)
    on.exit(contractr:::tyr_import_calls("tyrcalls", test_path("tyrcalls.tyr")))
    after <- count()

    expect_true(all(before > 0L))
    expect_true(all(after <= before))
    ## Now the domain of `c_f` accepts all values. The value does not agree
    ## with the domains of `c_g` and `c_h`.
    expect_equal(after, c(0L, 0L, 0L, 2L))
})

## ------------------------------------------------- queries with no answer ---

## Runs `expr` with an oracle that gives no answer (`NA`) to the queries for
## which `silent(query)` is `TRUE`. The other queries go to rstt.
tyr_run_silent <- function(expr, silent, ret) {
    ask <- contractr:::tyr_ask
    local_mocked_bindings(
        tyr_ask = function(query) if (silent(query)) NA else ask(query),
        .package = "contractr")
    tyr_run(expr, ret = ret)
}

test_that("a return query with no answer gives unknown, not ok", {
    r <- tyr_run_silent(tc$m_xy(1L, TRUE), ret = TRUE,
                        silent = function(q) grepl(") -> ~(", q, fixed = TRUE))
    expect_equal(r$domain, "ok")
    expect_equal(r$ret, "unknown")
})

test_that("a domain query with no answer never gives a return violation", {
    ## The call is outside the domain. Thus `F <= ((C & D) -> ~R)` is true.
    r <- tyr_run_silent(tc$m_xy(TRUE, TRUE), ret = TRUE,
                        silent = function(q) grepl("<= empty", q, fixed = TRUE))
    expect_clean(r)
    expect_equal(r$domain, "unknown")
    expect_equal(r$ret, "unknown")
})

test_that("a peek query with no answer never gives a return violation", {
    ## The peeked call `m_x(1L, "a")` is outside the domain.
    r <- tyr_run_silent(tc$m_x(1L, "a"), ret = TRUE,
                        silent = function(q) grepl("<= empty", q, fixed = TRUE) &&
                                             grepl("^CHR1", q, fixed = TRUE))
    expect_clean(r)
    expect_equal(r$domain, "unknown")
    expect_equal(r$ret, "unknown")
})

## ------------------------------------------------------------ call site ---

test_that("the call site is the call of the monitored function, not a call inside contractr", {
    ## `inner` has a source reference, as contractr has when R keeps its source.
    inner <- function(rho) {
        contractr:::tyr_call_site(rho)
    }
    monitored <- function() inner(environment())
    site <- monitored(); line <- getSrcLocation(function() NULL, "line")
    expect_match(site, paste0(":", line, "$"))
})
