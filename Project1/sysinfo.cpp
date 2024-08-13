#include "sysinfo.h"

uint64_t sysinfo::ByteToGb(uint64_t size_in_byte)
{
    return size_in_byte / (1024 * 1024 * 1024);
}

std::string sysinfo::WstringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

void sysinfo::getRAMinfo(struct RAM &ram)
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    
    if (GlobalMemoryStatusEx(&statex)) {
        uint64_t Total_RAM = statex.ullTotalPhys;
        uint64_t Used_RAM = (statex.ullTotalPhys - statex.ullAvailPhys);

        //std::cout << "Total RAM: " << totalRAMinGB << " GB" << std::endl;
        //std::cout << "Used RAM: " << usedRAMinGB << " GB" << std::endl;

        ram.Total_RAM = Total_RAM;
        ram.Used_RAM = Used_RAM;

    }
    else {
        //std::cerr << "Failed to get memory status." << std::endl;
    }
}

void sysinfo::ListDrives(std::vector<std::wstring>& paths) {
    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        //std::cerr << "GetLogicalDrives failed with error: " << GetLastError() << std::endl;
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

            //std::wcout << L"Drive " << driveName << L": " << driveTypeStr << std::endl;
            paths.push_back(driveName);
        }
        drives >>= 1;
        ++driveLetter;
    }

}

void sysinfo::getDiskUsage(const std::wstring& path, struct Disk& disk) {
    ULARGE_INTEGER freeBytesAvailable;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceEx(path.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        ULONGLONG usedBytes = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
        double usedPercentage = (static_cast<double>(usedBytes) / totalNumberOfBytes.QuadPart) * 100.0;

        disk.Path = WstringToString(path);
        disk.Total_Size = totalNumberOfBytes.QuadPart;
        disk.Used_Space = usedBytes;
        //std::wcout << L"Disk Path: " << path << std::endl;
        //std::wcout << L"Total Size: " << totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024) << L" GB" << std::endl;
        //std::wcout << L"Used Space: " << usedBytes / (1024 * 1024 * 1024) << L" GB" << std::endl;
        //std::wcout << L"Free Space: " << totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024) << L" GB" << std::endl;
        //std::wcout << L"Percentage Used: " << usedPercentage << L"%" << std::endl;

    }
    else {
        //std::cerr << "Failed to get disk space information." << std::endl;
    }
    return;
}


bool sysinfo::IsRunningInVirtualMachine() {
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
    if (isVirtualMachine) {
        //std::cout << "The system is running in a virtual machine." << std::endl;
    }
    else {
        //std::cout << "The system is running on physical hardware." << std::endl;
    }
    return isVirtualMachine;

}
std::string sysinfo::GetAdapterFriendlyName(PIP_ADAPTER_INFO pAdapterInfo)
{
    HKEY hKey;
    LONG lResult;
    TCHAR subKey[512];
    DWORD dwSize;
    TCHAR szValueData[256];
    std::string friendlyName;

    // Open the registry key where network adapter info is stored
    _stprintf_s(subKey, _T("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%S\\Connection"), pAdapterInfo->AdapterName);
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {
        dwSize = sizeof(szValueData);
        lResult = RegQueryValueEx(hKey, _T("Name"), NULL, NULL, (LPBYTE)szValueData, &dwSize);

        if (lResult == ERROR_SUCCESS) {
            // Convert TCHAR (szValueData) to std::string (friendlyName)
#ifdef UNICODE
            std::wstring ws(szValueData);
            friendlyName = std::string(ws.begin(), ws.end());
#else
            friendlyName = std::string(szValueData);
#endif
        }
        else {
            //std::cerr << "Failed to query registry value. Error: " << lResult << std::endl;
        }

        RegCloseKey(hKey);
    }
    else {
        //std::cerr << "Failed to open registry key. Error: " << lResult << std::endl;
    }

    return friendlyName;
}

void sysinfo::printNetworkAdapterFriendlyNames(std::vector<struct Network>& networks)
{
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
    if (pAdapterInfo == NULL) {
        //std::cerr << "Error allocating memory for GetAdaptersInfo\n";
        return;
    }

    // Make an initial call to GetAdaptersInfo to get the necessary size
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            //std::cerr << "Error allocating memory for GetAdaptersInfo\n";
            return;
        }
    }

    // Get the actual data
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            struct Network network;
            std::string fname = GetAdapterFriendlyName(pAdapter);
            //printf("Adapter Desc: %s\n", pAdapter->Description);
            //printf("IP Address: %s\n", pAdapter->IpAddressList.IpAddress.String);
            //printf("IP Mask: %s\n", pAdapter->IpAddressList.IpMask.String);
            //printf("Gateway: %s\n", pAdapter->GatewayList.IpAddress.String);
            network.Friendly_Name = fname;
            network.Adapter_Desc = pAdapter->Description;
            network.Ip_Address = pAdapter->IpAddressList.IpAddress.String;
            network.Ip_Mask = pAdapter->IpAddressList.IpMask.String;
            network.Gateway = pAdapter->GatewayList.IpAddress.String;


            if (pAdapter->DhcpEnabled) {
                //printf("DHCP Server: %s\n", pAdapter->DhcpServer.IpAddress.String);
                network.Dhcp_Server = pAdapter->DhcpServer.IpAddress.String;
            }
            else {
                //printf("DHCP Enabled: No\n");
                network.Dhcp_Server = "Dhcp servet disabled";
            }
            //printf("\n");

            networks.push_back(network);
            pAdapter = pAdapter->Next;
        }
    }
    else {
        //std::cerr << "GetAdaptersInfo failed.\n";
    }

    if (pAdapterInfo)
        free(pAdapterInfo);
}

