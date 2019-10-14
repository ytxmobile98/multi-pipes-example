name = multi-pipeline
compile = gcc -g -Wall -Werror

all: $(name).out

$(name).out: $(name).c
	$(compile) $(name).c -o $(name).out

clean:
	rm *.out
