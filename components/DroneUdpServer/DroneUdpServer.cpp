#include "DroneUdpServer.hpp"

static const char* TAG = "UDPServer";

DroneUdpServer::DroneUdpServer() :
    drone(nullptr), sock(-1), clientKnown(false)
{
    memset(&clientAddr, 0, sizeof(clientAddr));
}

void DroneUdpServer::start(Drone* dr) {
    drone = dr;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock < 0){
        ESP_LOGE(TAG, "socket create failed");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5005);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0){
        ESP_LOGE(TAG, "bind failed");
        return;
    }

    xTaskCreatePinnedToCore(recvTask, "recvTask", 4096, this, 5, nullptr, 0);
    xTaskCreatePinnedToCore(sendTask, "sendTask", 4096, this, 5, nullptr, 0);
}

// --- receive commands from client ---
void DroneUdpServer::recvTask(void* arg) {
    auto* self = (DroneUdpServer*)arg;

    char buf[64];
    sockaddr_in source{};
    socklen_t slen = sizeof(source);

    for(;;){
        int n = recvfrom(self->sock, buf, sizeof(buf)-1, 0,
                         (sockaddr*)&source, &slen);

        if(n <= 0) continue;

        buf[n] = 0;

        if(!self->clientKnown){
            self->clientAddr = source;
            self->clientKnown = true;
        }

        // expected format: ANGLES=roll,pitch,yaw
        if(strncmp(buf, "ANGLES=", 7) == 0){
            float roll, pitch, yaw;
            if(sscanf(buf + 7, "%f,%f,%f", &roll, &pitch, &yaw) == 3){
                Angles target{roll, pitch, yaw};
                self->drone->setTargetAngles(target);
            }
        }
    }
}

// --- send measured angles to client ---
void DroneUdpServer::sendTask(void* arg) {
    auto* self = (DroneUdpServer*)arg;

    char pkt[64];
    Angles measured;

    for(;;){
        if(self->clientKnown){
            if(self->drone->getMeasuredAngles(measured)){
                int len = snprintf(pkt, sizeof(pkt), "%.2f,%.2f,%.2f",
                                   measured.roll, measured.pitch, measured.yaw);
                sendto(self->sock, pkt, len, 0,
                       (sockaddr*)&self->clientAddr, sizeof(self->clientAddr));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // ~50 Hz
    }
}
