#pragma warning (disable: 4786 4503)
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <string.h>
#include <direct.h>
#include <list>

using namespace std;

typedef list<string> string_list_t;
typedef map<string, string_list_t> stringlist_map_t;
typedef map<string, stringlist_map_t> module_map_t;


void fix_path(string &path);
bool file_exists(const char *fn);
void trim_str(string &str);
bool copy_file(const char *src, const char *dest);
bool parse_module_file(const char *modulefilename);
void make_rel_modules(bool bMake);
