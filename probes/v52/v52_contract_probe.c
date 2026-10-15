/*
 * v52 read-only WinRT contract probe for a user's authorized Windows host.
 * This source is a proposed test, NOT a completed native execution.
 * No Shapr3D DLLs are loaded and Windows settings are not changed.
 *
 * Build on the existing isolated Wine WSL machine:
 * x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror -Wno-cast-function-type \
 *   -Wl,--no-insert-timestamp -o v52_contract_probe.exe v52_contract_probe.c
 *
 * Run directly on Windows, in a bounded process:
 *   v52_contract_probe.exe corewindow sta
 *   v52_contract_probe.exe corewindow mta
 *   v52_contract_probe.exe accessibility sta
 *   v52_contract_probe.exe accessibility mta
 *
 * ABI reconstructed from the Wine 11.19 generated WinRT headers:
 *   ICoreWindowStatic IID: 4d239005-3c2a-41b1-9022-536bb9cf93b1
 *   IAccessibilitySettings IID: fe0e8147-c4c0-4562-b962-1327b52ad5b9
 *   ITypedEventHandler<AccessibilitySettings,IInspectable> IID:
 *       f5917e6f-5abf-5e65-b5b4-1b9c8d94e788
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* Keep signatures independent of generated WinRT C header name differences. */
typedef void *V52HSTRING;
typedef struct V52Token { int64_t value; } V52Token;
typedef HRESULT (WINAPI *V52RoInitialize)(int);
typedef void (WINAPI *V52RoUninitialize)(void);
typedef HRESULT (WINAPI *V52CreateString)(const wchar_t *, unsigned, V52HSTRING *);
typedef HRESULT (WINAPI *V52DeleteString)(V52HSTRING);
typedef HRESULT (WINAPI *V52GetFactory)(V52HSTRING, const GUID *, void **);
typedef HRESULT (WINAPI *V52Activate)(V52HSTRING, void **);
typedef HRESULT (WINAPI *V52CoreGetCurrentThread)(void *, void **);
typedef HRESULT (WINAPI *V52GetHighContrast)(void *, unsigned char *);
typedef HRESULT (WINAPI *V52AddHighContrast)(void *, void *, V52Token *);
typedef HRESULT (WINAPI *V52RemoveHighContrast)(void *, V52Token);

typedef struct V52UnknownVtbl {
    HRESULT (WINAPI *QueryInterface)(void *, const GUID *, void **);
    ULONG (WINAPI *AddRef)(void *);
    ULONG (WINAPI *Release)(void *);
} V52UnknownVtbl;

static const GUID V52IID_IUnknown =
    {0x00000000,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID V52IID_ICoreWindowStatic =
    {0x4d239005,0x3c2a,0x41b1,{0x90,0x22,0x53,0x6b,0xb9,0xcf,0x93,0xb1}};
static const GUID V52IID_IAccessibilitySettings =
    {0xfe0e8147,0xc4c0,0x4562,{0xb9,0x62,0x13,0x27,0xb5,0x2a,0xd5,0xb9}};
static const GUID V52IID_HighContrastHandler =
    {0xf5917e6f,0x5abf,0x5e65,{0xb5,0xb4,0x1b,0x9c,0x8d,0x94,0xe7,0x88}};

static int v52_equal_guid(const GUID *a, const GUID *b)
{
    return memcmp(a,b,sizeof(GUID))==0;
}
static V52UnknownVtbl *v52_unknown_methods(void *iface)
{
    return *(V52UnknownVtbl **)iface;
}
static void v52_release(void *iface)
{
    if (iface) v52_unknown_methods(iface)->Release(iface);
}
static HRESULT v52_qi(void *iface, const GUID *iid, void **out)
{
    *out = NULL;
    return v52_unknown_methods(iface)->QueryInterface(iface,iid,out);
}
static void **v52_vtable(void *iface) { return *(void ***)iface; }
static void v52_hr(const char *label, HRESULT value)
{
    printf("V52_%s=0x%08x\n",label,(unsigned)(uint32_t)value);
}

typedef struct V52Handler {
    struct V52HandlerVtbl const *lpVtbl;
    LONG refs;
    LONG adds;
    LONG releases;
    LONG invoked;
} V52Handler;
typedef struct V52HandlerVtbl {
    HRESULT (WINAPI *QueryInterface)(V52Handler *, const GUID *, void **);
    ULONG (WINAPI *AddRef)(V52Handler *);
    ULONG (WINAPI *Release)(V52Handler *);
    HRESULT (WINAPI *Invoke)(V52Handler *, void *, void *);
} V52HandlerVtbl;

static ULONG WINAPI v52_handler_addref(V52Handler *self)
{
    InterlockedIncrement(&self->adds);
    return (ULONG)InterlockedIncrement(&self->refs);
}
static ULONG WINAPI v52_handler_release(V52Handler *self)
{
    InterlockedIncrement(&self->releases);
    return (ULONG)InterlockedDecrement(&self->refs);
}
static HRESULT WINAPI v52_handler_qi(V52Handler *self, const GUID *iid, void **out)
{
    if (!out) return E_POINTER;
    *out=NULL;
    if (!v52_equal_guid(iid,&V52IID_IUnknown) &&
        !v52_equal_guid(iid,&V52IID_HighContrastHandler)) return E_NOINTERFACE;
    v52_handler_addref(self);
    *out=self;
    return S_OK;
}
static HRESULT WINAPI v52_handler_invoke(V52Handler *self, void *sender, void *args)
{
    (void)sender; (void)args;
    InterlockedIncrement(&self->invoked);
    return S_OK;
}
static const V52HandlerVtbl handler_vtbl =
    {v52_handler_qi,v52_handler_addref,v52_handler_release,v52_handler_invoke};

typedef struct V52API {
    HMODULE combase;
    V52RoInitialize initialize;
    V52RoUninitialize uninitialize;
    V52CreateString create;
    V52DeleteString destroy;
    V52GetFactory factory;
    V52Activate activate;
} V52API;
static int v52_api_init(V52API *api)
{
    memset(api,0,sizeof(*api));
    api->combase=LoadLibraryW(L"combase.dll");
    if (!api->combase) return 0;
    api->initialize=(V52RoInitialize)GetProcAddress(api->combase,"RoInitialize");
    api->uninitialize=(V52RoUninitialize)GetProcAddress(api->combase,"RoUninitialize");
    api->create=(V52CreateString)GetProcAddress(api->combase,"WindowsCreateString");
    api->destroy=(V52DeleteString)GetProcAddress(api->combase,"WindowsDeleteString");
    api->factory=(V52GetFactory)GetProcAddress(api->combase,"RoGetActivationFactory");
    api->activate=(V52Activate)GetProcAddress(api->combase,"RoActivateInstance");
    return api->initialize && api->uninitialize && api->create &&
           api->destroy && api->factory && api->activate;
}
static HRESULT v52_create_class(V52API *api,const wchar_t *name,V52HSTRING *out)
{
    *out=NULL;
    return api->create(name,(unsigned)wcslen(name),out);
}

static int v52_corewindow(V52API *api)
{
    void *statics=NULL, *window=(void *)(uintptr_t)0x1234;
    V52HSTRING name=NULL;
    HRESULT hr;
    hr=v52_create_class(api,L"Windows.UI.Core.CoreWindow",&name);
    v52_hr("COREWINDOW_HSTRING_HR",hr);
    if (FAILED(hr)) return 10;
    hr=api->factory(name,&V52IID_ICoreWindowStatic,&statics);
    v52_hr("COREWINDOW_FACTORY_HR",hr);
    api->destroy(name);
    if (FAILED(hr) || !statics) return 11;
    hr=((V52CoreGetCurrentThread)v52_vtable(statics)[6])(statics,&window);
    v52_hr("COREWINDOW_GET_CURRENT_HR",hr);
    printf("V52_COREWINDOW_GET_CURRENT_NULL=%d\n",window==NULL);
    if (SUCCEEDED(hr) && window && window!=(void *)(uintptr_t)0x1234) v52_release(window);
    v52_release(statics);
    printf("V52_COREWINDOW_PROBE_DONE=1\n");
    return 0;
}
static int v52_accessibility(V52API *api)
{
    V52HSTRING name=NULL;
    void *raw=NULL,*settings=NULL;
    V52Handler handler={&handler_vtbl,1,0,0,0};
    V52Token t1={INT64_C(0x1122334455667788)}, t2={INT64_C(0x1122334455667788)};
    unsigned char enabled=0xff;
    HRESULT hr,hr2;
    hr=v52_create_class(api,L"Windows.UI.ViewManagement.AccessibilitySettings",&name);
    v52_hr("ACCESSIBILITY_HSTRING_HR",hr);
    if (FAILED(hr)) return 12;
    hr=api->activate(name,&raw);
    v52_hr("ACCESSIBILITY_ACTIVATE_HR",hr);
    api->destroy(name);
    if (FAILED(hr) || !raw) return 13;
    hr=v52_qi(raw,&V52IID_IAccessibilitySettings,&settings);
    v52_hr("ACCESSIBILITY_QI_HR",hr);
    v52_release(raw);
    if (FAILED(hr) || !settings) return 14;
    hr=((V52GetHighContrast)v52_vtable(settings)[6])(settings,&enabled);
    v52_hr("ACCESSIBILITY_GET_HIGHCONTRAST_HR",hr);
    printf("V52_ACCESSIBILITY_GET_HIGHCONTRAST_VALUE=%u\n",(unsigned)enabled);
    /* First, measure the native behavior for a NULL handler. The token is
     * initialized with a visible sentinel to establish output semantics. */
    hr=((V52AddHighContrast)v52_vtable(settings)[8])(settings,NULL,&t1);
    v52_hr("ACCESSIBILITY_ADD_NULL_HANDLER_HR",hr);
    printf("V52_ACCESSIBILITY_ADD_NULL_HANDLER_TOKEN=0x%016llx\n",
           (unsigned long long)(uint64_t)t1.value);
    /* Register a correct typed WinRT delegate; no system contrast is changed. */
    t1.value=INT64_C(0x1122334455667788);
    hr=((V52AddHighContrast)v52_vtable(settings)[8])(settings,&handler,&t1);
    v52_hr("ACCESSIBILITY_ADD1_HR",hr);
    printf("V52_ACCESSIBILITY_ADD1_TOKEN=0x%016llx\n",
           (unsigned long long)(uint64_t)t1.value);
    printf("V52_HANDLER_REFS_AFTER_ADD1=%ld\n",(long)handler.refs);
    if (SUCCEEDED(hr)) {
        hr2=((V52AddHighContrast)v52_vtable(settings)[8])(settings,&handler,&t2);
        v52_hr("ACCESSIBILITY_ADD2_HR",hr2);
        printf("V52_ACCESSIBILITY_ADD2_TOKEN=0x%016llx\n",
               (unsigned long long)(uint64_t)t2.value);
        if (SUCCEEDED(hr2)) {
            HRESULT removed=((V52RemoveHighContrast)v52_vtable(settings)[9])(settings,t2);
            v52_hr("ACCESSIBILITY_REMOVE2_HR",removed);
        }
        hr2=((V52RemoveHighContrast)v52_vtable(settings)[9])(settings,t1);
        v52_hr("ACCESSIBILITY_REMOVE1_HR",hr2);
    }
    printf("V52_HANDLER_REFS_FINAL=%ld\n",(long)handler.refs);
    printf("V52_HANDLER_ADDS=%ld\n",(long)handler.adds);
    printf("V52_HANDLER_RELEASES=%ld\n",(long)handler.releases);
    printf("V52_HANDLER_INVOKED=%ld\n",(long)handler.invoked);
    v52_release(settings);
    printf("V52_ACCESSIBILITY_PROBE_DONE=1\n");
    return 0;
}
int main(int argc,char **argv)
{
    V52API api;
    HRESULT hr;
    int apartment,code;
    if (argc!=3 || (strcmp(argv[1],"corewindow") &&
        strcmp(argv[1],"accessibility")) ||
        (strcmp(argv[2],"sta") && strcmp(argv[2],"mta"))) {
        fprintf(stderr,"usage: v52_contract_probe.exe {corewindow|accessibility} {sta|mta}\n");
        return 2;
    }
    printf("V52_PROBE_VERSION=52\n");
    printf("V52_PROBE_NAME=%s\n",argv[1]);
    printf("V52_PROBE_APARTMENT=%s\n",argv[2]);
    if (!v52_api_init(&api)) {
        printf("V52_WINRT_EXPORTS_AVAILABLE=0\n");
        return 3;
    }
    apartment=strcmp(argv[2],"sta")==0?0:1;
    hr=api.initialize(apartment);
    v52_hr("ROINITIALIZE_HR",hr);
    if (FAILED(hr)) return 4;
    code=strcmp(argv[1],"corewindow")==0 ?
         v52_corewindow(&api) : v52_accessibility(&api);
    api.uninitialize();
    printf("V52_PROBE_EXIT=%d\n",code);
    return code;
}
