#!/bin/bash

function ignore_warnings {
	  grep -v 'ranlib: warning for library: .* the table of contents is empty' \
	| grep -v 'ranlib: file: .* has no symbols'
}

ar "$@" 2> >(ignore_warnings 1>&2)

