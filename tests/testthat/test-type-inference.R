test_that("type inference for ... works", {

    expect_identical(infer_type(NULL, "..."), "...")

    expect_identical(infer_type(, "..."), "...")

    expect_identical(infer_type(2, "..."), "...")

})


test_that("type inference for missing values works", {

    expect_identical(infer_type(), "???")

})


test_that("type inference for null values works", {

    expect_identical(infer_type(NULL), "null")

})


test_that("type inference for list values works", {

    expect_identical(infer_type(list(NULL, NULL, NULL, NULL, NULL, NULL)), "list<null>")

    expect_identical(infer_type(list(1, 2, "3", NULL, list(1, 2), list(TRUE, FALSE))),
                 "list<? character | double | tuple<double, double> | tuple<logical, logical>>")


    expect_identical(infer_type(create_tuple(0)), "tuple<>")

    expect_identical(infer_type(list(1, "3", NULL, list(1, 2), list(1, "2", TRUE, list(1, 2), max))),
                     str_c("tuple<",
                           "double, ",
                           "character, ",
                           "null, ",
                           "tuple<double, double>, ",
                           "tuple<double, character, logical, tuple<double, double>, any => any>>"))

    expect_identical(infer_type(create_struct(0)), "struct<>")

    expect_identical(infer_type(create_struct(1)), "struct<`name`: double>")

    expect_identical(infer_type(create_struct(1, NA)), "struct<^: double>")

    object <- list(name1 = 1, "3", name3 = NULL, list(1, 2), `another name` = list(1, "2", TRUE, list(1, 2), max))
    expect_identical(infer_type(object),
                     str_c("struct<",
                           "`name1`: double, ",
                           "``: character, ",
                           "`name3`: null, ",
                           "``: tuple<double, double>, ",
                           "`another name`: tuple<double, character, logical, tuple<double, double>, any => any>>"))

    names(object) <- NA
    expect_identical(infer_type(object),
                     str_c("struct<",
                           "^: double, ",
                           "^: character, ",
                           "^: null, ",
                           "^: tuple<double, double>, ",
                           "^: tuple<double, character, logical, tuple<double, double>, any => any>>"))

})


test_that("type inference for function values works", {

    expect_identical(infer_type(max), "any => any")

    expect_identical(infer_type(`if`), "any => any")

    expect_identical(infer_type(function(x) x), "any => any")

})


test_that("type inference for integer values works", {

    expect_identical(infer_type(integer()), "integer[]")

    expect_identical(infer_type(2L), "integer")

    expect_identical(infer_type(c(2L, 3L)), "integer[]")

    expect_identical(infer_type(NA_integer_), "^integer")

    expect_identical(infer_type(c(32L, NA_integer_)), "^integer[]")

})


test_that("type inference for character values works", {

    expect_identical(infer_type(character()), "character[]")

    expect_identical(infer_type("c"), "character")

    expect_identical(infer_type(c("2", '3')), "character[]")

    expect_identical(infer_type(NA_character_), "^character")

    expect_identical(infer_type(c("32", NA_character_)), "^character[]")

})


test_that("type inference for complex values works", {

    expect_identical(infer_type(complex()), "complex[]")

    expect_identical(infer_type(2 + 2i), "complex")

    expect_identical(infer_type(c(2 + 1i, 3)), "complex[]")

    expect_identical(infer_type(NA_complex_), "^complex")

    expect_identical(infer_type(c(32 + 9i, NA_complex_)), "^complex[]")

})


test_that("type inference for double values works", {

    expect_identical(infer_type(double()), "double[]")

    expect_identical(infer_type(2), "double")

    expect_identical(infer_type(c(2, 3)), "double[]")

    expect_identical(infer_type(NA_real_), "^double")

    expect_identical(infer_type(c(32, NA_real_)), "^double[]")

})


test_that("type inference for logical values works", {

    expect_identical(infer_type(logical()), "logical[]")

    expect_identical(infer_type(TRUE), "logical")

    expect_identical(infer_type(c(TRUE, FALSE)), "logical[]")

    expect_identical(infer_type(na_logical_), "^logical")

    expect_identical(infer_type(c(TRUE, na_logical_)), "^logical[]")

})


test_that("type inference for raw values works", {

    expect_identical(infer_type(raw()), "raw[]")

    expect_identical(infer_type(as.raw(40)), "raw")

    expect_identical(infer_type(as.raw(c(2, 3))), "raw[]")

})


test_that("type inference for environment values works", {

    expect_identical(infer_type(new.env()), "environment")

})


test_that("type inference for expression values works", {

    expect_identical(infer_type(expression(1 + 2)), "expression")

})


test_that("type inference for language values works", {

    expect_identical(infer_type(quote(1 + 2)), "language")

})


test_that("type inference for symbol values works", {

    expect_identical(infer_type(as.symbol("sym")), "symbol")

})


test_that("type inference for pairlist values works", {

    expect_identical(infer_type(pairlist(1, 2, 3)), "pairlist")

})


test_that("type inference for S4 values works", {

    expect_identical(infer_type(create_s4()), "s4")

})


test_that("type inference for weakref values works", {

    expect_identical(infer_type(create_weakref()), "weakref")

})
