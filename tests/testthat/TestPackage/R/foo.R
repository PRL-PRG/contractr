#' @export
foo1 <- function() {
  1
}

#' @export
foo2 <- function(x) {
  x
}

#' @export
foo3 <- function(x, y) {
  x+y
}

#' @export
foo4 <- function(...) {
  list(...)
}

#' @export
foo5 <- function(x, y, ...) {
  c(x, y, list(...))
}
