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


#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

//SHARED
static void getUniqueFileName(char *fileName);
static bool getLock();

//JIT

//PROBED
static void wrap_func_pre(void *wrapcxt, OUT void **user_data);
static void wrap_func_post(void *wrapcxt, void *user_data);
////////////////////////////////////////////////////////////////////////////////

//TODO make a prototypes cleaninig 
static void event_exit(void);
static void wrap_malloc_pre(void *wrapcxt, OUT void **user_data);
static void wrap_malloc_post(void *wrapcxt, void *user_data);
static void wrap_free_pre(void *wrapcxt, OUT void **user_data);

static void print_address(file_t f, app_pc addr, const char *prefix);
static void add_call(app_pc instr_addr, app_pc target_addr);
static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data);

static void addFuncToList(app_pc addr);
static void getFunctionName(app_pc addr, std::string* funcName);

static void print_address_origin(app_pc addr);

#define MAX_SYM_RESULT 256
#define DURATION_THRASHOLD 100000 //100 miliseconds

std::ofstream LogFile;
enum INSTUMENT_MODE {FUNCTION_DETECTION, FUNCTION_RUNTIME_MEASUREMENT};
int APPLICATION_MODE;

long GLOBAL_TMP;


////////////////////////////////////////////////////////////////////////////////
//uint64 timestamp; //TODO remove

std::set<std::string> funcFilterList;
const std::string FILTERFILE = "DRIO_FILTER";

// Holds instruction count for a single procedure
typedef struct RtnDesc
{
	std::string name;
//	std::string image;
//	std::string path;
//	long address;
	uint64 lastStart;	//time
	uint64 maxDuration;	//runtime
	uint64 minDuration;	//runtime
//	struct RtnDesc * _next;
} RTN_DESC;

std::map<std::string, RtnDesc> funcPROBEDList;
std::map <std::string, int> funcJITList;
uint64 startTime;

//----------------------------------------------------------------------------//
inline std::string getenv(const std::string &param) //, std::string defaultValue)
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
		return "";//defaultValue;
	}
}

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
	/* Get hostname */
	int buffSize = 256;
	char hostName[buffSize];
	gethostname(hostName, buffSize);

	fileName[0] = '\0';
	strcat(fileName, "uniquefile-");
	strcat(fileName, hostName);
	strcat(fileName, "-");

	/* get pid */
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

	/* create file */
	FILE *fd = fopen(fileName, "w");
	fclose(fd);

	/* create link */
	int r = link(fileName,lockName);
	if (r == 0)
		ret = true;

	return ret;
}

static void getFunctionName(app_pc addr, std::string* funcName)
{
    drsym_error_t symres;
    drsym_info_t sym;
    char name[MAX_SYM_RESULT];
    char file[MAXIMUM_PATH];
    module_data_t *data;
    data = dr_lookup_module(addr);
    if (data == NULL) {	
        dr_fprintf(STDERR, "[-] getFunctionName(): dr_lookup_module(addr) error\n"); //TODO improve the message
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
/*        const char *modname = dr_module_preferred_name(data);
        if (modname == NULL)
            modname = "<noname>";
        dr_fprintf(f, "%s "PFX" %s!%s+"PIFX, prefix, addr,
                   modname, sym.name, addr - data->start - sym.start_offs);
        if (symres == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
            dr_fprintf(f, " ??:0\n");
        } else {
            dr_fprintf(f, " %s:%"UINT64_FORMAT_CODE"+"PIFX"\n",
                       sym.file, sym.line, sym.line_offs);
        }
*/
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
static void add_call(app_pc instr_addr, app_pc target_addr)	//TODO prejmenovat
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


// PROBED MODE /////////////////////////////////////////////////////////////////

static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
//	dr_fprintf(STDERR, "[-] APP name: %s\n", dr_get_application_name());
//	dr_fprintf(STDERR, "[-] mod path: %s\n", mod->full_path);
/*
	if (strcmp(mod->full_path,dr_get_application_name())) //if we have the main module
	{
		; //TODO - depends if we want to analyze just the main module ...
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

static void wrap_func_pre(void *wrapcxt, OUT void **user_data)
{
	uint64 timestamp = dr_get_microseconds();
	app_pc addr;
	addr = drwrap_get_func(wrapcxt);
	std::string funcName = "";
	getFunctionName(addr, &funcName);

	if (!funcPROBEDList.count(funcName))
	{
		RtnDesc function;
		function.name = funcName;
		function.maxDuration = 0;
		function.minDuration = 0;
		funcPROBEDList[funcName]=function;
	}
	funcPROBEDList[funcName].lastStart = timestamp;

//	LogFile << "START "<<funcName << std::endl; //TODO
}

static void wrap_func_post(void *wrapcxt, void *user_data)
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

/*		
		LogFile << "\tSTOP "<<funcName<<" after "<<duration/1000<<" [ms]\n";	//TODO
		LogFile 
			<< funcName <<"\t"
			<< funcPROBEDList[funcName].minDuration<<"\t"
			<< funcPROBEDList[funcName].maxDuration<<"\n";
*/
	}
}

// MAIN ////////////////////////////////////////////////////////////////////////

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{

	if (getLock())
	{

		processFilterFile();
		LogFile.open("memprofile.out");

		dr_set_client_name("DynamoRIO Sample Client 'wrap'", "http://dynamorio.org/issues");	//TODO
		dr_log(NULL, LOG_ALL, 1, "Client 'wrap' initializing\n");	//TODO
		if (dr_is_notify_on())
		{
			dr_fprintf(STDERR, "[*] DBI analysis runnnig\n");
		}

		drmgr_init();
		drwrap_init();
		//DynamoRIOv7.0.0 - The drsyms API will eventually support both sideline and online use, and the parameter to drsym_init() will specify the symbol server to use for sideline use. Today only online use is supported and NULL should be passed.
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
		
		dr_register_exit_event(event_exit);
		startTime = dr_get_microseconds();
	}
}

static void event_exit(void)
{
//process the results
	uint64 duration = dr_get_microseconds()-startTime;
	
	if (APPLICATION_MODE == FUNCTION_DETECTION)
	{
		for (auto& funcRecord : funcJITList)
		{
			if (duration/funcRecord.second > DURATION_THRASHOLD)
				LogFile << funcRecord.first << std::endl; // << " has value " << funcRecord.second << std::endl;
		}
	}
	else //if (APPLICATION_MODE == FUNCTION_RUNTIME_MEASUREMENT)
	{
		for (auto& funcRecord : funcPROBEDList)
		{
			if (funcRecord.second.minDuration > DURATION_THRASHOLD)
			{
				LogFile << funcRecord.first << std::endl;
				dr_fprintf(STDERR, "[*] %d\t%d\t%s\n",funcRecord.second.minDuration/1000, funcRecord.second.maxDuration/1000, funcRecord.first.c_str());
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
	drmgr_exit();
}

////////////////////////////////////////////////////////////////////////////////
















static void print_address_origin(app_pc addr)
{
    drsym_error_t symres;
    drsym_info_t sym;
    char name[MAX_SYM_RESULT];
    char file[MAXIMUM_PATH];
    module_data_t *data;
    data = dr_lookup_module(addr);
    if (data == NULL) {
//        dr_fprintf(f, "%s "PFX" ? ??:0\n", prefix, addr);
		LogFile << "dr_lookup_module(addr) == NULL\n";
        return;
    }
    sym.struct_size = sizeof(sym);
    sym.name = name;
    sym.name_size = MAX_SYM_RESULT;
    sym.file = file;
    sym.file_size = MAXIMUM_PATH;
    symres = drsym_lookup_address(data->full_path, addr - data->start, &sym,
                                  DRSYM_DEFAULT_FLAGS);
    if (symres == DRSYM_SUCCESS || symres == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
        const char *modname = dr_module_preferred_name(data);
        if (modname == NULL)
            modname = "<noname>";
//        dr_fprintf(f, "%s "PFX" %s!%s+"PIFX, prefix, addr, modname, sym.name, addr - data->start - sym.start_offs);
		LogFile << (long)addr <<"\t"<< modname <<"\t"<< sym.name <<"\t"<< (long)addr <<"-"<<(long) data->start <<"-"<< (long)sym.start_offs<<"="<<(long)(addr - data->start - sym.start_offs) << std::endl;
/*
        if (symres == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
            dr_fprintf(f, " ??:0\n");
        } else {
            dr_fprintf(f, " %s:%"UINT64_FORMAT_CODE"+"PIFX"\n",sym.file, sym.line, sym.line_offs);
               LogFile << "TODO sym print\n";
        }
*/
    } 
//    else
//        dr_fprintf(f, "%s "PFX" ? ??:0\n", prefix, addr);
    dr_free_module_data(data);
}














