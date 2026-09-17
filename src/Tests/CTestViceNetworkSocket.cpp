#include "CTestViceNetworkSocket.h"
#include "EmulatorsConfig.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include <cstring>

#if defined(RUN_COMMODORE64)
extern "C" {
#include "vicesocket.h"
}
#endif

// Characterization test for the VICE socket layer that HAVE_NETWORK switches
// on (root/socket.c + arch/socketimpl.h). The IDE64 USB server is built on
// exactly these calls, so if this fails, the USB server cannot work either.
//
// Port 64246 rather than the IDE64 default 64245, so a debugger started by
// hand with -IDE64USB does not collide with the test.
#define TEST_ADDRESS "ip4://127.0.0.1:64246"

void CTestViceNetworkSocket::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;

#if !defined(RUN_COMMODORE64)
	TestCompleted(true, "C64 emulator disabled, skipping");
	return;
#else
	int step = 0;

	vice_network_socket_address_t *addr = vice_network_address_generate(TEST_ADDRESS, 0);
	step++;
	if (addr == NULL)
	{
		StepCompleted(step, false, "vice_network_address_generate returned NULL");
		TestCompleted(false, "address generate failed");
		return;
	}
	StepCompleted(step, true, "address generated for " TEST_ADDRESS);

	vice_network_socket_t *server = vice_network_server(addr);
	step++;
	if (server == NULL)
	{
		StepCompleted(step, false, "vice_network_server returned NULL (port already taken?)");
		vice_network_address_close(addr);
		TestCompleted(false, "server socket failed");
		return;
	}
	StepCompleted(step, true, "server listening on " TEST_ADDRESS);

	vice_network_socket_t *client = vice_network_client(addr);
	step++;
	if (client == NULL)
	{
		StepCompleted(step, false, "vice_network_client returned NULL");
		vice_network_socket_close(server);
		vice_network_address_close(addr);
		TestCompleted(false, "client connect failed");
		return;
	}
	StepCompleted(step, true, "client connected");

	// accept (poll up to ~2s; localhost connects are fast but not instant)
	vice_network_socket_t *conn = NULL;
	for (int i = 0; i < 200 && conn == NULL; i++)
	{
		if (vice_network_select_poll_one(server))
		{
			conn = vice_network_accept(server);
			break;
		}
		SYS_Sleep(10);
	}
	step++;
	if (conn == NULL)
	{
		StepCompleted(step, false, "no incoming connection within 2s");
		vice_network_socket_close(client);
		vice_network_socket_close(server);
		vice_network_address_close(addr);
		TestCompleted(false, "accept failed");
		return;
	}
	StepCompleted(step, true, "connection accepted");

	// round-trip client -> server
	const char ping[4] = { 'P', 'I', 'N', 'G' };
	char buf[4];
	memset(buf, 0, sizeof(buf));

	bool ok = (vice_network_send(client, ping, 4, 0) == 4);
	if (ok)
	{
		int received = 0;
		for (int i = 0; i < 200 && received < 4; i++)
		{
			if (vice_network_select_poll_one(conn))
			{
				int r = vice_network_receive(conn, buf + received, 4 - received, 0);
				if (r <= 0)
					break;
				received += r;
			}
			else
			{
				SYS_Sleep(10);
			}
		}
		ok = (received == 4) && (memcmp(buf, ping, 4) == 0);
	}
	step++;
	StepCompleted(step, ok, ok ? "PING round-trip ok" : "send/receive round-trip failed");

	// vice_network_select_multiple() is the function mon_util.c needs and our
	// tree was missing; exercise it so a future re-trim cannot silently drop it
	vice_network_socket_t *sockets[2];
	sockets[0] = conn;
	sockets[1] = NULL;
	int selectMultiple = vice_network_select_multiple(sockets);
	step++;
	StepCompleted(step, selectMultiple >= 0,
				  selectMultiple >= 0 ? "vice_network_select_multiple returned a valid result"
									  : "vice_network_select_multiple failed");

	vice_network_socket_close(conn);
	vice_network_socket_close(client);
	vice_network_socket_close(server);
	vice_network_address_close(addr);

	bool allOk = ok && (selectMultiple >= 0);
	TestCompleted(allOk, allOk ? "vice_network socket layer works"
							   : "socket layer verification failed");
#endif
}

void CTestViceNetworkSocket::Cancel()
{
	isRunning = false;
}
