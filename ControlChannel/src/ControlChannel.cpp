//============================================================================
// Name        : ControlChannel.cpp
// Author      : VT
// Version     :
// Copyright   : Copyright 2014-2025 Jimbo S. Harris. All Rights Reserved.
// Description : Tiny XPUB/XSUB proxy (multi-publisher, multi-subscriber bus)
//               Binds XSUB (inbound from publishers) and XPUB (outbound to subscribers)
//               Default ports:
//                 - XSUB bind: tcp://127.0.0.1:1314 (publishers connect here)
//                 - XPUB bind: tcp://127.0.0.1:1313 (subscribers connect here)
//============================================================================

#include <iostream>
using namespace std;
#include <zmq.h>
#include <assert.h>

int main(int argc, char** argv)
{
	const char* xsub_addr = (argc > 1) ? argv[1] : "tcp://127.0.0.1:1314"; // inbound from publishers
	const char* xpub_addr = (argc > 2) ? argv[2] : "tcp://127.0.0.1:1313";  // outbound to subscribers

	void *context = zmq_ctx_new();
	assert(context);

	void *xsub = zmq_socket(context, ZMQ_XSUB);
	assert(xsub);
	int rc = zmq_bind(xsub, xsub_addr);
	assert(rc == 0);

	void *xpub = zmq_socket(context, ZMQ_XPUB);
	assert(xpub);
	int verbose = 0; // set to 1 for subscription messages
	zmq_setsockopt(xpub, ZMQ_XPUB_VERBOSE, &verbose, sizeof(verbose));
	rc = zmq_bind(xpub, xpub_addr);
	assert(rc == 0);

	std::cout << "Starting XPUB/XSUB proxy\n  XSUB (inbound from publishers): " << xsub_addr
			  << "\n  XPUB (outbound to subscribers): " << xpub_addr << std::endl;

	// This will block until terminated
	zmq_proxy(xsub, xpub, NULL);

	// Shutdown
	std::cout << "Exiting XPUB/XSUB proxy" << std::endl;
	zmq_close(xsub);
	zmq_close(xpub);
	zmq_ctx_term(context);
	return 0;
}
