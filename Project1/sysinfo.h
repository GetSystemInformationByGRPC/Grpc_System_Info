#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <codecvt>
#include <grpcpp/grpcpp.h>
#include <grpc/grpc_security.h>
#include <thread>
#include <grpc++/grpc++.h>
#include <fstream>
#include <cstdint>
#include "Logger.h"


#ifdef _WIN32
#include <tchar.h>
#include <Wbemidl.h>
#include <comutil.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <intrin.h>
#include <windows.h>
#include <sysinfoapi.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Iphlpapi.lib")


#elif __linux__
#include <sstream>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif
struct RAM {
	uint64_t Total_RAM;
	uint64_t Used_RAM;

};

struct Disk {
    std::string Path;
    uint64_t Total_Size;
    uint64_t Used_Space;

};

struct Network {
    std::string Friendly_Name;
    std::string Adapter_Desc;
    std::string Ip_Address;
    std::string Ip_Mask;
    std::string Gateway;
    std::string Dhcp_Server;
    uint64_t Send_Bytes;
    uint64_t Receive_Bytes;
    uint64_t Bandwidth;
};



class sysinfo
{

public:
#ifdef _WIN32
    std::string GetAdapterFriendlyName(PIP_ADAPTER_INFO pAdapterInfo);
    static std::string convertToString(DWORD error_code);
    std::string WstringToString(const std::wstring& wstr);
    void ListDrives(std::vector<std::wstring>& paths);
#endif
	void getRAMinfo(struct RAM &ram);
	void getDiskUsage(const std::wstring& path,struct Disk &disk);
    void printNetworkAdapterFriendlyNames(std::vector<struct Network>& networks);
    uint64_t ByteToGb(uint64_t size_in_byte);
    uint64_t readSysfsValue(const std::string& path);
	bool IsRunningInVirtualMachine();
    class CpuUtilization {
    public:
        CpuUtilization() {
#ifdef _WIN32
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            numCores = sysInfo.dwNumberOfProcessors;

            prevIdleTimes.resize(numCores);
            prevKernelTimes.resize(numCores);
            prevUserTimes.resize(numCores);


#elif __linux__
            numCores = std::thread::hardware_concurrency();
            prevIdleTimes.resize(numCores);
            prevTotalTimes.resize(numCores);
#endif
            UpdateTimes();
        }

        double PrintAverageCpuUsage() {
            UpdateTimes();
#ifdef _WIN32
            std::vector<double> cpuUsages(numCores);
            double totalCpuUsage = 0.0;

            for (DWORD i = 0; i < numCores; ++i) {
                ULONGLONG idleDelta = idleTimes[i] - prevIdleTimes[i];
                ULONGLONG kernelDelta = kernelTimes[i] - prevKernelTimes[i];
                ULONGLONG userDelta = userTimes[i] - prevUserTimes[i];

                prevIdleTimes[i] = idleTimes[i];
                prevKernelTimes[i] = kernelTimes[i];
                prevUserTimes[i] = userTimes[i];

                ULONGLONG totalDelta = kernelDelta + userDelta;
                double cpuUsage = totalDelta > 0 ? (static_cast<double>(totalDelta - idleDelta) / totalDelta) * 100.0 : 0.0;

                cpuUsages[i] = cpuUsage;
                totalCpuUsage += cpuUsage;
            }
#elif __linux__
            double totalCpuUsage = 0.0;

            for (unsigned int i = 0; i < numCores; ++i) {
                uint64_t idleDelta = idleTimes[i] - prevIdleTimes[i];
                uint64_t totalDelta = totalTimes[i] - prevTotalTimes[i];

                double cpuUsage = (totalDelta > 0) ? (1.0 - (static_cast<double>(idleDelta) / static_cast<double>(totalDelta))) * 100.0 : 0.0;

                totalCpuUsage += cpuUsage;

                prevIdleTimes[i] = idleTimes[i];
                prevTotalTimes[i] = totalTimes[i];
            }
#endif


            double averageCpuUsage = totalCpuUsage / numCores;
            //std::cout << "Average CPU Usage: " << averageCpuUsage << "%" << std::endl;
            return averageCpuUsage;
        }

    private:
#ifdef _WIN32
        DWORD numCores;
        std::vector<ULONGLONG> prevIdleTimes, prevKernelTimes, prevUserTimes;
        std::vector<ULONGLONG> idleTimes, kernelTimes, userTimes;
#elif __linux__
        unsigned int numCores;
        std::vector<uint64_t> prevIdleTimes, prevTotalTimes;
        std::vector<uint64_t> idleTimes, totalTimes;

#endif

        void UpdateTimes() {
#ifdef _WIN32
            idleTimes.resize(numCores);
            kernelTimes.resize(numCores);
            userTimes.resize(numCores);

            for (DWORD i = 0; i < numCores; ++i) {
                FILETIME idleTime, kernelTime, userTime;
                GetSystemTimes(&idleTime, &kernelTime, &userTime);

                idleTimes[i] = FileTimeToUInt64(idleTime);
                kernelTimes[i] = FileTimeToUInt64(kernelTime);
                userTimes[i] = FileTimeToUInt64(userTime);
            }
        }

        ULONGLONG FileTimeToUInt64(const FILETIME& fileTime) {
            return (static_cast<ULONGLONG>(fileTime.dwLowDateTime) | (static_cast<ULONGLONG>(fileTime.dwHighDateTime) << 32));
        }
#elif __linux__
            idleTimes.resize(numCores, 0);
            totalTimes.resize(numCores, 0);

            std::ifstream statFile("/proc/stat");
            std::string line;

            int coreIndex = 0;

            while (std::getline(statFile, line)) {
                if (line.substr(0, 3) == "cpu") {
                    std::istringstream iss(line);
                    std::string cpuLabel;
                    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;

                    iss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

                    idleTimes[coreIndex] = idle + iowait;
                    totalTimes[coreIndex] = user + nice + system + idle + iowait + irq + softirq + steal;

                    coreIndex++;
                    if (coreIndex >= numCores) {
                        break;
                    }
                }
            }
#endif

        }
    };
};
