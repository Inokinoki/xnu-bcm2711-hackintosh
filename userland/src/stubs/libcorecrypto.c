/*
 * libcorecrypto is closed. The C runtime lists it as a dependency (so the file must exist) and libSystem's fork
 * handlers call the cc_atfork hooks; there is no corecrypto state to protect across fork.
 */
void cc_atfork_prepare(void) { }
void cc_atfork_parent(void) { }
void cc_atfork_child(void) { }
