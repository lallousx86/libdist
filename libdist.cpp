/*
History
--------
02/29/2004 -
* initial version
03/15/2004 -
* fix: now it will not create reference of non existing files
* add: support for positive depth reference
05/13/2005 -
* 0.1.2
* add: added conditional directive to allow using of STDAFX if needed (for precompiled headers support)
04/07/2008
* 0.1.3
* fix: trim_str() was choking on empty string input when compiled with VC8
* added display count of warning and errors

ToDo
-------
. Allow adding a comment to the beginning of the file saying the date the file was last released
. Add copying of .hpp files as well
*/

#include "libdist.h"
module_map_t g_modules;

const int module_extensions_cnt = 4;
constexpr char *libdistversion = "libdist v0.1.3 (" __DATE__ " " __TIME__") ";
int g_nWarning = 0, g_nError = 0;

enum action_types_e
{
    at_make,
    at_rel,
    at_hardlink
};

static const char *module_extensions[module_extensions_cnt] =
{
    ".h", ".cpp", ".hpp", ".c"
};

// adds a backslash to path name
void fix_path(std::string &path)
{
    if (path[path.length() - 1] != '\\')
        path = path + '\\';
}

void fix_path_distance(string refereed_path, string referent, string &fixed)
{
    // fix pathes
    fix_path(refereed_path);
    fix_path(referent);

    // if path starts with current path designator then remove it
    if (referent.substr(0, 2) == ".\\")
        referent = referent.substr(2);

    if (refereed_path.substr(0, 2) == ".\\")
        refereed_path = refereed_path.substr(2);

    // accessing absolute path?
    if ((refereed_path.substr(1, 2) == ":\\") || (refereed_path[0] == '\\'))
    {
        fixed = refereed_path;
        return;
    }

    // positive depth
    if (referent.substr(0, 2) != "..")
    {
        int depth(0);
        char *p = (char *)referent.c_str();
        while (*p)
        {
            if (*p == '\\')
                depth++;
            p++;
        }

        fixed = "";
        for (int i = 0; i<depth; i++)
            fixed = fixed + "..\\";

        fixed = fixed + refereed_path;
    }
    // negative depth (such as ..\test\blah\)
    else
    {
        char cwdbuf[256];
        _getcwd(cwdbuf, sizeof(cwdbuf));
        string cwd(cwdbuf);
        fix_path(cwd);

        // not supported yet
    }
    return;
}

// checks if a file exists
bool file_exists(const char *fn)
{
    ifstream f(fn);
    if (!f)
        return false;
    else
    {
        f.close();
        return true;
    }
}
// trims white spaces from left and right of a string
void trim_str(string &str)
{
    int i, j;
    char ch;

    if (str.size() == 0)
    {
        str = "";
        return;
    }
    for (i = 0;; i++)
    {
        ch = str[i];
        if (ch == '\t' || ch == ' ')
            continue;
        else
            break;
    }

    for (j = str.length() - 1;; j--)
    {
        ch = str[j];
        if (ch == '\t' || ch == ' ')
            continue;
        else
            break;
    }

    if (j == str.length() - 1)
        j++;

    str = str.substr(i, j - i);
}


// copies a binary file
bool copy_file(const char *src, const char *dest)
{
    FILE *infile = fopen(src, "rb");

    if (infile == NULL)
        return false;

    FILE *outfile = fopen(dest, "wb");
    if (outfile == NULL)
    {
        fclose(infile);
        return false;
    }

    char buf[4096];
    long fsize, readlength;

    fseek(infile, 0, SEEK_END);
    fsize = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    while (fsize > 0)
    {
        readlength = fsize > sizeof(buf) ? sizeof(buf) : fsize;
        fsize -= readlength;

        fread(buf, readlength, 1, infile);
        fwrite(buf, readlength, 1, outfile);
    }

    fclose(infile);
    fclose(outfile);

    return true;
}

// parses module file name
bool parse_module_file(const char *modulefilename)
{
    fstream f(modulefilename);

    if (!f)
    {
        cout << "[!] module file not found!" << endl;
        return false;
    }

    string line;
    int    line_no;

    g_modules.clear();

    string last_reference, last_path, module;

    string::size_type pos;

    for (line_no = 1; !f.eof(); line_no++)
    {
        getline(f, line);
        trim_str(line);

        // is it a comment line?
        if (line[0] == ';')
        {
            cout << "[!] skipping comment line " << line_no << endl;
            continue;
        }

        // did we find a reference?
        if ((pos = line.find("reference")) != string::npos)
        {
            last_reference = line.substr(10);
            cout << "[i] found reference: " << last_reference << endl;
            continue;
        }

        // did we find a path?
        if ((pos = line.find("path")) != string::npos)
        {
            last_path = line.substr(5);
            cout << "[i] found path: " << last_path << endl;
            continue;
        }

        // did we find a module?
        if ((pos = line.find("module")) != string::npos)
        {
            module = line.substr(7);

            if ((last_reference.length() == 0) || (last_path.length() == 0))
            {
                cout << "[!] module encountered but no path and reference were ready!" << endl;
                continue;
            }
            cout << "[i] found module: '" << module << "'" << endl;
        }
        g_modules[last_path][last_reference].push_back(module);
    }

    f.close();
    return true;
}

// releases or makes module files
void act_upon_modules(int nActType)
{
    if (g_modules.size() == 0)
    {
        cout << "[!] no modules to make!" << endl;
        return;
    }

    module_map_t::iterator path_it;
    string module, path, fixed_path, reference, pathfile, reffile, fixed_pathfile;

    for (path_it = g_modules.begin(); path_it != g_modules.end(); ++path_it)
    {
        stringlist_map_t::iterator ref_it;
        stringlist_map_t &ref = path_it->second;

        for (ref_it = ref.begin(); ref_it != ref.end(); ++ref_it)
        {
            string_list_t &lst = ref_it->second;

            while (lst.size())
            {
                path = path_it->first;
                reference = ref_it->first;

                fix_path(reference);
                fix_path(path);

                fixed_path = path;
                fix_path_distance(path, reference, fixed_path);

                module = lst.back();
                lst.pop_back();

                for (int i = 0; i<module_extensions_cnt; i++)
                {
                    reffile = reference + module + module_extensions[i];
                    pathfile = path + module + module_extensions[i];
                    fixed_pathfile = fixed_path + module + module_extensions[i];

                    // make references
                    if (nActType == at_make)
                    {
                        if (!file_exists(pathfile.c_str()))
                        {
                            cout << "[!] will not create reference non existing file: " << pathfile.c_str() << endl;
                            g_nWarning++;
                            continue;
                        }

                        ofstream out(reffile.c_str());

                        if (!out)
                        {
                            cout << "[!] could not create ref file: " << reffile << endl;
                            g_nError++;
                            break; // goes to next module
                        }
                        /*
                        out << "#ifdef __USE_STDAFX__" << endl;
                        out << "#include \"stdafx.h\"" << endl;
                        out << "#endif" << endl << endl;
                        */

                        out << "#include \"" << fixed_pathfile << "\"" << endl;
                        out.close();

                        cout << "[>] created reference module: " << reffile << endl;
                    }
                    // release files
                    else if (nActType == at_rel)
                    {
                        if (!file_exists(pathfile.c_str()))
                        {
                            cout << "[!] path file " << pathfile << " not found" << endl;
                            continue;
                        }
                        if (!copy_file(pathfile.c_str(), reffile.c_str()))
                            cout << "[!] could not release to " << reffile << endl;
                        else
                            cout << "[>] released to " << reffile << endl;
                    }
                    else if (nActType == at_hardlink)
                    {
                        if (!file_exists(pathfile.c_str()))
                        {
                            cout << "[!] path file " << pathfile << " not found" << endl;
                            continue;
                        }

                        std::string cmd;

                        if (file_exists(reffile.c_str()))
                        {
                            cmd = "del " + reffile;
                            system(cmd.c_str());
                        }

                        cmd = "fsutil hardlink create \"" + reffile + "\" \"" + pathfile + "\">nul";
                        system(cmd.c_str());

                        if (file_exists(reffile.c_str()))
                            cout << "[>] hardlinked with " << reffile << endl;
                        else
                            cout << "[!] could not hardlink with " << reffile << endl;
                    }
                } // extensions loop
            } // list of modules in same path/ref/
        } // refs in same path loop
    } // refs loop
}

//-------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout
            << libdistversion << endl
            << "usage: libdist module.ini action" << endl
            << "action can be: 'make', 'hardlink' or 'rel'" << endl;
        return 1;
    }

    int act_type;
    const char *act_type_str;
    if (_stricmp(argv[2], "rel") == 0)
    {
        act_type_str = "releasing";
        act_type = at_rel;
    }
    else if (_stricmp(argv[2], "make") == 0)
    {
        act_type = at_make;
        act_type_str = "making references";
    }
    else if (_stricmp(argv[2], "hardlink") == 0)
    {
        act_type = at_hardlink;
        act_type_str = "hardlinking";
    }

    cout << "[i] " << libdistversion << " " << act_type_str << "..." << endl;

    if (!parse_module_file(argv[1]))
    {
        cout << "failed while parsing module file!";
        return 3;
    }

    act_upon_modules(act_type);

    cout << "Completed with " << g_nWarning << " warning(s) " << g_nError << " error(s)" << endl;
    return 0;
}
