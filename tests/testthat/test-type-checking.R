test_that("inserting into promises", {

  g <- function(y) {
    browser()
    print(y)
  }

  f1 <- function(a, ...) {
    .Call(inject_type_checks, "contractR", "f1", sys.function(), sys.frame(sys.nframe()))
    browser()

    g(a)
  }

  f2 <- function(a,b,c,d) {
    .Call(inject_type_checks, "contractR", "f2", sys.function(), sys.frame(sys.nframe()))
    browser()

    g(a)
  }

  f1(1+1,2+2)
  f2(d=4,1+1,b=,3,5)


})
