#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>

typedef struct			s_message {
	int				sender;
	char				*content;
	size_t			length;
	size_t			offset;
	struct s_message	*next;
}					t_message;

typedef struct			s_client {
	int				id;
	int				fd;
	char				*buffer;
	t_message			*queue;
	struct s_client	*next;
}					t_client;

typedef struct			s_server {
	int				total;
	int				sockfd;
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
	memcpy(merged + len1, str2, len2);
	merged[len1 + len2] = 0;
	if (*str1)
		free(*str1);
	*str1 = NULL;
	return (merged);
}

int				add_message(t_server *server, int sender, char *content, size_t length) {
	t_client		*client = server->clients;

	while (client) {
		if (client->id != sender) {
			t_message	*message = NULL;
			if (!(message = (t_message*)malloc(sizeof(t_message))))
				return (0);
			if (!(message->content = (char*)malloc(length + 1))) {
				free(message);
				return (0);
			}

			strcpy(message->content, content);
			message->content[length] = 0;
			message->length = length;
			message->next = NULL;
			message->offset = 0;
			message->sender = sender;
			if (!client->queue)
				client->queue = message;
			else {
				t_message	*current_message = client->queue;
				while (current_message->next)
					current_message = current_message->next;
				current_message->next = message;
			}
		}
		client = client->next;
	}
	return (1);
}

t_client			*clean_client(t_client **client) {
	t_client		*next_client = (*client)->next;
	t_message		*current_message = (*client)->queue;
	
	while (current_message) {
		t_message	*next_message = current_message->next;
		free(current_message->content);
		current_message->content = NULL;
		current_message->next = NULL;
		free(current_message);
		current_message = next_message;
	}

	(*client)->queue = NULL;
	if ((*client)->buffer)
		free((*client)->buffer);
	(*client)->buffer = NULL;
	(*client)->next = NULL;
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
			client = clean_client(&client);
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

	char		buffer[65535];
	char		recv_buffer[65535];

	fd_set	reads;
	fd_set	writes;

	while (1) {
		FD_ZERO(&reads);
		FD_ZERO(&writes);
		FD_SET(server.sockfd, &reads);
	
		int		max = server.sockfd;
		t_client	*client = server.clients;
		while (client) {
			FD_SET(client->fd, &reads);
			if (client->queue)
				FD_SET(client->fd, &writes);
			if (client->fd > max)
				max = client->fd;
			client = client->next;
		}

		int		activity = select(max + 1, &reads, &writes, NULL, NULL);
		if (activity < 0)
			return exit_fatal(&server);
		else if (activity > 0) {
			if (FD_ISSET(sockfd, &reads)) {
				int	new_client = accept(sockfd, NULL, NULL);
				if (new_client) {
					t_client	*client = NULL;
					if (!(client = (t_client*)malloc(sizeof(t_client)))) {
						close(new_client);
						return exit_fatal(&server);
					}

					client->id = server.total++;
					client->fd = new_client;
					client->buffer = NULL;
					client->queue = NULL;
					client->next = NULL;
					if (!server.clients)
						server.clients = client;
					else {
						size_t	length = sprintf(buffer, "server: client %d just arrived\n", client->id);
						if (!add_message(&server, client->id, buffer, length)) {
							clean_client(&client);
							return exit_fatal(&server);
						}
						t_client	*current_client = server.clients;
						while (current_client->next)
							current_client = current_client->next;
						current_client->next = client;
					}
				}
			}

			t_client	*client = server.clients;
			t_client	*previous = NULL;
			t_client	*next = NULL;
			while (client) {
				next = client->next;
				if (client && FD_ISSET(client->fd, &reads)) {
					ssize_t		received = recv(client->fd, recv_buffer, 65535 - 1, 0);
					if (received <= 0) {
						size_t	length = sprintf(buffer, "server: client %d just left\n", client->id);
						if (!add_message(&server, client->id, buffer, length))
							return exit_fatal(&server);
						t_client	*next_client = clean_client(&client);
						if (!previous)
							server.clients = next_client;
						else
							previous->next = next_client;
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
								size_t	length = sprintf(buffer, "client %d: ", client->id);
								if (!add_message(&server, client->id, buffer, length)) {
									free(to_send);
									return exit_fatal(&server);
								}
								if (!add_message(&server, client->id, to_send, strlen(to_send))) {
									free(to_send);
									return exit_fatal(&server);
								}
								free(to_send);
							}
						}
						previous = client;
					}
				}

				if (client && FD_ISSET(client->fd, &writes) && client->queue) {
					t_message		*message = client->queue;
					ssize_t		sent = send(client->fd, message->content + message->offset, message->length - message->offset, 0);
					if (sent > 0) {
						if (message->offset + sent < message->length)
							message->offset += sent;
						else {
							t_message* next_message = message->next;
							free(message->content);
							free(message);
							client->queue = next_message;
						}
					}
					previous = client;
				}
				client = next;
			}
		}
	}
	
	return clean_exit(&server, 0);
}