
#include <WS2tcpip.h>
#include <Windows.h>
#include <mstcpip.h>

#if !PHONE
#include <Iphlpapi.h>
#endif

#undef min
#undef max

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Utilities\Utilities.h"