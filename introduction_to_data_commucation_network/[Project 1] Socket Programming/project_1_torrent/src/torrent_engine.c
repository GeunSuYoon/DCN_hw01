// NXC Data Communications Network torrent_engine.c for BitTorrent-like P2P File Sharing System
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


#include "torrent_engine.h"
#include "torrent_ui.h"
#include "torrent_utils.h"

//// TORRENT ENGINE FUNCTIONS ////

// The implementations for below functions are provided as a reference for the project.

// Request torrent info from a remote host. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->last_torrent_info_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_INFO 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);
    
    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// Push torrent INFO to peer. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE] [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_INFO 0x%08x %d 0x%08x %s %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, torrent->torrent_name, torrent->file_size);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }

    // Send block hashes.
    if (write_bytes (peer_sock, torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// Handle a request for torrent info, using push_torrent_info()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_info (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", (double)get_elapsed_msec()/1000,
        torrent->torrent_hash, peer->ip, peer->port);

    // If torrent info exists, Push torrent info to peer.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_info (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): push_torrent_info() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a push of torrent info.
// Returns 0 on success, -1 on error.
// Message protocol: 
// PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE] [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_info (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL || msg_body == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Parse torrent name.
    char *val = strtok (msg_body, " ");
    char torrent_name[STR_LEN] = {0};
    strncpy (torrent_name, val, STR_LEN - 1);

    // Parse file size.
    val = strtok (NULL, " ");
    size_t file_size = atoi (val);

    // Check for extra arguments.
    val = strtok (NULL, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid message.\n");
        close (peer_sock);
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED PUSH FOR 0x%08x INFO FROM %s:%d- NAME: %s, SIZE: %ld\n"
        , (double)get_elapsed_msec()/1000, torrent->torrent_hash, peer->ip, peer->port, torrent_name, file_size);

    // Check if torrent already has info.
    if (is_torrent_info_set (torrent) == 1)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): torrent already has info.\n");
        close (peer_sock);
        return -1;
    }

    // Update torrent info.
    if (set_torrent_info (torrent, torrent_name, file_size) < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): set_torrent_info() failed.\n");
        close (peer_sock);
        return -1;
    }

    // Read block hashes.
    if (read_bytes (peer_sock, torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    return 0;
}

//// TODO: IMPLEMENT THE FOLLOWING FUNCTIONS ////

// TODO: Complete the Client Routine.
int torrent_client (torrent_engine_t *engine)
{

    // Iterate through all torrents and take action.
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        torrent_t *torrent = engine->torrents[i];
        if (torrent == NULL)
        {
            ERROR_PRTF ("ERROR torrent_client(): invalid torrent.\n");
            return -1;
        }

        // If torrent info has been received...
        if (is_torrent_info_set (torrent) == 1)
        {
            // If TORRENT_SAVE_INTERVAL_MSEC has passed since last save, save the torrent.
            if (get_elapsed_msec() - torrent->last_torrent_save_msec > TORRENT_SAVE_INTERVAL_MSEC)
            {
                if (save_torrent_as_file (torrent) < 0)
                {
                    ERROR_PRTF ("ERROR torrent_client(): save_torrent_as_file() failed.\n");
                    return -1;
                }
            }

            // TODO: If RESET_BLOCK_STATUS_INTERVAL_MSEC has passed since last reset, reset blocks in
            //       REQUESTED state to MISSING state.
            if (get_elapsed_msec() - torrent->last_torrent_save_msec == TORRENT_SAVE_INTERVAL_MSEC)
			{
				for (size_t	block_idx = 0; block_idx < torrent->num_blocks; i++)
					if (torrent->block_status[block_idx] == B_REQUESTED)
						torrent->block_status[block_idx] = B_MISSING;
			}

        }

        // Iterate through peers.
        for (size_t peer_idx = 0; peer_idx < torrent->num_peers; peer_idx++)
        {
            peer_data_t *peer = torrent->peers[peer_idx];
            // TODO: If torrent info has NOT been received, and REQUEST_TORRENT_INFO_INTERVAL_MSEC 
            //       has passed since last request, request the torrent info from the peer.
            // HINT: Make sure to use request_torrent_info_thread() instead of request_torrent_info().
			if (request_torrent_info_thread(peer, torrent) == -1)
			{
				if (get_elapsed_msec() - torrent->last_torrent_save_msec == REQUEST_TORRENT_INFO_INTERVAL_MSEC)
				{
					int ret_int = request_torrent_info_thread(peer, torrent);
					while (ret_int < 0)
						ret_int = request_torrent_info_thread(peer, torrent);
				}
			}
            // TODO: If REQUEST_PEER_LIST_INTERVAL_MSEC has passed since last request, request peer list.
            // HINT: Make sure to use request_torrent_peer_list_thread() instead of request_torrent_peer_list().
			if (get_elapsed_msec() - torrent->last_torrent_save_msec == REQUEST_PEER_LIST_INTERVAL_MSEC)
			{
				int ret_int = request_torrent_peer_list_thread(peer, torrent);
				if (ret_int < 0)
				{
					ERROR_PRTF ("ERROR torrent_client(): invalid request peer list.\n");
					return -1;
				}
				return (ret_int);
			}
            // TODO: If REQUEST_BLOCK_STATUS_INTERVAL_MSEC has passed since last request, request block status.
            // HINT: Make sure to use request_torrent_block_status_thread() instead of request_torrent_block_status().
			if (get_elapsed_msec() - torrent->last_torrent_save_msec == REQUEST_BLOCK_STATUS_INTERVAL_MSEC)
			{
				int ret_int = request_torrent_block_status_thread(peer, torrent);
				if (ret_int < 0)
				{
					ERROR_PRTF ("ERROR torrent_client(): invalid request block status.\n");
					return -1;
				}
			}
            // TODO: If REQUEST_BLOCK_INTERVAL_MSEC has passed since last request, request a block.
            // HINT: Make sure to use request_torrent_block_thread() instead of request_torrent_block().
            //       Use get_rand_missing_block_that_peer_has () function to randomly select a block to request.
			if (get_elapsed_msec() - torrent->last_torrent_save_msec == REQUEST_BLOCK_INTERVAL_MSEC)
			{
				// int	block_idx = find_peer
				int block_idx = get_rand_missing_block_that_peer_has(torrent, peer);
				int ret_int = request_torrent_block_thread(peer, torrent, block_idx);
				if (ret_int < 0)
				{
					ERROR_PRTF ("ERROR torrent_client(): invalid request block.\n");
					return -1;
				}
			}
        }
    }

    return 0;
}

// TODO: Complete the server Routine.

int torrent_server (torrent_engine_t *engine)
{
    // TODO: Accept incoming connections.
	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);
	int	peer_sock;
	peer_sock = accept_socket(engine->listen_sock, &peer_addr, &peer_addr_len);
	if (peer_sock < 0) {
		ERROR_PRTF ("ERROR torrent_server(): fail to accept server socket.\n");
		close(peer_sock);
		return -1;
	}
	// struct sockaddr_in	listen_addr;
	// memset(&listen_addr, 0, sizeof(listen_addr));
    // listen_addr.sin_family = AF_INET;
    // listen_addr.sin_addr.s_addr = INADDR_ANY;
    // listen_addr.sin_port = htons(port);
	// if (bind(listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1) {
	// 	ERROR_PRTF ("ERROR listen_socket(): fail to bind listening socket.\n");
    //     close(listen_sock);
	// 	return -1;
    // }
	
    // TODO: Get peer ip and port.
	char	peer_addr_int[INET_ADDRSTRLEN];
	int	peer_port = peer_addr.sin_port;
	inet_ntop(AF_INET, &(peer_addr.sin_addr), peer_addr_int, INET_ADDRSTRLEN);

    // TODO: Read message.
	char	msg[MSG_LEN] = {0};
	ssize_t msg_bytes = recv(peer_sock, msg, sizeof(msg), 0);
	if (msg_bytes < -1)
	{
		return (-1);
	}
    // TODO: Parse command from message.
    char	*msg_com = strtok(msg, " ");
    // TODO: Parse peer engine hash from message.
    //       If the peer engine hash is the same as the local engine hash, ignore the message.
	char	*msg_eng_hash = strtok(NULL, " ");
    // TODO: Parse peer listen port from message.
	char	*msg_lis_port = strtok(NULL, " ");
	int		msg_lis_port_int = atoi(msg_lis_port);
    // TODO: Parse peer torrent hash from message.
	char	*msg_tor_hash = strtok(NULL, " ");
	HASH_t	msg_tor_hash_uint = str_to_hash(msg_tor_hash);
    // TODO: Add peer to torrent if it doesn't exist.
	ssize_t	target_torrent = find_torrent(engine, msg_tor_hash_uint);
	ssize_t	target_torrent_peer = find_peer(engine->torrents[target_torrent], peer_addr_int, peer_port);
	if (target_torrent == -1)
	{
		if (add_torrent(engine, msg_tor_hash_uint) == -1)
		{
			return (-1);
		}
	}
	else if (target_torrent_peer == -1)
	{
		if ((target_torrent_peer = add_peer(engine, msg_tor_hash_uint, peer_addr_int, peer_port)) == -1)
		{
			return (-1);
		}
	}
    // TODO: Call different handler function based on message command.
    // HINT: The handler functions take the engine, peer_sock, peer, torrent, and msg_body as arguments.
    //       The engine, peer, torrent arguments refers to the torrent engine, 
    //       peer that sent the message, and the torrent the message is about.
    //       The peer_sock argument refers to the socket that the message was received from. (Return value of accept_socket ())
    //       The msg_body argument refers to the part of the message after [TORRENT_HASH], excluding the space after [TORRENT_HASH], if it exists.
	if (strcmp(msg_com, "REQUEST_TORRENT_INFO") == 0)
	{
		if (handle_request_torrent_info(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
	if (strcmp(msg_com, "PUSH_TORRENT_INFO") == 0)
	{
		if (handle_push_torrent_info(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
	if (strcmp(msg_com, "REQUEST_TORRENT_PEER_LIST") == 0)
	{
		if (handle_request_torrent_peer_list(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
	if (strcmp(msg_com, "PUSH_TORRENT_PEER_LIST") == 0)
	{
		if (handle_push_torrent_peer_list(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
	if (strcmp(msg_com, "REQUEST_TORRENT_BLOCK_STATUS") == 0)
	{
		if (handle_request_torrent_block_status(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
	if (strcmp(msg_com, "PUSH_TORRENT_BLOCK_STATUS") == 0)
	{
		if (handle_push_torrent_block_status(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
	if (strcmp(msg_com, "REQUEST_TORRENT_BLOCK") == 0)
	{
		if (handle_request_torrent_block(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
	if (strcmp(msg_com, "PUSH_TORRENT_BLOCK") == 0)
	{
		if (handle_push_torrent_block(engine, peer_sock, 
			engine->torrents[target_torrent]->peers[target_torrent_peer], engine->torrents[target_torrent], msg))
		{
			ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
			close(peer_sock);
			return (-2);
		}
	}
    return 0;
}

// TODO: Open a socket and listen for incoming connections. 
//       Returns the socket file descriptor on success, -1 on error.
int listen_socket (int port)
{
	int	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in	listen_addr;
	memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);

	if (listen_sock < 0)
	{
		ERROR_PRTF ("ERROR listen_socket(): fail to creat the listen socket.\n");
		return (-1);
	}
	if (bind(listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1) {
		ERROR_PRTF ("ERROR listen_socket(): fail to bind listening socket.\n");
        close(listen_sock);
		return -1;
    }
	if (listen(listen_sock, MAX_QUEUED_CONNECTIONS) == -1)
	{
		ERROR_PRTF ("ERROR listen_socket(): fail to listen the listen socket.\n");
		close(listen_sock);
		return (-1);
	}
    return listen_sock;
}

// TODO: Accept an incoming connection with a timeout. 
//       Return the connected socket file descriptor on success, -2 on error, -1 on timeout.
//       MUST use a non-blocking socket with a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll() for timeout. kbhit() in torrent_util.c for an example on using poll().
int accept_socket(int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen)
{
	int	accept_sock = accept(listen_sock, (struct sockaddr *)&cli_addr, clilen);
	if (accept_sock == -1)
	{
		ERROR_PRTF ("ERROR accept_socket(): fail to creat the accept socket.\n");
		return (-2);
	}
	if (kbhit() > 0)
	{
		ERROR_PRTF ("ERROR accept_socket(): timeout.\n");
		close(accept_sock);
		return (-1);
	}
    return accept_sock;
}

// TODO: Connect to a server with a timeout. 
//       Return the socket file descriptor on success, -2 on error, -1 on timeout.
//       MUST use a non-blocking socket with a timeout of TIMEOUT_MSEC msec.
// HINT: Use fcntl() to set the socket as non-blocking. 
//       Use poll() for timeout. See kbhit() in torrent_utils.c for an example on using poll().
int connect_socket(char *server_ip, int port)
{
	int	connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect_sock < 0)
	{
		ERROR_PRTF ("ERROR connect_socket(): fail to creat the connect socket.\n");
		return (-1);
	}
	struct sockaddr_in connect_add;
	connect_add.sin_family = AF_INET;
	connect_add.sin_port = htons(port);
	connect_add.sin_addr.s_addr = inet_addr(server_ip);
	socklen_t connect_add_len = sizeof(connect_add);
	if (fcntl(connect_sock, F_SETFL, O_NONBLOCK) != 0)
	{
		ERROR_PRTF ("ERROR connect_socket(): fail to non-blocking the connect socket.\n");
		close(connect_sock);
		return (-2);
	}
	if (connect(connect_sock, (struct sockaddr *)&connect_add, connect_add_len) < 0)
	{
		ERROR_PRTF ("ERROR connect_socket(): fail to connect the connect socket.\n");
		close(connect_sock);
		return (-2);
	}
	if (kbhit() > 0)
	{
		ERROR_PRTF ("ERROR connect_socket(): timeout.\n");
		return (-1);
	}
    return connect_sock;
}

// TODO: Request peer's peer list from a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: 1.Update peer info. 2. Format message. 3. Connect to peer. 4. Send message.
//       Follow the example in request_torrent_info().
int request_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->last_torrent_info_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_PEER_LIST 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// TODO: Request peer's block info from a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: 1.Update peer info. 2. Format message. 3. Connect to peer. 4. Send message.
//       Follow the example in request_torrent_info().
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
	if (peer == NULL || torrent == NULL)
	{
        ERROR_PRTF ("ERROR request_torrent_block_status(): invalid argument.\n");
		return (-2);
	}
    peer->last_torrent_info_request_msec = get_elapsed_msec();

	char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_BLOCK_STATUS 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);
	
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// TODO: Request a block from a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
// HINT: 1.Update peer info. 2. Format message. 3. Connect to peer. 4. Send message. 5. Set block status.
//       Follow the example in request_torrent_info().
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
	if (peer == NULL || torrent == NULL)
	{
        ERROR_PRTF ("ERROR request_torrent_block(): invalid argument.\n");
		return (-2);
	}
    peer->last_torrent_info_request_msec = get_elapsed_msec();

	char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_BLOCK 0x%08x %d 0x%08x %zu", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, block_index);
	
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_block(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_block(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
	torrent->block_status[block_index] = B_REQUESTED;

    close (peer_sock);

    return 0;
}

// TODO: Push my list of peers to a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: 
//       PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEER_1_IP]:[PEER_1_PORT] ...
//       [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// HINT: 1. Format message. 2. Connect to peer. 3. Send message. 4. Send peer list.
//       [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
//       The peer list is  [NUM_PEERS] * PEER_LIST_MAX_BYTE_PER_PEER bytes long, including the space.
//       Make sure NOT to send the IP and port of the receiving peer back to itself.
//       Follow the example in push_torrent_info().
int push_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
	char	msg_peer_list[PEER_LIST_MAX_BYTE_PER_PEER * DEFAULT_ARR_MAX_NUM] = {0};
    sprintf (msg, "PUSH_TORRENT_INFO 0x%08x %d 0x%08x %zu", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, torrent->num_peers);
	for (size_t i = 0; i < torrent->num_peers; i++)
	    sprintf (msg_peer_list, "%s:%d ", torrent->peers[i]->ip, torrent->peers[i]->port);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }
    if (write_bytes (peer_sock, msg_peer_list, PEER_LIST_MAX_BYTE_PER_PEER * DEFAULT_ARR_MAX_NUM) != PEER_LIST_MAX_BYTE_PER_PEER * DEFAULT_ARR_MAX_NUM)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }

    close (peer_sock);

    return 0;
}

// TODO: Push a torrent block status to a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: 
//       PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
//       [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
// HINT: 1. Format message. 2. Connect to peer. 3. Send message. 4. Send block status.
//       Follow the example in push_torrent_info().
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
	char msg_stat[sizeof(B_STAT)] = {0};
    sprintf (msg, "PUSH_TORRENT_BLOCK_STATUS 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, peer->torrent->torrent_hash);
    sprintf (msg_stat, "%d", *(torrent->block_status));

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }
    if (write_bytes (peer_sock, msg_stat, sizeof(B_STAT)) != sizeof(B_STAT))
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }

    return 0;
} 

// TODO: Push a block to a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol:
//       PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
//       [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
// HINT: 1. Format message. 2. Connect to peer. 3. Send message. 4. Send block data.
//       Follow the example in push_torrent_info().
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
	if (peer == NULL || torrent == NULL)
	{
        ERROR_PRTF ("ERROR push_torrent_block(): invalid argument.\n");
		return (-2);
	}
    peer->last_torrent_info_request_msec = get_elapsed_msec();

	char msg[MSG_LEN] = {0};
	char msg_stat[BLOCK_SIZE] = {0};
    sprintf (msg, "PUSH_TORRENT_BLOCK 0x%08x %d 0x%08x %zu", torrent->engine->engine_hash,
        torrent->engine->port, peer->torrent->torrent_hash, block_index);
    sprintf (msg_stat, "%p", get_block_ptr(torrent, block_index));
	
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    if (write_bytes (peer_sock, msg_stat, BLOCK_SIZE) != BLOCK_SIZE)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);
	
    return 0;
}

// TODO: Handle a request for a peer list, using push_torrent_peer_list()
//       Returns 0 on success, -1 on error.
//       Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: Follow the example in handle_request_torrent_info(). 
//       Use strtok(). Check if torrent info has been received.
int handle_request_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", (double)get_elapsed_msec()/1000,
        torrent->torrent_hash, peer->ip, peer->port);

    // If torrent info exists, Push torrent info to peer.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_peer_list (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): push_torrent_info() failed.\n");
        return -1;
    }

    return 0;
}

// TODO: Handle a request for a block info, using push_torrent_block_status()
//       Returns 0 on success, -1 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: Follow the example in handle_request_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_request_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", (double)get_elapsed_msec()/1000,
        torrent->torrent_hash, peer->ip, peer->port);

    // If torrent info exists, Push torrent info to peer.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_block_status (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): push_torrent_info() failed.\n");
        return -1;
    }

    return 0;
}

// TODO: Handle a request for a block, using push_torrent_block()
//       Returns 0 on success, -1 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
// HINT: Follow the example in handle_request_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_request_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", (double)get_elapsed_msec()/1000,
        torrent->torrent_hash, peer->ip, peer->port);
	char *block_idx = strtok(msg_body, " ");
	block_idx = strtok(msg_body, " ");
	block_idx = strtok(msg_body, " ");
	block_idx = strtok(msg_body, " ");
	block_idx = strtok(msg_body, " ");
    // If torrent info exists, Push torrent info to peer.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_block (peer, torrent, atoi(block_idx)) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): push_torrent_info() failed.\n");
        return -1;
    }

    return 0;
}

// TODO: Handle a push of a peer list.
//       Returns 0 on success, -1 on error.
//       PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
//       [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// HINT: Follow the example in handle_push_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_push_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", (double)get_elapsed_msec()/1000,
        torrent->torrent_hash, peer->ip, peer->port);

    // If torrent info exists, Push torrent info to peer.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_peer_list(peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): push_torrent_info() failed.\n");
        return -1;
    }

    return 0;
}

// TODO: Handle a push of a peer block status.
//       Returns 0 on success, -1 on error.
//       PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
//       [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
// HINT: Follow the example in handle_push_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_push_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL || msg_body == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Parse torrent name.
    char *val = strtok (msg_body, " ");
    char torrent_name[STR_LEN] = {0};
    strncpy (torrent_name, val, STR_LEN - 1);

    // Parse file size.
    val = strtok (NULL, " ");
    size_t file_size = atoi (val);

    // Check for extra arguments.
    val = strtok (NULL, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): invalid message.\n");
        close (peer_sock);
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED PUSH FOR 0x%08x INFO FROM %s:%d- NAME: %s, SIZE: %ld\n"
        , (double)get_elapsed_msec()/1000, torrent->torrent_hash, peer->ip, peer->port, torrent_name, file_size);

    // Check if torrent already has info.
    if (is_torrent_info_set (torrent) == 1)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): torrent already has info.\n");
        close (peer_sock);
        return -1;
    }

    // Update torrent info.
    if (set_torrent_info (torrent, torrent_name, file_size) < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): set_torrent_info() failed.\n");
        close (peer_sock);
        return -1;
    }

    // Read block hashes.
    if (read_bytes (peer_sock, torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    return 0;
}

// TODO: Handle a push of a block.
//       Check hash of the received block. If it does NOT match the expected hash, drop the received block.
//       Returns 0 on success, -1 on error.
//       PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
//       [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
// HINT: Follow the example in handle_push_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_push_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}
