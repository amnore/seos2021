#!/bin/sh

PROJROOT=$(echo "${PWD}" | sed -r 's/(.*\/lab3).*/\1/')
export PATH="${PROJROOT}/bin:${PATH}"
export TOKEN="pHad4L3g"
alias make="bear --append -- make"
