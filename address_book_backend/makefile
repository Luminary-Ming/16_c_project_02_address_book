address_book : main.o contact.o contact_llist.o db_mysql.o utils.o minio_server.o http_server.o
	gcc -o $@ $^ -lmysqlclient -lmicrohttpd -lcurl -lpthread
clean :
	rm -rf *.o address_book