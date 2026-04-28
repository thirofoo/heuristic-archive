g++ ../main.cpp -o ../main -std=c++17

for i in $(seq 0 9); do
    echo "Seed $i"
    cargo run -r --bin tester ../main < ./inA/000$i.txt > ./outA/000$i.txt 2> error.txt
    tail -n 1 error.txt | while IFS= read -r line; do echo "$line"; done
    tail -n 1 error.txt | head -n 1 | cut -c 9- >> score.txt  # 5行目の9文字目以降を追記
    echo ""
done
for i in $(seq 10 99) ; do
    echo "Seed $i"
    cargo run -r --bin tester ../main < ./inA/00$i.txt > ./outA/00$i.txt 2> error.txt
    tail -n 1 error.txt | while IFS= read -r line; do echo "$line"; done
    tail -n 1 error.txt | head -n 1 | cut -c 9- >> score.txt  # 5行目の9文字目以降を追記
    echo ""
done
# for i in $(seq 100 999) ; do
#     echo "Seed $i"
#     cargo run -r --bin tester ../main < ./inA/0$i.txt > ./outA/0$i.txt 2> error.txt
#     tail -n 1 error.txt | while IFS= read -r line; do echo "$line"; done
#     tail -n 1 error.txt | head -n 1 | cut -c 9- >> score.txt  # 5行目の9文字目以降を追記
#     echo ""
# done
echo "All test cases have been tested!"
