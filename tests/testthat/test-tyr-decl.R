## These tests examine the reader of the declarations that `tyr --typedecl`
## writes (`R/tyr_call.R`). Each overload is one `#|` line. These tests do not
## use rstt.

read_decl <- function(...) {
    file <- tempfile(fileext = ".tyr")
    writeLines(c(...), file)
    names <- contractr:::tyr_import_calls("p", file)
    structure(lapply(names, function(n) contractr:::tyr_call_declaration("p", n)),
              names = names)
}

test_that("the lines of a name are its overloads", {
    d <- read_decl("# tyr: a comment",
                   "#| p::f : (x: dbl) -> DBL | INT",
                   "#| p::f : (x: dbl1) -> DBL1")
    expect_equal(d$f, list(type = "((x: dbl) -> DBL | INT) & ((x: dbl1) -> DBL1)",
                           domain = "(x: dbl) | (x: dbl1)",
                           domain_rigid = "(x: dbl) | (x: dbl1)"))
})

test_that("variables are flexible in F and D, rigid in D_rigid, and apart per line", {
    d <- read_decl("#| p::g : (x: 'a, y: v('b)<`a>) -> 'b",
                   "#| p::g : (x: 'a) -> (() -> 'a)")
    expect_equal(d$g$type, "((x: 'A, y: v('B)<`A>) -> 'B) & ((x: 'C) -> (() -> 'C))")
    expect_equal(d$g$domain, "(x: 'A, y: v('B)<`A>) | (x: 'C)")
    expect_equal(d$g$domain_rigid, "(x: 'a, y: v('b)<`a>) | (x: 'c)")
})

test_that("string literals and quoted identifiers hold no variable and no arrow", {
    d <- read_decl('#| p::h : (x: "it\'s (a) -> \\"b\\"" | y\'z) -> \'a')
    expect_equal(d$h$domain, '(x: "it\'s (a) -> \\"b\\"" | y\'z)')
    expect_equal(d$h$domain_rigid, d$h$domain)
    expect_equal(d$h$type, '((x: "it\'s (a) -> \\"b\\"" | y\'z) -> \'A)')
})

test_that("operator names, names without a namespace, and other namespaces", {
    d <- read_decl("#| p::(%||%) : (x: any, y: any) -> any",
                   "#| p::names<- : (x: any, value: any) -> any",
                   "#| k : () -> null",
                   "#| base::c : (...: any) -> any")
    expect_named(d, c("%||%", "names<-", "k"))
})

test_that("a line that is not a signature is reported", {
    expect_warning(d <- read_decl("#| p::v : int"), "cannot read tyr declaration")
    expect_length(d, 0L)
    expect_warning(read_decl("#| p::w : (x: int)"), "cannot read tyr declaration")
})
