[![QIO CI Build](https://github.com/QIOToolkit/QIOToolkit/actions/workflows/qio-toolkit-ci-integration.yaml/badge.svg?branch=feature%2Fci-workflow-integration)](https://github.com/QIOToolkit/QIOToolkit/actions/workflows/qio-toolkit-ci-integration.yaml)
![Code coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/DennisJensen-KPMG/fba0eacadbe787c890db8a8288ff9bd5/raw/qiotoolkit-coverage.json)

# Introduction

This project provides the components for building and running Quantum inspired optimization solvers that can solve different types of optimization problems that can be expressed as unbounded binary optimization problems.  

## Getting Started

The QIOToolkit build environment supported is targeted for linux systems. The
dockerfile located in the repo is the main build environment which the CI also
runs in. If you want to build natively, you can follow the dockerfile for
setting up your native environment. We do however recommend to stick with the
dockerfile for the build process for building the x86-64 binary application.

Running the two commands below should provide you with the build environment:

```bash
qio-toolkit$ docker build -t qio-toolkit .
qio-toolkit$ docker run -it --rm -v $(pwd):/qio-toolkit qio-toolkit
```

## Build and Test

Once you have the docker environment up and running and have attached the
repostiory inside of your docker environment you are ready to build the application and run tests.

We have a single entrypoint for operating and building the application which is
the root makefile. This file will provide you with all the necessary targets to
do development on the QIOToolkit.

The following targets are available:

* `build` - Builds the application in debug mode
* `build-release` - Builds the application in release mode
* `build-coverage` - Builds the application in debug mode with coverage
* `test` - Runs the unit tests
* `test-coverage` - Runs the unit tests with coverage
* `clean` - Cleans the build directory
* `build-documentation` - Builds the documentation
* `static-code-analysis` - Runs the static code analysis

### Building the application

Building the application in debug mode can be done with the following command
within your build environment:

```bash
qio-toolkit$ make build
```

### Testing

Testing the application can be done with the following command within your build
environment:

```bash
qio-toolkit$ make test
```

### Getting test coverage

Getting the test coverage can be done with the following command within your
build environment:

```bash
qio-toolkit$ make test-coverage
```

## Contributions

We have a [contributing guide](./doc/CONTRIBUTING.md) that outlines how to
contribute to the project. Please read it before making any contributions.

## Code of Conduct

We have a [code of conduct](./doc/CODE_OF_CONDUCT.md) that we expect project
participants to adhere to. Please read it before participating in the project.

## License

[MIT](https://choosealicense.com/licenses/mit/)
