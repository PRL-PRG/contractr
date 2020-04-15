R_EXECUTABLE := R
R_CMD_CHECK_OUTPUT_DIRPATH := /tmp

all: install

build: clean
	$(R_EXECUTABLE) CMD build .

install: clean
	$(R_EXECUTABLE) CMD INSTALL --with-keep.source .

clean:
	rm -rf contractR*.tar.gz
	rm -rf *.Rcheck
	rm -rf src/*.so
	rm -rf src/*.o

document:
	$(R_EXECUTABLE) -e "devtools::document()"

check: build
	$(R_EXECUTABLE) CMD check --output=$(R_CMD_CHECK_OUTPUT_DIRPATH) typetesterdyntracer_*.tar.gz

test:
	$(R_EXECUTABLE) -e "devtools::test()"

install-dependencies:
	$(R_EXECUTABLE) -e "install.packages(c('withr', 'testthat', 'devtools', 'roxygen2'), repos='http://cran.us.r-project.org')"

.PHONY: all build install clean document check test install-dependencies
