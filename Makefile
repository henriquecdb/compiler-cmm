run:
	g++ -std=c++17 -Wall -Wextra *.cpp -o main && ./main && rm -rf main
clean:
	rm -rf main