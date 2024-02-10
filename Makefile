exec-test:
	g++ main.cpp -o main -std=c++17
	cd tools; \
	cargo run -r --bin tester ../main < ../_in > ../_out 2> ../_err

exec-all-test:
	cd tools; \
	./test.sh

exec-sample:
	cd tools; \
	cargo run -r --bin tester python3 ../sample.py < ../_in > ../_out 2> ../_err
