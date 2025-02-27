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
     * Table 2-1 CPUID Signature Values of DisplayFamily_DisplayModel Mappings
     */
    enum class CpuidSignatureValues
    {
        UNKNOWN, // Returns (0) if CPUID signature is not recognised by the stringToCpuidSignature below

        INTEL_06_BDH, // Intel Series 2 Core Ultra processors supporting Lunar Lake performance hybrid architecture
        INTEL_06_ADH, // Intel Xeon 6 P-core processors based on Granite Rapids microarchitecture
        INTEL_06_AEH, // Intel Xeon 6 P-core processors based on Granite Rapids microarchitecture
        INTEL_06_AFH, // Intel Xeon 6 E-core processors based on Sierra Forest microarchitecture
        INTEL_06_AAH, // Intel Core Ultra 7 processors supporting Meteor Lake performance hybrid architecture
        INTEL_06_CFH, // 5th generation Intel Xeon Scalable Processor Family based on Emerald Rapids microarchitecture
        INTEL_06_8FH, // 4th generation Intel Xeon Scalable Processor Family based on Sapphire Rapids microarchitecture
        INTEL_06_BAH, // 13th generation Intel Core processors supporting Raptor Lake performance hybrid architecture
        INTEL_06_B7H, // 13th generation Intel Core processors supporting Raptor Lake performance hybrid architecture
        INTEL_06_BFH, // 13th generation Intel Core processors supporting Raptor Lake performance hybrid architecture
        INTEL_06_97H, // 12th generation Intel Core processors supporting Alder Lake performance hybrid architecture
        INTEL_06_9AH, // 12th generation Intel Core processors supporting Alder Lake performance hybrid architecture
        INTEL_06_8CH, // 11th generation Intel Core processors based on Tiger Lake microarchitecture
        INTEL_06_8DH, // 11th generation Intel Core processors based on Tiger Lake microarchitecture
        INTEL_06_A7H, // 11th generation Intel Core processors based on Rocket Lake microarchitecture
        INTEL_06_7DH, // 10th generation Intel Core processors based on Ice Lake microarchitecture
        INTEL_06_7EH, // 10th generation Intel Core processors based on Ice Lake microarchitecture
        INTEL_06_A5H, // 10th generation Intel Core processors based on Comet Lake microarchitecture
        INTEL_06_A6H, // 10th generation Intel Core processors based on Comet Lake microarchitecture
        INTEL_06_66H, // Intel Core processors based on Cannon Lake microarchitecture
        INTEL_06_8EH, // 7th generation Intel Core processors based on Kaby Lake microarchitecture, 8th and 9th gen
        INTEL_06_9EH, // Intel Xeon E processors based on Coffee Lake microarchitecture
        INTEL_06_6AH, // 3rd generation Intel Xeon Scalable Processor Family based on Ice Lake microarchitecture
        INTEL_06_6CH, // 3rd generation Intel Xeon Scalable Processor Family based on Ice Lake microarchitecture
        INTEL_06_55H, // Intel Xeon Scalable Processor Family based on Skylake microarchitecture
        INTEL_06_4EH, // 6th generation Intel Core processors based on Skylake microarchitecture
        INTEL_06_5EH, // Intel Xeon processor E3-1500m v5 and E3-1200 v5 product family
        INTEL_06_85H, // Intel Xeon Phi Processor 7215, 7285, 7295 Series based on Knights Mill microarchitecture
        INTEL_06_57H, // Intel Xeon Phi Processor 3200, 5200, 7200 Series based on Knights Landing microarchitecture
        INTEL_06_56H, // Intel Xeon processor D-1500 product family based on Broadwell microarchitecture
        INTEL_06_4FH, // Intel Xeon processor E5 v4 Family based on Broadwell microarchitecture
        INTEL_06_47H, // 5th generation Intel Core processors based on Broadwell microarchitecture
        INTEL_06_3DH, // Intel Core M-5xxx Processor based on Broadwell microarchitecture
        INTEL_06_3FH, // Intel Xeon processor E5-4600/2600/1600 v3 product families based on Haswell-E
        INTEL_06_3CH, // 4th Generation Intel Core processor based on Haswell microarchitecture
        INTEL_06_45H, // 4th Generation Intel Core processor based on Haswell microarchitecture
        INTEL_06_46H, // 4th Generation Intel Core processor based on Haswell microarchitecture
        INTEL_06_3EH, // Intel Xeon processor E7-8800/4800/2800 v2 product families based on Ivy Bridge-E
        INTEL_06_3AH, // 3rd Generation Intel Core Processor based on Ivy Bridge microarchitecture
        INTEL_06_2DH, // Intel Xeon processor E5 Family based on Sandy Bridge microarchitecture
        INTEL_06_2FH, // Intel Xeon Processor E7 Family
        INTEL_06_2AH, // Intel Xeon processor E3-1200 product family, 2nd Gen Intel Core Processors
        INTEL_06_2EH, // Intel Xeon processor 7500, 6500 series
        INTEL_06_25H, // Intel Xeon processors 3600, 5600 series, Intel Core i7, i5, i3 Processors
        INTEL_06_2CH, // Intel Xeon processors 3600, 5600 series, Intel Core i7, i5, i3 Processors
        INTEL_06_1EH, // Intel Core i7 and i5 Processors
        INTEL_06_1FH, // Intel Core i7 and i5 Processors
        INTEL_06_1AH, // Intel Core i7 Processor, Intel Xeon processor 3400, 3500, 5500 series
        INTEL_06_1DH, // Intel Xeon processor MP 7400 series
        INTEL_06_17H, // Intel Xeon processor 3100, 3300, 5200, 5400 series
        INTEL_06_0FH, // Intel Core 2 processors, Intel Xeon processor 3000, 3200, 5100, 5300 series
        INTEL_06_0EH, // Intel Core Duo, Intel Core Solo processors
        INTEL_06_0DH, // Intel Pentium M processor
        INTEL_06_86H, // Intel Atom processors based on Tremont Microarchitecture
        INTEL_06_96H, // Intel Atom processors based on Tremont Microarchitecture
        INTEL_06_9CH, // Intel Atom processors based on Tremont Microarchitecture
        INTEL_06_7AH, // Intel Atom processors based on Goldmont Plus microarchitecture
        INTEL_06_5FH, // Intel Atom processors based on Goldmont microarchitecture (Denverton)
        INTEL_06_5CH, // Intel Atom processors based on Goldmont microarchitecture
        INTEL_06_4CH, // Intel Atom processor X7-Z8000 and X5-Z8000 series based on Airmont microarchitecture
        INTEL_06_5DH, // Intel Atom processor X3-C3000 based on Silvermont microarchitecture
        INTEL_06_5AH, // Intel Atom processor Z3500 series
        INTEL_06_4AH, // Intel Atom processor Z3400 series
        INTEL_06_37H, // Intel Atom processor E3000 series, Z3600 series, Z3700 series
        INTEL_06_4DH, // Intel Atom processor C2000 series
        INTEL_06_36H, // Intel Atom processor S1000 Series
        INTEL_06_1CH, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
        INTEL_06_26H, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
        INTEL_06_27H, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
        INTEL_06_35H, // Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
        INTEL_0F_06H, // Intel Xeon processor 7100, 5000 Series, Intel Xeon Processor MP, Intel Pentium 4, Pentium D processors
        INTEL_0F_03H, // Intel Xeon processor, Intel Xeon processor MP, Intel Pentium 4, Pentium D processors
        INTEL_0F_04H, // Intel Xeon processor, Intel Xeon processor MP, Intel Pentium 4, Pentium D processors
        INTEL_0F_09H, // Intel Pentium M processor
        INTEL_0F_02H, // Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
        INTEL_0F_00H, // Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
        INTEL_0F_01H, // Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
        INTEL_06_07H, // Intel Pentium III Xeon processor, Intel Pentium III processor
        INTEL_06_08H, // Intel Pentium III Xeon processor, Intel Pentium III processor
        INTEL_06_0AH, // Intel Pentium III Xeon processor, Intel Pentium III processor
        INTEL_06_0BH, // Intel Pentium III Xeon processor, Intel Pentium III processor
        INTEL_06_03H, // Intel Pentium II Xeon processor, Intel Pentium II processor
        INTEL_06_05H, // Intel Pentium II Xeon processor, Intel Pentium II processor
        INTEL_06_01H, // Intel Pentium Pro processor
        INTEL_05_01H, // Intel Pentium processor, Intel Pentium processor with MMX Technology
        INTEL_05_02H, // Intel Pentium processor, Intel Pentium processor with MMX Technology
        INTEL_05_04H, // Intel Pentium processor, Intel Pentium processor with MMX Technology

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

}

#endif