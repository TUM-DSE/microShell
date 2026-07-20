#!/bin/bash

BINARY="./bin/test"

for t in 8 12 16; do
    echo "Running: $BINARY -p 1 -t $t"
    $BINARY -p 1 -t "$t"
done