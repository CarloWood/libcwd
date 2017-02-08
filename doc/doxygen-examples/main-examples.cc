/* Add this at the top of your main() to have a quick start.	*/

  Debug(myproject::debug::init());		// This is a custom function defined
  						// in example-project/debug.cc.

/* Add this at the start of each new thread.	*/

  Debug(myproject::debug::thread_init());	// This is a custom function defined
  						// in example-project/debug.cc.

