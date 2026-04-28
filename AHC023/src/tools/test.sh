g++ ../main.cpp -o ../main -std=c++17
for i in {0..9}; do
    echo "Seed $i"
    ../main < in/000$i.txt > out/000$i.txt 2> error.txt
done
for i in {10..99} ; do
    echo "Seed $i"
    ../main < in/00$i.txt > out/00$i.txt 2> error.txt
done
# for i in {100..299} ; do
#     echo "Seed $i"
#     ../main < in/0$i.txt > out/0$i.txt 2> error.txt
# done
echo "All test cases have been tested!"

