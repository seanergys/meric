#ifndef SRC_BASIS_SYSFSUTILS_H_
#define SRC_BASIS_SYSFSUTILS_H_

#include <dirent.h>
#include <sys/types.h>
#include <fts.h>
#include <string.h>

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

namespace meric {
namespace sysfsutils {

template<typename F>
bool IterateDir(const std::string &path, F &&f)
{
	DIR *pDir;
	if((pDir = opendir(path.c_str())) == NULL)
	{
		return false;
	}
	
	dirent *pDirEnt;
	while((pDirEnt = readdir(pDir)) != NULL)
	{
		f(pDirEnt->d_name);
	}
	closedir(pDir);
	return true;
}

template<typename F>
void ScanTree(const std::string &path, int maxDepth, F &&f)
{
	char *rootPaths[] = { const_cast<char *>(path.c_str()), NULL };

	FTS *pFs = fts_open(rootPaths, FTS_NOCHDIR | FTS_LOGICAL, 
		+[](const FTSENT** one, const FTSENT** two) { 
			return strcmp((*one)->fts_name, (*two)->fts_name); 
		});
	
	if(!pFs)
		return;

	FTSENT *pNode = nullptr;
	while((pNode = fts_read(pFs)) != nullptr)
	{
		if(pNode->fts_level > maxDepth)
		{
			fts_set(pFs, pNode, FTS_SKIP);
		}
		else if(pNode->fts_info == FTS_F)
		{
			f(std::string(pNode->fts_name), std::string(pNode->fts_path));
		}
	}

	fts_close(pFs);
}

template<typename T>
bool ReadValue(std::ifstream &fs, T &out)
{
	if(!fs.is_open())
	{
		return false;
	}
	fs.seekg(0);
	fs >> out;
	return true;
};

template<>
inline bool ReadValue(std::ifstream &fs, std::string &out)
{
	if(!fs.is_open())
	{
		return false;
	}
	fs.seekg(0);
	std::getline(fs, out);
	return true;
};

template<typename T>
bool ReadValue(const std::string &filename, T &out)
{
	std::ifstream fs(filename);
	if(!fs.is_open())
	{
		return false;
	}
	return ReadValue(fs, out);
};

template<typename T>
T ParseString(const std::string &str)
{
	T value = T();
	std::istringstream ss(str);
	ss >> value;
	return value;
}

std::string MakeCounterName(const std::string &name, 
	std::map<const std::string, std::pair<std::string, std::string> > &counterLabels,
	const std::vector<std::pair<int, int>> &nums = {}, 
	bool isLabel = false);

std::string MakeCounterName(const std::string &name, 
	const std::string &prefix = "", const std::string &suffix = "",
	const std::vector<std::pair<int, int>> &nums = {});

std::string &RTrim(std::string &s, const char *t);
std::string &LTrim(std::string &s, const char *t);
std::string &Trim(std::string &s, const char *t = " \t\n\r\f\v");
std::string Trim(const std::string &s, const char *t = " \t\n\r\f\v");
std::string NormalizeLabel(const std::string &str);

}
}

#endif //SRC_BASIS_SYSFSUTILS_H_
