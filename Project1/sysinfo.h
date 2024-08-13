#pragma once

#include <iostream>
#include <windows.h>
#include <sysinfoapi.h>
#include <vector>
#include <iphlpapi.h>
#include <iptypes.h>
#include <intrin.h>
#include <string>
#include <codecvt>
#include <tchar.h>

#pragma comment(lib, "Iphlpapi.lib")

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
};


class sysinfo
{

public:
	void getRAMinfo(struct RAM &ram);
	void ListDrives(std::vector<std::wstring>& paths);
	void getDiskUsage(const std::wstring& path,struct Disk &disk);
    std::string GetAdapterFriendlyName(PIP_ADAPTER_INFO pAdapterInfo);
    void printNetworkAdapterFriendlyNames(std::vector<struct Network>& networks);
    std::string WstringToString(const std::wstring& wstr);
    uint64_t ByteToGb(uint64_t size_in_byte);
	bool IsRunningInVirtualMachine();
    class CpuUtilization {
    public:
        CpuUtilization() {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            numCores = sysInfo.dwNumberOfProcessors;

            prevIdleTimes.resize(numCores);
            prevKernelTimes.resize(numCores);
            prevUserTimes.resize(numCores);

            UpdateTimes();
        }

        double PrintAverageCpuUsage() {
            UpdateTimes();

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

            double averageCpuUsage = totalCpuUsage / numCores;
            std::cout << "Average CPU Usage: " << averageCpuUsage << "%" << std::endl;
            return averageCpuUsage;
        }

    private:
        DWORD numCores;
        std::vector<ULONGLONG> prevIdleTimes, prevKernelTimes, prevUserTimes;
        std::vector<ULONGLONG> idleTimes, kernelTimes, userTimes;

        void UpdateTimes() {
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
    };
};

