
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include "chat.pb-c.h"

#ifndef COMMON_H
#define COMMON_H

#include <netinet/in.h>
#include <stdbool.h>

#define MAX_USERS 10
#define BUFFER_SIZE 500
#define TIME_LIMIT 30

typedef struct {
    ChatSistOS__User users[MAX_USERS];
    int count;
} user_list_t;

extern user_list_t user_list;
extern int current_socket_id;

#endif // COMMON_H


user_list_t user_list;
int current_socket_id = 0;

// Función para buscar un usuario por nombre de usuario
ChatSistOS__User *find_user_by_username(char *username) {
    for (int i = 0; i < user_list.count; i++) {
        if (strcmp(user_list.users[i].user_name, username) == 0) {
            return &user_list.users[i];
        }
    }
    return NULL;
}

bool user_exists(const char *username) {
    for (int i = 0; i < user_list.count; i++) {
        if (strcmp(user_list.users[i].user_name, username) == 0) {
            return true;
        }
    }
    return false;
}

int add_user(char *usernameC) {
    ChatSistOS__User user = CHAT_SIST_OS__USER__INIT;

    user.user_name = malloc(strlen(usernameC) + 1); // Asignar memoria para user_name
    if (user.user_name != NULL) {
        strcpy(user.user_name, usernameC);
    }
    user.user_state = 1;

    if (user_list.count < MAX_USERS && !user_exists(usernameC)) {
        user_list.users[user_list.count++] = user;
        return 1;
    } else {
        return 0;
    }
}


void remove_user(const char* username) {
    for (int i = 0; i < user_list.count; i++) {
        if (strcmp(user_list.users[i].user_name, username) == 0) {
            for (int j = i; j < user_list.count - 1; j++) {
                user_list.users[j] = user_list.users[j + 1];
            }
            user_list.count--;
            break;
        }
    }
}


void send_message(int client_sock, ChatSistOS__Message *chat_message) {

    ChatSistOS__Answer answ = CHAT_SIST_OS__ANSWER__INIT;
    
    
    for (int i = 0; i < user_list.count; i++) {
        // Ver si es un broadcast
        if (chat_message->message_private == false){
            // Verificar que esté conectado
            if (user_list.users[i].user_state == 1) {
                // Serializar respuesta
                chat_message->message_destination = strdup(user_list.users[i].user_name);

                size_t answSize = chat_sist_os__answer__get_packed_size(&answ);
                uint8_t *answbuffer = malloc(answSize);
                chat_sist_os__answer__pack(&answ, answbuffer);

                // Enviar
                send(client_sock, answbuffer, answSize, 0);
            }
            
        } else {
            // Ver si es privado
            if (strcmp(user_list.users[i].user_name, chat_message->message_destination) == 0 && user_list.users[i].user_state == 1) {
                size_t answSize = chat_sist_os__answer__get_packed_size(&answ);
                uint8_t *answbuffer = malloc(answSize);
                chat_sist_os__answer__pack(&answ, answbuffer);

                // Enviar
                send(client_sock, answbuffer, answSize, 0);
            }
        }
    }
}


// Cambiar status
char* toggle_user_state(const char *username) {
    static char message[1024]; // Buffer para el mensaje
    for (int i = 0; i < user_list.count; i++) {
        if (strcmp(user_list.users[i].user_name, username) == 0) {
            user_list.users[i].user_state = !user_list.users[i].user_state;
            sprintf(message, "El estado del usuario %s ha cambiado a %s.\n", username,
                   user_list.users[i].user_state ? "Conectado" : "Desconectado");
            return message;
        }
    }

    sprintf(message, "No se encontró el usuario con el nombre de usuario: %s\n", username);
    return message;
}


// Listar usuarios conectados
char* list_active_users() {
    static char message[1024]; // Buffer para el mensaje
    int message_length = 0; // Longitud actual del mensaje

    // Agregar el encabezado del mensaje
    message_length += sprintf(message + message_length, "Usuarios activos:\n");

    // Agregar los nombres de los usuarios activos
    for (int i = 0; i < user_list.count; i++) {
        if (user_list.users[i].user_state) {
            message_length += sprintf(message + message_length, "%s\n", user_list.users[i].user_name);
        }
    }

    return message;
}


char *get_user_info(const char *username) {
    static char info[1024];  // Buffer para almacenar la información del usuario
    for (int i = 0; i < user_list.count; i++) {
        if (strcmp(user_list.users[i].user_name, username) == 0 && user_list.users[i].user_state) {
            sprintf(info, "\n\nInformación del usuario %s:\nEstado: %s\n", 
                    username, user_list.users[i].user_state ? "Conectado" : "Desconectado");
            return info;
        }
    }

    sprintf(info, "No se encontró un usuario activo con el nombre de usuario: %s\n", username);
    return info;
}


void *client_handler(void *arg);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pthread_t thread_id;

    server_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(12345);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    while (1) {
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);

        printf("Cliente conectado\n");
        pthread_create(&thread_id, NULL, client_handler, (void *)&client_sock);
        pthread_detach(thread_id);
    }

    close(server_sock);
    return 0;
}

void *client_handler(void *arg) {
    int client_sock = *((int *)arg);
    ssize_t received_size_opc;
    time_t last_active_time;
    uint8_t buffermsj[4096];
    ssize_t received_size;
    ssize_t bytes_recv_m;
    char username[1024];
    int addUserReturn;
    int opc;

    while ((received_size = recv(client_sock, username, sizeof(username) - 1, 0)) > 0) {
        username[received_size] = '\0';
        printf("Username recibido: %s", username);
        addUserReturn = add_user(username);
        send(client_sock, &addUserReturn, sizeof(addUserReturn), 0);

        // Recibir la opción del menú
        int opc;
        char *returnValue;

        while (opc != 7) {
            if ((received_size_opc = recv(client_sock, &opc, sizeof(opc), 0)) <= 0) {
                // Error o el cliente ha cerrado la conexión
                break;
            }
            printf("El cliente %s seleccionó la opción %d\n", username, opc);

            if (opc == 1){
                while (1) {
                    bytes_recv_m = recv(client_sock, buffermsj, sizeof(buffermsj), 0);
                    if (bytes_recv_m > 0) {
                        // Deserializar mensaje recibido
                        ChatSistOS__Message *chat_message = chat_sist_os__message__unpack(NULL, bytes_recv_m, buffermsj);
                        if (chat_message == NULL) {
                            printf("Error unpacking message\n");
                            break;
                        }

                        // Enviar mensaje a los usuarios correspondientes
                        send_message(client_sock, chat_message);

                        // Liberar memoria del mensaje recibido
                        chat_sist_os__message__free_unpacked(chat_message, NULL);

                        // Salir del loop si se recibió algo
                        break;
                    } else {
                        // Si no se recibió nada, continuar con la siguiente iteración
                        continue;
                    }
                }

            }

            // Procesar la opción del menú
            switch (opc) {
                case 1:

                    break;
                case 2:
                    
                    while (1) {
                        bytes_recv_m = recv(client_sock, buffermsj, sizeof(buffermsj), 0);
                        if (bytes_recv_m > 0) {
                            // Deserializar mensaje recibido
                            ChatSistOS__Message *chat_message = chat_sist_os__message__unpack(NULL, bytes_recv_m, buffermsj);
                            if (chat_message == NULL) {
                                printf("Error unpacking message\n");
                                break;
                            }

                            // Enviar mensaje a los usuarios correspondientes
                            send_message(client_sock, chat_message);

                            // Liberar memoria del mensaje recibido
                            chat_sist_os__message__free_unpacked(chat_message, NULL);

                            // Salir del loop si se recibió algo
                            break;
                        }
                    }
                    
                    break;
                case 3:
                    // Cambiar estatus
                    returnValue = toggle_user_state(username);
                    send(client_sock, returnValue, strlen(returnValue), 0);

                    break;
                case 4:
                    // Listar usuarios conectados
                    returnValue = list_active_users();
                    send(client_sock, returnValue, strlen(returnValue), 0);

                    break;
                case 5:
                    // Desplegar información de un usuario
                    returnValue = get_user_info(username);
                    send(client_sock, returnValue, strlen(returnValue), 0);

                    break;
                case 6:
                    // Ayuda
                    //help();
                    break;
                case 7:
                    // Salir del menú
                    remove_user(username);
                    printf("Saliendo...\n");
                    break;
                default:
                    printf("Opción inválida\n");
                    break;
            }
        }

        if (opc == 7) {
            break;
        }
    }

    printf("Cliente desconectado\n");
    close(client_sock);
    return NULL;
}
