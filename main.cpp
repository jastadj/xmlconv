#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include "tinyxml2.h"
#include <vector>

#include <dirent.h> // for directory / files
#include <sys/types.h>
#include <sys/stat.h>

using namespace tinyxml2;

//html target file out stream
std::ofstream ofile;
int parsecounter = 0;

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

    //std::cout << root->Value() << std::endl;

    ofile << "<html>" << std::endl;
    ofile << "<body>" << std::endl;
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

        ofile << "   <tr>" << std::endl;
        ofile << "      <th style=\"text-align:left\">" << configdatum->Attribute("name") << "</th>" << std::endl;

        XMLElement *child = configdatum->FirstChildElement();

        std::string strdefault = "N\\A";
        std::string strcurrent = "N\\A";
        std::string strshortdesc = "N\\A";
        std::string strmin = "N\\A";
        std::string strmax = "N\\A";

        while(child != NULL)
        {
            //std::cout << "child->value = " << child->Value() << std::endl;
            if( std::string(child->Value()) == "Current") strcurrent = child->GetText();
            else if( std::string(child->Value()) == "Default") strdefault = child->GetText();
            else if( std::string(child->Value()) == "MinValue") strmin = child->GetText();
            else if( std::string(child->Value()) == "MaxValue") strmax = child->GetText();
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
    ofile << "</body>" << std::endl;
    ofile << "</html>" << std::endl;

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

    std::vector<std::string> dirs = getFolders(".\\", true);
    std::string fileoutput = "default.html";

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
        std::cout << "Unable to find Registry folder in root directory...\n";
        return 0;
    }

    //save html file as system serial number
    std::string buf;
    std::cout << "Enter system serial number:";
    std::getline(std::cin, buf);
    if(buf != "")
    {
        ofile.open(std::string(buf + ".html").c_str());
    }

    //initialize registry folder file structure
    //parse all xml files to html format
    RegistryFolder.path = ".\\Registry\\";
    addFolders(RegistryFolder);

    std::cout << "\n\nParsed " << parsecounter << " files to " << buf + ".html\n";


    ofile.close();


    return 0;
}
