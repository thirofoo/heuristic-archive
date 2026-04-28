#!/bin/bash

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
    --problem)
        problem="$2"
        shift
        shift
        ;;
    --case)
        case="$2"
        shift
        shift
        ;;
    esac
done

if [ -z "$problem" ] || [ -z "$case" ]; then
    echo "Usage: test.sh --problem <problem> --case <case>"
    echo "Example: test.sh --problem A --case 0001"
    exit 1
fi

g++ main.cpp -o a.out
cd tools
cargo run -r --bin tester ../a.out <in$problem/$case.txt >out.txt
