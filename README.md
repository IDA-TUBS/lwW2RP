# lightweight W2RP 

This library contains a lightweight (minimum) implementation of the Wireless Reliable Real-Time Protocol (W2RP) [1]. W2RP has been specifically designed to address reliable wireless exchange of large objects such as sensor data. To address the shortcomings of state-of-the-art wireless standards (packet-level backward error correction (BEC) of 802.11 and cellular tech inefficient for large data), W2RP employs a sample-level BEC approach that allows to relax the packet-level constraints and rather exploit the whole sample/object deadline for more efficient retransmission schemes.

Possible fields of deployment for W2RP could be cooperative and safety-critical applications in V2X environments or for robotics. For more detailed information on how the protocol works as well as performance evaluation (in simulation and using physical demonstrator setups), please refer to the [LOTUS](https://ida-tubs.github.io/lotus/) project website or the papers listed below.



## Requirements

lightweight W2RP (lwW2RP) is completely written in C++ and intended to be used on an arbitrary linux host. So far, lwW2RP has successfully been deployed on both x86 (Intel) and ARM64 (Nvidia Jetson) - both running Ubuntu 22.04. Deployment on other versions and distribution should not be a problem. All packages required for running lwW2RP (specific boost version) are bundled with this library.

In general, W2RP is a technology-agnostic protocol that could be used on top of arbitrary wireless protocols such as 802.11 or cellular technology. So far, lwW2RP has only been evaluated over 802.11. However, there are some recommendations to follow to ensure best performance:

* If possible, disable MAC layer retransmissions
* W2RP will not help to improve reliability in overloaded channels. 
* ...



## Installation

Clone this repository.

```bash
git clone https://github.com/IDA-TUBS/lwW2RP
```

From within the cloned repository install boost using the following script (mandatory!)

```bash
sudo ./install_boost.sh
```

Then build and install lwW2RP library:

```bash
sudo ./build.sh -j <n_threads>
```

**Important:** lwW2RP will be installed as a system-wide library. This allows use of lwW2RP in any arbitrary C++ program. Standard practices need to be followed when including lwW2RP (linking in makefile, includes in source code).

### Build configuration

The build scripts offers the possibility to configure different logging options.

* **LOG=ON** enables logging statements
* **CONSOLE=ON** Adds a console log
* **FILE=ON** Adds a file log

#### Debug build type

Using -DCMAKE_BUILD_TYPE=Debug, logging will be enabled and a console log is supplied by default

#### Logging

For logging boost/log is employed. 
The console log will display logging messages for all severity levels besides "trace"
The file log only logs "trace" messages



## Features

lightweight W2RP features:

- writer and reader implementations
- sample-/object-level BEC
- configurable minimum distance shaping (MDS)
- support for dynamically sized objects
- (shaped) burst transmissions to make better us aggregation mechanisms (effectiveness not tested yet)

In general, the inner, data handling structure of lwW2RP is inspired by the [RTPS](https://www.omg.org/spec/DDSI-RTPS/2.2/PDF) (Real-Time Publish-Subscribe) protocol with a *history cache* managing samples/objects (*cacheChanges*). Notably, while messages are also based on the RTPS specification, lwW2RP is not compatible with arbitrary RTPS implementations!

Furthermore, lwW2RP purposefully omits implementing dynamic discovery. Given the proof-of-concept nature of this implementation, its focus was on evaluating the basic W2RP mechanisms and not providing a feature-rich implementation. Details on how to use lwW2RP can be found below.



## Usage

To use lwW2RP, one needs to add a *writer* (sender) and a *reader* (receiver) to the receiving parts of a (distributed) application, e.g, by putting the entities into dedicated publisher and subscriber classes. An example can be found in `examples/app_videoStream`.

Add a writer to the publisher class:

```cpp
bool Publisher::init(uint16_t participant_id ,std::string cfg, std::string setup)
{
    counterRow = 0;
    config::writerCfg w_config("WRITER_01", cfg, setup);    
    writer = new Writer(participant_id, w_config);
    return true;
}
```

Data can then be transmitted using the `write` function:

```cpp
SerializedPayload *payload = new SerializedPayload(data, size);

if(writer->write(payload)
{
    // sample transmitted successfully
}
else
{
    // problem occured
    return false;
}
```

Importantly, lwW2RP only accepts `SerializedPayload` objects. Thus, there is a need to manually convert your object into such an object. 

On the receiving (subscriber) side, create a reader:

```cpp
bool Subscriber::init(uint16_t participant_id ,std::string cfg, std::string setup)
{
    config::readerCfg r_config("READER_01", cfg, setup);

    reader = new Reader(participant_id, r_config);
    return true;
}
```

Data can then be retrieved by the subscribing app using:

```cpp
reader->retrieveSample(payload);			
```

Note, `retrieveSample` is a blocking call. It is recommended to create a dedicated rx thread to avoid further blocking effects (see `examples/app_videoStream/reader_test.cpp`).

### Configuration

lwW2RP is configured via two config files `w2rp_config.json` and `setup_defines.json`. The latter contains unique node IDs. The former contains the W2RP parameters as well as static connection definitions:

* **ADDRESS**: Address of interface used to address this entity
* **PORT**: Port used for W2RP 
* **READERS**: For a writer, names (cf. `setup_defines.json`) of readers that will "subscribe" to the writer's data
* **WRITER**: For a reader, name of the corresponding writer
* **HOST**: Name (cf. `setup_defines.json`) of the node hosting the writer/reader

For explanations all other parameters, we kindly refer the user to the papers linked below.

Both config files have to be presented on all devices hosting (distributed) parts of an application. Examples of how to load the configs can be found in `examples/app_videoStream/reader_test.cpp` and `examples/app_videoStream/writer_test.cpp`.



## References

[1] J. Peeck, M. Möstl, T. Ishigooka and R. Ernst, "A Middleware Protocol for Time-Critical Wireless Communication of Large Data Samples," *2021 IEEE Real-Time Systems Symposium (RTSS)*, Dortmund, DE, 2021, pp. 1-13, doi: https://doi.org/10.1109/RTSS52674.2021.00013

[2] Jonas Peeck, Mischa Möstl, Tasuku Ishigooka and Rolf Ernst, “A Protocol for Reliable Real-Time Wireless Communication of Large Data Samples”, IEEE Transactions on Vehicular Technology, vol. 72, no. 10, pp. 13146–13161, 2023, October. https://doi.org/10.24355/dbbs.084-202405030723-0

[3] A. Bendrick, J. Peeck and R. Ernst, "On the effectiveness of W2RP in physical environments", 2023, *technical memorandum*, doi: https://doi.org/10.24355/dbbs.084-202301231301-0

[4] A. Bendrick, J. Peeck and R. Ernst, "An Error Protection Protocol for the Multicast Transmission of Data Samples in V2X environments", 2023, https://doi.org/10.24355/dbbs.084-202405020823-0



## Contact

Alex Bendrick (bendrick@ida.ing.tu-bs.de)

Daniel Tappe (tappe@ida.ing.tu-bs.de)

When using or referencing W2RP in scientific works we kindly ask you reference the papers mentioned above.



