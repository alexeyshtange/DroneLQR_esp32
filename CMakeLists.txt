cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(DroneLQR)

set(EXTRA_COMPONENT_DIRS components/Drone 
						 components/WiFiManager 
						 components/DroneUdpServer 
						 components/Sampler 
						 components/SamplingTimer 
						 components/Filter
						 components/Controller
						 components/Motor)