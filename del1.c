#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>


#define USER_FILE "./users"


struct sockaddr_in setup_addr_opts(int port) {
	// Setup socket options
	struct sockaddr_in address;
	address.sin_family = AF_INET;		   // Use IP (OSI layer 3)
	address.sin_addr.s_addr = INADDR_ANY;  // Listen on 0.0.0.0
	address.sin_port = htons(port);		   // Set port

	return address;
}

void open_socket(int *sockfd, struct sockaddr_in address) {
	// Create socket file descriptor.
	// Options are: AF_INET (use IPv4), SOCK_STREAM (use TCP), 0 (use the IP
	// protocol)
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {  // Error handeling
		printf("Error opening socket");
		exit(1);
	}

	// Force bind (comment some more)
	int opt = 1;
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
				   sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Attach socket to a port
	int bind_res = bind(*sockfd, (struct sockaddr *)&address, sizeof(address));
	if (bind_res < 0) {	 // Error handeling
		printf("Error binding to port");
		exit(1);
	}

	// Set socket mode to listen, and have a max backlog of 10
	listen(*sockfd, 10);
}

char *extract_request_body(char buffer[]) {
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
		else if (buffer[i] == '\n' && buffer[i - 1] == '\r' &&
				 buffer[i - 2] == '\n' && buffer[i - 3] == '\r') {
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

char *parse_JSON(char *body, char find_me[10]) {
	int in_str = 0;
	char key[50] = {0};
	char *value = malloc(50);
	int key_or_value = 0;  // 0 if key, 1 if value
	int str_or_num = 0; // 0 if str, 1 if number  
	int index = 0;

	// Loop through all chars of the JSON
	for (int i = 0; i < 1024; i++) {
		// Check if string has endend
		if (body[i] == 0x00) {
			break;
		}
		// Check if we enter/leave a string
		else if (body[i] == '"') {
			// If we're entering update stauts, and set index to 0
			if (in_str == 0) {
				in_str = 1;
				str_or_num = 0;
				index = 0;
			}
			// If we're leaving update status and null terminate string 
			else {
				in_str = 0;
				key[index] = 0x0;
				value[index] = 0x0;
			}
		}
		// If we're in a string copy it into key/value
		else if (in_str == 1) {
			if (key_or_value == 0) {
				key[index] = body[i];
				index += 1;
			} else {
				value[index] = body[i];
				index += 1;
			}
		}
		// If it's a number [0-9,\-] (ascii 0-9 numbers = 0x30-0x39) copy into value
		else if ((body[i] >= 0x30 && body[i] <= 0x39 || body[i] == '-' || body[i] == '.') && !in_str && key_or_value == 1) {
			str_or_num = 1; 
			value[index] = body[i];
			index += 1;
		}
		// Determine if it's key or value in the JSON
		else if (body[i] == ':') {
			key_or_value = 1;
			index = 0;
		} 
		// Check if current key-value pair has ended 
		else if ((body[i] == ',' || body[i] == '}') && in_str != 1) {
			// Null terminate, if the value was a int/float
			if (str_or_num == 1) {
				value[index] = 0x0;
			}
			// If the key is the one requested return the value of it
			if (strstr(find_me, key) > 0) {
				printf("%s\n", value);
				return value;
			}
			// Reset values for next key-value pair 
			key_or_value = 0;
			index = 0;
		}
	}
}

int parse_and_save(char *buffer) {
	// Handle HTTP requests
	// Check if it's a POST request
	if (strstr(buffer, "POST / HTTP/1.1") != NULL) {
		// Extract body from the request
		char *body = extract_request_body(buffer);
		printf("%s\n", body);
		char *RFID = parse_JSON(body, "RFID");
		char *weight = parse_JSON(body, "weight");
		char *type = parse_JSON(body, "type");
		printf("Got user and values: %s, %s, %s\n", RFID, weight, type);


		// Get datetime
		time_t t = time(NULL);
 		struct tm tm = *localtime(&t);
		// Write to userfile
		FILE *fp = fopen(USER_FILE, "a+"); // Append to file, create if not found
		if (strstr(type, "pap") > 0) {
			fprintf(fp, "%s %s Pap %s Metal 0 Plastik 0 %d %d\n", RFID, "Some Name", weight, tm.tm_mday, tm.tm_mon + 1);
		}
		else if (strstr(type, "metal") > 0) {
			fprintf(fp, "%s %s Pap 0 Metal %s Plastik 0 %d %d\n", RFID, "Some Name", weight, tm.tm_mday, tm.tm_mon + 1);
		}
		else if (strstr(type, "plastik") > 0) {
			fprintf(fp, "%s %s Pap 0 Metal 0 Plastik %s %d %d\n", RFID, "Some Name", weight, tm.tm_mday, tm.tm_mon + 1);
		}
		else {
			fprintf(fp, "%s %s Pap 0 Metal 0 Plastik 0 %d %d\n", RFID, "Some Name", tm.tm_mday, tm.tm_mon + 1);
		}
		fclose(fp);

		printf("Wrote to file\n");

	} else {
		return 1;
	}
}

int main() {
	// Define variables
	int sockfd, conn;

	// Open socket
	struct sockaddr_in address = setup_addr_opts(4455);
	open_socket(&sockfd, address);
	socklen_t addr_len = sizeof(address);

	// Enter loop to read from socket
	while (1) {
		// Create/clear buffer
		char buffer[1024] = {0};

		// Accept connection
		conn = accept(sockfd, (struct sockaddr *)&address, &addr_len);
		// Read to buffer
		read(conn, buffer, 1024 - 1);

		// Parse HTTP and JSON data
		parse_and_save(buffer);

		// Close connection
		close(conn);
	}

	// close socket and return with no errors (should never get here)
	close(sockfd);
	return 0;
}
