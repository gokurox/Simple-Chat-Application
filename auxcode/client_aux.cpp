#include "../header/sysheads.h"
#include "../header/common_head.h"
#include "../header/client_head.h"

using namespace std;

// Local Prototypes
void log_out_user (fd_t &);
void request_who (fd_t &);
void msg_user (fd_t &, const vector_string_t);
void create_group (fd_t &, const vector_string_t);
void join_group (fd_t &, const vector_string_t);
void msg_group (fd_t &, const vector_string_t);
void send_file (fd_t &, const vector_string_t);
void recv_file (fd_t &, const vector_string_t);

void connect_to_signup_server (fd_t &signup_fd)
{
	sockaddr_in signup_sock;
	int rc;

	signup_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert (signup_fd >= 0);

	bzero ((char *) &signup_sock, sizeof (signup_sock));
	signup_sock.sin_family = AF_INET;
	signup_sock.sin_port = htons (SIGNUP_PORT_NUM);
	rc = inet_pton (AF_INET, SERVER_IPv4_ADDR.c_str(), &(signup_sock.sin_addr));
	assert (rc == 1);

	rc = connect (signup_fd, (sockaddr *) &signup_sock, sizeof (signup_sock));
	assert (rc == 0);
	cout << "INFO " << "@connect_to_signup_server: " << "Connected to server." << endl;

	rc = recv_ACK (signup_fd);
	assert (rc != 0);
}

void connect_to_signin_server (fd_t &signin_fd)
{
	sockaddr_in signin_sock;
	int rc;

	signin_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert (signin_fd >= 0);

	bzero ((char *) &signin_sock, sizeof (signin_sock));
	signin_sock.sin_family = AF_INET;
	signin_sock.sin_port = htons (SIGNIN_PORT_NUM);
	rc = inet_pton (AF_INET, SERVER_IPv4_ADDR.c_str(), &(signin_sock.sin_addr));
	assert (rc == 1);

	rc = connect (signin_fd, (sockaddr *) &signin_sock, sizeof (signin_sock));
	assert (rc == 0);
	cout << "INFO " << "@connect_to_signin_server: " << "Connected to server." << endl;

	rc = recv_ACK (signin_fd);
	assert (rc != 0);
}

void process_signup_request (fd_t &fd, const vector_string_t client_signup_request)
{
	string_t username = client_signup_request[1];
	string_t password = client_signup_request[2];

	send_composed_message (fd, const_enums::CMD, const_enums::REG, username, password);
	recv_ACK (fd);

	close_fd (fd);
}

void process_signin_request (fd_t &fd, const vector_string_t client_signin_request)
{
	string_t username = client_signin_request[1];
	string_t password = client_signin_request[2];

	send_composed_message (fd, const_enums::CMD, const_enums::LOG_IN, username, password);
	int rc = recv_ACK (fd);
	assert (rc != 0);

	if (rc == 2) {
		cout << "WARNING " << "@process_signin_request: " << "Could not Sign In." << endl;
		return;
	}

	CLIENT_USERNAME = username;

	fd_set sockfd_set;
	fd_t max_fd;

	char client_input[1025];
	bool break_loop = false;

	cin.get();		// Pause
	system ("clear");

	do {
		if (fd < 0)
			break;

		show_prompt ();

		// Clear the Socket Set
		FD_ZERO (&sockfd_set);

		FD_SET (fileno (stdin), &sockfd_set);
		FD_SET (fd, &sockfd_set);

		max_fd = max (fd, fileno (stdin));

		select (max_fd +1, &sockfd_set, NULL, NULL, NULL);

		if (FD_ISSET (fd, &sockfd_set))
		{
			vector_string_t server_request;
			recv_composed_message (fd, server_request);

			switch (const_maps::MSG_TYPE_MAP [server_request[1]])
			{
				case const_enums::CMD:
				{
					switch (const_maps::CMD_TYPE_MAP [server_request[2]])
					{
						case const_enums::MSG:
						{
							cout << endl << endl;
							cout << "MESSAGE RECEIVED:" << endl;
							cout << "FROM: " << server_request[3] << endl;
							cout << "MESSAGE:" << endl;
							cout << server_request[5] << endl;
							cout << endl;
							break;
						}
						case const_enums::SEND_FILE:
						{
							cout << endl << endl;
							cout << "FILE SEND REQUEST RECEIVED:" << endl;
							cout << "FROM: " << server_request[3] << endl;
							cout << "The file will be saved at the server only for a short time. Please send RECV request soon" << endl;
							cout << endl;
							break;
						}
						default:
							break;
					}
					break;
				}
				case const_enums::ACK:
					break;
				default:
					break;
			}
		}

		if (FD_ISSET (fileno (stdin), &sockfd_set))
		{
			bzero (client_input, 1025);
			cin.getline (client_input, 1024);
			
			vector_string_t client_request;
			if (string_t (client_input).empty() == false) {
				split_message (client_input, " ", client_request);
			}
			else
			{
				cout << "Invalid command! Please type \"/help\" at the prompt to view valid commands." << endl;
				continue;
			}

			switch (client_const::CLIENT_INPUT_COMMAND_MAP[client_request[0]])
			{
				case client_const::clnt_register:
				case client_const::clnt_login:
				{
					cout << "INFO " << "@process_signin_request: " << "Logout to use this command." << endl;
					break;
				}
				case client_const::clnt_logout:
				{
					log_out_user (fd);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_who:
				{
					request_who (fd);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_msg:
				{
					msg_user (fd, client_request);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_create_grp:
				{
					create_group (fd, client_request);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_join_grp:
				{
					join_group (fd, client_request);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_msg_grp:
				{
					msg_group (fd, client_request);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_send:
				{
					send_file (fd, client_request);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_recv:
				{
					recv_file (fd, client_request);
					if (fd < 0)
						break_loop = true;
					break;
				}
				case client_const::clnt_help:
				{	
					help ();
					break;
				}
				case client_const::clnt_exit:
				{
					cout << "INFO " << "@process_signin_request: " << "Logout first and then use '/exit'." << endl;
					break;
				}
				default:
				{
					cout << "WARNING " << "@process_signin_request: " << "Invalid command." << endl;
					break;
				}
			}
		}
	} while (break_loop == false);
}

void show_prompt (bool endl_before) {
	string_t prompt = "chatportal";

	if (CLIENT_USERNAME.empty()) {
		prompt += "::>> ";
	}
	else {
		prompt += ":" + CLIENT_USERNAME + ":>> ";
	}

	cout.flush();

	if (endl_before) cout << endl;
	cout << prompt;
	
	cout.flush();
}

void help () {
	cout << "Use the following commands to interact with the chat portal:" << endl;
	cout << "/register <uname> <pword>" << endl;
	cout << "/login <uname> <pword>" << endl;
	cout << "/logout" << endl;
	cout << "/who" << endl;
	cout << "/msg <to_uname>" << endl;
	cout << "/create_grp <group_name>" << endl;
	cout << "/join_grp <group_name>" << endl;
	cout << "/msg_grp <group_name>" << endl;
	cout << "/send <to_uname> <path_to_file>" << endl;
	cout << "/recv <from_uname> <target_directory \"*/\">" << endl;
	cout << "/exit" << endl;
}

// Local methods

void log_out_user (fd_t &fd)
{
	send_composed_message (fd, const_enums::CMD, const_enums::LOG_OUT);
	int rc = recv_ACK (fd);
	assert (rc != 0);

	if (rc == 1) {
		close_fd (fd);

		CLIENT_USERNAME.clear();
		CLIENT_PASSWORD.clear();
	}
}

void request_who (fd_t &fd)
{
	send_composed_message (fd, const_enums::CMD, const_enums::WHO);

	vector_string_t server_response;
	recv_composed_message (fd, server_response);

	if (is_negative_ACK (server_response)) {
		cout << "Ack Type: NEGATIVE" << endl;
		cout << "Message : " << server_response[3] << endl;
		return;
	}

	if (is_positive_ACK (server_response)) {
		cout << "Ack Type: POSITIVE" << endl;
	}

	string_t online_users_string = server_response[3];
	string_t substring;
	string_t delim = "|";
	vector_string_t online_users;
	int spos, epos;

	spos = 0;
	epos = 0;
	single_split (online_users_string, delim, spos, epos, substring);

	while (epos != string_t::npos)
	{
		single_split (online_users_string, delim, spos, epos, substring);

		if (epos != string_t::npos)
			skip_read_by_length (online_users_string, spos, epos, substring);
		else
			break;

		if (substring.empty() == false)
			online_users.push_back (substring);
	}

	assert (online_users.size() != 0);
	
	int ctr = 0;
	cout << endl << "CURRENTLY ONLINE USERS ARE:" << endl;
	for (string_t user: online_users) {
		cout << ++ctr << ". " << user << endl;
	}
	cout << endl;
}

void msg_user (fd_t &fd, const vector_string_t client_request)
{
	string_t to_uname = client_request[1];
	char input_message[2049];

	bzero (input_message, 2049);
	cout << "Enter Message: ";
	cin.getline (input_message, 2048);

	string_t message;
	message.assign (input_message);

	send_composed_message (fd, const_enums::CMD, const_enums::MSG, CLIENT_USERNAME, to_uname, message);
	int rc = recv_ACK (fd);
	assert (rc != 0);
}

void create_group (fd_t &fd, const vector_string_t client_request)
{
	string_t group_name = client_request[1];

	send_composed_message (fd, const_enums::CMD, const_enums::CREATE_GROUP, group_name);
	int rc = recv_ACK (fd);
	assert (rc != 0);
}

void join_group (fd_t &fd, const vector_string_t client_request)
{
	string_t group_name = client_request[1];

	send_composed_message (fd, const_enums::CMD, const_enums::JOIN_GROUP, group_name);
	int rc = recv_ACK (fd);
	assert (rc != 0);
}

void msg_group (fd_t &fd, const vector_string_t client_request)
{
	string_t group_name = client_request[1];
	char input_message[2049];

	cout << "Enter Message: ";
	cin.getline (input_message, 2048);

	send_composed_message (fd, const_enums::CMD, const_enums::MSG_GROUP, group_name, string_t (input_message));
	int rc = recv_ACK (fd);
	assert (rc != 0);
}

void send_file (fd_t &fd, const vector_string_t client_request)
{
	string_t to_uname = client_request[1];
	string_t filepath = client_request[2];

	if (CLIENT_USERNAME.compare (to_uname) == 0) {
		cout << "WARNING " << "@send_file: " << "Cannot Send File To Yourself." << endl;
		return;
	}

	struct stat file_info;
	if (stat (filepath.c_str(), &(file_info)) < 0) {
		cout << "WARNING " << "@send_file: " << "Error Reading The File." << endl;
		return;
	}

	if (file_info.st_size > MAX_FILE_SIZE_B) {
		cout << "WARNING" << "@send_file: " << "File Too Big To Send." << endl;
		return;
	}

	string_t file_to_send;
	read_file (filepath, file_to_send);
	
	vector_string_t temp;
	split_message (filepath, "/", temp);
	
	string_t filename = temp.back();

	send_composed_message (fd, const_enums::CMD, const_enums::SEND_FILE, CLIENT_USERNAME, to_uname);

	int rc = recv_ACK (fd);
	assert (rc != 0);

	if (rc == 2)
		return;

	send_composed_message (fd, const_enums::CMD, const_enums::FILE, filename, file_to_send);

	rc = recv_ACK (fd);
	assert (rc != 0);
}

void recv_file (fd_t &fd, const vector_string_t client_request)
{
	string_t from_uname = client_request[1];
	string_t fileDIR = client_request[2];

	if (CLIENT_USERNAME.compare (from_uname) == 0) {
		cout << "WARNING " << "@recv_file: " << "Cannot Recv File From Yourself." << endl;
		return;
	}

	send_composed_message (fd, const_enums::CMD, const_enums::RECV_FILE, from_uname, CLIENT_USERNAME);

	int rc = recv_ACK (fd);
	assert (rc != 0);

	if (rc == 2)
		return;

	vector_string_t server_response;
	recv_composed_message (fd, server_response);

	if (is_negative_ACK (server_response)) {
		cout << "INFO " << "@recv_file: " << "File not received." << endl;
		return;
	}

	string_t filename = server_response[3];
	string_t file_to_save = server_response[4];
	overwrite_file (fileDIR + filename, file_to_save);

	cout << "INFO " << "@recv_file: " << "File Received." << endl;
}
