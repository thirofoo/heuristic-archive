seed=$1
if [[ $seed =~ ^[0-9]+$ ]]; then
    seed=$(printf "%04d" $seed)
fi

problem=$2
if [[ ! $problem =~ ^[A-C]$ ]]; then
    echo "Problem must be A, B, or C"
    exit 1
fi

cd solver
cargo build --release

time ./target/release/solve <../tools/in$problem/$seed.txt >../.out
