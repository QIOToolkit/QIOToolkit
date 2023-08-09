---
title: Building the application
description: This tutorial shows how to build the QIOToolkit.
topic: article
uid: qiotoolkit.tutorial.setup.build
---

Building the QIOToolkit
======================

The QIOToolkit build environment supported is targeted for linux systems. The
dockerfile located in the repo is the main build environment which the CI also
runs in. If you want to build natively, you can follow the dockerfile for
setting up your native environment. We do however recommend to stick with the
dockerfile for the build process for building the x86-64 binary application.

Running the two commands below should provide you with the build environment:

```bash
qio-toolkit$ docker build -t qio-toolkit .
qio-toolkit$ docker run -it --rm -v $(pwd):/qio-toolkit -w /qio-toolkit qio-toolkit
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
