## Each function returns the value that a test puts in the option
## `tyrcalls.ret`. Thus a test can examine the return check independently of
## the arguments. Each function forces its arguments in the order that its
## name gives.
ret <- function() getOption("tyrcalls.ret")

## Scenario 1: an overload. Another function forces the argument.
s1 <- function(x) { y <- s1_g(x); y }
s1_g <- function(x) { x; ret() }
s1_lazy <- function(x) ret()

## F2: the type of x sets the type of y
m_yx <- function(x, y) { y; x; ret() }
m_xy <- function(x, y) { x; y; ret() }
m_x <- function(x, y) { x; ret() }
m_none <- function(x, y) ret()

## F3: the overload changes if the call supplies y
o_force <- function(x, y) { x; if (!missing(y)) y; ret() }
o_lazy <- function(x, y) { x; ret() }

## a union of argument lists
u_ab <- function(a, b) { a; b; ret() }

## dots
d_all <- function(x, ...) { x; list(...); ret() }
d_lazy <- function(x, ...) { x; ret() }
d_post <- function(x, ..., z = TRUE) { x; list(...); z; ret() }

## polymorphism
tag <- function(x) { x; ret() }
p_aa <- function(x, y) { x; y; ret() }
p_ab <- function(x, y) { x; y; ret() }
p_ab_lazy <- function(x, y) { x; ret() }
p_bound <- function(x, y) { x; y; ret() }

## the records of calls
r_req <- function(x, y) { x; ret() }
r_rec <- function(n, x) if (n > 0) r_rec(n - 1, x) else x
r_def <- function(x, y = 1) { x; y; ret() }
r_unused <- function(x, y) { y; ret() }
r_pm <- function(text, n) { text; n; ret() }
r_err <- function(x) { x; stop("inner") }
r_missing <- function(x, y) { x; missing(y) }
r_missing_fwd <- function(x, y) r_missing(x, y)

## chains of monitored calls
c_f <- function(x) c_g(x)
c_g <- function(x) c_h(x)
c_h <- function(x) { x; ret() }
c3_f <- function(x) c3_g(x)
c3_g <- function(x) c3_h(x)
c3_h <- function(x) { x; ret() }
c4_f <- function(x) c4_g(x)
c4_g <- function(x) c_h(c(x, x))
c5_f <- function(x) c5_g(x)
c5_g <- function(x) c5_k(x)
c5_k <- function(x) c_h(x)          # No declaration: contractr does not monitor it.

## arguments that the call returns, keeps or does not use
ret_x <- function(x) return(x)
mk <- function(x) function() x
lazy1 <- function(x) ret()

## The default of a formal goes to another call. Some R versions give TRUE for
## `missing()` in the callee, but the argument has a value. `praise()` does
## this when it calls `is_template(template)`.
r_def_fwd <- function(x, y = TRUE) r_missing(x, y)
c_def <- function(t = 1L) c_h(t)
c_def_bad <- function(t = "a") c_h(t)
