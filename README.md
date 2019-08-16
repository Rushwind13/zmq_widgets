## Jimbo's implementation of ZeroMQ

*Synopsis:* helper library and broker to allow other apps to communicate via ZMQ Pub/Sub

ControlChannel is an executable that brokers all requests between /tmp/feeds/control (in) and /tmp/feeds/broadcast (out)
Widget is a dynamic lib (libWidget.dylib) that acts as a wrapper for apps; it follows a setup/loop type interface

*Notes:*
* you must create the folder /tmp/feeds or else ControlChannel will crash
* RayTracer2014 has a number of apps that implement Widget
* Run ControlChannel before running any Widget-enabled apps

*TODO:*
* this method of broker is extremely CPU-intensive; the load could be spread around a bit if the individual apps talked directly to each other instead of everyone going back through the broker.
