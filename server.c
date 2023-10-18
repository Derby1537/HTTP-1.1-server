
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void sendFile(FILE* file, int sockfd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    size_t bytes_read;
    while((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(sockfd, buffer, bytes_read, 0);
        memset(buffer, 0, bytes_read);
    }
}

size_t get_file_size(FILE* file) {
    fseek(file, 0, SEEK_END);
    size_t ret = ftell(file);
    rewind(file);
    return ret;
}

void respond_to_GET(char* path, int sockfd) {
    FILE* file;
    char newPath[BUFFER_SIZE];
    snprintf(newPath, BUFFER_SIZE, "./public%s", path);
    if(!(file = fopen(newPath, "rb"))) {
        FILE* not_found;
        if(!(not_found = fopen("./public/html/not_found.html", "rb"))) {
            fprintf(stderr, "500 Internal Server Error\n");
            char* response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 56\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n<html><body>Error 500 Internal Server Error</body</html>";
            send(sockfd, response, strlen(response), 0);
        } else {
            fprintf(stderr, "404 Not Found\n");
            size_t content_length = get_file_size(not_found);
            char response[BUFFER_SIZE];
            snprintf(response, BUFFER_SIZE, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: %zu\r\nConnection: keep-alive\r\n\r\n", content_length);
            send(sockfd, response, strlen(response), 0);
            sendFile(not_found, sockfd);
            fclose(not_found);
        }
    } else {
        fprintf(stderr, "200 OK\n");
        size_t content_length = get_file_size(file);
        char response[BUFFER_SIZE];
        memset(response, 0, BUFFER_SIZE);
        snprintf(response, BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\nConnection: keep-alive\r\n", content_length);
        send(sockfd, response, strlen(response), 0);
        if(!strncmp(newPath, "./public/html/", 14)) {
            send(sockfd, "Content-Type: text/html", 23, 0);
        } else if(!strncmp(newPath, "./public/img/", 13)) {
            send(sockfd, "Content-Type: image/*", 21, 0);
        } else if(!strncmp(newPath, "./public/js/", 12)) {
            send(sockfd, "Content-Type: text/javascript", 29, 0);
        } else if(!strncmp(newPath, "./public/css/", 13)) {
            send(sockfd, "Content-Type: text/css", 22, 0);
        }
        send(sockfd, "\r\n\r\n", 4, 0);
        sendFile(file, sockfd);
        fclose(file);
    }
}



struct client_info {
    int sockfd;
    struct sockaddr_in* sock;
};

void* handle_client(void* arg) {
    struct client_info *client = (struct client_info*) arg;
    int sockfd = client->sockfd;
    struct sockaddr_in sock = *client->sock;
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    size_t bytes_read;
    if((bytes_read = read(sockfd, message, BUFFER_SIZE)) <= 0) {
        fprintf(stderr, "Error receiving request\n");
        close(sockfd);
        return NULL;
    }
    char* reqMethod = strtok(message, " ");
    char* reqPath = strtok(NULL, " ");
    char* reqProtocol = strtok(NULL, "\r");
    fprintf(stderr, "%s requests %s via %s protocol -> ", inet_ntoa(sock.sin_addr), reqPath, reqProtocol);
    if(!strcmp(reqMethod, "GET")) {
        if(!strcmp(reqPath, "/")) {
            respond_to_GET("/html/index.html", sockfd);
        } else {
            respond_to_GET(reqPath, sockfd);
        }
    } else {
        fprintf(stderr, "400 Bad Request\n");
        char response[] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 46\r\nConnection: keep-alive\r\n\r\n<html><body>Error 400 Bad Request</body</html>";
        send(sockfd, response, strlen(response), 0);
    }
    pthread_join(pthread_self(), NULL);
    close(sockfd);
    return NULL;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        fprintf(stderr, "ERROR: Wrong syntax\n%s <PORT>\n", argv[0]);
        exit(-1);
    }

    int local_fd, remote_fd;
    struct sockaddr_in local_sock, remote_sock;
    socklen_t len = sizeof(local_sock);

    if((local_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error opening socket\n");
        exit(-1);
    }

    int reuseaddr = 1;
    if(setsockopt(local_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(-1);
    }

    memset(&local_sock, 0, len);
    local_sock.sin_family = AF_INET;
    local_sock.sin_addr.s_addr = htonl(INADDR_ANY);
    local_sock.sin_port = htons(atoi(argv[1]));

    if(bind(local_fd, (struct sockaddr*) &local_sock, len) < 0) {
        fprintf(stderr, "Error binding port %s\n", argv[1]);
        exit(-1);
    }

    if(listen(local_fd, 20) < 0) {
        fprintf(stderr, "Error while listening\n");
        exit(-1);
    }

    fprintf(stderr, "Listening on port %s\n", argv[1]);

    while(1) {
        remote_fd = accept(local_fd, (struct sockaddr*) &remote_sock, &len);
        if(accept <= 0) {
            perror("Accept failed");
            continue;
        }
        struct client_info client;
        client.sockfd = remote_fd;
        client.sock = &remote_sock;
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, (void*) &client);
    }

    close(local_fd);
    exit(0);
}
