test_that("type inference for ... works", {

    expect_identical(infer_type(NULL, "..."), "...")

    expect_identical(infer_type(, "..."), "...")

    expect_identical(infer_type(2, "..."), "...")

})


test_that("type inference for missing values works", {

    expect_identical(infer_type(), "any")

})


test_that("type inference for null values works", {

    expect_identical(infer_type(NULL), "null")

})


test_that("type inference for list values works", {

    expect_identical(infer_type(list(NULL, NULL, NULL, NULL, NULL, NULL)), "list<null>")

    expect_identical(infer_type(list(1, 2, "3", NULL, list(1, 2), list(TRUE, FALSE))),
                 "list<character | double | list<double> | list<logical> | null>")


    expect_identical(infer_type(create_list()), "list<any>")

    expect_identical(infer_type(create_list(1)), "list<double>")

    expect_identical(infer_type(create_list(1L)), "list<integer>")

    expect_identical(infer_type(create_list(1, NA_real_)), "list<^double | double>")

    expect_identical(infer_type(list(1, "3", NULL, list(1, 2), list(1, "2", TRUE, list(1, 2), max))),
                     "list<character | double | list<any => any | character | double | list<double> | logical> | list<double> | null>")


    object <- list(name1 = 1, "3", name3 = NULL, list(1, 2), `another name` = list(1, "2", TRUE, list(1, 2), max))
    expect_identical(infer_type(object),
                     "list<character | double | list<any => any | character | double | list<double> | logical> | list<double> | null>")

    names(object) <- NA
    expect_identical(infer_type(object),
                     "list<character | double | list<any => any | character | double | list<double> | logical> | list<double> | null>")

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

    expect_identical(infer_type(quote(1 + 2)), "class<call>")

})


test_that("type inference for symbol values works", {

    expect_identical(infer_type(as.symbol("sym")), "class<name>")

})


test_that("type inference for pairlist values works", {

    expect_identical(infer_type(pairlist(1, 2, 3)), "pairlist")

})


test_that("type inference for S4 values works", {

    expect_identical(infer_type(create_s4()), "class<Person>")

})
