/*
 * libcorecrypto is closed. Libraries of the C runtime list it among their dependencies, so the file has to exist for dyld
 * to load them, but nothing is imported from it yet (tools/macho_undef.py checks this): an empty dylib.
 */
int _libcorecrypto_stand_in;
