R_EXECUTABLE := R
R_CMD_CHECK_OUTPUT_DIRPATH := /tmp

all: install

build: clean
	$(R_EXECUTABLE) CMD build .

install:
	$(R_EXECUTABLE) CMD INSTALL --with-keep.source .

clean:
	rm -rf contractr*.tar.gz
	rm -rf *.Rcheck
	rm -rf src/*.so
	rm -rf src/*.o

document:
	$(R_EXECUTABLE) --slave -e "devtools::document()"
	$(R_EXECUTABLE) --slave -e "pkgdown::build_site()"

check: build
	$(R_EXECUTABLE) CMD check --output=$(R_CMD_CHECK_OUTPUT_DIRPATH) contractr_*.tar.gz

test:
	$(R_EXECUTABLE) --slave -e "devtools::test()"

install-dependencies:
	$(R_EXECUTABLE) --slave -e "install.packages(c('withr', 'testthat', 'devtools', 'roxygen2', 'lintr', 'pkgdown'), repos='http://cran.us.r-project.org')"

lint:
	$(R_EXECUTABLE) --slave -e "lintr::lint_package()"

.PHONY: all build install clean document check test install-dependencies lint
