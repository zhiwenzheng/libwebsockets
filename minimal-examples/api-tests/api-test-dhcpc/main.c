/*
 * lws-api-test-dhcpc
 *
 * Written in 2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 */

#include <libwebsockets.h>
#include <signal.h>

static int interrupted, ok, fail, exp = 1;
struct lws_context *context;
const char *nif;

static int
lws_dhcpc_cb(void *opaque, int af, uint8_t *ip, int ip_len)
{
	lwsl_user("%s: dhcp set OK\n", __func__);
	ok = 1;
	interrupted = 1;
	return 0;
}

void sigint_handler(int sig)
{
	interrupted = 1;
}

int
main(int argc, const char **argv)
{
	int n = 1, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
	struct lws_context_creation_info info;
	const char *p;

	/* the normal lws init */

	signal(SIGINT, sigint_handler);

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS API selftest: Async DNS\n");

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

	if ((p = lws_cmdline_option(argc, argv, "-i")))
		nif = p;

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}

	if (nif) {
		lwsl_user("%s: requesting DHCP for %s\n", __func__, nif);
		lws_dhcpc_request(context, nif, AF_INET, lws_dhcpc_cb, NULL);
	} else {
		lwsl_err("%s: use -i <network-interface> to select if\n", __func__);
		interrupted = 1;
	}

	/* the usual lws event loop */

	n = 1;
	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

	lws_context_destroy(context);

	if (fail || ok != exp)
		lwsl_user("Completed: PASS: %d / %d, FAIL: %d\n", ok, exp,
				fail);
	else
		lwsl_user("Completed: ALL PASS: %d / %d\n", ok, exp);

	return !(ok == exp && !fail);
}
