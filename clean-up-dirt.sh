#!/bin/sh
find . -name '._*' -exec rm '{}' ';'
find . -name '.DS_Store' -exec rm '{}' ';'
find . -name '*~' -exec rm '{}' ';'
rm -rf .Spotlight-V100
rm -rf .fseventsd
rm -rf .Trashes
