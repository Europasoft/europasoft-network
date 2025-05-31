// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once

#include <string>
#include <string_view>

namespace HTTP
{
	bool injectElementIntoHeader(std::string& htmlDocument, const std::string& element);

	std::string makeBlankHtmlPage();

	std::string makePageLoadWarningScript();

	std::string makeBootstrapScript(const std::string& originalUri);

	std::string makeDynamicBootstrapPage(const std::string& originalUri);
	
	void makeHtmlDynamicPage(std::string& originalHtmlPage, const std::string& uri);

	std::string removeSurroundingWhitespace(const std::string str);
}
