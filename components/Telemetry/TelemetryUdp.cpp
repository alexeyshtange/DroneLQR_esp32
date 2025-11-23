#include "TelemetryUdp.hpp"

TelemetryUdp::TelemetryUdp() {
    measuredQueue = xQueueCreate(1, sizeof(Angles));
    targetQueue   = xQueueCreate(1, sizeof(Angles));
    controlOutputQueue = xQueueCreate(1, sizeof(ControlOutput));
}

TelemetryUdp::~TelemetryUdp() {
    stop();
    if(measuredQueue) vQueueDelete(measuredQueue);
    if(targetQueue) vQueueDelete(targetQueue);
    if(controlOutputQueue) vQueueDelete(controlOutputQueue);
}

void TelemetryUdp::start() {
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

    running = true;
    xTaskCreatePinnedToCore(recvTask, "recvTask", 4096, this, 5, &recvTaskHandle, 0);
    xTaskCreatePinnedToCore(sendTask, "sendTask", 4096, this, 5, &sendTaskHandle, 0);
}

void TelemetryUdp::stop() {
    running = false;

    if(recvTaskHandle) vTaskDelete(recvTaskHandle);
    if(sendTaskHandle) vTaskDelete(sendTaskHandle);

    if(sock >= 0) {
        close(sock);
        sock = -1;
    }

    disconnectFromClient();
}

void TelemetryUdp::putMeasuredAngles(const Angles& measured) {
    xQueueOverwrite(measuredQueue, &measured);
}

void TelemetryUdp::getTargetAngles(Angles& target) {
    xQueuePeek(targetQueue, &target, 0);
}

void TelemetryUdp::putControlOutput(const ControlOutput& control) {
    xQueueOverwrite(controlOutputQueue, &control);
}

void TelemetryUdp::disconnectFromClient() {
    clientKnown = false;
    memset(&clientAddr, 0, sizeof(clientAddr));
    lastClientTick = 0;
}

void TelemetryUdp::checkClientTimeout() {
    if(clientKnown){
        TickType_t now = xTaskGetTickCount();
        if(now - lastClientTick > pdMS_TO_TICKS(5000)){
            disconnectFromClient();
        }
    }
}

// --- receive commands from client ---
void TelemetryUdp::recvTask(void* arg) {
    auto* self = static_cast<TelemetryUdp*>(arg);

    char buf[64];
    sockaddr_in source{};
    socklen_t slen = sizeof(source);

    while(self->running){
        int n = recvfrom(self->sock, buf, sizeof(buf)-1, 0, (sockaddr*)&source, &slen);
        if(n <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        buf[n] = 0;

        if(!self->clientKnown){
            self->clientAddr = source;
            self->clientKnown = true;
        }

        self->lastClientTick = xTaskGetTickCount();

        if(strncmp(buf, "DISCONNECT", 10) == 0){
            self->disconnectFromClient();
            continue;
        }

        // expected format: ANGLES=roll,pitch,yaw
        if(strncmp(buf, "ANGLES=", 7) == 0){
            float roll, pitch, yaw;
            if(sscanf(buf + 7, "%f,%f,%f", &roll, &pitch, &yaw) == 3){
                Angles target{roll, pitch, yaw};
                xQueueOverwrite(self->targetQueue, &target);
            }
        }
    }
}

// --- send measured angles to client ---
void TelemetryUdp::sendTask(void* arg) {
    auto* self = static_cast<TelemetryUdp*>(arg);

    char pkt[64];
    Angles measured;

    while(self->running){
        self->checkClientTimeout();

        if(self->clientKnown){
            if(xQueuePeek(self->measuredQueue, &measured, 0) == pdTRUE){
                int len = snprintf(pkt, sizeof(pkt), "ANGLES=%.2f,%.2f,%.2f",
                                   measured.roll, measured.pitch, measured.yaw);
                sendto(self->sock, pkt, len, 0, (sockaddr*)&self->clientAddr, sizeof(self->clientAddr));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // ~20 Hz
    }
}
