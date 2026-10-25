#include "e4wrapper.h"
#include "environmentwrapper.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>

using namespace meric;

#if HAVE_E4_PDU

#define DCPOWER_PATH "/opt/share/scripts/powerdiscovery/dcpower"

static const char *POWER_OID_DEFAULT = ".1.3.6.1.4.1.534.6.6.7.6.5.1.3.0";
static const char *VA_OID_DEFAULT = ".1.3.6.1.4.1.534.6.6.7.6.5.1.2.0";

static std::string g_powerOid = POWER_OID_DEFAULT;
static std::string g_vaOid = VA_OID_DEFAULT;

static inline std::string trim_copy(std::string s)
{
	const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
	while (!s.empty() && isSpace(static_cast<unsigned char>(s.front())))
		s.erase(s.begin());
	while (!s.empty() && isSpace(static_cast<unsigned char>(s.back())))
		s.pop_back();
	return s;
}

static inline void normalize_oid(std::string &oid)
{
	oid = trim_copy(oid);
	if (!oid.empty() && oid.front() != '.')
		oid.insert(oid.begin(), '.');
}

static bool parse_python_string_assignment(const std::string &line, const std::string &name, std::string &outValue)
{
	std::string s = trim_copy(line);
	if (s.rfind(name, 0) != 0)
		return false;

	const auto eqPos = s.find('=');
	if (eqPos == std::string::npos)
		return false;

	std::string rhs = trim_copy(s.substr(eqPos + 1));
	if (rhs.empty())
		return false;

	const char quote = rhs.front();
	if (quote != '\'' && quote != '"')
		return false;

	const auto endQuote = rhs.find(quote, 1);
	if (endQuote == std::string::npos)
		return false;

	outValue = rhs.substr(1, endQuote - 1);
	outValue = trim_copy(outValue);
	return !outValue.empty();
}

static void load_oids_from_dcpower_if_present()
{
	static bool s_done = false;
	if (s_done)
		return;
	s_done = true;

	constexpr const char *kDcPowerPath = DCPOWER_PATH;
	std::ifstream in(kDcPowerPath);
	if (!in.is_open())
		return;

	std::string line;
	std::string power;
	std::string va;
	while (std::getline(in, line))
	{
		const auto commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line.resize(commentPos);

		parse_python_string_assignment(line, "POWER_OID", power);
		parse_python_string_assignment(line, "VA_OID", va);
		if (!power.empty() && !va.empty())
			break;
	}
	// std::cout << "Parsed from dcpower: POWER_OID='" << power << "', VA_OID='" << va << "'" << std::endl;

	if (!power.empty())
	{
		g_powerOid = power;
		normalize_oid(g_powerOid);
	}
	if (!va.empty())
	{
		g_vaOid = va;
		normalize_oid(g_vaOid);
	}
}

using PortsByIp = std::unordered_map<std::string, std::vector<int>>;
static const std::unordered_map<std::string, PortsByIp> DATACENTER_DEFAULT = {
	// ===============[RACK 4]==============================
	{"rcnode01", {{"172.18.19.121", {3}}, {"172.18.19.120", {3}}}},
	{"rcnode02", {{"172.18.19.121", {2}}, {"172.18.19.120", {2}}}},
	{"ngnode03", {{"172.18.19.121", {5}}, {"172.18.19.120", {5}}}},
	{"tcnode13", {{"172.18.19.121", {6}}, {"172.18.19.120", {6}}}},
	{"isnode04", {{"172.18.19.121", {7}}, {"172.18.19.120", {7}}}},
	{"tcnode14", {{"172.18.19.121", {9}}, {"172.18.19.120", {9}}}},
	{"ncnode01", {{"172.18.19.121", {11, 13}}}},
	{"ngnode02", {{"172.18.19.121", {14, 15}}}},
	{"ncnode02", {{"172.18.19.120", {14, 25}}}},
	// ====================[RACK 5]==========================
	{"asnode06", {{"172.18.19.116", {1}}, {"172.18.19.115", {1}}}},
	{"acnode12", {{"172.18.19.116", {2}}, {"172.18.19.115", {2}}}},
	{"agnode07", {{"172.18.19.115", {3, 10}}}},
	{"acnode11", {{"172.18.19.116", {3}}, {"172.18.19.115", {7}}}},
	{"acnode10", {{"172.18.19.116", {5}}, {"172.18.19.115", {5}}}},
	{"isolde", {{"172.18.19.116", {7, 10}}}},
	{"iwnode0508", {{"172.18.19.116", {8}}, {"172.18.19.115", {8}}}},
	{"attract01", {{"172.18.19.116", {9, 11, 15}}, {"172.18.19.115", {9}}}},
	{"awnode0911", {{"172.18.19.115", {11, 15}}}},
	{"icnode05", {{"172.18.19.116", {12}}, {"172.18.19.115", {12}}}},
	{"acnode04", {{"172.18.19.116", {13}}, {"172.18.19.115", {13}}}},
	{"acnode03", {{"172.18.19.116", {14}}, {"172.18.19.115", {14}}}},
	{"icnode04", {{"172.18.19.116", {16}}, {"172.18.19.115", {16}}}},
	{"awnode0508", {{"172.18.19.116", {17}}, {"172.18.19.115", {17}}}},
	{"qemu01", {{"172.18.19.116", {18}}, {"172.18.19.115", {18}}}},
};

static std::unordered_map<std::string, PortsByIp> g_datacenter = DATACENTER_DEFAULT;

static bool parse_ports_list(const std::string &s, std::vector<int> &ports)
{
	const auto l = s.find('[');
	const auto r = s.find(']', (l == std::string::npos) ? 0 : (l + 1));
	if (l == std::string::npos || r == std::string::npos || r <= l + 1)
		return false;

	std::string inside = s.substr(l + 1, r - (l + 1));
	ports.clear();

	std::string tok;
	for (size_t i = 0; i <= inside.size(); ++i)
	{
		const char ch = (i < inside.size()) ? inside[i] : ',';
		if (ch == ',')
		{
			tok = trim_copy(tok);
			if (!tok.empty())
			{
				char *endPtr = nullptr;
				const long v = std::strtol(tok.c_str(), &endPtr, 10);
				if (endPtr && endPtr != tok.c_str())
					ports.push_back(static_cast<int>(v));
			}
			tok.clear();
		}
		else
		{
			tok.push_back(ch);
		}
	}

	return !ports.empty();
}

static bool parse_quoted_key(const std::string &line, std::string &outKey)
{
	const auto firstQuote = line.find('"');
	if (firstQuote == std::string::npos)
		return false;
	const auto secondQuote = line.find('"', firstQuote + 1);
	if (secondQuote == std::string::npos || secondQuote <= firstQuote + 1)
		return false;
	outKey = line.substr(firstQuote + 1, secondQuote - (firstQuote + 1));
	outKey = trim_copy(outKey);
	return !outKey.empty();
}

static void load_datacenter_from_dcpower_if_present()
{
	static bool s_done = false;
	if (s_done)
		return;
	s_done = true;

	constexpr const char *kDcPowerPath = DCPOWER_PATH;
	std::ifstream in(kDcPowerPath);
	if (!in.is_open())
		return;

	std::unordered_map<std::string, PortsByIp> parsed;
	bool inDatacenter = false;
	int datacenterDepth = 0;

	std::string currentNode;
	int nodeDepth = 0;
	bool inIps = false;
	int ipsDepth = 0;

	std::string line;
	while (std::getline(in, line))
	{
		const auto commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line.resize(commentPos);
		line = trim_copy(line);
		if (line.empty())
			continue;

		const int openBraces = static_cast<int>(std::count(line.begin(), line.end(), '{'));
		const int closeBraces = static_cast<int>(std::count(line.begin(), line.end(), '}'));
		const int delta = openBraces - closeBraces;

		if (!inDatacenter)
		{
			if (line.find("datacenter") != std::string::npos && line.find('{') != std::string::npos)
			{
				inDatacenter = true;
				datacenterDepth = delta;
			}
			continue;
		}

		// Node start: "name": {
		if (currentNode.empty())
		{
			if (line.find('"') != std::string::npos && line.find(":") != std::string::npos && line.find('{') != std::string::npos)
			{
				std::string maybeNode;
				if (parse_quoted_key(line, maybeNode) && maybeNode != "ips")
				{
					currentNode = maybeNode;
					parsed[currentNode] = {};
					nodeDepth = delta;
					inIps = false;
					ipsDepth = 0;
				}
			}
		}
		else
		{
			nodeDepth += delta;

			if (!inIps)
			{
				if (line.find("\"ips\"") != std::string::npos && line.find('{') != std::string::npos)
				{
					inIps = true;
					ipsDepth = delta;
				}
			}
			else
			{
				ipsDepth += delta;

				// IP entry: "1.2.3.4": [1,2],
				if (line.find('"') != std::string::npos && line.find('[') != std::string::npos)
				{
					std::string ip;
					if (parse_quoted_key(line, ip))
					{
						std::vector<int> ports;
						if (parse_ports_list(line, ports))
							parsed[currentNode][ip] = std::move(ports);
					}
				}

				if (ipsDepth <= 0)
				{
					inIps = false;
					ipsDepth = 0;
				}
			}

			if (nodeDepth <= 0)
			{
				currentNode.clear();
				nodeDepth = 0;
				inIps = false;
				ipsDepth = 0;
			}
		}

		datacenterDepth += delta;
		if (datacenterDepth <= 0)
			break;
	}

	// Only replace the built-in mapping if parsing produced something usable.
	if (!parsed.empty())
		g_datacenter = std::move(parsed);
}

static double snmp_get_value(const std::string &ip, const std::string &oid_str)
{
	struct snmp_session session, *ss;
	struct snmp_pdu *pdu;
	struct snmp_pdu *response;
	oid anOID[MAX_OID_LEN];
	size_t anOID_len = MAX_OID_LEN;
	struct variable_list *vars;
	int status;
	double result = 0.0;

	init_snmp("e4wrapper");

	snmp_sess_init(&session);
	session.peername = strdup(ip.c_str());
	session.version = SNMP_VERSION_1;
	session.community = (u_char *)"public";
	session.community_len = strlen((char *)session.community);

	SOCK_STARTUP;
	ss = snmp_open(&session);
	if (!ss)
	{
		snmp_perror("snmp_open");
		SOCK_CLEANUP;
		free(session.peername);
		return 0.0;
	}

	pdu = snmp_pdu_create(SNMP_MSG_GET);
	if (!read_objid(oid_str.c_str(), anOID, &anOID_len))
	{
		snmp_perror("read_objid");
		snmp_close(ss);
		SOCK_CLEANUP;
		free(session.peername);
		return 0.0;
	}

	snmp_add_null_var(pdu, anOID, anOID_len);
	status = snmp_synch_response(ss, pdu, &response);

	if (status == STAT_SUCCESS && response && response->errstat == SNMP_ERR_NOERROR)
	{
		for (vars = response->variables; vars; vars = vars->next_variable)
		{
			if (vars->type == ASN_INTEGER)
				result = (double)(*vars->val.integer);
			else if (vars->type == ASN_GAUGE || vars->type == ASN_COUNTER || vars->type == ASN_TIMETICKS || vars->type == ASN_UINTEGER)
				result = (double)(*vars->val.integer);
			else if (vars->type == ASN_COUNTER64)
				result = (double)(vars->val.counter64->high * static_cast<double>(1ULL << 32) + vars->val.counter64->low);
		}
	}
	else
	{
		if (status == STAT_TIMEOUT)
		{
			std::cerr << "MERIC ERROR: SNMP timeout, no response from " << ip << std::endl;
		}
		else if (status == STAT_SUCCESS && response)
		{
			std::cerr << "MERIC ERROR: SNMP error in packet: " << snmp_errstring(response->errstat) << std::endl;
		}
		else
		{
			std::cerr << "MERIC ERROR: SNMP session error" << std::endl;
		}
	}

	if (response)
		snmp_free_pdu(response);
	snmp_close(ss);
	SOCK_CLEANUP;
	free(session.peername);

	return result;
}

#endif

const std::string E4::SUM_ENERGY_LABEL;
int E4::PROBE_NUMBER;
std::vector<std::string> E4::vrSensorLabel;

std::vector<E4::Sampler_t *> E4::samplersById;
std::map<std::string, E4::Sampler_t> E4::samplers;
std::string E4::powerDevice;

std::chrono::nanoseconds E4::s_defaultSamplePeriod(std::chrono::seconds(1));
std::chrono::nanoseconds E4::s_minSamplePeriod(std::chrono::seconds(1));
std::chrono::nanoseconds E4::s_currentSamplePeriod(0);
std::atomic<bool> E4::s_sampleStoreEnabled(false);
AsyncSampler<E4::CounterRecord_t> E4::s_asyncSampler;

double E4::getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int &cumulative, std::string counter,
						  std::string regionName)
{
#if HAVE_E4_PDU
	(void)runtime;
	(void)counter;
	(void)regionName;
	// E4 counter is cumulative energy (J).
	cumulative = 1;
	return double(stopValue - startValue);
#endif // HAVE_E4_PDU
	return 0.0;
}

void E4::init(std::string label, bool detailed, double sampleFreqHz)
{
#if HAVE_E4_PDU
	powerDevice = label;

	load_oids_from_dcpower_if_present();
	load_datacenter_from_dcpower_if_present();

	if (g_datacenter.find(powerDevice) == g_datacenter.end())
	{
		std::cerr << "[E4] Node '" << powerDevice << "' not found in datacenter mapping (see dcpower); power will be 0." << std::endl;
	}

	s_currentSamplePeriod = s_defaultSamplePeriod;

	if (sampleFreqHz > 0.0)
	{
		s_currentSamplePeriod = std::chrono::nanoseconds(uint64_t(1.0e9 / sampleFreqHz));
	}

	samplers[powerDevice] = {powerDevice, 0.0, {}};
	initSamplers(detailed);
#endif // HAVE_E4_PDU
}

void E4::close()
{
#if HAVE_E4_PDU
	closeSamplers();
#endif // HAVE_E4_PDU
}

void E4::stop()
{
#if HAVE_E4_PDU
	s_sampleStoreEnabled.store(false);
#endif // HAVE_E4_PDU
}

void E4::start(timespec &time, bool storeSamples, double sampleFreqHz)
{
#if HAVE_E4_PDU
	if (sampleFreqHz > 0.0)
	{
		s_currentSamplePeriod = std::max(s_minSamplePeriod, std::chrono::nanoseconds(uint64_t(1.0e9 / sampleFreqHz)));
		s_asyncSampler.SetPeriod(s_currentSamplePeriod);
	}

	s_sampleStoreEnabled.store(storeSamples);
#endif // HAVE_E4_PDU
}

void E4::getCounterValues(std::map<std::string, unsigned long long> &record)
{
#if HAVE_E4_PDU
	s_asyncSampler.Sample(&record);
#else
	(void)record;
#endif // HAVE_E4_PDU
}

void E4::initSamplers(bool detailed)
{
	(void)detailed;
	samplersById.clear();
	vrSensorLabel.clear();

	for (auto &sampler : samplers)
		samplersById.push_back(&sampler.second);

	std::sort(samplersById.begin(), samplersById.end(), [](const Sampler_t *pLeft, const Sampler_t *pRight) { return pLeft->name < pRight->name; });

	PROBE_NUMBER = samplersById.size();
	for (Sampler_t *pSampler : samplersById)
	{
		vrSensorLabel.push_back(pSampler ? pSampler->name : "");
	}

#if HAVE_E4_PDU
	const std::string nodeName = E4::powerDevice;
	const auto mappingIt = g_datacenter.find(nodeName);
	const PortsByIp *portsByIp = (mappingIt != g_datacenter.end()) ? &mappingIt->second : nullptr;

	s_asyncSampler.Start(s_currentSamplePeriod,
						 [portsByIp, nodeName](bool isSampling, CounterRecord_t *pRequest)
						 {
							 timespec now;
							 clock_gettime(CLOCK_REALTIME, &now);

							 const uint64_t timestamp = uint64_t(now.tv_sec) * 1000000000ull + now.tv_nsec;

							 if (isSampling)
							 {
								 double totalpower = 0.0;
								 if (portsByIp)
								 {
									 for (const auto &ipEntry : *portsByIp)
									 {
										 const std::string &ip = ipEntry.first;
										 for (int port : ipEntry.second)
										 {
											 const std::string oidPower = g_powerOid + "." + std::to_string(port);
											 totalpower += snmp_get_value(ip, oidPower);
											 (void)g_vaOid;
										 }
									 }
								 }

								 // Store instantaneous power samples (W) and integrate energy (J).
								 samplers[nodeName].totalpower += totalpower * (double(s_currentSamplePeriod.count() * 1.0e-9));

								 if (s_sampleStoreEnabled)
								 {
									 samplers[nodeName].samples.push_back({timestamp, totalpower});
								 }
							 }

							 if (!pRequest)
								 return;

							 for (const auto &entry : samplers)
							 {
								 const std::string &title = entry.first;
								 const double &value = entry.second.totalpower;
								 (*pRequest)[title] = static_cast<unsigned long long>(value);
							 }
						 });
#endif // HAVE_E4_PDU
}

void E4::closeSamplers()
{
#if HAVE_E4_PDU
	s_asyncSampler.Stop();
#endif // HAVE_E4_PDU
}

int E4::getSamples() { return 0; }

void E4::getSamplesList(size_t sensorId, size_t first, size_t last, std::vector<double> &samples)
{
	if (sensorId >= samplersById.size() || !samplersById[sensorId])
		return;

	const Sampler_t &desc = *samplersById[sensorId];
	for (size_t i = first; i < last; ++i)
	{
		samples.push_back(double(desc.samples[i].value) * getUnit(desc.name));
	}
}

std::string E4::getData(double startTime, double duration, SamplerRecord_t &record)
{
	uint64_t intervalStart = uint64_t(startTime * 1.0e9);
	uint64_t intervalEnd = intervalStart + uint64_t(duration * 1.0e9);

	SourceInfo_t sourceInfo;
	sourceInfo.domain = SourceInfo_t::DOMAIN_PER_NODE;
	sourceInfo.type = SourceInfo_t::TYPE_POWER;
	sourceInfo.flags = SourceInfo_t::FLAG_ACCUMULATIVE_BIT;

	for (size_t i = 0; i < samplersById.size(); ++i)
	{
		if (!samplersById[i])
			continue;

		record.push_back(SamplerData_t(sourceInfo));

		const Sampler_t &desc = *samplersById[i];
		SamplerData_t &data = record.back();
		data.id = i;
		data.name = desc.name;
		data.energy = 0.0;
		data.min = std::numeric_limits<double>::max();
		data.max = 0.0;

		auto startIt = std::lower_bound(desc.samples.begin(), desc.samples.end(), intervalStart,
										[](const Sample_t &sample, uint64_t value) { return sample.timestamp < value; });
		auto endIt = std::upper_bound(desc.samples.begin(), desc.samples.end(), intervalEnd,
									  [](uint64_t value, const Sample_t &sample) { return value < sample.timestamp; });

		for (auto it = startIt; it != endIt; ++it)
		{
			double power = double(it->value);

			data.energy += power;
			data.min = std::min(data.min, power);
			data.max = std::max(data.max, power);
		}

		data.first = std::distance(desc.samples.begin(), startIt);
		data.last = std::distance(desc.samples.begin(), endIt);

		if (data.min > data.max)
			data.min = data.max;
	}

	return "";
}
