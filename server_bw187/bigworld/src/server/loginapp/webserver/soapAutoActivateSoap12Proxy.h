/* soapAutoActivateSoap12Proxy.h
   Generated by gSOAP 2.7.13 from Register.h
   Copyright(C) 2000-2009, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#ifndef soapAutoActivateSoap12Proxy_H
#define soapAutoActivateSoap12Proxy_H
#include "soapH.h"

class SOAP_CMAC AutoActivateSoap12Proxy : public soap
{ public:
	/// Endpoint URL of service 'AutoActivateSoap12Proxy' (change as needed)
	const char *soap_endpoint;
    std::string serverAdd_;
	/// Constructor
	AutoActivateSoap12Proxy();
	/// Constructor with copy of another engine state
	AutoActivateSoap12Proxy(const struct soap&);
	/// Constructor with engine input+output mode control
	AutoActivateSoap12Proxy(soap_mode iomode);
	/// Constructor with engine input and output mode control
	AutoActivateSoap12Proxy(soap_mode imode, soap_mode omode);
	/// Destructor frees deserialized data
	virtual	~AutoActivateSoap12Proxy();
	/// Initializer used by constructor
	virtual	void AutoActivateSoap12Proxy_init(soap_mode imode, soap_mode omode);
	/// Disables and removes SOAP Header from message
	virtual	void soap_noheader();
	/// Get SOAP Fault structure (NULL when absent)
	virtual	const SOAP_ENV__Fault *soap_fault();
	/// Get SOAP Fault string (NULL when absent)
	virtual	const char *soap_fault_string();
	/// Get SOAP Fault detail as string (NULL when absent)
	virtual	const char *soap_fault_detail();
	/// Force close connection (normally automatic, except for send_X ops)
	virtual	int soap_close_socket();
	/// Print fault
	virtual	void soap_print_fault(FILE*);
#ifndef WITH_LEAN
	/// Print fault to stream
	virtual	void soap_stream_fault(std::ostream&);
	/// Put fault into buffer
	virtual	char *soap_sprint_fault(char *buf, size_t len);
#endif

	/// Web service operation 'Register' (returns error code or SOAP_OK)
	virtual	int Register(_ns1__Register *ns1__Register, _ns1__RegisterResponse *ns1__RegisterResponse);
};
#endif
