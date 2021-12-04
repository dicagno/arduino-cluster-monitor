#ClusterMonitor

Arduino-based monitor for racks/clusters, exposing Prometheus-compatible metrics and acting as a Modbus TCP slave as well.

##Supported metrics

1. Temperature & relative humidity, based on DHT22 (AM2302)
2. DC current, based on ACS712

##Supported protocols

1. Modbus TCP - available on port 502, exposes the following `16-bit Holding Registers`:
    1. offset 0 - temperature * 100
    2. offset 1 - humidity * 100
    3. offset 2 - current sense on A0 pin * 1000
    4. offset 2 - current sense on A1 pin * 1000
2. HTTP - available on port 8080, exposes the following endpoints:
    1. `/health` returns `HTTP 204 No Content` to show that the device is up and running
    2. `/metrics` returns OpenMetrics data, according to the following sample:
        ```text
        cluster_rack_temperature 22.60
        cluster_rack_humidity 44.80
        cluster_rack_current{bus="5v"} 0.22
        cluster_rack_current{bus="12v"} 1.25
        ```
##Device ID
Hardcoded MAC address is `90:A2:DA:00:51:06`, which can be changed according to your needs on line 40:
```cpp
uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x51, 0x06 };
```