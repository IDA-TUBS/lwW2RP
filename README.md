# lightweight standalone W2RP implementation

## Build configuration
* **LOG=ON** enables logging statements
* **CONSOLE=ON** Adds a console log
* **FILE=ON** Adds a file log

Usage: cmake .. -DLOG=ON -DCONSOLE=ON -DFILE=ON

### Debug build type
Using -DCMAKE_BUILD_TYPE=Debug, logging will be enabled and a console log is supplied by default

### Logging
For logging boost/log is employed. 
The console log will display logging messages for all severity levels besides "trace"
The file log only logs "trace" messages