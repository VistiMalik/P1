// TODO: Parse JSON
// TODO: Save to files
// TODO: Optimize some places



#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


struct weight_data {
    char RFID[8];
    int type; 
    float weight;
};
typedef struct weight_data weight_data;

struct sockaddr_in setup_addr_opts(int port) {
    // Setup socket options
    struct sockaddr_in address;
    address.sin_family = AF_INET; // Use IP (OSI layer 3)
    address.sin_addr.s_addr = INADDR_ANY; // Listen on 0.0.0.0
    address.sin_port = htons(port); // Set port

    return address;
}

void open_socket(int *sockfd, struct sockaddr_in address) {
    // Create socket file descriptor. 
    // Options are: AF_INET (use IPv4), SOCK_STREAM (use TCP), 0 (use the IP protocol)
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { // Error handeling
        printf("Error opening socket");
        exit(1);
    }


    // Force bind (comment some more)
    int opt = 1;
    if (setsockopt(*sockfd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    // Attach socket to a port
    int bind_res = bind(*sockfd, (struct sockaddr*)&address, sizeof(address));
    if (bind_res < 0) { // Error handeling
        printf("Error binding to port");
        exit(1);
    }

    // Set socket mode to listen, and have a max backlog of 10
    listen(*sockfd, 10);     
}

char * extract_request_body(char buffer[]) {
    int request_size = strlen(buffer);
    int body_index = -1;
    char *body = malloc(1024); 

    // Go through whole request untill we meet the body
    for (int i = 0; i < request_size; i++) {
        // If buffer[i] == null, then the string is done and break out 
        if (buffer[i] == 0) {
            break;
        }
        // Check if we've meet \n\r\n\r 
        else if (buffer[i] == '\n' && buffer[i-1] == '\r' && buffer[i-2] == '\n' && buffer[i-3] == '\r') { 
            body_index = 0;
        }

        // If we've meet \n\r\n\r then append char to body
        if (body_index != -1) { 
            body[body_index] = buffer[i];
            body_index += 1;
        }
    }
    return body;
}


weight_data parse_JSON(char *body, char find_me[10]) {
    weight_data my_data;
    int in_str = 0;
    char key[50] = {0};
    char value[50] = {0};
    int key_or_value = 0; // 0 if key, 1 if value
    int index = 0;


    // Check if string has endend
    for (int i = 0; i < 1024; i++) {
        if (body[i] == 0x00) {
            break;
        }
        // Check if were 
        else if (body[i] == '"') {
            // Check if were currently in a string
            if (in_str == 0) {
                in_str = 1;
                index = 0;
            }
            else {
                in_str = 0;
                key[index] = 0x0;
                value[index] = 0x0;
            }
        }
        // Determine if it's key or value in the JSON
        else if (body[i] == ':') {
            key_or_value = 1;
        }
        else if (body[i] == ',' && in_str != 1) {
            key_or_value = 0;
        }


        // Copy string into tmp_str
        if (in_str == 1) {
            if (key_or_value == 0) {
                key[index] = body[i];
                index += 1;
            }
            else {
                value[index] = body[i];
                index += 1;
            }
        }

        


    }

    return my_data;

}

int parse_data(char *buffer) {
    
    // Handle HTTP requests
    // Check if it's a POST request
    if (strstr(buffer, "POST / HTTP/1.1") != NULL) {
        // Extract body from the request
        char *body = extract_request_body(buffer);
	    printf("%s\n", body);
       	weight_data to_save = parse_JSON(body);

    } 
    else {
        return 1;
    }
}

int main()
{
    // Define variables
    int sockfd, conn;


    // Open socket
    struct sockaddr_in address = setup_addr_opts(4455); 
    open_socket(&sockfd, address);
    socklen_t addr_len = sizeof(address);

    // Enter loop to read from socket
    while (1) {
        // Create/clear buffer
        char buffer[1024] = { 0 };

        // Accept connection
        conn = accept(sockfd, (struct sockaddr*)&address, &addr_len); 
        // Read to buffer
        read(conn, buffer, 1024-1);

        // Parse HTTP and JSON data
        parse_data(buffer);

        // Close connection
        close(conn);
    }
    
    // close socket and return with no errors (should never get here)
    close(sockfd);
    return 0;
}
