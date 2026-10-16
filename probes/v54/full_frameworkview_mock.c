#define INITGUID
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#define WIDL_using_Windows_Foundation
#include <windows.foundation.h>
#define WIDL_using_Windows_UI
#include <windows.ui.h>
#define WIDL_using_Windows_ApplicationModel_Core
#include <windows.applicationmodel.core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long src_refs=1,view_refs=1;
static int created,qi_view,init_calls,setwin_calls,load_calls,run_calls,uninit_calls;
static HRESULT requested_init_hr=S_OK;


typedef struct Source { IFrameworkViewSource IFrameworkViewSource_iface; } Source;
typedef struct View { IFrameworkView IFrameworkView_iface; } View;
static Source source;
static View view;
static HRESULT WINAPI common_unused_getiids(void *obj,ULONG *count,IID **iids) {
 if(count)*count=0;
 if(iids)*iids=NULL;
 return E_NOTIMPL;
}
static HRESULT WINAPI common_unused_name(void *obj,HSTRING *name){
 if(name)*name=NULL;
 return E_NOTIMPL;
}
static HRESULT WINAPI common_unused_trust(void *obj,TrustLevel *level){return E_NOTIMPL;}
static HRESULT WINAPI src_qi(IFrameworkViewSource *s,REFIID iid,void **out){
 if(!out)return E_POINTER;
 *out=NULL;
 if(IsEqualGUID(iid,&IID_IUnknown)||IsEqualGUID(iid,&IID_IInspectable)||
    IsEqualGUID(iid,&IID_IFrameworkViewSource)){
    *out=s; IFrameworkViewSource_AddRef(s);return S_OK;
 }
 return E_NOINTERFACE;
}
static ULONG WINAPI src_add(IFrameworkViewSource *s){return (ULONG)InterlockedIncrement(&src_refs);}
static ULONG WINAPI src_release(IFrameworkViewSource *s){return (ULONG)InterlockedDecrement(&src_refs);}
static HRESULT WINAPI view_qi(IFrameworkView *v,REFIID iid,void **out){
 if(!out)return E_POINTER;
 *out=NULL;
 if(IsEqualGUID(iid,&IID_IUnknown)||IsEqualGUID(iid,&IID_IInspectable)||
    IsEqualGUID(iid,&IID_IFrameworkView)){
    *out=v;IFrameworkView_AddRef(v);qi_view++;return S_OK;
 }
 return E_NOINTERFACE;
}
static ULONG WINAPI view_add(IFrameworkView *v){return (ULONG)InterlockedIncrement(&view_refs);}
static ULONG WINAPI view_release(IFrameworkView *v){return (ULONG)InterlockedDecrement(&view_refs);}
static HRESULT WINAPI view_initialize(IFrameworkView *v,ICoreApplicationView *core) {
 ICoreApplicationView *again=NULL;
 HRESULT hr=E_POINTER;
 init_calls++;
 printf("V54_INITIALIZE_CALLED=%d CORE_PTR_NULL=%d\n",init_calls,core==NULL);
 if(core) {
  hr=ICoreApplicationView_QueryInterface(core,&IID_ICoreApplicationView,(void **)&again);
  printf("V54_INITIALIZE_COREVIEW_QI_HR=0x%08lx\n",(unsigned long)hr);
  if(again)ICoreApplicationView_Release(again);
  if(SUCCEEDED(hr)){
    __x_ABI_CWindows_CUI_CCore_CICoreWindow *window=NULL;
    hr=ICoreApplicationView_get_CoreWindow(core,&window);
    printf("V54_INITIALIZE_GET_COREWINDOW_HR=0x%08lx\n",(unsigned long)hr);
    if(window)window->lpVtbl->Release(window);
  }
 }
 return requested_init_hr;
}
static HRESULT WINAPI view_setwindow(IFrameworkView *v,__x_ABI_CWindows_CUI_CCore_CICoreWindow *wnd){
 setwin_calls++;printf("V54_SETWINDOW_CALLED=%d\n",setwin_calls);return E_NOTIMPL;
}
static HRESULT WINAPI view_load(IFrameworkView *v,HSTRING str){load_calls++;return E_NOTIMPL;}
static HRESULT WINAPI view_run(IFrameworkView *v){run_calls++;return E_NOTIMPL;}
static HRESULT WINAPI view_uninitialize(IFrameworkView *v){
 uninit_calls++;
 printf("V54_UNINITIALIZE_CALLED=%d\n",uninit_calls);
 return S_OK;
}
static const IFrameworkViewVtbl view_vtbl={
 view_qi,view_add,view_release,
 (void*)common_unused_getiids,(void*)common_unused_name,(void*)common_unused_trust,
 view_initialize,view_setwindow,view_load,view_run,view_uninitialize
};
static HRESULT WINAPI source_create(IFrameworkViewSource *s,IFrameworkView **out){
 created++;
 if(!out)return E_POINTER;
 *out=&view.IFrameworkView_iface;
 IFrameworkView_AddRef(*out);
 return S_OK;
}
static const IFrameworkViewSourceVtbl source_vtbl={
 src_qi,src_add,src_release,
 (void*)common_unused_getiids,(void*)common_unused_name,(void*)common_unused_trust,
 source_create
};
int main(int argc,char **argv){
 HSTRING classstr=NULL;
 IActivationFactory *factory=NULL;
 ICoreApplication *core=NULL;
 HRESULT hr;
 int err=0;
 if(argc!=2 || (strcmp(argv[1],"init_ok")&&strcmp(argv[1],"init_fail")&&strcmp(argv[1],"v47"))){
  fprintf(stderr,"Usage: mock.exe init_ok|init_fail|v47\n");return 2;
 }
 if(!strcmp(argv[1],"init_fail"))requested_init_hr=E_FAIL;
 source.IFrameworkViewSource_iface.lpVtbl=&source_vtbl;
 view.IFrameworkView_iface.lpVtbl=&view_vtbl;
 hr=RoInitialize(RO_INIT_MULTITHREADED);
 if(FAILED(hr))return 3;
 hr=WindowsCreateString(L"Windows.ApplicationModel.Core.CoreApplication",(UINT32)wcslen(L"Windows.ApplicationModel.Core.CoreApplication"),&classstr);
 if(FAILED(hr))return 4;
 hr=RoGetActivationFactory(classstr,&IID_IActivationFactory,(void**)&factory);
 printf("V54_FACTORY_HR=0x%08lx\n",(unsigned long)hr);
 if(SUCCEEDED(hr))hr=IActivationFactory_QueryInterface(factory,&IID_ICoreApplication,(void**)&core);
 printf("V54_COREAPP_HR=0x%08lx\n",(unsigned long)hr);
 if(SUCCEEDED(hr))hr=ICoreApplication_Run(core,&source.IFrameworkViewSource_iface);
 printf("V54_RUN_HR=0x%08lx\n",(unsigned long)hr);
 printf("V54_COUNTS CREATE=%d QI=%d INITIALIZE=%d SETWINDOW=%d LOAD=%d RUN=%d UNINIT=%d\n",
  created,qi_view,init_calls,setwin_calls,load_calls,run_calls,uninit_calls);
 printf("V54_FINAL_REFERENCES SOURCE=%ld VIEW=%ld\n",src_refs,view_refs);
 if(!strcmp(argv[1],"init_ok")){
  err=!(hr==E_NOTIMPL && created==1 && init_calls==1 && uninit_calls==1 &&
        !setwin_calls && !load_calls && !run_calls);
 }else if(!strcmp(argv[1],"init_fail")){
  err=!(hr==E_FAIL && created==1 && init_calls==1 && uninit_calls==0 &&
        !setwin_calls && !load_calls && !run_calls);
 }else{
  err=!(hr==E_NOTIMPL && created==1 && !init_calls && !uninit_calls);
 }
 if(src_refs!=1 || view_refs!=1)err=1;
 if(core)ICoreApplication_Release(core);
 if(factory)IActivationFactory_Release(factory);
 if(classstr)WindowsDeleteString(classstr);
 RoUninitialize();
 printf("V54_TEST_STATUS=%s\n",err?"FAIL":"PASS");
 return err?1:0;
}
