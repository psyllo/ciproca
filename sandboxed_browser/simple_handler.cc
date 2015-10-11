// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefsimple/simple_handler.h"

#include <sstream>
#include <string>
#include <iostream>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "cefsimple/EdUrlParser.h"


namespace {

SimpleHandler* g_instance = NULL;

}  // namespace

SimpleHandler::SimpleHandler()
    : is_closing_(false) {
  DCHECK(!g_instance);
  g_instance = this;
}

SimpleHandler::~SimpleHandler() {
  g_instance = NULL;
}

// static
SimpleHandler* SimpleHandler::GetInstance() {
  return g_instance;
}

/*
	returns the offset |result| was found at
*/
size_t parseNextDirFromUriPath(std::string &result, std::string path, size_t start_offset) {
	size_t start = start_offset;

	while (path[start] == '/' && start < path.length()) start++;

	// Require a leading slash
	if (start > start_offset && start < path.length())
	{
		size_t end = path.find('/', start);
		if(end != std::string::npos)
			result = path.substr(start, end - start);
	}

	return start;
}

/*
	Returns 1 if just app_type was found and 2 if app_name was found as well, else 0.
*/
size_t parseTypeAndNameFromUriPath(std::string path, std::string &app_type, std::string &app_name) {
	size_t count = 0;
	size_t start_offset = parseNextDirFromUriPath(app_type, path, 0);
	if (!app_type.empty()) {
		count++;
		parseNextDirFromUriPath(app_name, path, start_offset + app_type.length());
		if (!app_name.empty()) {
			count++;
		}
	}
	return count;
}

/*
	Valid app names contain only [A-Za-z0-9]
*/
bool isValidAppName(std::string app_name) {
	bool result = true;
	for (size_t i = 0; i < app_name.length(); i++) {
		if (!isalpha(app_name[i]) && !isdigit(app_name[i])) {
			result = false;
			break;
		}
	}
	return result;
}

bool isValidateAppTypeName(std::string app_type_name) {
	return isValidAppName(app_type_name);
}

bool isValidAppScheme(std::string scheme) {
	return	scheme.compare("http") == 0 ||
		scheme.compare("https") == 0;
}

bool isValidAppTypeName(std::string app_type) {
	return isValidateAppTypeName(app_type) &&
		(app_type.compare("core") == 0 || app_type.compare("app") == 0);
}

std::string apps_port_3rd_party = "10001";
bool isValid3rdPartyAppPort(std::string port) {
	return port.compare(apps_port_3rd_party) == 0;
}

std::string apps_host_name = "localhost";
bool isValidAppHostName(std::string host_name) {
	return host_name.compare(apps_host_name) == 0;
}

/*
	|uri_or_path| can be a URL, file path or similar.
	Searches for standardized app parent dirs.
	return 0 if not found
*/
size_t findAppBasePathOffset(std::string uri_or_path) {
	std::string app = "/app/";
	size_t start = uri_or_path.find(app, 0);
	size_t result = std::string::npos;
	if (start != std::string::npos) {
		result = start + app.length() - 1;
	}
	return result;
}

bool findAppBasePath(std::string uri_or_path, std::string &result) {
	size_t start = findAppBasePathOffset(uri_or_path);
	if (start != std::string::npos) {
		result = uri_or_path.substr(start, uri_or_path.length() - start);
		return true;
	}
	return false;
}

bool findAppName(std::string url_or_path, std::string &result) {
	std::string base;
	bool base_found = findAppBasePath(url_or_path, base);
	if (base_found && !base.empty()) {
		size_t start = 0;
		if (base[0] == '/')
			start++;
		size_t end = base.find("/", start);
		if (end != std::string::npos
			&& end - start > 0)
		{
			std::string tmp = base.substr(start, end - start);
			if (isValidAppName(tmp)) {
				result = tmp;
				return true;
			}
		}
	}
	return false;
}

bool isSameAppByUrl(std::string url1, std::string url2) {
	size_t o1 = findAppBasePathOffset(url1);
	size_t o2 = findAppBasePathOffset(url2);
	if (o1 != std::string::npos && o1 == o2) {
		std::string n1, n2;
		bool f1 = findAppName(url1, n1);
		bool f2 = findAppName(url2, n2);
		if (f1 && f2 && n1.compare(n2) == 0) {
			return true;
		}
	}
	return false;
}

/*
	A 3rd party app can access just itself. Since they're hosted on their own port
	at the root of the URL path, just check scheme, host name and port for equality.
*/

std::string core_app_url_scheme = "http";
std::string core_app_host_name = "localhost";
std::string core_app_port = "10000";
std::string database_url_scheme = "http";
std::string database_host_name = "localhost";
std::string database_port = "5984";

bool isValidWebDestFor3rdPartyApp(std::string from_url_string, std::string dest_url_string) {
	bool result = false;
	EdUrlParser* from_url = EdUrlParser::parseUrl(from_url_string);
	EdUrlParser* dest_url = EdUrlParser::parseUrl(dest_url_string);
	if (isSameAppByUrl(from_url_string, dest_url_string)
		&& isValidAppScheme(dest_url->scheme)
		&& dest_url->scheme.compare(from_url->scheme) == 0
		&& isValidAppHostName(dest_url->hostName)
		&& dest_url->hostName.compare(from_url->hostName) == 0
		&& isValid3rdPartyAppPort(from_url->port)
		&& dest_url->port.compare(from_url->port) == 0)
	{
		result = true;
	}
	if (from_url != NULL) delete from_url;
	if (dest_url != NULL) delete dest_url;
	return result;
}

bool isValid3rdPartyAppUrl(std::string url_string) {
	bool result = false;
	EdUrlParser* url = EdUrlParser::parseUrl(url_string);
	if (   isValidAppScheme(url->scheme)
		&& isValidAppHostName(url->hostName)
		&& isValid3rdPartyAppPort(url->port))
	{
		std::string app_name;
		bool found = findAppName(url_string, app_name);
		result = found;
	}
	if (url != NULL) delete url;
	return result;
}

bool isDatabaseUrl(EdUrlParser *dest_url) {
	return dest_url->scheme.compare(database_url_scheme) == 0
		&& dest_url->hostName.compare(database_host_name) == 0
		&& dest_url->port.compare(database_port) == 0;
}

bool isDatabaseUrl(std::string dest_url_string) {
	EdUrlParser* dest_url = EdUrlParser::parseUrl(dest_url_string);
	bool result = isDatabaseUrl(dest_url);
	if (dest_url != NULL) delete dest_url;
	return result;
}

/*
RT_MAIN_FRAME not allowed to prevent removing outter nav
*/
bool isValidResourceTypeFor3rdPartyApp(cef_resource_type_t rtype) {
	bool result = false;
	if (rtype == RT_FAVICON
		|| rtype == RT_FONT_RESOURCE
		|| rtype == RT_IMAGE
		|| rtype == RT_MEDIA
		// || rtype == RT_MAIN_FRAME // Prevent link target _top, _parent and pop-ups
		// || rtype == RT_OBJECT // TODO: Understand before permitting
		// || rtype == RT_PING // TODO: Understand before permitting
		// || rtype == RT_PREFETCH // TODO: Understand before permitting
		|| rtype == RT_SCRIPT
		// || rtype == RT_SERVICE_WORKER // TODO: Understand before permitting
		// || rtype == RT_SHARED_WORKER // TODO: Understand before permitting
		|| rtype == RT_STYLESHEET
		|| rtype == RT_SUB_FRAME
		// || rtype == RT_SUB_RESOURCE // TODO: Understand before permitting
		// || rtype == RT_WORKER // TODO: Understand before permitting
		|| rtype == RT_XHR) {
		result = true;
	}
	return result;
}

bool isValidDestFor3rdPartyApp(std::string from_url_string, std::string dest_url_string,
								cef_resource_type_t rtype)
{
	return isValidResourceTypeFor3rdPartyApp(rtype)
		&& isValid3rdPartyAppUrl(from_url_string)
		&& (isValidWebDestFor3rdPartyApp(from_url_string, dest_url_string)
			|| isDatabaseUrl(dest_url_string));
}

bool isCoreAppUrl(std::string url) {
	bool result = false;
	EdUrlParser* from_url = EdUrlParser::parseUrl(url);

	if (from_url->scheme.compare(core_app_url_scheme) == 0
		&& from_url->hostName.compare(core_app_host_name) == 0
		&& from_url->port.compare(core_app_port) == 0)
	{
		result = true;
	}

	if (from_url != NULL) delete from_url;

	return result;
}

bool is3rdPartyApp(std::string url) {
	return !isCoreAppUrl(url);
}

std::string core_app_token = "DEVCORETOK"; // TODO: Security: hard-coded

bool isCorrectCoreTokenInUrl(std::string url) {
	std::string query_arg = "apptok=";
	size_t query_arg_start = url.find(query_arg, 0);
	if (query_arg_start != std::string::npos) {
		size_t val_start = query_arg_start + query_arg.length();
		size_t val_len = core_app_token.length();
		if (val_start + val_len <= url.length())
		{
			std::string url_tok = url.substr(val_start, val_len);
			if (url_tok.compare(core_app_token) == 0) {
				return true;
			}
		}
	}
	return false;
}

/*
	return true for database access or anything on the same host name regardless of port
*/
bool isValidInternalDestForCoreApp(CefRefPtr<CefFrame> frame,
		std::string request_url,
		cef_resource_type_t rtype)
{
	std::string frame_url;
	frame_url = frame->GetURL();

	if (rtype == RT_MAIN_FRAME) {
		if (isCorrectCoreTokenInUrl(request_url)
			&& isCoreAppUrl(request_url)
			&& isCoreAppUrl(frame_url)) {
			return true;
		}
	} else if (!isCoreAppUrl(frame_url)) {
		return false;
	} else if (isCoreAppUrl(request_url)) {
		return true;
	} else if (!frame->IsMain()) {
		return true;
	}
	return false;
}

bool isValidInternalDestForCoreApp(CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) {
	return isValidInternalDestForCoreApp(frame, request->GetURL(), request->GetResourceType());
}

/*
	If the argument has a URL then it will simply be returned as the result.
	Check the resulting CefFrame->GetURL() to determine if a URL a frame
	with a URL was found.
*/
CefRefPtr<CefFrame> findFirstFrameWithNonEmptyUrl(CefRefPtr<CefFrame> frame) {
	CefRefPtr<CefFrame> result = frame;
	CefRefPtr<CefFrame> prev_result = frame;
	while (result != NULL && result->GetURL().empty()) {
		prev_result = result;
		result = result->GetParent();
	}
	if (result == NULL)
		result = prev_result;
	return result;
}

/*
	This function checks as much state as it can that I found to be true
	when the first browser launches.
*/

bool isInitialDoc(	CefRefPtr<CefBrowser> browser,
					CefRefPtr<CefRequest> request)
{
	if (browser == NULL || request == NULL) {
		// TODO: Log a warning
		return false;
	}

	bool ttype_ok = request->GetTransitionType() == TT_EXPLICIT;
	bool rtype_ok = request->GetResourceType() == RT_MAIN_FRAME;

	return !browser->HasOneRef() && !browser->HasDocument()
			&& ttype_ok && rtype_ok;
}

bool aPopupWasApproved = false; // TODO: Concurrency: Security: Not a thread safe operation

bool isPermissibleResourceAttempt(  CefRefPtr<CefBrowser> browser,
									CefRefPtr<CefFrame> frame,
									CefRefPtr<CefRequest> request)
{
	if (!frame->IsValid() || request == NULL || request->GetURL().empty())
		return false;

	bool result = false;

	frame = findFirstFrameWithNonEmptyUrl(frame);
	if (frame == NULL || frame->GetURL().empty() && !aPopupWasApproved)
		return false;

	std::string frame_url = frame->GetURL().ToString();
	std::string req_url = request->GetURL().ToString();

	if (!frame_url.empty() && !req_url.empty())
	{
		if (isValidInternalDestForCoreApp(frame, request)) {
			result = true;
		}
		else if(isValidDestFor3rdPartyApp(frame_url, req_url, request->GetResourceType()))
		{
			result = true;
		}
	}
	else if (aPopupWasApproved) {
		aPopupWasApproved = false;
		result = true;
	}

	return result;
}

bool isPermissibleRequest(	CefRefPtr<CefBrowser> browser,
							CefRefPtr<CefFrame> frame,
							CefRefPtr<CefRequest> request)
{
	bool result = false;
	if (isPermissibleResourceAttempt(browser, frame, request)) {
		result = true;
	}
	else if (isInitialDoc(browser, request)) {
		result = true;
	}
	return result;
}

bool SimpleHandler::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
	const CefString& target_url, CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture)
{
	// TODO: log that it was prevented
	return true; // cancel navigation - prevent pop-up through control-click etc
}

/*
	TODO: log illegal access attempts
*/
CefRequestHandler::ReturnValue SimpleHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback)
{
	CEF_REQUIRE_IO_THREAD();
	CefRequestHandler::ReturnValue result = RV_CANCEL;
	if (isPermissibleRequest(browser, frame, request)) {
		result = RV_CONTINUE;
	}
	return result;
}



void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Add to the list of existing browsers.
  browser_list_.push_back(browser);
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (browser_list_.size() == 1) {
    // Set a flag to indicate that the window close should be allowed.
    is_closing_ = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Remove from the list of existing browsers.
  BrowserList::iterator bit = browser_list_.begin();
  for (; bit != browser_list_.end(); ++bit) {
    if ((*bit)->IsSame(browser)) {
      browser_list_.erase(bit);
      break;
    }
  }

  if (browser_list_.empty()) {
    // All browser windows have closed. Quit the application message loop.
    CefQuitMessageLoop();
  }
}

bool SimpleHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url,
	const CefString& target_frame_name, CefLifeSpanHandler::WindowOpenDisposition target_disposition,
	bool user_gesture, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo,
	CefRefPtr< CefClient >& client, CefBrowserSettings& settings, bool* no_javascript_access)
{
	bool permitted = isValidInternalDestForCoreApp(frame, target_url, RT_MAIN_FRAME);
	if (permitted) {
		aPopupWasApproved = true;
		return false;
	}else {
		// TODO: Log malicious attempt
	}
	return true;
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Display a load error message.
  std::stringstream ss;
  ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL " << std::string(failedUrl) <<
        " with error " << std::string(errorText) << " (" << errorCode <<
        ").</h2></body></html>";
  frame->LoadString(ss.str(), failedUrl);
}

void SimpleHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&SimpleHandler::CloseAllBrowsers, this, force_close));
    return;
  }

  if (browser_list_.empty())
    return;

  BrowserList::const_iterator it = browser_list_.begin();
  for (; it != browser_list_.end(); ++it)
    (*it)->GetHost()->CloseBrowser(force_close);
}


bool SimpleHandler::OnJSDialog(CefRefPtr<CefBrowser> browser, const CefString& origin_url,
	const CefString& accept_lang, CefJSDialogHandler::JSDialogType dialog_type,
	const CefString& message_text, const CefString& default_prompt_text,
	CefRefPtr<CefJSDialogCallback> callback, bool& suppress_message)
{
	suppress_message = true;
	return false;
}