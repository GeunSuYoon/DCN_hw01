// NXC Data Communications Network http_engine.c for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "http_functions.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

#define MAX_WAITING_CONNECTIONS 10 // Maximum number of waiting connections
#define MAX_PATH_SIZE 256 // Maximum size of path
#define SERVER_ROOT "./server_root"
#define ALBUM_PATH "/public/album"
#define ALBUM_HTML_PATH "./server_root/public/album/album_images.html"
#define ALBUM_HTML_TEMPLATE "<div class=\"card\"> <img src=\"/public/album/%s\" alt=\"Unable to load %s\"> </div>\n"

void processMultipartFormData(char *boundary, char *data, size_t dataSize) {
    char *part = strtok(data, boundary);
    while (part != NULL) {
        printf("%s\n", part);
        part = strtok(NULL, boundary);
    }
}
char	*find_content_type(char *file_path, char *request_accept)
{
	char	*tmp_path = copy_string(file_path);
	char	*tmp_acpt = copy_string(request_accept);
	char	*extention = strtok(tmp_path + 1, ".");
	extention = strtok(NULL, ".");
	
	if (strstr(tmp_acpt, extention) == NULL)
	{
		free(tmp_path);
		free(tmp_acpt);
		return (copy_string("ASD"));
	}
	char	*content_type = strtok(tmp_acpt, ",;");
	while (strstr(content_type, extention) == NULL)
	{
		content_type = strtok(NULL, ",;");
	}
	free(tmp_path);
	free(tmp_acpt);
	return (content_type);
}

char	*string_cutter(char *start, char *end)
{
	if (start == NULL || end == NULL)
		return (NULL);
	size_t	str_len = 0;
	while (start[str_len] != *end)
		str_len++;
	char	*ret_char = (char *)calloc(str_len + 1, sizeof(char));
	if (ret_char == NULL)
		return (NULL);
	size_t	str_cnt = 0;

	while (str_cnt < str_len)
	{
		ret_char[str_cnt] = start[str_cnt];
		str_cnt ++;
	}
	return (ret_char);
}

// http_t	*post_body(char *header_buffer, http_t *request)
// {
// 	http_t	*ret_http = init_http();
// 	char	*tmp = strstr(find_http_field_val(request, "Content-Type"), "boundary=");
// 	if (tmp == NULL)
// 		return (NULL);
// 	tmp += strlen("boundary=");
// 	char	*asdf = strstr(header_buffer, tmp);
// 	printf("%s\n", asdf);
// 	asdf = strstr(asdf + strlen(tmp), tmp);
// 	printf("%s\n", asdf);
// 	return (ret_http);
// }

// You are NOT REQUIRED to implement and use parse_http_header() function for this project.
// However, if you do, you will be able to use the http struct and its member functions,
// which will make things MUCH EASIER for you. We highly recommend you to do so.
// HINT: Use strtok() to tokenize the header strings, based on the delimiters.
http_t *parse_http_header (char *header_str)
{
    http_t *http = init_http();
	char	*string_cut_ptr1 = header_str;
	char	*string_cut_ptr2 = strstr(header_str, " ");
	char	*http_method = string_cutter(string_cut_ptr1, string_cut_ptr2);
	char	*http_path = string_cutter(string_cut_ptr2 + 1, string_cut_ptr1 = strstr(string_cut_ptr2 + 1, " "));
	char	*http_version = string_cutter(string_cut_ptr1 + 1 , string_cut_ptr2 = strstr(string_cut_ptr1 + 1, "\r\n"));
	char	*http_field;
	char	*http_val;

	if (http_method == NULL || http_path == NULL || http_version == NULL)
	{
		free_http(http);
		return (NULL);
	}
	http = init_http_with_arg(http_method, http_path, http_version, "");
	free(http_method);
	free(http_path);
	free(http_version);
	http->status = NULL;
	while (strncmp(string_cut_ptr2, "\r\n\r\n", 4) != 0)
	{
		http_field = string_cutter(string_cut_ptr2 + 2,string_cut_ptr1 = strstr(string_cut_ptr2 + 2, ":"));
		if (http_field == NULL)
		{
			free_http(http);
			return (NULL);
		}
		http_val = string_cutter(string_cut_ptr1 + 2,string_cut_ptr2 = strstr(string_cut_ptr1 + 2, "\r\n"));
		if (http_val == NULL)
		{
			free(http_field);
			free_http(http);
			return (NULL);
		}
		if (add_field_to_http(http, http_field, http_val) == -1)
		{
			free(http_field);
			free(http_val);
			free_http(http);
			return (NULL);
		}
		free(http_field);
		free(http_val);
	}
    return http;
}

// TODO: Initialize server socket and serve incoming connections, using server_routine.
// HINT: Refer to the implementations in socket_util.c from the previous project.
int server_engine (int server_port)
{
    int server_listening_sock = -1;
    // TODO: Initialize server socket
	server_listening_sock = socket(AF_INET, SOCK_STREAM, 0);
    // TODO: Set socket options to reuse the port immediately after the connection is closed
	int	reuse = 1;
	setsockopt(server_listening_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    // TODO: Bind server socket to the given port
    struct sockaddr_in server_addr_info;
	server_addr_info.sin_family = AF_INET;
	server_addr_info.sin_port = htons(62123);
	server_addr_info.sin_addr.s_addr = INADDR_ANY;
	int	server_bind = bind(server_listening_sock, (struct sockaddr*)&server_addr_info, sizeof(server_addr_info));
	if (server_bind == -1)
	{
		close(server_listening_sock);
    	return 0;
	}
    // TODO: Listen for incoming connections
	int server_listen = listen(server_listening_sock, MAX_WAITING_CONNECTIONS);
	if (server_listen == -1)
	{
		close(server_listening_sock);
    	return 0;
	}
    // Serve incoming connections forever
    while (1)
    {
        struct sockaddr_in client_addr_info;
        socklen_t client_addr_info_len = sizeof(client_addr_info);
        int client_connected_sock = -1;

        // TODO: Accept incoming connections
		client_connected_sock = accept(server_listening_sock, (struct sockaddr*)&client_addr_info, &client_addr_info_len);
		char	client_ip[INET_ADDRSTRLEN];
		unsigned int	client_port = ntohs(client_addr_info.sin_port);
		inet_ntop(AF_INET, &(client_addr_info.sin_addr), client_ip, INET_ADDRSTRLEN);
		printf ("CLIENT %s:%u ", client_ip, client_port);
		GREEN_PRTF ("CONNECTED.\n");
        // Serve the client
        server_routine (client_connected_sock);
        
        // TODO: Close the connection with the client
		printf ("CLIENT %s:%u ", client_ip, client_port);
		GREEN_PRTF ("DISCONNECTED.\n\n");
		close(client_connected_sock);
    }

    // TODO: Close the server socket
	close(server_listening_sock);
    return 0;
}

// TODO: Implement server routine for HTTP/1.0.
//       Return -1 if error occurs, 0 otherwise.
// HINT: Your implementation will be MUCH EASIER if you use the functions & structs provided in http_util.c.
//       Please read the descriptions in http_functions.h and the function definitions in http_util.c,
//       and use the provided functions as much as possible!
//       Also, please read https://en.wikipedia.org/wiki/List_of_HTTP_header_fields to get a better understanding
//       on how the structure and protocol of HTTP messages are defined.
int server_routine (int client_sock)
{
    if (client_sock == -1)
        return -1;

    size_t bytes_received = 0;
    char *http_version = "HTTP/1.0"; // We will only support HTTP/1.0 in this project.
    char header_buffer[MAX_HTTP_MSG_HEADER_SIZE] = {0};
    int header_too_large_flag = 0;
    http_t *response = NULL, *request = NULL;

    // TODO: Receive the HEADER of the client http message.
    //       You have to consider the following cases:
    //       1. End of header delimiter is received (HINT: https://en.wikipedia.org/wiki/List_of_HTTP_header_fields)
    //       2. Error occurs on read() (i.e. read() returns -1)
    //       3. Client disconnects (i.e. read() returns 0)
    //       4. MAX_HTTP_MSG_HEADER_SIZE is reached (i.e. message is too long)
	// bytes_received = send(client_sock, header_buffer, MAX_HTTP_MSG_HEADER_SIZE, 0);
	bytes_received = read(client_sock, header_buffer, MAX_HTTP_MSG_HEADER_SIZE);
	if (bytes_received > MAX_HTTP_MSG_HEADER_SIZE)
		header_too_large_flag = -1;

    // while (1)
    // {
    //     // Remove this line and implement the logic described above.
    //     header_too_large_flag = 1;
    //     break;
    // }

    // Send different http response depending on the request.
    // Carefully follow the four cases and their TODOs described below.
    // HINT: Please refer to https://developer.mozilla.org/en-US/docs/Web/HTTP/Status for more information on HTTP status codes.
    //       We will be using code 200, 400, 401, 404, and 431 in this project.

    // Case 1: If the received header message is too large... 
    // TODO: Send 431 Request Header Fields Too Large. (IMPLEMENTED)
    // HINT: In most real-world web browsers, this error rarely occurs.
    //       However, we implemented this case to give you a HINT on how to use the included functions in http_util.c.

    if (header_too_large_flag)
    {
        // Create the response, with the appropriate status code and http version.
        // Refer to http_util.c for more details.
        response = init_http_with_arg (NULL, NULL, http_version, "431");
        if (response == NULL)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
            return -1;
        }
        // Add the appropriate fields to the header of the response.
        add_field_to_http (response, "Content-Type", "text/html");
        add_field_to_http (response, "Connection", "close");

        // Generate and add the body of the response.
        char body[] = "<html><body><h1>431 Request Header Fields Too Large</h1></body></html>";
        add_body_to_http (response, sizeof(body), body);
    }
    else
    {
        // We have successfully received the header of the client http message.
        // TODO: Parse the header of the client http message.
        // HINT: Implement and use parse_http_header() function to format the received http message into a struct.
        //       You are NOT REQUIRED to implement and use parse_http_header().
        //       However, if you do, you will be able to use the http struct and its member functions,
        //       which will make things MUCH EASIER for you. We highly recommend you to do so.

        request = parse_http_header (header_buffer); // TODO: Change this to your implementation.
		if (request == NULL)
		{
			ERROR_PRTF ("SERVER ERROR: Failed to receive HTTP request\n");
			return -1;
		}
		printf ("\tHTTP ");
		GREEN_PRTF ("REQUEST:\n");
		print_http_header (request);

        // We must behave differently depending on the type of the request.
        if (strncmp (request->method, "GET", 3) == 0)
        {
            // Case 2: GET request is received.
            // HINT: It is common to return index.html when the client requests a directory.

            // TODO: First check if the requested file needs authorization. If so, check if the client is authorized.
            // HINT: The client will send the ID and password in BASE64 encoding in the Authorization header field, 
            //       in the format of "Basic <ID:password>", where <ID:password> is encoded in BASE64.
            //       Refer to https://developer.mozilla.org/ko/docs/Web/HTTP/Authentication for more information.
            int auth_flag = 0;
            char *auth_list[] = {"/secret.html", "/public/images/khl.jpg"};
            char ans_plain[] = "DCN:FALL2023"; // ID:password (Please do not change this.)
			if (strstr(request->path, auth_list[0]) || strstr(request->path, auth_list[1]))
			{
				if (find_http_field_val(request, "Authorization") == NULL)
					auth_flag = 1;
				else
				{
					char	*input_auth = strchr(find_http_field_val(request, "Authorization"), ' ');
					if (input_auth != NULL)
						input_auth += 1;
					size_t	max_decode_len = strlen(input_auth);
					char	*encode_ans = base64_encode(ans_plain, max_decode_len);
					printf("%s\n", encode_ans);
					if (strncmp(input_auth, encode_ans, max_decode_len) != 0)
						auth_flag = 1;
				}
			}

			char *file_path = (char *)malloc(MAX_PATH_SIZE);
			file_path = strcpy(file_path, SERVER_ROOT);
			void *content = NULL;
			file_path = strcat(file_path, request->path);
			if (strcmp(request->path, "/") == 0)
				file_path = strcat(file_path, "index.html");
			ssize_t	body_size = read_file(&content, file_path);
            // Case 2-1: If authorization succeeded...
            // TODO: Get the file path from the request.
			if (auth_flag == 0)
			{
                // Case 2-1-1: If the file does not exist...
                // TODO: Send 404 Not Found.
				if (body_size < 0)
				{
					response = init_http_with_arg (NULL, NULL, http_version, "404");
					if (response == NULL)
					{
						ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
						free(file_path);
						free(content);
						return -1;
					}
					add_field_to_http (response, "Content-Type", "text/html");
					add_field_to_http (response, "Connection", "close");
        			char body[] = "<html><body><h1>404 Not Found</h1></body></html>";
					add_body_to_http (response, sizeof(body), body);
				}
                // Case 2-1-2: If the file exists...
                // TODO: Send 200 OK with the file as the body.
				else
				{
					response = init_http_with_arg (NULL, NULL, http_version, "200");
					if (response == NULL)
					{
						ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
						free(file_path);
						free(content);
						return -1;
					}
					char	*body_type = find_content_type(file_path, find_http_field_val(request, "Accept"));
					add_body_to_http (response, (size_t)body_size, content);
					add_field_to_http (response, "Connection", "close");
					add_field_to_http (response, "Content-Type", body_type);
				}
			}
            // Case 2-2: If authorization failed...
            // TODO: Send 401 Unauthorized with WWW-Authenticate field set to Basic.
            //       Refer to https://developer.mozilla.org/ko/docs/Web/HTTP/Authentication for more information.
			else
			{
				response = init_http_with_arg (NULL, NULL, http_version, "401");
				if (response == NULL)
				{
					ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
					free(file_path);
					free(content);
					return -1;
				}
				add_field_to_http (response, "Content-Type", "text/html");
				add_field_to_http (response, "Connection", "close");
        		char body[] = "<html><body><h1>401 Unauthorized</h1></body></html>";
				add_body_to_http (response, sizeof(body), body);
				add_field_to_http (response, "WWW-Authenticate", "Basic realm=\"ID & Password?\"");
			}
			free(file_path);
			free(content);
        }
        else if (strncmp (request->method, "POST", 4) == 0)
        {
            // Case 3: POST request is received.
            // TODO: Receive the body of the POST http message.
            // HINT: Use the Content-Length & boundary in Content-type field in the header to determine 
            //       the start & the size of the body.
            //       Also, there might be some parts of the body that were received along with the header...
            //       Refer to https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST for more information.

            // TODO: Parse each request_body of the multipart content request_body.			
			http_t	*request_body = NULL;
			// request_body = post_body(header_buffer, request);
			// CURL	*curl;
			// CURLcode	res;
			// char	body_buffer[MAX_HTTP_MSG_HEADER_SIZE];
			// curl_global_init(CURL_GLOBAL_ALL);
			// curl = curl_easy_init();
			char	*write_path = (char *)calloc(strlen(SERVER_ROOT) + strlen(ALBUM_PATH) + 1, sizeof(char));
			write_path = strcat(write_path, SERVER_ROOT);
			write_path = strcat(write_path, ALBUM_PATH);
			// if (curl)
			// {
			// 	curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:62123/");
			// 	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_bytes);
			// 	curl_easy_setopt(curl, CURLOPT_WRITEDATA, body_buffer);

			// 	res = curl_easy_perform(curl);
			// 	if (res != CURLE_OK) {
			// 		fprintf(stderr, "cURL failed: %s\n", curl_easy_strerror(res));
        	// 	}
			// }
			// char	*tmp = strstr(header_buffer, "Content-Disposition");
			// printf("%s\n", tmp);
			char	*boundary = strstr(find_http_field_val(request, "Content-Type"), "boundary=");
			if (boundary != NULL)
				boundary += strlen("boundary=");
			processMultipartFormData(boundary, header_buffer, sizeof(header_buffer));
			// printf("%s\n", boundary);
			// char	*body_data = strstr(header_buffer, boundary);
			// char	*tmp = strstr(header_buffer, "\r\n\r\n");
			// printf("%s\n", tmp);
			// char	*boundary_start = strstr(header_buffer, "boundary=");
			// if (boundary_start == NULL)
			// {
			// 	return (-1);
			// }
			// boundary_start += strlen("boundary=");
			// char	*boundary_end = strstr(boundary_start, "\r\n");
			// if (add_field_to_http(request_body, "Content-Disposition", "form-data; name=\"upload\"; filename=\"\\") == -1)
			// {
			// 	free_http(request_body);
			// 	return (-1);
			// }
			// if (add_field_to_http(request_body, "Content-Disposition", "form-data; name=\"upload\"; filename=\"\\") == -1)
			// {
			// 	free_http(request_body);
			// 	return (-1);
			// }
			// if (add_field_to_http(request_body, "Content-Disposition", "form-data; name=\"upload\"; filename=\"\\") == -1)
			// {
			// 	free_http(request_body);
			// 	return (-1);
			// }
            // TODO: Get the filename of the file.
			char	*file_content = "add what you found";
			char	*tmp_fname = strstr(find_http_field_val(request_body, "Content-Disposition"), "filename=");
			tmp_fname = strstr(tmp_fname, "\"");
            char	*filename = tmp_fname;
			char	*file_extention = get_file_extension(tmp_fname);
			printf ("\tHTTP ");
			GREEN_PRTF ("POST BODY:\n");
			print_http_header (request_body);
            // TODO: Check if the file is an image file.
			if (strncmp(file_extention, ".jpg", 4) != 0)
			{
				ERROR_PRTF ("SERVER ERROR: Invalid file type\n");
				free_http (request);
				free_http (request_body);
				return -1;
			}
            // TODO: Save the file in the album.
			// char	*write_path = (char *)calloc(MAX_PATH_SIZE, sizeof(char));
			// write_path = strcat(write_path, SERVER_ROOT);
			// write_path = strcat(write_path, ALBUM_PATH);
			ssize_t	file_size = write_file(write_path, file_content, sizeof(file_content));
			if (file_size == -1)
			{
            	ERROR_PRTF ("SERVER ERROR: Failed to write file to album_images.html\n");
				free_http (request);
				free_http (request_body);
				return -1;
			}
            // Append the appropriate html for the new image to album.html.
            size_t html_append_size = strlen (ALBUM_HTML_TEMPLATE) + strlen (filename)*2 + 1;
            char *html_append = (char *)calloc (1, html_append_size);
            sprintf (html_append, ALBUM_HTML_TEMPLATE, filename, filename);
            append_file (ALBUM_HTML_PATH , html_append, strlen (html_append));
            free (html_append);

            // TODO: Respond with a 200 OK.
			char *file_path = (char *)malloc(MAX_PATH_SIZE);
			file_path = strcpy(file_path, SERVER_ROOT);
			void *content = NULL;
			file_path = strcat(file_path, request->path);
			if (strcmp(request->path, "/") == 0)
				file_path = strcat(file_path, "index.html");
			ssize_t	body_size = read_file(&content, file_path);
			response = init_http_with_arg (NULL, NULL, http_version, "200");
			if (response == NULL)
			{
				ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
				free(file_path);
				free(content);
				return -1;
			}
			char	*body_type = find_content_type(file_path, find_http_field_val(request, "Accept"));
			add_body_to_http (response, (size_t)file_size, file_content);
			add_field_to_http (response, "Connection", "close");
			add_field_to_http (response, "Content-Type", body_type);
        }
        else
        {
            // Case 4: Other requests...
            // TODO: Send 400 Bad Request.
			response = init_http_with_arg (NULL, NULL, http_version, "400");
			if (response == NULL)
			{
				ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
				return -1;
			}
			add_field_to_http (response, "Content-Type", "text/html");
			add_field_to_http (response, "Connection", "close");
      			char body[] = "<html><body><h1>404 Bad Request</h1></body></html>";
			add_body_to_http (response, sizeof(body), body);
        }
    }

    // Send the response to the client.
    if (response != NULL)
    {
        printf ("\tHTTP ");
        GREEN_PRTF ("RESPONSE:\n");
        print_http_header (response);

        // Parse http response to buffer
        void *response_buffer = NULL;
        ssize_t response_size = write_http_to_buffer (response, &response_buffer);
        if (response_size == -1)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to write HTTP response to buffer\n");
            free_http (request);
            free_http (response);
            return 0;
        }

        // Send http response to client
        if (write_bytes (client_sock, response_buffer, response_size) == -1)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to send response to client\n");
            free (response_buffer);
            free_http (request);
            free_http (response);
            return 0;
        }

        free (response_buffer);
    }
    free_http (request);
    free_http (response);
    return 0;
}
