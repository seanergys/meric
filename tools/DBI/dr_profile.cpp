/*
g++ -fPIC -shared -lgcc -DLINUX -DX86_64 -I$DYNAMORIO_HOME/include -I$DYNAMORIO_HOME/ext/include mall_rio.c -L$DYNAMORIO_HOME/ext/lib64/release/ -ldrmgr -ldrwrap -o librio.so
*/

#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <string.h>
	#include <time.h>
#include <sstream>
#include <map>
#include <set>

//#define DR_FAST_IR	//http://dynamorio.org/docs/API_BT.html
#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

////////////////////////////////////////////////////////////////////////////////
static void getUniqueFileName(char *fileName);
static bool getLock();

static void wrap_func_pre(void *wrapcxt, OUT void **user_data);
static void wrap_func_post(void *wrapcxt, void *user_data);

static void event_thread_init(void *drcontext);
static int threadCount; //needed for threadId production
static int tls_idx; //stores unique threadId starting 0
static void *max_lock; //mutex
static void event_thread_exit(void *drcontext);

static void event_exit(void);
static void add_call(app_pc instr_addr, app_pc target_addr);
static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data);
static void getFunctionName(app_pc addr, std::string* funcName);

#define MAX_SYM_RESULT 256
#define DURATION_THRESHOLD 100000 //100 miliseconds

// Holds information about each function
typedef struct RtnDesc
{
	std::string name;
//	std::string image;
//	std::string path;
//	long address;
	uint64 lastStart;	//time
	uint64 maxDuration;	//runtime
	uint64 minDuration;	//runtime
} RTN_DESC;

std::map<std::string, RtnDesc> funcPROBEDList;
std::map <std::string, int> funcJITList;
uint64 startTime;
enum INSTUMENT_MODE {FUNCTION_DETECTION, FUNCTION_RUNTIME_MEASUREMENT};
int APPLICATION_MODE;
std::set<std::string> funcFilterList;
std::ofstream LogFile;
const std::string FILTERFILE = "DRIO_FILTER";

////////////////////////////////////////////////////////////////////////////////
//reads environment variable
inline std::string getenv(const std::string &param)
{
	std::string value;
	char *var = std::getenv(param.c_str());

	if (var != NULL)
	{
		std::stringstream ss(var);
		return ss.str();
	} 
	else
	{
		return "";
	}
}
//stores content of the input file - list of regions to instrument
static void processFilterFile()
{
	std::string filterFileName = getenv(FILTERFILE);
	if(filterFileName != "")
	{
		std::ifstream filterFile (filterFileName.c_str());
		std::string functionName;
		while (filterFile >> functionName)
			funcFilterList.insert(functionName);
		filterFile.close();
	}
}

//function based on Ignacio Laguna code
static void getUniqueFileName(char *fileName)
{
	int buffSize = 256;
	char hostName[buffSize];
	gethostname(hostName, buffSize);

	fileName[0] = '\0';
	strcat(fileName, "uniquefile-");
	strcat(fileName, hostName);
	strcat(fileName, "-");

	int pid = getpid();
	char pidBuff[128];
	sprintf(pidBuff, "%d", pid);
	strcat(fileName, pidBuff);
}

//based on Ignacio Laguna code
static bool getLock()
{
	bool ret = false;
	const char * lockName = "lock.f";
	char fileName[512];
	getUniqueFileName(fileName);

	// create file
	FILE *fd = fopen(fileName, "w");
	fclose(fd);

	// create link
	int r = link(fileName,lockName);
	if (r == 0)
		ret = true;

	return ret;
}

//get function name based on addres of the function
static void getFunctionName(app_pc addr, std::string* funcName)
{
    drsym_error_t symres;
    drsym_info_t sym;
    char name[MAX_SYM_RESULT];
    char file[MAXIMUM_PATH];
    module_data_t *data;
    data = dr_lookup_module(addr);
    if (data == NULL) {
        dr_fprintf(STDERR, "[-] function name not found\n");
        return;
    }
    sym.struct_size = sizeof(sym);
    sym.name = name;
    sym.name_size = MAX_SYM_RESULT;
    sym.file = file;
    sym.file_size = MAXIMUM_PATH;
	symres = drsym_lookup_address(data->full_path, addr - data->start, &sym, DRSYM_LEAVE_MANGLED);	//DRSYM_DEMANGLE_FULL; DRSYM_LEAVE_MANGLED; DRSYM_DEMANGLE
	
	if (symres == DRSYM_SUCCESS || symres == DRSYM_ERROR_LINE_NOT_AVAILABLE)
	{
		(*funcName) = std::string(sym.name);
	}
	dr_free_module_data(data);
}

// JIT /////////////////////////////////////////////////////////////////////////
//CATCH function return 
static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
                      bool for_trace, bool translating, void *user_data)
{
    if (instr_is_return(instr)) {
        dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)add_call, SPILL_SLOT_1);
    }
    return DR_EMIT_DEFAULT;
}

//on function return add the function to the list of functions
static void add_call(app_pc instr_addr, app_pc target_addr)
{
	int myId = (int)(ptr_uint_t)drmgr_get_tls_field(dr_get_current_drcontext(), tls_idx);
	if (!myId)
	{
		std::string funcName = "";
		getFunctionName(instr_addr, &funcName);
	
		if (funcName != "" && !funcJITList.count(funcName))
		{
			funcJITList[funcName] = 1;
		}
		else
		{
			funcJITList[funcName]++;
		}
	}
}


// PROBED MODE /////////////////////////////////////////////////////////////////

static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
/*
	if (strcmp(mod->full_path,dr_get_application_name())) //if we have the main module
	{
		; // depends if we want to analyze just the main module ...
	}
*/	
	app_pc towrap;
	for (std::string funcName : funcFilterList)
	{
		towrap = (app_pc)dr_get_proc_address(mod->handle, funcName.c_str());
		if (towrap != NULL)
		{
			bool ok = drwrap_wrap(towrap, wrap_func_pre, wrap_func_post);
			if (!ok)
			{
				dr_fprintf(STDERR, "[-] Could not wrap '%s': might be already wrapped\n",funcName.c_str());
			}
		}
	}
}

//start time measurement befor the functions starts
static void wrap_func_pre(void *wrapcxt, OUT void **user_data)
{
	int myId = (int)(ptr_uint_t)drmgr_get_tls_field(dr_get_current_drcontext(), tls_idx);
	if (!myId)
	{
		uint64 timestamp = dr_get_microseconds();
		app_pc addr;
		addr = drwrap_get_func(wrapcxt);
		std::string funcName = "";
		getFunctionName(addr, &funcName);
//		dr_fprintf(STDERR, "[-] %s START\n",funcName.c_str());

		if (!funcPROBEDList.count(funcName))
		{
			RtnDesc function;
			function.name = funcName;
			function.maxDuration = 0;
			function.minDuration = 0;
			funcPROBEDList[funcName]=function;
		}
		funcPROBEDList[funcName].lastStart = timestamp;
	}
}

//stop time measurement when the function stops
static void wrap_func_post(void *wrapcxt, void *user_data)
{
	int myId = (int)(ptr_uint_t)drmgr_get_tls_field(dr_get_current_drcontext(), tls_idx);
	if (!myId)
	{
		app_pc addr;
		addr = drwrap_get_func(wrapcxt);
		std::string funcName = "";
		getFunctionName(addr, &funcName);
	
		if (funcPROBEDList.count(funcName) && funcPROBEDList[funcName].lastStart != 0)
		{
			uint64 duration = dr_get_microseconds()-funcPROBEDList[funcName].lastStart;

			if (duration > (funcPROBEDList[funcName].maxDuration))
				funcPROBEDList[funcName].maxDuration = duration;
			if (duration < funcPROBEDList[funcName].minDuration || funcPROBEDList[funcName].minDuration == 0)
				funcPROBEDList[funcName].minDuration = duration;
			funcPROBEDList[funcName].lastStart=0;
//			dr_fprintf(STDERR, "[-] %s STOP in %f [ms]\n",funcName.c_str(), duration/1000);
		}
	}
}

// MAIN ////////////////////////////////////////////////////////////////////////
static void event_thread_init(void *drcontext)
{
	int myId = -1;
	dr_mutex_lock(max_lock);
		myId = threadCount++;
	dr_mutex_unlock(max_lock);
	drmgr_set_tls_field(drcontext, tls_idx, (void *)(ptr_uint_t)myId);
}

static void event_thread_exit(void *drcontext)
{
	dr_mutex_lock(max_lock);
		threadCount--;
	dr_mutex_unlock(max_lock);
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{

	if (getLock())
	{
		threadCount = 0;
		processFilterFile();
		LogFile.open("memprofile.out");

		dr_set_client_name("DynamoRIO tool for application profiling 'dr_profile'", "https://code.it4i.cz/vys0053/meric/");
		dr_log(NULL, LOG_ALL, 1, "Tool 'dr_profile' initializing\n");
		if (dr_is_notify_on())
		{
			dr_fprintf(STDERR, "[*] DBI analysis runnnig\n");
		}

		drmgr_init();
		drwrap_init();
		if (drsym_init(0) != DRSYM_SUCCESS)
		{
			dr_log(NULL, LOG_ALL, 1, "WARNING: unable to initialize symbol translation\n");
		}



		if (funcFilterList.empty())
		{	//catch all the functions in the application
			dr_fprintf(STDERR, "[*] Profiling all the application functions\n");
			APPLICATION_MODE = FUNCTION_DETECTION;
			drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL);
		}
		else
		{	//instrument only selected functions
			dr_fprintf(STDERR, "[*] Profiling selected application functions - %d\n",funcFilterList.size());
			APPLICATION_MODE = FUNCTION_RUNTIME_MEASUREMENT;
			drmgr_register_module_load_event(module_load_event);
		}

		max_lock = dr_mutex_create();
	    drmgr_register_thread_init_event(event_thread_init);
	    drmgr_register_thread_exit_event(event_thread_exit);
	    
		tls_idx = drmgr_register_tls_field();
		DR_ASSERT(tls_idx > -1);
				
		dr_register_exit_event(event_exit);
		startTime = dr_get_microseconds();
	}
}

//at the end of the application run process the results
static void event_exit(void)
{
	uint64 duration = dr_get_microseconds()-startTime;	
	if (APPLICATION_MODE == FUNCTION_DETECTION)
	{
		for (auto& funcRecord : funcJITList)
		{
			if (duration/funcRecord.second > DURATION_THRESHOLD)
			{
				LogFile << funcRecord.first << std::endl;
				dr_fprintf(STDERR, "[-] %s\n", funcRecord.first.c_str());
			}
		}
	}
	else //if (APPLICATION_MODE == FUNCTION_RUNTIME_MEASUREMENT)
	{
		dr_fprintf(STDERR, "[*] Writing list of significant functions - from %d\n", funcPROBEDList.size());
		for (auto& funcRecord : funcPROBEDList)
		{
			if (funcRecord.second.minDuration > DURATION_THRESHOLD)
			{
				LogFile << funcRecord.first << std::endl;
				dr_fprintf(STDERR, "[*] %f\t%f\t%s\n",funcRecord.second.minDuration/1000, funcRecord.second.maxDuration/1000, funcRecord.first.c_str());
			}
		}
	}

	
	LogFile.close();

	//finalize DynamoRIO
	if (drsym_exit() != DRSYM_SUCCESS)
	{
		dr_log(NULL, LOG_ALL, 1, "WARNING: error cleaning up symbol library\n");
	}
	
	drwrap_exit();
	drmgr_unregister_tls_field(tls_idx);
	drmgr_exit();
}


