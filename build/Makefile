DEBUG= g++ -std=c++14 -ggdb
using = -pthread -lmysqlclient
RELEASE =g++ -std=c++14 -O2
c_debug=g++ -std=c++14 -c -ggdb
c_release=g++ -std=c++14 -c -O2
dirs =$(shell ls ../)
h_files += $(foreach n,$(dirs),$(wildcard ../$(n)/*.h))
cpp_files+= $(foreach n,$(dirs),$(notdir $(wildcard ../$(n)/*.cpp)))
lib_files +=$(foreach n,$(cpp_files),./lib$(subst .cpp,.a,$(n)))
server.out:../main.cpp $(lib_files)
	$(DEBUG) ../main.cpp libweb_server.a libepoller.a libtimer.a libsql_pool.a libhttp_con.a libhttp_request.a libhttp_response.a liblog.a libbuffer.a -o ../bin/server.out $(using)
libweb_server.a: ../server/web_server.cpp
	$(c_debug) ../server/web_server.cpp
	ar rcs libweb_server.a web_server.o
libepoller.a: ../server/epoller.cpp
	$(c_debug) ../server/epoller.cpp 
	ar rcs libepoller.a epoller.o
libtimer.a: ../pq_timer/timer.cpp
	$(c_debug) ../pq_timer/timer.cpp
	ar rcs libtimer.a timer.o
libsql_pool.a:../pool/sql_pool.cpp liblog.a
	$(c_debug) ../pool/sql_pool.cpp
	ar rcs libsql_pool.a sql_pool.o
libhttp_con.a: ../http/http_con.cpp
	$(c_debug) ../http/http_con.cpp
	ar rcs libhttp_con.a http_con.o
libhttp_request.a: ../http/http_request.cpp 
	$(c_debug) ../http/http_request.cpp
	ar rcs libhttp_request.a http_request.o
libhttp_response.a:../http/http_response.cpp
	$(c_debug) ../http/http_response.cpp
	ar rcs libhttp_response.a http_response.o
liblog.a: ../log/log.cpp 
	$(c_debug) ../log/log.cpp
	ar rcs liblog.a log.o
libbuffer.a:../buffer/buffer.cpp
	$(c_debug)  ../buffer/buffer.cpp
	ar rcs libbuffer.a buffer.o
clean:
	rm -f ../bin/*.out
	rm -f ./*.o ./*.out ./*.a