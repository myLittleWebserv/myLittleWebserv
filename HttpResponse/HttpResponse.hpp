#ifndef HTTPRESPONSE_HPP

class HttpRequest;
class CgiResponse;
class LocationInfo;

class HttpResponse {
  // Constructor
 public:
  HttpResponse(HttpRequest& http_request, LocationInfo& location_info) { (void)http_request, (void)location_info; }
  HttpResponse(CgiResponse& cgi_reponse, LocationInfo& location_info) { (void)cgi_reponse, (void)location_info; }
  // Interface
 public:
};

#endif
