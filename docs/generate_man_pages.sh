#!/bin/bash

for m in ../man/*.[15]
do
	BASE=$(basename "$m")
	TITLE=${BASE//\.[0-9]/}
	pandoc --standalone --shift-heading-level-by 1 --metadata-file man_metadata.yaml --metadata "title=$TITLE" -o "man/$BASE.md" "$m"
done
