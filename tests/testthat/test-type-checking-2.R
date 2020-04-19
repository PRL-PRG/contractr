test_that("type checking for ... works", {

    expect_true(check_type_2(NULL, "...", "..."))

    expect_true(check_type_2(, "...", "..."))

    expect_true(check_type_2(2, "...", "..."))

})


test_that("type checking for missing values works", {

    expect_true(check_type_2(, "???"))

})


test_that("type checking for null values works", {

    expect_true(check_type_2(NULL, "null"))

})


test_that("type checking for list values works", {

    expect_true(check_type_2(list(NULL, NULL, NULL, NULL, NULL, NULL), "list<null>"))

    ## FIXME
    ## expect_true(check_type_2(list(1, 2, "3", NULL, list(1, 2), list(TRUE, FALSE)),
    ##              "list<? character | double | tuple<double, double> | tuple<logical, logical>>"))


    ## FIXME
    ## expect_true(check_type_2(create_tuple(0), "tuple<>"))

    ## FIXME
    ## expect_true(check_type_2(list(1, "3", NULL, list(1, 2), list(1, "2", TRUE, list(1, 2), max)),
    ##                  str_c("tuple<",
    ##                        "double, ",
    ##                        "character, ",
    ##                        "null, ",
    ##                        "tuple<double, double>, ",
    ##                        "tuple<double, character, logical, tuple<double, double>, any => any>>")))

    ## FIXME
    ## expect_true(check_type_2(create_struct(0), "struct<>"))

    expect_true(check_type_2(create_struct(1), "struct<`name`: double>"))

    ## FIXME
    ## expect_true(check_type_2(create_struct(1, NA), "struct<^: double>"))

    ## FIXME
    ## object <- list(name1 = 1, "3", name3 = NULL, list(1, 2), `another name` = list(1, "2", TRUE, list(1, 2), max))
    ## expect_true(check_type_2(object,
    ##                          str_c("struct<",
    ##                                "`name1`: double, ",
    ##                                "``: character, ",
    ##                                "`name3`: null, ",
    ##                                "``: tuple<double, double>, ",
    ##                                "`another name`: tuple<double, character, logical, tuple<double, double>, any => any>>")))

    ## FIXME
    ## names(object) <- NA
    ## expect_true(check_type_2(object,
    ##                          str_c("struct<",
    ##                                "^: double, ",
    ##                                "^: character, ",
    ##                                "^: null, ",
    ##                                "^: tuple<double, double>, ",
    ##                                "^: tuple<double, character, logical, tuple<double, double>, any => any>>")))

})


test_that("type checking for function values works", {

    expect_true(check_type_2(max, "any => any"))

    expect_true(check_type_2(`if`, "any => any"))

    expect_true(check_type_2(function(x) x, "any => any"))

})


test_that("type checking for integer values works", {

    expect_true(check_type_2(integer(), "integer[]"))

    expect_true(check_type_2(2L, "integer"))

    expect_true(check_type_2(c(2L, 3L), "integer[]"))

    expect_true(check_type_2(NA_integer_, "^integer"))

    expect_true(check_type_2(c(32L, NA_integer_), "^integer[]"))

})


test_that("type checking for character values works", {

    expect_true(check_type_2(character(), "character[]"))

    expect_true(check_type_2("c", "character"))

    expect_true(check_type_2(c("2", '3'), "character[]"))

    expect_true(check_type_2(NA_character_, "^character"))

    expect_true(check_type_2(c("32", NA_character_), "^character[]"))

})


test_that("type checking for complex values works", {

    expect_true(check_type_2(complex(), "complex[]"))

    expect_true(check_type_2(2 + 2i, "complex"))

    expect_true(check_type_2(c(2 + 1i, 3), "complex[]"))

    expect_true(check_type_2(NA_complex_, "^complex"))

    expect_true(check_type_2(c(32 + 9i, NA_complex_), "^complex[]"))

})


test_that("type checking for double values works", {

    expect_true(check_type_2(double(), "double[]"))

    expect_true(check_type_2(2, "double"))

    expect_true(check_type_2(c(2, 3), "double[]"))

    expect_true(check_type_2(NA_real_, "^double"))

    expect_true(check_type_2(c(32, NA_real_), "^double[]"))

})


test_that("type checking for logical values works", {

    expect_true(check_type_2(logical(), "logical[]"))

    expect_true(check_type_2(TRUE, "logical"))

    expect_true(check_type_2(c(TRUE, FALSE), "logical[]"))

    expect_true(check_type_2(na_logical_, "^logical"))

    expect_true(check_type_2(c(TRUE, na_logical_), "^logical[]"))

})


test_that("type checking for raw values works", {

    expect_true(check_type_2(raw(), "raw[]"))

    expect_true(check_type_2(as.raw(40), "raw"))

    expect_true(check_type_2(as.raw(c(2, 3)), "raw[]"))

})


test_that("type checking for environment values works", {

    expect_true(check_type_2(new.env(), "environment"))

})


test_that("type checking for expression values works", {

    expect_true(check_type_2(expression(1 + 2), "expression"))

})


test_that("type checking for language values works", {

    expect_true(check_type_2(quote(1 + 2), "language"))

})


test_that("type checking for symbol values works", {

    expect_true(check_type_2(as.symbol("sym"), "symbol"))

})


test_that("type checking for pairlist values works", {

    expect_true(check_type_2(pairlist(1, 2, 3), "pairlist"))

})


test_that("type checking for S4 values works", {

    expect_true(check_type_2(create_s4(), "s4"))

})


test_that("type checking for weakref values works", {

    expect_true(check_type_2(create_weakref(), "weakref"))

})
