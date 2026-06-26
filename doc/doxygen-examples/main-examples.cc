/* Add this at the top of your main(). */

  Debug(NAMESPACE_DEBUG::init());               // This function is defined by cwds.

/* Add this at the start of each new thread. */

  Debug(NAMESPACE_DEBUG::thread_init());        // This function is defined by cwds.
