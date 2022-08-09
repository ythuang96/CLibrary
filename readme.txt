To use the CLibrary, create a file called "CLibrarySettings.h".

See the file: "./LibraraySettingsTemplate/LibraraySettings.h" for the
definitions that must exist in the file.

These definitions can be changed, but must exist.

The ENUM state_e can include mutliple different states, but the "EXITING"
state must be present

In addition, a few global variables have to be defined in the main script:

/* pointer for error log file */
FILE *error_log_;

/* Code running state */
state_e state_;

/* TCP Communication */
int master_socket_, client_socket_[MAXCLIENTS];
