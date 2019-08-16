//============================================================================
// Name        : ControlChannel.cpp
// Author      : VT
// Version     :
// Copyright   : Copyright 2014 Jimbo S. Harris. All Rights Reserved.
// Description : Passthru PUB/SUB proxy
//============================================================================

#include <iostream>
using namespace std;
#include <zmq.h>
#include <assert.h>

void ControlChannel()
{
	void *subscriber;
	void *publisher;
	void *capture = NULL;

	std::string in_port = "ipc:///tmp/feeds/control";
	std::string out_port = "ipc:///tmp/feeds/broadcast";

	int rc = 0;
	void *context = zmq_ctx_new();

	std::cout << "Starting Control Channel Proxy between " << in_port << " and " << out_port << std::endl;

	// IN port initialization
	subscriber = zmq_socket(context, ZMQ_SUB);
	assert( subscriber );

	rc = zmq_setsockopt( subscriber, ZMQ_SUBSCRIBE, "", 0);
	assert( rc == 0 );

	rc = zmq_bind( subscriber, in_port.c_str() );
	assert( rc == 0 );

	// from https://zeromq.jira.com/browse/LIBZMQ-270
	// to fix problem of first message never gets through
	zmq_pollitem_t pollitems [] = {
	{ subscriber, 0, ZMQ_POLLIN, 0 }
	};
	zmq_poll (pollitems, 1, 1);

	// OUT port initialization
	publisher = zmq_socket(context, ZMQ_PUB);
	assert( publisher );

	rc = zmq_bind( publisher, out_port.c_str() );
	assert( rc == 0 );

	zmq_proxy( subscriber, publisher, capture );

	// only gets here at shutdown time
	std::cout << "Exiting Control Channel Proxy" << std::endl;

	zmq_close(subscriber);
	subscriber = NULL;

	zmq_close(publisher);
	publisher = NULL;
}

int main()
{
	ControlChannel();
	return 0;
}

