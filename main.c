#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/types.h>

#include "config.h"
#include "repos.h"

unsigned int flags;
unsigned int mode;

enum {
	FLAG_KEEP_COPY = 0x01,
	MODE_INSTALL   = 0x01,
	MODE_REMOVE    = 0x02,
	MODE_LIST      = 0x03
};

enum {
	TYPE_ERROR,
	TYPE_INFO,
	TYPE_OK
};

void _log(int, const char*);
int  _spawn(const char*);
void pulse_sync(void);
void pulse_list(void);
void _dir_list_recurse(const char*);

int
main(argc, argv)
	int argc;
	char** argv;
{
	char opt;
	bool need_package = false;
	while ((opt = getopt(argc, argv, "hirSs")) != -1)
	{
		switch (opt)
		{
			case 'i':
			{
				if (mode != 0)
				{
					_log(TYPE_ERROR, "Only one mode allowed at a time.");
					return EXIT_FAILURE;
				} else
				{
					need_package = true;
					mode = MODE_INSTALL;
				}
				break;
			}
			case 'r':
			{
				if (mode != 0)
				{
					_log(TYPE_ERROR, "Only one mode allowed at a time.");
					return EXIT_FAILURE;
				} else
				{
					need_package = true;
					mode = MODE_REMOVE;
				}
				break;
			}
			case 's':
			{
				if (mode != 0)
				{
					_log(TYPE_ERROR, "Only one mode allowed at a time.");
					return EXIT_FAILURE;
				} else
				{
					mode = MODE_LIST;
				}
				pulse_list();
				break;
			}
			case 'S':
			{
				pulse_sync();
				break;
			}
			case 'h':
			{
				fprintf(stdout, "-i\tInstall a package.\n-r\tRemove a package\n-S\tSync the package tree\n-s\tList all available packages.\n");
				break;
			}
			default:
			{
				fprintf(stderr, "Usage: %s [-hsS] [-ir [packagename]]\n", argv[0]);
				return EXIT_FAILURE;
			}
		}
	}

	if (mode != 0 && need_package == true && optind >= argc)
	{
		fprintf(stderr, "[\033[31mFAIL\033[0m]: Expected package name\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void
pulse_list()
{
	_dir_list_recurse(TREE);
}

void
_dir_list_recurse(path)
	const char* path;
{
	DIR *dp;
	struct dirent *ep;
	dp = opendir(path);

	if (dp != NULL)
	{
		while (ep = readdir(dp))
		{
			char buffer_path[256];
			if (ep->d_name[0] != '.' &&
				ep->d_type == DT_DIR)
			{
				sprintf(buffer_path, "%s%s/", path, ep->d_name);
				_dir_list_recurse(buffer_path);
			} else if(ep->d_type == DT_REG)
			{
				sprintf(buffer_path, "%s%s", path, ep->d_name);
				_log(TYPE_INFO, buffer_path);
			}
		}
		(void) closedir(dp);
	} else
	{
		_log(TYPE_ERROR, "Could not open tree\n\tHave you cloned it?");
	}
	
}

void
pulse_sync()
{
	register int i;
	char sync[128];
	_log(TYPE_INFO, "Starting Sync...");
	for (i = 0; i < NUM_REPOS; i++)
	{
		_log(-1, repositories[i][1]);
		sprintf(sync, "TREE=%s; %s", repositories[i][1], CMD_SYNC);

		int error_code = 0;
		if ((error_code = _spawn(sync)) != 0)
		{
			_log(TYPE_ERROR, "Sync failed!");
			printf("\t--> \033[1;31m%d\033[0m\n", error_code);
			exit(EXIT_FAILURE);
		}
	}
}

/* Spawn a command from shell. */
int
_spawn(cmd)
	const char* cmd;
{
	FILE* f;
	char* buffer = malloc(512);

	printf("%s,%d", cmd, strlen(cmd));
	f = popen(cmd, "r");
	if (f == NULL)
	{
		_log(TYPE_ERROR, "could not run command");
	}

	while (fscanf(f, "%[^'\n']", buffer) > 0)
	{
		fgetc(f);
		_log(-1, buffer);
	}
	
	free(buffer);
	return pclose(f)/256;
}

void
_log(type, message)
	int type;
	const char* message;
{
	static int pre_type;
	if (type < 0)
	{
		fprintf(stderr, "\t");
	} else
	{
		switch (type)
		{
			case TYPE_ERROR:
			{
				fprintf(stderr, "[\033[31mFAIL\033[0m]: ");
				break;
			}
			case TYPE_INFO:
			{
				fprintf(stderr, "[\033[34mINFO\033[0m]: ");
				break;
			}
			case TYPE_OK:
			{
				fprintf(stderr, "[\033[32m OK \033[0m]: ");
				break;
			}
		}
	}
	fprintf(stderr, "%s\n", message);
}
