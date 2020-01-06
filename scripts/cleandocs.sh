#!/bin/bash

doxybook -i ./tmp/xml -o ./gitbook/docs -t gitbook -s ./SUMMARY.md

sed -i '/Related Pages/d' ./SUMMARY.md
sed -i '/Class List/,$d' ./SUMMARY.md

rm ./gitbook/docs/_*.md
rm ./gitbook/docs/dir_*.md
rm ./gitbook/docs/pages.md
rm ./gitbook/docs/files.md
rm ./gitbook/docs/macros.md
rm ./gitbook/docs/variables.md
rm ./gitbook/docs/functions.md

rm ./gitbook/docs/annotated.md
rm ./gitbook/docs/classes.md
rm ./gitbook/docs/hierarchy.md
rm ./gitbook/docs/class_members.md
rm ./gitbook/docs/class_member_functions.md
rm ./gitbook/docs/class_member_variables.md
rm ./gitbook/docs/class_member_typedefs.md
rm ./gitbook/docs/class_member_enums.md

rm ./gitbook/docs/namespaces.md
rm ./gitbook/docs/namespacestd.md
rm ./gitbook/docs/namespaceeosio.md
rm ./gitbook/docs/namespace_members.md
rm ./gitbook/docs/namespace_member_functions.md
rm ./gitbook/docs/namespace_member_variables.md
rm ./gitbook/docs/namespace_member_typedefs.md
rm ./gitbook/docs/namespace_member_enums.md

# gitbook build ./gitbook/
# gitbook serve