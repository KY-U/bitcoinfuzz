all: module.a ../../pybitcoinkernel_main.py

CXXFLAGS += -Wall -Wextra -fsanitize=address,fuzzer -std=c++20 -I ../../include
PYTHON_CFLAGS := $(shell python3-config --includes)

../../pybitcoinkernel_main.py: pybitcoinkernel_lib.py
	cp $< $@

module.a: module.o
	$(AR) rcs $@ $^
	ranlib $@

module.o: module.cpp module.h
	$(CXX) $(CXXFLAGS) $(PYTHON_CFLAGS) -I . -fPIC -c $< -o $@

format:
	clang-format -i ./module.cpp ./module.h
	black ./pybitcoinkernel_lib.py

check-format:
	clang-format -Werror --fail-on-incomplete-format -n ./module.cpp ./module.h
	black --check ./pybitcoinkernel_lib.py

clean:
	rm -rf *.o *.a ./pybitcoinkernel_lib/pybitcoinkernel_lib.o ../../pybitcoinkernel_main.py

.PHONY: all clean format check-format
