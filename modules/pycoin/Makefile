all: module.a ../../pycoin_main.py

CXXFLAGS += -Wall -Wextra -fsanitize=address,fuzzer -std=c++20 -I ../../include
PYTHON_CFLAGS := $(shell python3-config --includes)

../../pycoin_main.py: pycoin_lib.py
	cp $< $@

module.a: module.o
	$(AR) rcs $@ $^
	ranlib $@

module.o: module.cpp module.h
	$(CXX) $(CXXFLAGS) $(PYTHON_CFLAGS) -I . -fPIC -c $< -o $@

format:
	clang-format -i ./module.cpp ./module.h
	black ./pycoin_lib.py

check-format:
	clang-format -Werror --fail-on-incomplete-format -n ./module.cpp ./module.h
	black --check ./pycoin_lib.py

clean:
	rm -rf *.o *.a ./pycoin_lib/pycoin_lib.o ../../pycoin_main.py

.PHONY: all clean format check-format
