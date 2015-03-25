#define VERSION "reg2html v0.2"
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include "tinyxml2.h"
#include <vector>

#include <dirent.h> // for directory / files
#include <sys/types.h>
#include <sys/stat.h>

using namespace tinyxml2;

std::string versionstring(VERSION);

//html target file out stream
std::ofstream ofile;
//files parsed
int parsecounter = 0;
//default and working file name
std::string defaultfile = "registry";
//write only deltas
bool deltasonly = false;
//flag to determine if floats should not be converted for readability
bool rawfloats = false;

//used to traverse registry folders for xml files
struct folder
{
    std::string path;
    std::vector<folder> folders;
    std::vector<std::string> files;
};

folder RegistryFolder;

/////////////////////////////////////////////////////
// FUNCTIONS
std::vector<std::string> getFiles(std::string directory, std::string extension)
{
    //create file list vector
    std::vector<std::string> fileList;

    //create directory pointer and directory entry struct
    DIR *dir;
    struct dirent *ent;
    struct stat st;

    //if able to open directory
    if ( (dir = opendir (directory.c_str() ) ) != NULL)
    {
      //go through each file in directory
      while ((ent = readdir (dir)) != NULL)
      {
            //convert read in file to a string
            std::string filename(ent->d_name);

            if(filename != "." && filename != "..")
            {

                //check that entry is a directory
                stat(std::string(directory+filename).c_str(), &st);
                if(!S_ISDIR(st.st_mode))
                {
                    //check that file matches given extension parameter
                    //if an extension is provided, make sure it matches
                    if(extension != "")
                    {
                        //find location of extension identifier '.'
                        size_t trim = filename.find_last_of('.');
                        if(trim > 200) std::cout << "ERROR:getFiles extension is invalid\n";
                        else
                        {
                            std::string target_extension = filename.substr(trim);

                            if(target_extension == extension) fileList.push_back(filename);
                            //else std::cout << "Filename:" << filename << " does not match extension parameter - ignoring\n";
                        }
                    }
                    else fileList.push_back(filename);
                }

            }

      }

      closedir (dir);
    }
    //else failed to open directory
    else
    {
      std::cout << "Failed to open directory:" << directory << std::endl;
      //return empty file list
      return fileList;
    }

    return fileList;
}

std::vector<std::string> getFolders(std::string directory, bool ignorerelative = false)
{
    //create file list vector
    std::vector<std::string> fileList;

    //create directory pointer and directory entry struct
    DIR *dir;
    struct dirent *ent;
    struct stat st;

    //if able to open directory
    if ( (dir = opendir (directory.c_str() ) ) != NULL)
    {
      //go through each file in directory
      while ((ent = readdir (dir)) != NULL)
      {
            //convert read in file to a string
            std::string filename = (ent->d_name);

            //check that entry is a directory, if so, add it to the list
            stat(std::string(directory + filename).c_str(), &st);
            if(S_ISDIR(st.st_mode))
            {
                //if ignoring relative, dont add
                if(filename == "." || filename == ".." && ignorerelative) continue;
                fileList.push_back(filename);
            }

      }

      closedir (dir);
    }
    //else failed to open directory
    else
    {
      std::cout << "Failed to open directory:" << directory << std::endl;
      //return empty file list
      return fileList;
    }

    return fileList;
}



bool parseXML(std::string filename)
{
    XMLDocument test;
    if(test.LoadFile(filename.c_str())) return false;

    XMLElement *root = test.FirstChildElement("ConfigDatabase");
    XMLElement *configdatum = root->FirstChildElement();

    XMLElement *configdatumdeltas = root->FirstChildElement();
    std::vector<XMLElement*> deltaslist;

    //if deltas only is enabled, first find the configdatums that have deltas
    if(deltasonly)
    {
        while(configdatumdeltas != NULL)
        {
            XMLElement *child_d = configdatumdeltas->FirstChildElement();

            std::string strdefault = "N\\A";
            std::string strcurrent = "N\\A";

            while(child_d != NULL)
            {
                if( std::string(child_d->Value()) == "Current") strcurrent = child_d->GetText();
                else if( std::string(child_d->Value()) == "Default") strdefault = child_d->GetText();
                child_d = child_d->NextSiblingElement();
            }
            //if current != default, add to deltas list
            if(strcurrent != strdefault) deltaslist.push_back(configdatumdeltas);
            //next config datum
            configdatumdeltas = configdatumdeltas->NextSiblingElement();
        }

        //if deltas only, and this xml has no config datums that have any deltas, return
        if(deltaslist.empty()) return true;

    }






    //std::cout << root->Value() << std::endl;
    ofile << "<h2>" << filename << "</h2>" << std::endl;

    ofile << "   <table border=\"1\">" << std::endl;
    ofile << "  <tr bgcolor=\"#C8C8C8\">" << std::endl;
    ofile << "   <th>" << std::endl;
    ofile << "       <th style=\"text-align:left\">Current</td>" << std::endl;
    ofile << "       <th style=\"text-align:left\">Default</td>" << std::endl;
    ofile << "       <th style=\"text-align:left\">Min</td>" << std::endl;
    ofile << "       <th style=\"text-align:left\">Max</td>" << std::endl;
    ofile << "       <th style=\"text-align:left\">Description</td>" << std::endl;
    ofile << "   </th>" << std::endl;
    ofile << "  </tr>" << std::endl;

    while(configdatum != NULL)
    {
        //if deltas only and this configdatum is not in the list, continue
        if(deltasonly)
        {
            bool hasdelta = false;

            for(int i = 0; i < int(deltaslist.size()); i++)
            {
                if(deltaslist[i] == configdatum) hasdelta = true;
            }

            if(!hasdelta)
            {
                configdatum = configdatum->NextSiblingElement();
                continue;
            }
        }


        ofile << "   <tr>" << std::endl;
        ofile << "      <th style=\"text-align:left\">" << configdatum->Attribute("name") << "</th>" << std::endl;
        std::string cdtype = std::string(configdatum->Attribute("type"));

        XMLElement *child = configdatum->FirstChildElement();

        std::string strdefault = "N\\A";
        std::string strcurrent = "N\\A";
        std::string strshortdesc = "N\\A";
        std::string strmin = "N\\A";
        std::string strmax = "N\\A";

        while(child != NULL)
        {
            //std::cout << "child->value = " << child->Value() << std::endl;
            if( std::string(child->Value()) == "Current")
            {
                strcurrent = child->GetText();

                //format floats for readability
                if(!rawfloats)
                {
                    if(cdtype == "float32")
                    {
                        float num = atof(strcurrent.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strcurrent = fstring.str();
                    }
                    else if(cdtype == "float64")
                    {
                        double num = atol(strcurrent.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strcurrent = fstring.str();
                    }
                }
            }
            else if( std::string(child->Value()) == "Default")
            {
                strdefault = child->GetText();

                //format floats for readability
                if(!rawfloats)
                {
                    if(cdtype == "float32")
                    {
                        float num = atof(strdefault.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strdefault = fstring.str();
                    }
                    else if(cdtype == "float64")
                    {
                        double num = atol(strdefault.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strdefault = fstring.str();
                    }
                }

            }
            else if( std::string(child->Value()) == "MinValue")
            {
                strmin = child->GetText();

                //format floats for readability
                if(!rawfloats)
                {
                    if(cdtype == "float32")
                    {
                        float num = atof(strmin.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strmin = fstring.str();
                    }
                    else if(cdtype == "float64")
                    {
                        double num = atol(strmin.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strmin = fstring.str();
                    }
                }
            }
            else if( std::string(child->Value()) == "MaxValue")
            {
                strmax = child->GetText();

                //format floats for readability
                if(!rawfloats)
                {
                    if(cdtype == "float32")
                    {
                        float num = atof(strmax.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strmax = fstring.str();
                    }
                    else if(cdtype == "float64")
                    {
                        double num = atol(strmax.c_str());
                        std::stringstream fstring;
                        fstring << num;
                        strmax = fstring.str();
                    }
                }
            }

            else if( std::string(child->Value()) == "ShortDesc") strshortdesc = child->GetText();


            child = child->NextSiblingElement();
        }

        //if current != default
        if(strcurrent != strdefault) ofile << "      <td bgcolor=\"#ffff00\">" << strcurrent << "</td>" << std::endl;
        else ofile << "      <td>" << strcurrent << "</td>" << std::endl;
        ofile << "      <td>" << strdefault << "</td>" << std::endl;
        ofile << "      <td>" << strmin << "</td>" << std::endl;
        ofile << "      <td>" << strmax << "</td>" << std::endl;
        ofile << "      <td>" << strshortdesc << "</td>" << std::endl;
        ofile << "   </tr>" << std::endl;

        configdatum = configdatum->NextSiblingElement();
    }

    //terminate html
    ofile << "    </table>" << std::endl;


    return true;
}

void addFolders(folder tfolder)
{
    std::vector<std::string> foundfolders = getFolders(tfolder.path, true);

    tfolder.files = getFiles(tfolder.path, ".xml");

    //parse each file to html
    for(int i = 0; i < int(tfolder.files.size()); i++)
    {

        if(tfolder.files[i] == "Manager.xml")
        {
            std::cout << "Ignoring manager.xml\n";
        }
        else
        {
            std::cout << tfolder.path << tfolder.files[i] << std::endl;
            parseXML(std::string(tfolder.path + tfolder.files[i]));
            parsecounter++;
        }

    }

    for(int i = 0; i < int(foundfolders.size()); i++)
    {
        folder newfolder;
        newfolder.path = std::string( tfolder.path + foundfolders[i] + "\\");
        //std::cout << "Adding folder:" << newfolder.path << std::endl;
        tfolder.folders.push_back(newfolder);
        addFolders(tfolder.folders.back());
    }
}

int main(int argc, char *argv[])
{
    std::cout << versionstring << std::endl;
    //fastrun -n param bypasses the serial number entry
    bool fastrun = false;


    for(int i = 0; i < argc; i++)
    {
        if(std::string(argv[i]) == "-f") fastrun = true;
        else if(std::string(argv[i]) == "-r") rawfloats = true;
        else if(std::string(argv[i]) == "-d") deltasonly = true;
        else if(std::string(argv[i]) == "-?" || std::string(argv[i]) == "-h")
        {
            std::cout << "Usage:\n";
            std::cout << "-f : Fast run parameter, does not query user for serial number\n";
            std::cout << "-d : Only record elements that have deltas between default and current\n";
            std::cout << "-r : Write floats as raw values without conversion\n";
            std::cout << "-h : Help\n\n";
            return 0;
        }
    }

    std::vector<std::string> dirs = getFolders(".\\", true);


    //find registry folder
    bool foundregistry = false;
    for(int i = 0; i < int(dirs.size()); i++)
    {
        if(dirs[i] == "Registry")
        {
            foundregistry = true;
        }
    }
    if(!foundregistry)
    {
        std::cout << "Error: Registry folder not found.\n";
        return 0;
    }

    //save html file as system serial number
    if(!fastrun)
    {
        std::string buf;
        std::cout << "Enter system serial number:";
        std::getline(std::cin, buf);
        if(buf == "") buf = defaultfile;
        else defaultfile = buf;
    }

    ofile.open(std::string(defaultfile + ".html").c_str());

    //create sn header
    ofile << "<html>" << std::endl;
    ofile << "<body>" << std::endl;
    ofile << "<h2>" << defaultfile << "</h2>" << std::endl;

    //initialize registry folder file structure
    //parse all xml files to html format
    RegistryFolder.path = ".\\Registry\\";
    addFolders(RegistryFolder);


    //terminate html
    ofile << "</body>" << std::endl;
    ofile << "</html>" << std::endl;

    std::cout << "\n\nParsed " << parsecounter << " files to " << defaultfile + ".html\n";


    ofile.close();


    return 0;
}
