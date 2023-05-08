
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
#include <arpa/inet.h>
#include "chat.pb-c.h"

#ifndef COMMON_H
#define COMMON_H

#include <netinet/in.h>
#include <stdbool.h>

#define MAX_USERS 10
#define BUFFER_SIZE 1024

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
    user.user_state = true;

    if (user_list.count < MAX_USERS && !user_exists(usernameC)) {
        user_list.users[user_list.count++] = user;
        return 1;
    } else {
        return 0;
    }
}

// TODO: Modificar para que ahora borre según username
// void remove_user(int username) {
//     for (int i = 0; i < user_list.count; i++) {
//         if (user_list.users[i].socket_id == socket_id) {
//             for (int j = i; j < user_list.count - 1; j++) {
//                 user_list.users[j] = user_list.users[j + 1];
//             }
//             user_list.count--;
//             break;
//         }
//     }
// }

// void send_message(message_t chat_message, int destination_socket_id) {
//     // Serializar la estructura message_t en un buffer
//     char buffer[BUFFER_SIZE];
//     snprintf(buffer, sizeof(buffer), "Privado: %s | Destinatario: %s | Mensaje: %s | Remitente: %s",
//              chat_message.message_private ? "Sí" : "No",
//              chat_message.message_destination,
//              chat_message.message_content,
//              chat_message.message_sender);

//     // Enviar el buffer al destinatario utilizando el socket_id proporcionado
//     if (send(destination_socket_id, buffer, strlen(buffer), 0) == -1) {
//         perror("Error al enviar el mensaje");
//     }
// }

// void send_broadcast_message(const char *messageContent, const char *destination, bool private) {
//     message_t chat_message;

//     for (int i = 0; i < user_list.count; i++) {
//         if (user_list.users[i].socket_id != sender_socket_id
//                 && (destination == NULL || strcmp(destination, user_list.users[i].username) == 0)) {
//             chat_message.message_private = private;
//             strcpy(chat_message.message_destination, user_list.users[i].username);
//             strcpy(chat_message.message_content, messageContent);
//             strcpy(chat_message.message_sender, find_user_by_socket_id(sender_socket_id)->username);
//             send_message(chat_message, user_list.users[i].socket_id);
//         }
//     }
// }

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

int main(int argc, char **argv) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pthread_t thread_id;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    server_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

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
    uint8_t recv_buffer_newUser[BUFFER_SIZE];
    uint8_t recv_buffer_opc[BUFFER_SIZE];
    int client_sock = *((int *)arg);
    size_t received_size_opc;
    size_t recv_size_newUser;
    char responsemsj[360];
    int addUserReturn;
    int opc;

    // Recibe el NewUser
    while ((recv_size_newUser = recv(client_sock, recv_buffer_newUser, sizeof(recv_buffer_newUser), 0)) > 0) {

        
        // Deserializar NewUser
        ChatSistOS__NewUser *user = chat_sist_os__new_user__unpack(NULL, recv_size_newUser, recv_buffer_newUser);

        printf("Username recibido:%s\n", user->username);
        addUserReturn = add_user(user->username);

        // Enviar confirmación de creación de usuario
        ChatSistOS__Answer answ = CHAT_SIST_OS__ANSWER__INIT;
        if (addUserReturn == 1) {
            answ.response_status_code = 200;
            strcpy(responsemsj, "Usuario creado exitosamente!");
            answ.response_message = responsemsj;
        } else {
            answ.response_status_code = 400;
            strcpy(responsemsj, "@! Error el usuario ya existe.");
            answ.response_message = responsemsj;
        }

        printf("STATUS: %d\n.", answ.response_status_code);

        // Serializar respuesta
        size_t answSize = chat_sist_os__answer__get_packed_size(&answ);
        uint8_t *answbuffer = malloc(answSize);
        chat_sist_os__answer__pack(&answ, answbuffer);
        send(client_sock, answbuffer, answSize, 0);

        // Recibir la opción del menú
        int opc;
        char *returnValue;

        while (1) {
            received_size_opc = recv(client_sock, &opc, sizeof(opc), 0);
            // Deserializo
            ChatSistOS__UserOption *opcV = chat_sist_os__user_option__unpack(NULL, received_size_opc, recv_buffer_opc);

            printf("El cliente %s seleccionó la opción %d\n", user->username, opcV->op);
            opc = opcV->op;

            // // Procesar la opción del menú
            // switch (opc->op) {
            //     case 1:
            //         // Chatear con todos los usuarios
            //         // Recibir mensaje de cliente

            //         // Almacenarlo
            //         // Enviarlo a todos los usuarios conectados

            //         // send_broadcast_message('messageContent', NULL, false);
            //         break;
            //     case 2:
            //         // Enviar o recibir mensajes privados
                    
            //         break;
            //     case 3:
            //         // Cambiar estatus

            //         break;
            //     case 4:
            //         // Listar usuarios conectados

            //         break;
            //     case 5:
            //         // Desplegar información de un usuario


            //         break;
            //     case 6:
            //         // Ayuda
            //         // help();
            //         break;
            //     case 7:
            //         // Salir del menú
            //         printf("Saliendo...\n");
            //         break;
            //     default:
            //         printf("Opción inválida\n");
            //         break;
            // }
        }

        if (opc == 7) {
            break;
        }
    }

    printf("Cliente desconectado\n");
    close(client_sock);
    return NULL;
}
