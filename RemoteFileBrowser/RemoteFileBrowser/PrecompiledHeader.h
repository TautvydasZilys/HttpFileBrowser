#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

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
#include <unordered_set>
#include <vector>

#include "Utilities\Utilities.h"