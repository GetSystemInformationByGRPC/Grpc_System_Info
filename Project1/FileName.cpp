#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <grpcpp/grpcpp.h>
#include <string>
#include <strsafe.h>
#include <iostream>
#include <thread>
#include <grpc++/grpc++.h>
#include "SysInfo.grpc.pb.h"
#include "sysinfo.h"



using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using namespace std;

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  _T("System Info Service")   

void InstallService();
void UninstallService();
void StartService();
void Console_App();


void InstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        std::wcout << L"OpenSCManager failed, error: " << GetLastError() << std::endl;
        return;
    }

    wchar_t path[MAX_PATH];
    if (!GetModuleFileName(NULL, path, MAX_PATH))
    {
        std::wcout << L"GetModuleFileName failed, error: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return;
    }

    SC_HANDLE schService = CreateService(
        schSCManager, SERVICE_NAME, SERVICE_NAME,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
        path, NULL, NULL, NULL, NULL, NULL);

    if (schService == NULL)
    {
        std::wcout << L"CreateService failed, error: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return;
    }

    std::wcout << L"Service installed successfully" << std::endl;

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

void UninstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        std::wcout << L"OpenSCManager failed, error: " << GetLastError() << std::endl;
        return;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (schService == NULL)
    {
        std::wcout << L"OpenService failed, error: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return;
    }

    // Stop the service if it's running
    SERVICE_STATUS ss;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ss))
    {
        std::wcout << L"Stopping service..." << std::endl;
        Sleep(1000);
    }

    if (DeleteService(schService))
    {
        std::wcout << L"Service uninstalled successfully" << std::endl;
    }
    else
    {
        std::wcout << L"DeleteService failed, error: " << GetLastError() << std::endl;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

void StartService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        std::wcout << L"OpenSCManager failed, error: " << GetLastError() << std::endl;
        return;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_START);
    if (schService == NULL)
    {
        std::wcout << L"OpenService failed, error: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return;
    }

    if (StartService(schService, 0, NULL))
    {
        std::wcout << L"Service started successfully" << std::endl;
    }
    else
    {
        std::wcout << L"StartService failed, error: " << GetLastError() << std::endl;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}




/**************************************GRPC SERVIC***************************************/

class SysInfoServiceImpl final : public SystemInfo::Service {
private:
    sysinfo systeminformation;
    sysinfo::CpuUtilization cpuut;
public:
    Status GetRAMinfo(ServerContext* context, const Empty* request,
        RAMResponse* reply) override {
        std::string peer = context->peer();
        std::cout << "Received request from: " << peer << std::endl;
        struct RAM ram;
        systeminformation.getRAMinfo(ram);
        reply->set_total_ram(ram.Total_RAM);
        reply->set_used_ram(ram.Used_RAM);
  
        return Status::OK;
    }
    Status GetCPUutilization(ServerContext* context, const Empty* request,
        CPUResponse* reply) override {
        reply->set_avg_cpu_utilization(cpuut.PrintAverageCpuUsage());
        return Status::OK;
    }
    Status GetDiskUsage(ServerContext* context, const Empty* request,
        DiskResponse* reply) override {
        std::vector<std::wstring> paths;
        systeminformation.ListDrives(paths);

        for (auto path : paths)
        {
            struct Disk disk;
            systeminformation.getDiskUsage(path, disk);
            DiskInfo* driveInfo = reply->add_drives();
            driveInfo->set_total_size(disk.Total_Size);
            driveInfo->set_path(disk.Path);
            driveInfo->set_used_space(disk.Used_Space);
    

        }
        return  Status::OK;

    }

    Status GetOsType(ServerContext* context, const Empty* request,
        OsResponse* reply) override {
        bool type = systeminformation.IsRunningInVirtualMachine();
        if (type) {
            reply->set_is_virtual_machine("true");
        }
        else {
            reply->set_is_virtual_machine("false");
        }
        return Status::OK;

    }

    Status GetNetworkAdapters(ServerContext* context, const Empty* request,
        NetworkResponse* reply) override {
        std::vector<struct Network> networks;
        systeminformation.printNetworkAdapterFriendlyNames(networks);
        for (auto network : networks)
        {
            NetworkAdapterInfo* networkInfo = reply->add_networks_adapter();
            networkInfo->set_friendly_name(network.Friendly_Name);
            networkInfo->set_adapter_desc(network.Adapter_Desc);
            networkInfo->set_ip_address(network.Ip_Address);
            networkInfo->set_ip_mask(network.Ip_Mask);
            networkInfo->set_gateway(network.Gateway);
            networkInfo->set_dhcp_server(network.Dhcp_Server);
        }
        return Status::OK;
    }


};
class Server_grpc {
public:
    void run() {
        std::string server_address("0.0.0.0:50051");
        SysInfoServiceImpl service;

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        server->Wait();
    }

    void stop() {
        server->Shutdown();
    }
private:
    std::unique_ptr<grpc::Server> server;
};

Server_grpc seGrpc;

/***************************************GRPC SERVICE**********************************************/

/****************************************Console APP*********************************************/
void Console_App()
{
    sysinfo systeminformation;
    sysinfo::CpuUtilization cpuu;
    int command = 1;
    while (command != 0)
    {
        cout << "\t***********************************************************\n";
        cout << "\t***************WELCOME TO CONSOLE APP *********************\n";
        cout << "\tPlease enter your command:\n\t1) Enter 1 for RAM informatio.\n\t2) Enter 2 for CPU Utilization\n\t3) Enter 3 for DISK information"
            "\n\t4) Enter 4 for Network Adapters\n\t5) Enter 5 for OS type\n\t0) Enter 0 for exit:\n\t";
        cout << "\n\t Please Enter Your Command:";
        cin >> command;
        if (command == 1)
        {
            RAM ram;
            systeminformation.getRAMinfo(ram);
            std::cout << "\n\t Total RAM Size In GB : " << systeminformation.ByteToGb(ram.Total_RAM) << endl;
            std::cout << "\n\t Used RAM Size In GB : " << systeminformation.ByteToGb(ram.Used_RAM) << endl;
            std::cout << "\n\t Precent Of RAM Usage : " << (ram.Total_RAM / ram.Used_RAM) * 100 << endl;
        }
        else if (command == 2)
        {
            cpuu.PrintAverageCpuUsage();

        }
        else if (command == 3)
        {
            std::vector<std::wstring> paths;
            systeminformation.ListDrives(paths);
            for (auto path : paths)
            {
                Disk disk;
                systeminformation.getDiskUsage(path, disk);
            }
        }
        else if (command == 4)
        {
            std::vector<Network> network;
            systeminformation.printNetworkAdapterFriendlyNames(network);
        }
        else if (command == 5)
        {
            systeminformation.IsRunningInVirtualMachine();
            
        }
        else if (command == 0)
            break;

    }

}

/****************************************Console APP*********************************************/













VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;

    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T(
            "My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    /*
     * Perform tasks necessary to start the service here
     */

     // Create a service stop event to wait on later
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        // Error creating event
        // Tell service controller we are stopped and exit
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T(
                "My Sample Service: ServiceMain: SetServiceStatus returned error"));
        }
        return;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T(
            "My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    // Start a thread that will perform the main task of the service
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    // Wait until our worker thread exits signaling that the service needs to stop
    WaitForSingleObject(hThread, INFINITE);


    /*
     * Perform any cleanup tasks
     */

    CloseHandle(g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T(
            "My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks necessary to stop the service here
         */
        seGrpc.stop();

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T(
                "My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;

    default:
        break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{

    //std::thread grpcThread(server);
    //grpcServer = new GrpcServerWrapper("0.0.0.0:50051");
    //grpcServer->Start();
    
    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        /*
         * Perform main service function here
         */
        seGrpc.run();

         //  Simulate some work by sleeping
        Sleep(3000);
    }
    //grpcServer->Stop();
    //delete grpcServer;
    //grpcThread.join();
    //server_grpc->Shutdown();
    return ERROR_SUCCESS;
}




int _tmain(int argc, TCHAR* argv[])
{
    if (argc > 1)
    {
        if (_tcscmp(argv[1], _T("-i")) == 0)
        {
            InstallService();
            StartService();
            return 0;
        }
        else if (_tcscmp(argv[1], _T("-u")) == 0)
        {
            UninstallService();
            return 0;
        }
        else if (_tcscmp(argv[1], _T("-c")) == 0)
        {
            Console_App();
        }
        else if (_tcscmp(argv[1], _T("-s")) == 0)
        {
            seGrpc.run();

        }
    }
    wchar_t sn[] = L"System Info Service";
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {sn, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        return GetLastError();
    }

    return 0;
}
