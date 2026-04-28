g++ ../main.cpp -o ../main -std=c++17

for i in $(seq 0 9); do
    ../main < ./in/000$i.txt > ./out/000$i.txt
    cargo run -r --bin vis ./in/000$i.txt ./out/000$i.txt > error.txt 2>&1
    echo "Seed $i: $(tail -n 1 error.txt)"
    tail -n 1 error.txt | cut -c 9- >> score.txt  # 5行目の9文字目以降を追記
done
for i in $(seq 10 99) ; do
    ../main < ./in/00$i.txt > ./out/00$i.txt
    cargo run -r --bin vis ./in/00$i.txt ./out/00$i.txt > error.txt 2>&1
    echo "Seed $i: $(tail -n 1 error.txt)"
    tail -n 1 error.txt | cut -c 9- >> score.txt  # 5行目の9文字目以降を追記
done
for i in $(seq 100 999) ; do
    ../main < ./in/0$i.txt > ./out/0$i.txt
    cargo run -r --bin vis ./in/0$i.txt ./out/0$i.txt > error.txt 2>&1
    echo "Seed $i: $(tail -n 1 error.txt)"
    tail -n 1 error.txt | cut -c 9- >> score.txt  # 5行目の9文字目以降を追記
done
echo "All test cases have been tested!"
