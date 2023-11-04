#include <srt/srt.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "logka.h"

void* handle_client(void* client_sock) {
    info("Connection received [%zu]", (size_t)client_sock);
    SRTSOCKET client = *(SRTSOCKET*)client_sock;
    const char* message = "hello!\n";
    while (1) {
        int bytes_sent = srt_sendmsg(client, message, strlen(message), 0, 0);
        if (bytes_sent == SRT_ERROR) {
            error("Send failed: %s", srt_getlasterror_str());
            break;
        }
        sleep(1);
    }
    srt_close(client);
    info("Client closed [%zu]", (size_t)client_sock);
    free(client_sock);
    return NULL;
}

int main() {
    if (srt_startup() == -1) {
        error("SRT startup failed");
        exit(1);
    }

    SRTSOCKET server = srt_create_socket();
    if (server == SRT_INVALID_SOCK) {
        error("SRT socket creation failed");
        srt_cleanup();
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Binding to localhost

    if (srt_bind(server, (struct sockaddr*)&server_addr, sizeof server_addr) == -1) {
        error("SRT bind failed");
        srt_cleanup();
        exit(1);
    }

    if (srt_listen(server, 5) == -1) {
        error("SRT listen failed");
        srt_cleanup();
        exit(1);
    }

    info("Server is listening on localhost:5000");

    while(1) {
        struct sockaddr_in client_addr;
        int addrlen = sizeof client_addr;
        SRTSOCKET* client_sock = malloc(sizeof(SRTSOCKET));
        *client_sock = srt_accept(server, (struct sockaddr*)&client_addr, &addrlen);
        if (*client_sock == SRT_INVALID_SOCK) {
            error("SRT accept failed");
            free(client_sock);
            continue;
        }
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void*)client_sock);
    }

    srt_close(server);
    srt_cleanup();

    return 0;
}
