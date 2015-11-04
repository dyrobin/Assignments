#!/bin/sh

if [ $1 ]; then
    pattern=$1
else
    echo "NO PATTERN"
    exit 1
fi

if [ $2 ]; then
    tail -f log | grep $pattern -A 1
else
    cat log | grep $pattern -A 1
fi

