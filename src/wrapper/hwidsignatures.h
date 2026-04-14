#ifndef SRC_WRAPPER_CPUID_SIGNATURE_VALUES_H_
#define SRC_WRAPPER_CPUID_SIGNATURE_VALUES_H_

#include <string>
#include <unordered_map>
#include "../basis/utils.h"

namespace meric
{
	/**
	 *  Intel® 64 and IA-32 Architectures Software Developer’s Manual
	 *                                      Volume 4:
	 *                                      Model-Specific Registers
	 *
	 *                                      Order Number: 335592-086US
	 *                                      December 2024
	 * Table 2-1 CPUID Signature Values of DisplayFamily_DisplayModel Mappings
	 */
	enum class CpuidSignatureValues
	{  
		INTEL_06_BDH = 87, // Intel Series 2 Core Ultra processors supporting Lunar Lake performance hybrid architecture
		INTEL_06_ADH = 86, // Intel Xeon 6 P-core processors based on Granite Rapids microarchitecture
		INTEL_06_AEH = 85, // Intel Xeon 6 P-core processors based on Granite Rapids microarchitecture
		INTEL_06_AFH = 84, // Intel Xeon 6 E-core processors based on Sierra Forest microarchitecture
		INTEL_06_AAH = 83, // Intel Core Ultra 7 processors supporting Meteor Lake performance hybrid architecture
		INTEL_06_CFH = 82, // 5th generation Intel Xeon Scalable Processor Family based on Emerald Rapids microarchitecture
		INTEL_06_8FH = 81, // 4th generation Intel Xeon Scalable Processor Family based on Sapphire Rapids microarchitecture
		INTEL_06_BAH = 80, // 13th generation Intel Core processors supporting Raptor Lake performance hybrid architecture
		INTEL_06_B7H = 79, // 13th generation Intel Core processors supporting Raptor Lake performance hybrid architecture
		INTEL_06_BFH = 78, // 13th generation Intel Core processors supporting Raptor Lake performance hybrid architecture
		INTEL_06_97H = 77, // 12th generation Intel Core processors supporting Alder Lake performance hybrid architecture
		INTEL_06_9AH = 76, // 12th generation Intel Core processors supporting Alder Lake performance hybrid architecture
		INTEL_06_8CH = 75, // 11th generation Intel Core processors based on Tiger Lake microarchitecture
		INTEL_06_8DH = 74, // 11th generation Intel Core processors based on Tiger Lake microarchitecture
		INTEL_06_A7H = 73, // 11th generation Intel Core processors based on Rocket Lake microarchitecture
		INTEL_06_7DH = 72, // 10th generation Intel Core processors based on Ice Lake microarchitecture
		INTEL_06_7EH = 71, // 10th generation Intel Core processors based on Ice Lake microarchitecture
		INTEL_06_A5H = 70, // 10th generation Intel Core processors based on Comet Lake microarchitecture
		INTEL_06_A6H = 69, // 10th generation Intel Core processors based on Comet Lake microarchitecture
		INTEL_06_66H = 68, // Intel Core processors based on Cannon Lake microarchitecture
		INTEL_06_8EH = 67, // 7th generation Intel Core processors based on Kaby Lake microarchitecture, 8th and 9th gen
		INTEL_06_9EH = 66, // Intel Xeon E processors based on Coffee Lake microarchitecture
		INTEL_06_6AH = 65, // 3rd generation Intel Xeon Scalable Processor Family based on Ice Lake microarchitecture
		INTEL_06_6CH = 64, // 3rd generation Intel Xeon Scalable Processor Family based on Ice Lake microarchitecture
		INTEL_06_55H = 63, // Intel Xeon Scalable Processor Family based on Skylake microarchitecture
		INTEL_06_4EH = 62, // 6th generation Intel Core processors based on Skylake microarchitecture
		INTEL_06_5EH = 61, // Intel Xeon processor E3-1500m v5 and E3-1200 v5 product family
		INTEL_06_85H = 60, // Intel Xeon Phi Processor 7215, 7285, 7295 Series based on Knights Mill microarchitecture
		INTEL_06_57H = 59, // Intel Xeon Phi Processor 3200, 5200, 7200 Series based on Knights Landing microarchitecture
		INTEL_06_56H = 58, // Intel Xeon processor D-1500 product family based on Broadwell microarchitecture
		INTEL_06_4FH = 57, // Intel Xeon processor E5 v4 Family based on Broadwell microarchitecture
		INTEL_06_47H = 56, // 5th generation Intel Core processors based on Broadwell microarchitecture
		INTEL_06_3DH = 55, // Intel Core M-5xxx Processor based on Broadwell microarchitecture
		INTEL_06_3FH = 54, // Intel Xeon processor E5-4600/2600/1600 v3 product families based on Haswell-E
		INTEL_06_3CH = 53, // 4th Generation Intel Core processor based on Haswell microarchitecture
		INTEL_06_45H = 52, // 4th Generation Intel Core processor based on Haswell microarchitecture
		INTEL_06_46H = 51, // 4th Generation Intel Core processor based on Haswell microarchitecture
		INTEL_06_3EH = 50, // Intel Xeon processor E7-8800/4800/2800 v2 product families based on Ivy Bridge-E
		INTEL_06_3AH = 49, // 3rd Generation Intel Core Processor based on Ivy Bridge microarchitecture
		INTEL_06_2DH = 48, // Intel Xeon processor E5 Family based on Sandy Bridge microarchitecture
		INTEL_06_2FH = 47, // Intel Xeon Processor E7 Family
		INTEL_06_2AH = 46, // Intel Xeon processor E3-1200 product family, 2nd Gen Intel Core Processors
		INTEL_06_2EH = 45, // Intel Xeon processor 7500, 6500 series
		INTEL_06_25H = 44, // Intel Xeon processors 3600, 5600 series, Intel Core i7, i5, i3 Processors
		INTEL_06_2CH = 43, // Intel Xeon processors 3600, 5600 series, Intel Core i7, i5, i3 Processors
		INTEL_06_1EH = 42, // Intel Core i7 and i5 Processors
		INTEL_06_1FH = 41, // Intel Core i7 and i5 Processors
		INTEL_06_1AH = 40, // Intel Core i7 Processor, Intel Xeon processor 3400, 3500, 5500 series
		INTEL_06_1DH = 39, // Intel Xeon processor MP 7400 series
		INTEL_06_17H = 38, // Intel Xeon processor 3100, 3300, 5200, 5400 series
		INTEL_06_0FH = 37, // Intel Core 2 processors, Intel Xeon processor 3000, 3200, 5100, 5300 series
		INTEL_06_0EH = 36, // Intel Core Duo, Intel Core Solo processors
		INTEL_06_0DH = 35, // Intel Pentium M processor
		INTEL_06_86H = 34, // Intel Atom processors based on Tremont Microarchitecture
		INTEL_06_96H = 33, // Intel Atom processors based on Tremont Microarchitecture
		INTEL_06_9CH = 32, // Intel Atom processors based on Tremont Microarchitecture
		INTEL_06_7AH = 31, // Intel Atom processors based on Goldmont Plus microarchitecture
		INTEL_06_5FH = 30, // Intel Atom processors based on Goldmont microarchitecture (Denverton)
		INTEL_06_5CH = 29, // Intel Atom processors based on Goldmont microarchitecture
		INTEL_06_4CH = 28, // Intel Atom processor X7-Z8000 and X5-Z8000 series based on Airmont microarchitecture
		INTEL_06_5DH = 27, // Intel Atom processor X3-C3000 based on Silvermont microarchitecture
		INTEL_06_5AH = 26, // Intel Atom processor Z3500 series
		INTEL_06_4AH = 25, // Intel Atom processor Z3400 series
		INTEL_06_37H = 24, // Intel Atom processor E3000 series, Z3600 series, Z3700 series
		INTEL_06_4DH = 23, // Intel Atom processor C2000 series
		INTEL_06_36H = 22, // Intel Atom processor S1000 Series
		INTEL_06_1CH = 21, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
		INTEL_06_26H = 20, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
		INTEL_06_27H = 19, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
		INTEL_06_35H = 18, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
		INTEL_0F_06H = 17, // Intel Xeon processor 7100, 5000 Series, Intel Xeon Processor MP, Intel Pentium 4, Pentium D processors
		INTEL_0F_03H = 16, // Intel Xeon processor, Intel Xeon processor MP, Intel Pentium 4, Pentium D processors
		INTEL_0F_04H = 15, // Intel Xeon processor, Intel Xeon processor MP, Intel Pentium 4, Pentium D processors
		INTEL_0F_09H = 14, // Intel Pentium M processor
		INTEL_0F_02H = 13, // Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
		INTEL_0F_00H = 12, // Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
		INTEL_0F_01H = 11, // Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
		INTEL_06_07H = 10, // Intel Pentium III Xeon processor, Intel Pentium III processor
		INTEL_06_08H = 9, // Intel Pentium III Xeon processor, Intel Pentium III processor
		INTEL_06_0AH = 8, // Intel Pentium III Xeon processor, Intel Pentium III processor
		INTEL_06_0BH = 7, // Intel Pentium III Xeon processor, Intel Pentium III processor
		INTEL_06_03H = 6, // Intel Pentium II Xeon processor, Intel Pentium II processor
		INTEL_06_05H = 5, // Intel Pentium II Xeon processor, Intel Pentium II processor
		INTEL_06_01H = 4, // Intel Pentium Pro processor
		INTEL_05_01H = 3, // Intel Pentium processor, Intel Pentium processor with MMX Technology
		INTEL_05_02H = 2, // Intel Pentium processor, Intel Pentium processor with MMX Technology
		INTEL_05_04H = 1, // Intel Pentium processor, Intel Pentium processor with MMX Technology


		UNKNOWN = 0, // Returns (0) if CPUID signature is not recognised by the stringToCpuidSignature below

	};

	inline CpuidSignatureValues stringToCpuidSignature(const std::string &str)
	{

		static const std::unordered_map<std::string, CpuidSignatureValues> LUT = {
			{"UNKNOWN", CpuidSignatureValues::UNKNOWN},
			{"INTEL_06_BDH", CpuidSignatureValues::INTEL_06_BDH},
			{"INTEL_06_ADH", CpuidSignatureValues::INTEL_06_ADH},
			{"INTEL_06_AEH", CpuidSignatureValues::INTEL_06_AEH},
			{"INTEL_06_AFH", CpuidSignatureValues::INTEL_06_AFH},
			{"INTEL_06_AAH", CpuidSignatureValues::INTEL_06_AAH},
			{"INTEL_06_CFH", CpuidSignatureValues::INTEL_06_CFH},
			{"INTEL_06_8FH", CpuidSignatureValues::INTEL_06_8FH},
			{"INTEL_06_BAH", CpuidSignatureValues::INTEL_06_BAH},
			{"INTEL_06_B7H", CpuidSignatureValues::INTEL_06_B7H},
			{"INTEL_06_BFH", CpuidSignatureValues::INTEL_06_BFH},
			{"INTEL_06_97H", CpuidSignatureValues::INTEL_06_97H},
			{"INTEL_06_9AH", CpuidSignatureValues::INTEL_06_9AH},
			{"INTEL_06_8CH", CpuidSignatureValues::INTEL_06_8CH},
			{"INTEL_06_8DH", CpuidSignatureValues::INTEL_06_8DH},
			{"INTEL_06_A7H", CpuidSignatureValues::INTEL_06_A7H},
			{"INTEL_06_7DH", CpuidSignatureValues::INTEL_06_7DH},
			{"INTEL_06_7EH", CpuidSignatureValues::INTEL_06_7EH},
			{"INTEL_06_A5H", CpuidSignatureValues::INTEL_06_A5H},
			{"INTEL_06_A6H", CpuidSignatureValues::INTEL_06_A6H},
			{"INTEL_06_66H", CpuidSignatureValues::INTEL_06_66H},
			{"INTEL_06_8EH", CpuidSignatureValues::INTEL_06_8EH},
			{"INTEL_06_9EH", CpuidSignatureValues::INTEL_06_9EH},
			{"INTEL_06_6AH", CpuidSignatureValues::INTEL_06_6AH},
			{"INTEL_06_6CH", CpuidSignatureValues::INTEL_06_6CH},
			{"INTEL_06_55H", CpuidSignatureValues::INTEL_06_55H},
			{"INTEL_06_4EH", CpuidSignatureValues::INTEL_06_4EH},
			{"INTEL_06_5EH", CpuidSignatureValues::INTEL_06_5EH},
			{"INTEL_06_85H", CpuidSignatureValues::INTEL_06_85H},
			{"INTEL_06_57H", CpuidSignatureValues::INTEL_06_57H},
			{"INTEL_06_56H", CpuidSignatureValues::INTEL_06_56H},
			{"INTEL_06_4FH", CpuidSignatureValues::INTEL_06_4FH},
			{"INTEL_06_47H", CpuidSignatureValues::INTEL_06_47H},
			{"INTEL_06_3DH", CpuidSignatureValues::INTEL_06_3DH},
			{"INTEL_06_3FH", CpuidSignatureValues::INTEL_06_3FH},
			{"INTEL_06_3CH", CpuidSignatureValues::INTEL_06_3CH},
			{"INTEL_06_45H", CpuidSignatureValues::INTEL_06_45H},
			{"INTEL_06_46H", CpuidSignatureValues::INTEL_06_46H},
			{"INTEL_06_3EH", CpuidSignatureValues::INTEL_06_3EH},
			{"INTEL_06_3AH", CpuidSignatureValues::INTEL_06_3AH},
			{"INTEL_06_2DH", CpuidSignatureValues::INTEL_06_2DH},
			{"INTEL_06_2FH", CpuidSignatureValues::INTEL_06_2FH},
			{"INTEL_06_2AH", CpuidSignatureValues::INTEL_06_2AH},
			{"INTEL_06_2EH", CpuidSignatureValues::INTEL_06_2EH},
			{"INTEL_06_25H", CpuidSignatureValues::INTEL_06_25H},
			{"INTEL_06_2CH", CpuidSignatureValues::INTEL_06_2CH},
			{"INTEL_06_1EH", CpuidSignatureValues::INTEL_06_1EH},
			{"INTEL_06_1FH", CpuidSignatureValues::INTEL_06_1FH},
			{"INTEL_06_1AH", CpuidSignatureValues::INTEL_06_1AH},
			{"INTEL_06_1DH", CpuidSignatureValues::INTEL_06_1DH},
			{"INTEL_06_17H", CpuidSignatureValues::INTEL_06_17H},
			{"INTEL_06_0FH", CpuidSignatureValues::INTEL_06_0FH},
			{"INTEL_06_0EH", CpuidSignatureValues::INTEL_06_0EH},
			{"INTEL_06_0DH", CpuidSignatureValues::INTEL_06_0DH},
			{"INTEL_06_86H", CpuidSignatureValues::INTEL_06_86H},
			{"INTEL_06_96H", CpuidSignatureValues::INTEL_06_96H},
			{"INTEL_06_9CH", CpuidSignatureValues::INTEL_06_9CH},
			{"INTEL_06_7AH", CpuidSignatureValues::INTEL_06_7AH},
			{"INTEL_06_5FH", CpuidSignatureValues::INTEL_06_5FH},
			{"INTEL_06_5CH", CpuidSignatureValues::INTEL_06_5CH},
			{"INTEL_06_4CH", CpuidSignatureValues::INTEL_06_4CH},
			{"INTEL_06_5DH", CpuidSignatureValues::INTEL_06_5DH},
			{"INTEL_06_5AH", CpuidSignatureValues::INTEL_06_5AH},
			{"INTEL_06_4AH", CpuidSignatureValues::INTEL_06_4AH},
			{"INTEL_06_37H", CpuidSignatureValues::INTEL_06_37H},
			{"INTEL_06_4DH", CpuidSignatureValues::INTEL_06_4DH},
			{"INTEL_06_36H", CpuidSignatureValues::INTEL_06_36H},
			{"INTEL_06_1CH", CpuidSignatureValues::INTEL_06_1CH},
			{"INTEL_06_26H", CpuidSignatureValues::INTEL_06_26H},
			{"INTEL_06_27H", CpuidSignatureValues::INTEL_06_27H},
			{"INTEL_06_35H", CpuidSignatureValues::INTEL_06_35H},
			{"INTEL_0F_06H", CpuidSignatureValues::INTEL_0F_06H},
			{"INTEL_0F_03H", CpuidSignatureValues::INTEL_0F_03H},
			{"INTEL_0F_04H", CpuidSignatureValues::INTEL_0F_04H},
			{"INTEL_0F_09H", CpuidSignatureValues::INTEL_0F_09H},
			{"INTEL_0F_02H", CpuidSignatureValues::INTEL_0F_02H},
			{"INTEL_0F_00H", CpuidSignatureValues::INTEL_0F_00H},
			{"INTEL_0F_01H", CpuidSignatureValues::INTEL_0F_01H},
			{"INTEL_06_07H", CpuidSignatureValues::INTEL_06_07H},
			{"INTEL_06_08H", CpuidSignatureValues::INTEL_06_08H},
			{"INTEL_06_0AH", CpuidSignatureValues::INTEL_06_0AH},
			{"INTEL_06_0BH", CpuidSignatureValues::INTEL_06_0BH},
			{"INTEL_06_03H", CpuidSignatureValues::INTEL_06_03H},
			{"INTEL_06_05H", CpuidSignatureValues::INTEL_06_05H},
			{"INTEL_06_01H", CpuidSignatureValues::INTEL_06_01H},
			{"INTEL_05_01H", CpuidSignatureValues::INTEL_05_01H},
			{"INTEL_05_02H", CpuidSignatureValues::INTEL_05_02H},
			{"INTEL_05_04H", CpuidSignatureValues::INTEL_05_04H}
		};

		auto it = LUT.find(str);
		if (it != LUT.end())
		{
			return it->second;
		}
		else
		{
			return CpuidSignatureValues::UNKNOWN;
		}
	}
	
	
	inline std::string hexToAMDGPUName(const std::string &hexStr)
	{
		static const std::unordered_map<long, std::string> LUT = {
			{0x686c, "AMD Instinct MI25"},
			{0x66a1, "AMD Instinct MI50/MI60 / AMD Radeon Pro VII"},
			{0x738c, "AMD Instinct MI100"},
			{0x740f, "AMD Instinct MI210"},
			{0x7408, "AMD Instinct MI250X"},
			{0x740c, "AMD Instinct MI250X/MI250"},
			{0x74a0, "AMD Instinct MI300A"},
			{0x74a1, "AMD Instinct MI300X"},
			{0x74a2, "AMD Instinct MI308X"},
			{0x74a5, "AMD Instinct MI325X"},
			{0x74a9, "AMD Instinct MI300X HF"},
			{0x74b5, "AMD Instinct MI300X VF"},
			{0x74b6, "AMD Instinct MI308X"},
			{0x74bd, "AMD Instinct MI300X HF"},
			{0x75a0, "AMD Instinct MI350X"},
			{0x75a3, "AMD Instinct MI355X"},
			{0x75b0, "AMD Instinct MI350X VF"},
			{0x75b3, "AMD Instinct MI355X VF"},
			{0x66af, "AMD Radeon VII"},
			{0x67c2, "AMD Radeon Pro V7350x2/V7300X"},
			{0x67d0, "AMD Radeon Pro V7350x2/V7300X"},
			{0x67df, "AMD Radeon Pro 580X"},
			{0x67eb, "AMD Radeon Pro V5300X"},
			{0x7312, "AMD Radeon Pro W5700"},
			{0x731f, "AMD Radeon RX 5700 (XT)/RX 5600 OEM"},
			{0x7340, "AMD Radeon RX 5500 (XT)/RX 5300"},
			{0x7341, "AMD Radeon Pro W5500"},
			{0x7347, "AMD Radeon Pro W5500M"},
			{0x7360, "AMD Radeon Pro V520"},
			{0x7362, "AMD Radeon Pro V540/V520"},
			{0x73a1, "AMD Radeon Pro V620"},
			{0x73a3, "AMD Radeon Pro W6800"},
			{0x73a5, "AMD Radeon RX 6950 XT"},
			{0x73ae, "AMD Radeon Pro V620 MxGPU"},
			{0x73af, "AMD Radeon RX 6900 XT"},
			{0x73bf, "AMD Radeon RX 6900 XT/RX 6800 (XT)"},
			{0x73df, "AMD Radeon RX 6750/6700 XT"},
			{0x73e3, "AMD Radeon Pro W6600"},
			{0x73ef, "AMD Radeon RX 6650 XT"},
			{0x73ff, "AMD Radeon RX 6600 (XT/LE)/RX 6750 GRE 10GB"},
			{0x7421, "AMD Radeon Pro W6500M"},
			{0x7422, "AMD Radeon Pro W6400"},
			{0x7423, "AMD Radeon Pro W6300/W6300M"},
			{0x7424, "AMD Radeon RX 6300"},
			{0x743f, "AMD Radeon RX 6500 (XT)/RX 6400/RX 6550S"},
			{0x7448, "AMD Radeon Pro W7900"},
			{0x7449, "AMD Radeon Pro W7800 48GB"},
			{0x744a, "AMD Radeon Pro W7900 Dual Slot"},
			{0x744b, "AMD Radeon Pro W7900D"},
			{0x744c, "AMD Radeon RX 7900 XTX/XT/GRE"},
			{0x745e, "AMD Radeon Pro W7800"},
			{0x7460, "AMD Radeon Pro V710"},
			{0x7461, "AMD Radeon Pro V710 MxGPU"},
			{0x7470, "AMD Radeon Pro W7700"},
			{0x747e, "AMD Radeon RX 7800 XT/RX 7700 /RX 7700 XT"},
			{0x7480, "AMD Radeon Pro W7600/RX 7600 (XT)/RX 7650 GRE"},
			{0x7489, "AMD Radeon Pro W7500"},
			{0x7499, "AMD Radeon Pro W7400/RX 7400/RX 7300"},
			{0x7550, "AMD Radeon RX 9070 (XT/GRE)"},
			{0x7551, "AMD Radeon AI PRO R9700/R9600D"},
			{0x7590, "AMD Radeon RX 9060 (XT (LP))"}
		};
	
		long hex = 0;
		try
		{
			hex = std::stol(hexStr, nullptr, 16);
		}
		catch (...)
		{
			return "Unknown AMD GPU type";
		}
		
		auto it = LUT.find(hex);
		if (it != LUT.end())
		{
			return it->second;
		}
		else
		{
			return "Unknown AMD GPU type";
		}
	
	}
}

#endif
