run:
	g++ -std=c++17 -Wall -Wextra ast.cpp lexical.cpp main.cpp parser.cpp semantic.cpp simulador.cpp -o main && ./main && rm -rf main
clean:
	rm -rf main