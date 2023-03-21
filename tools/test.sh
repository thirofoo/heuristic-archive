g++ ../main.cpp -o ../main -std=c++17
for i in {0..9} ; do
    ../main < ./in/000$i.txt > ./out/000$i.txt
    cargo run --release --bin vis ./in/000$i.txt  ./out/000$i.txt > error.txt 2>/dev/null
    cat error.txt >> score.csv
    cat error.txt
done
for i in {10..99} ; do
    ../main < ./in/00$i.txt > ./out/00$i.txt
    cargo run --release --bin vis ./in/00$i.txt ./out/00$i.txt > error.txt 2>/dev/null
    cat error.txt >> score.csv
    cat error.txt
done
echo "All test cases have been tested!"