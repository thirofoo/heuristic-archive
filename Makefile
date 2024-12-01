# コンパイラとオプションの設定
CXX = g++
CXXFLAGS = -Wshadow -std=c++2a

# ターゲットとソースファイルの設定
TARGET = main
SOURCE = $(TARGET).cpp
OBJECT = $(TARGET).o

# デフォルトのターゲットを指定
all: $(TARGET)

# mainに対するルール
$(TARGET): $(SOURCE)
	clang-format -i -style=file $(SOURCE)
	$(CXX) $(CXXFLAGS) -o ./$(TARGET) $(SOURCE)

# クリーンアップ
clean:
	rm -f ./out/$(TARGET)
	rm -f $(OBJECT)

# 単体テスト
test:
	@clear
	@echo "単体 test"
	@cd tools; \
	cargo run -r --bin tester ../main < ./in/0000.txt > ./out/0000.txt

# 全体テスト
all_test:
	@clear
	@echo "全体 test"
	@cd tools; \
	./test.sh

# サンプルテスト
sample:
	@clear
	@echo "Sample test"
	@python3 sample.py < _in > _out
	@cd tools; \
	cargo run -r --bin vis ../_in ../_out
