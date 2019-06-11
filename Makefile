
grep: main.c
	cc -o grep main.c

test: grep
	bash -c "echo -n -e \"a\\nb\"" | ./grep  "a|b"

clean:
	rm grep

.PHONY: test clean
