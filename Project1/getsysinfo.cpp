#include <iostream>
#include <windows.h>
#include <sysinfoapi.h>
#include <vector>
#include <iphlpapi.h>
#include <iptypes.h>
#include <intrin.h>
#include <string>
#include <stdlib.h>
#include <tchar.h>

#pragma comment(lib, "Iphlpapi.lib")


void printRAMinfo()
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);

    if (GlobalMemoryStatusEx(&statex)) {
        long long totalRAMinGB = statex.ullTotalPhys / (1024 * 1024 * 1024);
        long long usedRAMinGB = (statex.ullTotalPhys - statex.ullAvailPhys) / (1024 * 1024 * 1024);

        long long  totalRAMinMB = statex.ullTotalPhys / (1024 * 1024);
        long long usedRAMinMB = (statex.ullTotalPhys - statex.ullAvailPhys) / (1024 * 1024);

        std::cout << "Total RAM: " << totalRAMinGB << " GB" << std::endl;
        std::cout << "Used RAM: " << usedRAMinGB << " GB" << std::endl;

        std::cout << "Total RAM: " << totalRAMinMB << " MB" << std::endl;
        std::cout << "Used RAM: " << usedRAMinMB << " MB" << std::endl;
        std::cout << "Percent Of RAM Usage: " << (usedRAMinMB * 100) / totalRAMinMB << "% " << std::endl;

    }
    else {
        std::cerr << "Failed to get memory status." << std::endl;
    }
}

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

    void PrintAverageCpuUsage() {
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


void ListDrives(std::vector<std::wstring> &paths) {
    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        std::cerr << "GetLogicalDrives failed with error: " << GetLastError() << std::endl;
        return;
    }

    wchar_t driveLetter = L'A';
    while (drives) {
        if (drives & 1) {
            std::wstring driveName = std::wstring(1, driveLetter) + L":\\";
            UINT driveType = GetDriveTypeW(driveName.c_str());
            std::wstring driveTypeStr;

            switch (driveType) {
            case DRIVE_UNKNOWN:
                driveTypeStr = L"Unknown Drive";
                break;
            case DRIVE_NO_ROOT_DIR:
                driveTypeStr = L"Invalid Path";
                break;
            case DRIVE_REMOVABLE:
                driveTypeStr = L"Removable Drive";
                break;
            case DRIVE_FIXED:
                driveTypeStr = L"Fixed Drive";
                break;
            case DRIVE_REMOTE:
                driveTypeStr = L"Network Drive";
                break;
            case DRIVE_CDROM:
                driveTypeStr = L"CD-ROM Drive";
                break;
            case DRIVE_RAMDISK:
                driveTypeStr = L"RAM Disk";
                break;
            default:
                driveTypeStr = L"Other Drive";
                break;
            }

            std::wcout << L"Drive " << driveName << L": " << driveTypeStr << std::endl;
            paths.push_back(driveName);
        }
        drives >>= 1;
        ++driveLetter;
    }
}

void PrintDiskUsage(const std::wstring& path) {
    ULARGE_INTEGER freeBytesAvailable;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceEx(path.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        ULONGLONG usedBytes = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
        double usedPercentage = (static_cast<double>(usedBytes) / totalNumberOfBytes.QuadPart) * 100.0;

        std::wcout << L"Disk Path: " << path << std::endl;
        std::wcout << L"Total Size: " << totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024) << L" GB" << std::endl;
        std::wcout << L"Used Space: " << usedBytes / (1024 * 1024 * 1024) << L" GB" << std::endl;
        std::wcout << L"Free Space: " << totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024) << L" GB" << std::endl;
        std::wcout << L"Percentage Used: " << usedPercentage << L"%" << std::endl;
    }
    else {
        std::cerr << "Failed to get disk space information." << std::endl;
    }
    return;
    /* how to use hard print
    std::wstring path = L"C:\\";
    std::wstring path1 = L"D:\\";
    std::wstring path2 = L"E:\\";
    std::wstring path3 = L"F:\\";
    std::wstring path4 = L"G:\\";

    PrintDiskUsage(path);
    PrintDiskUsage(path1);
    PrintDiskUsage(path2);
    PrintDiskUsage(path3);
    PrintDiskUsage(path4);
*/

}

void printNewtworkInfo()
{
    IP_ADAPTER_INFO* pAdapterInfo;
    ULONG            ulOutBufLen;
    DWORD            dwRetVal;
    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
    }
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) != ERROR_SUCCESS) {
        printf("GetAdaptersInfo call failed with %d\n", dwRetVal);
    }
    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    while (pAdapter) {
        printf("Adapter Name: %s\n", pAdapter->AdapterName);
        printf("Adapter Desc: %s\n", pAdapter->Description);
        printf("\tAdapter Addr: \t");
        for (UINT i = 0; i < pAdapter->AddressLength; i++) {
            if (i == (pAdapter->AddressLength - 1))
                printf("%.2X\n", (int)pAdapter->Address[i]);
            else
                printf("%.2X-", (int)pAdapter->Address[i]);
        }
        printf("IP Address: %s\n", pAdapter->IpAddressList.IpAddress.String);
        printf("IP Mask: %s\n", pAdapter->IpAddressList.IpMask.String);
        printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
        printf("\t***\n");
        if (pAdapter->DhcpEnabled) {
            printf("\tDHCP Enabled: Yes\n");
            printf("\t\tDHCP Server: \t%s\n", pAdapter->DhcpServer.IpAddress.String);
        }
        else
            printf("\tDHCP Enabled: No\n");

        pAdapter = pAdapter->Next;
    }
    if (pAdapterInfo)
        free(pAdapterInfo);
}

bool IsRunningInVirtualMachine() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 0x1);

    bool isVirtualMachine = (cpuInfo[2] & (1 << 31)) != 0;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    std::string processorName;
    char buffer[0x40];
    DWORD size = sizeof(buffer);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", RRF_RT_REG_SZ, NULL, buffer, &size) == ERROR_SUCCESS) {
        processorName = buffer;
    }

    if (processorName.find("VMware") != std::string::npos ||
        processorName.find("VirtualBox") != std::string::npos ||
        processorName.find("KVM") != std::string::npos ||
        processorName.find("Hyper-V") != std::string::npos) {
        isVirtualMachine = true;
    }

    return isVirtualMachine;
    /*for run this func
    if (IsRunningInVirtualMachine()) {
        std::cout << "The system is running in a virtual machine." << std::endl;
    }
    else {
        std::cout << "The system is running on physical hardware." << std::endl;
    }
    */
}
void printNetworkInfo2()
{
    IP_ADAPTER_INFO* pAdapterInfo;
    ULONG            ulOutBufLen;
    DWORD            dwRetVal;

    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) != ERROR_SUCCESS) {
        printf("GetAdaptersInfo call failed with %d\n", dwRetVal);
    }

    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    while (pAdapter) {
        printf("Adapter Desc: %s\n", pAdapter->Description);
        printf("IP Address: %s\n", pAdapter->IpAddressList.IpAddress.String);
        printf("IP Mask: %s\n", pAdapter->IpAddressList.IpMask.String);

        if (pAdapter->DhcpEnabled) {
            printf("DHCP Server: %s\n", pAdapter->DhcpServer.IpAddress.String);
        }
        else {
            printf("DHCP Enabled: No\n");
        }
        printf("\n");

        pAdapter = pAdapter->Next;
    }

    if (pAdapterInfo)
        free(pAdapterInfo);
}


std::wstring GetAdapterFriendlyName(PIP_ADAPTER_INFO pAdapterInfo)
{
    HKEY hKey;
    LONG lResult;
    TCHAR subKey[512];
    DWORD dwSize;
    TCHAR szValueData[256];

    // Open the registry key where network adapter info is stored
    _stprintf_s(subKey, _T("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%S\\Connection"), pAdapterInfo->AdapterName);
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {
        dwSize = sizeof(szValueData);
        lResult = RegQueryValueEx(hKey, _T("Name"), NULL, NULL, (LPBYTE)szValueData, &dwSize);

        if (lResult == ERROR_SUCCESS) {
           // _tprintf(_T("Adapter Name: %s\n"), pAdapterInfo->AdapterName);
            _tprintf(_T("Friendly Name: %s\n"), szValueData);
            std::wstring rt(&szValueData[0], sizeof(szValueData) / sizeof(szValueData[0]));
            return rt;
        }
        else {
            _tprintf(_T("Failed to query registry value. Error: %d\n"), lResult);
        }

        RegCloseKey(hKey);
    }
    else {
        _tprintf(_T("Failed to open registry key. Error: %d\n"), lResult);
    }
}

void printNetworkAdapterFriendlyNames()
{
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
    if (pAdapterInfo == NULL) {
        std::cerr << "Error allocating memory for GetAdaptersInfo\n";
        return;
    }

    // Make an initial call to GetAdaptersInfo to get the necessary size
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            std::cerr << "Error allocating memory for GetAdaptersInfo\n";
            return;
        }
    }

    // Get the actual data
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            GetAdapterFriendlyName(pAdapter);
            printf("Adapter Desc: %s\n", pAdapter->Description);
            printf("IP Address: %s\n", pAdapter->IpAddressList.IpAddress.String);
            printf("IP Mask: %s\n", pAdapter->IpAddressList.IpMask.String);

            if (pAdapter->DhcpEnabled) {
                printf("DHCP Server: %s\n", pAdapter->DhcpServer.IpAddress.String);
            }
            else {
                printf("DHCP Enabled: No\n");
            }
            printf("\n");


            pAdapter = pAdapter->Next;
        }
    }
    else {
        std::cerr << "GetAdaptersInfo failed.\n";
    }

    if (pAdapterInfo)
        free(pAdapterInfo);
}
/*
int main()
{/*printRAMinfo();
    CpuUtilization cpuu;
    cpuu.PrintAverageCpuUsage();
    
    std::vector<std::wstring> paths;
    ListDrives(paths);
    for (auto path : paths)
    {
        PrintDiskUsage(path);
    }
    printNewtworkInfo();
    if (IsRunningInVirtualMachine()) {
        std::cout << "The system is running in a virtual machine." << std::endl;
    }
    else {
        std::cout << "The system is running on physical hardware." << std::endl;
    }
    printNetworkAdapterFriendlyNames();
    
    return 0;
}
*/