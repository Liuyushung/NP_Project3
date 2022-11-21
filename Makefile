CC = /bin/g++
EXE = http_server console.cgi cgi_server

part1:
	$(CC) http_server.cpp -o http_server
	$(CC) console.cpp -o console.cgi

part2:
	$(CC) cgi_server.cpp -o cgi_server

clean:
	rm $(EXE)