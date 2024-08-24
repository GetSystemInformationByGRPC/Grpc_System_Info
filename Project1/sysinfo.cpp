#include "sysinfo.h"

#ifdef _WIN32
std::string sysinfo::convertToString(DWORD error_code) {
    std::ostringstream oss;
    oss << "OpenSCManager failed, error: " << error_code;
    return oss.str();
}
std::string sysinfo::WstringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
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


#endif

uint64_t sysinfo::ByteToGb(uint64_t size_in_byte)
{
    return size_in_byte / (1024 * 1024 * 1024);
}


void sysinfo::getRAMinfo(struct RAM &ram)
{
#ifdef _WIN32
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
#elif  __linux__
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        std::cerr << "Failed to open /proc/meminfo" << std::endl;
        return;
    }

    uint64_t MemTotal = 0;
    uint64_t MemFree = 0;
    uint64_t Buffers = 0;
    uint64_t Cached = 0;

    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            MemTotal = std::stoull(line.substr(line.find_first_of("0123456789")));
        } else if (line.find("MemFree:") == 0) {
            MemFree = std::stoull(line.substr(line.find_first_of("0123456789")));
        } else if (line.find("Buffers:") == 0) {
            Buffers = std::stoull(line.substr(line.find_first_of("0123456789")));
        } else if (line.find("Cached:") == 0) {
            Cached = std::stoull(line.substr(line.find_first_of("0123456789")));
        }
    }

    meminfo.close();

    uint64_t Total_RAM = MemTotal * 1024; // Convert from kB to Bytes
    uint64_t Used_RAM = (Total_RAM - (MemFree + Buffers + Cached) * 1024); // Convert from kB to Bytes

    ram.Total_RAM = Total_RAM;
    ram.Used_RAM = Used_RAM;
#endif

}


void sysinfo::getDiskUsage(const std::wstring& path, struct Disk& disk) {
#ifdef _WIN32
    ULARGE_INTEGER freeBytesAvailable;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceEx(path.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        ULONGLONG usedBytes = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
        double usedPercentage = (static_cast<double>(usedBytes) / totalNumberOfBytes.QuadPart) * 100.0;

        disk.Path = WstringToString(path);
        disk.Total_Size = totalNumberOfBytes.QuadPart;
        disk.Used_Space = usedBytes;


    }
    else {
        //std::cerr << "Failed to get disk space information." << std::endl;
    }
#elif __linux__
    FILE* mounts = fopen("/proc/mounts", "r");
    if (mounts == nullptr) {
        std::cerr << "Failed to open /proc/mounts" << std::endl;
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), mounts)) {
        std::string mountPoint;
        std::string device;
        std::stringstream ss(line);
        ss >> device >> mountPoint;

        struct statvfs stat;
        if ( (statvfs(mountPoint.c_str(), &stat) == 0) && mountPoint == "/" ) {

            disk.Path = mountPoint;
            disk.Total_Size = stat.f_blocks * stat.f_frsize;
            uint64_t Free_Size = stat.f_bfree * stat.f_frsize;
            disk.Used_Space = disk.Total_Size - Free_Size;
            break;
        } else {
            //std::cerr << "Failed to get file system statistics for " << mountPoint << std::endl;
        }
    }

    fclose(mounts);
#endif

    return;
}


bool sysinfo::IsRunningInVirtualMachine() {
#ifdef _WIN32
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
#elif __linux__
    bool isVirtualMachine;
    std::ifstream dmiFile("/sys/class/dmi/id/product_name");
    std::string product_name;
    if (dmiFile.is_open()) {
        std::getline(dmiFile, product_name);
        dmiFile.close();

        if (product_name.find("VMware") != std::string::npos ||
            product_name.find("VirtualBox") != std::string::npos ||
            product_name.find("KVM") != std::string::npos ||
            product_name.find("QEMU") != std::string::npos ||
            product_name.find("Hyper-V") != std::string::npos) {
            isVirtualMachine = true;
            }
    }

    // بررسی CPU flags برای تشخیص Hypervisor
    std::ifstream cpuinfoFile("/proc/cpuinfo");
    std::string line;
    if (cpuinfoFile.is_open()) {
        while (std::getline(cpuinfoFile, line)) {
            if (line.find("hypervisor") != std::string::npos) {
                isVirtualMachine = true;
                break;
            }
        }
        cpuinfoFile.close();
    }

#endif
    return isVirtualMachine;

}

uint64_t sysinfo::readSysfsValue(const std::string& path) {
    std::ifstream file(path);
    uint64_t value = 0;
    if (file.is_open()) {
        file >> value;
    }
    return value;
}

void sysinfo::printNetworkAdapterFriendlyNames(std::vector<struct Network>& networks)
{
#ifdef _WIN32
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
            MIB_IFTABLE* pIfTable;
            MIB_IFROW* pIfRow;
            ULONG ulSize = 0;
            pIfTable = (MIB_IFTABLE*)malloc(sizeof(MIB_IFTABLE));
            if (pIfTable == NULL) {
                Logger::error("Error allocating memory for GetIfTable");
                free(pAdapterInfo);
                return;
            }
            ulSize = sizeof(MIB_IFTABLE);

            if (GetIfTable(pIfTable, &ulSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
                free(pIfTable);
                pIfTable = (MIB_IFTABLE*)malloc(ulSize);
                if (pIfTable == NULL) {
                    Logger::error("Error allocating memory for GetIfTable");
                    free(pAdapterInfo);
                    return;
                }
            }

            if (GetIfTable(pIfTable, &ulSize, FALSE) == NO_ERROR) {
                for (int i = 0; i < (int)pIfTable->dwNumEntries; i++) {
                    pIfRow = (MIB_IFROW*)&pIfTable->table[i];
                    if (pIfRow->dwIndex == pAdapter->Index) {
                        network.Send_Bytes = pIfRow->dwOutOctets;
                        network.Receive_Bytes = pIfRow->dwInOctets;
                        network.Bandwidth = pIfRow->dwSpeed;
                        break;
                    }
                }
            }
            free(pIfTable);

            networks.push_back(network);
            pAdapter = pAdapter->Next;
        }
    }
    else {
        //std::cerr << "GetAdaptersInfo failed.\n";
    }

    if (pAdapterInfo)
        free(pAdapterInfo);
#elif __linux__
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return ;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
            continue;

        Network network;
        network.Friendly_Name = ifa->ifa_name;
        network.Adapter_Desc = ""; // Still need to implement

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, ip, INET_ADDRSTRLEN);
        network.Ip_Address = ip;

        char mask[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr, mask, INET_ADDRSTRLEN);
        network.Ip_Mask = mask;

        network.Gateway = ""; // Still need to implement
        network.Dhcp_Server = ""; // Still need to implement

        // Read statistics from sysfs
        std::string sysfs_path = "/sys/class/net/" + network.Friendly_Name + "/statistics/";
        network.Send_Bytes = readSysfsValue(sysfs_path + "tx_bytes");
        network.Receive_Bytes = readSysfsValue(sysfs_path + "rx_bytes");

        // Get bandwidth (speed)
        std::ifstream speed_file("/sys/class/net/" + network.Friendly_Name + "/speed");
        if (speed_file.is_open()) {
            speed_file >> network.Bandwidth;
            //network.Bandwidth *= 1000000; // Convert from Mbps to bps
            //network.Bandwidth /= 8; //convert from bps to Bps
        } else {
            network.Bandwidth = 0;
        }

        networks.push_back(network);
    }

    freeifaddrs(ifaddr);
#endif
}
