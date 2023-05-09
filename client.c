#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include "chat.pb-c.h"

char username[1024];

// void* client_msj_recv(void* arg) {
//     int client_sock = *((int*)arg);

//     ChatSistOS__Answer* answ = NULL;
//     while (1) {
//         uint8_t buffer[4096];
//         ssize_t bytes_recv = recv(client_sock, buffer, sizeof(buffer), 0);
//         if (bytes_recv < 0) {
//             perror("recv");
//             break;
//         } else if (bytes_recv == 0) {
//             printf("Server disconnected\n");
//             break;
//         }

//         answ = chat_sist_os__answer__unpack(NULL, bytes_recv, buffer);
//         if (answ == NULL) {
//             printf("Error unpacking message\n");
//             break;
//         }

//         // Verificar si el mensaje es para este usuario
//         if (strcmp(answ->message->message_destination, username) == 0) {
//             printf("%s: %s\n", answ->message->message_sender, answ->message->message_content);
//         }

//         // Liberar memoria
//         chat_sist_os__answer__free_unpacked(answ, NULL);
//     }

//     close(client_sock);
//     return NULL;
// }



void help(){
    printf("\n\nAYUDA\n");
    printf("-> Si deseas ver todos los mensajes del chat general debes seleccionar la opcinon 1\n");
    printf("-> Si deseas habalar con un usuario por privado debes colocar el username de la persona con la que deseas hablar\n");
    printf("-> Si desea cambiar su estado debe de selecionar la opcion 3. Cabe destacar que depues de 1 minuto su estatus cambia a inactivo\n");
    printf("-> Si desea observar todos los usuarios que se encuentran conectados debe de seleccionar 4\n");
    printf("-> Si desea var la informacion de un usuario en especifico debe de determinar el username de este y seleccionar la opcion 5\n");
    printf("-> Si desea salir del chat presione 7 \n");
}


int menu() {
    int option;

    printf("\n\nMENU\n");
    printf("1. Chatear con todos los usuarios\n");
    printf("2. Enviar o recibir mensajes privados\n");
    printf("3. Cambiar estatus\n");
    printf("4. Listar usuarios conectados\n");
    printf("5. Desplegar informacion de un usuario\n");
    printf("6. Ayuda\n");
    printf("7. Salir\n");

    printf("\n>> Ingrese una opcion:\n");
    scanf("%d", &option);
    printf("\n\n\n");
    return option;
}


int main() {

    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char mensaje[256];
    char userDest[256];
    int bytes_received;
    int opc = -1; // Inicializar con un valor inválido
    int userCreated;

    client_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(12345);

    connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Conectado al servidor\n");
    printf("BIENVENIDO AL CHAT.\n");

    printf("Ingrese el nombre de usuario: ");
    fgets(username, sizeof(username), stdin);
    send(client_sock, username, strlen(username), 0);

    // Recibir confirmación de creación de usuario.
    bytes_received = recv(client_sock, &userCreated, sizeof(userCreated), 0);
    if (bytes_received > 0) {
        bytes_received = 0;

        // Usuario creado
        if (userCreated == 1) {
            printf("Usuario creado.\n");

            // // Crear thread que revice si recibe mensajes.
            // pthread_t thread;
            // if (pthread_create(&thread, NULL, client_msj_recv, (void *)&client_sock) < 0) {
            //     perror("@! El thread encargado de recibir mensajes no pudo ser creado.");
            //     exit(1);
            // }

            // Menú de chat.
            while (opc != 7) {
                opc = menu();
                if (opc != -1) {
                    // Enviar opción seleccionada
                    if (opc == 1) {
                        send(client_sock, &opc, sizeof(opc), 0);
                        
                        // Instanciar Message
                        ChatSistOS__Message msj = CHAT_SIST_OS__MESSAGE__INIT;
                        
                        // Pedir mensaje
                        printf("Ingrese el mensaje que desea enviar a todos los usuarios: \n");
                        getchar(); 
                        scanf("%199[^\n]%*c", mensaje);

                        // Agregar parámetros de Message
                        msj.message_private = false;
                        msj.message_content = strdup(mensaje);
                        msj.message_sender = strdup(username);
                        
                        // Serializar mensaje
                        size_t sizeMsj = chat_sist_os__message__get_packed_size(&msj);
                        uint8_t *buffer_msj = malloc(sizeMsj);
                        chat_sist_os__message__pack(&msj, buffer_msj);

                        printf("Buffer contents: %s\n", buffer_msj);

                        // Enviar
                        send(client_sock, buffer_msj, sizeMsj, 0);

                    } else if ( opc == 2 ) {
                        send(client_sock, &opc, sizeof(opc), 0);
                        // Instanciar Message
                        ChatSistOS__Message msj = CHAT_SIST_OS__MESSAGE__INIT;
                        
                        // Pedir usuario destino
                        printf(">> Ingrese el usuario destino: \n");
                        getchar(); 
                        scanf("%[^\n]%*c", userDest);

                        // Pedir mensaje
                        printf(">> Ingrese el mensaje que desea enviar: \n");
                        getchar(); 
                        scanf("%[^\n]%*c", mensaje);

                        // Agregar parámetros de Message
                        msj.message_private = false;
                        strcpy(msj.message_destination, userDest);
                        strcpy(msj.message_content, mensaje);
                        strcpy(msj.message_sender, username);
                        
                        // Serializar mensaje
                        size_t sizeMsj = chat_sist_os__message__get_packed_size(&msj);
                        uint8_t *buffer_msj = malloc(sizeMsj);
                        chat_sist_os__message__pack(&msj, buffer_msj);

                        // Enviar
                        // send(client_sock, buffer_msj, sizeMsj, 0);
                    } else if (opc == 3 || opc == 4 || opc == 5) {
                        send(client_sock, &opc, sizeof(opc), 0);
                        // Imprimir resultado de opción seleccionada.
                        bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
                        if (bytes_received > 0) {
                            buffer[bytes_received] = '\0';
                            printf("%s", buffer);
                        }
                    } 
                }
            }
        } else {
            // El usuario ya existía.
            printf("No se pudo crear el usuario. Ya existe un usuario con el mismo nombre de usuario y/o dirección IP.\n");
        }
    }

    close(client_sock);
    return 0;
}

