#include "../../../networking_libaries.h"
#include <iostream>

const char* get_content_type(const char* path)
{
	const char* last_dot = strrchr(path, '.');
	if (last_dot)
	{
		if (strcmp(last_dot, ".css") == 0) return "text/css";
		if (strcmp(last_dot, ".csv") == 0) return "text/csv";
		if (strcmp(last_dot, ".gif") == 0) return "image/gif";
		if (strcmp(last_dot, ".htm") == 0) return "text/htm";
		if (strcmp(last_dot, ".html") == 0) return "text/html";
		if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
		if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
		if (strcmp(last_dot, ".jpg") == 0) return "image/jpg";
		if (strcmp(last_dot, ".js") == 0) return "application/javascript";
		if (strcmp(last_dot, ".json") == 0) return "application/json";
		if (strcmp(last_dot, ".png") == 0) return "image/png";
		if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
		if (strcmp(last_dot, ".svg") == 0) return "text/svg+xml";
		if (strcmp(last_dot, ".txt") == 0) return "text/plain";
	}
	return "application/octet-stream";

}

SOCKET create_socket(const char* host, const char* port)
{
	std::cout << "Configuring local address...\n";
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo* bind_address;
	if (getaddrinfo(host, port, &hints, &bind_address))
	{
		std::cout << "getaddrinfo() failed with error: " << GETSOCKETERRNO() << "\n";
		exit(EXIT_FAILURE);
	}
	SOCKET socket_listen;
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
	{
		std::cout << "socket() has failed with error: " << GETSOCKETERRNO() << "\n";
		exit(EXIT_FAILURE);
	}

	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
	{
		std::cout << "bind() failed with error: " << GETSOCKETERRNO() << "\n";
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(bind_address);
	std::cout << "Listening...\n";
	if (!listen(socket_listen, 10) < 0)
	{
		std::cout << "listen() failed with error: " << GETSOCKETERRNO() << "\n";
	}
	return socket_listen;
}

#define MAX_REQUEST_SIZE 2047

struct client_info
{
	socklen_t address_length;
	struct sockaddr_storage address;
	SOCKET socket;
	char request[MAX_REQUEST_SIZE + 1];
	int recieved;
	struct client_info* next;
};

static struct client_info* clients = nullptr;

struct client_info* get_client(SOCKET s)
{
	struct client_info* ci = clients;
	while (ci)
	{
		if (ci->socket == s) break;
		ci = ci->next;
	}
	if (ci) return ci;

	struct client_info* n = (struct client_info*) new(struct client_info);

	if (!n)
	{
		std::cout << "Out of memory\n";
		exit(EXIT_FAILURE);
	}
	n->address_length = sizeof(n->address);
	n->next = clients;
	clients = n;
	return n;
}

void drop_client(struct client_info* client)
{
	CLOSESOCKET(client->socket);
	struct client_info** p = &clients;
	while (*p)
	{
		if (*p == client)
		{
			*p = client->next;
			delete(client);
			return;
		}
		p = &(*p)->next;
	}
	std::cout << "drop_client not found\n";
	exit(EXIT_FAILURE);
}

const char* get_client_address(struct client_info* ci)
{
	static char address_buffer[100];
	getnameinfo((struct sockaddr*)&ci->address, ci->address_length, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
	return address_buffer;
}

fd_set wait_on_clients(SOCKET server)
{
	fd_set reads;
	FD_ZERO(&reads);
	FD_SET(server, &reads);
	SOCKET max_socket = server;
	struct client_info* ci = clients;
	while (ci)
	{
		FD_SET(ci->socket, &reads);
		if (ci->socket > max_socket) {
			max_socket = ci->socket;
			ci = ci->next;
		}
		if (select(max_socket + 1, &reads, 0, 0, 0) < 0)
		{
			std::cout << "select() failed with error: " << GETSOCKETERRNO() << "\n";
			exit(EXIT_FAILURE);
		}
		return reads;
	}
}
void send_400(struct client_info *client)
{
	const char* c400 = "HTTP/1.1 400 Bad Request\r\n"
		"Connection: close\r\n"
		"Content-Length: 11\r\n\r\nBad Request";
	send(client->socket, c400, strlen(c400), 0);
	drop_client(client);
}
void send_404(struct client_info* client)
{
	const char* c404 = "HTTP/.1 404 Not Found\r\n"
		"Connection: close\r\n"
		"Content-Length: 9\r\n\r\nNot Found";
	send(client->socket, c404, strlen(c404), 0);
}



int main()
{

}