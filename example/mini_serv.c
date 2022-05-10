#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>

typedef struct			s_client {
	int				id;
	int				fd;
	char				*buffer;
	struct s_client	*next;
}					t_client;

typedef struct			s_server {
	int				total;
	int				sockfd;
	fd_set			reads;
	fd_set			writes;
	t_client			*clients;
}					t_server;

int				extract_message(const char *buffer, char **stack) {
	int			i = 0;
	char			*copy = NULL;

	while (buffer[i]) {
		if (buffer[i] == '\n') {
			if (!(copy = calloc(i + 2, sizeof(char))))
				return (-1);
			memcpy(copy, buffer, i + 1);
			copy[i + 1] = 0;
			*stack = copy;
			return (1);
		}
		i++;
	}

	return (0);
}

char				*str_join(char **str1, char *str2) {
	char			*merged = NULL;
	size_t		len1 = (*str1) ? strlen(*str1) : 0;
	size_t		len2 = str2 ? strlen(str2) : 0;

	if (!(merged = calloc(len1 + len2 + 1, sizeof(char))))
		return (NULL);
	if (*str1)
		memcpy(merged, *str1, len1);
	if (str2)
		memcpy(merged + len1, str2, len2);
	merged[len1 + len2] = 0;
	if (*str1)
		free(*str1);
	*str1 = NULL;
	return (merged);
}

int				send_all(t_server *server, int sender, char *content, size_t length) {
	t_client		*client = server->clients;

	while (client) {
		if (client->id != sender && FD_ISSET(client->fd, &server->writes)) {
			if (send(client->fd, content, length, 0) < 0)
				return (0);
		}

		client = client->next;
	}
	return (1);
}

t_client			*clean_client(t_server *server, t_client **client) {
	t_client		*next_client = (*client)->next;

	if ((*client)->buffer)
		free((*client)->buffer);
	(*client)->buffer = NULL;
	(*client)->next = NULL;
	FD_CLR((*client)->fd, &server->reads);
	FD_CLR((*client)->fd, &server->writes);
	close((*client)->fd);
	(*client)->fd = 0;
	free(*client);
	*client = NULL;
	return (next_client);
}

int				clean_exit(t_server *server, int return_code) {
	if (server) {
		t_client	*client = server->clients;
		while (client)
			client = clean_client(server, &client);
		server->clients = NULL;
		if (server->sockfd > 0)
			close(server->sockfd);
	}

	return return_code;
}

int				exit_fatal(t_server *server) {
	write(2, "Fatal error\n", 12);
	return clean_exit(server, 1);
}

int				main(int argc, char **argv) {
	if (argc != 2) {
		write(2, "Wrong number of arguments\n", 26);
		return (1);
	}

	t_server	server;
	server.clients = NULL;
	server.total = 0;
	server.sockfd = 0;

	int			sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return exit_fatal(&server);
	server.sockfd = sockfd;

	int				port = atoi(argv[1]);
	struct sockaddr_in	self;
	socklen_t			len = sizeof(self);

	bzero(&self, len);
	self.sin_family = AF_INET;
	self.sin_addr.s_addr = htonl(2130706433);
	self.sin_port = htons(port);

	if (bind(server.sockfd, (struct sockaddr *)&self, len) != 0)
		return exit_fatal(&server);
	if (listen(server.sockfd, 10) != 0)
		return exit_fatal(&server);

	char		buffer[65535 * 2];
	char		recv_buffer[65535];

	while (1) {
		FD_ZERO(&server.reads);
		FD_ZERO(&server.writes);
		FD_SET(server.sockfd, &server.reads);

		int		max = server.sockfd;
		t_client	*client = server.clients;
		while (client) {
			FD_SET(client->fd, &server.reads);
			FD_SET(client->fd, &server.writes);
			if (client->fd > max)
				max = client->fd;
			client = client->next;
		}

		if (select(max + 1, &server.reads, &server.writes, NULL, NULL) < 0)
			return exit_fatal(&server);
		
		if (FD_ISSET(sockfd, &server.reads)) {
			int	new_client = accept(sockfd, NULL, NULL);
			if (new_client) {
				client = NULL;
				if (!(client = (t_client*)malloc(sizeof(t_client)))) {
					close(new_client);
					return exit_fatal(&server);
				}

				client->id = server.total++;
				client->fd = new_client;
				client->buffer = NULL;
				client->next = NULL;
				if (!server.clients) {
					server.clients = client;
				} else {
					size_t	length = sprintf(buffer, "server: client %d just arrived\n", client->id);
					if (!send_all(&server, client->id, buffer, length)) {
						clean_client(&server, &client);
						return exit_fatal(&server);
					}
					t_client	*current_client = server.clients;
					while (current_client->next)
						current_client = current_client->next;
					current_client->next = client;
				}
			}
		}

		t_client	*head = server.clients;
		t_client	*previous = NULL;
		t_client	*next = NULL;

		client = server.clients;
		while (client) {
			next = client->next;
			if (client && FD_ISSET(client->fd, &server.reads)) {
				ssize_t		received = recv(client->fd, recv_buffer, 65535 - 1, 0);
				if (received <= 0) {
					size_t length = sprintf(buffer, "server: client %d just left\n", client->id);
					if (!send_all(&server, client->id, buffer, length))
						return exit_fatal(&server);
					t_client	*next_client = clean_client(&server, &client);
					if (previous)
						previous->next = next_client;
					server.clients = previous ? head : next_client;
				} else {
					recv_buffer[received] = 0;
					ssize_t		offset = 0;
					char			*line = NULL;

					while (offset < received) {
						int	extracted = extract_message(recv_buffer + offset, &line);
						if (extracted < 0)
							return exit_fatal(&server);
						else if (extracted == 0) {
							client->buffer = str_join(&(client->buffer), recv_buffer);
							offset = received;
						} else {
							size_t	line_length = strlen(line);
							offset += line_length;
							char		*to_send = str_join(&(client->buffer), line);
							free(line);
							size_t	length = sprintf(buffer, "client %d: %s", client->id, to_send);
							if (!send_all(&server, client->id, buffer, length)) {
								free(to_send);
								return exit_fatal(&server);
							}
							free(to_send);
						}
					}
				}
			}

			previous = client;
			client = next;
		}
	}
	
	return clean_exit(&server, 0);
}