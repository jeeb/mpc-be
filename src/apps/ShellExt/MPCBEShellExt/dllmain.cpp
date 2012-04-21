// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "MPCBEShellExt_i.h"
#include "dllmain.h"

CMPCBEShellExtModule _AtlModule;

class CMPCBEShellExtApp : public CWinApp
{
public:

// Overrides
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CMPCBEShellExtApp, CWinApp)
END_MESSAGE_MAP()

CMPCBEShellExtApp theApp;

BOOL CMPCBEShellExtApp::InitInstance()
{
	return CWinApp::InitInstance();
}

int CMPCBEShellExtApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
