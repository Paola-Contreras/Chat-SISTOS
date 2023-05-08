#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.pb-c.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

char* get_local_ip_address() {
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));

    struct hostent *host = gethostbyname(hostname);
    if (host == NULL) {
        printf("Error al obtener la dirección IP local.\n");
        return NULL;
    }

    struct in_addr **addr_list = (struct in_addr **)host->h_addr_list;
    char *ip_address = malloc(INET_ADDRSTRLEN * sizeof(char));
    if (ip_address == NULL) {
        printf("Error al reservar memoria para la dirección IP.\n");
        return NULL;
    }

    for (int i = 0; addr_list[i] != NULL; i++) {
        inet_ntop(AF_INET, addr_list[i], ip_address, INET_ADDRSTRLEN);
        return ip_address;
    }

    free(ip_address);
    return NULL;
}


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


int main(int argc, char **argv) {

    int client_sock;
    struct sockaddr_in server_addr;
    uint8_t recv_buffer_confirmationNewUser[BUFFER_SIZE];
    char mensaje[1024];
    int bytes_received;
    int opc = -1; // Inicializar con un valor inválido
    int userCreated;

    // Administrar parámetros de compilación
    char *username = argv[1];
    char *ip = argv[2];
    char *puerto = argv[3];

    // Crear socket
    client_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(atoi(puerto));

    // Conectar con servidor
    connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Conectado al servidor\n");
    printf("BIENVENIDO AL CHAT %s.\n", username);

    // Hacer un useroption para crear el usuario
    

    // Crear nuevo usuario y enviarlo
    ChatSistOS__NewUser user = CHAT_SIST_OS__NEW_USER__INIT;
    // Agregar parámetros de Usuario
    user.username = username;
    user.ip = get_local_ip_address();
    
    // Serializar
    size_t userSize = chat_sist_os__new_user__get_packed_size(&user);
    uint8_t *buffer = malloc(userSize);
    chat_sist_os__new_user__pack(&user, buffer);
    send(client_sock, buffer, userSize, 0);


    // Recibir confirmación de creación de usuario.
    bytes_received = recv(client_sock, recv_buffer_confirmationNewUser, sizeof(recv_buffer_confirmationNewUser), 0);
    ChatSistOS__Answer *answ = chat_sist_os__answer__unpack(NULL, bytes_received, recv_buffer_confirmationNewUser);
    if (bytes_received > 0) {
        bytes_received = 0;
        // Usuario creado
        if (answ->response_status_code == 200) {
            printf("%s\n", answ->response_message);

            // Menú de chat.
            while (opc != 7) {
                opc = menu();
                if (opc != -1) {

                    // Instanciar option
                    ChatSistOS__UserOption opcUser = CHAT_SIST_OS__USER_OPTION__INIT;
                    ChatSistOS__Status status = CHAT_SIST_OS__STATUS__INIT;
                    ChatSistOS__Message msj = CHAT_SIST_OS__MESSAGE__INIT;

                    switch (opc) {
                        case 1:
                            status.user_name = username;
                            status.user_state = 1;

                            // Pedir mensaje
                            printf("Ingrese el mensaje que desea enviar a todos los usuarios: \n");
                            fgets(mensaje, sizeof(mensaje), stdin);

                            // Agregar parámetros de Message
                            msj.message_private = false;
                            strcpy(msj.message_content, mensaje);
                            strcpy(msj.message_sender, username);

                            // Agregar parámetros de la opción del usuario
                            opcUser.op = opc;
                            opcUser.createuser = &user;
                            opcUser.status = &status; // Activo
                            opcUser.message = &msj;

                            // Serializar 
                            size_t sizeOpc = chat_sist_os__user_option__get_packed_size(&opcUser);
                            uint8_t *buffer_opc = malloc(sizeOpc);
                            chat_sist_os__user_option__pack(&opcUser, buffer_opc);

                            // Enviar a servidor
                            send(client_sock, buffer_opc, sizeOpc, 0);
                    }
                    
                }
            }
        } else {
            // El usuario ya existía.
            printf("%s\n", answ->response_message);
        }
    }

    close(client_sock);
    return 0;
}

