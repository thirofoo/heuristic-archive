test:
	@clear
	@echo "単体 test"
	g++ main.cpp -o main -std=c++17
	@./main < _in > _out 2> _err
	@cd tools; \
    cargo run -r --bin vis ../_in ../_out

all-a:
	@clear
	@echo "全体 test"
	@cd tools; \
	./test_a.sh

all-b:
	@clear
	@echo "全体 test"
	@cd tools; \
	./test_b.sh

all-c:
	@clear
	@echo "全体 test"
	@cd tools; \
	./test_c.sh
