#!/usr/bin/bash

pip3 install -r requirements.txt

rm -rf doxygen
cd ..
doxygen doxygen.config
cd ./doc

find api -iname "*yml" | grep -v "toc.yml" | while read file; do
  rm $file
done

find doxygen -iname "struct*.xml" -or -iname "class*.xml" -or -iname "namespace*.xml" | grep -v "structure_" | grep -v "_1_1_0d" | while read file; do
  python doxygen2docfx.py $file
done

python generate_xrefmap.py $(find api -iname "*.yml" | grep -v "toc.yml")

cat templates/cpp/partials/footer.tmpl.partial.in | sed -e"s#{{GIT_BRANCH}}#`git branch --show-current`#g" -e"s#{{GIT_HASH}}#`git rev-parse HEAD`#g" -e"s#{{GIT_HASH_SHORT}}#`git rev-parse --short HEAD`#g" > templates/cpp/partials/footer.tmpl.partial

rm -rf _site
docfx

find _site -iname "*.html" | while read file; do 
  sed -i -e"s#amp;version=GBdoc#amp;version=GBmaster#g" $file
done
