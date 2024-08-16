#include "sysinfo.h"
#include <sstream>
#include "Logger.h"

std::string convertToString1(DWORD error_code) {
    std::ostringstream oss;
    oss << "OpenSCManager failed, error: " << error_code;
    return oss.str();
}

std::string sysinfo::wstring_to_string(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

void sysinfo::getRAMinfo(struct RAM &ram)
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    
    if (GlobalMemoryStatusEx(&statex)) {
        uint64_t totalRAMinGB = statex.ullTotalPhys / (1024 * 1024 * 1024);
        uint64_t usedRAMinGB = (statex.ullTotalPhys - statex.ullAvailPhys) / (1024 * 1024 * 1024);

        uint64_t  totalRAMinMB = statex.ullTotalPhys / (1024 * 1024);
        uint64_t usedRAMinMB = (statex.ullTotalPhys - statex.ullAvailPhys) / (1024 * 1024);
        
        //std::cout << "Total RAM: " << totalRAMinGB << " GB" << std::endl;
        //std::cout << "Used RAM: " << usedRAMinGB << " GB" << std::endl;

        //std::cout << "Total RAM: " << totalRAMinMB << " MB" << std::endl;
        //std::cout << "Used RAM: " << usedRAMinMB << " MB" << std::endl;
        //std::cout << "Percent Of RAM Usage: " << (usedRAMinMB * 100) / totalRAMinMB << "% " << std::endl;

        /*
        std::string info = "Total RAM: " + std::to_string(totalRAMinGB) + " GB";
        Logger::debug(info);

        info = "Used RAM: " + std::to_string(usedRAMinGB) + " GB";
        Logger::debug(info);

        info = "Total RAM: " + std::to_string(totalRAMinMB) + " MB";
        Logger::debug(info);

        info = "Used RAM: " + std::to_string(usedRAMinMB) + " MB";
        Logger::debug(info);

        info = "Percent Of RAM Usage: " + std::to_string((usedRAMinMB * 100) / totalRAMinMB) + "% ";
        Logger::debug(info);
        */
        ram.totalRAMinGB = totalRAMinGB;
        ram.totalRAMinMB = totalRAMinMB;
        ram.usedRAMinGB = usedRAMinGB;
        ram.usedRAMinMB = usedRAMinMB;
    }
    else {
        Logger::error("Failed to get memory status.", true, true);
    }
}

void sysinfo::ListDrives(std::vector<std::wstring>& paths) {
    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        Logger::error("GetLogicalDrives failed with error: " + convertToString1(GetLastError()), true, true);
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

        disk.path = wstring_to_string(path);
        disk.totalsize = totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024);
        disk.usedspace = usedBytes / (1024 * 1024 * 1024);
        disk.freespace = totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024);
        disk.percentage = usedPercentage;
        /*std::string msg;
        msg = "Disk Path: " + wstring_to_string(path);
        Logger::debug(msg, true, true);
        msg = "Total Size: " + std::to_string((totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024))) + " GB";
        Logger::debug(msg, true, true);
        msg = "Used Space: " + std::to_string(usedBytes / (1024 * 1024 * 1024)) + " GB";
        Logger::debug(msg, true, true);
        msg = "Free Space: " + std::to_string(totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024)) + " GB";
        Logger::debug(msg, true, true);
        msg = "Percentage Used: " + std::to_string(usedPercentage) + "%";
        Logger::debug(msg, true, true);*/
    }
    else {
        Logger::error("Failed to get disk space information.", true, true);
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
        //Logger::debug("The system is running in a virtual machine.", true, true);
    }
    else {
        //Logger::debug("The system is running on physical hardware.", true, true);
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
            std::string error = "Failed to query registry value. Error: " + std::to_string(lResult);
            Logger::error(error, true, true);
        }

        RegCloseKey(hKey);
    }
    else {
        std::string error = "Failed to open registry key. Error: " + std::to_string(lResult);
        Logger::error(error, true, true);
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
        Logger::error("Error allocating memory for GetAdaptersInfo", true, true);
        return;
    }

    // Make an initial call to GetAdaptersInfo to get the necessary size
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            std::cerr << "Error allocating memory for GetAdaptersInfo\n";
            Logger::error("Error allocating memory for GetAdaptersInfo", true, true);
            return;
        }
    }

    // Get the actual data
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            struct Network network;
            std::string fname = GetAdapterFriendlyName(pAdapter);
           /* std::string info = "Adapter Desc: " + std::string(pAdapter->Description);
            Logger::debug(info, true, true);
            info = "IP Address: " + std::string(pAdapter->IpAddressList.IpAddress.String);
            Logger::debug(info, true, true);
            info = "IP Mask: " + std::string(pAdapter->IpAddressList.IpMask.String);
            Logger::debug(info, true, true);
            info = "Gateway: " + std::string(pAdapter->GatewayList.IpAddress.String);*/
            network.friendly_name = fname;
            network.Adapter_Desc = pAdapter->Description;
            network.ip_address = pAdapter->IpAddressList.IpAddress.String;
            network.ip_mask = pAdapter->IpAddressList.IpMask.String;
            network.gateway = pAdapter->GatewayList.IpAddress.String;


            if (pAdapter->DhcpEnabled) {
                /*std::string info = "DHCP Server: " + std::string(pAdapter->DhcpServer.IpAddress.String);
                Logger::debug(info, true, true);*/
                network.dhcp_server = pAdapter->DhcpServer.IpAddress.String;
            }
            else {
                //Logger::debug("DHCP Enabled: No", true, true);
                network.dhcp_server = "Dhcp servet disabled";
            }

            networks.push_back(network);
            pAdapter = pAdapter->Next;
        }
    }
    else {
        Logger::error("GetAdaptersInfo failed.", true, true);
    }

    if (pAdapterInfo)
        free(pAdapterInfo);
}

