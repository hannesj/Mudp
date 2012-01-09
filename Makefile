
all: mudp_server mudp_client

mudp_server:
	gcc -o server/mudp_server server/sctp_server1.c server/mudp_server_api.c -lpthread -lpcap -static

mudp_client:
	gcc -o client/mudp_client client/sctp_client.c client/mudp_client_api.c -lpthread -lpcap -static

clean:
	rm server/mudp_server
	rm client/mudp_client
