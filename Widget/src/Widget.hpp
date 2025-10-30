/*
 * Widget.hpp
 *
 *  Created on: Apr 4, 2014
 *      Author: jiharris
 */

#ifndef WIDGET_HPP_
#define WIDGET_HPP_
#include <zmq.hpp>
#include <msgpack.hpp>
#include <assert.h>
#include <string>
#include <sstream> //for std::stringstream
#include <vector>
#include <chrono>
#include <ctime>

typedef std::vector<unsigned char> byte_vector;
template<class T>
static inline std::string to_string( T i )
{
        std::stringstream ss;
        std::string s;
        ss << i;
        s = ss.str();

        return s;
}

class Widget
{
public:
	Widget( char *_name, char *_subscription, char *_sub_endpoint, char *_publication, char *_pub_endpoint, bool _connect_sub = false, bool _connect_pub=false )
	{
		strcpy( name, _name );
		strcpy( subscription, _subscription );
		strcpy( sub_endpoint, _sub_endpoint );
		strcpy( publication, _publication );
		strcpy( pub_endpoint, _pub_endpoint );
		context = new zmq::context_t(1);
		assert(context);
		running = true;
		connect_pub = _connect_pub;
		connect_sub = _connect_sub;
		subscriber = NULL;
		publisher = NULL;
    	printf("Widget constructor %d %d\n", connect_pub, connect_sub);
	}
	virtual ~Widget(){};

	int run();

	// Connection mode overrides (use before run())
	void forceBindSubscriber() { connect_sub = false; }
	void forceConnectSubscriber() { connect_sub = true; }
	void forceBindPublisher() { connect_pub = false; }
	void forceConnectPublisher() { connect_pub = true; }

protected:
	virtual void local_setup() = 0;
	virtual void local_shutdown() = 0;
	virtual bool local_work( msgpack::sbuffer *header, msgpack::sbuffer *payload ) = 0;
	virtual void local_send( msgpack::sbuffer *header, msgpack::sbuffer *payload )
	{
		sendMessage( header, payload, publication );
	}
	//void sendMessage( void *message, int length );
	//void sendMessage( void *accumulator, int accum_length, void *payload, int pay_length );
	void sendMessage( msgpack::sbuffer *header, msgpack::sbuffer *payload, char *pub = NULL, zmq::socket_t *out_socket = NULL );
	void unPackPart( const msgpack::sbuffer *in, msgpack::object *out );

private:
	void setup( const char *hint );
	void loop();
	void shutdown();

	void receiveMessage( msgpack::sbuffer *header, msgpack::sbuffer *payload );
	void receivePart( msgpack::sbuffer *part, const char *tag = NULL );
	void sendPart( msgpack::sbuffer *part, int more = 0, zmq::socket_t *out_socket = NULL ); // used for header data, default to "no more coming"
	void sendPart( char *part, int more = ZMQ_SNDMORE ); // used for envelopes, so default to "more coming"
	void sendPart( void *part, int length, int more = 0 ); // used for header data, so default to "more coming"

protected:
	char name[255];
	char subscription[255];
	char sub_endpoint[255];
	char publication[255];
	char pub_endpoint[255];
	bool running = false;
	bool connect_sub = false;
	bool connect_pub = false;

	// ZMQ bits
	zmq::context_t *context;
	zmq::socket_t *subscriber;
	zmq::socket_t *publisher;

private:
	msgpack::sbuffer header;
	msgpack::sbuffer payload;

	// Timing
	std::chrono::steady_clock::time_point start_tp;
	std::time_t start_wall = 0;
};

	/*
	 * #!/usr/bin/python

import sys
import traceback
import time

import zmq
import json

import logging
from logging.handlers import RotatingFileHandler


#### Error handling
def excinfo():
	"""Retrieve exception info suitable for printing as command error."""
	exc_type, exc_value, exc_traceback = sys.exc_info()
	#string = ''.join(traceback.format_exception_only(exc_type, exc_value))
	string = traceback.format_exception(exc_type, exc_value, exc_traceback)
	#exc_type, exc_value, _ = sys.exc_info()
	#string = ''.join(traceback.format_exception_only(exc_type, exc_value))
	return string
#### end Error Handling

class Widget:
	running = False
	name = ""
	sub = None
	pub = None
	publication = ""
	subscription = ""
	pub_endpoint = ""
	sub_endpoint = ""
	context = None

	def __init__(self, _name = "testing", _subscription = None, _sub_endpoint = None, _publication = None, _pub_endpoint = None):
		self.running = True
		self.name = _name
		self.subscription = _subscription
		self.sub_endpoint = _sub_endpoint
		self.publication = _publication
		self.pub_endpoint = _pub_endpoint

	#def __init__( self ):
	#	self.running = True
	#	self.name = "testing"

	#### Logging

	log = logging.getLogger(name)

	def start_logging(self):
		sys.stderr.write('starting %s log...' % self.name)
		# create log instance (use self.log.info() to write to log

		logdir='.'

		# 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'
		self.log.setLevel(logging.DEBUG)

		# standardize formatting between file and console
		logFormatter = logging.Formatter("%(asctime)s [%(module)s.%(funcName)s] [%(levelname)-5.5s]  %(header)s")

		# output to file
		logfile = '%s/%s.log' % (logdir, self.name)
		fileHandler = RotatingFileHandler(logfile, maxBytes=1000000, backupCount=10)
		fileHandler.setFormatter(logFormatter)
		self.log.addHandler(fileHandler)

		# output to console
		consoleHandler = logging.StreamHandler()
		consoleHandler.setFormatter(logFormatter)
		self.log.addHandler(consoleHandler)
		sys.stderr.write('started.\n')

	def stop_logging(self):
		sys.stderr.write('stopping %s log...' % self.name)
		logging.shutdown()
		sys.stderr.write('stopped.\n')
	#### end Logging

	####
	#### MAIN FUNCTIONS
	####


	def local_setup( self ):
		self.log.error("widget.local_setup is virtual")
		return

	def local_shutdown( self ):
		self.log.error("widget.local_shutdown is virtual")
		return

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

	def receivePart( self, tag ):
		#self.log.debug( "receiving %s" % tag )
		length = self.sub.recv()
		#self.log.debug( "%s length %s" % ( tag, length ) )
		data = self.sub.recv()
		#self.log.debug( "Got " + data )

		if length == 0: data = None

		return data

	def sendPart( self, data, more ):
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

#endif /* WIDGET_HPP_ */
