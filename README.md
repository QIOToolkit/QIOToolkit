[![QIO Documentation build](https://github.com/QIOToolkit/QIOToolkit/actions/workflows/qio-toolkit-ci-documentation.yaml/badge.svg?branch=feature%2Fci-workflow-integration)](https://github.com/QIOToolkit/QIOToolkit/actions/workflows/qio-toolkit-ci-documentation.yaml)
[![QIO CI Build](https://github.com/QIOToolkit/QIOToolkit/actions/workflows/qio-toolkit-ci-integration.yaml/badge.svg?branch=feature%2Fci-workflow-integration)](https://github.com/QIOToolkit/QIOToolkit/actions/workflows/qio-toolkit-ci-integration.yaml)
![Code coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/QIOToolkit/some-gist-id/raw/qio-toolkit-coverage.json)

# Introduction 
This project provides the components for building and running Quantum inspired optimization solvers that can solve different types of optimization problems that can be expressed as unbounded binary optimization problems.  

# Getting Started
1.	Installation process
## Linux / WSL
If running wsl, you may need to convert the setup bash file to run with linux file endings. To do that 
    1. Install dos2unix: sudo apt-get install dos2unix
    2. Assuming you are in the project directory, run: dos2unix ./setup/linuxsetup.h 

You will need admin permission for setup. 
Run
1. ./setup/linuxsetup.sh 
    Note: Please install and run dos2unix ./linuxsetup.h in case running from wsl, to avoid windows line endings related errors. 

This process will take few minutes to complete.

# Build and Test
## Linux / WSL
1. After installation is complete, the make and make test commands will be run from the linuxsetup.sh file

# Contributions

We have a [contributing guide](./doc/CONTRIBUTING.md) that outlines how to
contribute to the project. Please read it before making any contributions.

# Code of Conduct

We have a [code of conduct](./doc/CODE_OF_CONDUCT.md) that we expect project
participants to adhere to. Please read it before participating in the project.

# License

[MIT](https://choosealicense.com/licenses/mit/)
