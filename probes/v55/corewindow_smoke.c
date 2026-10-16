#define INITGUID
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>
#include <roapi.h>
#include <winstring.h>
#define WIDL_using_Windows_Foundation
#include <windows.foundation.h>
#define WIDL_using_Windows_UI
#include <windows.ui.h>
#define WIDL_using_Windows_UI_Core
#include <windows.ui.core.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef HRESULT (WINAPI *v55_create_fn)(ICoreWindow **window,HWND *hwnd);

struct worker_data {
    ICoreDispatcher *dispatcher;
    ICoreWindow *window;
    HWND hwnd;
    HRESULT access_hr;
    HRESULT close_hr;
    HRESULT dispatcher_hr;
    HRESULT pump_hr;
    boolean has_access;
};
static DWORD WINAPI wrong_thread(LPVOID data)
{
    struct worker_data *worker=data;
    worker->has_access=2;
    worker->access_hr=ICoreDispatcher_get_HasThreadAccess(worker->dispatcher,
        &worker->has_access);
    {
        ICoreDispatcher *different=NULL;
        worker->dispatcher_hr=ICoreWindow_get_Dispatcher(worker->window,&different);
        if(different) ICoreDispatcher_Release(different);
    }
    worker->pump_hr=ICoreDispatcher_ProcessEvents(worker->dispatcher,
        CoreProcessEventsOption_ProcessOneIfPresent);
    worker->close_hr=ICoreWindow_Close(worker->window);
    return 0;
}
static DWORD WINAPI send_close(LPVOID data)
{
    struct worker_data *worker=data;
    Sleep(110);
    if (!PostMessageW(worker->hwnd,WM_CLOSE,0,0))
        return GetLastError();
    return 0;
}
static int create_corewindow_factory(ICoreWindowStatic **out)
{
    HSTRING name=NULL;
    HRESULT hr;
    const WCHAR str[]=L"Windows.UI.Core.CoreWindow";
    *out=NULL;
    hr=WindowsCreateString(str,(UINT32)lstrlenW(str),&name);
    if (SUCCEEDED(hr))
        hr=RoGetActivationFactory(name,&IID_ICoreWindowStatic,(void **)out);
    if (name) WindowsDeleteString(name);
    printf("V55_FACTORY_HR=0x%08lx\n",(unsigned long)hr);
    return SUCCEEDED(hr);
}
static void require_(int cond,const char *name,int *errors)
{
    printf("V55_CHECK_%s=%s\n",name,cond?"PASS":"FAIL");
    if (!cond) ++*errors;
}
int main(int argc,char **argv)
{
    HRESULT hr;
    int failures=0;
    ICoreWindowStatic *factory=NULL;
    ICoreWindow *window=NULL,*same=NULL;
    ICoreDispatcher *dispatcher=NULL;
    IUnknown *iwin=NULL,*idisp=NULL;
    HMODULE library=NULL;
    v55_create_fn create;
    HWND hwnd=NULL,untouched=(HWND)(uintptr_t)0xcafe;
    boolean owner=99,visible=99;
    ULONG final_ref=123;
    DWORD thread_exit=999;
    const char *mode;
    printf("V55_PROBE_REVISION=corewindow-real-hwnd-v1\n");
    if (argc!=2) return 2;
    mode=argv[1];
    printf("V55_MODE=%s\n",mode);
    hr=RoInitialize(RO_INIT_MULTITHREADED);
    printf("V55_ROINIT_HR=0x%08lx\n",(unsigned long)hr);
    if (FAILED(hr)) return 3;
    library=LoadLibraryW(L"windows.ui.dll");
    if (!library) return 4;
    create=(v55_create_fn)(void *)GetProcAddress(library,"WineCoreWindowV55_Create");
    require_(create!=NULL,"PRIVATE_EXPORT_FOUND",&failures);
    require_(create_corewindow_factory(&factory),"STATIC_FACTORY",&failures);
    if(!create || !factory){failures++;goto done;}
    hr=ICoreWindowStatic_GetForCurrentThread(factory,&same);
    printf("V55_BEFORE_GETCURRENT_HR=0x%08lx NULL=%d\n",(unsigned long)hr,same==NULL);
    require_(hr==S_OK && !same,"BEFORE_CURRENT_NULL",&failures);
    if(same){ICoreWindow_Release(same);same=NULL;}
    if(!strcmp(mode,"disabled")){
        window=(ICoreWindow *)(uintptr_t)0x1234;
        hr=create(&window,&untouched);
        printf("V55_DISABLED_CREATE_HR=0x%08lx\n",(unsigned long)hr);
        require_(hr==E_NOTIMPL && !window && !untouched,"DISABLED_NO_WINDOW",&failures);
        goto done;
    }
    if(strcmp(mode,"basic") && strcmp(mode,"pump") && strcmp(mode,"until_quit") &&
       strcmp(mode,"wrong_thread")){
        failures++;goto done;
    }
    hr=create(NULL,&untouched);
    require_(hr==E_POINTER,"NULL_WINDOW_OUT_REJECTED",&failures);
    hr=create(&window,NULL);
    require_(hr==E_POINTER,"NULL_HWND_OUT_REJECTED",&failures);
    require_(!window,"NULL_HWND_OUT_ZEROES_WINDOW",&failures);
    hr=create(&window,&hwnd);
    printf("V55_CREATE_HR=0x%08lx HWND=%p\n",(unsigned long)hr,hwnd);
    require_(hr==S_OK && window && hwnd && IsWindow(hwnd),"REAL_HWND_CREATED",&failures);
    if(FAILED(hr) || !window || !hwnd)goto done;
    hr=ICoreWindowStatic_GetForCurrentThread(factory,&same);
    require_(hr==S_OK && same==window,"THREAD_LOCAL_COM_IDENTITY",&failures);
    if(same){ICoreWindow_Release(same);same=NULL;}
    hr=ICoreWindow_get_Dispatcher(window,&dispatcher);
    printf("V55_GET_DISPATCHER_HR=0x%08lx\n",(unsigned long)hr);
    require_(hr==S_OK && dispatcher!=NULL,"REAL_DISPATCHER",&failures);
    if(!dispatcher)goto close;
    hr=ICoreDispatcher_get_HasThreadAccess(dispatcher,&owner);
    require_(hr==S_OK && owner==TRUE,"DISPATCHER_OWNER_ACCESS",&failures);
    hr=ICoreWindow_get_Visible(window,&visible);
    require_(hr==S_OK && visible==FALSE,"HIDDEN_WINDOW_NOT_SHOWN",&failures);
    hr=ICoreWindow_QueryInterface(window,&IID_IUnknown,(void **)&iwin);
    require_(hr==S_OK && iwin,"WINDOW_IUNKNOWN",&failures);
    hr=ICoreDispatcher_QueryInterface(dispatcher,&IID_IUnknown,(void **)&idisp);
    require_(hr==S_OK && idisp==iwin,"SHARED_IUNKNOWN_IDENTITY",&failures);
    if (iwin) IUnknown_Release(iwin);
    if (idisp) IUnknown_Release(idisp);
    iwin=idisp=NULL;
    hr=ICoreWindow_Activate(window);
    require_(hr==E_NOTIMPL,"NO_FAKE_ACTIVATION",&failures);
    hr=ICoreWindow_put_IsInputEnabled(window,FALSE);
    require_(hr==E_NOTIMPL,"NO_FAKE_INPUT_SUPPRESSION",&failures);
    {
        boolean enabled=77;
        hr=ICoreWindow_get_IsInputEnabled(window,&enabled);
        require_(hr==E_NOTIMPL,"INPUT_GET_UNIMPLEMENTED",&failures);
    }
    {
        IAsyncAction *action=(IAsyncAction*)(uintptr_t)0x1234;
        hr=ICoreDispatcher_RunAsync(dispatcher,CoreDispatcherPriority_Normal,NULL,&action);
        require_(hr==E_NOTIMPL && !action,"ASYNC_NOT_FAKED",&failures);
    }
    if(!strcmp(mode,"basic")){
        hr=create(&same,&untouched);
        require_(hr==HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) && !same,
            "DUPLICATE_WINDOW_REJECTED",&failures);
    }
    if(!strcmp(mode,"pump")){
        MSG msg;
        hr=ICoreDispatcher_ProcessEvents(dispatcher,
            (CoreProcessEventsOption)99);
        require_(hr==E_INVALIDARG,"INVALID_DISPATCH_OPTION",&failures);
        hr=ICoreDispatcher_ProcessEvents(dispatcher,
            CoreProcessEventsOption_ProcessOneIfPresent);
        require_(hr==S_OK,"NONBLOCKING_PUMP",&failures);
        require_(PostMessageW(hwnd,WM_APP+55,0,0)!=FALSE,
            "POST_WINDOW_MESSAGE",&failures);
        hr=ICoreDispatcher_ProcessEvents(dispatcher,
            CoreProcessEventsOption_ProcessAllIfPresent);
        require_(hr==S_OK,"DISPATCH_ALL_PENDING",&failures);
        require_(!PeekMessageW(&msg,hwnd,WM_APP+55,WM_APP+55,PM_NOREMOVE),
            "MESSAGE_REMOVED",&failures);
    }
    if(!strcmp(mode,"wrong_thread")){
        struct worker_data work={0};
        HANDLE thread;
        work.dispatcher=dispatcher;work.window=window;
        thread=CreateThread(NULL,0,wrong_thread,&work,0,NULL);
        require_(thread!=NULL,"WORKER_CREATED",&failures);
        if(thread){
            if(WaitForSingleObject(thread,12000)!=WAIT_OBJECT_0)failures++;
            GetExitCodeThread(thread,&thread_exit);
            CloseHandle(thread);
        }
        require_(work.access_hr==S_OK && work.has_access==FALSE,
          "WRONG_THREAD_NOT_OWNER",&failures);
        require_(work.dispatcher_hr==RPC_E_WRONG_THREAD,
          "WRONG_THREAD_CANNOT_GET_DISPATCHER",&failures);
        require_(work.pump_hr==RPC_E_WRONG_THREAD,
          "WRONG_THREAD_CANNOT_PUMP",&failures);
        require_(work.close_hr==RPC_E_WRONG_THREAD && IsWindow(hwnd),
          "WRONG_THREAD_CANNOT_CLOSE",&failures);
    }
    if(!strcmp(mode,"until_quit")){
        struct worker_data work={0};
        HANDLE thread;
        work.hwnd=hwnd;
        thread=CreateThread(NULL,0,send_close,&work,0,NULL);
        require_(thread!=NULL,"CLOSE_THREAD_CREATED",&failures);
        if(thread){
            hr=ICoreDispatcher_ProcessEvents(dispatcher,
                CoreProcessEventsOption_ProcessUntilQuit);
            printf("V55_DISPATCH_UNTIL_QUIT_HR=0x%08lx\n",(unsigned long)hr);
            require_(hr==S_OK && !IsWindow(hwnd),
                "DISPATCH_UNTIL_QUIT_STOPPED",&failures);
            if(WaitForSingleObject(thread,12000)!=WAIT_OBJECT_0)failures++;
            GetExitCodeThread(thread,&thread_exit);
            require_(thread_exit==0,"CLOSE_MESSAGE_POSTED",&failures);
            CloseHandle(thread);
        }
    }
close:
    if(IsWindow(hwnd)){
        hr=ICoreWindow_Close(window);
        printf("V55_CLOSE_HR=0x%08lx\n",(unsigned long)hr);
        require_(hr==S_OK && !IsWindow(hwnd),"CLOSE_DESTROYED_HWND",&failures);
    }
    hr=ICoreWindowStatic_GetForCurrentThread(factory,&same);
    require_(hr==S_OK && !same,"CURRENT_CLEARED_ON_DESTROY",&failures);
    if(same) ICoreWindow_Release(same);
    if(dispatcher){
        final_ref=ICoreDispatcher_Release(dispatcher);
        printf("V55_DISPATCHER_RELEASE_REF=%lu\n",final_ref);
        dispatcher=NULL;
    }
done:
    if(window){
        final_ref=ICoreWindow_Release(window);
        printf("V55_WINDOW_FINAL_RELEASE_REF=%lu\n",final_ref);
        require_(final_ref==0,"COM_LIFETIME_COMPLETE",&failures);
    }
    if(factory) ICoreWindowStatic_Release(factory);
    if(library) FreeLibrary(library);
    RoUninitialize();
    printf("V55_PROBE_STATUS=%s\n",failures?"FAIL":"PASS");
    return failures?1:0;
}
