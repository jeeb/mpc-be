// dllmain.h : Declaration of module class.

class CMPCBEShellExtModule : public CAtlDllModuleT< CMPCBEShellExtModule >
{
public :
	DECLARE_LIBID(LIBID_MPCBEShellExtLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_MPCBESHELLEXT, "{631B91DB-D412-4947-8E5A-340E4FF7E1BD}")
};

extern class CMPCBEShellExtModule _AtlModule;
