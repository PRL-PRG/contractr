test_that("type checking for ... works", {

    expect_true(check_type(NULL, "...", "..."))

    expect_true(check_type(, "...", "..."))

    expect_true(check_type(2, "...", "..."))

})


test_that("type checking for missing values works", {

    expect_true(check_type(, "???"))

})


test_that("type checking for null values works", {

    expect_true(check_type(NULL, "null"))

})


test_that("type checking for list values works", {

    expect_true(check_type(list(NULL, NULL, NULL, NULL, NULL, NULL), "list<null>"))

    ## FIXME
    ## expect_true(check_type(list(1, 2, "3", NULL, list(1, 2), list(TRUE, FALSE)),
    ##              "list<? character | double | tuple<double, double> | tuple<logical, logical>>"))


    ## FIXME
    ## expect_true(check_type(create_tuple(0), "tuple<>"))

    ## FIXME
    ## expect_true(check_type(list(1, "3", NULL, list(1, 2), list(1, "2", TRUE, list(1, 2), max)),
    ##                  str_c("tuple<",
    ##                        "double, ",
    ##                        "character, ",
    ##                        "null, ",
    ##                        "tuple<double, double>, ",
    ##                        "tuple<double, character, logical, tuple<double, double>, any => any>>")))

    ## FIXME
    ## expect_true(check_type(create_struct(0), "struct<>"))

    expect_true(check_type(create_struct(1), "struct<`name`: double>"))

    ## FIXME
    ## expect_true(check_type(create_struct(1, NA), "struct<^: double>"))

    ## FIXME
    ## object <- list(name1 = 1, "3", name3 = NULL, list(1, 2), `another name` = list(1, "2", TRUE, list(1, 2), max))
    ## expect_true(check_type(object,
    ##                          str_c("struct<",
    ##                                "`name1`: double, ",
    ##                                "``: character, ",
    ##                                "`name3`: null, ",
    ##                                "``: tuple<double, double>, ",
    ##                                "`another name`: tuple<double, character, logical, tuple<double, double>, any => any>>")))

    ## FIXME
    ## names(object) <- NA
    ## expect_true(check_type(object,
    ##                          str_c("struct<",
    ##                                "^: double, ",
    ##                                "^: character, ",
    ##                                "^: null, ",
    ##                                "^: tuple<double, double>, ",
    ##                                "^: tuple<double, character, logical, tuple<double, double>, any => any>>")))

})


test_that("type checking for function values works", {

    expect_true(check_type(max, "any => any"))

    expect_true(check_type(`if`, "any => any"))

    expect_true(check_type(function(x) x, "any => any"))

})


test_that("type checking for integer values works", {

    expect_true(check_type(integer(), "integer[]"))

    expect_true(check_type(2L, "integer"))

    expect_true(check_type(c(2L, 3L), "integer[]"))

    expect_true(check_type(NA_integer_, "^integer"))

    expect_true(check_type(c(32L, NA_integer_), "^integer[]"))

})


test_that("type checking for character values works", {

    expect_true(check_type(character(), "character[]"))

    expect_true(check_type("c", "character"))

    expect_true(check_type(c("2", '3'), "character[]"))

    expect_true(check_type(NA_character_, "^character"))

    expect_true(check_type(c("32", NA_character_), "^character[]"))

})


test_that("type checking for complex values works", {

    expect_true(check_type(complex(), "complex[]"))

    expect_true(check_type(2 + 2i, "complex"))

    expect_true(check_type(c(2 + 1i, 3), "complex[]"))

    expect_true(check_type(NA_complex_, "^complex"))

    expect_true(check_type(c(32 + 9i, NA_complex_), "^complex[]"))

})


test_that("type checking for double values works", {

    expect_true(check_type(double(), "double[]"))

    expect_true(check_type(2, "double"))

    expect_true(check_type(c(2, 3), "double[]"))

    expect_true(check_type(NA_real_, "^double"))

    expect_true(check_type(c(32, NA_real_), "^double[]"))

})


test_that("type checking for logical values works", {

    expect_true(check_type(logical(), "logical[]"))

    expect_true(check_type(TRUE, "logical"))

    expect_true(check_type(c(TRUE, FALSE), "logical[]"))

    expect_true(check_type(na_logical_, "^logical"))

    expect_true(check_type(c(TRUE, na_logical_), "^logical[]"))

})


test_that("type checking for raw values works", {

    expect_true(check_type(raw(), "raw[]"))

    expect_true(check_type(as.raw(40), "raw"))

    expect_true(check_type(as.raw(c(2, 3)), "raw[]"))

})


test_that("type checking for environment values works", {

    expect_true(check_type(new.env(), "environment"))

})


test_that("type checking for expression values works", {

    expect_true(check_type(expression(1 + 2), "expression"))

})


test_that("type checking for language values works", {

    expect_true(check_type(quote(1 + 2), "language"))

})


test_that("type checking for symbol values works", {

    expect_true(check_type(as.symbol("sym"), "symbol"))

})


test_that("type checking for pairlist values works", {

    expect_true(check_type(pairlist(1, 2, 3), "pairlist"))

})


test_that("type checking for S4 values works", {

    expect_true(check_type(create_s4(), "s4"))

})


test_that("type checking for weakref values works", {

    expect_true(check_type(create_weakref(), "weakref"))

})
