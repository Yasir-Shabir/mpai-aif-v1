/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * Based on the official sample by Intel at https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/net/sockets/coap_client
 *
 SPDX-License-Identifier: Apache-2.0
 */

#include <coap_connect.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <kernel.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(COAP_CONNECT, LOG_LEVEL_INF);

#include <net_private.h>

/* CoAP socket fd */
struct pollfd fds[1];
int nfds;
int coap_sock;

struct coap_block_context blk_ctx;

/*** PRIVATE ***/
void extract_data_result(struct coap_packet packet, uint8_t* data_result, bool add_termination);

/*** PUBLIC ***/
int get_coap_sock(void)
{
	return coap_sock;
}

struct coap_block_context get_block_context(void)
{
	return blk_ctx;
}

struct coap_block_context* get_block_context_ptr(void)
{
	return &blk_ctx;
}

void wait(void)
{
	if (poll(fds, nfds, -1) < 0) {
		LOG_ERR("Error in poll:%d", errno);
	}
}

void prepare_fds(void)
{
	fds[nfds].fd = coap_sock;
	fds[nfds].events = POLLIN;
	nfds++;
}

int start_coap_client(void)
{
	int ret = 0;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PEER_PORT);

	inet_pton(AF_INET, IP_ADDRESS_COAP_SERVER,
		  &addr.sin_addr);

	coap_sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
	if (coap_sock < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	ret = connect(coap_sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Cannot connect to UDP remote : %d", errno);
		return -errno;
	}

	prepare_fds();

	return 0;
}

int process_simple_coap_reply(uint8_t * data_result)
{
	struct coap_packet reply = {};
	uint8_t *data;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(coap_sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Raw response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	extract_data_result(reply, data_result, true);
	printk("Response %s\n", data_result);

	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

end:
	k_free(data);

	return ret;
}

int process_large_coap_reply(uint8_t * data_result)
{
	struct coap_packet reply = {};
	uint8_t *data;
	bool last_block;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(coap_sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	// net_hexdump("Raw response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	ret = coap_update_from_block(&reply, &blk_ctx);
	if (ret < 0) {
		goto end;
	}

	extract_data_result(reply, data_result, false);
	printk("Response block %zd: %s\n", get_block_context().current / 64, data_result);

	last_block = coap_next_block(&reply, &blk_ctx);
	if (!last_block) {
		ret = 1;
		goto end;
	}

	ret = 0;

end:
	k_free(data);

	return ret;
}

int process_obs_coap_reply(void)
{
	struct coap_packet reply;
	uint16_t id;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint8_t type;
	uint8_t tkl;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(coap_sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	// net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	tkl = coap_header_get_token(&reply, token);
	id = coap_header_get_id(&reply);

	type = coap_header_get_type(&reply);
	if (type == COAP_TYPE_ACK) {
		ret = 0;
	} else if (type == COAP_TYPE_CON) {
		ret = send_obs_reply_ack(id, token, tkl);
	}
end:
	k_free(data);

	return ret;
}

int send_simple_coap_request(uint8_t method, char ** simple_path)
{
	uint8_t payload[] = "payload";
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     method, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = simple_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	switch (method) {
	case COAP_METHOD_GET:
	case COAP_METHOD_DELETE:
		break;

	case COAP_METHOD_PUT:
	case COAP_METHOD_POST:
		r = coap_packet_append_payload_marker(&request);
		if (r < 0) {
			LOG_ERR("Unable to append payload marker");
			goto end;
		}

		r = coap_packet_append_payload(&request, (uint8_t *)payload,
					       sizeof(payload) - 1);
		if (r < 0) {
			LOG_ERR("Not able to append payload");
			goto end;
		}

		break;
	default:
		r = -EINVAL;
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(get_coap_sock(), request.data, request.offset, 0);

end:
	k_free(data);

	return 0;
}

static int send_large_coap_request(char ** large_path)
{
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	if (get_block_context().total_size == 0) {
		coap_block_transfer_init(get_block_context_ptr(), COAP_BLOCK_64,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = large_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	r = coap_append_block2_option(&request, get_block_context_ptr());
	if (r < 0) {
		LOG_ERR("Unable to add block2 option.");
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(get_coap_sock(), request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

char* get_large_coap_msgs(char ** large_path)
{
	int r;
	uint8_t data_single_result[MAX_COAP_MSG_LEN];
	char * data_large_result_tmp = NULL;
	while (1) {
		/* Test CoAP Large GET method */
		printk("\nCoAP client Large GET (block %zd)\n",
		       get_block_context().current / 64 /*COAP_BLOCK_64*/);
		r = send_large_coap_request(large_path);
		if (r < 0) {
			return NULL;
		}
		
		memset(data_single_result, 0, MAX_COAP_MSG_LEN);
		r = process_large_coap_reply(data_single_result);
		if (r < 0) {
			return NULL;
		}

		if (data_large_result_tmp != NULL) 
		{
			char * data_large_result_concat = append_strings(data_large_result_tmp, (char*)data_single_result);
			k_free(data_large_result_tmp);
			data_large_result_tmp = k_malloc(strlen(data_large_result_concat-1));
			strcpy(data_large_result_tmp, data_large_result_concat);
			k_free(data_large_result_concat);
		} else {
			data_large_result_tmp = append_strings("", (char*)data_single_result);
		}
		
		/* Received last block */
		if (r == 1) {
			memset(get_block_context_ptr(), 0, sizeof(get_block_context()));
			char * data_large_result_concat = append_strings(data_large_result_tmp, "");
			return data_large_result_concat;
		}
	}
	return NULL;
}

void extract_data_result(struct coap_packet packet, uint8_t* data_result, bool add_termination)
{
	uint16_t len = 0;
	uint8_t* payload = coap_packet_get_payload(&packet, &len);
	if (payload != NULL) 
	{
		memcpy(data_result, payload, len * sizeof(uint8_t)); 
		if (add_termination) 
		{
			char tmp[2] = ""; 
			strcat(data_result, tmp);
		}
	}
	else {
		memset(data_result, 0, MAX_COAP_MSG_LEN);
	} 
}