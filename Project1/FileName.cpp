#include <iostream>
#include <clipp.h>
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
#include "Logger.h"
#include <sstream>
#include <sstream>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstring>


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

std::string convertToString(DWORD error_code) {
    std::ostringstream oss;
    oss << "OpenSCManager failed, error: " << error_code;
    return oss.str();
}

void InstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        std::string error = "OpenSCManager failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
        return;
    }

    wchar_t path[MAX_PATH];
    if (!GetModuleFileName(NULL, path, MAX_PATH))
    {   

        std::string error = "GetModuleFileName failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
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
        std::string error = "CreateService failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
        CloseServiceHandle(schSCManager);
        return;
    }

    Logger::debug("Service installed successfully", true, true);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

void UninstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {   
        std::string error = "OpenSCManager failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
        return;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (schService == NULL)
    {
        std::string error = "OpenService failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
        CloseServiceHandle(schSCManager);
        return;
    }

    // Stop the service if it's running
    SERVICE_STATUS ss;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ss))
    {   
        Logger::debug("Stopping service...", true, true);
        Sleep(1000);
    }

    if (DeleteService(schService))
    {   
        Logger::debug("Service uninstalled successfully", true, true);
    }
    else
    {   
        std::string error = "DeleteService failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

void StartService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {   
        std::string error = "OpenSCManager failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
        return;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_START);
    if (schService == NULL)
    {   
        std::string error = "OpenService failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
        CloseServiceHandle(schSCManager);
        return;
    }

    if (StartService(schService, 0, NULL))
    {   
        Logger::debug("Service started successfully", true, true);
    }
    else
    {   
        std::string error = "StartService failed, error: " + convertToString(GetLastError());
        Logger::error(error, true, true);
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
        

        Logger::debug("**************************************Request for Ram Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: "+peer, true, true);

        struct RAM ram;
        systeminformation.getRAMinfo(ram);
        reply->set_totalramingb(ram.totalRAMinGB);
        reply->set_totalraminmb(ram.totalRAMinMB);
        reply->set_usedramingb(ram.usedRAMinGB);
        reply->set_usedraminmb(ram.usedRAMinMB);
        return Status::OK;
    }
    Status GetCPUutilization(ServerContext* context, const Empty* request,
        CPUResponse* reply) override {
        Logger::debug("**************************************Request for CPU Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer, true, true);

        reply->set_avgcpuusage(cpuut.PrintAverageCpuUsage());
        return Status::OK;
    }
    Status GetDiskUsage(ServerContext* context, const Empty* request,
        DiskResponse* reply) override {
        std::vector<std::wstring> paths;
        Logger::debug("**************************************Request for Disk Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer, true, true);

        systeminformation.ListDrives(paths);

        for (auto path : paths)
        {
            struct Disk disk;
            systeminformation.getDiskUsage(path, disk);
            DiskInfo* driveInfo = reply->add_drives();
            driveInfo->set_totalsize(disk.totalsize);
            driveInfo->set_path(disk.path);
            driveInfo->set_usedspace(disk.usedspace);
            driveInfo->set_usedpercent(disk.percentage);
            driveInfo->set_freespace(disk.freespace);

        }
        return  Status::OK;

    }

    Status GetOsType(ServerContext* context, const Empty* request,
        OsResponse* reply) override {
        Logger::debug("**************************************Request for OS Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer, true, true);

        bool type = systeminformation.IsRunningInVirtualMachine();

        if (type) {
            reply->set_isvirtualmachine("The system is running in a virtual machine.");
        }
        else {
            reply->set_isvirtualmachine("The system is running on physical hardware.");
        }
        return Status::OK;

    }

    Status GetNetworkAdapters(ServerContext* context, const Empty* request,
        NetworkResponse* reply) override {
        std::vector<struct Network> networks;
        Logger::debug("**************************************Request for Network Adaptor Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer, true, true);

        systeminformation.printNetworkAdapterFriendlyNames(networks);
        

        for (auto network : networks)
        {
            NetworkAdapterInfo* networkInfo = reply->add_networks_adapter();
            networkInfo->set_friendly_name(network.friendly_name);
            networkInfo->set_adapter_desc(network.Adapter_Desc);
            networkInfo->set_ip_address(network.ip_address);
            networkInfo->set_ip_mask(network.ip_mask);
            networkInfo->set_gateway(network.gateway);
            networkInfo->set_dhcp_server(network.dhcp_server);
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
        Logger::debug("Server listening on "+server_address, true, true);

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
        std::cout << "\t*************************************************************************************\n";
        std::cout << "\t****************************WELCOME TO CONSOLE APP **********************************\n";
        std::cout << "\t*************************************************************************************\n\n\n";

        std::cout <<"\t\t1) Enter 1 for RAM informatio.\n\t\t2) Enter 2 for CPU Utilization\n\t\t3) Enter 3 for DISK information"
            "\n\t\t4) Enter 4 for Network Adapters\n\t\t5) Enter 5 for OS type\n\t\t0) Enter 0 for exit:";
        std::cout << "\n\t\tPlease Enter Your Command:";
        cin >> command;
        if (command == 1)
        {
            RAM ram;
            systeminformation.getRAMinfo(ram);
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
    
    // تبدیل wchar_t* به std::string
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
        std::wstring ws(argv[i]);
        std::string str(ws.begin(), ws.end());
        args.push_back(str);
    }

    // تبدیل std::vector<std::string> به آرایه‌ای از char*
    std::vector<const char*> cargs;
    for (const auto& arg : args) {
        cargs.push_back(arg.c_str());
    }

    bool install = false;
    bool uninstall = false;
    bool console = false;
    bool service = false;
    bool help = false;

    auto cli = (
        clipp::option("-i", "--install").set(install).doc("Install and start the service"),
        clipp::option("-u", "--uninstall").set(uninstall).doc("Uninstall the service"),
        clipp::option("-c", "--console").set(console).doc("Run the application in console mode"),
        clipp::option("-s", "--service").set(service).doc("Run the gRPC server"),
        clipp::option("-h", "--help").set(help).doc("Display this help message")

        );

    // استفاده از clipp::parse با آرگومان‌های char*
    if (!clipp::parse(static_cast<int>(cargs.size()), const_cast<char**>(cargs.data()), cli)) {
        std::cout << "Usage:\n" << clipp::usage_lines(cli, "ServiceApp") << "\n";
        return 0;
    }

    if (install) {
        InstallService();
        StartService();
        return 0;
    }

    if (uninstall) {
        UninstallService();
        return 0;
    }

    if (console) {
        Console_App();
        return 0;
    }

    if (service) {
        seGrpc.run();
        return 0;
    }
    if (help) {
        std::cout << "Usage:\n\n" << clipp::usage_lines(cli, "ServiceApp") << "\n\n\n\n";
        std::cout << "Detailed Documentation:\n\n" << clipp::documentation(cli) << "\n\n";
       
        return 0;
    }
    wchar_t sn[] = L"System Info Service";
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {sn, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
        return GetLastError();
    }

    return 0;
}
