

/*
class SysInfoServiceImpl final : public SystemInfo::Service{
private:
    sysinfo systeminformation;
    sysinfo::CpuUtilization cpuut;
public:
    Status GetRAMinfo(ServerContext* context, const RAMRequest* request,
        RAMResponse* reply) override {
        struct RAM ram;
        systeminformation.getRAMinfo(ram);
        reply->set_totalramingb(ram.totalRAMinGB);
        reply->set_totalraminmb(ram.totalRAMinMB);
        reply->set_usedramingb(ram.usedRAMinGB);
        reply->set_usedraminmb(ram.usedRAMinMB);

        return Status::OK;
    }
    Status GetCPUutilization(ServerContext* context, const CPURequest* request,
        CPUResponse* reply) override {
        reply->set_avgcpuusage(cpuut.PrintAverageCpuUsage());
        return Status::OK;
    }
    Status GetDiskUsage(ServerContext* context, const DiskRequest* request,
        DiskResponse* reply) override {
        std::vector<std::wstring> paths;
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

    Status GetOsType(ServerContext* context, const OsRequest* request,
        OsResponse* reply) override {
        bool type = systeminformation.IsRunningInVirtualMachine();
        if (type) {
            reply->set_isvirtualmachine("The system is running in a virtual machine.");
        }
        else {
            reply->set_isvirtualmachine("The system is running on physical hardware.");
        }
        return Status::OK;

    }

    Status GetNetworkAdapters(ServerContext* context, const NetworkRequest* request,
        NetworkResponse* reply) override {
        std::vector<struct Network> networks;
        systeminformation.getNetworkInfo(networks);
        for (auto network : networks)
        {
            NetworkAdapterInfo* networkInfo = reply->add_networks_adapter();
            networkInfo->set_adapter_name(network.Adapter_name);
            networkInfo->set_adapter_desc(network.Adapter_Desc);
            networkInfo->set_adapter_addr(network.Adapter_addr);
            networkInfo->set_ip_address(network.ip_mask);
            networkInfo->set_gateway(network.gateway);
            networkInfo->set_dhcp_enable(network.dhcp_enable);
            networkInfo->set_dhpc_server(network.dhpc_server);
        }
        return Status::OK;
    }


};

void server() {
    std::string server_address("0.0.0.0:50051");
    SysInfoServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
}
*/
// Client function to be run in a separate thread
/*
void client() {
    // Sleep to ensure the server is up before the client sends a request
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for server to start

    std::string target_address = "localhost:50051";
    std::unique_ptr<SystemInfo::Stub> greeter = SystemInfo::NewStub(grpc::CreateChannel(
        target_address, grpc::InsecureChannelCredentials()));

    //RAMRequest request;
    //RAMResponse reply;

    NetworkRequest request;
    NetworkResponse reply;
    

    ClientContext context;

    // The actual RPC
    //Status status = greeter->GetRAMinfo(&context, request, &reply);
    Status status = greeter->GetNetworkAdapters(&context, request, &reply);

    // Act upon its status
    if (status.ok()) {
        for (const auto& network : reply.networks_adapter()) {
          
            std::cout << "Client received:\n Adapter Name  : "<< network.adapter_name() << "\n Adapter Desc :  " << network.adapter_desc() << " GB\n"
            <<" Adapter Addr : "<< network.adapter_addr() << "\n Ip Address: " << network.ip_address() << "\n Ip Mask: "
                << network.ip_mask() << "\n Gateway : " << network.gateway() << "DHCP Enabled" << network.dhcp_enable() << 
                 "\n DHCP Server" << network.dhpc_server() << std::endl;
        }
        
        //std::cout << reply.isvirtualmachine();
    }
    else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    }
}*/
/*
int main() {
    // Create a thread for the server
    //std::thread server_thread(server);

    // Create a thread for the client
    //std::thread client_thread(client);
    client();
    // Join both threads to ensure they complete execution
    //server_thread.join();
    //client_thread.join();

    return 0;
}
*/
