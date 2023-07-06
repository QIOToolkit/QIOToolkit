#!/bin/bash

# If in docker environment
if [ -f /.dockerenv ]; then
  echo "In docker environment"
  git config --global --add safe.directory /home/user/workdir
fi

# Check if folder exists
if [ ! -d "./doc/_site" ]; then
  echo "Documentation does not exist. Exiting."
  exit 1
fi

# Automated release process credentials
git config --global user.email "<>"
git config --global user.name "Documentation release Bot (via GitHub actions)"

# Move the whole directory to a temporary location
rm -rf /tmp/gh-pages
rm -rf /tmp/_site
cp -r . /tmp/gh-pages
cd /tmp/gh-pages

# Move all the files inside the doc/_site folder to the root directory
mv ./doc/_site /tmp/

# Prepare git repo for gh-pages
git restore .
git clean -fd
git checkout gh-pages

# Clean out the old documentation
rm -rf *

# Move all the files from /tmp/_site to the root directory
mv /tmp/_site/* .

git add .
git commit -m "Automatic update of documentation"
git push