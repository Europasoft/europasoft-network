// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServerUtils/DynamicPages.h"
#include "NetAgent/HttpServerUtils/HttpUtil.h"
#include "NetAgent/HttpServer.h"
#include "NetAgent/HttpServerUtils/Logging.h"

// With dynamic mode enabled, the requested page is not sent back directly, unlike static mode.
// In this mode, the static filesystem request handler is only used for serving data files, not HTML pages.
// 1. The dynamic request handler returns a minimal (blank) page without content, containing a bootstrap script.
// 2. The bootstrap script runs on the client, and sends a fetch request to get the actual content.
// 3. The page is updated clientside.
// Hyperlink events are intercepted clientside, and the appropriate content is fetched without fully reloading the page.
// Every request sent by the bootstrapping code will have the "SPA" tag.


namespace HTTP
{
	bool injectElementIntoHeader(std::string& htmlDocument, const std::string& element)
	{
		const std::string targetOpeningTag = "<head>";
		const auto headCharIndex = htmlDocument.find(targetOpeningTag);
		if (headCharIndex == std::string::npos)
			return false;
		htmlDocument.insert(headCharIndex + targetOpeningTag.length(), element);
		return true;
	}

	std::string makeBlankHtmlPage()
	{
		return R"HTML(
			<!DOCTYPE html>
			<html> <head></head> <body> <main></main> </body> </html>
		)HTML";
	}

	std::string makePageLoadWarningScript()
	{
		return R"JS(
		<script name="es-loadtime">
		// Provide a load time warning message
		function addBodyElement(htmlString)
		{
			var fragment = document.createDocumentFragment();
			var elem = document.createElement('div');
			elem.innerHTML = htmlString;
			while (elem.firstChild) 
				fragment.appendChild(elem.firstChild);
			document.body.insertBefore(fragment, document.body.childNodes[0]);
		}

		var firstHydrationDone = false;

		function firstHydrationWaitTimeout()
		{
			if (!firstHydrationDone)
				addBodyElement("<p style='font-size: calc(16px + 0.2vw);'>It is taking longer than expected to load the page</p>");
		}
		setTimeout(firstHydrationWaitTimeout, 8000)
		</script>
		)JS";
	}

	std::string makeBootstrapScript(const std::string& originalUri)
	{
		std::string script = R"JS(
		<script name="es-bootstrap-dynamic">
		(function ()
		{
			// Load the current page content dynamically
			function loadPage(path, replace = false)
			{
				// Fetch the page, indicating to the server that the request is for SPA
				fetch(path, { headers: { 'X-Requested-With': 'SPA' }})
				.then(res => res.text())
				.then(html =>
					{
						const parser = new DOMParser();
						const doc = parser.parseFromString(html, 'text/html');
						const newMain = doc.querySelector('main');
						const currentMain = document.querySelector('main');
					
						if (newMain && currentMain)
							currentMain.innerHTML = newMain.innerHTML;
					
						// Optionally update the document title
						const newTitle = doc.querySelector('title');
						if (newTitle) document.title = newTitle.innerText;
					
						if (!replace)
							history.pushState(null, '', path);
						
						firstHydrationDone = true;
					})
					.catch(err => console.error('page load error:', err));
			}
		
			// Intercept link clicks
			document.addEventListener('click', function (e)
			{
				const link = e.target.closest('a');
				if (!link) return;
		
				const url = new URL(link.href);
		
				// Only intercept same-origin navigation
				if (url.origin !== location.origin) return;
		
				// Allow new tab, download, etc.
				if (link.target === '_blank' || link.hasAttribute('download') || e.ctrlKey || e.metaKey || e.shiftKey) return;
		
				e.preventDefault();
				if (url.pathname !== location.pathname)
					loadPage(url.pathname);
			});
		
			// Handle back/forward buttons
			window.addEventListener('popstate', function ()
			{
				loadPage(location.pathname, true);
			});

			// Request the initial document again as hydration data
			loadPage("_ES_ORIGINAL_URI_", true)
		})();
		</script>
		)JS";

		replaceSubstring(script, "_ES_ORIGINAL_URI_", originalUri);
		return script;
	}

	std::string makeDynamicBootstrapPage(const std::string& originalUri)
	{
		auto page = makeBlankHtmlPage();
		injectElementIntoHeader(page, makePageLoadWarningScript());
		injectElementIntoHeader(page, makeBootstrapScript(originalUri));
		return page;
	}

	void makeHtmlDynamicPage(std::string& originalHtmlPage, const std::string& uri)
	{
		auto bodyStart = originalHtmlPage.find("<body>");
		auto bodyEnd = originalHtmlPage.find("</body>");
		if (bodyStart == std::string::npos or bodyEnd == std::string::npos)
		{
			ESLog::es_error(ESLog::FormatStr() << "File '" << uri << "' could not be parsed, no body tag found (tags cannot contain whitespace)");
			return;
		}
		bodyStart += 6;
		std::string body = originalHtmlPage.substr(bodyStart, bodyEnd - bodyStart);
		originalHtmlPage = (ESLog::FormatStr() << "<main>" << body << "</main>");
	}

	

	


	
}
