#!/bin/bash

# Check if folder exists
if [ ! -d "./doc/_site" ]; then
  echo "Documentation does not exist. Exiting."
  exit 1
fi

# Move the whole directory to a temporary location
rm -rf /tmp/gh-pages
rm -rf /tmp/_site
cp -r . /tmp/gh-pages
cd /tmp/gh-pages

# Automated release process credentials
git config --global user.email "<>"
git config --global user.name "Documentation release Bot (via GitHub actions)"

# Move all the files inside the doc/_site folder to the root directory
mv ./doc/_site /tmp/

# Prepare git repo for gh-pages
git restore .
git clean -fd
git checkout gh-pages

# Fail if branch is not gh-pages
if [ "$(git branch --show-current)" != "gh-pages" ]; then
  echo "Not on gh-pages branch. Exiting."
  exit 1
fi

# Clean out the old documentation
rm -rf *

# Move all the files from /tmp/_site to the root directory
mv /tmp/_site/* .

git add .
git commit -m "Automatic update of documentation"
git push