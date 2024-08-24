﻿#include <iostream>
#include <grpcpp/grpcpp.h>
#include <grpc/grpc_security.h>
#include <string>
#include <iostream>
#include <thread>
#include <grpc++/grpc++.h>
#include "SysInfo.grpc.pb.h"
#include "SysInfo.pb.h"
#include "sysinfo.h"
#include <fstream>
#ifdef _WIN32
#include <strsafe.h>
#include <clipp.h>
#include <Windows.h>
#include <tchar.h>
#endif

#ifdef __linux__
#include "clipp.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using namespace std;

#ifdef _WIN32
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



void InstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        std::string error = "OpenSCManager failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
        return;
    }

    wchar_t path[MAX_PATH];
    if (!GetModuleFileName(NULL, path, MAX_PATH))
    {   
        std::string error = "GetModuleFileName failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
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
        std::string error = "CreateService failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
        CloseServiceHandle(schSCManager);
        return;
    }
    Logger::debug("Service installed successfully");

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

void UninstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {   
        std::string error = "OpenSCManager failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
        return;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (schService == NULL)
    {
        std::string error = "OpenService failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
        CloseServiceHandle(schSCManager);
        return;
    }

    // Stop the service if it's running
    SERVICE_STATUS ss;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ss))
    {
        Logger::debug("Stopping service...");
        Sleep(1000);
    }

    if (DeleteService(schService))
    {
        Logger::debug("Service uninstalled successfully");
    }
    else
    {
        std::string error = "DeleteService failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

void StartService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        std::string error = "OpenSCManager failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
        
        return;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_START);
    if (schService == NULL)
    {
        std::string error = "OpenService failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
        CloseServiceHandle(schSCManager);
        return;
    }

    if (StartService(schService, 0, NULL))
    {
        Logger::debug("Service started successfully");
    }
    else
    {
        std::string error = "StartService failed, error: " + sysinfo::convertToString(GetLastError());
        Logger::error(error);
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}


#endif

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
        Logger::debug("Received request from: " + peer);
        struct RAM ram;
        systeminformation.getRAMinfo(ram);
        reply->set_total_ram(ram.Total_RAM);
        reply->set_used_ram(ram.Used_RAM);
  
        return Status::OK;
    }
    Status GetCPUutilization(ServerContext* context, const Empty* request,
        CPUResponse* reply) override {
        Logger::debug("**************************************Request for CPU Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer);

        reply->set_avg_cpu_utilization(cpuut.PrintAverageCpuUsage());
        return Status::OK;
    }
    Status GetDiskUsage(ServerContext* context, const Empty* request,
        DiskResponse* reply) override {
        std::vector<std::wstring> paths;

        Logger::debug("**************************************Request for Disk Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer);
#ifdef _WIN32
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
#elif __linux__
        struct Disk disk;
        systeminformation.getDiskUsage(L"", disk);
        DiskInfo* driveInfo = reply->add_drives();
        driveInfo->set_total_size(disk.Total_Size);
        driveInfo->set_path(disk.Path);
        driveInfo->set_used_space(disk.Used_Space);
#endif
        return  Status::OK;


    }

    Status GetOsType(ServerContext* context, const Empty* request,
        OsResponse* reply) override {

        Logger::debug("**************************************Request for OS Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer);


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

        Logger::debug("**************************************Request for Network Adaptor Information**************************************");
        std::string peer = context->peer();
        Logger::debug("Received request from: " + peer);

        std::vector<struct Network> networks;
        systeminformation.printNetworkAdapterFriendlyNames(networks);
        for (Network network : networks)
        {
            NetworkAdapterInfo* networkInfo = reply->add_networks_adapter();
            networkInfo->set_friendly_name(network.Friendly_Name);
            networkInfo->set_adapter_desc(network.Adapter_Desc);
            networkInfo->set_ip_address(network.Ip_Address);
            networkInfo->set_ip_mask(network.Ip_Mask);
            networkInfo->set_gateway(network.Gateway);
            networkInfo->set_dhcp_server(network.Dhcp_Server);
            networkInfo->set_send_bytes(network.Send_Bytes);
            networkInfo->set_received_bytes(network.Receive_Bytes);
            networkInfo->set_bandwidth(network.Bandwidth);
           
        }
        return Status::OK;
    }


};
class Server_grpc {
public:
    std::string LoadFile(const std::string& file_path) {
        std::ifstream file(file_path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    void run() {
        std::string server_address("0.0.0.0:50051");
        SysInfoServiceImpl service;

        /*
        grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair = {
            LoadFile("server.key"),
            LoadFile("server.crt")
        };
        grpc::SslServerCredentialsOptions ssl_opts;
        ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
        ssl_opts.pem_root_certs = LoadFile("ca.crt");
        */
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        //builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
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

    Logger::debug("System is running in console...");

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
        }
        else if (command == 2)
        {
            std::cout << "Average CPU Usage: " << cpuu.PrintAverageCpuUsage() << "%" << std::endl;
         

        }
        else if (command == 3)
        {
#ifdef _WIN32
            std::vector<std::wstring> paths;

            systeminformation.ListDrives(paths);

            for (auto path : paths)
            {
                Disk disk;
                systeminformation.getDiskUsage(path, disk);
                std::cout << "\n\t Path of Drive : " << disk.Path << endl;
                std::cout << "\n\t Total Size in GB : " << systeminformation.ByteToGb(disk.Total_Size) << endl;
                std::cout << "\n\t Used Size in GB : " << systeminformation.ByteToGb(disk.Used_Space) << endl;
                std::cout << "\n\t Free Size in GB : " << systeminformation.ByteToGb(disk.Total_Size) - systeminformation.ByteToGb(disk.Used_Space) << endl;
                std::cout << "\t********************************************\t" << endl;

            }
#elif __linux__
            Disk disk;
            systeminformation.getDiskUsage(L"",disk);
            std::cout << "\n\t Path of Drive : " << disk.Path << endl;
            std::cout << "\n\t Total Size in GB : " << systeminformation.ByteToGb(disk.Total_Size) << endl;
            std::cout << "\n\t Used Size in GB : " << systeminformation.ByteToGb(disk.Used_Space) << endl;
            std::cout << "\n\t Free Size in GB : " << systeminformation.ByteToGb(disk.Total_Size) - systeminformation.ByteToGb(disk.Used_Space) << endl;
            std::cout << "\t********************************************\t" << endl;
#endif
        }
        else if (command == 4)
        {
            std::vector<Network> networks;
            systeminformation.printNetworkAdapterFriendlyNames(networks);
            for (Network network : networks)
            {
                std::cout << "\n\t Adapter Name : " << network.Friendly_Name << endl;
                std::cout << "\n\t Adapter Descrition : " << network.Adapter_Desc << endl;
                std::cout << "\n\t Ip Address : " << network.Ip_Address << endl;
                std::cout << "\n\t Ip Mask : " << network.Gateway << endl;
                std::cout << "\n\t Dhcp Server : " << network.Dhcp_Server << endl;
                std::cout << "\n\t Send Byte : " << network.Send_Bytes << endl;
                std::cout << "\n\t Received Byte : " << network.Receive_Bytes << endl;
                std::cout << "\n\t Bandwidth : " << network.Bandwidth << endl;
                std::cout << "\t********************************************\t" << endl;
            }
        }
        else if (command == 5)
        {
            if (systeminformation.IsRunningInVirtualMachine()) {
                std::cout << "\n\t The system is running in a virtual machine." << std::endl;
            }
            else {
                std::cout << "\n\t The system is running on physical hardware." << std::endl;
            }
            
            
        }
        else if (command == 0)
            break;

    }

}

/****************************************Console APP*********************************************/










#ifdef _WIN32


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
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
        std::wstring ws(argv[i]);
        std::string str(ws.begin(), ws.end());
        args.push_back(str);
    }

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

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        return GetLastError();
    }

    return 0;
}
#elif __linux__
void installService(const std::string &serviceName, const std::string &servicePath) {

    std::string serviceFilePath = "/etc/systemd/system/" + serviceName + ".service";

    std::string serviceFileContent =
        "[Unit]\n"
        "Description=My C++ Daemon Service\n"
        "After=network.target\n\n"
        "[Service]\n"
        "ExecStart=" + servicePath + "\n"
        "Restart=always\n"
        "RestartSec=5\n\n"
        "[Install]\n"
        "WantedBy=multi-user.target\n";

    std::ofstream serviceFile(serviceFilePath);
    if (!serviceFile.is_open()) {
        std::cerr << "Error: Could not create service file." << std::endl;
        exit(1);
    }
    serviceFile << serviceFileContent;
    serviceFile.close();

    std::system("systemctl daemon-reload");
    std::system(("systemctl enable " + serviceName).c_str());
    std::system(("systemctl start " + serviceName).c_str());

    std::cout << "Service installed and started successfully!" << std::endl;
}

void uninstallService(const std::string &serviceName) {
    std::system(("systemctl stop " + serviceName).c_str());

    std::system(("systemctl disable " + serviceName).c_str());

    std::string serviceFilePath = "/etc/systemd/system/" + serviceName + ".service";
    if (remove(serviceFilePath.c_str()) != 0) {
        std::cerr << "Error: Could not delete service file." << std::endl;
    } else {
        std::cout << "Service file deleted successfully!" << std::endl;
    }

    std::system("systemctl daemon-reload");

    std::cout << "Service uninstalled successfully!" << std::endl;
}


int main(int argc, char* argv[]) {

    if (argc > 1 && std::string(argv[1]) == "-i") {

        char result[1024];
        ssize_t count = readlink("/proc/self/exe", result, sizeof(result) - 1);
        if (count != -1) {
            result[count] = '\0';
            std::string binaryPath(result);

            installService("Sys_Info_Service", binaryPath);
        } else {
            std::cerr << "Error: Could not determine binary path." << std::endl;
            return 1;
        }
    } else if (argc > 1 && std::string(argv[1]) == "-u") {
        uninstallService("Sys_Info_Service");
    } else if (argc > 1 && std::string(argv[1]) == "-s") {
        seGrpc.run();
    }
    else if (argc > 1 && std::string(argv[1]) == "-c") {
        Console_App();
    }
    else {

        while (true) {
            //std::cout << "Running as a service..." << std::endl;
            seGrpc.run();
            sleep(1);
        }
    }

    return 0;
}
#endif