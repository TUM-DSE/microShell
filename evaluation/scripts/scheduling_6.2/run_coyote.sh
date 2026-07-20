#!/bin/bash

BINARY="./bin/test"

for t in 8 12 16; do
    echo "Running: $BINARY -p 0 -t $t"
    $BINARY -p 0 -t "$t"
done