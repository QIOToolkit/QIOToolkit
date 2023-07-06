PHONY: build test build-coverage test-coverage clean build-documentation

BUILD_DIR := cpp/build_make
BUILD_RELEASE_DIR := cpp/build_release
COVERAGE_BUILD_DIR := cpp/build_coverage
COVERAGE_FLAGS := --html -o coverage.html --xml coverage.xml -e '.*/build.*/.*' -e '.*build.*' -e '.*test.*' -e '.*/test/.*' -e '.*_deps.*' -e '.*/_deps/.*' -e '.*/externals/' -e '.*/examples/'

build:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR} && cmake -GNinja ..
	cd ${BUILD_DIR} && ninja

build-release:
	mkdir -p ${BUILD_RELEASE_DIR}
	cd ${BUILD_RELEASE_DIR} && cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
	cd ${BUILD_RELEASE_DIR} && ninja

test: build
	cd ${BUILD_DIR} && ../run_tests.sh

build-coverage:
	mkdir -p $(COVERAGE_BUILD_DIR)
	cd $(COVERAGE_BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug -DGET_CODE_COVERAGE=ON -GNinja ..
	cd $(COVERAGE_BUILD_DIR) && ninja

test-coverage: build-coverage
	cd $(COVERAGE_BUILD_DIR) && ../run_tests.sh
	cd $(COVERAGE_BUILD_DIR) && gcovr -r .. ${COVERAGE_FLAGS}

clean:
	rm -rf ${BUILD_DIR}
	rm -rf ${COVERAGE_BUILD_DIR}
	rm -rf ${BUILD_RELEASE_DIR}

build-documentation:
	cd doc && ./generate.sh

static-code-analysis:
	cppcheck --enable=all --suppress=missingIncludeSystem -i build_coverage -i build_make -i externals -i build_release -i test --xml ./cpp 2> cppcheck-result.xml
	python3 ci/static_code_analysis/count_cppcheck_analysis.py -f cppcheck-result.xml --error-threshold 2 --warning-threshold 2

release-documentation: build-documentation
	sudo ./ci/release_documentation.sh # sudo because the docker container needs to be able to write to the host filesystem

