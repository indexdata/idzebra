
#include <passwddb.h>

int main (int argc, char **argv)
{
	Passwd_db db;

	db = passwd_db_open();

	passwd_db_file (db, "/etc/passwd");
	passwd_db_auth (db, "adam", "xtx9Y=");
	passwd_db_close (db);
	return 0;
}
