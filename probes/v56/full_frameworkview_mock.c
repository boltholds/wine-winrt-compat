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
#define WIDL_using_Windows_UI_Core
#include <windows.ui.core.h>
#define WIDL_using_Windows_ApplicationModel_Core
#include <windows.applicationmodel.core.h>
#include <stdio.h>
#include <string.h>

typedef struct V56Source { IFrameworkViewSource iface; } V56Source;
typedef struct V56View { IFrameworkView iface; } V56View;

static V56Source source;
static V56View view;
static LONG src_refs=1,view_refs=1;
static ICoreApplication *core;
static ICoreWindow *borrowed_window;
static const char *scenario;
static LONG created,query_view,initialized,set_window,loaded,ran,uninitialized;
static DWORD owner_thread;
static LONG messages_sent;
static BOOL current_view_seen,corewindow_seen,dispatcher_seen;
static int errors;

static void check(int valid,const char *description)
{
    printf("V56_CHECK_%s=%s\n",description,valid?"PASS":"FAIL");
    if(!valid) errors++;
}
static BOOL matches(const char *name){return !strcmp(scenario,name);}
static HRESULT WINAPI inspect_getiids(void *iface,ULONG *count,IID **iids)
{
    if(count)*count=0;
    if(iids)*iids=NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI inspect_getclass(void *iface,HSTRING *name)
{
    if(name)*name=NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI inspect_gettrust(void *iface,TrustLevel *value)
{
    if(value)*value=BaseTrust;
    return S_OK;
}
static ULONG WINAPI src_add(IFrameworkViewSource *iface)
{
    return (ULONG)InterlockedIncrement(&src_refs);
}
static ULONG WINAPI src_release(IFrameworkViewSource *iface)
{
    return (ULONG)InterlockedDecrement(&src_refs);
}
static HRESULT WINAPI src_qi(IFrameworkViewSource *iface,REFIID iid,void **out)
{
    if(!out)return E_POINTER;
    *out=NULL;
    if(IsEqualGUID(iid,&IID_IUnknown)||IsEqualGUID(iid,&IID_IInspectable)||
        IsEqualGUID(iid,&IID_IFrameworkViewSource))
    {
        *out=iface;src_add(iface);return S_OK;
    }
    return E_NOINTERFACE;
}
static ULONG WINAPI view_add(IFrameworkView *iface)
{
    return (ULONG)InterlockedIncrement(&view_refs);
}
static ULONG WINAPI view_release(IFrameworkView *iface)
{
    return (ULONG)InterlockedDecrement(&view_refs);
}
static HRESULT WINAPI view_qi(IFrameworkView *iface,REFIID iid,void **out)
{
    if(!out)return E_POINTER;
    *out=NULL;
    if(IsEqualGUID(iid,&IID_IUnknown)||IsEqualGUID(iid,&IID_IInspectable)||
       IsEqualGUID(iid,&IID_IFrameworkView))
    {
        *out=iface;view_add(iface);query_view++;return S_OK;
    }
    return E_NOINTERFACE;
}
static HRESULT inspect_view_identity(ICoreApplicationView *application_view)
{
    ICoreApplicationView *returned=NULL,*qi=NULL;
    HRESULT hr;
    hr=ICoreApplicationView_QueryInterface(application_view,
        &IID_ICoreApplicationView,(void **)&qi);
    printf("V56_INITIALIZE_VIEW_QI_HR=0x%08lx\n",(unsigned long)hr);
    check(hr==S_OK && qi==application_view,"INITIALIZE_VIEW_QI");
    if(qi)ICoreApplicationView_Release(qi);
    hr=ICoreApplication_GetCurrentView(core,&returned);
    printf("V56_CURRENT_VIEW_HR=0x%08lx\n",(unsigned long)hr);
    if(matches("v54_fallback"))
        check(hr==E_NOTIMPL && !returned,"V54_FALLBACK_NO_CURRENT_VIEW");
    else
    {
        current_view_seen=(hr==S_OK && returned==application_view);
        check(current_view_seen,"CURRENT_VIEW_IDENTITY");
    }
    if(returned) ICoreApplicationView_Release(returned);
    return hr;
}
static HRESULT inspect_window(ICoreApplicationView *application_view)
{
    ICoreWindow *window=NULL,*from_thread=NULL;
    ICoreWindowStatic *statics=NULL;
    HRESULT hr;
    HSTRING class_string=NULL;
    hr=ICoreApplicationView_get_CoreWindow(application_view,&window);
    printf("V56_GET_COREWINDOW_HR=0x%08lx NULL=%d\n",
        (unsigned long)hr,window==NULL);
    if(matches("v54_fallback"))
    {
        check(hr==E_NOTIMPL && !window,"V54_FALLBACK_NO_COREWINDOW");
        if(window) ICoreWindow_Release(window);
        return hr;
    }
    check(hr==S_OK && window!=NULL,"REAL_COREWINDOW_FROM_APPVIEW");
    if(FAILED(hr)||!window)return hr;
    borrowed_window=window;
    ICoreWindow_Release(window);
    window=NULL;
    hr=WindowsCreateString(L"Windows.UI.Core.CoreWindow",26,&class_string);
    if(SUCCEEDED(hr))
        hr=RoGetActivationFactory(class_string,&IID_ICoreWindowStatic,(void **)&statics);
    check(hr==S_OK && statics,"COREWINDOW_STATIC_FACTORY");
    if(statics)
    {
        hr=ICoreWindowStatic_GetForCurrentThread(statics,&from_thread);
        printf("V56_STATIC_GET_CURRENT_HR=0x%08lx\n",(unsigned long)hr);
        corewindow_seen=(hr==S_OK && from_thread==borrowed_window);
        check(corewindow_seen,"COREWINDOW_THREAD_LOCAL_IDENTITY");
        if(from_thread)ICoreWindow_Release(from_thread);
        ICoreWindowStatic_Release(statics);
    }
    if(class_string)WindowsDeleteString(class_string);
    return S_OK;
}
static HRESULT WINAPI view_initialize(IFrameworkView *iface,ICoreApplicationView *appview)
{
    initialized++;
    printf("V56_INITIALIZE=%ld ARG_NONNULL=%d\n",initialized,appview!=NULL);
    check(appview!=NULL && owner_thread==GetCurrentThreadId(),"INITIALIZE_THREAD");
    if(appview)
    {
        inspect_view_identity(appview);
        inspect_window(appview);
    }
    if(matches("initialize_fail"))return E_FAIL;
    return S_OK;
}
static HRESULT WINAPI view_set_window(IFrameworkView *iface,ICoreWindow *window)
{
    ICoreDispatcher *dispatcher=NULL;
    boolean thread_access=FALSE;
    HRESULT hr;
    set_window++;
    printf("V56_SETWINDOW=%ld PTR_MATCH=%d\n",set_window,window==borrowed_window);
    check(window==borrowed_window && window!=NULL,"SETWINDOW_REAL_COREWINDOW");
    if(window)
    {
        hr=ICoreWindow_get_Dispatcher(window,&dispatcher);
        dispatcher_seen=(hr==S_OK && dispatcher!=NULL);
        check(dispatcher_seen,"SETWINDOW_GET_DISPATCHER");
        if(dispatcher)
        {
            hr=ICoreDispatcher_get_HasThreadAccess(dispatcher,&thread_access);
            check(hr==S_OK && thread_access,"SETWINDOW_OWNER_THREAD");
            ICoreDispatcher_Release(dispatcher);
        }
    }
    if(matches("setwindow_fail"))return E_ACCESSDENIED;
    return S_OK;
}
static HRESULT WINAPI view_load(IFrameworkView *iface,HSTRING entrypoint)
{
    loaded++;
    printf("V56_LOAD=%ld ENTRY_LENGTH=%u\n",loaded,
        (unsigned)WindowsGetStringLen(entrypoint));
    check(WindowsGetStringLen(entrypoint)==0,"LOAD_EMPTY_ENTRYPOINT");
    if(matches("load_fail"))return E_INVALIDARG;
    return S_OK;
}
struct close_task { HWND hwnd; LONG result; };
static DWORD WINAPI close_after_delay(LPVOID data)
{
    struct close_task *task=data;
    Sleep(115);
    if(!PostMessageW(task->hwnd,WM_APP+55,0,0)){task->result=GetLastError();return 1;}
    if(!PostMessageW(task->hwnd,WM_CLOSE,0,0)){task->result=GetLastError();return 2;}
    task->result=0;
    return 0;
}
static HRESULT WINAPI view_run(IFrameworkView *iface)
{
    ICoreDispatcher *dispatcher=NULL;
    HRESULT hr;
    HWND hwnd;
    ran++;
    printf("V56_VIEW_RUN=%ld\n",ran);
    if(matches("run_fail")||matches("run_uninit_fail"))return E_UNEXPECTED;
    if(!borrowed_window)return E_POINTER;
    hwnd=FindWindowW(L"WineExperimentalWinRTCoreWindowV55",NULL);
    check(hwnd!=NULL && IsWindow(hwnd),"RUN_FOUND_REAL_HWND");
    if(!hwnd)return E_FAIL;
    hr=ICoreWindow_get_Dispatcher(borrowed_window,&dispatcher);
    check(hr==S_OK && dispatcher,"RUN_GET_DISPATCHER");
    if(FAILED(hr)||!dispatcher)return hr;
    {
        struct close_task task={hwnd,-1};
        HANDLE worker=CreateThread(NULL,0,close_after_delay,&task,0,NULL);
        check(worker!=NULL,"RUN_WORKER_CREATED");
        if(!worker){ICoreDispatcher_Release(dispatcher);return E_OUTOFMEMORY;}
        hr=ICoreDispatcher_ProcessEvents(dispatcher,CoreProcessEventsOption_ProcessUntilQuit);
        printf("V56_DISPATCHER_UNTIL_QUIT_HR=0x%08lx\n",(unsigned long)hr);
        check(hr==S_OK && !IsWindow(hwnd),"RUN_MESSAGE_LOOP_QUIT");
        check(WaitForSingleObject(worker,12000)==WAIT_OBJECT_0,"RUN_WORKER_FINISHED");
        check(task.result==0,"RUN_CLOSE_MESSAGES_POSTED");
        CloseHandle(worker);
        messages_sent+=2;
    }
    ICoreDispatcher_Release(dispatcher);
    return hr;
}
static HRESULT WINAPI view_uninitialize(IFrameworkView *iface)
{
    ICoreApplicationView *current=NULL;
    HRESULT hr;
    uninitialized++;
    printf("V56_UNINITIALIZE=%ld\n",uninitialized);
    if(!matches("v54_fallback"))
    {
        hr=ICoreApplication_GetCurrentView(core,&current);
        check(hr==S_OK && current,"CURRENT_VIEW_ALIVE_DURING_UNINIT");
        if(current) ICoreApplicationView_Release(current);
    }
    if(matches("uninit_fail")||matches("run_uninit_fail"))return E_ABORT;
    return S_OK;
}
static IFrameworkViewVtbl view_vtbl={
    view_qi,view_add,view_release,
    (void *)inspect_getiids,(void *)inspect_getclass,(void *)inspect_gettrust,
    view_initialize,view_set_window,view_load,view_run,view_uninitialize
};
static HRESULT WINAPI src_create(IFrameworkViewSource *iface,IFrameworkView **out)
{
    if(!out)return E_POINTER;
    *out=&view.iface;
    view_add(&view.iface);
    created++;
    printf("V56_CREATE_VIEW=%ld\n",created);
    return S_OK;
}
static IFrameworkViewSourceVtbl src_vtbl={
    src_qi,src_add,src_release,
    (void *)inspect_getiids,(void *)inspect_getclass,(void *)inspect_gettrust,
    src_create
};
static HRESULT expected_hr(void)
{
    if(matches("initialize_fail"))return E_FAIL;
    if(matches("setwindow_fail"))return E_ACCESSDENIED;
    if(matches("load_fail"))return E_INVALIDARG;
    if(matches("run_fail")||matches("run_uninit_fail"))return E_UNEXPECTED;
    if(matches("uninit_fail"))return E_ABORT;
    if(matches("complete"))return S_OK;
    return E_NOTIMPL;
}
int main(int argc,char **argv)
{
    IActivationFactory *factory=NULL;
    ICoreApplicationView *postview=NULL;
    HSTRING runtime=NULL;
    HRESULT hr;
    ULONG refs;
    if(argc!=2)return 2;
    scenario=argv[1];
    owner_thread=GetCurrentThreadId();
    printf("V56_MOCK_REVISION=full-frameworkview-lifecycle-v1\n");
    printf("V56_SCENARIO=%s\n",scenario);
    source.iface.lpVtbl=&src_vtbl;
    view.iface.lpVtbl=&view_vtbl;
    hr=RoInitialize(RO_INIT_MULTITHREADED);
    if(FAILED(hr))return 3;
    hr=WindowsCreateString(L"Windows.ApplicationModel.Core.CoreApplication",
        (UINT32)lstrlenW(L"Windows.ApplicationModel.Core.CoreApplication"),&runtime);
    if(SUCCEEDED(hr))
        hr=RoGetActivationFactory(runtime,&IID_IActivationFactory,(void **)&factory);
    check(hr==S_OK && factory,"GET_COREAPP_FACTORY");
    if(SUCCEEDED(hr))
        hr=IActivationFactory_QueryInterface(factory,&IID_ICoreApplication,(void **)&core);
    check(hr==S_OK && core,"GET_COREAPP");
    if(!core){errors++;goto done;}
    hr=ICoreApplication_GetCurrentView(core,&postview);
    check(hr==E_NOTIMPL && !postview,"BEFORE_RUN_NO_CURRENT_VIEW");
    if(postview){ICoreApplicationView_Release(postview);postview=NULL;}
    hr=ICoreApplication_Run(core,&source.iface);
    printf("V56_COREAPP_RUN_HR=0x%08lx\n",(unsigned long)hr);
    check(hr==expected_hr(),"EXPECTED_ORIGINAL_HRESULT");
    hr=ICoreApplication_GetCurrentView(core,&postview);
    check(hr==E_NOTIMPL && !postview,"AFTER_RUN_NO_CURRENT_VIEW");
    if(postview){ICoreApplicationView_Release(postview);postview=NULL;}
    {
        ICoreWindowStatic *statics=NULL;
        ICoreWindow *live=NULL;
        HSTRING cls=NULL;
        HRESULT whr=WindowsCreateString(L"Windows.UI.Core.CoreWindow",26,&cls);
        if(SUCCEEDED(whr))
            whr=RoGetActivationFactory(cls,&IID_ICoreWindowStatic,(void **)&statics);
        if(statics)
        {
            whr=ICoreWindowStatic_GetForCurrentThread(statics,&live);
            check(whr==S_OK && !live,"AFTER_RUN_COREWINDOW_CLEARED");
            if(live)ICoreWindow_Release(live);
            ICoreWindowStatic_Release(statics);
        }
        if(cls)WindowsDeleteString(cls);
    }
    check(created==1 && query_view==1,"SOURCE_AND_VIEW_QI_COUNTS");
    if(matches("initialize_fail")||matches("missing_window_flag"))
        check(initialized==(matches("initialize_fail")?1:0) && !set_window && !loaded && !ran &&
              !uninitialized,"EARLY_FAILURE_SEQUENCE");
    else if(matches("setwindow_fail"))
        check(initialized==1 && set_window==1 && loaded==0 && ran==0 &&
              uninitialized==1,"SETWINDOW_FAILURE_SEQUENCE");
    else if(matches("load_fail"))
        check(initialized==1 && set_window==1 && loaded==1 && ran==0 &&
              uninitialized==1,"LOAD_FAILURE_SEQUENCE");
    else if(matches("v54_fallback"))
        check(initialized==1 && !set_window && !loaded && !ran &&
              uninitialized==1,"V54_FALLBACK_SEQUENCE");
    else if(matches("v47_disabled"))
        check(initialized==0 && !set_window && !loaded && !ran &&
              uninitialized==0,"V47_FLAG_OFF_SEQUENCE");
    else
        check(initialized==1 && set_window==1 && loaded==1 && ran==1 &&
              uninitialized==1,"FULL_LIFECYCLE_METHOD_COUNT");
done:
    printf("V56_COUNTS=%ld,%ld,%ld,%ld,%ld,%ld,%ld\n",
       created,query_view,initialized,set_window,loaded,ran,uninitialized);
    refs=src_refs;
    printf("V56_SOURCE_REFS=%lu VIEW_REFS=%ld\n",refs,view_refs);
    check(src_refs==1 && view_refs==1,"COM_SOURCE_AND_VIEW_BALANCED");
    if(core)ICoreApplication_Release(core);
    if(factory)IActivationFactory_Release(factory);
    if(runtime)WindowsDeleteString(runtime);
    RoUninitialize();
    printf("V56_MOCK_STATUS=%s\n",errors?"FAIL":"PASS");
    return errors?1:0;
}
