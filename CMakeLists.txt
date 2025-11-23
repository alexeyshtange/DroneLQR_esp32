cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(DroneLQR)

set(EXTRA_COMPONENT_DIRS components/Drone 
						 components/WiFiManager 
						 components/Telemetry 
						 components/Sampler 
						 components/Sample 
						 components/ControlTimer 
						 components/Filter
						 components/Controller
						 components/Motor)