#include "DroneWebServer.hpp"
#include "esp_log.h"
#include "portmacro.h"
#include <stdio.h>
#include <string>

static const char htmlPage[]  = R"rawliteral(
<html>
<body>
<h3>Drone Control</h3>
Frequency: <input type="range" min="0.1" max="10" value="1" step="0.1" id="freqSlider" oninput="updateFreq(this.value)">
<span id="freqVal">1</span> Hz
<canvas id="graph" width="500" height="200" style="border:1px solid #000"></canvas>
<script>
let canvas = document.getElementById("graph");
let ctx = canvas.getContext("2d");
let data = [];

async function updateData() {
    let r = await fetch('/data');
    let j = await r.json();
    if(data.length>=canvas.width) data.shift();
    data.push(j.value);
    drawGraph();
}

function drawGraph() {
    ctx.clearRect(0,0,canvas.width,canvas.height);
    ctx.beginPath();
    for(let i=0;i<data.length;i++){
        let y=canvas.height/2-data[i]*50;
        if(i==0) ctx.moveTo(i,y);
        else ctx.lineTo(i,y);
    }
    ctx.stroke();
}

function updateFreq(val){
    document.getElementById("freqVal").innerText = val;
    fetch('/setFreq?value='+val);
}

setInterval(updateData, 50);
</script>
</body>
</html>
)rawliteral";

DroneWebServer::DroneWebServer() : server(nullptr), drone(nullptr) {}

void DroneWebServer::start(Drone* dr) {
    drone = dr;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;
    if(httpd_start(&server, &config) != ESP_OK){
        ESP_LOGE("WebServer","Failed to start HTTP server");
        return;
    }

    httpd_uri_t uris[] = {
        { "/", HTTP_GET, indexHandler, this },
        { "/setFreq", HTTP_GET, setFreqHandler, this },
        { "/data", HTTP_GET, dataHandler, this }
    };

    for(auto &uri : uris)
        httpd_register_uri_handler(server, &uri);
}

void DroneWebServer::stop() {
    if(server) httpd_stop(server);
}

esp_err_t DroneWebServer::indexHandler(httpd_req_t *req) {
    httpd_resp_send(req, htmlPage, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t DroneWebServer::setFreqHandler(httpd_req_t *req) {
    DroneWebServer* self = (DroneWebServer*)req->user_ctx;
    char buf[32];
    
    if(httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK){
        char valStr[16];
        if(httpd_query_key_value(buf,"value",valStr,sizeof(valStr))==ESP_OK){
            float val = atof(valStr);
            self->drone->setFrequency(val);
        }
    }
    httpd_resp_send(req,"ok",HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t DroneWebServer::dataHandler(httpd_req_t *req) {
    DroneWebServer* self = (DroneWebServer*)req->user_ctx;
    float v = self->drone->getValue();
    char buf[64];
    snprintf(buf,sizeof(buf),"{\"value\": %.2f}",v);
    httpd_resp_set_type(req,"application/json");
    httpd_resp_send(req,buf,HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
