.PHONY:
run_test: test
	./output/test


test: test_output_dir
	g++ -Wall -o ./output/test test.cpp

.PHONY:
test_output_dir:
	mkdir -p ./output

.PHONY:
clean:
	rm ./output/*
