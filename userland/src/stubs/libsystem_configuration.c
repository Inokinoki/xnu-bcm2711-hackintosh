/*
 * libsystem_configuration (SystemConfiguration's libSC_info) is closed. libSystem's fork handlers call these three;
 * there is no state behind them.
 */
void _libSC_info_fork_prepare(void) { }
void _libSC_info_fork_parent(void) { }
void _libSC_info_fork_child(void) { }
