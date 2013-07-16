#include "hotpot/hp_error.h"
#include "globals.h"

#include "hotprotocol/hp_json_writer.h"
#include "hotscript/hotobject.h"
#include "hotscript/hot_vm.h"
#include "hotscript/hotobject_reader.h"
#include "hotscript/hotobject_writer.h"
#include "hotscript/script_parser.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "hotdata_parser.h"
#include "hotprotocol/hp_xml_writer.h"
#include "hotprotocol/hp_xml_reader.h"

#include "language/language_types.h"
#include "language/language_reader.h"
#include "language/language.h"
#include "hotjson/json_parser.h"

#include "hotscript/hot_vm.h"
#include <io.h>

#define HOTDATA_VERSION "0.0.1"

void version()
{
	printf("HotData version %s\n", HOTDATA_VERSION);
}

void usage()
{
	fprintf(stderr, "Usage: hd [options] file\n\n");
	fprintf(stderr, "Use hd -help for a list of options\n");
}

void help()
{
	fprintf(stderr, "Usage: thrift [options] file\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -version					Print the compiler version\n");
	fprintf(stderr, "  -hs filename				Set the template file\n");
	fprintf(stderr, "  -i dir					Add a directory to the list of directories\n");
	fprintf(stderr, "  -j dir					set json output path\n");
fprintf(stderr, "  -o dir					set output path\n");
}


LanguageLib language_lib;
DATA_PARSER dp;
HP_JSON_WRITER jw;
char file_name[HP_MAX_FILE_PATH_LENGTH];
HotObjectReader reader;
HotObjectWriter writer;
SCRIPT_PARSER sp;
FILE* output_file = NULL;
FILE* json_output_file = NULL;


char root_dir[HP_MAX_FILE_PATH_LENGTH];
char real_script_path[HP_MAX_FILE_PATH_LENGTH];
char path_prefix[HP_MAX_FILE_PATH_LENGTH];
char language_path[HP_MAX_FILE_PATH_LENGTH];

void script_putc(HotVM *self, char c)
{
	fputc(c, (FILE*)self->user_data);
}

void get_real_file_path()
{
	snprintf(path_prefix, HP_MAX_FILE_PATH_LENGTH, "%s%cresource%ctemplate%c", root_dir, HP_FILE_SEPARATOR, HP_FILE_SEPARATOR, HP_FILE_SEPARATOR);
	//strncpy(real_script_path, root_dir, HP_MAX_FILE_PATH_LENGTH);

	if(access(file_name, 00) == 0)
	{
		snprintf(real_script_path, HP_MAX_FILE_PATH_LENGTH, "%s", file_name);		
	}
	else
	{
		snprintf(real_script_path, HP_MAX_FILE_PATH_LENGTH, "%s%s", path_prefix, file_name);
	}
}

const char *json_input = NULL;
JSON_PARSER jp;
SCRIPT_PARSER sp;

int main(int argc, char **argv)
{
	int i;
	int count = 0;
	HotObject *obj = hotobject_new();
	hpuint32 len;
	strncpy(root_dir, argv[0], HP_MAX_FILE_PATH_LENGTH);

	
	
	//首先获得根目录
	snprintf(root_dir, HP_MAX_FILE_PATH_LENGTH, getenv("HotPot_Dir"));
	if(root_dir[strlen(root_dir) - 1] != HP_FILE_SEPARATOR)
	{
		root_dir[strlen(root_dir)] = HP_FILE_SEPARATOR;
	}
#ifdef _DEBUG
	snprintf(root_dir, HP_MAX_FILE_PATH_LENGTH, "D:\\HotPot\\");
#endif//_DEBUG

	data_parser_init(&dp);
	for (i = 1; i < argc - 1; ++i)
	{
		char* arg;

		arg = strtok(argv[i], " ");
		// Treat double dashes as single dashes
		if (arg[0] == '-' && arg[1] == '-')
		{
			++arg;
		}
		if (strcmp(arg, "-help") == 0)
		{
			help();
			goto ERROR_RET;
		}
		else if (strcmp(arg, "-version") == 0)
		{
			version();
			goto ERROR_RET;
		}
		else if (strcmp(arg, "-hs") == 0)
		{
			arg = argv[++i];
			if (arg == NULL)
			{
				fprintf(stderr, "Missing template file specification\n");
				usage();
				goto ERROR_RET;
			}
			strncpy(file_name, arg, HP_MAX_FILE_PATH_LENGTH);
		}
		else if (strcmp(arg, "-i") == 0)
		{
			arg = argv[++i];
			if (arg == NULL)
			{
				fprintf(stderr, "Missing template file specification\n");
				usage();
				goto ERROR_RET;
			}
			scanner_stack_add_path(&dp.scanner_stack, arg);
		}
		else if(strcmp(arg, "-jin") == 0)
		{
			arg = argv[++i];
			if (arg == NULL)
			{
				fprintf(stderr, "Missing template file specification\n");
				usage();
				goto ERROR_RET;
			}
			json_input = arg;
			
		}
		else if(strcmp(arg, "-jout") == 0)
		{
			arg = argv[++i];
			if (arg == NULL)
			{
				fprintf(stderr, "Missing template file specification\n");
				usage();
				goto ERROR_RET;
			}
			if(strcmp(arg, "stdout") == 0)
			{
				json_output_file= stdout;
			}
			else
			{
				json_output_file = fopen(arg, "wb");
			}
		}
		else if(strcmp(arg, "-o") == 0)
		{
			arg = argv[++i];
			if (arg == NULL)
			{
				fprintf(stderr, "Missing template file specification\n");
				usage();
				goto ERROR_RET;
			}
			output_file = fopen(arg, "wb");;
		}
		else
		{
			fprintf(stderr, "Unrecognized option: %s\n", arg);
			usage();
			goto ERROR_RET;
		}
	}

	snprintf(language_path, HP_MAX_FILE_PATH_LENGTH, "%s%cresource%clanguage%csimplified_chinese.xml", root_dir, HP_FILE_SEPARATOR, HP_FILE_SEPARATOR, HP_FILE_SEPARATOR);
	load_language(&language_lib, language_path);
	
	if(json_output_file != NULL)
	{
		ddekit_json_encoding_writer_init(&jw, json_output_file);
		
		if(data_parser(&dp, argv[i], &jw.super, &language_lib) != E_HP_NOERROR)
		{
			goto ERROR_RET;
		}
		fclose(json_output_file);
		json_output_file = NULL;
	}

	if(output_file == NULL)
	{
		goto ERROR_RET;
	}
	
	
	
	hotobject_writer_init(&writer, obj);
	
	if(data_parser(&dp, argv[i], &writer.super, &language_lib) != E_HP_NOERROR)
	{
		goto ERROR_RET;
	}

	json_parser_str(&jp, json_input, strlen(json_input), obj, &sp);
	
	
	hotobject_reader_init(&reader, obj);
	get_real_file_path();
	if(script_parser(&sp, real_script_path, &reader.super, output_file, script_putc) != 0)
	{
		goto ERROR_RET;
	}
	hotobject_free(obj);

	return 0;
ERROR_RET:
	return 1;
}
