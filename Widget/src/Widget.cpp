/*
 * Widget.cpp
 *
 *  Created on: Apr 4, 2014
 *      Author: jiharris
 *
 *      Base (abstract) Widget class, encapsulates ZeroMQ PUB/SUB handling in a simple setup() / loop() environment
 *      subclasses need to provide a local_setup(), local_work(), and local_shutdown().
 *      during each loop(), a widget will wait for a subscribed message to come in, it assumes a header and a payload
 *      (each is a std::vector<unsigned char>), and passes both to local_work() for processing. If an outbound message
 *      should be sent, local_work() returns True. Both header and payload are passed along to the publisher socket.
 */

#include "Widget.hpp"
#include <iostream>

void Widget::setup( const char *hint = "connect" )
{
	int rc = 0;
	// Record start timestamps
	start_tp = std::chrono::steady_clock::now();
	start_wall = std::time(nullptr);
	{
		char buf[64];
		std::tm* tm_info = std::localtime(&start_wall);
		if (tm_info) {
			std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", tm_info);
			std::cout << "[TIMING] START " << buf << std::endl;
		} else {
			std::cout << "[TIMING] START epoch " << static_cast<long long>(start_wall) << std::endl;
		}
	}
	// start logging
	// create context (catch exception)
	// if sub_endpoint, create a SUB socket(catch exception)
	if( strcmp(sub_endpoint, "") != 0 )
	{
		subscriber = new zmq::socket_t( *context, ZMQ_SUB );
		subscriber->setsockopt( ZMQ_SUBSCRIBE, subscription, strlen(subscription) );

		// set SUB to connect if hinted, else bind
		if( connect_sub )
		{
			subscriber->connect( sub_endpoint );
			std::cout << "Connect to " << sub_endpoint << std::endl;
		}
		else
		{
			subscriber->bind( sub_endpoint );
			std::cout << "Bind to " << sub_endpoint << std::endl;
		}
	}

	// if pub_endpoint, create a PUB socket (catch exception)
	if( strcmp(pub_endpoint, "") != 0 )
	{
		publisher = new zmq::socket_t( *context, ZMQ_PUB );

		// set PUB to connect if hinted, else bind (note: same as above?)
		if( connect_pub )
		{
			publisher->connect( pub_endpoint );
			std::cout << "Connect to " << pub_endpoint << std::endl;
		}
		else
		{
			publisher->bind( pub_endpoint );
			std::cout << "Bind to " << pub_endpoint << std::endl;
		}
	}

	// call local_setup() (virtual)
	local_setup();

	// Log "setup complete"
}

void Widget::loop()
{
	bool shouldSend = false;
	zmq::message_t envelope;
	// TODO: Must make a special widget for the initial publish (with no subscribe)
	//sendPart( "TESTING", 0);
	//std::cout << "sent test msg" << std::endl;
	// Log "waiting for %s packet", subscription
	//std::cout << "Waiting for " << subscription << " packet" << std::endl;
	if( (subscriber->recv( &envelope ) != false) )
	{
		//std::cout << "got a message...";
		// only gets here if something comes off the socket.
		// Now that you know a header is inbound, grab all of its pieces.
		receiveMessage( &header, &payload );

		//std::cout << "received." << std::endl;

		shouldSend = local_work( &header, &payload );

		//std::cout << "local_work done." << std::endl;

		if( shouldSend && strcmp( publication, "Not Implemented") != 0 )
		{
			//std::cout << "about to send... ";
			local_send( &header, &payload );
			//std::cout << "done." << std::endl;
		}

		//std::cout << "done with message...";
		header.clear();
		payload.clear();
		//std::cout << "cleared." << std::endl;
	}
	/*
	 *
	def loop( self ):
		#self.log.info( "Waiting for %s packet..." % (self.subscription) )
		envelope = self.sub.recv() # envelope
		#self.log.debug( "Got " + msg )
		asjson	= self.receivePart( "metadata" )
		c_blob	= self.receivePart( "client" )
		#self.log.debug( c_blob )
		s_blob	= self.receivePart( "server" )

		metadata = json.loads( asjson )
		metadata["format"] = envelope
		self.work( metadata, c_blob, s_blob )
		asjson = json.dumps(metadata)

		# If we could not determine the correct header envelope, just drop it
		if self.publication != "Not Implemented":
			self.write_to_log( asjson, c_blob, s_blob )
		#elif self.publication != "None":
		#	self.log.info( "Dropping unimplemented header type: %s" % metadata["format"] )
		elif self.publication == "None":
			self.log.warning( "Unknown header envelope for: %s" % metadata )
	 */
}

void Widget::receiveMessage( msgpack::sbuffer *header, msgpack::sbuffer *payload )
{
	// Right now, there's only one part per header, but this makes it easy to have more than one.
	//std::cout << "header ";
	receivePart( header );
	//std::cout << "payload ";
	receivePart( payload );
}
// receive a header part in the form:
// length
// data (length bytes)
void Widget::receivePart( msgpack::sbuffer *part, const char *tag )
{
	zmq::message_t inbound;
	subscriber->recv( &inbound );

	// Convert zmq::message_t to msgpack::sbuffer
	part->write(static_cast<const char *>(inbound.data()), inbound.size());
}

// When a msgpack::sbuffer is received, it needs to be tweaked a little before it can be
// rehydrated into the original class structure.
void Widget::unPackPart( const msgpack::sbuffer *in, msgpack::object *out )
{
	// deserialize it.
	msgpack::unpacked msg;
	msgpack::unpack(msg, in->data(), in->size());

	// print the deserialized object.
	*out = msg.get();
	//std::cout << out << std::endl;
}


/*// available to other widgets, for creating ad hoc messages
void Widget::sendMessage( void *message, int length )
{
	// First, send the subscription envelope
	sendPart( publication, ZMQ_SNDMORE );
	// then send the header payload. This could be multi-part if desired.
	sendPart( message, length, 0 );
}/**/

void Widget::sendMessage( msgpack::sbuffer *header, msgpack::sbuffer *payload, char *pub, zmq::socket_t *out_socket)
{
	if( pub == NULL )
	{
		pub = publication;
	}
	/*else
	{
		std::cout << pub << std::endl;
	}/**/

    if( out_socket == NULL )
    {
        out_socket = publisher;
    }
	// First, send the subscription envelope (don't send via sendPart, which leads with a strlen segment)
	out_socket->send( pub, strlen(pub), ZMQ_SNDMORE );
	// then send the header payload. This could be multi-part if desired.
	sendPart( header, ZMQ_SNDMORE, out_socket );
	sendPart( payload, 0, out_socket );
}

/*void Widget::sendPart( void *part, int length, int more )
{
	size_t len_part = length;
	std::string len_p_str = to_string(len_part);
	zmq_send(publisher, len_p_str.c_str(), len_p_str.length(),	ZMQ_SNDMORE);
	zmq_send(publisher, part, len_part, more);
}/**/

void Widget::sendPart( char *part, int more )
{
	std::cout << "sPc:" << strlen(part);
	/*size_t len_part = strlen(part);
	std::string len_p_str = to_string(len_part);
	zmq_send(publisher, len_p_str.c_str(), len_p_str.length(),	ZMQ_SNDMORE);
	zmq_send(publisher, part, len_part, more);/**/
}

void Widget::sendPart( msgpack::sbuffer *part, int more, zmq::socket_t *out_socket)
{
	zmq::message_t outbound(part->size());

	// Convert msgpack::sbuffer to zmq::message_t
	memcpy( outbound.data(), part->data(), part->size() );

    if( out_socket == NULL )
    {
        out_socket = publisher;
    }
	out_socket->send( outbound, more );
}

/*
 * 	def sendPart( self, data, more ):
	length = bytearray(str(len(data)))
	payload = bytearray(data)
	self.pub.send( length, zmq.SNDMORE )
	self.pub.send( payload, more )

def work( self, metadata, c_blob, s_blob ):
	log.error("widget.work is virtual")
	return

def write_to_log( self, metadata, c_blob, s_blob ):
	#self.log.debug("Publishing %s header" % self.publication)
	self.pub.send( self.publication, zmq.SNDMORE )
	self.sendPart( metadata, zmq.SNDMORE )
	self.sendPart( c_blob, zmq.SNDMORE )
	self.sendPart( s_blob, 0 )
 */
void Widget::shutdown()
{
	//LOGGER_INFO << "Exiting Scatter Proxy";
	local_shutdown();

	// Record stop timestamps and elapsed
	auto stop_tp = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop_tp - start_tp).count();
	std::time_t stop_wall = std::time(nullptr);
	{
		char buf[64];
		std::tm* tm_info = std::localtime(&stop_wall);
		if (tm_info) {
			std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", tm_info);
			std::cout << "[TIMING] STOP  " << buf << " (elapsed " << (elapsed/1000.0) << "s)" << std::endl;
		} else {
			std::cout << "[TIMING] STOP epoch " << static_cast<long long>(stop_wall)
					  << " (elapsed " << (elapsed/1000.0) << "s)" << std::endl;
		}
	}

	// Ensure sockets do not linger during shutdown
	int linger = 0;
	if( subscriber != NULL )
	{
		try {
			subscriber->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
			subscriber->close();
		} catch(...) { /* swallow close errors */ }
		subscriber = NULL;
	}

	if( publisher != NULL )
	{
		try {
			publisher->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
			publisher->close();
		} catch(...) { /* swallow close errors */ }
		publisher = NULL;
	}

	running = false;
	if (context != NULL) {
		try {
			context->close();
		} catch(...) { /* swallow close errors */ }
		context = NULL;
	}
	// stop logging
}

int Widget::run()
{
	std::cout << "run()" << std::endl;
	setup();
	std::cout << "setup done" << std::endl;
	while( running )
	{
		//std::cout << "looping... " << std::flush;
		loop();
	}
	std::cout << "shutting down widget" << std::endl;
	shutdown();
	std::cout << "shutdown complete. Goodbye." << std::endl;
	return 0;
}
/*from usage import Usage
import sys
import getopt
def main(argv=None):
	if argv is None:
		argv = sys.argv
	try:
		try:
			opts, args = getopt.getopt(argv[1:], "hvr", ["help", "verbose"])
		except getopt.error, msg:
			raise Usage(msg)

		# process options
		for o, a in opts:
			if o in ("-h", "--help"):
				print """ bridge.py <file1> <file2> ... <filen> """
				return 0
			elif o in ("-v", "--verbose"):
				prefs['verbose'] += 1
		# process arguments
#		for arg in args:
#			scenefile = arg
#			process(arg) # process() is defined elsewhere
		if len(args) == 4:
			( subscription, sub_endpoint, publication, pub_endpoint ) = args
		else:
			( subscription, sub_endpoint, publication, pub_endpoint ) = ( "_VIRTUAL_", "inproc:virtual", "_VIRTUAL", "inproc:virtual" )
	except Usage, err:
		print >>sys.stderr, err.msg
		print >>sys.stderr, "for help use --help"
		return 2

	# in order for a Python object to properly use an ipc,
	# it needs to run as root (or the user that created the endpoint)
	w = Widget( "widget", subscription, sub_endpoint, publication, pub_endpoint )
	w.setup()
	while w.running:
		#self.log.debug("Main loop")
		w.loop()
	w.shutdown()

if __name__ == "__main__":
	sys.exit(main())
/**/
