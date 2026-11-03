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

void CreateDir(const std::string &dir);
bool CheckDirExists(const std::string &dir);

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
	{
		return;
	}

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

/**
 * @brief Check file access in CNF (Conjuctive Normal Form),
 *			where at least one file from each file group has to be accessible
 *			with specified access type.
 *			The basePath is prepended to all filenames mentioned in fileGroups.
 *
 *
 * @param fileGroups Groups of files to be checked.
 * @param basePath 	 Base path common to all files.
 * @return true 	 When at least one file check in *each* group succeeds.
 * @return false 	 Otherwise.
 */
bool CheckFileAccess(const std::vector<std::vector<std::pair<std::string, int>>> &fileGroups, const std::string &basePath = "");

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

/**
 * @brief Non-owning FD handle wrapper.
 *        This does neither track number of instances of the FD nor close the FD when out of scope.
 */
class WeakFdHandle
{
public:
	WeakFdHandle() : m_handle(-1) {}

	operator int() const { return m_handle; }
	operator bool() const { return IsValid(); }

	bool IsValid() const { return m_handle >= 0; }

protected:
	int m_handle;
};

/**
 * @brief Unique FD handle wrapper.
 *        Closes associated FD when destroyed and therefore is non-copyable (can be moved).
 */
class UniqueFdHandle : public WeakFdHandle
{
public:
	UniqueFdHandle(int fd = -1) { m_handle = fd; }

	UniqueFdHandle(UniqueFdHandle &&other) { std::swap(m_handle, other.m_handle); }

	UniqueFdHandle &operator=(UniqueFdHandle &&other)
	{
		std::swap(m_handle, other.m_handle);
		return *this;
	}

	UniqueFdHandle(const UniqueFdHandle &other) = delete;
	UniqueFdHandle &operator=(const UniqueFdHandle &other) = delete;

	~UniqueFdHandle();
};

/**
 * @brief File Descriptor (FD) cache to manage opened FDs.
 *        This is optimized for FD associated with contiguous integer indexes (eg. CPU cores)
 *        as FDs are stored in dense array (std::vector).
 */
class FdCache
{
public:
	FdCache() {}

	FdCache(FdCache &&other);
	FdCache &operator=(FdCache &other);

	FdCache(const FdCache &) = delete;
	FdCache &operator=(const FdCache &) = delete;

	/**
	 * @brief Uses functor `op` to open FD or returns already opened FD associated
	 *        with index `id`.
	 *        NOTE: `op` is not called at all when FD is already opened.
	 *              That means Open(...) can be safely called every time, even when `op` contains expensive code
	 *              and the code in `op` will be executed only once.
	 *        NOTE: This is not thread-safe.
	 *
	 * @tparam F Type of functor to open the FD
	 * @param id ID associated with the FD
	 * @param op Functor to open the FD (if not already opened)
	 * @return WeakFdHandle Returns weak handle containing the FD
	 */
	template <typename F> WeakFdHandle Open(int id, F &&op)
	{
		if (id < m_fds.size() && m_fds[id])
		{
			return m_fds[id];
		}

		m_fds.resize(std::max(m_fds.size(), size_t(id + 1)));
		m_fds[id] = UniqueFdHandle(op());

		return m_fds[id];
	}

	/**
	 * @brief Returns weak FD handle for FD associated with `id`.
	 *
	 * @param id ID associated with cached FD
	 * @return WeakFdHandle Returns weak handle containing the FD or invalid handle.
	 */
	WeakFdHandle Get(int id) const
	{
		if (id >= m_fds.size())
		{
			return WeakFdHandle();
		}
		return m_fds[id];
	}

	const std::vector<UniqueFdHandle> &GetAll() const { return m_fds; }

protected:
	std::vector<UniqueFdHandle> m_fds;
};
}
}

#endif //SRC_BASIS_SYSFSUTILS_H_
