#pragma once

namespace HttpHeaderBuilder
{
	std::string BuildPostHeader(const std::string& path, const std::string& contentType, int contentLength);
};